/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#include <fstream>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/ptrace.h>
#include <arpa/inet.h>

#include "macros.h"
#include "ptrace_wrapper.h"

union union_intptr_t {
    intptr_t as_intptr;
    char     as_chars[PT_SIZE];
};

PtraceWrapper::PtraceWrapper()
: _pid(0)
, _isZygote(false) {
}

/*
 * Q:  why PTRACE_SYSCALL after PTRACE_ATTACH?
 * 
 * A:  if PTRACE_ATTACH on a tracee already hangup by system call, it will force the current
 *     system call interrupt and return immediately. meanwhile, the system modify tracee's
 *     $arch_x86(EIP - 2)/$arch_arm(PC - 4) so when our PTRACE_ATTACH being processed, it would
 *     call $arch_x86(INT 0x80)/$arch_arm(SVC 0) instruction to restart the original
 *     interrupted system call. (hint: bionic/libc.so is compiled by ARM, not Thumb.)
 *
 *     so when PTRACE_SYSCALL after PTRACE_ATTACH, PTRACE_SYSCALL would only set the
 *     corresponding flag on tracee process, rather than raising signal or interruption.
 *     and the tracee will hangup and return when it encounters the next system call.
 * 
 *     Reference:
 *     https://elixir.bootlin.com/linux/latest/source/arch/arm/kernel/signal.c#L589
 *     https://elixir.bootlin.com/linux/latest/source/arch/arm64/kernel/signal.c#L851
 *     https://elixir.bootlin.com/linux/latest/source/arch/x86/kernel/signal.c#L732
 */
bool PtraceWrapper::attach(pid_t pid) {
    // just in case
    errno = 0;
    bool ok = true;
    do {
        // check attached already
        BREAK_IF_WITH_LOGE(this->_pid, "PtraceWrapper::attach already attached to pid %d\n", this->_pid);
        this->_pid = pid;

        // check file accessable
        char cmdline[0x100];
        ::sprintf(cmdline, "/proc/%d/cmdline", pid);
        std::string content;
        std::fstream read(cmdline);
        if (read.is_open()) {
            std::getline(read, content);
            read.close();
            // furthermore, is it a zygote process?
            this->_isZygote = (::strcmp(content.c_str(), "zygote") == 0 || ::strcmp(content.c_str(), "zygote64") == 0);
        } else {
            LOGGER_LOGE("PtraceWrapper::attach failed to open file %s: %s\n", cmdline, ::strerror(errno));
        }

        // do attach
        ok &= (::ptrace(PTRACE_ATTACH, this->_pid, nullptr, 0) != -1);
        BREAK_IF_WITH_LOGE(!ok, "PtraceWrapper::attach attach to process %d failed: %s\n", this->_pid, ::strerror(errno));
        ok &= this->waitForSignals({ SIGTRAP, SIGSTOP });
        BREAK_IF_WITH_LOGE(!ok, "PtraceWrapper::attach wait for SIGTRAP/SIGSTOP failed: %s\n", ::strerror(errno));
        
        // wait for syscall enter
        ok &= (::ptrace(PTRACE_SYSCALL, this->_pid, nullptr, 0) != -1);
        BREAK_IF_WITH_LOGE(!ok, "PtraceWrapper::attach enter syscall failed: %s\n", ::strerror(errno));
        ok &= this->waitForSignals({ SIGTRAP, SIGSTOP });
        BREAK_IF_WITH_LOGE(!ok, "PtraceWrapper::attach wait for SIGTRAP/SIGSTOP(enter) failed: %s\n", ::strerror(errno));
        
        // a workaround for zygote
        if (this->_isZygote) {
            ::sleep(2);
            this->_connectToZygote();
        }
        
        // wait for syscall exit
        ok &= (::ptrace(PTRACE_SYSCALL, this->_pid, nullptr, 0) != -1);
        BREAK_IF_WITH_LOGE(!ok, "PtraceWrapper::attach exit syscall failed: %s\n", ::strerror(errno));
        ok &= this->waitForSignals({ SIGTRAP, SIGSTOP });
        BREAK_IF_WITH_LOGE(!ok, "PtraceWrapper::attach wait for SIGTRAP/SIGSTOP(exit) failed: %s\n", ::strerror(errno));
    }
    while (false);

    if (!ok) {
        // bailout
        this->_pid = 0;
    }

    return ok;
}

bool PtraceWrapper::detach() {
    bool ok = false;
    if (this->_pid) {
        ok = (::ptrace(PTRACE_DETACH, this->_pid, nullptr, 0) != -1);
        if (!ok) {
            LOGGER_LOGE("PtraceWrapper::detach failed: %s\n", ::strerror(errno));
        }
    }
    return ok;
}

bool PtraceWrapper::kontinue() {
    bool ok = false;
    if (this->_pid) {
        ok = (::ptrace(PTRACE_CONT, this->_pid, nullptr, 0) != -1);
        if (!ok) {
            LOGGER_LOGE("PtraceWrapper::kontinue failed: %s\n", ::strerror(errno));
        }
    }
    return ok;
}

bool PtraceWrapper::waitForSignal(int signal) {
    return this->waitForSignals({ signal });
}

bool PtraceWrapper::waitForSignals(const std::vector<int>& signals) {
    int status = 0;
    bool ret = false;
    bool kontinue = true;
    while (this->_pid && kontinue) {
        errno = 0;
        if (::waitpid(this->_pid, &status, WUNTRACED) == _pid) {
            // check exit signal
            if (WIFEXITED(status)) {
                LOGGER_LOGE("PtraceWrapper::waitForSignals process %d has exited\n", _pid);
                break;
            }
            // check stopped signal
            if (WIFSTOPPED(status)) {
                for (int signal : signals) {
                    if (signal == 0 || WSTOPSIG(status) == signal) {
                        ret = true;
                        kontinue = false;
                        break;
                    }
                }
                if (ret == false) {
                    LOGGER_LOGE("PtraceWrapper::waitForSignals process stopped by unexcepted signal %d.\n", WSTOPSIG(status));
                }
            }
        } else {
            // retry syscalls that can return EINTR
            if (errno != EINTR) {
                LOGGER_LOGE("PtraceWrapper::waitForSignals waitpid error: %s\n", ::strerror(errno));
                break;
            }
        }
    }
    return ret;
}

bool PtraceWrapper::readText(void* dest, const void* src, size_t count) {
    return this->_readInternal(PTRACE_PEEKTEXT, dest, src, count);
}

bool PtraceWrapper::readData(void* dest, const void* src, size_t count) {
    return this->_readInternal(PTRACE_PEEKDATA, dest, src, count);
}

bool PtraceWrapper::writeText(const void* dest, const void* src, size_t count) {
    return this->_writeInternal(PTRACE_POKETEXT, dest, src, count);
}

bool PtraceWrapper::writeData(const void* dest, const void* src, size_t count) {
    return this->_writeInternal(PTRACE_POKEDATA, dest, src, count);
}

bool PtraceWrapper::_connectToZygote() {
    bool ret = false;
    // usually, zygote has little changes to encounter a system call.
    // so if we dont do anything, it might cost you a long time(really, A LONG TIME)
    // to wait until zygote perform a system call and captured by ptrace.
    // the following code performs a hit-and-run to force zygote to trigger system calls
    // by connecting to its abstract TCP servers, which are used to handle requests
    // from ActivityManagerService.
    for (const std::string& abstractName : { "/dev/socket/zygote_secondary", "/dev/socket/zygote" }) {
        int sock = -1;
        if ((sock = ::socket(AF_UNIX, SOCK_STREAM, 0)) != -1) {
            int opt = 1;
            // set non-blocking
            if (::ioctl(sock, FIONBIO, &opt) == 0) {
                struct sockaddr_un remote;
                remote.sun_family = AF_UNIX;
                ::strcpy(remote.sun_path, abstractName.c_str());
                size_t len = ::strlen(remote.sun_path) + sizeof(remote.sun_family);
                if (::connect(sock, (struct sockaddr*)&remote, len) == 0) {
                    ::close(sock);
                    ret = true;
                    // don't break here.
                    // each zygote process needs to be waked up.
                    // break;
                }
            }
            ::close(sock);
        }
    }
    return ret;
}

bool PtraceWrapper::_readInternal(int peakAction, void* dest, const void* src, size_t count) {
    if (!this->_pid) {
        return false;
    }

    errno = 0;
    bool succ = true;
    size_t p = 0;
    size_t c = count / PT_SIZE;
    size_t r = count % PT_SIZE;
    uint8_t* destBytes = static_cast<uint8_t*>(dest);

    union_intptr_t un;
    for (size_t i = 0; i < c; ++ i) {
        un.as_intptr = ::ptrace(peakAction, this->_pid, (const uint8_t*)src + i * PT_SIZE, 0);
        if (un.as_intptr == -1) {
            LOGGER_LOGE("PtraceWrapper::readInternal action of %d failed at 0x%zx: %s\n", peakAction, uintptr_t((const uint8_t*)src + i * PT_SIZE), ::strerror(errno));
            succ = false;
            break;
        }
        ::memcpy(destBytes + i * PT_SIZE, un.as_chars, PT_SIZE);
        p += PT_SIZE;
    }
    if (succ && r > 0) {
        un.as_intptr = ::ptrace(peakAction, this->_pid, (const uint8_t*)src + p, 0);
        if (un.as_intptr == -1) {
            succ = false;
        }
        ::memcpy(destBytes + p, un.as_chars, r);
    }
    return succ;
}

bool PtraceWrapper::_writeInternal(int pokeAction, const void* dest, const void* src, size_t count) {
    if (!this->_pid) {
        return false;
    }

    errno = 0;
    bool succ = true;
    size_t p = 0;
    size_t c = count / PT_SIZE;
    size_t r = count % PT_SIZE;
    uint8_t* srcBytes = const_cast<uint8_t*>((const uint8_t*)src);
    uint8_t* destBytes = const_cast<uint8_t*>((const uint8_t*)dest);

    union_intptr_t un;
    for (size_t i = 0; i < c; ++ i) {
        ::memcpy(un.as_chars, srcBytes + i * PT_SIZE, PT_SIZE);
        if (::ptrace(pokeAction, this->_pid, destBytes + i * PT_SIZE, un.as_intptr) == -1) {
            LOGGER_LOGE("PtraceWrapper::writeInternal action of %d failed at 0x%zx: %s\n", pokeAction, uintptr_t((const uint8_t*)srcBytes + i * PT_SIZE), ::strerror(errno));
            succ = false;
            break;
        }
        p += PT_SIZE;
    }
    if (succ && r > 0) {
        /* 
         * in case of overwriting original stack data. here we need to
         * read original PT_SIZE page and modify the remaing bytes on it
         * before writing the whole page back.
         */
        int peakAction = (pokeAction == PTRACE_POKETEXT) ? PTRACE_PEEKTEXT : PTRACE_PEEKDATA;
        un.as_intptr = ::ptrace(peakAction, this->_pid, destBytes + p, 0);
        for (size_t i = 0; i < r; ++ i) un.as_chars[i] = *(srcBytes + p + i);
        if (::ptrace(pokeAction, this->_pid, destBytes + p, un.as_intptr) == -1) {
            succ = false;
        }
    }
    return succ;
}

#if $is($arch_arm64)

// special tricks on arm64

#include <fcntl.h>
#include <linux/elf.h>

bool PtraceWrapper::getRegisters(PtraceRegs* outRegs) {
    bool ok = false;
    if (this->_pid) {
        struct iovec iovec;
        iovec.iov_base = outRegs;
        iovec.iov_len = sizeof(PtraceRegs);
        int regset = NT_PRSTATUS;
        ok = (::ptrace(PTRACE_GETREGSET, _pid, reinterpret_cast<void*>(regset), &iovec) != -1);
        if (!ok) {
            LOGGER_LOGE("PtraceWrapper::getRegisters failed: %s\n", ::strerror(errno));
        }
    }
    return ok;
}

bool PtraceWrapper::setRegisters(const PtraceRegs& regs) {
    bool ok = false;
    if (this->_pid) {
        struct iovec iovec;
        iovec.iov_base = const_cast<PtraceRegs*>(&regs);
        iovec.iov_len = sizeof(PtraceRegs);
        int regset = NT_PRSTATUS;
        ok = (::ptrace(PTRACE_SETREGSET, _pid, reinterpret_cast<void*>(regset), &iovec) != -1);
        if (!ok) {
            LOGGER_LOGE("PtraceWrapper::setRegisters failed: %s\n", ::strerror(errno));
        }
    }
    return ok;
}

#else

bool PtraceWrapper::getRegisters(PtraceRegs* outRegs) {
    bool ok = false;
    if (this->_pid) {
        ok = (::ptrace(PTRACE_GETREGS, this->_pid, nullptr, outRegs) != -1);
        if (!ok) {
            LOGGER_LOGE("PtraceWrapper::getRegisters failed: %s\n", ::strerror(errno));
        }
    }
    return ok;
}

bool PtraceWrapper::setRegisters(const PtraceRegs& regs) {
    bool ok = false;
    if (this->_pid) {
        ok = (::ptrace(PTRACE_SETREGS, this->_pid, nullptr, &regs) != -1);
        if (!ok) {
            LOGGER_LOGE("PtraceWrapper::setRegisters failed: %s\n", ::strerror(errno));
        }
    }
    return ok;
}

#endif

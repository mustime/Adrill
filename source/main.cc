/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#include <fstream>
#include <dirent.h>

#include <config.h>
#include <mem/module.h>
#include <mem/pattern.h>
#include <mem/protect.h>
#include <mem/cmd_param.h>
#include <mem/cmd_param-inl.h>

#include "macros.h"
#include "selinux.h"
#include "sdk_code.h"
#include "file_utils.h"
#include "ptrace_wrapper.h"
#include "call_procedure.h"

int moduleMatcher(mem::region_info* region, void* data) {
    mem::region_info* result = static_cast<mem::region_info*>(data);
    // // only interest in mmap module by system
    // // which is offset == 0 & readable & private
    // if (region->offset == 0 && (region->prot & PROT_READ) && (region->flags & MAP_PRIVATE)) {
        // region->path_name could be nullptr
        if (region->path_name && ::strcmp(region->path_name, result->path_name) == 0) {
            result->start = std::min(result->start, region->start);
            result->end = std::max(result->end, region->end);
            // result->offset = region->offset;
            // result->prot   = region->prot;
            // result->flags  = region->flags;
        }
    // }
    return 0;
}

uintptr_t resolveRemoteFunction(const char* name, uintptr_t localAddr, mem::region_info* localRegionInfo, mem::region_info* remoteRegionInfo) {
    // overcome address space layout randomization(ASLR)
    uintptr_t remoteAddr = 0;
    do {
        BREAK_IF_WITH_LOGE(!localAddr,
            "[!] func '%s' is nullptr\n", name);
        BREAK_IF_WITH_LOGE(::strcmp(localRegionInfo->path_name, remoteRegionInfo->path_name) != 0,
            "[!] local module(%s) and remote module(%s) should refer to the same path\n", localRegionInfo->path_name, remoteRegionInfo->path_name);
        BREAK_IF_WITH_LOGE(!localRegionInfo->start || !remoteRegionInfo->start,
            "[!] local/remote module '%s' not found\n", localRegionInfo->path_name);
        BREAK_IF_WITH_LOGE(localAddr < localRegionInfo->start || localAddr > localRegionInfo->end,
            "[!] func '%s'(0x%zx) is not within module '%s'(0x%zx-0x%zx)\n", name, localAddr, localRegionInfo->path_name, localRegionInfo->start, localRegionInfo->end);
        // same module shares the same offset
        remoteAddr = localAddr - localRegionInfo->start + remoteRegionInfo->start;
    } while (false);
    return remoteAddr;
}

uintptr_t resolveLocalFunction(const std::vector<std::string>& symbols) {
    uintptr_t localAddr = 0;
    for (const auto& symbol : symbols) {
        localAddr = (uintptr_t)dlsym(RTLD_DEFAULT, symbol.c_str());
        if (localAddr > 0) break;
    }
    return localAddr;
}

std::string getBionicLib(const std::string& libname) {
    FileSearcher searcher;
    // Android version < 10.x
    searcher.addSearchPath("/system/lib" $arch_64("64") "/");
    // Android version >= 10.x makes runtime libraries
    // independently OTA updatable through APEX bundles
    if (SDKCode::get() >= SDKCode::Q) {
        /* and makes it search first */
        std::string runtimeRoot("/apex/com.android.runtime");
        searcher.addSearchPath(runtimeRoot.append("/lib" $arch_64("64") "/bionic/"), true);
    }

    std::string location = searcher.resolveFullPath(libname);
    if (location.empty()) {
        LOGGER_LOGE("file %s not found!\n", libname.c_str());
    }
    return location;
}

bool doInject(pid_t pid, const std::string& libPath) {
    errno = 0;
    if (::access(libPath.c_str(), R_OK) != 0) {
        LOGGER_LOGE("[!] file '%s' unavailable: %s\n", libPath.c_str(), ::strerror(errno));
        return false;
    }
    
    // necessary local & remote modules
    std::string libcPath = getBionicLib("libc.so");
    std::string libdlPath = getBionicLib("libdl.so");
    if (libcPath.empty() || libdlPath.empty()) {
        return false;
    }

    mem::region_info localLibcRegionInfo    { $arch_32(UINT_MAX) $arch_64(UINT64_MAX), 0, 0, 0, 0, libcPath.c_str() };
    mem::region_info localLibdlRegionInfo   { $arch_32(UINT_MAX) $arch_64(UINT64_MAX), 0, 0, 0, 0, libdlPath.c_str() };
    mem::region_info remoteLibcRegionInfo   { $arch_32(UINT_MAX) $arch_64(UINT64_MAX), 0, 0, 0, 0, libcPath.c_str() };
    mem::region_info remoteLibdlRegionInfo  { $arch_32(UINT_MAX) $arch_64(UINT64_MAX), 0, 0, 0, 0, libdlPath.c_str() };

    // resolve modules
    mem::iter_proc_maps(0,   moduleMatcher, &localLibcRegionInfo);
    mem::iter_proc_maps(0,   moduleMatcher, &localLibdlRegionInfo);
    mem::iter_proc_maps(pid, moduleMatcher, &remoteLibcRegionInfo);
    mem::iter_proc_maps(pid, moduleMatcher, &remoteLibdlRegionInfo);

    // check target process accessable
    if (!remoteLibcRegionInfo.start || !remoteLibdlRegionInfo.start) {
        LOGGER_LOGE("[!] process %d not found!\n", pid);
        return false;
    }

    // that's the minimum functions to make it work
    uintptr_t remoteFuncMmap    = resolveRemoteFunction("mmap",     (uintptr_t)mmap,    &localLibcRegionInfo,   &remoteLibcRegionInfo);
    uintptr_t remoteFuncMunmap  = resolveRemoteFunction("munmap",   (uintptr_t)munmap,  &localLibcRegionInfo,   &remoteLibcRegionInfo);
    uintptr_t remoteFuncDlopen  = resolveRemoteFunction("dlopen",   (uintptr_t)dlopen,  &localLibdlRegionInfo, &remoteLibdlRegionInfo);
    uintptr_t remoteFuncDlerror = resolveRemoteFunction("dlerror",  (uintptr_t)dlerror, &localLibdlRegionInfo, &remoteLibdlRegionInfo);
    if (!remoteFuncMmap || !remoteFuncMunmap || !remoteFuncDlopen || !remoteFuncDlerror) {
        return false;
    }

    bool ok = true;
    PtraceRegs oriRegs;
    PtraceWrapper ptrace;
    do {
        // attach to target process
        LOGGER_LOGI("[-] attcahing to process %d ...\n", pid);
        ok &= ptrace.attach(pid);
        BREAK_IF_WITH_LOGE(!ok, "[!] failed to attach to process %d: %s\n", pid, ::strerror(errno));

        // save tracee's registers
        LOGGER_LOGI("[-] saving registers ...\n");
        ok &= ptrace.getRegisters(&oriRegs);
        BREAK_IF_WITH_LOGE(!ok, "[!] failed to save registers\n");

        // call remote mmap, alloc the params for dlopen
        LOGGER_LOGI("[-] calling remote mmap ...\n");
        CallProcedure caller(&ptrace);
        const size_t size = PATH_MAX + 1;
        int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
        int flags = MAP_ANONYMOUS | MAP_PRIVATE;
        ok &= caller.remoteCall(remoteFuncMmap, nullptr, size, prot, flags, 0, 0, CallProcedure::ARG_END);
        BREAK_IF_WITH_LOGE(!ok, "[!] failed to call remote mmap\n");
        
        // get the call return value, i.e., the mapped address
        uintptr_t mappedAddr = (uintptr_t)caller.returnValue();
        LOGGER_LOGI("[>] remote mmap return 0x%zx\n", mappedAddr);

        // write libpath string to mapped address
        ok &= ptrace.writeText((void*)mappedAddr, libPath.data(), libPath.length() + 1);
        BREAK_IF_WITH_LOGE(!ok, "[!] failed to write params to 0x%zx\n", mappedAddr);

        // call remote dlopen, load library to tracee process
        LOGGER_LOGI("[-] calling remote dlopen '%s' ...\n", libPath.c_str());
        ok &= caller.remoteCall(remoteFuncDlopen, mappedAddr, RTLD_NOW | RTLD_GLOBAL, /*possible caller since Android7.0*/nullptr, CallProcedure::ARG_END);
        BREAK_IF_WITH_LOGE(!ok, "[!] failed to call dlopen\n");
        
        // get the call return value, i.e., remote module handle
        uintptr_t handle = (uintptr_t)caller.returnValue();
        if (!handle) {
            if (caller.remoteCall(remoteFuncDlerror, nullptr, CallProcedure::ARG_END)) {
                // dlerror return remote error string header
                // we should retrieve it by ptrace.readText
                char* errMsg = (char*)::malloc(size);
                if (ptrace.readText(errMsg, (const void*)caller.returnValue(), size)) {
                    // strip it if the error msg length exceed <size>
                    // shoule be long enough to explain the error though
                    ::strncpy((char*)errMsg + size - 4, "...\0", 4);
                    LOGGER_LOGE("[!] %s\n", errMsg);
                } else {
                    LOGGER_LOGE("[!] dlopen unknown error at 0x%zx\n", caller.returnValue());
                }
                ::free(errMsg);
            }
            // remote call itself works fine, but the result is bad
            ok = false;
        } else {
            LOGGER_LOGI("[>] remote dlopen return 0x%zx\n", handle);
        }
        
        // call remote munmap to free useless params
        LOGGER_LOGI("[-] calling remote munmap ...\n");
        caller.remoteCall(remoteFuncMunmap, mappedAddr, size, CallProcedure::ARG_END);
    } while(false);

    // restore tracee's registers
    LOGGER_LOGI("[-] restoring registers ...\n");
    ptrace.setRegisters(oriRegs);

    // detach safely
    LOGGER_LOGI("[-] detaching from process %d ...\n", pid);
    ptrace.detach();

    return ok;
}

int getPidByName(const std::string& name) {
    int pid = 0;
    std::string rootDir("/proc/");
    std::string rearName("/cmdline");
    DIR* dp = ::opendir(rootDir.c_str());
    if (dp) {
        std::string content;
        struct dirent* entry;
        while ((entry = ::readdir(dp)) != nullptr) {
            // only interest in numeric dir name
            int pendingPid = ::atoi(entry->d_name);
            if (!pendingPid) continue;
            
            std::string curName(entry->d_name);
            std::string filePath = rootDir + curName + rearName;
            std::fstream readf(filePath);
            if (readf.is_open()) {
                std::getline(readf, content);
                if (::strcmp(content.c_str(), name.c_str()) == 0) {
                    LOGGER_LOGE("[>] found pid %d for process '%s'\n", pendingPid, name.c_str());
                    pid = pendingPid;
                    break;
                }
            } else {
                LOGGER_LOGE("[!] failed to open file '%s': %s\n", filePath.c_str(), ::strerror(errno));
            }
        }
        ::closedir(dp);
    } else {
        LOGGER_LOGE("[!] failed to open dir '/proc': %s\n", ::strerror(errno));
    }
    return pid;
}

void help() {
    LOGGER_LOGI("usage: adrill [--pid <number>] | [--pname <string>]\n");
    LOGGER_LOGI("              --libpath <path>\n");
    LOGGER_LOGI("\n");
    LOGGER_LOGI("version: v%d.%d(%s)\n", ADRILL_VERSION_MAJOR, ADRILL_VERSION_MINOR, $arch_arm("arm") $arch_arm64("arm64") $arch_x86("x86") $arch_x64("x86_64"));
    LOGGER_LOGI("   -h,--help      print this message.\n");
    LOGGER_LOGI("      --pid       target process id. e.g., grep from 'ps' command\n");
    LOGGER_LOGI("      --pname     target process name. used to match with content in /proc/<pid>/cmdline.\n");
    LOGGER_LOGI("      --libpath   absolute path to inject. only supports ELF file.\n");
    LOGGER_LOGI("\n");
}

int main(int argc, char *argv[]) {
    int ret = 1;
    mem::cmd_param cmdPid("pid");
    mem::cmd_param cmdPname("pname");
    mem::cmd_param cmdLibpath("libpath");
    mem::cmd_param::init(argc, argv);

    int pid;
    std::string pname;
    std::string libPath;

    cmdPid.get(pid);
    cmdPname.get(pname);
    cmdLibpath.get(libPath);

    if (!pname.empty()) {
        pid = getPidByName(pname);
        if (!pid) {
            LOGGER_LOGE("[!] process '%s' not found!\n", pname.c_str());
            ret = 2;
            return ret;
        }
    }
    
    if (pid && libPath.length()) {
        SELinux::init();
        // already permissive or set to permissive 
        if (SELinux::getEnforce() == SELinuxStatus::PERMISSIVE ||
            SELinux::setEnforce(SELinuxStatus::PERMISSIVE)) {
            ret = doInject(pid, libPath) ? 0 : 3;
        } else {
            LOGGER_LOGE("[!] failed to disable selinux\n");
        }
        LOGGER_LOGI("%s\n\n", ret == 0 ? "[>] enjoy!" : "[!] something went wrong, see errors listed above.");
    } else {
        // bailout info 
        help();
    }
    return ret;
}

/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#ifndef __ADRILL_PTRACE_WRAPPER_H__
#define __ADRILL_PTRACE_WRAPPER_H__

#include <vector>
#include <asm/ptrace.h>

#include "arch.h"

#define PT_SIZE sizeof(intptr_t)

#if   $is($arch_arm64)
    typedef struct user_pt_regs     PtraceRegs;
#elif $is($arch_x64)
    typedef struct user_regs_struct PtraceRegs;
#else
    typedef struct pt_regs          PtraceRegs;
#endif

class PtraceWrapper {
public:
    PtraceWrapper();
    
    /*
     * flow controlling
     */
    bool attach(pid_t pid);
    bool detach();
    bool kontinue(); // alias for 'continue'. you know why
    
    /*
     * wait for the tracee to raise specified signal or signals(any of list)
     */
    bool waitForSignal(int signal = 0);
    bool waitForSignals(const std::vector<int>& signals);

    /*
     * technically, Linux does not have separate text and data address spaces,
     * so these two requests are currently equivalent
     * but who knows, welcome to Android
     */
    bool readText(void* dest, const void* src, size_t count);
    bool readData(void* dest, const void* src, size_t count);
    bool writeText(const void* dest, const void* src, size_t count);
    bool writeData(const void* dest, const void* src, size_t count);

    /*
     * registers operation
     */
    bool getRegisters(PtraceRegs* outRegs);
    bool setRegisters(const PtraceRegs& regs);

protected:
    // here's a workaround when the speficied pid indicates a zygote process
    bool _connectToZygote();
    bool _readInternal(int action, void* dest, const void* src, size_t count);
    bool _writeInternal(int action, const void* dest, const void* src, size_t count);

protected:
    pid_t _pid;
    bool  _isZygote;

};

#endif // __ADRILL_PTRACE_WRAPPER_H__

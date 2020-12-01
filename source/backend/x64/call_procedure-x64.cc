/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

// for registers offset
#define __FRAME_OFFSETS
#include <asm/ptrace-abi.h>
//

#include "arch.h"
#include "call_procedure.h"

#if $is($arch_x64)

#define MAX_ARG_REGS 6

// referenced from: https://wiki.osdev.org/Calling_Conventions#Cheat_Sheets

bool CallProcedure::_setupCall(uintptr_t remoteAddr, const std::vector<intptr_t>& args) {
    // push up to MAX_ARG_REGS arguments into the following registers respectively
    const int ARG_REGS_OFFSET[] = { RDI, RSI, RDX, RCX, R8, R9 };
    int pushn = 0;
    int argn = args.size();
    int maxArgNum = std::min(argn, MAX_ARG_REGS);
    for (; pushn < maxArgNum ; ++ pushn) {
        // copy param to corresponding register
        *((uintptr_t*)&this->_curRegs + ARG_REGS_OFFSET[pushn] / PT_SIZE) = args[pushn];
    }

    // stack pointer must be aligned by 16 before any CALL instruction,
    // so that the value of RSP is 8 modulo 16 at the entry of a function
    uintptr_t extraStackSize = PT_SIZE; // return address
    extraStackSize += argn > MAX_ARG_REGS ? PT_SIZE * (argn - MAX_ARG_REGS) : 0; // extra params
    // calculate the closest aligned stack
    while (((this->_curRegs.rsp - extraStackSize - PT_SIZE) & 0xf) != 0) this->_curRegs.rsp --;

    // further params are transferred on the stack with the first params
    // at the lowest address and aligned by 8
    if (argn > MAX_ARG_REGS) {
        const intptr_t* argHead = args.data();
        this->_curRegs.rsp -= PT_SIZE * (argn - MAX_ARG_REGS);
        this->_ptraceWrapper->writeText((void*)this->_curRegs.rsp, (const void*)(argHead + pushn), (argn - pushn) * PT_SIZE);
    }

    // set return address to null
    uintptr_t nullAddr = 0;
    this->_curRegs.rsp -= PT_SIZE;
    this->_ptraceWrapper->writeText((void*)this->_curRegs.rsp, (void*)&nullAddr, PT_SIZE);
    
    // modify pc
    this->_curRegs.rip = remoteAddr;

    this->_curRegs.rax = this->_curRegs.orig_rax = 0;
    return true;
}

intptr_t CallProcedure::returnValue() {
    return this->_curRegs.rax;
}

bool CallProcedure::_checkCall() {
    // return address has been set to null
    return this->_curRegs.rip == 0;
}

#endif
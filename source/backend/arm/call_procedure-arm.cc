/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#include "arch.h"
#include "call_procedure.h"

#if $is($arch_arm)

#define MAX_ARG_REGS 4

// bit 5 in CPSR(current program status register)
// indicates the Thumb state
// reference: https://en.wikipedia.org/wiki/ARM_architecture#Registers
#define MASK_CPSR_THUMB_STATE (1u << 5)

bool CallProcedure::_setupCall(uintptr_t remoteAddr, const std::vector<intptr_t>& args) {
    // push up to MAX_ARG_REGS arguments into registers
    int pushn = 0;
    int argn = args.size();
    int maxArgNum = std::min(argn, MAX_ARG_REGS);
    for (; pushn < maxArgNum ; ++ pushn) {
        // copy param to corresponding register
        this->_curRegs.uregs[pushn] = args[pushn];
    }

    // push the remaining arguments onto stack
    if (pushn < argn) {
        const intptr_t* argHead = args.data();
        this->_curRegs.ARM_sp -= (argn - pushn) * PT_SIZE;
        this->_ptraceWrapper->writeText((void*)this->_curRegs.ARM_sp, (const void*)(argHead + pushn), (argn - pushn) * PT_SIZE);
    }

    // setup target func address depends on instruction state
    // while ARM instructions are always 16bit(Thumb)/32bit(Arm) aligned,
    // the LSB(least significant bit) of PC is used to determine the instruction state
    if (remoteAddr & 0x1) {
        // target function is compiled to Thumb instructions
        // set Thumb state bit in CPSR
        this->_curRegs.ARM_cpsr |= MASK_CPSR_THUMB_STATE;
        // clear the LSB of target function address, and the CPU would do the switch itself
        this->_curRegs.ARM_pc = remoteAddr & (~0x1u);
    } else {
        // clear Thumb state bit in CPSR
        this->_curRegs.ARM_cpsr &= ~MASK_CPSR_THUMB_STATE;
        this->_curRegs.ARM_pc = remoteAddr;
    }
    // return address set to null to raise SIGSEGV
    // so we could take it over through waitpid after function call
    this->_curRegs.ARM_lr = 0;
    
    return true;
}

intptr_t CallProcedure::returnValue() {
    return this->_curRegs.ARM_r0;
}

bool CallProcedure::_checkCall() {
    // return address has been set to null
    return this->_curRegs.ARM_pc == 0;
}

#endif
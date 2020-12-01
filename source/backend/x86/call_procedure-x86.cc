/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#include "arch.h"
#include "call_procedure.h"

#if $is($arch_x86)

// referenced from: https://wiki.osdev.org/Calling_Conventions#Cheat_Sheets

bool CallProcedure::_setupCall(uintptr_t remoteAddr, const std::vector<intptr_t>& args) {
    // pushing all arguments onto stack
    size_t argn = args.size();
    this->_curRegs.esp -= argn * PT_SIZE;
    if (argn > 0) {
        this->_ptraceWrapper->writeText((void*)this->_curRegs.esp, (const void*)args.data(), argn * PT_SIZE);
    }
    
    // set return address to null
    uintptr_t nullAddr = 0;
    this->_curRegs.esp -= PT_SIZE;
    this->_ptraceWrapper->writeText((void*)this->_curRegs.esp, (void*)&nullAddr, PT_SIZE);
    
    // modify pc
    this->_curRegs.eip = remoteAddr;
    return true;
}

intptr_t CallProcedure::returnValue() {
    return this->_curRegs.eax;
}

bool CallProcedure::_checkCall() {
    // return address has been set to null
    return this->_curRegs.eip == 0;
}

#endif
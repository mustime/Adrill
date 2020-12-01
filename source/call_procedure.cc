/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#include "macros.h"
#include "call_procedure.h"

// magic
intptr_t CallProcedure::ARG_END = (intptr_t)0xca1111ca;

CallProcedure::CallProcedure(PtraceWrapper* ptraceWrapper)
: _ptraceWrapper(ptraceWrapper) {
}

bool CallProcedure::remoteCall(uintptr_t remoteAddr, ...) {
    // fill the arguments list
    ::va_list args;
    ::va_start(args, remoteAddr);
    std::vector<intptr_t> vargs;
    for (intptr_t p = CallProcedure::ARG_END; (p = va_arg(args, intptr_t)) != CallProcedure::ARG_END;) {
        vargs.push_back(p);
    }
    ::va_end(args);
    
    bool ok = true;
    do {
        // first of all. backup the registers from tracee
        ok &= this->_ptraceWrapper->getRegisters(&this->_curRegs);
        BREAK_IF_WITH_LOGE(!ok, "CallProcedure::remoteCall failed to save registers through ptrace\n");
        
        // setup call procedure according to different archs.
        // registers would be modified here
        ok &= this->_setupCall(remoteAddr, vargs);
        BREAK_IF_WITH_LOGE(!ok, "CallProcedure::remoteCall failed to setup call procedure\n");
        
        // set modified registers to tracee
        ok &= this->_ptraceWrapper->setRegisters(this->_curRegs);
        BREAK_IF_WITH_LOGE(!ok, "CallProcedure::remoteCall failed to setup registers through ptrace\n");
        
        // continue running
        ok &= this->_ptraceWrapper->kontinue();
        BREAK_IF_WITH_LOGE(!ok, "CallProcedure::remoteCall failed to continue ptracee\n");
        
        // tracee should be stopped with signal SIGSEGV by now
        // just as what we excepted in _setupCall
        ok &= this->_ptraceWrapper->waitForSignal(SIGSEGV);
        BREAK_IF_WITH_LOGE(!ok, "CallProcedure::remoteCall failed to wait for SIGSEGV\n");
        
        // now we could get the function call return value from registers
        ok &= this->_ptraceWrapper->getRegisters(&this->_curRegs);
        BREAK_IF_WITH_LOGE(!ok, "CallProcedure::remoteCall failed to get registers through ptrace\n");
        
        // check call procedure
        ok &= this->_checkCall();
        BREAK_IF_WITH_LOGE(!ok, "CallProcedure::remoteCall failed to check call status\n");
    } while (false);
    return ok;
}
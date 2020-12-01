/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#ifndef __ADRILL_CALL_PROCEDURE_H__
#define __ADRILL_CALL_PROCEDURE_H__

#include "ptrace_wrapper.h"

class CallProcedure {
public:
    // used in performCall, indicates params' end
    static intptr_t ARG_END;

public:
    /*
     * we need a attached ptrace instance
     */
    CallProcedure(PtraceWrapper* ptraceWrapper);

    /*
     * preform a remote function call in tracee process
     * following with arguments
     */
    bool remoteCall(uintptr_t remoteAddr, ...);

    /*
     * get the return value after remote call
     */
    intptr_t returnValue();

protected:
    bool _setupCall(uintptr_t remoteAddr, const std::vector<intptr_t>& args);
    bool _checkCall();

protected:
    PtraceWrapper* _ptraceWrapper;
    PtraceRegs _curRegs;

};

#endif // __ADRILL_CALL_PROCEDURE_H__

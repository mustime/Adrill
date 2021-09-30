/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#ifndef __ADRILL_SDK_CODE_H__
#define __ADRILL_SDK_CODE_H__

class SDKCode final {
public:
    // 4.0
    static int ICS;
    // 4.0.3
    static int ICS_MR1;
    // 4.1
    static int JB;
    // 4.2
    static int JB_MR1;
    // 4.3
    static int JB_MR2;
    // 4.4
    static int K;
    // 4.4W
    static int K_W;
    // 5.0
    static int L;
    // 5.1
    static int L_MR1;
    // 6.0
    static int M;
    // 7.0
    static int N;
    // 7.1.1
    static int N_MR1;
    // 8.0
    static int O;
    // 8.1
    static int O_MR1;
    // 9.0
    static int P;
    // 10.0
    static int Q;
    // 11.0
    static int R;
    // 12.0
    static int S;

public:
    static int get();

private:
    static int _code;

};

#endif // __ADRILL_SDK_CODE_H__

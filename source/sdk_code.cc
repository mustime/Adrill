/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#include <stdlib.h>
#include <sys/system_properties.h>
#include <android/api-level.h>

#include "macros.h"
#include "sdk_code.h"

#define SDK_CODE_UNKNOWN -1

int SDKCode::_code = SDK_CODE_UNKNOWN;
// 4.0
int SDKCode::ICS = __ANDROID_API_I__;
// 4.0.3
int SDKCode::ICS_MR1 = __ANDROID_API_I__ + 1;
// 4.1
int SDKCode::JB = __ANDROID_API_J__;
// 4.2
int SDKCode::JB_MR1 = __ANDROID_API_J_MR1__;
// 4.3
int SDKCode::JB_MR2 = __ANDROID_API_J_MR2__;
// 4.4
int SDKCode::K = __ANDROID_API_K__;
// 4.4W
int SDKCode::K_W = __ANDROID_API_K__ + 1;
// 5.0
int SDKCode::L = __ANDROID_API_L__;
// 5.1
int SDKCode::L_MR1 = __ANDROID_API_L_MR1__;
// 6.0
int SDKCode::M = __ANDROID_API_M__;
// 7.0
int SDKCode::N = __ANDROID_API_N__;
// 7.1.1
int SDKCode::N_MR1 = __ANDROID_API_N_MR1__;
// 8.0
int SDKCode::O = __ANDROID_API_O__;
// 8.1
int SDKCode::O_MR1 = __ANDROID_API_O_MR1__;
// 9.0
int SDKCode::P = __ANDROID_API_P__;
// 10.0
int SDKCode::Q = __ANDROID_API_Q__;
// 11.0
int SDKCode::R = __ANDROID_API_R__;
// 12.0
int SDKCode::S = __ANDROID_API_R__ + 1;

int SDKCode::get() {
    if (SDKCode::_code == SDK_CODE_UNKNOWN) {
        int len = 0;
        char sdk[92] = {0}, *end;
        if ((len = ::__system_property_get("ro.build.version.sdk", sdk)) > 0) {
            end = sdk + len;
            SDKCode::_code = (int)::strtol(sdk, &end, 10);
        } else {
            LOGGER_LOGE("unable to get sdk version\n");
        }

        // if preview_sdk not equals to 0, it's an unofficially released sdk
        // and should be treated as next generation sdk version code
        // e,g.: sdk 30 + preview_sdk 2 => sdk 31
        if ((len = ::__system_property_get("ro.build.version.preview_sdk", sdk)) > 0) {
            end = sdk + len;
            SDKCode::_code += (int)::strtol(sdk, &end, 10) > 0 ? 1 : 0;
        }
    }
    return SDKCode::_code;
}
/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#ifndef __ADRILL_LOGGER_H__
#define __ADRILL_LOGGER_H__

#define LOGGER_USE_STD

#if defined(LOGGER_USE_STD)

#include <stdio.h>

#define LOGGER_LOG(...) ::printf(__VA_ARGS__)
#define LOGGER_LOGI(...) ::printf(__VA_ARGS__)
#define LOGGER_LOGE(...) ::printf(__VA_ARGS__)

#else

#include <android/log.h>

#define LOGGER_LOG(...) __android_log_print(ANDROID_LOG_DEBUG, "drill", __VA_ARGS__)
#define LOGGER_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "drill", __VA_ARGS__)
#define LOGGER_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "drill", __VA_ARGS__)

#endif

#endif // __ADRILL_LOGGER_H__

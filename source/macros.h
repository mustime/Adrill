/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#ifndef __ADRILL_MACROS_H__
#define __ADRILL_MACROS_H__

#include "logger.h"

#define BREAK_IF(cond) if ((cond)) { break; }
#define BREAK_IF_WITH_LOGE(cond, msg, ...) if ((cond)) { LOGGER_LOGE(msg, ##__VA_ARGS__); break; }

#endif // __ADRILL_MACROS_H__

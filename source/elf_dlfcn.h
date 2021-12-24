/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#ifndef __ADRILL_UTILS_ELF_DLFCN_H__
#define __ADRILL_UTILS_ELF_DLFCN_H__

#include <dlfcn.h>

#define RTLD_PARSE_ELF 0x0e1f0e1f

/**
 * for Android after Nougat, or explicitly specified flags of RTLD_PARSE_ELF,
 * elf_dlopen use mmap to load libpath and try to search symbols from ELF sections.
 * otherwise, it calls original linker's dl* functions.
 */
void* elf_dlopen(const char* libpath, int flags);
void* elf_dlsym(void* handle, const char* name);
int   elf_dlclose(void* handle);

#endif // __ADRILL_UTILS_ELF_DLFCN_H__

/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */

#include <fcntl.h>
#include <sys/mman.h>

#include <mem/protect.h>
#include <elfio/elfio.hpp>

#include "macros.h"
#include "sdk_code.h"
#include "elf_dlfcn.h"

#define MOCK_INDICATOR 0xfaced1fc

namespace internal {
    struct context {
        // always equals to MOCK_INDICATOR
        // recognized by elf_dlsym & elf_dlclose
        int indicator;
        mem::region_info* region_info;
        ELFIO::elfio* elf_reader;
        ELFIO::Elf64_Addr bias;
    };
} // namespace internal

void* elf_dlopen(const char* libpath, int flags) {
    // check flags
    if ((flags & RTLD_PARSE_ELF) != RTLD_PARSE_ELF && SDKCode::get() < SDKCode::N) {
        return ::dlopen(libpath, flags);
    }

    struct internal::context* ctx = nullptr;
    do {
        auto region_info = new mem::region_info();
        // resolve modules
        int ret = mem::get_region_info(/*self*/0, libpath, region_info);
        if (ret == 0) {
            LOGGER_LOGE("module '%s' not found in /proc/self/maps\n", libpath);
            delete region_info;
            break;
        }

        auto reader = new ELFIO::elfio();
        if (!reader->load(libpath)) {
            LOGGER_LOGE("not an ELF: '%s'\n", libpath);
            delete reader;
            break;
        }

        // calloc is zero-filled
        ctx = (struct internal::context*)::calloc(1, sizeof(struct internal::context));
        // indicating this is a elf_dlopened handle
        ctx->indicator = MOCK_INDICATOR;
        ctx->region_info = region_info;
        ctx->elf_reader = reader;
        ELFIO::Elf_Half seg_num = reader->segments.size();
        for (ELFIO::Elf_Half i = 0; i < seg_num; ++i) {
            ELFIO::segment* seg = reader->segments[i];
            if (seg->get_type() == ELFIO::PT_LOAD) {
                ctx->bias = seg->get_virtual_address();
                break;
            }
        }
    } while (false);
    return ctx;
}

void* elf_dlsym(void* handle, const char* symname) {
    auto ctx = (struct internal::context*)handle;
    if (handle == RTLD_DEFAULT ||
        handle == RTLD_NEXT ||
        ctx->indicator != MOCK_INDICATOR) {
        return ::dlsym(handle, symname);
    }

    std::string       name;
    ELFIO::Elf64_Addr value = 0;
    ELFIO::Elf_Xword  size = 0;
    uint8_t           bind = 0;
    uint8_t           type = 0;
    ELFIO::Elf_Half   section = 0;
    uint8_t           other = 0;
    // iterates all sections
    bool found = false;
    auto reader = ctx->elf_reader;
    ELFIO::Elf_Half sec_num = reader->sections.size();
    for (ELFIO::Elf_Half i = 0; i < sec_num && !found; i ++) {
        ELFIO::section* sec = reader->sections[i];
        if (sec->get_type() == ELFIO::SHT_SYMTAB || sec->get_type() == ELFIO::SHT_DYNSYM) {
            ELFIO::symbol_section_accessor symbols(*reader, sec);
            ELFIO::Elf_Xword sym_num = symbols.get_symbols_num();
            for (ELFIO::Elf_Xword j = 0; j < sym_num && !found; j++) {
                symbols.get_symbol(j, name, value, size, bind, type, section, other);
                found = (::strcmp(name.c_str(), symname) == 0);
            }
        }
    }
    return found ? (void*)(ctx->region_info->start + value - ctx->bias) : nullptr;
}

int elf_dlclose(void* handle) {
    auto ctx = (struct internal::context*)handle;
    if (ctx->indicator != MOCK_INDICATOR) {
        return ::dlclose(handle);
    }

    if (ctx->region_info) {
        delete ctx->region_info;
        ctx->region_info = nullptr;
    }
    if (ctx->elf_reader) {
        delete ctx->elf_reader;
        ctx->elf_reader = nullptr;
    }
    ::free(ctx);
    return 0;
}

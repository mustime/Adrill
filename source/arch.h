/*
 * Copyright (c) 2020, Irvin Pang <halo.irvin@gmail.com>
 * All rights reserved.
 * 
 * see LICENSE file for details
 */
 
#ifndef __ADRILL_ARCH_H__
#define __ADRILL_ARCH_H__

#define $yes(...)       __VA_ARGS__
#define $no(...)
#define $on(v)          (0 v(+1))  // usage: #if $on($android)
#define $is             $on        // usage: #if $is($debug)

#if defined(__arm__)
#   define $arch_32     $yes
#   define $arch_64     $no
#   define $arch_arm    $yes
#   define $arch_arm64  $no
#   define $arch_x86    $no
#   define $arch_x64    $no
#elif defined(__arm64__) || defined(__aarch64__)
#   define $arch_32     $no
#   define $arch_64     $yes
#   define $arch_arm    $no
#   define $arch_arm64  $yes
#   define $arch_x86    $no
#   define $arch_x64    $no
#elif defined(__x86__) || defined(__i386__)
#   define $arch_32     $yes
#   define $arch_64     $no
#   define $arch_arm    $no
#   define $arch_arm64  $no
#   define $arch_x86    $yes
#   define $arch_x64    $no
#elif defined(__x86_64__)
#   define $arch_32     $no
#   define $arch_64     $yes
#   define $arch_arm    $no
#   define $arch_arm64  $no
#   define $arch_x86    $no
#   define $arch_x64    $yes
#endif

#if !defined(DEBUG) || DEBUG == 0
#   define $release     $yes
#   define $debug       $no
#else
#   define $release     $no
#   define $debug       $yes
#endif

#endif  // __ADRILL_ARCH_H__


/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef LIBYUV_COMMON_BASIC_TYPES_H_
#define LIBYUV_COMMON_BASIC_TYPES_H_

#include <stddef.h>  // for NULL, size_t

#ifndef WIN32
#include <stdint.h>  // for uintptr_t
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "constructor_magic.h"


#ifndef INT_TYPES_DEFINED
#define INT_TYPES_DEFINED
#ifdef COMPILER_MSVC
typedef __int64 int64;
#else
typedef long long int64;
#endif /* COMPILER_MSVC */
typedef int int32;
typedef short int16;
typedef char int8;

#ifdef COMPILER_MSVC
typedef unsigned __int64 uint64;
typedef __int64 int64;
#ifndef INT64_C
#define INT64_C(x) x ## I64
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## UI64
#endif
#define INT64_F "I64"
#else
typedef unsigned long long uint64;
typedef long long int64;
#ifndef INT64_C
#define INT64_C(x) x ## LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## ULL
#endif
#define INT64_F "ll"
#endif /* COMPILER_MSVC */
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
#endif  // INT_TYPES_DEFINED

#ifdef WIN32
typedef int socklen_t;
#endif

namespace libyuv {
  template<class T> inline T _min(T a, T b) { return (a > b) ? b : a; }
  template<class T> inline T _max(T a, T b) { return (a < b) ? b : a; }

  // For wait functions that take a number of milliseconds, kForever indicates
  // unlimited time.
  const int kForever = -1;
}

// Detect compiler is for x86 or x64.
#if defined(__x86_64__) || defined(_M_X64) || \
    defined(__i386__) || defined(_M_IX86)
#define CPU_X86 1
#endif

#ifdef WIN32
#define alignof(t) __alignof(t)
#else  // !WIN32
#define alignof(t) __alignof__(t)
#endif  // !WIN32
#define IS_ALIGNED(p, a) (0==(reinterpret_cast<uintptr_t>(p) & ((a)-1)))
#define ALIGNP(p, t) \
  (reinterpret_cast<uint8*>(((reinterpret_cast<uintptr_t>(p) + \
  ((t)-1)) & ~((t)-1))))

#ifndef UNUSED
#define UNUSED(x) Unused(static_cast<const void *>(&x))
#define UNUSED2(x,y) Unused(static_cast<const void *>(&x)); Unused(static_cast<const void *>(&y))
#define UNUSED3(x,y,z) Unused(static_cast<const void *>(&x)); Unused(static_cast<const void *>(&y)); Unused(static_cast<const void *>(&z))
#define UNUSED4(x,y,z,a) Unused(static_cast<const void *>(&x)); Unused(static_cast<const void *>(&y)); Unused(static_cast<const void *>(&z)); Unused(static_cast<const void *>(&a))
#define UNUSED5(x,y,z,a,b) Unused(static_cast<const void *>(&x)); Unused(static_cast<const void *>(&y)); Unused(static_cast<const void *>(&z)); Unused(static_cast<const void *>(&a)); Unused(static_cast<const void *>(&b))
inline void Unused(const void *) { }
#endif // UNUSED

#if defined(__GNUC__)
#define GCC_ATTR(x) __attribute__ ((x))
#else  // !__GNUC__
#define GCC_ATTR(x)
#endif  // !__GNUC__

#endif // LIBYUV_COMMON_BASIC_TYPES_H_

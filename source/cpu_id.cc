/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/cpu_id.h"

#include <stdlib.h>  // For getenv()
#ifdef _MSC_VER
#include <intrin.h>  // For __cpuid()
#endif

// For ArmCpuCaps() but unittested on all platforms
#include <stdio.h>
#include <string.h>

#include "libyuv/basic_types.h"  // For CPU_X86

// TODO(fbarchard): Use cpuid.h when gcc 4.4 is used on OSX and Linux.
#if (defined(__pic__) || defined(__APPLE__)) && defined(__i386__)
static __inline void __cpuid(int cpu_info[4], int info_type) {
  asm volatile (
    "mov %%ebx, %%edi                          \n"
    "cpuid                                     \n"
    "xchg %%edi, %%ebx                         \n"
    : "=a"(cpu_info[0]), "=D"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type));
}
#elif defined(__i386__) || defined(__x86_64__)
static __inline void __cpuid(int cpu_info[4], int info_type) {
  asm volatile (
    "cpuid                                     \n"
    : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type));
}
#endif

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// based on libvpx arm_cpudetect.c
// For Arm, but testable on any CPU
int ArmCpuCaps(const char* cpuinfoname) {
  int flags = 0;
  FILE* fin = fopen(cpuinfoname, "r");
  if (fin) {
    char buf[512];
    while (fgets(buf, 511, fin)) {
      if (memcmp(buf, "Features", 8) == 0) {
        char* p = strstr(buf, " neon");
        if (p && (p[5] == ' ' || p[5] == '\n')) {
          flags |= kCpuHasNEON;
          break;
        }
      }
    }
    fclose(fin);
  }
  return flags;
}

// CPU detect function for SIMD instruction sets.
int cpu_info_ = 0;

int InitCpuFlags() {
#ifdef CPU_X86
  int cpu_info[4];
  __cpuid(cpu_info, 1);
  cpu_info_ = (cpu_info[3] & 0x04000000 ? kCpuHasSSE2 : 0) |
              (cpu_info[2] & 0x00000200 ? kCpuHasSSSE3 : 0) |
              (cpu_info[2] & 0x00080000 ? kCpuHasSSE41 : 0) |
              kCpuInitialized | kCpuHasX86;

  // environment variable overrides for testing.
  if (getenv("LIBYUV_DISABLE_X86")) {
    cpu_info_ &= ~kCpuHasX86;
  }
  if (getenv("LIBYUV_DISABLE_SSE2")) {
    cpu_info_ &= ~kCpuHasSSE2;
  }
  if (getenv("LIBYUV_DISABLE_SSSE3")) {
    cpu_info_ &= ~kCpuHasSSSE3;
  }
  if (getenv("LIBYUV_DISABLE_SSE41")) {
    cpu_info_ &= ~kCpuHasSSE41;
  }
  if (getenv("LIBYUV_DISABLE_ASM")) {
    cpu_info_ = kCpuInitialized;
  }
#elif defined(__linux__) && defined(__ARM_NEON__)
  cpu_info_ = ArmCpuCaps("/proc/cpuinfo") | kCpuInitialized;
#elif defined(__ARM_NEON__)
  // gcc -mfpu=neon defines __ARM_NEON__
  // Enable Neon if you want support for Neon and Arm, and use MaskCpuFlags
  // to disable Neon on devices that do not have it.
  cpu_info_ = kCpuHasNEON | kCpuInitialized | kCpuHasARM;
#else
  cpu_info_ = kCpuInitialized | kCpuHasARM;
#endif
  return cpu_info_;
}

void MaskCpuFlags(int enable_flags) {
  InitCpuFlags();
  cpu_info_ = (cpu_info_ & enable_flags) | kCpuInitialized;
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

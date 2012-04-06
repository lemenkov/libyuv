/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef INCLUDE_LIBYUV_CPU_ID_H_
#define INCLUDE_LIBYUV_CPU_ID_H_

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// These flags are only valid on x86 processors
static const int kCpuHasX86 = 1;
static const int kCpuHasSSE2 = 2;
static const int kCpuHasSSSE3 = 4;
static const int kCpuHasSSE41 = 8;

// These flags are only valid on ARM processors
static const int kCpuHasARM = 16;
static const int kCpuHasNEON = 32;

// Internal flag to indicate cpuid is initialized.
static const int kCpuInitialized = 64;

// Detect CPU has SSE2 etc.
// test_flag parameter should be one of kCpuHas constants above
// returns non-zero if instruction set is detected
static __inline int TestCpuFlag(int test_flag) {
  extern int cpu_info_;
  extern int InitCpuFlags();
  return (cpu_info_ ? cpu_info_ : InitCpuFlags()) & test_flag;
}

// For testing, allow CPU flags to be disabled.
// ie MaskCpuFlags(~kCpuHasSSSE3) to disable SSSE3.
// MaskCpuFlags(-1) to enable all cpu specific optimizations.
// MaskCpuFlags(kCpuInitialized) to disable all cpu specific optimizations.
// MaskCpuFlags(0) to re-initialize all cpu detection.
void MaskCpuFlags(int enable_flags);

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // INCLUDE_LIBYUV_CPU_ID_H_

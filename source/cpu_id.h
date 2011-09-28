/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef LIBYUV_SOURCE_CPU_ID_H_
#define LIBYUV_SOURCE_CPU_ID_H_

#include <string>

#include "basic_types.h"

namespace libyuv {
#ifdef CPU_X86
void cpuid(int cpu_info[4], int info_type);
#endif

class CpuInfo {
 public:
  // These flags are only valid on x86 processors
  static const int kCpuHasSSE2 = 1;
  static const int kCpuHasSSSE3 = 2;

  // SIMD support on ARM processors
  static const int kCpuHasNEON = 4;

  // Detect CPU has SSE2 etc.
  static bool TestCpuFlag(int flag);

  // Detect CPU vendor: "GenuineIntel" or "AuthenticAMD"
  static std::string GetCpuVendor();

  // For testing, allow CPU flags to be disabled.
  static void MaskCpuFlagsForTest(int enable_flags);

 private:
  // Global lock for the cpu initialization
  static bool cpu_info_initialized_;
  static int cpu_info_;

  static void InitCpuFlags();

  DISALLOW_IMPLICIT_CONSTRUCTORS(CpuInfo);
};

}  // namespace libyuv

#endif  // LIBYUV_SOURCE_CPU_ID_H_

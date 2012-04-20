/*
 *  Copyright (c) 2012 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <string.h>

#include "libyuv/basic_types.h"
#include "libyuv/cpu_id.h"
#include "libyuv/version.h"
#include "unit_test/unit_test.h"

namespace libyuv {

TEST_F(libyuvTest, TestVersion) {
  EXPECT_GE(LIBYUV_VERSION, 169);
}

TEST_F(libyuvTest, TestCpuHas) {
#if LIBYUV_VERSION >= 236
  int has_x86 = TestCpuFlag(kCpuHasX86);
  printf("Has X86 %d\n", has_x86);
#endif
  int has_sse2 = TestCpuFlag(kCpuHasSSE2);
  printf("Has SSE2 %d\n", has_sse2);
  int has_ssse3 = TestCpuFlag(kCpuHasSSSE3);
  printf("Has SSSE3 %d\n", has_ssse3);
#if LIBYUV_VERSION >= 236
  int has_sse41 = TestCpuFlag(kCpuHasSSE41);
  printf("Has SSE4.1 %d\n", has_sse41);
#endif
#if LIBYUV_VERSION >= 238
  int has_arm = TestCpuFlag(kCpuHasARM);
  printf("Has ARM %d\n", has_arm);
#endif
  int has_neon = TestCpuFlag(kCpuHasNEON);
  printf("Has NEON %d\n", has_neon);
}

// For testing purposes call the proc/cpuinfo parser directly
extern "C" int ArmCpuCaps(const char* cpuinfoname);

TEST_F(libyuvTest, TestLinuxNeon) {
  EXPECT_EQ(0, ArmCpuCaps("unit_test/testdata/arm_v7.txt"));
  EXPECT_EQ(kCpuHasNEON, ArmCpuCaps("unit_test/testdata/tegra3.txt"));
}

}  // namespace libyuv

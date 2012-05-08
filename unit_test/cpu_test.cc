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
  int has_x86 = TestCpuFlag(kCpuHasX86);
  printf("Has X86 %d\n", has_x86);
  int has_sse2 = TestCpuFlag(kCpuHasSSE2);
  printf("Has SSE2 %d\n", has_sse2);
  int has_ssse3 = TestCpuFlag(kCpuHasSSSE3);
  printf("Has SSSE3 %d\n", has_ssse3);
  int has_sse41 = TestCpuFlag(kCpuHasSSE41);
  printf("Has SSE4.1 %d\n", has_sse41);
  int has_arm = TestCpuFlag(kCpuHasARM);
  printf("Has ARM %d\n", has_arm);
  int has_neon = TestCpuFlag(kCpuHasNEON);
  printf("Has NEON %d\n", has_neon);
}

#if defined(__i386__) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
TEST_F(libyuvTest, TestCpuId) {
  int has_x86 = TestCpuFlag(kCpuHasX86);
  if (has_x86) {
    int cpu_info[4];
    // Vendor ID:
    // AuthenticAMD AMD processor
    // CentaurHauls Centaur processor
    // CyrixInstead Cyrix processor
    // GenuineIntel Intel processor
    // GenuineTMx86 Transmeta processor
    // Geode by NSC National Semiconductor processor
    // NexGenDriven NexGen processor
    // RiseRiseRise Rise Technology processor
    // SiS SiS SiS  SiS processor
    // UMC UMC UMC  UMC processor
    CpuId(cpu_info, 0);
    cpu_info[0] = cpu_info[1];  // Reorder output
    cpu_info[1] = cpu_info[3];
    cpu_info[2] = cpu_info[2];
    cpu_info[3] = 0;
    printf("Cpu Vendor: %s\n", reinterpret_cast<char*>(&cpu_info[0]));
    EXPECT_EQ(12, strlen(reinterpret_cast<char*>(&cpu_info[0])));

    // CPU Family and Model
    // 3:0 - Stepping
    // 7:4 - Model
    // 11:8 - Family
    // 13:12 - Processor Type
    // 19:16 - Extended Model
    // 27:20 - Extended Family
    CpuId(cpu_info, 1);
    int family = ((cpu_info[0] >> 8) & 0x0f) | ((cpu_info[0] >> 16) & 0xff0);
    int model = ((cpu_info[0] >> 4) & 0x0f) | ((cpu_info[0] >> 12) & 0xf0);
    printf("Cpu Family %d, Model %d\n", family, model);
  }
}
#endif

// For testing purposes call the proc/cpuinfo parser directly
extern "C" int ArmCpuCaps(const char* cpuinfoname);

TEST_F(libyuvTest, TestLinuxNeon) {
  EXPECT_EQ(0, ArmCpuCaps("unit_test/testdata/arm_v7.txt"));
  EXPECT_EQ(kCpuHasNEON, ArmCpuCaps("unit_test/testdata/tegra3.txt"));
}

}  // namespace libyuv

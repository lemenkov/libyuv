/*
 *  Copyright (c) 2012 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "unit_test.h"

#include <stdlib.h>
#include <string.h>

#include "libyuv/basic_types.h"
#include "libyuv/cpu_id.h"
#include "libyuv/version.h"

namespace libyuv {

// For testing purposes call the proc/cpuinfo parser directly
extern "C" int ArmCpuCaps(const char* cpuinfoname);

TEST_F(libyuvTest, TestLinuxNeon) {
  EXPECT_EQ(0, ArmCpuCaps("unit_test/testdata/arm_v7.txt"));
  EXPECT_EQ(kCpuHasNEON, ArmCpuCaps("unit_test/testdata/tegra3.txt"));
}

TEST_F(libyuvTest, TestVersion) {
  EXPECT_GE(LIBYUV_VERSION, 169);
}

}  // namespace libyuv

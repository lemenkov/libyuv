/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <string.h>

#include "../unit_test/unit_test.h"
#include "libyuv/basic_types.h"

namespace libyuv {

TEST_F(libyuvTest, Endian) {
  uint16 v16 = 0x1234u;
  uint8 first_byte = *reinterpret_cast<uint8*>(&v16);
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(0x34u, first_byte);
#elif defined(ARCH_CPU_BIG_ENDIAN)
  EXPECT_EQ(0x12u, first_byte);
#endif
}

TEST_F(libyuvTest, SizeOfTypes) {
  EXPECT_EQ(1u, sizeof(int8));  // NOLINT Using sizeof(type)
  EXPECT_EQ(1u, sizeof(uint8));  // NOLINT
  EXPECT_EQ(2u, sizeof(int16));  // NOLINT
  EXPECT_EQ(2u, sizeof(uint16));  // NOLINT
  EXPECT_EQ(4u, sizeof(int32));  // NOLINT
  EXPECT_EQ(4u, sizeof(uint32));  // NOLINT
  EXPECT_EQ(8u, sizeof(int64));  // NOLINT
  EXPECT_EQ(8u, sizeof(uint64));  // NOLINT
}

TEST_F(libyuvTest, SizeOfConstants) {
  EXPECT_EQ(8u, sizeof(INT64_C(0)));
  EXPECT_EQ(8u, sizeof(UINT64_C(0)));
  EXPECT_EQ(8u, sizeof(INT64_C(0x1234567887654321)));
  EXPECT_EQ(8u, sizeof(UINT64_C(0x8765432112345678)));
}

}  // namespace libyuv

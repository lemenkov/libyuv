/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

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
  int8 i8;
  uint8 u8;
  int16 i16;
  uint16 u16;
  int32 i32;
  uint32 u32;
  int64 i64;
  uint64 u64;
  EXPECT_EQ(1u, sizeof(i8));
  EXPECT_EQ(1u, sizeof(u8));
  EXPECT_EQ(2u, sizeof(i16));
  EXPECT_EQ(2u, sizeof(u16));
  EXPECT_EQ(4u, sizeof(i32));
  EXPECT_EQ(4u, sizeof(u32));
  EXPECT_EQ(8u, sizeof(i64));
  EXPECT_EQ(8u, sizeof(u64));
}

TEST_F(libyuvTest, SizeOfConstants) {
  EXPECT_EQ(8u, sizeof(INT64_C(0)));
  EXPECT_EQ(8u, sizeof(UINT64_C(0)));
  EXPECT_EQ(8u, sizeof(INT64_C(0x1234567887654321)));
  EXPECT_EQ(8u, sizeof(UINT64_C(0x8765432112345678)));
}

}  // namespace libyuv

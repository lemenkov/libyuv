/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <time.h>

#include "libyuv/convert_from.h"
#include "libyuv/cpu_id.h"
#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"
#include "unit_test/unit_test.h"

#if defined(_MSC_VER)
#define SIMD_ALIGNED(var) __declspec(align(16)) var
#else  // __GNUC__
#define SIMD_ALIGNED(var) var __attribute__((aligned(16)))
#endif

namespace libyuv {

TEST_F(libyuvTest, BenchmarkI420ToARGB_C) {
  align_buffer_16(src_y, benchmark_width_ * benchmark_height_);
  align_buffer_16(src_u, (benchmark_width_ * benchmark_height_) >> 2);
  align_buffer_16(src_v, (benchmark_width_ * benchmark_height_) >> 2);
  align_buffer_16(dst_argb, (benchmark_width_ << 2) * benchmark_height_);

  MaskCpuFlags(kCpuInitialized);

  for (int i = 0; i < benchmark_iterations_; ++i)
    I420ToARGB(src_y, benchmark_width_,
               src_u, benchmark_width_ >> 1,
               src_v, benchmark_width_ >> 1,
               dst_argb, benchmark_width_ << 2,
               benchmark_width_, benchmark_height_);

  MaskCpuFlags(-1);

  EXPECT_EQ(0, 0);

  free_aligned_buffer_16(src_y)
  free_aligned_buffer_16(src_u)
  free_aligned_buffer_16(src_v)
  free_aligned_buffer_16(dst_argb)
}

TEST_F(libyuvTest, BenchmarkI420ToARGB_OPT) {
  align_buffer_16(src_y, benchmark_width_ * benchmark_height_);
  align_buffer_16(src_u, (benchmark_width_ * benchmark_height_) >> 2);
  align_buffer_16(src_v, (benchmark_width_ * benchmark_height_) >> 2);
  align_buffer_16(dst_argb, (benchmark_width_ << 2) * benchmark_height_);

  for (int i = 0; i < benchmark_iterations_; ++i)
    I420ToARGB(src_y, benchmark_width_,
               src_u, benchmark_width_ >> 1,
               src_v, benchmark_width_ >> 1,
               dst_argb, benchmark_width_ << 2,
               benchmark_width_, benchmark_height_);

  free_aligned_buffer_16(src_y)
  free_aligned_buffer_16(src_u)
  free_aligned_buffer_16(src_v)
  free_aligned_buffer_16(dst_argb)
}

#define TESTI420TO(FMT, BPP)                                                   \
TEST_F(libyuvTest, I420To##FMT##_CvsOPT) {                                     \
  const int src_width = 1280;                                                  \
  const int src_height = 720;                                                  \
  align_buffer_16(src_y, src_width * src_height);                              \
  align_buffer_16(src_u, (src_width * src_height) >> 2);                       \
  align_buffer_16(src_v, (src_width * src_height) >> 2);                       \
  align_buffer_16(dst_rgb_c, (src_width * BPP) * src_height);                  \
  align_buffer_16(dst_rgb_opt, (src_width * BPP) * src_height);                \
  srandom(time(NULL));                                                         \
  for (int i = 0; i < src_height; ++i)                                         \
    for (int j = 0; j < src_width; ++j)                                        \
      src_y[(i * src_width) + j] = (random() & 0xff);                          \
  for (int i = 0; i < src_height >> 1; ++i)                                    \
    for (int j = 0; j < src_width >> 1; ++j) {                                 \
      src_u[(i * src_width >> 1) + j] = (random() & 0xff);                     \
      src_v[(i * src_width >> 1) + j] = (random() & 0xff);                     \
    }                                                                          \
  MaskCpuFlags(kCpuInitialized);                                               \
  I420To##FMT(src_y, src_width,                                                \
              src_u, src_width >> 1,                                           \
              src_v, src_width >> 1,                                           \
              dst_rgb_c, src_width * BPP,                                      \
              src_width, src_height);                                          \
  MaskCpuFlags(-1);                                                            \
  const int runs = 1000;                                                       \
  for (int i = 0; i < runs; ++i) {                                             \
    I420To##FMT(src_y, src_width,                                              \
                src_u, src_width >> 1,                                         \
                src_v, src_width >> 1,                                         \
                dst_rgb_opt, src_width * BPP,                                  \
                src_width, src_height);                                        \
  }                                                                            \
  int err = 0;                                                                 \
  for (int i = 0; i < src_height; ++i) {                                       \
    for (int j = 0; j < src_width * BPP; ++j) {                                \
      int diff = static_cast<int>(dst_rgb_c[i * src_width * BPP + j]) -        \
                 static_cast<int>(dst_rgb_opt[i * src_width * BPP + j]);       \
      if (abs(diff) > 2)                                                       \
        err++;                                                                 \
    }                                                                          \
  }                                                                            \
  EXPECT_EQ(err, 0);                                                           \
  free_aligned_buffer_16(src_y)                                                \
  free_aligned_buffer_16(src_u)                                                \
  free_aligned_buffer_16(src_v)                                                \
  free_aligned_buffer_16(dst_rgb_c)                                            \
  free_aligned_buffer_16(dst_rgb_opt)                                          \
}

TESTI420TO(ARGB, 4)
TESTI420TO(BGRA, 4)
TESTI420TO(ABGR, 4)
TESTI420TO(RAW, 3)
TESTI420TO(RGB24, 3)
TESTI420TO(RGB565, 2)
TESTI420TO(ARGB1555, 2)
TESTI420TO(ARGB4444, 2)

#define TESTATOB(FMT_A, BPP_A, FMT_B, BPP_B)                                   \
TEST_F(libyuvTest, ##FMT_A##To##FMT_B##_CvsOPT) {                              \
  const int src_width = 1280;                                                  \
  const int src_height = 720;                                                  \
  align_buffer_16(src_argb, src_width * src_height * BPP_A);                   \
  align_buffer_16(dst_rgb_c, (src_width * BPP_B) * src_height);                \
  align_buffer_16(dst_rgb_opt, (src_width * BPP_B) * src_height);              \
  srandom(time(NULL));                                                         \
  for (int i = 0; i < src_height; ++i)                                         \
    for (int j = 0; j < src_width * BPP_A; ++j)                                \
      src_argb[(i * src_width * BPP_A) + j] = (random() & 0xff);               \
  MaskCpuFlags(kCpuInitialized);                                               \
  ##FMT_A##To##FMT_B(src_argb, src_width * BPP_A,                              \
              dst_rgb_c, src_width * BPP_B,                                    \
              src_width, src_height);                                          \
  MaskCpuFlags(-1);                                                            \
  const int runs = 1000;                                                       \
  for (int i = 0; i < runs; ++i) {                                             \
    ##FMT_A##To##FMT_B(src_argb, src_width * BPP_A,                            \
                dst_rgb_opt, src_width * BPP_B,                                \
                src_width, src_height);                                        \
  }                                                                            \
  int err = 0;                                                                 \
  for (int i = 0; i < src_height; ++i) {                                       \
    for (int j = 0; j < src_width * BPP_B; ++j) {                              \
      int diff = static_cast<int>(dst_rgb_c[i * src_width * BPP_B + j]) -      \
                 static_cast<int>(dst_rgb_opt[i * src_width * BPP_B + j]);     \
      if (abs(diff) > 2)                                                       \
        err++;                                                                 \
    }                                                                          \
  }                                                                            \
  EXPECT_EQ(err, 0);                                                           \
  free_aligned_buffer_16(src_argb)                                             \
  free_aligned_buffer_16(dst_rgb_c)                                            \
  free_aligned_buffer_16(dst_rgb_opt)                                          \
}

// TODO(fbarchard): Expose more ARGBToRGB functions and test.
TESTATOB(ARGB, 4, BGRA, 4)
TESTATOB(ARGB, 4, ABGR, 4)
TESTATOB(ARGB, 4, RAW, 3)
TESTATOB(ARGB, 4, RGB24, 3)
TESTATOB(ARGB, 4, RGB565, 2)
//TESTATOB(ARGB, 4, ARGB1555, 2)
//TESTATOB(ARGB, 4, ARGB4444, 2)

TESTATOB(YUY2, 2, ARGB, 4)
TESTATOB(UYVY, 2, ARGB, 4)

TEST_F(libyuvTest, TestAttenuate) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);
  SIMD_ALIGNED(uint8 atten_pixels[256][4]);
  SIMD_ALIGNED(uint8 unatten_pixels[256][4]);
  SIMD_ALIGNED(uint8 atten2_pixels[256][4]);

  // Test unattenuation clamps
  orig_pixels[0][0] = 200u;
  orig_pixels[0][1] = 129u;
  orig_pixels[0][2] = 127u;
  orig_pixels[0][3] = 128u;
  // Test unattenuation transparent and opaque are unaffected
  orig_pixels[1][0] = 16u;
  orig_pixels[1][1] = 64u;
  orig_pixels[1][2] = 192u;
  orig_pixels[1][3] = 0u;
  orig_pixels[2][0] = 16u;
  orig_pixels[2][1] = 64u;
  orig_pixels[2][2] = 192u;
  orig_pixels[2][3] = 255u;
  orig_pixels[3][0] = 16u;
  orig_pixels[3][1] = 64u;
  orig_pixels[3][2] = 192u;
  orig_pixels[3][3] = 128u;
  ARGBUnattenuate(&orig_pixels[0][0], 0, &unatten_pixels[0][0], 0, 4, 1);
  EXPECT_EQ(255u, unatten_pixels[0][0]);
  EXPECT_EQ(255u, unatten_pixels[0][1]);
  EXPECT_EQ(254u, unatten_pixels[0][2]);
  EXPECT_EQ(128u, unatten_pixels[0][3]);
  EXPECT_EQ(16u, unatten_pixels[1][0]);
  EXPECT_EQ(64u, unatten_pixels[1][1]);
  EXPECT_EQ(192u, unatten_pixels[1][2]);
  EXPECT_EQ(0u, unatten_pixels[1][3]);
  EXPECT_EQ(16u, unatten_pixels[2][0]);
  EXPECT_EQ(64u, unatten_pixels[2][1]);
  EXPECT_EQ(192u, unatten_pixels[2][2]);
  EXPECT_EQ(255u, unatten_pixels[2][3]);
  EXPECT_EQ(32u, unatten_pixels[3][0]);
  EXPECT_EQ(128u, unatten_pixels[3][1]);
  EXPECT_EQ(255u, unatten_pixels[3][2]);
  EXPECT_EQ(128u, unatten_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }
  ARGBAttenuate(&orig_pixels[0][0], 0, &atten_pixels[0][0], 0, 256, 1);
  ARGBUnattenuate(&atten_pixels[0][0], 0, &unatten_pixels[0][0], 0, 256, 1);
  for (int i = 0; i < 1000 * 1280 * 720 / 256; ++i) {
    ARGBAttenuate(&unatten_pixels[0][0], 0, &atten2_pixels[0][0], 0, 256, 1);
  }
  for (int i = 0; i < 256; ++i) {
    EXPECT_NEAR(atten_pixels[i][0], atten2_pixels[i][0], 2);
    EXPECT_NEAR(atten_pixels[i][1], atten2_pixels[i][1], 2);
    EXPECT_NEAR(atten_pixels[i][2], atten2_pixels[i][2], 2);
    EXPECT_NEAR(atten_pixels[i][3], atten2_pixels[i][3], 2);
  }
  // Make sure transparent, 50% and opaque are fully accurate.
  EXPECT_EQ(0, atten_pixels[0][0]);
  EXPECT_EQ(0, atten_pixels[0][1]);
  EXPECT_EQ(0, atten_pixels[0][2]);
  EXPECT_EQ(0, atten_pixels[0][3]);
  EXPECT_EQ(64, atten_pixels[128][0]);
  EXPECT_EQ(32, atten_pixels[128][1]);
  EXPECT_EQ(21,  atten_pixels[128][2]);
  EXPECT_EQ(128, atten_pixels[128][3]);
  EXPECT_EQ(255, atten_pixels[255][0]);
  EXPECT_EQ(127, atten_pixels[255][1]);
  EXPECT_EQ(85,  atten_pixels[255][2]);
  EXPECT_EQ(255, atten_pixels[255][3]);
}

TEST_F(libyuvTest, TestAddRow) {
  SIMD_ALIGNED(uint8 orig_pixels[256]);
  SIMD_ALIGNED(uint16 added_pixels[256]);

  libyuv::AddRow AddRow = GetAddRow(added_pixels, 256);
  libyuv::AddRow SubRow = GetSubRow(added_pixels, 256);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i] = i;
  }
  memset(added_pixels, 0, sizeof(uint16) * 256);

  AddRow(orig_pixels, added_pixels, 256);
  EXPECT_EQ(7u, added_pixels[7]);
  EXPECT_EQ(250u, added_pixels[250]);
  AddRow(orig_pixels, added_pixels, 256);
  EXPECT_EQ(14u, added_pixels[7]);
  EXPECT_EQ(500u, added_pixels[250]);
  SubRow(orig_pixels, added_pixels, 256);
  EXPECT_EQ(7u, added_pixels[7]);
  EXPECT_EQ(250u, added_pixels[250]);

  for (int i = 0; i < 1000 * (1280 * 720 * 4 / 256); ++i) {
    AddRow(orig_pixels, added_pixels, 256);
  }
}

TEST_F(libyuvTest, TestARGBGray) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);

  // Test blue
  orig_pixels[0][0] = 255u;
  orig_pixels[0][1] = 0u;
  orig_pixels[0][2] = 0u;
  orig_pixels[0][3] = 128u;
  // Test green
  orig_pixels[1][0] = 0u;
  orig_pixels[1][1] = 255u;
  orig_pixels[1][2] = 0u;
  orig_pixels[1][3] = 0u;
  // Test red
  orig_pixels[2][0] = 0u;
  orig_pixels[2][1] = 0u;
  orig_pixels[2][2] = 255u;
  orig_pixels[2][3] = 255u;
  // Test color
  orig_pixels[3][0] = 16u;
  orig_pixels[3][1] = 64u;
  orig_pixels[3][2] = 192u;
  orig_pixels[3][3] = 224u;
  // Do 16 to test asm version.
  ARGBGray(&orig_pixels[0][0], 0, 0, 0, 16, 1);
  EXPECT_EQ(27u, orig_pixels[0][0]);
  EXPECT_EQ(27u, orig_pixels[0][1]);
  EXPECT_EQ(27u, orig_pixels[0][2]);
  EXPECT_EQ(128u, orig_pixels[0][3]);
  EXPECT_EQ(151u, orig_pixels[1][0]);
  EXPECT_EQ(151u, orig_pixels[1][1]);
  EXPECT_EQ(151u, orig_pixels[1][2]);
  EXPECT_EQ(0u, orig_pixels[1][3]);
  EXPECT_EQ(75u, orig_pixels[2][0]);
  EXPECT_EQ(75u, orig_pixels[2][1]);
  EXPECT_EQ(75u, orig_pixels[2][2]);
  EXPECT_EQ(255u, orig_pixels[2][3]);
  EXPECT_EQ(96u, orig_pixels[3][0]);
  EXPECT_EQ(96u, orig_pixels[3][1]);
  EXPECT_EQ(96u, orig_pixels[3][2]);
  EXPECT_EQ(224u, orig_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }

  for (int i = 0; i < 1000 * 1280 * 720 / 256; ++i) {
    ARGBGray(&orig_pixels[0][0], 0, 0, 0, 256, 1);
  }
}

TEST_F(libyuvTest, TestARGBSepia) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);

  // Test blue
  orig_pixels[0][0] = 255u;
  orig_pixels[0][1] = 0u;
  orig_pixels[0][2] = 0u;
  orig_pixels[0][3] = 128u;
  // Test green
  orig_pixels[1][0] = 0u;
  orig_pixels[1][1] = 255u;
  orig_pixels[1][2] = 0u;
  orig_pixels[1][3] = 0u;
  // Test red
  orig_pixels[2][0] = 0u;
  orig_pixels[2][1] = 0u;
  orig_pixels[2][2] = 255u;
  orig_pixels[2][3] = 255u;
  // Test color
  orig_pixels[3][0] = 16u;
  orig_pixels[3][1] = 64u;
  orig_pixels[3][2] = 192u;
  orig_pixels[3][3] = 224u;
  // Do 16 to test asm version.
  ARGBSepia(&orig_pixels[0][0], 0, 0, 0, 16, 1);
  EXPECT_EQ(33u, orig_pixels[0][0]);
  EXPECT_EQ(43u, orig_pixels[0][1]);
  EXPECT_EQ(47u, orig_pixels[0][2]);
  EXPECT_EQ(128u, orig_pixels[0][3]);
  EXPECT_EQ(135u, orig_pixels[1][0]);
  EXPECT_EQ(175u, orig_pixels[1][1]);
  EXPECT_EQ(195u, orig_pixels[1][2]);
  EXPECT_EQ(0u, orig_pixels[1][3]);
  EXPECT_EQ(69u, orig_pixels[2][0]);
  EXPECT_EQ(89u, orig_pixels[2][1]);
  EXPECT_EQ(99u, orig_pixels[2][2]);
  EXPECT_EQ(255u, orig_pixels[2][3]);
  EXPECT_EQ(88u, orig_pixels[3][0]);
  EXPECT_EQ(114u, orig_pixels[3][1]);
  EXPECT_EQ(127u, orig_pixels[3][2]);
  EXPECT_EQ(224u, orig_pixels[3][3]);

  for (int i = 0; i < 256; ++i) {
    orig_pixels[i][0] = i;
    orig_pixels[i][1] = i / 2;
    orig_pixels[i][2] = i / 3;
    orig_pixels[i][3] = i;
  }

  for (int i = 0; i < 1000 * 1280 * 720 / 256; ++i) {
    ARGBSepia(&orig_pixels[0][0], 0, 0, 0, 256, 1);
  }
}
}  // namespace libyuv

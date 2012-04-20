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

#define TESTI420TO(FMT)                                                        \
TEST_F(libyuvTest, I420To##FMT##_CvsOPT) {                                     \
  const int src_width = 1280;                                                  \
  const int src_height = 720;                                                  \
  align_buffer_16(src_y, src_width * src_height);                              \
  align_buffer_16(src_u, (src_width * src_height) >> 2);                       \
  align_buffer_16(src_v, (src_width * src_height) >> 2);                       \
  align_buffer_16(dst_rgb_c, (src_width << 2) * src_height);                   \
  align_buffer_16(dst_rgb_opt, (src_width << 2) * src_height);                 \
  srandom(time(NULL));                                                         \
  for (int i = 0; i < src_height; ++i)                                         \
    for (int j = 0; j < src_width; ++j)                                        \
      src_y[(i * src_height) + j] = (random() & 0xff);                         \
  for (int i = 0; i < src_height >> 1; ++i)                                    \
    for (int j = 0; j < src_width >> 1; ++j) {                                 \
      src_u[(i * src_height >> 1) + j] = (random() & 0xff);                    \
      src_v[(i * src_height >> 1) + j] = (random() & 0xff);                    \
    }                                                                          \
  MaskCpuFlags(kCpuInitialized);                                               \
  I420To##FMT(src_y, src_width,                                                \
              src_u, src_width >> 1,                                           \
              src_v, src_width >> 1,                                           \
              dst_rgb_c, src_width << 2,                                       \
              src_width, src_height);                                          \
  MaskCpuFlags(-1);                                                            \
  I420To##FMT(src_y, src_width,                                                \
              src_u, src_width >> 1,                                           \
              src_v, src_width >> 1,                                           \
              dst_rgb_opt, src_width << 2,                                     \
              src_width, src_height);                                          \
  int err = 0;                                                                 \
  for (int i = 0; i < src_height; ++i) {                                       \
    for (int j = 0; j < src_width << 2; ++j) {                                 \
      int diff = static_cast<int>(dst_rgb_c[i * src_height + j]) -             \
                 static_cast<int>(dst_rgb_opt[i * src_height + j]);            \
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

TESTI420TO(ARGB)
TESTI420TO(BGRA)
TESTI420TO(ABGR)

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
}

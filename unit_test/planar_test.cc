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

#define TESTPLANARTOB(FMT_PLANAR, SUBSAMP_X, SUBSAMP_Y, FMT_B, BPP_B)          \
TEST_F(libyuvTest, ##FMT_PLANAR##To##FMT_B##_OptVsC) {                         \
  const int kWidth = 1280;                                                     \
  const int kHeight = 720;                                                     \
  align_buffer_16(src_y, kWidth * kHeight);                                    \
  align_buffer_16(src_u, kWidth / SUBSAMP_X * kHeight / SUBSAMP_Y);            \
  align_buffer_16(src_v, kWidth / SUBSAMP_X * kHeight / SUBSAMP_Y);            \
  align_buffer_16(dst_argb_c, (kWidth * BPP_B) * kHeight);                     \
  align_buffer_16(dst_argb_opt, (kWidth * BPP_B) * kHeight);                   \
  srandom(time(NULL));                                                         \
  for (int i = 0; i < kHeight; ++i)                                            \
    for (int j = 0; j < kWidth; ++j)                                           \
      src_y[(i * kWidth) + j] = (random() & 0xff);                             \
  for (int i = 0; i < kHeight / SUBSAMP_X; ++i)                                \
    for (int j = 0; j < kWidth / SUBSAMP_Y; ++j) {                             \
      src_u[(i * kWidth / SUBSAMP_X) + j] = (random() & 0xff);                 \
      src_v[(i * kWidth / SUBSAMP_X) + j] = (random() & 0xff);                 \
    }                                                                          \
  MaskCpuFlags(kCpuInitialized);                                               \
  ##FMT_PLANAR##To##FMT_B(src_y, kWidth,                                       \
                          src_u, kWidth / SUBSAMP_X,                           \
                          src_v, kWidth / SUBSAMP_X,                           \
                          dst_argb_c, kWidth * BPP_B,                          \
                          kWidth, kHeight);                                    \
  MaskCpuFlags(-1);                                                            \
  const int runs = 1000;                                                       \
  for (int i = 0; i < runs; ++i) {                                             \
    ##FMT_PLANAR##To##FMT_B(src_y, kWidth,                                     \
                            src_u, kWidth / SUBSAMP_X,                         \
                            src_v, kWidth / SUBSAMP_X,                         \
                            dst_argb_opt, kWidth * BPP_B,                      \
                            kWidth, kHeight);                                  \
  }                                                                            \
  int err = 0;                                                                 \
  for (int i = 0; i < kHeight; ++i) {                                          \
    for (int j = 0; j < kWidth * BPP_B; ++j) {                                 \
      int diff = static_cast<int>(dst_argb_c[i * kWidth * BPP_B + j]) -        \
                 static_cast<int>(dst_argb_opt[i * kWidth * BPP_B + j]);       \
      if (abs(diff) > 2) {                                                     \
        ++err;                                                                 \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  EXPECT_EQ(err, 0);                                                           \
  free_aligned_buffer_16(src_y)                                                \
  free_aligned_buffer_16(src_u)                                                \
  free_aligned_buffer_16(src_v)                                                \
  free_aligned_buffer_16(dst_argb_c)                                           \
  free_aligned_buffer_16(dst_argb_opt)                                         \
}

TESTPLANARTOB(I420, 2, 2, ARGB, 4)
TESTPLANARTOB(I420, 2, 2, BGRA, 4)
TESTPLANARTOB(I420, 2, 2, ABGR, 4)
TESTPLANARTOB(I420, 2, 2, RAW, 3)
TESTPLANARTOB(I420, 2, 2, RGB24, 3)
TESTPLANARTOB(I420, 2, 2, RGB565, 2)
TESTPLANARTOB(I420, 2, 2, ARGB1555, 2)
TESTPLANARTOB(I420, 2, 2, ARGB4444, 2)
TESTPLANARTOB(I411, 4, 1, ARGB, 4)
TESTPLANARTOB(I422, 2, 1, ARGB, 4)
TESTPLANARTOB(I444, 1, 1, ARGB, 4)


#define TESTBIPLANARTOB(FMT_PLANAR, SUBSAMP_X, SUBSAMP_Y, FMT_B, BPP_B)        \
TEST_F(libyuvTest, ##FMT_PLANAR##To##FMT_B##_OptVsC) {                         \
  const int kWidth = 1280;                                                     \
  const int kHeight = 720;                                                     \
  align_buffer_16(src_y, kWidth * kHeight);                                    \
  align_buffer_16(src_uv, kWidth / SUBSAMP_X * kHeight / SUBSAMP_Y * 2);       \
  align_buffer_16(dst_argb_c, (kWidth * BPP_B) * kHeight);                     \
  align_buffer_16(dst_argb_opt, (kWidth * BPP_B) * kHeight);                   \
  srandom(time(NULL));                                                         \
  for (int i = 0; i < kHeight; ++i)                                            \
    for (int j = 0; j < kWidth; ++j)                                           \
      src_y[(i * kWidth) + j] = (random() & 0xff);                             \
  for (int i = 0; i < kHeight / SUBSAMP_X; ++i)                                \
    for (int j = 0; j < kWidth / SUBSAMP_Y * 2; ++j) {                         \
      src_uv[(i * kWidth / SUBSAMP_X) * 2 + j] = (random() & 0xff);            \
    }                                                                          \
  MaskCpuFlags(kCpuInitialized);                                               \
  ##FMT_PLANAR##To##FMT_B(src_y, kWidth,                                       \
                          src_uv, kWidth / SUBSAMP_X * 2,                      \
                          dst_argb_c, kWidth * BPP_B,                          \
                          kWidth, kHeight);                                    \
  MaskCpuFlags(-1);                                                            \
  const int runs = 1000;                                                       \
  for (int i = 0; i < runs; ++i) {                                             \
    ##FMT_PLANAR##To##FMT_B(src_y, kWidth,                                     \
                            src_uv, kWidth / SUBSAMP_X * 2,                    \
                            dst_argb_opt, kWidth * BPP_B,                      \
                            kWidth, kHeight);                                  \
  }                                                                            \
  int err = 0;                                                                 \
  for (int i = 0; i < kHeight; ++i) {                                          \
    for (int j = 0; j < kWidth * BPP_B; ++j) {                                 \
      int diff = static_cast<int>(dst_argb_c[i * kWidth * BPP_B + j]) -        \
                 static_cast<int>(dst_argb_opt[i * kWidth * BPP_B + j]);       \
      if (abs(diff) > 2) {                                                     \
        ++err;                                                                 \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  EXPECT_EQ(err, 0);                                                           \
  free_aligned_buffer_16(src_y)                                                \
  free_aligned_buffer_16(src_uv)                                               \
  free_aligned_buffer_16(dst_argb_c)                                           \
  free_aligned_buffer_16(dst_argb_opt)                                         \
}

TESTBIPLANARTOB(NV12, 2, 2, ARGB, 4)
TESTBIPLANARTOB(NV21, 2, 2, ARGB, 4)
TESTBIPLANARTOB(NV12, 2, 2, RGB565, 2)
TESTBIPLANARTOB(NV21, 2, 2, RGB565, 2)

#define TESTATOPLANAR(FMT_A, BPP_A, FMT_PLANAR, SUBSAMP_X, SUBSAMP_Y)          \
TEST_F(libyuvTest, ##FMT_A##To##FMT_PLANAR##_OptVsC) {                         \
  const int kWidth = 1280;                                                     \
  const int kHeight = 720;                                                     \
  align_buffer_16(src_argb, (kWidth * BPP_A) * kHeight);                       \
  align_buffer_16(dst_y_c, kWidth * kHeight);                                  \
  align_buffer_16(dst_u_c, kWidth / SUBSAMP_X * kHeight / SUBSAMP_Y);          \
  align_buffer_16(dst_v_c, kWidth / SUBSAMP_X * kHeight / SUBSAMP_Y);          \
  align_buffer_16(dst_y_opt, kWidth * kHeight);                                \
  align_buffer_16(dst_u_opt, kWidth / SUBSAMP_X * kHeight / SUBSAMP_Y);        \
  align_buffer_16(dst_v_opt, kWidth / SUBSAMP_X * kHeight / SUBSAMP_Y);        \
  srandom(time(NULL));                                                         \
  for (int i = 0; i < kHeight; ++i)                                            \
    for (int j = 0; j < kWidth * BPP_A; ++j)                                   \
      src_argb[(i * kWidth * BPP_A) + j] = (random() & 0xff);                  \
  MaskCpuFlags(kCpuInitialized);                                               \
  ##FMT_A##To##FMT_PLANAR(src_argb, kWidth * BPP_A,                            \
                          dst_y_c, kWidth,                                     \
                          dst_u_c, kWidth / SUBSAMP_X,                         \
                          dst_v_c, kWidth / SUBSAMP_X,                         \
                          kWidth, kHeight);                                    \
  MaskCpuFlags(-1);                                                            \
  const int runs = 1000;                                                       \
  for (int i = 0; i < runs; ++i) {                                             \
    ##FMT_A##To##FMT_PLANAR(src_argb, kWidth * BPP_A,                          \
                            dst_y_opt, kWidth,                                 \
                            dst_u_opt, kWidth / SUBSAMP_X,                     \
                            dst_v_opt, kWidth / SUBSAMP_X,                     \
                            kWidth, kHeight);                                  \
  }                                                                            \
  int err = 0;                                                                 \
  for (int i = 0; i < kHeight; ++i) {                                          \
    for (int j = 0; j < kWidth; ++j) {                                         \
      int diff = static_cast<int>(dst_y_c[i * kWidth + j]) -                   \
                 static_cast<int>(dst_y_opt[i * kWidth + j]);                  \
      if (abs(diff) > 2) {                                                     \
        ++err;                                                                 \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  EXPECT_EQ(err, 0);                                                           \
  for (int i = 0; i < kHeight / SUBSAMP_Y; ++i) {                              \
    for (int j = 0; j < kWidth / SUBSAMP_X; ++j) {                             \
      int diff = static_cast<int>(dst_u_c[i * kWidth / SUBSAMP_X + j]) -       \
                 static_cast<int>(dst_u_opt[i * kWidth / SUBSAMP_X + j]);      \
      if (abs(diff) > 2) {                                                     \
        ++err;                                                                 \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  EXPECT_EQ(err, 0);                                                           \
  for (int i = 0; i < kHeight / SUBSAMP_Y; ++i) {                              \
    for (int j = 0; j < kWidth / SUBSAMP_X; ++j) {                             \
      int diff = static_cast<int>(dst_v_c[i * kWidth / SUBSAMP_X + j]) -       \
                 static_cast<int>(dst_v_opt[i * kWidth / SUBSAMP_X + j]);      \
      if (abs(diff) > 2) {                                                     \
        ++err;                                                                 \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  EXPECT_EQ(err, 0);                                                           \
  free_aligned_buffer_16(dst_y_c)                                              \
  free_aligned_buffer_16(dst_u_c)                                              \
  free_aligned_buffer_16(dst_v_c)                                              \
  free_aligned_buffer_16(dst_y_opt)                                            \
  free_aligned_buffer_16(dst_u_opt)                                            \
  free_aligned_buffer_16(dst_v_opt)                                            \
  free_aligned_buffer_16(src_argb)                                             \
}

TESTATOPLANAR(ARGB, 4, I420, 2, 2)
TESTATOPLANAR(BGRA, 4, I420, 2, 2)
TESTATOPLANAR(ABGR, 4, I420, 2, 2)
TESTATOPLANAR(RAW, 3, I420, 2, 2)
TESTATOPLANAR(RGB24, 3, I420, 2, 2)
TESTATOPLANAR(RGB565, 2, I420, 2, 2)
TESTATOPLANAR(ARGB1555, 2, I420, 2, 2)
TESTATOPLANAR(ARGB4444, 2, I420, 2, 2)
//TESTATOPLANAR(ARGB, 4, I411, 4, 1)
TESTATOPLANAR(ARGB, 4, I422, 2, 1)
//TESTATOPLANAR(ARGB, 4, I444, 1, 1)
// TODO(fbarchard): Implement and test 411 and 444

#define TESTATOB(FMT_A, BPP_A, STRIDE_A, FMT_B, BPP_B)                         \
TEST_F(libyuvTest, ##FMT_A##To##FMT_B##_OptVsC) {                              \
  const int kWidth = 1280;                                                     \
  const int kHeight = 720;                                                     \
  align_buffer_16(src_argb, (kWidth * BPP_A) * kHeight);                       \
  align_buffer_16(dst_argb_c, (kWidth * BPP_B) * kHeight);                     \
  align_buffer_16(dst_argb_opt, (kWidth * BPP_B) * kHeight);                   \
  srandom(time(NULL));                                                         \
  for (int i = 0; i < kHeight * kWidth * BPP_A; ++i) {                         \
    src_argb[i] = (random() & 0xff);                                           \
  }                                                                            \
  MaskCpuFlags(kCpuInitialized);                                               \
  ##FMT_A##To##FMT_B(src_argb, kWidth * STRIDE_A,                              \
              dst_argb_c, kWidth * BPP_B,                                      \
              kWidth, kHeight);                                                \
  MaskCpuFlags(-1);                                                            \
  const int runs = 1000;                                                       \
  for (int i = 0; i < runs; ++i) {                                             \
    ##FMT_A##To##FMT_B(src_argb, kWidth * STRIDE_A,                            \
                dst_argb_opt, kWidth * BPP_B,                                  \
                kWidth, kHeight);                                              \
  }                                                                            \
  int err = 0;                                                                 \
  for (int i = 0; i < kHeight * kWidth * BPP_B; ++i) {                         \
    int diff = static_cast<int>(dst_argb_c[i]) -                               \
               static_cast<int>(dst_argb_opt[i]);                              \
    if (abs(diff) > 2)                                                         \
      err++;                                                                   \
  }                                                                            \
  EXPECT_EQ(err, 0);                                                           \
  free_aligned_buffer_16(src_argb)                                             \
  free_aligned_buffer_16(dst_argb_c)                                           \
  free_aligned_buffer_16(dst_argb_opt)                                         \
}

TESTATOB(ARGB, 4, 4, ARGB, 4)
TESTATOB(ARGB, 4, 4, BGRA, 4)
TESTATOB(ARGB, 4, 4, ABGR, 4)
TESTATOB(ARGB, 4, 4, RAW, 3)
TESTATOB(ARGB, 4, 4, RGB24, 3)
TESTATOB(ARGB, 4, 4, RGB565, 2)
TESTATOB(ARGB, 4, 4, ARGB1555, 2)
TESTATOB(ARGB, 4, 4, ARGB4444, 2)

TESTATOB(BGRA, 4, 4, ARGB, 4)
TESTATOB(ABGR, 4, 4, ARGB, 4)
TESTATOB(RAW, 3, 3, ARGB, 4)
TESTATOB(RGB24, 3, 3, ARGB, 4)
TESTATOB(RGB565, 2, 2, ARGB, 4)
TESTATOB(ARGB1555, 2, 2, ARGB, 4)
TESTATOB(ARGB4444, 2, 2, ARGB, 4)

TESTATOB(YUY2, 2, 2, ARGB, 4)
TESTATOB(UYVY, 2, 2, ARGB, 4)
TESTATOB(M420, 3 / 2, 1, ARGB, 4)

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

TEST_F(libyuvTest, TestARGBComputeCumulativeSum) {
  SIMD_ALIGNED(uint8 orig_pixels[16][16][4]);
  SIMD_ALIGNED(int32 added_pixels[16][16][4]);

  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      orig_pixels[y][x][0] = 1u;
      orig_pixels[y][x][1] = 2u;
      orig_pixels[y][x][2] = 3u;
      orig_pixels[y][x][3] = 255u;
    }
  }

  ARGBComputeCumulativeSum(&orig_pixels[0][0][0], 16 * 4,
                           &added_pixels[0][0][0], 16 * 4,
                           16, 16);

  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      EXPECT_EQ((x + 1) * (y + 1), added_pixels[y][x][0]);
      EXPECT_EQ((x + 1) * (y + 1) * 2, added_pixels[y][x][1]);
      EXPECT_EQ((x + 1) * (y + 1) * 3, added_pixels[y][x][2]);
      EXPECT_EQ((x + 1) * (y + 1) * 255, added_pixels[y][x][3]);
    }
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

TEST_F(libyuvTest, TestARGBColorMatrix) {
  SIMD_ALIGNED(uint8 orig_pixels[256][4]);

  // Matrix for Sepia.
  static const int8 kARGBToSepiaB[] = {
    17, 68, 35, 0,
    22, 88, 45, 0,
    24, 98, 50, 0,
  };

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
  ARGBColorMatrix(&orig_pixels[0][0], 0, &kARGBToSepiaB[0], 0, 0, 16, 1);
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
    ARGBColorMatrix(&orig_pixels[0][0], 0, &kARGBToSepiaB[0], 0, 0, 256, 1);
  }
}

}  // namespace libyuv

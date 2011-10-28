/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/cpu_id.h"
#include "libyuv/scale.h"
#include "unit_test.h"
#include <stdlib.h>
#include <time.h>

using namespace libyuv;

#define align_buffer_16(var, size) \
  uint8 *var; \
  uint8 *var##_mem; \
  var##_mem = reinterpret_cast<uint8*>(calloc(size+15, sizeof(uint8))); \
  var = reinterpret_cast<uint8*> \
        ((reinterpret_cast<intptr_t>(var##_mem) + 15) & (~0x0f));

#define free_aligned_buffer_16(var) \
  free(var##_mem);  \
  var = 0;

TEST_F(libyuvTest, ScaleDownBy4) {
  int b = 128;
  int src_width = 1280;
  int src_height = 720;
  int src_width_uv = (src_width + 1) >> 1;
  int src_height_uv = (src_height + 1) >> 1;

  int src_y_plane_size = (src_width + (2 * b)) * (src_height + (2 * b));
  int src_uv_plane_size = (src_width_uv + (2 * b)) * (src_height_uv + (2 * b));

  int src_stride_y = 2 * b + src_width;
  int src_stride_uv = 2 * b + src_width_uv;

  align_buffer_16(src_y, src_y_plane_size)
  align_buffer_16(src_u, src_uv_plane_size)
  align_buffer_16(src_v, src_uv_plane_size)

  int dst_width = src_width >> 2;
  int dst_height = src_height >> 2;

  int dst_width_uv = (dst_width + 1) >> 1;
  int dst_height_uv = (dst_height + 1) >> 1;

  int dst_y_plane_size = (dst_width + (2 * b)) * (dst_height + (2 * b));
  int dst_uv_plane_size = (dst_width_uv + (2 * b)) * (dst_height_uv + (2 * b));

  int dst_stride_y = 2 * b + dst_width;
  int dst_stride_uv = 2 * b + dst_width_uv;

  align_buffer_16(dst_y, dst_y_plane_size)
  align_buffer_16(dst_u, dst_uv_plane_size)
  align_buffer_16(dst_v, dst_uv_plane_size)

  // create an image with random data reoccurring in 4x4 grid.  When the image
  // is filtered all the values should be the same.
  srandom(time(NULL));

  uint8 block_data[16];

  int i, j;

  // Pulling 16 random numbers there is an infinitesimally small
  //  chance that they are all 0.  Then the output will be all 0.
  //  Output buffer is filled with 0, want to make sure that after the
  //  filtering something went into the output buffer.
  //  Avoid this by setting one of the values to 128.  Also set the
  //  random data to at least 1 for when point sampling to prevent
  //  output all being 0.
  block_data[0] = 128;

  for (i = 1; i < 16; i++)
    block_data[i] = (random() & 0xfe) + 1;

  for (i = b; i < (src_height + b); i += 4) {
    for (j = b; j < (src_width + b); j += 4) {
      uint8 *ptr = src_y + (i * src_stride_y) + j;
      int k, l;
      for (k = 0; k < 4; ++k)
        for (l = 0; l < 4; ++l)
          ptr[k + src_stride_y * l] = block_data[k + 4 * l];
    }
  }

  for (i = 1; i < 16; i++)
    block_data[i] = (random() & 0xfe) + 1;

  for (i = b; i < (src_height_uv + b); i += 4) {
    for (j = b; j < (src_width_uv + b); j += 4) {
      uint8 *ptru = src_u + (i * src_stride_uv) + j;
      uint8 *ptrv = src_v + (i * src_stride_uv) + j;
      int k, l;
      for (k = 0; k < 4; ++k)
        for (l = 0; l < 4; ++l) {
          ptru[k + src_stride_uv * l] = block_data[k + 4 * l];
          ptrv[k + src_stride_uv * l] = block_data[k + 4 * l];
        }
    }
  }

  int f;
  int err = 0;

  // currently three filter modes, defined as FilterMode in scale.h
  for (f = 0; f < 3; ++f) {
    I420Scale(src_y + (src_stride_y * b) + b, src_stride_y,
              src_u + (src_stride_uv * b) + b, src_stride_uv,
              src_v + (src_stride_uv * b) + b, src_stride_uv,
              src_width, src_height,
              dst_y + (dst_stride_y * b) + b, dst_stride_y,
              dst_u + (dst_stride_uv * b) + b, dst_stride_uv,
              dst_v + (dst_stride_uv * b) + b, dst_stride_uv,
              dst_width, dst_height,
              static_cast<FilterMode>(f));

    int value = dst_y[(dst_stride_y * b) + b];

    // catch the case that the output buffer is all 0
    if (value == 0)
      ++err;

    for (i = b; i < (dst_height + b); ++i) {
      for (j = b; j < (dst_width + b); ++j) {
        if (value != dst_y[(i * dst_stride_y) + j])
          ++err;
      }
    }

    value = dst_u[(dst_stride_uv * b) + b];

    if (value == 0)
      ++err;

    for (i = b; i < (dst_height_uv + b); ++i) {
      for (j = b; j < (dst_width_uv + b); ++j) {
        if (value != dst_u[(i * dst_stride_uv) + j])
          ++err;
        if (value != dst_v[(i * dst_stride_uv) + j])
          ++err;
      }
    }
  }

  free_aligned_buffer_16(src_y)
  free_aligned_buffer_16(src_u)
  free_aligned_buffer_16(src_v)
  free_aligned_buffer_16(dst_y)
  free_aligned_buffer_16(dst_u)
  free_aligned_buffer_16(dst_v)

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, ScaleDownBy34) {
  int b = 128;
  int src_width = 1280;
  int src_height = 720;
  int src_width_uv = (src_width + 1) >> 1;
  int src_height_uv = (src_height + 1) >> 1;

  int src_y_plane_size = (src_width + (2 * b)) * (src_height + (2 * b));
  int src_uv_plane_size = (src_width_uv + (2 * b)) * (src_height_uv + (2 * b));

  int src_stride_y = 2 * b + src_width;
  int src_stride_uv = 2 * b + src_width_uv;

  align_buffer_16(src_y, src_y_plane_size)
  align_buffer_16(src_u, src_uv_plane_size)
  align_buffer_16(src_v, src_uv_plane_size)

  int dst_width = (src_width*3) >> 2;
  int dst_height = (src_height*3) >> 2;

  int dst_width_uv = (dst_width + 1) >> 1;
  int dst_height_uv = (dst_height + 1) >> 1;

  int dst_y_plane_size = (dst_width + (2 * b)) * (dst_height + (2 * b));
  int dst_uv_plane_size = (dst_width_uv + (2 * b)) * (dst_height_uv + (2 * b));

  int dst_stride_y = 2 * b + dst_width;
  int dst_stride_uv = 2 * b + dst_width_uv;

  srandom(time(NULL));

  int i, j;

  for (i = b; i < (src_height + b); ++i) {
    for (j = b; j < (src_width + b); ++j) {
      src_y[(i * src_stride_y) + j] = (random() & 0xff);
    }
  }

  for (i = b; i < (src_height_uv + b); ++i) {
    for (j = b; j < (src_width_uv + b); ++j) {
      src_u[(i * src_stride_uv) + j] = (random() & 0xff);
      src_v[(i * src_stride_uv) + j] = (random() & 0xff);
    }
  }

  int f;
  int err = 0;

  // currently three filter modes, defined as FilterMode in scale.h
  for (f = 0; f < 3; ++f) {
    int max_diff = 0;
    align_buffer_16(dst_y_c, dst_y_plane_size)
    align_buffer_16(dst_u_c, dst_uv_plane_size)
    align_buffer_16(dst_v_c, dst_uv_plane_size)
    align_buffer_16(dst_y_opt, dst_y_plane_size)
    align_buffer_16(dst_u_opt, dst_uv_plane_size)
    align_buffer_16(dst_v_opt, dst_uv_plane_size)

    libyuv::MaskCpuFlagsForTest(0);
    I420Scale(src_y + (src_stride_y * b) + b, src_stride_y,
              src_u + (src_stride_uv * b) + b, src_stride_uv,
              src_v + (src_stride_uv * b) + b, src_stride_uv,
              src_width, src_height,
              dst_y_c + (dst_stride_y * b) + b, dst_stride_y,
              dst_u_c + (dst_stride_uv * b) + b, dst_stride_uv,
              dst_v_c + (dst_stride_uv * b) + b, dst_stride_uv,
              dst_width, dst_height,
              static_cast<FilterMode>(f));

    libyuv::MaskCpuFlagsForTest(-1);
    I420Scale(src_y + (src_stride_y * b) + b, src_stride_y,
              src_u + (src_stride_uv * b) + b, src_stride_uv,
              src_v + (src_stride_uv * b) + b, src_stride_uv,
              src_width, src_height,
              dst_y_opt + (dst_stride_y * b) + b, dst_stride_y,
              dst_u_opt + (dst_stride_uv * b) + b, dst_stride_uv,
              dst_v_opt + (dst_stride_uv * b) + b, dst_stride_uv,
              dst_width, dst_height,
              static_cast<FilterMode>(f));

    // C version may be a little off from the optimized.  Order of
    //  operations may introduce rounding somewhere.  So do a difference
    //  of the buffers and look to see that the max difference isn't
    //  over 2.
    for (i = b; i < (dst_height + b); ++i) {
      for (j = b; j < (dst_width + b); ++j) {
        int abs_diff = abs(dst_y_c[(i * dst_stride_y) + j] -
                           dst_y_opt[(i * dst_stride_y) + j]);
        if (abs_diff > max_diff)
          max_diff = abs_diff;
      }
    }

    for (i = b; i < (dst_height_uv + b); ++i) {
      for (j = b; j < (dst_width_uv + b); ++j) {
        int abs_diff = abs(dst_u_c[(i * dst_stride_uv) + j] -
                           dst_u_opt[(i * dst_stride_uv) + j]);
        if (abs_diff > max_diff)
          max_diff = abs_diff;
        abs_diff = abs(dst_v_c[(i * dst_stride_uv) + j] -
                       dst_v_opt[(i * dst_stride_uv) + j]);
        if (abs_diff > max_diff)
          max_diff = abs_diff;

      }
    }

    if (max_diff > 2)
      err++;

    free_aligned_buffer_16(dst_y_c)
    free_aligned_buffer_16(dst_u_c)
    free_aligned_buffer_16(dst_v_c)
    free_aligned_buffer_16(dst_y_opt)
    free_aligned_buffer_16(dst_u_opt)
    free_aligned_buffer_16(dst_v_opt)
  }

  free_aligned_buffer_16(src_y)
  free_aligned_buffer_16(src_u)
  free_aligned_buffer_16(src_v)

  EXPECT_EQ(0, err);
}

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

#include "libyuv/cpu_id.h"
#include "libyuv/scale_argb.h"
#include "unit_test/unit_test.h"

namespace libyuv {

static int ARGBTestFilter(int src_width, int src_height,
                      int dst_width, int dst_height,
                      FilterMode f) {

  int b = 128;

  int src_y_plane_size = (src_width + (2 * b)) * (src_height + (2 * b)) * 4;
  int src_stride_y = (2 * b + src_width) * 4;

  align_buffer_16(src_y, src_y_plane_size)

  int dst_y_plane_size = (dst_width + (2 * b)) * (dst_height + (2 * b)) * 4;
  int dst_stride_y = (2 * b + dst_width) * 4;

  srandom(time(NULL));

  int i, j;

  for (i = b; i < (src_height + b); ++i) {
    for (j = b; j < (src_width + b) * 4; ++j) {
      src_y[(i * src_stride_y) + j] = (random() & 0xff);
    }
  }

  const int runs = 1000;
  align_buffer_16(dst_y_c, dst_y_plane_size)
  align_buffer_16(dst_y_opt, dst_y_plane_size)

  MaskCpuFlags(kCpuInitialized);
  double c_time = get_time();

  for (i = 0; i < runs; ++i)
    ARGBScale(src_y + (src_stride_y * b) + b * 4, src_stride_y,
              src_width, src_height,
              dst_y_c + (dst_stride_y * b) + b * 4, dst_stride_y,
              dst_width, dst_height, f);

  c_time = (get_time() - c_time) / runs;

  MaskCpuFlags(-1);
  double opt_time = get_time();

  for (i = 0; i < runs; ++i)
    ARGBScale(src_y + (src_stride_y * b) + b * 4, src_stride_y,
              src_width, src_height,
              dst_y_opt + (dst_stride_y * b) + b * 4, dst_stride_y,
              dst_width, dst_height, f);

  opt_time = (get_time() - opt_time) / runs;

  printf ("filter %d - %8d us c - %8d us opt\n",
          f, (int)(c_time*1e6), (int)(opt_time*1e6));

  // C version may be a little off from the optimized.  Order of
  //  operations may introduce rounding somewhere.  So do a difference
  //  of the buffers and look to see that the max difference isn't
  //  over 2.
  int err = 0;
  int max_diff = 0;
  for (i = b; i < (dst_height + b); ++i) {
    for (j = b * 4; j < (dst_width + b) * 4; ++j) {
      int abs_diff = abs(dst_y_c[(i * dst_stride_y) + j] -
                         dst_y_opt[(i * dst_stride_y) + j]);
      if (abs_diff > max_diff)
        max_diff = abs_diff;
    }
  }

  if (max_diff > 2)
    err++;

  free_aligned_buffer_16(dst_y_c)
  free_aligned_buffer_16(dst_y_opt)
  free_aligned_buffer_16(src_y)
  return err;
}

TEST_F(libyuvTest, ARGBScaleDownBy2) {

  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width / 2;
  const int dst_height = src_height / 2;
  int err = 0;

  for (int f = 0; f < 1; ++f) {
    err += ARGBTestFilter(src_width, src_height,
                          dst_width, dst_height,
                          static_cast<FilterMode>(f));
  }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, ARGBScaleDownBy4) {

  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width / 4;
  const int dst_height = src_height / 4;
  int err = 0;

  for (int f = 0; f < 1; ++f) {
    err += ARGBTestFilter(src_width, src_height,
                          dst_width, dst_height,
                          static_cast<FilterMode>(f));
  }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, ARGBScaleDownBy34) {

  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width * 3 / 4;
  const int dst_height = src_height * 3 / 4;
  int err = 0;

  for (int f = 0; f < 1; ++f) {
    err += ARGBTestFilter(src_width, src_height,
                          dst_width, dst_height,
                          static_cast<FilterMode>(f));
  }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, ARGBScaleDownBy38) {
  int src_width = 1280;
  int src_height = 720;
  int dst_width = src_width * 3 / 8;
  int dst_height = src_height * 3 / 8;
  int err = 0;

  for (int f = 0; f < 1; ++f) {
    err += ARGBTestFilter(src_width, src_height,
                          dst_width, dst_height,
                          static_cast<FilterMode>(f));
  }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, ARGBScalePlaneBilinear) {
  int src_width = 1280;
  int src_height = 720;
  int dst_width = 1366;
  int dst_height = 768;
  int err = 0;

  for (int f = 0; f < 1; ++f) {
    err += ARGBTestFilter(src_width, src_height,
                          dst_width, dst_height,
                          static_cast<FilterMode>(f));
  }

  EXPECT_EQ(0, err);
}

}  // namespace libyuv

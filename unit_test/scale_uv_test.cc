/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <time.h>

#include "../unit_test/unit_test.h"
#include "libyuv/cpu_id.h"
#include "libyuv/scale_uv.h"
#include "libyuv/video_common.h"

namespace libyuv {

#define STRINGIZE(line) #line
#define FILELINESTR(file, line) file ":" STRINGIZE(line)

// Test scaling with C vs Opt and return maximum pixel difference. 0 = exact.
static int UVTestFilter(int src_width,
                        int src_height,
                        int dst_width,
                        int dst_height,
                        FilterMode f,
                        int benchmark_iterations,
                        int disable_cpu_flags,
                        int benchmark_cpu_info) {
  if (!SizeValid(src_width, src_height, dst_width, dst_height)) {
    return 0;
  }

  int i, j;
  const int b = 0;  // 128 to test for padding/stride.
  int64_t src_uv_plane_size =
      (Abs(src_width) + b * 2) * (Abs(src_height) + b * 2) * 2LL;
  int src_stride_uv = (b * 2 + Abs(src_width)) * 2;

  align_buffer_page_end(src_uv, src_uv_plane_size);
  if (!src_uv) {
    printf("Skipped.  Alloc failed " FILELINESTR(__FILE__, __LINE__) "\n");
    return 0;
  }
  MemRandomize(src_uv, src_uv_plane_size);

  int64_t dst_uv_plane_size = (dst_width + b * 2) * (dst_height + b * 2) * 2LL;
  int dst_stride_uv = (b * 2 + dst_width) * 2;

  align_buffer_page_end(dst_uv_c, dst_uv_plane_size);
  align_buffer_page_end(dst_uv_opt, dst_uv_plane_size);
  if (!dst_uv_c || !dst_uv_opt) {
    printf("Skipped.  Alloc failed " FILELINESTR(__FILE__, __LINE__) "\n");
    return 0;
  }
  memset(dst_uv_c, 2, dst_uv_plane_size);
  memset(dst_uv_opt, 3, dst_uv_plane_size);

  // Warm up both versions for consistent benchmarks.
  MaskCpuFlags(disable_cpu_flags);  // Disable all CPU optimization.
  UVScale(src_uv + (src_stride_uv * b) + b * 2, src_stride_uv, src_width,
          src_height, dst_uv_c + (dst_stride_uv * b) + b * 2, dst_stride_uv,
          dst_width, dst_height, f);
  MaskCpuFlags(benchmark_cpu_info);  // Enable all CPU optimization.
  UVScale(src_uv + (src_stride_uv * b) + b * 2, src_stride_uv, src_width,
          src_height, dst_uv_opt + (dst_stride_uv * b) + b * 2, dst_stride_uv,
          dst_width, dst_height, f);

  MaskCpuFlags(disable_cpu_flags);  // Disable all CPU optimization.
  double c_time = get_time();
  UVScale(src_uv + (src_stride_uv * b) + b * 2, src_stride_uv, src_width,
          src_height, dst_uv_c + (dst_stride_uv * b) + b * 2, dst_stride_uv,
          dst_width, dst_height, f);

  c_time = (get_time() - c_time);

  MaskCpuFlags(benchmark_cpu_info);  // Enable all CPU optimization.
  double opt_time = get_time();
  for (i = 0; i < benchmark_iterations; ++i) {
    UVScale(src_uv + (src_stride_uv * b) + b * 2, src_stride_uv, src_width,
            src_height, dst_uv_opt + (dst_stride_uv * b) + b * 2, dst_stride_uv,
            dst_width, dst_height, f);
  }
  opt_time = (get_time() - opt_time) / benchmark_iterations;

  // Report performance of C vs OPT
  printf("filter %d - %8d us C - %8d us OPT\n", f,
         static_cast<int>(c_time * 1e6), static_cast<int>(opt_time * 1e6));

  // C version may be a little off from the optimized. Order of
  //  operations may introduce rounding somewhere. So do a difference
  //  of the buffers and look to see that the max difference isn't
  //  over 2.
  int max_diff = 0;
  for (i = b; i < (dst_height + b); ++i) {
    for (j = b * 2; j < (dst_width + b) * 2; ++j) {
      int abs_diff = Abs(dst_uv_c[(i * dst_stride_uv) + j] -
                         dst_uv_opt[(i * dst_stride_uv) + j]);
      if (abs_diff > max_diff) {
        max_diff = abs_diff;
      }
    }
  }

  free_aligned_buffer_page_end(dst_uv_c);
  free_aligned_buffer_page_end(dst_uv_opt);
  free_aligned_buffer_page_end(src_uv);
  return max_diff;
}

#define TEST_SCALETO1(name, width, height, filter, max_diff)                \
  TEST_F(LibYUVScaleTest, name##To##width##x##height##_##filter) {          \
    int diff = UVTestFilter(benchmark_width_, benchmark_height_, width,     \
                            height, kFilter##filter, benchmark_iterations_, \
                            disable_cpu_flags_, benchmark_cpu_info_);       \
    EXPECT_LE(diff, max_diff);                                              \
  }                                                                         \
  TEST_F(LibYUVScaleTest, name##From##width##x##height##_##filter) {        \
    int diff = UVTestFilter(width, height, Abs(benchmark_width_),           \
                            Abs(benchmark_height_), kFilter##filter,        \
                            benchmark_iterations_, disable_cpu_flags_,      \
                            benchmark_cpu_info_);                           \
    EXPECT_LE(diff, max_diff);                                              \
  }

/// Test scale to a specified size with all 3 filters.
#define TEST_SCALETO(name, width, height)       \
  TEST_SCALETO1(name, width, height, None, 0)   \
  TEST_SCALETO1(name, width, height, Linear, 3) \
  TEST_SCALETO1(name, width, height, Bilinear, 3)

TEST_SCALETO(UVScale, 1, 1)
TEST_SCALETO(UVScale, 320, 240)
TEST_SCALETO(UVScale, 569, 480)
TEST_SCALETO(UVScale, 640, 360)
#ifdef ENABLE_SLOW_TESTS
TEST_SCALETO(UVScale, 1280, 720)
TEST_SCALETO(UVScale, 1920, 1080)
#endif  // ENABLE_SLOW_TESTS
#undef TEST_SCALETO1
#undef TEST_SCALETO

}  // namespace libyuv

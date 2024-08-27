/*
 *  Copyright 2024 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/scale_row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(LIBYUV_DISABLE_SME) && defined(CLANG_HAS_SME) && \
    defined(__aarch64__)

__arm_locally_streaming void ScaleRowDown2_SME(const uint8_t* src_ptr,
                                               ptrdiff_t src_stride,
                                               uint8_t* dst,
                                               int dst_width) {
  // Streaming-SVE only, no use of ZA tile.
  (void)src_stride;
  int vl;
  asm volatile(
      "cntb     %x[vl]                                       \n"
      "subs     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "b.lt     2f                                           \n"

      "1:                                                    \n"
      "ptrue    p0.b                                         \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_ptr]]             \n"
      "incb     %[src_ptr], all, mul #2                      \n"
      "subs     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "st1b     {z1.b}, p0, [%[dst_ptr]]                     \n"
      "incb     %[dst_ptr]                                   \n"
      "b.ge     1b                                           \n"

      "2:                                                    \n"
      "adds     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "b.eq     99f                                          \n"

      "whilelt  p0.b, wzr, %w[dst_width]                     \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_ptr]]             \n"
      "st1b     {z1.b}, p0, [%[dst_ptr]]                     \n"

      "99:                                                   \n"
      : [src_ptr] "+r"(src_ptr),      // %[src_ptr]
        [dst_ptr] "+r"(dst),          // %[dst_ptr]
        [dst_width] "+r"(dst_width),  // %[dst_width]
        [vl] "=r"(vl)                 // %[vl]
      :
      : "memory", "cc", "z0", "z1", "p0");
}

#endif  // !defined(LIBYUV_DISABLE_SME) && defined(CLANG_HAS_SME) &&
        // defined(__aarch64__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

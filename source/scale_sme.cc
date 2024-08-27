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

__arm_locally_streaming void ScaleRowDown2Linear_SME(const uint8_t* src_ptr,
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
      "urhadd   z0.b, p0/m, z0.b, z1.b                       \n"
      "subs     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "st1b     {z0.b}, p0, [%[dst_ptr]]                     \n"
      "incb     %[dst_ptr]                                   \n"
      "b.ge     1b                                           \n"

      "2:                                                    \n"
      "adds     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "b.eq     99f                                          \n"

      "whilelt  p0.b, wzr, %w[dst_width]                     \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_ptr]]             \n"
      "urhadd   z0.b, p0/m, z0.b, z1.b                       \n"
      "st1b     {z0.b}, p0, [%[dst_ptr]]                     \n"

      "99:                                                   \n"
      : [src_ptr] "+r"(src_ptr),      // %[src_ptr]
        [dst_ptr] "+r"(dst),          // %[dst_ptr]
        [dst_width] "+r"(dst_width),  // %[dst_width]
        [vl] "=r"(vl)                 // %[vl]
      :
      : "memory", "cc", "z0", "z1", "p0");
}

#define SCALEROWDOWN2BOX_SVE                                 \
  "ld2b     {z0.b, z1.b}, p0/z, [%[src_ptr]]             \n" \
  "ld2b     {z2.b, z3.b}, p0/z, [%[src2_ptr]]            \n" \
  "incb     %[src_ptr], all, mul #2                      \n" \
  "incb     %[src2_ptr], all, mul #2                     \n" \
  "uaddlb   z4.h, z0.b, z1.b                             \n" \
  "uaddlt   z5.h, z0.b, z1.b                             \n" \
  "uaddlb   z6.h, z2.b, z3.b                             \n" \
  "uaddlt   z7.h, z2.b, z3.b                             \n" \
  "add      z4.h, z4.h, z6.h                             \n" \
  "add      z5.h, z5.h, z7.h                             \n" \
  "rshrnb   z0.b, z4.h, #2                               \n" \
  "rshrnt   z0.b, z5.h, #2                               \n" \
  "subs     %w[dst_width], %w[dst_width], %w[vl]         \n" \
  "st1b     {z0.b}, p0, [%[dst_ptr]]                     \n" \
  "incb     %[dst_ptr]                                   \n"

__arm_locally_streaming void ScaleRowDown2Box_SME(const uint8_t* src_ptr,
                                                  ptrdiff_t src_stride,
                                                  uint8_t* dst,
                                                  int dst_width) {
  // Streaming-SVE only, no use of ZA tile.
  const uint8_t* src2_ptr = src_ptr + src_stride;
  int vl;
  asm volatile(
      "cntb     %x[vl]                                       \n"
      "subs     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "b.lt     2f                                           \n"

      "ptrue    p0.b                                         \n"
      "1:                                                    \n"  //
      SCALEROWDOWN2BOX_SVE
      "b.ge     1b                                           \n"

      "2:                                                    \n"
      "adds     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "b.eq     99f                                          \n"

      "whilelt  p0.b, wzr, %w[dst_width]                     \n"  //
      SCALEROWDOWN2BOX_SVE

      "99:                                                   \n"
      : [src_ptr] "+r"(src_ptr),      // %[src_ptr]
        [src2_ptr] "+r"(src2_ptr),    // %[src2_ptr]
        [dst_ptr] "+r"(dst),          // %[dst_ptr]
        [dst_width] "+r"(dst_width),  // %[dst_width]
        [vl] "=r"(vl)                 // %[vl]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "p0");
}

#undef SCALEROWDOWN2BOX_SVE

__arm_locally_streaming void ScaleUVRowDown2_SME(const uint8_t* src_uv,
                                                 ptrdiff_t src_stride,
                                                 uint8_t* dst_uv,
                                                 int dst_width) {
  // Streaming-SVE only, no use of ZA tile.
  (void)src_stride;
  int vl;
  asm volatile(
      "cnth     %x[vl]                                       \n"
      "subs     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "b.lt     2f                                           \n"

      "1:                                                    \n"
      "ptrue    p0.b                                         \n"
      "ld2h     {z0.h, z1.h}, p0/z, [%[src_uv]]              \n"
      "incb     %[src_uv], all, mul #2                       \n"
      "subs     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "st1h     {z1.h}, p0, [%[dst_uv]]                      \n"
      "incb     %[dst_uv]                                    \n"
      "b.ge     1b                                           \n"

      "2:                                                    \n"
      "adds     %w[dst_width], %w[dst_width], %w[vl]         \n"
      "b.eq     99f                                          \n"

      "whilelt  p0.h, wzr, %w[dst_width]                     \n"
      "ld2h     {z0.h, z1.h}, p0/z, [%[src_uv]]              \n"
      "st1h     {z1.h}, p0, [%[dst_uv]]                      \n"

      "99:                                                   \n"
      : [src_uv] "+r"(src_uv),        // %[src_uv]
        [dst_uv] "+r"(dst_uv),        // %[dst_uv]
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

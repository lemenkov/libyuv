/*
 *  Copyright 2024 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/compare_row.h"
#include "libyuv/row.h"

#if !defined(LIBYUV_DISABLE_SME) && \
    (defined(CLANG_HAS_SME) || defined(GCC_HAS_SME)) && defined(__aarch64__)
#if defined(__clang__)
#pragma clang attribute push( \
    __attribute__((target("arch=armv8.5-a+i8mm+sme"))), apply_to = function)
#elif defined(__GNUC__)
#pragma GCC push_options
#pragma GCC target("arch=armv8.5-a+i8mm+sve2+sme")
#endif
#endif

#include "libyuv/row_sve.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Note: every register written by an inline asm block (vector, general
// purpose, mask/predicate, "cc" for flags and "memory" for stores) must be
// listed in its clobber list, even when the ABI treats it as caller-saved.
// With LTO/LTCG the compiler may keep values live in any register across the
// asm statement, and other OS ABIs (e.g. Windows) differ in which registers
// are callee-saved. List "memory" first, then "cc", then registers.

#if !defined(LIBYUV_DISABLE_SME) && \
    (defined(CLANG_HAS_SME) || defined(GCC_HAS_SME)) && defined(__aarch64__)

__arm_locally_streaming void I444ToARGBRow_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  // Streaming-SVE only, no use of ZA tile.
  I444ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I444ToRGB24Row_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_rgb24,
    const struct YuvConstants* yuvconstants,
    int width) {
  I444ToRGB24Row_SVE_SC(src_y, src_u, src_v, dst_rgb24, yuvconstants, width);
}

__arm_locally_streaming void I400ToARGBRow_SME(
    const uint8_t* src_y,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  // Streaming-SVE only, no use of ZA tile.
  I400ToARGBRow_SVE_SC(src_y, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I422ToARGBRow_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  // Streaming-SVE only, no use of ZA tile.
  I422ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I422ToRGB24Row_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I422ToRGB24Row_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I422ToRGB565Row_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_rgb565,
    const struct YuvConstants* yuvconstants,
    int width) {
  I422ToRGB565Row_SVE_SC(src_y, src_u, src_v, dst_rgb565, yuvconstants, width);
}

__arm_locally_streaming void I422ToARGB1555Row_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb1555,
    const struct YuvConstants* yuvconstants,
    int width) {
  I422ToARGB1555Row_SVE_SC(src_y, src_u, src_v, dst_argb1555, yuvconstants,
                           width);
}

__arm_locally_streaming void I422ToARGB4444Row_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb4444,
    const struct YuvConstants* yuvconstants,
    int width) {
  I422ToARGB4444Row_SVE_SC(src_y, src_u, src_v, dst_argb4444, yuvconstants,
                           width);
}

__arm_locally_streaming void I422ToRGBARow_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I422ToRGBARow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I422ToAR30Row_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I422ToAR30Row_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I422AlphaToARGBRow_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    const uint8_t* src_a,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I422AlphaToARGBRow_SVE_SC(src_y, src_u, src_v, src_a, dst_argb, yuvconstants,
                            width);
}

__arm_locally_streaming void I444AlphaToARGBRow_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    const uint8_t* src_a,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I444AlphaToARGBRow_SVE_SC(src_y, src_u, src_v, src_a, dst_argb, yuvconstants,
                            width);
}

__arm_locally_streaming void NV12ToARGBRow_SME(
    const uint8_t* src_y,
    const uint8_t* src_uv,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  NV12ToARGBRow_SVE_SC(src_y, src_uv, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void NV21ToARGBRow_SME(
    const uint8_t* src_y,
    const uint8_t* src_vu,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  NV21ToARGBRow_SVE_SC(src_y, src_vu, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void NV12ToRGB24Row_SME(
    const uint8_t* src_y,
    const uint8_t* src_uv,
    uint8_t* dst_rgb24,
    const struct YuvConstants* yuvconstants,
    int width) {
  NV12ToRGB24Row_SVE_SC(src_y, src_uv, dst_rgb24, yuvconstants, width);
}

__arm_locally_streaming void NV21ToRGB24Row_SME(
    const uint8_t* src_y,
    const uint8_t* src_vu,
    uint8_t* dst_rgb24,
    const struct YuvConstants* yuvconstants,
    int width) {
  NV21ToRGB24Row_SVE_SC(src_y, src_vu, dst_rgb24, yuvconstants, width);
}

__arm_locally_streaming void YUY2ToARGBRow_SME(
    const uint8_t* src_yuy2,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  YUY2ToARGBRow_SVE_SC(src_yuy2, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void UYVYToARGBRow_SME(
    const uint8_t* src_uyvy,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  UYVYToARGBRow_SVE_SC(src_uyvy, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I210ToARGBRow_SME(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I210ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I210AlphaToARGBRow_SME(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    const uint16_t* src_a,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I210AlphaToARGBRow_SVE_SC(src_y, src_u, src_v, src_a, dst_argb, yuvconstants,
                            width);
}

__arm_locally_streaming void I210ToAR30Row_SME(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    uint8_t* dst_ar30,
    const struct YuvConstants* yuvconstants,
    int width) {
  I210ToAR30Row_SVE_SC(src_y, src_u, src_v, dst_ar30, yuvconstants, width);
}

__arm_locally_streaming void P210ToARGBRow_SME(
    const uint16_t* src_y,
    const uint16_t* src_uv,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  P210ToARGBRow_SVE_SC(src_y, src_uv, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void P210ToAR30Row_SME(
    const uint16_t* src_y,
    const uint16_t* src_uv,
    uint8_t* dst_ar30,
    const struct YuvConstants* yuvconstants,
    int width) {
  P210ToAR30Row_SVE_SC(src_y, src_uv, dst_ar30, yuvconstants, width);
}

__arm_locally_streaming void I410ToARGBRow_SME(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I410ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void I410AlphaToARGBRow_SME(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    const uint16_t* src_a,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I410AlphaToARGBRow_SVE_SC(src_y, src_u, src_v, src_a, dst_argb, yuvconstants,
                            width);
}

__arm_locally_streaming void I410ToAR30Row_SME(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    uint8_t* dst_ar30,
    const struct YuvConstants* yuvconstants,
    int width) {
  I410ToAR30Row_SVE_SC(src_y, src_u, src_v, dst_ar30, yuvconstants, width);
}

__arm_locally_streaming void P410ToARGBRow_SME(
    const uint16_t* src_y,
    const uint16_t* src_uv,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  P410ToARGBRow_SVE_SC(src_y, src_uv, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void P410ToAR30Row_SME(
    const uint16_t* src_y,
    const uint16_t* src_uv,
    uint8_t* dst_ar30,
    const struct YuvConstants* yuvconstants,
    int width) {
  P410ToAR30Row_SVE_SC(src_y, src_uv, dst_ar30, yuvconstants, width);
}

__arm_locally_streaming void I212ToAR30Row_SME(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    uint8_t* dst_ar30,
    const struct YuvConstants* yuvconstants,
    int width) {
  I212ToAR30Row_SVE_SC(src_y, src_u, src_v, dst_ar30, yuvconstants, width);
}

__arm_locally_streaming void I212ToARGBRow_SME(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  I212ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

__arm_locally_streaming void MultiplyRow_16_SME(const uint16_t* src_y,
                                                uint16_t* dst_y,
                                                int scale,
                                                int width) {
  // Streaming-SVE only, no use of ZA tile.
  int vl;
  asm volatile(
      "cnth    %x[vl]                                   \n"
      "mov     z0.h, %w[scale]                          \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "b.lt    2f                                       \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue   p0.h                                     \n"
      "1:                                               \n"
      "ld1h    {z1.h}, p0/z, [%[src_y]]                 \n"
      "incb    %[src_y]                                 \n"
      "mul     z1.h, z0.h, z1.h                         \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "st1h    {z1.h}, p0, [%[dst_y]]                   \n"
      "incb    %[dst_y]                                 \n"
      "b.ge    1b                                       \n"

      "2:                                               \n"
      "adds    %w[width], %w[width], %w[vl]             \n"
      "b.eq    99f                                      \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt p0.h, wzr, %w[width]                     \n"
      "ld1h    {z1.h}, p0/z, [%[src_y]]                 \n"
      "mul     z1.h, z0.h, z1.h                         \n"
      "st1h    {z1.h}, p0, [%[dst_y]]                   \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),  // %[src_y]
        [dst_y] "+r"(dst_y),  // %[dst_y]
        [width] "+r"(width),  // %[width]
        [vl] "=&r"(vl)        // %[vl]
      : [scale] "r"(scale)    // %[scale]
      : "memory", "cc", "z0", "z1", "p0");
}

__arm_locally_streaming void ARGBMultiplyRow_SME(const uint8_t* src_argb,
                                                 const uint8_t* src_argb1,
                                                 uint8_t* dst_argb,
                                                 int width) {
  // Streaming-SVE only, no use of ZA tile.
  width *= 4;
  int vl;
  asm volatile(
      "cntb    %x[vl]                                   \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "b.lt    2f                                       \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue   p0.b                                     \n"
      "1:                                               \n"
      "ld1b    {z0.b}, p0/z, [%[src_argb]]              \n"
      "ld1b    {z1.b}, p0/z, [%[src_argb1]]             \n"
      "incb    %[src_argb]                              \n"
      "incb    %[src_argb1]                             \n"
      "umullb  z2.h, z0.b, z1.b                         \n"
      "umullt  z1.h, z0.b, z1.b                         \n"
      "rshrnb  z0.b, z2.h, #8                           \n"
      "rshrnt  z0.b, z1.h, #8                           \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "st1b    {z0.b}, p0, [%[dst_argb]]                \n"
      "incb    %[dst_argb]                              \n"
      "b.ge    1b                                       \n"

      "2:                                               \n"
      "adds    %w[width], %w[width], %w[vl]             \n"
      "b.eq    99f                                      \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt p0.b, wzr, %w[width]                     \n"
      "ld1b    {z0.b}, p0/z, [%[src_argb]]              \n"
      "ld1b    {z1.b}, p0/z, [%[src_argb1]]             \n"
      "umullb  z2.h, z0.b, z1.b                         \n"
      "umullt  z1.h, z0.b, z1.b                         \n"
      "rshrnb  z0.b, z2.h, #8                           \n"
      "rshrnt  z0.b, z1.h, #8                           \n"
      "st1b    {z0.b}, p0, [%[dst_argb]]                \n"

      "99:                                              \n"
      : [src_argb] "+r"(src_argb),    // %[src_argb]
        [src_argb1] "+r"(src_argb1),  // %[src_argb1]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [width] "+r"(width),          // %[width]
        [vl] "=&r"(vl)                // %[vl]
      :
      : "memory", "cc", "z0", "z1", "z2", "p0");
}

__arm_locally_streaming void MergeUVRow_SME(const uint8_t* src_u,
                                            const uint8_t* src_v,
                                            uint8_t* dst_uv,
                                            int width) {
  MergeUVRow_SVE_SC(src_u, src_v, dst_uv, width);
}

__arm_locally_streaming void SplitUVRow_SME(const uint8_t* src_uv,
                                            uint8_t* dst_u,
                                            uint8_t* dst_v,
                                            int width) {
  SplitUVRow_SVE_SC(src_uv, dst_u, dst_v, width);
}

__arm_locally_streaming void HalfMergeUVRow_SME(const uint8_t* src_u,
                                                int src_stride_u,
                                                const uint8_t* src_v,
                                                int src_stride_v,
                                                uint8_t* dst_uv,
                                                int width) {
  HalfMergeUVRow_SVE_SC(src_u, src_stride_u, src_v, src_stride_v, dst_uv,
                        width);
}

#ifdef HAS_HALFUVROW_SME
__arm_locally_streaming void HalfUVRow_SME(const uint8_t* src_u,
                                           ptrdiff_t src_stride_u,
                                           const uint8_t* src_v,
                                           ptrdiff_t src_stride_v,
                                           uint8_t* dst_uv,
                                           int width) {
  HalfUVRow_SVE_SC(src_u, src_stride_u, src_v, src_stride_v, dst_uv, width);
}
#endif  // HAS_HALFUVROW_SME

__arm_locally_streaming void MergeUVRow_16_SME(const uint16_t* src_u,
                                               const uint16_t* src_v,
                                               uint16_t* dst_uv,
                                               int depth,
                                               int width) {
  int shift = 16 - depth;
  // Streaming-SVE only, no use of ZA tile.
  int vl;
  asm volatile(
      "cnth    %x[vl]                                   \n"
      "mov     z0.h, %w[shift]                          \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "b.lt    2f                                       \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue   p0.h                                     \n"
      "1:                                               \n"
      "ld1h    {z1.h}, p0/z, [%[src_u]]                 \n"
      "ld1h    {z2.h}, p0/z, [%[src_v]]                 \n"
      "incb    %[src_u]                                 \n"
      "incb    %[src_v]                                 \n"
      "lsl     z1.h, p0/m, z1.h, z0.h                   \n"
      "lsl     z2.h, p0/m, z2.h, z0.h                   \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "st2h    {z1.h, z2.h}, p0, [%[dst_uv]]            \n"
      "incb    %[dst_uv], all, mul #2                   \n"
      "b.ge    1b                                       \n"

      "2:                                               \n"
      "adds    %w[width], %w[width], %w[vl]             \n"
      "b.eq    99f                                      \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt p0.h, wzr, %w[width]                     \n"
      "ld1h    {z1.h}, p0/z, [%[src_u]]                 \n"
      "ld1h    {z2.h}, p0/z, [%[src_v]]                 \n"
      "lsl     z1.h, p0/m, z1.h, z0.h                   \n"
      "lsl     z2.h, p0/m, z2.h, z0.h                   \n"
      "st2h    {z1.h, z2.h}, p0, [%[dst_uv]]            \n"

      "99:                                              \n"
      : [src_u] "+r"(src_u),    // %[src_u]
        [src_v] "+r"(src_v),    // %[src_v]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [width] "+r"(width),    // %[width]
        [vl] "=&r"(vl)          // %[vl]
      : [shift] "r"(shift)      // %[shift]
      : "memory", "cc", "z0", "z1", "z2", "p0");
}

// Use scale to convert lsb formats to msb, depending how many bits there are:
// 32768 = 9 bits = shr 1
// 16384 = 10 bits = shr 2
// 4096 = 12 bits = shr 4
// 256 = 16 bits = shr 8
__arm_locally_streaming void Convert16To8Row_SME(const uint16_t* src_y,
                                                 uint8_t* dst_y,
                                                 int scale,
                                                 int width) {
  // 15 - clz(scale), + 8 to shift result into the high half of the lane to
  // saturate, then we can just use UZP2 to narrow rather than a pair of
  // saturating narrow instructions.
  const int shift = 23 - __builtin_clz((int32_t)scale);
  int vl;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "dup      z0.h, %w[shift]                         \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p0.b                                    \n"
      "1:                                               \n"
      "ld1h     {z1.h}, p0/z, [%[src_y]]                \n"
      "ld1h     {z2.h}, p0/z, [%[src_y], #1, mul vl]    \n"
      "incb     %[src_y], all, mul #2                   \n"
      "uqshl    z1.h, p0/m, z1.h, z0.h                  \n"
      "uqshl    z2.h, p0/m, z2.h, z0.h                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "uzp2     z1.b, z1.b, z2.b                        \n"
      "st1b     {z1.b}, p0, [%[dst_y]]                  \n"
      "incb     %[dst_y]                                \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      // We need separate predicates for the load and store instructions since
      // they are operating on different element sizes (.b vs .h).
      "cnth     %x[vl]                                  \n"
      "whilelt  p0.h, wzr, %w[width]                    \n"
      "whilelt  p1.h, %w[vl], %w[width]                 \n"
      "whilelt  p2.b, wzr, %w[width]                    \n"
      "ld1h     {z1.h}, p0/z, [%[src_y]]                \n"
      "ld1h     {z2.h}, p1/z, [%[src_y], #1, mul vl]    \n"
      "uqshl    z1.h, p0/m, z1.h, z0.h                  \n"
      "uqshl    z2.h, p1/m, z2.h, z0.h                  \n"
      "uzp2     z1.b, z1.b, z2.b                        \n"
      "st1b     {z1.b}, p2, [%[dst_y]]                  \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),  // %[src_y]
        [dst_y] "+r"(dst_y),  // %[dst_y]
        [width] "+r"(width),  // %[width]
        [vl] "=&r"(vl)        // %[vl]
      : [shift] "r"(shift)    // %[shift]
      : "memory", "cc", "z0", "z1", "z2", "p0", "p1", "p2");
}

__arm_locally_streaming void CopyRow_SME(const uint8_t* src,
                                         uint8_t* dst,
                                         int width) {
  CopyRow_SVE_SC(src, dst, width);
}

__arm_locally_streaming void HalfRow_SME(const uint8_t* src_ptr,
                                         ptrdiff_t src_stride,
                                         uint8_t* dst_ptr,
                                         int width) {
  HalfRow_SVE_SC(src_ptr, src_stride, dst_ptr, width);
}

__arm_locally_streaming void InterpolateRow_SME(uint8_t* dst_ptr,
                                                const uint8_t* src_ptr,
                                                ptrdiff_t src_stride,
                                                int width,
                                                int source_y_fraction) {
  InterpolateRow_SVE_SC(dst_ptr, src_ptr, src_stride, width, source_y_fraction);
}

__arm_locally_streaming void HalfRow_16_SME(const uint16_t* src_ptr,
                                            ptrdiff_t src_stride,
                                            uint16_t* dst_ptr,
                                            int width) {
  HalfRow_16_SVE_SC(src_ptr, src_stride, dst_ptr, width);
}

__arm_locally_streaming void InterpolateRow_16_SME(uint16_t* dst_ptr,
                                                   const uint16_t* src_ptr,
                                                   ptrdiff_t src_stride,
                                                   int width,
                                                   int source_y_fraction) {
  InterpolateRow_16_SVE_SC(dst_ptr, src_ptr, src_stride, width,
                           source_y_fraction);
}

__arm_locally_streaming void DivideRow_16_SME(const uint16_t* src_y,
                                              uint16_t* dst_y,
                                              int scale,
                                              int width) {
  DivideRow_16_SVE_SC(src_y, dst_y, scale, width);
}

__arm_locally_streaming void HalfFloatRow_SME(const uint16_t* src,
                                              uint16_t* dst,
                                              float scale,
                                              int width) {
  HalfFloatRow_SVE_SC(src, dst, scale, width);
}

__arm_locally_streaming void HalfFloat1Row_SME(const uint16_t* src,
                                               uint16_t* dst,
                                               float scale,
                                               int width) {
  HalfFloat1Row_SVE_SC(src, dst, scale, width);
}

__arm_locally_streaming void ARGBToRGB565Row_SME(const uint8_t* src_argb,
                                                 uint8_t* dst_rgb,
                                                 int width) {
  ARGBToRGB565Row_SVE_SC(src_argb, dst_rgb, width);
}

__arm_locally_streaming void ARGBToRGB565DitherRow_SME(const uint8_t* src_argb,
                                                       uint8_t* dst_rgb,
                                                       uint32_t dither4,
                                                       int width) {
  ARGBToRGB565DitherRow_SVE_SC(src_argb, dst_rgb, dither4, width);
}

__arm_locally_streaming void ARGB1555ToARGBRow_SME(const uint8_t* src_argb1555,
                                                   uint8_t* dst_argb,
                                                   int width) {
  ARGB1555ToARGBRow_SVE_SC(src_argb1555, dst_argb, width);
}

__arm_locally_streaming void AYUVToUVRow_SME(const uint8_t* src_ayuv,
                                             int src_stride_ayuv,
                                             uint8_t* dst_uv,
                                             int width) {
  AYUVToUVRow_SVE_SC(src_ayuv, src_stride_ayuv, dst_uv, width);
}

__arm_locally_streaming void AYUVToVURow_SME(const uint8_t* src_ayuv,
                                             int src_stride_ayuv,
                                             uint8_t* dst_vu,
                                             int width) {
  AYUVToVURow_SVE_SC(src_ayuv, src_stride_ayuv, dst_vu, width);
}

__arm_locally_streaming void RAWToARGBRow_SME(const uint8_t* src_raw,
                                              uint8_t* dst_argb,
                                              int width) {
  RAWToARGBRow_SVE_SC(src_raw, dst_argb, width);
}

__arm_locally_streaming void RAWToRGBARow_SME(const uint8_t* src_raw,
                                              uint8_t* dst_rgba,
                                              int width) {
  RAWToRGBARow_SVE_SC(src_raw, dst_rgba, width);
}

__arm_locally_streaming void RGB24ToARGBRow_SME(const uint8_t* src_rgb24,
                                                uint8_t* dst_argb,
                                                int width) {
  RGB24ToARGBRow_SVE_SC(src_rgb24, dst_argb, width);
}

__arm_locally_streaming void RAWToRGB24Row_SME(const uint8_t* src_raw,
                                               uint8_t* dst_rgb24,
                                               int width) {
  RAWToRGB24Row_SVE_SC(src_raw, dst_rgb24, width);
}

__arm_locally_streaming void ARGBToRGB24Row_SME(const uint8_t* src_argb,
                                                uint8_t* dst_rgb,
                                                int width) {
  ARGBToRGB24Row_SVE_SC(src_argb, dst_rgb, width);
}

__arm_locally_streaming void ARGBToRAWRow_SME(const uint8_t* src_argb,
                                              uint8_t* dst_rgb,
                                              int width) {
  ARGBToRAWRow_SVE_SC(src_argb, dst_rgb, width);
}

__arm_locally_streaming void YUY2ToYRow_SME(const uint8_t* src_yuy2,
                                            uint8_t* dst_y,
                                            int width) {
  YUY2ToYRow_SVE_SC(src_yuy2, dst_y, width);
}

__arm_locally_streaming void UYVYToYRow_SME(const uint8_t* src_uyvy,
                                            uint8_t* dst_y,
                                            int width) {
  UYVYToYRow_SVE_SC(src_uyvy, dst_y, width);
}

__arm_locally_streaming void YUY2ToUVRow_SME(const uint8_t* src_yuy2,
                                             int src_stride_yuy2,
                                             uint8_t* dst_u,
                                             uint8_t* dst_v,
                                             int width) {
  YUY2ToUVRow_SVE_SC(src_yuy2, src_stride_yuy2, dst_u, dst_v, width);
}

__arm_locally_streaming void UYVYToUVRow_SME(const uint8_t* src_uyvy,
                                             int src_stride_uyvy,
                                             uint8_t* dst_u,
                                             uint8_t* dst_v,
                                             int width) {
  UYVYToUVRow_SVE_SC(src_uyvy, src_stride_uyvy, dst_u, dst_v, width);
}

__arm_locally_streaming void HalfRow_16To8_SME(const uint16_t* src_ptr,
                                               ptrdiff_t src_stride,
                                               uint8_t* dst_ptr,
                                               int scale,
                                               int width) {
  const uint16_t* src_ptr1 = src_ptr + src_stride;

  // 15 - clz(scale), + 8 to shift result into the high half of the lane to
  // saturate, then we can just use UZP2 to narrow rather than a pair of
  // saturating narrow instructions.
  const int shift = 23 - __builtin_clz((int32_t)scale);

  int vl;
  asm volatile(
      "cnth     %x[vl]                                  \n"
      "dup      z31.h, %w[shift]                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p0.h                                    \n"
      "1:                                               \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z3.h}, p0/z, [%[src_ptr1]]             \n"
      "incb     %[src_ptr]                              \n"
      "incb     %[src_ptr1]                             \n"
      "urhadd   z2.h, p0/m, z2.h, z3.h                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "uqshl    z2.h, p0/m, z2.h, z31.h                 \n"
      "shrnb    z2.b, z2.h, #8                          \n"
      "st1b     {z2.h}, p0, [%[dst_ptr]]                \n"
      "inch     %[dst_ptr]                              \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p0.h, wzr, %w[width]                    \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z3.h}, p0/z, [%[src_ptr1]]             \n"
      "urhadd   z2.h, p0/m, z2.h, z3.h                  \n"
      "uqshl    z2.h, p0/m, z2.h, z31.h                 \n"
      "shrnb    z2.b, z2.h, #8                          \n"
      "st1b     {z2.h}, p0, [%[dst_ptr]]                \n"

      "99:                                              \n"
      : [src_ptr] "+r"(src_ptr),    // %[src_ptr]
        [src_ptr1] "+r"(src_ptr1),  // %[src_ptr1]
        [dst_ptr] "+r"(dst_ptr),    // %[dst_ptr]
        [width] "+r"(width),        // %[width]
        [vl] "=&r"(vl)              // %[vl]
      : [shift] "r"(shift)          // %[shift]
      : "memory", "cc", "z2", "z3", "z31", "p0");
}

__arm_locally_streaming void HalfWidthRow_16To8_SME(const uint16_t* src_ptr,
                                                    ptrdiff_t src_stride,
                                                    uint8_t* dst_ptr,
                                                    int scale,
                                                    int width) {
  HalfWidthRow_16To8_SVE_SC(src_ptr, src_stride, dst_ptr, scale, width);
}

// Odd width: the last output pixel is a 1x2 average.
void HalfWidthRow_16To8_Odd_SME(const uint16_t* src_uv,
                                ptrdiff_t src_uv_stride,
                                uint8_t* dst_uv,
                                int scale,
                                int width) {
  int n = width - 1;
  if (n > 0) {
    HalfWidthRow_16To8_SME(src_uv, src_uv_stride, dst_uv, scale, n);
    src_uv += (ptrdiff_t)n * 2;
    dst_uv += n;
  }
  HalfWidthRow_16To8_Odd_C(src_uv, src_uv_stride, dst_uv, scale, 1);
}

__arm_locally_streaming void Convert8To8Row_SME(const uint8_t* src_y,
                                                uint8_t* dst_y,
                                                int scale,
                                                int bias,
                                                int width) {
  Convert8To8Row_SVE_SC(src_y, dst_y, scale, bias, width);
}

#define CONVERT8TO16_SVE                                 \
  "ld1b        {z0.h}, p0/z, [%[src]]                \n" \
  "ld1b        {z1.h}, p1/z, [%[src], #1, mul vl]    \n" \
  "incb        %[src]                                \n" \
  "subs        %w[width], %w[width], %w[vl], lsl #1  \n" \
  "trn1        z0.b, z0.b, z0.b                      \n" \
  "trn1        z1.b, z1.b, z1.b                      \n" \
  "lsr         z0.h, p0/m, z0.h, z2.h                \n" \
  "lsr         z1.h, p1/m, z1.h, z2.h                \n" \
  "prfm        pldl1keep, [%[src], 448]              \n" \
  "st1h        {z0.h}, p0, [%[dst]]                  \n" \
  "st1h        {z1.h}, p1, [%[dst], #1, mul vl]      \n" \
  "incb        %[dst], all, mul #2                   \n"

__arm_locally_streaming void Convert8To16Row_SME(const uint8_t* src_y,
                                                 uint16_t* dst_y,
                                                 int bits,
                                                 int width) {
  // (src * 0x0101) >> (16 - bits).
  const int shift = 16 - bits;

  uint64_t vl;
  asm volatile(
      "dup         z2.h, %w[shift]                      \n"
      "cnth        %[vl]                                \n"
      "subs        %w[width], %w[width], %w[vl], lsl #1 \n"
      "b.lt        2f                                   \n"

      // Run bulk of computation with all-true predicates to avoid predicate
      // generation overhead.
      "ptrue       p0.h                                 \n"
      "ptrue       p1.h                                 \n"
      "1:                                               \n"  //
      CONVERT8TO16_SVE
      "b.ge        1b                                   \n"

      "2:                                               \n"
      "adds        %w[width], %w[width], %w[vl], lsl #1 \n"
      "b.eq        99f                                  \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt     p0.h, wzr, %w[width]                 \n"
      "whilelt     p1.h, %w[vl], %w[width]              \n"  //
      CONVERT8TO16_SVE

      "99:                                              \n"
      : [src] "+r"(src_y),    // %[src]
        [dst] "+r"(dst_y),    // %[dst]
        [width] "+r"(width),  // %[width]
        [vl] "=&r"(vl)        // %[vl]
      : [shift] "r"(shift)    // %[shift]
      : "cc", "memory", "z0", "z1", "z2", "p0", "p1");
}

__arm_locally_streaming void ARGBToUVMatrixRow_SME(
    const uint8_t* src_argb,
    int src_stride_argb,
    uint8_t* dst_u,
    uint8_t* dst_v,
    int width,
    const struct ArgbConstants* c) {
  ARGBToUVMatrixRow_SVE_SC(src_argb, src_stride_argb, dst_u, dst_v, width, c);
}

__arm_locally_streaming void ARGBToYMatrixRow_SME(
    const uint8_t* src_argb,
    uint8_t* dst_y,
    int width,
    const struct ArgbConstants* c) {
  ARGBToYMatrixRow_SVE_SC(src_argb, dst_y, width, c);
}

#ifdef HAS_SSIM4X4ROW_SME
__arm_locally_streaming void Ssim4x4Row_SME(const uint8_t* src_a,
                                            int stride_a,
                                            const uint8_t* src_b,
                                            int stride_b,
                                            uint32_t* dst,
                                            int width) {
  Ssim4x4Row_SVE_SC(src_a, stride_a, src_b, stride_b, dst, width);
}
#endif  // HAS_SSIM4X4ROW_SME

#ifdef HAS_SSIMWINDOWROW_SME
__arm_locally_streaming float SsimWindowRow_SME(const uint32_t* row0,
                                                const uint32_t* row1,
                                                int num_windows) {
  return SsimWindowRow_SVE_SC(row0, row1, num_windows);
}
#endif  // HAS_SSIMWINDOWROW_SME

#ifdef HAS_ARGBORROW_SME
__arm_locally_streaming void ARGBOrRow_SME(const uint8_t* src_argb,
                                           uint8_t* dst_argb,
                                           uint32_t value,
                                           int width) {
  ARGBOrRow_SVE_SC(src_argb, dst_argb, value, width);
}
#endif  // HAS_ARGBORROW_SME

#ifdef HAS_ARGBALPHABLENDROW_SME
__arm_locally_streaming void ARGBAlphaBlendRow_SME(const uint8_t* src_argb0,
                                                   const uint8_t* src_argb1,
                                                   uint8_t* dst_argb,
                                                   int width) {
  ARGBAlphaBlendRow_SVE_SC(src_argb0, src_argb1, dst_argb, width);
}
#endif  // HAS_ARGBALPHABLENDROW_SME

#define HALFWIDTHROW_SME                              \
  "ld2b        {z0.b, z1.b}, p0/z, [%[src]]       \n" \
  "incb        %[src], all, mul #2                \n" \
  "urhadd      z0.b, p0/m, z0.b, z1.b             \n" \
  "subs        %w[width], %w[width], %w[vl]       \n" \
  "st1b        {z0.b}, p0, [%[dst]]               \n" \
  "incb        %[dst]                             \n"

__arm_locally_streaming void HalfWidthRow_SME(const uint8_t* src_ptr,
                                              uint8_t* dst_ptr,
                                              int width) {
  int vl;
  asm volatile(
      "cntb        %x[vl]                             \n"
      "subs        %w[width], %w[width], %w[vl]       \n"
      "b.lt        2f                                 \n"

      "ptrue       p0.b                               \n"
      "1:                                             \n"  //
      HALFWIDTHROW_SME
      "b.ge        1b                                 \n"

      "2:                                             \n"
      "adds        %w[width], %w[width], %w[vl]       \n"
      "b.eq        99f                                \n"

      "whilelt     p0.b, wzr, %w[width]               \n"  //
      HALFWIDTHROW_SME

      "99:                                            \n"
      : [src] "+r"(src_ptr), [dst] "+r"(dst_ptr), [width] "+r"(width),
        [vl] "=&r"(vl)
      :
      : "memory", "cc", "z0", "z1", "p0");
}
#undef HALFWIDTHROW_SME

void HalfWidthRow_Odd_SME(const uint8_t* src_ptr, uint8_t* dst_ptr, int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthRow_SME(src_ptr, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  dst_ptr[0] = src_ptr[0];
}

#define HALFWIDTHROW_16_SME                           \
  "ld2h        {z0.h, z1.h}, p0/z, [%[src]]       \n" \
  "incb        %[src], all, mul #2                \n" \
  "urhadd      z0.h, p0/m, z0.h, z1.h             \n" \
  "subs        %w[width], %w[width], %w[vl]       \n" \
  "st1h        {z0.h}, p0, [%[dst]]               \n" \
  "incb        %[dst]                             \n"

__arm_locally_streaming void HalfWidthRow_16_SME(const uint16_t* src_ptr,
                                                 uint16_t* dst_ptr,
                                                 int width) {
  int vl;
  asm volatile(
      "cnth        %x[vl]                             \n"
      "subs        %w[width], %w[width], %w[vl]       \n"
      "b.lt        2f                                 \n"

      "ptrue       p0.h                               \n"
      "1:                                             \n"  //
      HALFWIDTHROW_16_SME
      "b.ge        1b                                 \n"

      "2:                                             \n"
      "adds        %w[width], %w[width], %w[vl]       \n"
      "b.eq        99f                                \n"

      "whilelt     p0.h, wzr, %w[width]               \n"  //
      HALFWIDTHROW_16_SME

      "99:                                            \n"
      : [src] "+r"(src_ptr), [dst] "+r"(dst_ptr), [width] "+r"(width),
        [vl] "=&r"(vl)
      :
      : "memory", "cc", "z0", "z1", "p0");
}
#undef HALFWIDTHROW_16_SME

void HalfWidthRow_16_Odd_SME(const uint16_t* src_ptr,
                             uint16_t* dst_ptr,
                             int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthRow_16_SME(src_ptr, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  dst_ptr[0] = src_ptr[0];
}

#define HALFWIDTHUVROW_SME                            \
  "ld2h        {z0.h, z1.h}, p0/z, [%[src]]       \n" \
  "incb        %[src], all, mul #2                \n" \
  "urhadd      z0.b, p1/m, z0.b, z1.b             \n" \
  "subs        %w[width], %w[width], %w[vl]       \n" \
  "st1h        {z0.h}, p0, [%[dst]]               \n" \
  "incb        %[dst]                             \n"

__arm_locally_streaming void HalfWidthUVRow_SME(const uint8_t* src_uv,
                                                uint8_t* dst_uv,
                                                int width) {
  int vl;
  asm volatile(
      "cnth        %x[vl]                             \n"
      "ptrue       p1.b                               \n"
      "subs        %w[width], %w[width], %w[vl]       \n"
      "b.lt        2f                                 \n"

      "ptrue       p0.h                               \n"
      "1:                                             \n"  //
      HALFWIDTHUVROW_SME
      "b.ge        1b                                 \n"

      "2:                                             \n"
      "adds        %w[width], %w[width], %w[vl]       \n"
      "b.eq        99f                                \n"

      "whilelt     p0.h, wzr, %w[width]               \n"  //
      HALFWIDTHUVROW_SME

      "99:                                            \n"
      : [src] "+r"(src_uv), [dst] "+r"(dst_uv), [width] "+r"(width),
        [vl] "=&r"(vl)
      :
      : "memory", "cc", "z0", "z1", "p0", "p1");
}
#undef HALFWIDTHUVROW_SME

void HalfWidthUVRow_Odd_SME(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthUVRow_SME(src_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  dst_uv[0] = src_uv[0];
  dst_uv[1] = src_uv[1];
}

#define HALFWIDTHUVROW_16_SME                         \
  "ld2w        {z0.s, z1.s}, p0/z, [%[src]]       \n" \
  "incb        %[src], all, mul #2                \n" \
  "urhadd      z0.h, p1/m, z0.h, z1.h             \n" \
  "subs        %w[width], %w[width], %w[vl]       \n" \
  "st1w        {z0.s}, p0, [%[dst]]               \n" \
  "incb        %[dst]                             \n"

__arm_locally_streaming void HalfWidthUVRow_16_SME(const uint16_t* src_uv,
                                                   uint16_t* dst_uv,
                                                   int width) {
  int vl;
  asm volatile(
      "cntw        %x[vl]                             \n"
      "ptrue       p1.b                               \n"
      "subs        %w[width], %w[width], %w[vl]       \n"
      "b.lt        2f                                 \n"

      "ptrue       p0.s                               \n"
      "1:                                             \n"  //
      HALFWIDTHUVROW_16_SME
      "b.ge        1b                                 \n"

      "2:                                             \n"
      "adds        %w[width], %w[width], %w[vl]       \n"
      "b.eq        99f                                \n"

      "whilelt     p0.s, wzr, %w[width]               \n"  //
      HALFWIDTHUVROW_16_SME

      "99:                                            \n"
      : [src] "+r"(src_uv), [dst] "+r"(dst_uv), [width] "+r"(width),
        [vl] "=&r"(vl)
      :
      : "memory", "cc", "z0", "z1", "p0", "p1");
}
#undef HALFWIDTHUVROW_16_SME

void HalfWidthUVRow_16_Odd_SME(const uint16_t* src_uv,
                               uint16_t* dst_uv,
                               int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthUVRow_16_SME(src_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  dst_uv[0] = src_uv[0];
  dst_uv[1] = src_uv[1];
}

__arm_locally_streaming void HalfBoxRow_SME(const uint8_t* src_ptr,
                                            ptrdiff_t src_stride,
                                            uint8_t* dst_ptr,
                                            int width) {
  HalfBoxRow_SVE_SC(src_ptr, src_stride, dst_ptr, width);
}

void HalfBoxRow_Odd_SME(const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        uint8_t* dst_ptr,
                        int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxRow_SME(src_ptr, src_stride, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  dst_ptr[0] = (src_ptr[0] + src_ptr[src_stride] + 1) >> 1;
}

__arm_locally_streaming void HalfBoxRow_16_SME(const uint16_t* src_ptr,
                                               ptrdiff_t src_stride,
                                               uint16_t* dst_ptr,
                                               int width) {
  HalfBoxRow_16_SVE_SC(src_ptr, src_stride, dst_ptr, width);
}

void HalfBoxRow_16_Odd_SME(const uint16_t* src_ptr,
                           ptrdiff_t src_stride,
                           uint16_t* dst_ptr,
                           int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxRow_16_SME(src_ptr, src_stride, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  dst_ptr[0] =
      (uint16_t)(((uint32_t)src_ptr[0] + src_ptr[src_stride] + 1u) >> 1);
}

__arm_locally_streaming void HalfBoxUVRow_SME(const uint8_t* src_uv,
                                              ptrdiff_t src_stride_uv,
                                              uint8_t* dst_uv,
                                              int width) {
  HalfBoxUVRow_SVE_SC(src_uv, src_stride_uv, dst_uv, width);
}

void HalfBoxUVRow_Odd_SME(const uint8_t* src_uv,
                          ptrdiff_t src_stride_uv,
                          uint8_t* dst_uv,
                          int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxUVRow_SME(src_uv, src_stride_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  dst_uv[0] = (src_uv[0] + src_uv[src_stride_uv] + 1) >> 1;
  dst_uv[1] = (src_uv[1] + src_uv[src_stride_uv + 1] + 1) >> 1;
}

__arm_locally_streaming void HalfBoxUVRow_16_SME(const uint16_t* src_uv,
                                                 ptrdiff_t src_stride_uv,
                                                 uint16_t* dst_uv,
                                                 int width) {
  HalfBoxUVRow_16_SVE_SC(src_uv, src_stride_uv, dst_uv, width);
}

void HalfBoxUVRow_16_Odd_SME(const uint16_t* src_uv,
                             ptrdiff_t src_stride_uv,
                             uint16_t* dst_uv,
                             int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxUVRow_16_SME(src_uv, src_stride_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  dst_uv[0] =
      (uint16_t)(((uint32_t)src_uv[0] + src_uv[src_stride_uv] + 1u) >> 1);
  dst_uv[1] =
      (uint16_t)(((uint32_t)src_uv[1] + src_uv[src_stride_uv + 1] + 1u) >> 1);
}

#if defined(__clang__)
#pragma clang attribute pop
#elif defined(__GNUC__)
#pragma GCC pop_options
#endif

#endif  // !defined(LIBYUV_DISABLE_SME) && (defined(CLANG_HAS_SME) ||
        // defined(GCC_HAS_SME)) && defined(__aarch64__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

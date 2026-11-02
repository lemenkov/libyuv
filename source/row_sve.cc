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

#if !defined(LIBYUV_DISABLE_SVE) && defined(__aarch64__)
#if defined(__clang__)
#pragma clang attribute push( \
    __attribute__((target("arch=armv8.5-a+i8mm+sve2"))), apply_to = function)
#elif defined(__GNUC__)
#pragma GCC push_options
#pragma GCC target("arch=armv8.5-a+i8mm+sve2")
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

#if !defined(LIBYUV_DISABLE_SVE) && defined(__aarch64__)

void I444ToARGBRow_SVE2(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I444ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

void I444ToRGB24Row_SVE2(const uint8_t* src_y,
                         const uint8_t* src_u,
                         const uint8_t* src_v,
                         uint8_t* dst_rgb24,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  I444ToRGB24Row_SVE_SC(src_y, src_u, src_v, dst_rgb24, yuvconstants, width);
}

void I400ToARGBRow_SVE2(const uint8_t* src_y,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I400ToARGBRow_SVE_SC(src_y, dst_argb, yuvconstants, width);
}

void I422ToARGBRow_SVE2(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I422ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

void I422ToRGB24Row_SVE2(const uint8_t* src_y,
                         const uint8_t* src_u,
                         const uint8_t* src_v,
                         uint8_t* dst_argb,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  I422ToRGB24Row_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

void I422ToRGB565Row_SVE2(const uint8_t* src_y,
                          const uint8_t* src_u,
                          const uint8_t* src_v,
                          uint8_t* dst_rgb565,
                          const struct YuvConstants* yuvconstants,
                          int width) {
  I422ToRGB565Row_SVE_SC(src_y, src_u, src_v, dst_rgb565, yuvconstants, width);
}

void I422ToARGB1555Row_SVE2(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_argb1555,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  I422ToARGB1555Row_SVE_SC(src_y, src_u, src_v, dst_argb1555, yuvconstants,
                           width);
}

void I422ToARGB4444Row_SVE2(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_argb4444,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  I422ToARGB4444Row_SVE_SC(src_y, src_u, src_v, dst_argb4444, yuvconstants,
                           width);
}

void I422ToRGBARow_SVE2(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_rgba,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I422ToRGBARow_SVE_SC(src_y, src_u, src_v, dst_rgba, yuvconstants, width);
}

void I422ToAR30Row_SVE2(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I422ToAR30Row_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

void I422AlphaToARGBRow_SVE2(const uint8_t* src_y,
                             const uint8_t* src_u,
                             const uint8_t* src_v,
                             const uint8_t* src_a,
                             uint8_t* dst_argb,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  I422AlphaToARGBRow_SVE_SC(src_y, src_u, src_v, src_a, dst_argb, yuvconstants,
                            width);
}

void I444AlphaToARGBRow_SVE2(const uint8_t* src_y,
                             const uint8_t* src_u,
                             const uint8_t* src_v,
                             const uint8_t* src_a,
                             uint8_t* dst_argb,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  I444AlphaToARGBRow_SVE_SC(src_y, src_u, src_v, src_a, dst_argb, yuvconstants,
                            width);
}

void NV12ToARGBRow_SVE2(const uint8_t* src_y,
                        const uint8_t* src_uv,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  NV12ToARGBRow_SVE_SC(src_y, src_uv, dst_argb, yuvconstants, width);
}

void NV21ToARGBRow_SVE2(const uint8_t* src_y,
                        const uint8_t* src_vu,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  NV21ToARGBRow_SVE_SC(src_y, src_vu, dst_argb, yuvconstants, width);
}

void NV12ToRGB24Row_SVE2(const uint8_t* src_y,
                         const uint8_t* src_uv,
                         uint8_t* dst_rgb24,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  NV12ToRGB24Row_SVE_SC(src_y, src_uv, dst_rgb24, yuvconstants, width);
}

void NV21ToRGB24Row_SVE2(const uint8_t* src_y,
                         const uint8_t* src_vu,
                         uint8_t* dst_rgb24,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  NV21ToRGB24Row_SVE_SC(src_y, src_vu, dst_rgb24, yuvconstants, width);
}

void ARGBToUVMatrixRow_SVE2(const uint8_t* src_argb,
                            int src_stride_argb,
                            uint8_t* dst_u,
                            uint8_t* dst_v,
                            int width,
                            const struct ArgbConstants* c) {
  int8_t uvconstants[8] = {(int8_t)c->kRGBToU[0], (int8_t)c->kRGBToU[1],
                           (int8_t)c->kRGBToU[2], (int8_t)c->kRGBToU[3],
                           (int8_t)c->kRGBToV[0], (int8_t)c->kRGBToV[1],
                           (int8_t)c->kRGBToV[2], (int8_t)c->kRGBToV[3]};
  ARGBToUVMatrixRow_SVE_SC(src_argb, src_stride_argb, dst_u, dst_v, width,
                           uvconstants);
}

void ARGBToYMatrixRow_SVE2(const uint8_t* src_argb,
                           uint8_t* dst_y,
                           int width,
                           const struct ArgbConstants* c) {
  ARGBToYMatrixRow_SVE_SC(src_argb, dst_y, width, c);
}

void ARGBToRGB565Row_SVE2(const uint8_t* src_argb,
                          uint8_t* dst_rgb,
                          int width) {
  ARGBToRGB565Row_SVE_SC(src_argb, dst_rgb, width);
}

void ARGBToRGB565DitherRow_SVE2(const uint8_t* src_argb,
                                uint8_t* dst_rgb,
                                uint32_t dither4,
                                int width) {
  ARGBToRGB565DitherRow_SVE_SC(src_argb, dst_rgb, dither4, width);
}

void ARGB1555ToARGBRow_SVE2(const uint8_t* src_argb1555,
                            uint8_t* dst_argb,
                            int width) {
  ARGB1555ToARGBRow_SVE_SC(src_argb1555, dst_argb, width);
}

void AYUVToUVRow_SVE2(const uint8_t* src_ayuv,
                      int src_stride_ayuv,
                      uint8_t* dst_uv,
                      int width) {
  AYUVToUVRow_SVE_SC(src_ayuv, src_stride_ayuv, dst_uv, width);
}

void AYUVToVURow_SVE2(const uint8_t* src_ayuv,
                      int src_stride_ayuv,
                      uint8_t* dst_vu,
                      int width) {
  AYUVToVURow_SVE_SC(src_ayuv, src_stride_ayuv, dst_vu, width);
}

void YUY2ToARGBRow_SVE2(const uint8_t* src_yuy2,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  YUY2ToARGBRow_SVE_SC(src_yuy2, dst_argb, yuvconstants, width);
}

void UYVYToARGBRow_SVE2(const uint8_t* src_uyvy,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  UYVYToARGBRow_SVE_SC(src_uyvy, dst_argb, yuvconstants, width);
}

void RAWToARGBRow_SVE2(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  RAWToARGBRow_SVE_SC(src_raw, dst_argb, width);
}

void RAWToRGBARow_SVE2(const uint8_t* src_raw, uint8_t* dst_rgba, int width) {
  RAWToRGBARow_SVE_SC(src_raw, dst_rgba, width);
}

void RGB24ToARGBRow_SVE2(const uint8_t* src_rgb24,
                         uint8_t* dst_argb,
                         int width) {
  RGB24ToARGBRow_SVE_SC(src_rgb24, dst_argb, width);
}

void RAWToRGB24Row_SVE2(const uint8_t* src_raw, uint8_t* dst_rgb24, int width) {
  RAWToRGB24Row_SVE_SC(src_raw, dst_rgb24, width);
}

void ARGBToRGB24Row_SVE2(const uint8_t* src_argb, uint8_t* dst_rgb, int width) {
  ARGBToRGB24Row_SVE_SC(src_argb, dst_rgb, width);
}

void ARGBToRAWRow_SVE2(const uint8_t* src_argb, uint8_t* dst_rgb, int width) {
  ARGBToRAWRow_SVE_SC(src_argb, dst_rgb, width);
}

void DivideRow_16_SVE2(const uint16_t* src_y,
                       uint16_t* dst_y,
                       int scale,
                       int width) {
  DivideRow_16_SVE_SC(src_y, dst_y, scale, width);
}

void HalfFloatRow_SVE2(const uint16_t* src,
                       uint16_t* dst,
                       float scale,
                       int width) {
  HalfFloatRow_SVE_SC(src, dst, scale, width);
}

void HalfFloat1Row_SVE2(const uint16_t* src,
                        uint16_t* dst,
                        float scale,
                        int width) {
  HalfFloat1Row_SVE_SC(src, dst, scale, width);
}

void I210ToARGBRow_SVE2(const uint16_t* src_y,
                        const uint16_t* src_u,
                        const uint16_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I210ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

void I210AlphaToARGBRow_SVE2(const uint16_t* src_y,
                             const uint16_t* src_u,
                             const uint16_t* src_v,
                             const uint16_t* src_a,
                             uint8_t* dst_argb,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  I210AlphaToARGBRow_SVE_SC(src_y, src_u, src_v, src_a, dst_argb, yuvconstants,
                            width);
}

void I210ToAR30Row_SVE2(const uint16_t* src_y,
                        const uint16_t* src_u,
                        const uint16_t* src_v,
                        uint8_t* dst_ar30,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I210ToAR30Row_SVE_SC(src_y, src_u, src_v, dst_ar30, yuvconstants, width);
}

void P210ToARGBRow_SVE2(const uint16_t* src_y,
                        const uint16_t* src_uv,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  P210ToARGBRow_SVE_SC(src_y, src_uv, dst_argb, yuvconstants, width);
}

void P210ToAR30Row_SVE2(const uint16_t* src_y,
                        const uint16_t* src_uv,
                        uint8_t* dst_ar30,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  P210ToAR30Row_SVE_SC(src_y, src_uv, dst_ar30, yuvconstants, width);
}

void I410ToARGBRow_SVE2(const uint16_t* src_y,
                        const uint16_t* src_u,
                        const uint16_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I410ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

void I410AlphaToARGBRow_SVE2(const uint16_t* src_y,
                             const uint16_t* src_u,
                             const uint16_t* src_v,
                             const uint16_t* src_a,
                             uint8_t* dst_argb,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  I410AlphaToARGBRow_SVE_SC(src_y, src_u, src_v, src_a, dst_argb, yuvconstants,
                            width);
}

void I410ToAR30Row_SVE2(const uint16_t* src_y,
                        const uint16_t* src_u,
                        const uint16_t* src_v,
                        uint8_t* dst_ar30,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I410ToAR30Row_SVE_SC(src_y, src_u, src_v, dst_ar30, yuvconstants, width);
}

void P410ToARGBRow_SVE2(const uint16_t* src_y,
                        const uint16_t* src_uv,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  P410ToARGBRow_SVE_SC(src_y, src_uv, dst_argb, yuvconstants, width);
}

void P410ToAR30Row_SVE2(const uint16_t* src_y,
                        const uint16_t* src_uv,
                        uint8_t* dst_ar30,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  P410ToAR30Row_SVE_SC(src_y, src_uv, dst_ar30, yuvconstants, width);
}

void I212ToAR30Row_SVE2(const uint16_t* src_y,
                        const uint16_t* src_u,
                        const uint16_t* src_v,
                        uint8_t* dst_ar30,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I212ToAR30Row_SVE_SC(src_y, src_u, src_v, dst_ar30, yuvconstants, width);
}

void I212ToARGBRow_SVE2(const uint16_t* src_y,
                        const uint16_t* src_u,
                        const uint16_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  I212ToARGBRow_SVE_SC(src_y, src_u, src_v, dst_argb, yuvconstants, width);
}

void Convert8To8Row_SVE2(const uint8_t* src_y,
                         uint8_t* dst_y,
                         int scale,
                         int bias,
                         int width) {
  Convert8To8Row_SVE_SC(src_y, dst_y, scale, bias, width);
}

void MergeUVRow_SVE2(const uint8_t* src_u,
                     const uint8_t* src_v,
                     uint8_t* dst_uv,
                     int width) {
  MergeUVRow_SVE_SC(src_u, src_v, dst_uv, width);
}

void SplitUVRow_SVE2(const uint8_t* src_uv,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  SplitUVRow_SVE_SC(src_uv, dst_u, dst_v, width);
}

void HalfMergeUVRow_SVE2(const uint8_t* src_u,
                         int src_stride_u,
                         const uint8_t* src_v,
                         int src_stride_v,
                         uint8_t* dst_uv,
                         int width) {
  HalfMergeUVRow_SVE_SC(src_u, src_stride_u, src_v, src_stride_v, dst_uv,
                        width);
}

void CopyRow_SVE2(const uint8_t* src, uint8_t* dst, int width) {
  CopyRow_SVE_SC(src, dst, width);
}

void HalfRow_SVE2(const uint8_t* src_ptr,
                  ptrdiff_t src_stride,
                  uint8_t* dst_ptr,
                  int width) {
  HalfRow_SVE_SC(src_ptr, src_stride, dst_ptr, width);
}

void HalfRow_16_SVE2(const uint16_t* src_ptr,
                     ptrdiff_t src_stride,
                     uint16_t* dst_ptr,
                     int width) {
  HalfRow_16_SVE_SC(src_ptr, src_stride, dst_ptr, width);
}

void InterpolateRow_SVE2(uint8_t* dst_ptr,
                         const uint8_t* src_ptr,
                         ptrdiff_t src_stride,
                         int width,
                         int source_y_fraction) {
  InterpolateRow_SVE_SC(dst_ptr, src_ptr, src_stride, width, source_y_fraction);
}

void InterpolateRow_16_SVE2(uint16_t* dst_ptr,
                            const uint16_t* src_ptr,
                            ptrdiff_t src_stride,
                            int width,
                            int source_y_fraction) {
  InterpolateRow_16_SVE_SC(dst_ptr, src_ptr, src_stride, width,
                           source_y_fraction);
}

void YUY2ToYRow_SVE2(const uint8_t* src_yuy2, uint8_t* dst_y, int width) {
  YUY2ToYRow_SVE_SC(src_yuy2, dst_y, width);
}

void UYVYToYRow_SVE2(const uint8_t* src_uyvy, uint8_t* dst_y, int width) {
  UYVYToYRow_SVE_SC(src_uyvy, dst_y, width);
}

void YUY2ToUVRow_SVE2(const uint8_t* src_yuy2,
                      int src_stride_yuy2,
                      uint8_t* dst_u,
                      uint8_t* dst_v,
                      int width) {
  YUY2ToUVRow_SVE_SC(src_yuy2, src_stride_yuy2, dst_u, dst_v, width);
}

void UYVYToUVRow_SVE2(const uint8_t* src_uyvy,
                      int src_stride_uyvy,
                      uint8_t* dst_u,
                      uint8_t* dst_v,
                      int width) {
  UYVYToUVRow_SVE_SC(src_uyvy, src_stride_uyvy, dst_u, dst_v, width);
}

#ifdef HAS_CONVERT16TO8ROW_SVE2
// Disabled: NEON version is faster for 128 bit vectors.
void Convert16To8Row_SVE2(const uint16_t* src_y,
                          uint8_t* dst_y,
                          int scale,
                          int width) {
  const int shift = 23 - __builtin_clz((int32_t)scale);
  int vl;
  asm volatile(
      "cnth     %x[vl]                                  \n"
      "dup      z31.h, %w[shift]                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.h                                    \n"
      "1:                                               \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "incb     %[src_ptr]                              \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "uqshl    z2.h, p0/m, z2.h, z31.h                 \n"
      "shrnb    z2.b, z2.h, #8                          \n"
      "st1b     {z2.h}, p0, [%[dst_ptr]]                \n"
      "inch     %[dst_ptr]                              \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.h, wzr, %w[width]                    \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "uqshl    z2.h, p0/m, z2.h, z31.h                 \n"
      "shrnb    z2.b, z2.h, #8                          \n"
      "st1b     {z2.h}, p0, [%[dst_ptr]]                \n"

      "99:                                              \n"
      : [src_ptr] "+r"(src_y),  // %[src_ptr]
        [dst_ptr] "+r"(dst_y),  // %[dst_ptr]
        [width] "+r"(width),    // %[width]
        [vl] "=&r"(vl)          // %[vl]
      : [shift] "r"(shift)      // %[shift]
      : "cc", "memory", "z2", "z31", "p0");
}
#endif  // HAS_CONVERT16TO8ROW_SVE2

#ifdef HAS_HALFROW_16TO8_SVE2
// Disabled: NEON version is faster for 128 bit vectors.
void HalfRow_16To8_SVE2(const uint16_t* src_uv,
                        ptrdiff_t src_uv_stride,
                        uint8_t* dst_uv,
                        int scale,
                        int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  const int shift = 23 - __builtin_clz((int32_t)scale);
  int vl;
  asm volatile(
      "cnth     %x[vl]                                  \n"
      "dup      z31.h, %w[shift]                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

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

      "whilelt  p0.h, wzr, %w[width]                    \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z3.h}, p0/z, [%[src_ptr1]]             \n"
      "urhadd   z2.h, p0/m, z2.h, z3.h                  \n"
      "uqshl    z2.h, p0/m, z2.h, z31.h                 \n"
      "shrnb    z2.b, z2.h, #8                          \n"
      "st1b     {z2.h}, p0, [%[dst_ptr]]                \n"

      "99:                                              \n"
      : [src_ptr] "+r"(src_uv),    // %[src_ptr]
        [src_ptr1] "+r"(src_uv1),  // %[src_ptr1]
        [dst_ptr] "+r"(dst_uv),    // %[dst_ptr]
        [width] "+r"(width),       // %[width]
        [vl] "=&r"(vl)             // %[vl]
      : [shift] "r"(shift)         // %[shift]
      : "cc", "memory", "z2", "z3", "z31", "p0");
}
#endif  // HAS_HALFROW_16TO8_SVE2

#ifdef HAS_HALFWIDTHROW_16TO8_SVE2
// Disabled: NEON version is faster for 128 bit vectors.
void HalfWidthRow_16To8_SVE2(const uint16_t* src_uv,
                             ptrdiff_t src_uv_stride,
                             uint8_t* dst_uv,
                             int scale,
                             int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  const int shift = 23 - __builtin_clz((int32_t)scale);
  int vl;
  int vl_input;
  asm volatile(
      "cnth     %x[vl]                                  \n"
      "dup      z31.h, %w[shift]                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.h                                    \n"
      "1:                                               \n"
      "ld1h     {z0.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z1.h}, p0/z, [%[src_ptr], #1, mul vl]  \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr1]]             \n"
      "ld1h     {z3.h}, p0/z, [%[src_ptr1], #1, mul vl] \n"
      "incb     %[src_ptr], all, mul #4                 \n"
      "incb     %[src_ptr1], all, mul #4                \n"
      "uzp1     z4.h, z0.h, z1.h                        \n"
      "uzp2     z5.h, z0.h, z1.h                        \n"
      "uzp1     z6.h, z2.h, z3.h                        \n"
      "uzp2     z7.h, z2.h, z3.h                        \n"
      "uaddlb   z0.s, z4.h, z5.h                        \n"
      "uaddlt   z1.s, z4.h, z5.h                        \n"
      "uaddlb   z2.s, z6.h, z7.h                        \n"
      "uaddlt   z3.s, z6.h, z7.h                        \n"
      "add      z0.s, z0.s, z2.s                        \n"
      "add      z1.s, z1.s, z3.s                        \n"
      "rshrnb   z0.h, z0.s, #2                          \n"
      "rshrnt   z0.h, z1.s, #2                          \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "uqshl    z0.h, p0/m, z0.h, z31.h                 \n"
      "shrnb    z0.b, z0.h, #8                          \n"
      "st1b     {z0.h}, p0, [%[dst_ptr]]                \n"
      "inch     %[dst_ptr]                              \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "lsl      %w[vl_input], %w[width], #1             \n"
      "whilelt  p0.h, wzr, %w[vl_input]                 \n"
      "whilelt  p1.h, %w[vl], %w[vl_input]              \n"
      "whilelt  p2.h, wzr, %w[width]                    \n"
      "ld1h     {z0.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z1.h}, p1/z, [%[src_ptr], #1, mul vl]  \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr1]]             \n"
      "ld1h     {z3.h}, p1/z, [%[src_ptr1], #1, mul vl] \n"
      "uzp1     z4.h, z0.h, z1.h                        \n"
      "uzp2     z5.h, z0.h, z1.h                        \n"
      "uzp1     z6.h, z2.h, z3.h                        \n"
      "uzp2     z7.h, z2.h, z3.h                        \n"
      "uaddlb   z0.s, z4.h, z5.h                        \n"
      "uaddlt   z1.s, z4.h, z5.h                        \n"
      "uaddlb   z2.s, z6.h, z7.h                        \n"
      "uaddlt   z3.s, z6.h, z7.h                        \n"
      "add      z0.s, z0.s, z2.s                        \n"
      "add      z1.s, z1.s, z3.s                        \n"
      "rshrnb   z0.h, z0.s, #2                          \n"
      "rshrnt   z0.h, z1.s, #2                          \n"
      "uqshl    z0.h, p2/m, z0.h, z31.h                 \n"
      "shrnb    z0.b, z0.h, #8                          \n"
      "st1b     {z0.h}, p2, [%[dst_ptr]]                \n"

      "99:                                              \n"
      : [src_ptr] "+r"(src_uv),     // %[src_ptr]
        [src_ptr1] "+r"(src_uv1),   // %[src_ptr1]
        [dst_ptr] "+r"(dst_uv),     // %[dst_ptr]
        [width] "+r"(width),        // %[width]
        [vl] "=&r"(vl),             // %[vl]
        [vl_input] "=&r"(vl_input)  // %[vl_input]
      : [shift] "r"(shift)          // %[shift]
      : "cc", "memory", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z31",
        "p0", "p1", "p2");
}
#endif  // HAS_HALFWIDTHROW_16TO8_SVE2

#ifdef HAS_SSIM4X4ROW_SVE2
void Ssim4x4Row_SVE2(const uint8_t* src_a,
                     int stride_a,
                     const uint8_t* src_b,
                     int stride_b,
                     uint32_t* dst,
                     int width) {
  Ssim4x4Row_SVE_SC(src_a, stride_a, src_b, stride_b, dst, width);
}
#endif  // HAS_SSIM4X4ROW_SVE2

#ifdef HAS_SSIMWINDOWROW_SVE2
float SsimWindowRow_SVE2(const uint32_t* row0,
                         const uint32_t* row1,
                         int num_windows) {
  return SsimWindowRow_SVE_SC(row0, row1, num_windows);
}
#endif  // HAS_SSIMWINDOWROW_SVE2

#ifdef HAS_ARGBORROW_SVE2
void ARGBOrRow_SVE2(const uint8_t* src_argb,
                    uint8_t* dst_argb,
                    uint32_t value,
                    int width) {
  ARGBOrRow_SVE_SC(src_argb, dst_argb, value, width);
}
#endif  // HAS_ARGBORROW_SVE2

#ifdef HAS_ARGBALPHABLENDROW_SVE2
void ARGBAlphaBlendRow_SVE2(const uint8_t* src_argb0,
                            const uint8_t* src_argb1,
                            uint8_t* dst_argb,
                            int width) {
  ARGBAlphaBlendRow_SVE_SC(src_argb0, src_argb1, dst_argb, width);
}
#endif  // HAS_ARGBALPHABLENDROW_SVE2

#if defined(__clang__)
#pragma clang attribute pop
#elif defined(__GNUC__)
#pragma GCC pop_options
#endif

#endif  // !defined(LIBYUV_DISABLE_SVE) && defined(__aarch64__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

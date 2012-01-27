/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef LIBYUV_SOURCE_ROW_H_
#define LIBYUV_SOURCE_ROW_H_

#include "libyuv/basic_types.h"

#define kMaxStride (2048 * 4)
#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))

#if defined(COVERAGE_ENABLED) || defined(TARGET_IPHONE_SIMULATOR)
#define YUV_DISABLE_ASM
#endif

// The following are available on all x86 platforms
#if (defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)) && \
    !defined(YUV_DISABLE_ASM)
#define HAS_ABGRTOARGBROW_SSSE3
#define HAS_BGRATOARGBROW_SSSE3
#define HAS_RGB24TOARGBROW_SSSE3
#define HAS_RAWTOARGBROW_SSSE3
#define HAS_ARGBTOYROW_SSSE3
#define HAS_BGRATOYROW_SSSE3
#define HAS_ABGRTOYROW_SSSE3
#define HAS_ARGBTOUVROW_SSSE3
#define HAS_BGRATOUVROW_SSSE3
#define HAS_ABGRTOUVROW_SSSE3
#define HAS_I400TOARGBROW_SSE2
#define HAS_FASTCONVERTYTOARGBROW_SSE2
#define HAS_FASTCONVERTYUVTOARGBROW_SSSE3
#define HAS_FASTCONVERTYUVTOBGRAROW_SSSE3
#define HAS_FASTCONVERTYUVTOABGRROW_SSSE3
#define HAS_FASTCONVERTYUV444TOARGBROW_SSSE3
#define HAS_MIRRORROW_SSSE3
#define HAS_MIRRORROW_SSE2
#endif

// The following are available on Windows platforms
#if defined(_M_IX86) && !defined(YUV_DISABLE_ASM)
#define HAS_RGB565TOARGBROW_SSE2
#define HAS_ARGB1555TOARGBROW_SSE2
#define HAS_ARGB4444TOARGBROW_SSE2

#define HAS_ARGBTORGB24ROW_SSSE3
#define HAS_ARGBTORAWROW_SSSE3
#define HAS_ARGBTORGB565ROW_SSE2
#define HAS_ARGBTOARGB1555ROW_SSE2
#define HAS_ARGBTOARGB4444ROW_SSE2
#endif

// The following are available on Neon platforms
#if defined(__ARM_NEON__) && !defined(YUV_DISABLE_ASM)
#define HAS_MIRRORROW_NEON
#define HAS_FASTCONVERTYUVTOARGBROW_NEON
#define HAS_FASTCONVERTYUVTOBGRAROW_NEON
#define HAS_FASTCONVERTYUVTOABGRROW_NEON
#endif

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if defined(_MSC_VER)
#define SIMD_ALIGNED(var) __declspec(align(16)) var
typedef __declspec(align(16)) signed char vec8[16];
typedef __declspec(align(16)) unsigned char uvec8[16];
typedef __declspec(align(16)) signed short vec16[8];
#else // __GNUC__
#define SIMD_ALIGNED(var) var __attribute__((aligned(16)))
typedef signed char __attribute__((vector_size(16))) vec8;
typedef unsigned char __attribute__((vector_size(16))) uvec8;
typedef signed short __attribute__((vector_size(16))) vec16;
#endif

void FastConvertYUVToARGBRow_NEON(const uint8* y_buf,
                                  const uint8* u_buf,
                                  const uint8* v_buf,
                                  uint8* rgb_buf,
                                  int width);
void FastConvertYUVToBGRARow_NEON(const uint8* y_buf,
                                  const uint8* u_buf,
                                  const uint8* v_buf,
                                  uint8* rgb_buf,
                                  int width);
void FastConvertYUVToABGRRow_NEON(const uint8* y_buf,
                                  const uint8* u_buf,
                                  const uint8* v_buf,
                                  uint8* rgb_buf,
                                  int width);

void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix);
void BGRAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix);
void ABGRToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix);
void ARGBToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width);
void BGRAToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width);
void ABGRToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width);

void MirrorRow_SSSE3(const uint8* src, uint8* dst, int width);
void MirrorRow_SSE2(const uint8* src, uint8* dst, int width);
void MirrorRow_NEON(const uint8* src, uint8* dst, int width);
void MirrorRow_C(const uint8* src, uint8* dst, int width);

void ARGBToYRow_C(const uint8* src_argb, uint8* dst_y, int pix);
void BGRAToYRow_C(const uint8* src_argb, uint8* dst_y, int pix);
void ABGRToYRow_C(const uint8* src_argb, uint8* dst_y, int pix);

void ARGBToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                   uint8* dst_u, uint8* dst_v, int width);
void BGRAToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                   uint8* dst_u, uint8* dst_v, int width);
void ABGRToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                   uint8* dst_u, uint8* dst_v, int width);

void ABGRToARGBRow_SSSE3(const uint8* src_abgr, uint8* dst_argb, int pix);
void BGRAToARGBRow_SSSE3(const uint8* src_bgra, uint8* dst_argb, int pix);
void RGB24ToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix);
void RAWToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix);
void ARGB1555ToARGBRow_SSE2(const uint8* src_argb, uint8* dst_argb, int pix);
void RGB565ToARGBRow_SSE2(const uint8* src_argb, uint8* dst_argb, int pix);
void ARGB4444ToARGBRow_SSE2(const uint8* src_argb, uint8* dst_argb, int pix);

void ABGRToARGBRow_C(const uint8* src_abgr, uint8* dst_argb, int pix);
void BGRAToARGBRow_C(const uint8* src_bgra, uint8* dst_argb, int pix);
void RGB24ToARGBRow_C(const uint8* src_rgb24, uint8* dst_argb, int pix);
void RAWToARGBRow_C(const uint8* src_rgb24, uint8* dst_argb, int pix);
void RGB565ToARGBRow_C(const uint8* src_rgb, uint8* dst_argb, int pix);
void ARGB1555ToARGBRow_C(const uint8* src_argb, uint8* dst_argb, int pix);
void ARGB4444ToARGBRow_C(const uint8* src_argb, uint8* dst_argb, int pix);

void ARGBToRGB24Row_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToRAWRow_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToRGB565Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToARGB1555Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToARGB4444Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix);

void ARGBToRGB24Row_C(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToRAWRow_C(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToRGB565Row_C(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToARGB1555Row_C(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToARGB4444Row_C(const uint8* src_argb, uint8* dst_rgb, int pix);

void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix);
void I400ToARGBRow_C(const uint8* src_y, uint8* dst_argb, int pix);

void FastConvertYUVToARGBRow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width);

void FastConvertYUVToBGRARow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width);

void FastConvertYUVToABGRRow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width);

void FastConvertYUV444ToARGBRow_C(const uint8* y_buf,
                                  const uint8* u_buf,
                                  const uint8* v_buf,
                                  uint8* rgb_buf,
                                  int width);

void FastConvertYToARGBRow_C(const uint8* y_buf,
                             uint8* rgb_buf,
                             int width);

void FastConvertYUVToARGBRow_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgb_buf,
                                   int width);

void FastConvertYUVToBGRARow_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgb_buf,
                                   int width);

void FastConvertYUVToABGRRow_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgb_buf,
                                   int width);

void FastConvertYUV444ToARGBRow_SSSE3(const uint8* y_buf,
                                      const uint8* u_buf,
                                      const uint8* v_buf,
                                      uint8* rgb_buf,
                                      int width);

void FastConvertYToARGBRow_SSE2(const uint8* y_buf,
                                uint8* rgb_buf,
                                int width);

// 'Any' wrappers use memcpy()
void FastConvertYUVToARGBAnyRow_SSSE3(const uint8* y_buf,
                                      const uint8* u_buf,
                                      const uint8* v_buf,
                                      uint8* rgb_buf,
                                      int width);

void FastConvertYUVToBGRAAnyRow_SSSE3(const uint8* y_buf,
                                      const uint8* u_buf,
                                      const uint8* v_buf,
                                      uint8* rgb_buf,
                                      int width);

void FastConvertYUVToABGRAnyRow_SSSE3(const uint8* y_buf,
                                      const uint8* u_buf,
                                      const uint8* v_buf,
                                      uint8* rgb_buf,
                                      int width);

void ARGBToRGB24AnyRow_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToRAWAnyRow_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToRGB565AnyRow_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToARGB1555AnyRow_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix);
void ARGBToARGB4444AnyRow_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix);

void FastConvertYUVToARGBAnyRow_NEON(const uint8* y_buf,
                                     const uint8* u_buf,
                                     const uint8* v_buf,
                                     uint8* rgb_buf,
                                     int width);

void FastConvertYUVToBGRAAnyRow_NEON(const uint8* y_buf,
                                     const uint8* u_buf,
                                     const uint8* v_buf,
                                     uint8* rgb_buf,
                                     int width);

void FastConvertYUVToABGRAnyRow_NEON(const uint8* y_buf,
                                     const uint8* u_buf,
                                     const uint8* v_buf,
                                     uint8* rgb_buf,
                                     int width);

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // LIBYUV_SOURCE_ROW_H_

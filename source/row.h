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

// The following are available on all x86 platforms
#if (defined(WIN32) || defined(__x86_64__) || defined(__i386__)) \
    && !defined(COVERAGE_ENABLED) && !defined(TARGET_IPHONE_SIMULATOR)
#define HAS_ARGBTOYROW_SSSE3
#define HAS_BG24TOARGBROW_SSSE3
#define HAS_RAWTOARGBROW_SSSE3
#define HAS_RGB24TOYROW_SSSE3
#define HAS_RAWTOYROW_SSSE3
#define HAS_RGB24TOUVROW_SSSE3
#define HAS_RAWTOUVROW_SSSE3
#endif

// The following are available only on Windows
#if defined(WIN32) \
    && !defined(COVERAGE_ENABLED) && !defined(TARGET_IPHONE_SIMULATOR)
#define HAS_BGRATOYROW_SSSE3
#define HAS_ABGRTOYROW_SSSE3
#define HAS_ARGBTOUVROW_SSSE3
#define HAS_BGRATOUVROW_SSSE3
#define HAS_ABGRTOUVROW_SSSE3
#endif

extern "C" {
#ifdef HAS_ARGBTOYROW_SSSE3
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix);
void BGRAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix);
void ABGRToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix);
void ARGBToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width);
void BGRAToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width);
void ABGRToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width);
#endif
#if defined(HAS_BG24TOARGBROW_SSSE3) && defined(HAS_ARGBTOYROW_SSSE3)
#define HASRGB24TOYROW_SSSE3
#endif
#ifdef HASRGB24TOYROW_SSSE3
void RGB24ToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix);
void RAWToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix);
void RGB24ToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                        uint8* dst_u, uint8* dst_v, int width);
void RAWToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);
#endif
void ARGBToYRow_C(const uint8* src_argb, uint8* dst_y, int pix);
void BGRAToYRow_C(const uint8* src_argb, uint8* dst_y, int pix);
void ABGRToYRow_C(const uint8* src_argb, uint8* dst_y, int pix);
void RGB24ToYRow_C(const uint8* src_argb, uint8* dst_y, int pix);
void RAWToYRow_C(const uint8* src_argb, uint8* dst_y, int pix);
void ARGBToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                   uint8* dst_u, uint8* dst_v, int width);
void BGRAToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                   uint8* dst_u, uint8* dst_v, int width);
void ABGRToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                   uint8* dst_u, uint8* dst_v, int width);
void RGB24ToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                    uint8* dst_u, uint8* dst_v, int width);
void RAWToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                  uint8* dst_u, uint8* dst_v, int width);

#ifdef HAS_BG24TOARGBROW_SSSE3
void BG24ToARGBRow_SSSE3(const uint8* src_bg24, uint8* dst_argb, int pix);
void RAWToARGBRow_SSSE3(const uint8* src_bg24, uint8* dst_argb, int pix);
#endif
void BG24ToARGBRow_C(const uint8* src_bg24, uint8* dst_argb, int pix);
void RAWToARGBRow_C(const uint8* src_bg24, uint8* dst_argb, int pix);

#if defined(_MSC_VER)
#define SIMD_ALIGNED(var) __declspec(align(16)) var
#define TALIGN16(t, var) static __declspec(align(16)) t _ ## var
#else
#define SIMD_ALIGNED(var) var __attribute__((aligned(16)))
#define TALIGN16(t, var) t var __attribute__((aligned(16)))
#endif

#ifdef OSX
extern SIMD_ALIGNED(const int16 kCoefficientsRgbY[768][4]);
extern SIMD_ALIGNED(const int16 kCoefficientsBgraY[768][4]);
extern SIMD_ALIGNED(const int16 kCoefficientsAbgrY[768][4]);
#else
extern SIMD_ALIGNED(const int16 _kCoefficientsRgbY[768][4]);
extern SIMD_ALIGNED(const int16 _kCoefficientsBgraY[768][4]);
extern SIMD_ALIGNED(const int16 _kCoefficientsAbgrY[768][4]);
#endif
void FastConvertYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width);

void FastConvertYUVToBGRARow(const uint8* y_buf,
                             const uint8* u_buf,
                             const uint8* v_buf,
                             uint8* rgb_buf,
                             int width);

void FastConvertYUVToABGRRow(const uint8* y_buf,
                             const uint8* u_buf,
                             const uint8* v_buf,
                             uint8* rgb_buf,
                             int width);

void FastConvertYUV444ToRGB32Row(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width);

void FastConvertYToRGB32Row(const uint8* y_buf,
                            uint8* rgb_buf,
                            int width);

// Method to force C version.
//#define USE_MMX 0
//#define USE_SSE2 0

#if !defined(USE_MMX)
// Windows, Mac and Linux use MMX
#if defined(__i386__) || defined(_MSC_VER)
#define USE_MMX 1
#else
#define USE_MMX 0
#endif
#endif

#if !defined(USE_SSE2)
#if defined(__SSE2__) || defined(ARCH_CPU_X86_64) || _M_IX86_FP==2
#define USE_SSE2 1
#else
#define USE_SSE2 0
#endif
#endif

// x64 uses MMX2 (SSE) so emms is not required.
// Warning C4799: function has no EMMS instruction.
// EMMS() is slow and should be called by the calling function once per image.
#if USE_MMX && !defined(ARCH_CPU_X86_64)
#if defined(_MSC_VER)
#define EMMS() __asm emms
#pragma warning(disable: 4799)
#else
#define EMMS() asm("emms")
#endif
#else
#define EMMS()
#endif


}  // extern "C"

#endif  // LIBYUV_SOURCE_ROW_H_

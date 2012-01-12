/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "row.h"

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

void ABGRToARGBRow_C(const uint8* src_abgr, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    // To support in-place conversion.
    uint8 r = src_abgr[0];
    uint8 g = src_abgr[1];
    uint8 b = src_abgr[2];
    uint8 a = src_abgr[3];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = a;
    dst_argb += 4;
    src_abgr += 4;
  }
}

void BGRAToARGBRow_C(const uint8* src_bgra, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    // To support in-place conversion.
    uint8 a = src_bgra[0];
    uint8 r = src_bgra[1];
    uint8 g = src_bgra[2];
    uint8 b = src_bgra[3];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = a;
    dst_argb += 4;
    src_bgra += 4;
  }
}

void RAWToARGBRow_C(const uint8* src_raw, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 r = src_raw[0];
    uint8 g = src_raw[1];
    uint8 b = src_raw[2];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_raw += 3;
  }
}

void RGB24ToARGBRow_C(const uint8* src_rgb24, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_rgb24[0];
    uint8 g = src_rgb24[1];
    uint8 r = src_rgb24[2];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_rgb24 += 3;
  }
}

void RGB565ToARGBRow_C(const uint8* src_rgb, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_rgb[0] & 0x1f;
    uint8 g = (src_rgb[0] >> 5) | ((src_rgb[1] & 0x07) << 3);
    uint8 r = src_rgb[1] >> 3;
    dst_argb[0] = (b << 3) | (b >> 2);
    dst_argb[1] = (g << 2) | (g >> 4);
    dst_argb[2] = (r << 3) | (r >> 2);
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_rgb += 2;
  }
}

void ARGB1555ToARGBRow_C(const uint8* src_rgb, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_rgb[0] & 0x1f;
    uint8 g = (src_rgb[0] >> 5) | ((src_rgb[1] & 0x03) << 3);
    uint8 r = (src_rgb[1] & 0x7c) >> 2;
    uint8 a = src_rgb[1] >> 7;
    dst_argb[0] = (b << 3) | (b >> 2);
    dst_argb[1] = (g << 3) | (g >> 2);
    dst_argb[2] = (r << 3) | (r >> 2);
    dst_argb[3] = -a;
    dst_argb += 4;
    src_rgb += 2;
  }
}

void ARGB4444ToARGBRow_C(const uint8* src_rgb, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 a = src_rgb[1] >> 4;
    uint8 r = src_rgb[1] & 0x0f;
    uint8 g = src_rgb[0] >> 4;
    uint8 b = src_rgb[0] & 0x0f;
    dst_argb[0] = (b << 4) | b;
    dst_argb[1] = (g << 4) | g;
    dst_argb[2] = (r << 4) | r;
    dst_argb[3] = (a << 4) | a;
    dst_argb += 4;
    src_rgb += 2;
  }
}

void ARGBToRGB24Row_C(const uint8* src_argb, uint8* dst_rgb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_argb[0];
    uint8 g = src_argb[1];
    uint8 r = src_argb[2];
    dst_rgb[0] = b;
    dst_rgb[1] = g;
    dst_rgb[2] = r;
    dst_rgb += 3;
    src_argb += 4;
  }
}

void ARGBToRAWRow_C(const uint8* src_argb, uint8* dst_rgb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_argb[0];
    uint8 g = src_argb[1];
    uint8 r = src_argb[2];
    dst_rgb[0] = r;
    dst_rgb[1] = g;
    dst_rgb[2] = b;
    dst_rgb += 3;
    src_argb += 4;
  }
}

// TODO(fbarchard): support big endian CPU
void ARGBToRGB565Row_C(const uint8* src_argb, uint8* dst_rgb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_argb[0] >> 3;
    uint8 g = src_argb[1] >> 2;
    uint8 r = src_argb[2] >> 3;
    *reinterpret_cast<uint16*>(dst_rgb) = (r << 11) | (g << 5) | b;
    dst_rgb += 2;
    src_argb += 4;
  }
}

void ARGBToARGB1555Row_C(const uint8* src_argb, uint8* dst_rgb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_argb[0] >> 3;
    uint8 g = src_argb[1] >> 3;
    uint8 r = src_argb[2] >> 3;
    uint8 a = src_argb[2] >> 7;
    *reinterpret_cast<uint16*>(dst_rgb) = (a << 15) | (r << 10) | (g << 5) | b;
    dst_rgb += 2;
    src_argb += 4;
  }
}

void ARGBToARGB4444Row_C(const uint8* src_argb, uint8* dst_rgb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_argb[0] >> 4;
    uint8 g = src_argb[1] >> 4;
    uint8 r = src_argb[2] >> 4;
    uint8 a = src_argb[2] >> 4;
    *reinterpret_cast<uint16*>(dst_rgb) = (a << 12) | (r << 8) | (g << 4) | b;
    dst_rgb += 2;
    src_argb += 4;
  }
}

void RGB24ToYRow_C(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  RGB24ToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_C(row, dst_y, pix);
}

void RAWToYRow_C(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  RAWToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_C(row, dst_y, pix);
}

void RGB565ToYRow_C(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  RGB565ToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_C(row, dst_y, pix);
}

void ARGB1555ToYRow_C(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  ARGB1555ToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_C(row, dst_y, pix);
}

void ARGB4444ToYRow_C(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  ARGB4444ToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_C(row, dst_y, pix);
}

void RGB24ToUVRow_C(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RGB24ToARGBRow_C(src_argb, row, pix);
  RGB24ToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

void RAWToUVRow_C(const uint8* src_argb, int src_stride_argb,
                  uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RAWToARGBRow_C(src_argb, row, pix);
  RAWToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

void RGB565ToUVRow_C(const uint8* src_argb, int src_stride_argb,
                     uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RGB565ToARGBRow_C(src_argb, row, pix);
  RGB565ToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

void ARGB1555ToUVRow_C(const uint8* src_argb, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  ARGB1555ToARGBRow_C(src_argb, row, pix);
  ARGB1555ToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

void ARGB4444ToUVRow_C(const uint8* src_argb, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  ARGB4444ToARGBRow_C(src_argb, row, pix);
  ARGB4444ToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
  return (( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
  return ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
  return ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
}

#define MAKEROWY(NAME,R,G,B) \
void NAME ## ToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {       \
  for (int x = 0; x < width; ++x) {                                            \
    dst_y[0] = RGBToY(src_argb0[R], src_argb0[G], src_argb0[B]);               \
    src_argb0 += 4;                                                            \
    dst_y += 1;                                                                \
  }                                                                            \
}                                                                              \
void NAME ## ToUVRow_C(const uint8* src_rgb0, int src_stride_rgb,              \
                       uint8* dst_u, uint8* dst_v, int width) {                \
  const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           \
  for (int x = 0; x < width - 1; x += 2) {                                     \
    uint8 ab = (src_rgb0[B] + src_rgb0[B + 4] +                                \
               src_rgb1[B] + src_rgb1[B + 4]) >> 2;                            \
    uint8 ag = (src_rgb0[G] + src_rgb0[G + 4] +                                \
               src_rgb1[G] + src_rgb1[G + 4]) >> 2;                            \
    uint8 ar = (src_rgb0[R] + src_rgb0[R + 4] +                                \
               src_rgb1[R] + src_rgb1[R + 4]) >> 2;                            \
    dst_u[0] = RGBToU(ar, ag, ab);                                             \
    dst_v[0] = RGBToV(ar, ag, ab);                                             \
    src_rgb0 += 8;                                                             \
    src_rgb1 += 8;                                                             \
    dst_u += 1;                                                                \
    dst_v += 1;                                                                \
  }                                                                            \
  if (width & 1) {                                                             \
    uint8 ab = (src_rgb0[B] + src_rgb1[B]) >> 1;                               \
    uint8 ag = (src_rgb0[G] + src_rgb1[G]) >> 1;                               \
    uint8 ar = (src_rgb0[R] + src_rgb1[R]) >> 1;                               \
    dst_u[0] = RGBToU(ar, ag, ab);                                             \
    dst_v[0] = RGBToV(ar, ag, ab);                                             \
  }                                                                            \
}

MAKEROWY(ARGB,2,1,0)
MAKEROWY(BGRA,1,2,3)
MAKEROWY(ABGR,0,1,2)

#if defined(HAS_RAWTOYROW_SSSE3)
void RGB24ToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  RGB24ToARGBRow_SSSE3(src_argb, row, pix);
  ARGBToYRow_SSSE3(row, dst_y, pix);
}

void RAWToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  RAWToARGBRow_SSSE3(src_argb, row, pix);
  ARGBToYRow_SSSE3(row, dst_y, pix);
}

// TODO(fbarchard): RGB565ToARGBRow_SSE2 for rgb conversion
void RGB565ToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  RGB565ToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_SSSE3(row, dst_y, pix);
}

void ARGB1555ToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  ARGB1555ToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_SSSE3(row, dst_y, pix);
}

void ARGB4444ToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
#ifdef HAS_ARGB4444TOARGBROW_SSE2
  ARGB4444ToARGBRow_SSE2(src_argb, row, pix);
#else
  ARGB4444ToARGBRow_C(src_argb, row, pix);
#endif
  ARGBToYRow_SSSE3(row, dst_y, pix);
}
#endif

#if defined(HAS_RAWTOUVROW_SSSE3)
#if defined(HAS_ARGBTOUVROW_SSSE3)
void RGB24ToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                        uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RGB24ToARGBRow_SSSE3(src_argb, row, pix);
  RGB24ToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}

void RAWToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RAWToARGBRow_SSSE3(src_argb, row, pix);
  RAWToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}

// TODO(fbarchard): RGB565ToARGBRow_SSE2 for rgb conversion
void RGB565ToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                         uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RGB565ToARGBRow_C(src_argb, row, pix);
  RGB565ToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}

void ARGB1555ToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                           uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  ARGB1555ToARGBRow_C(src_argb, row, pix);
  ARGB1555ToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}

void ARGB4444ToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                           uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
#ifdef HAS_ARGB4444TOARGBROW_SSE2
  ARGB4444ToARGBRow_SSE2(src_argb, row, pix);
  ARGB4444ToARGBRow_SSE2(src_argb + src_stride_argb, row + kMaxStride, pix);
#else
  ARGB4444ToARGBRow_C(src_argb, row, pix);
  ARGB4444ToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
#endif
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}

#else

void RGB24ToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                        uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RGB24ToARGBRow_SSSE3(src_argb, row, pix);
  RGB24ToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

void RAWToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RAWToARGBRow_SSSE3(src_argb, row, pix);
  RAWToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

#endif
#endif
#ifdef HAS_FASTCONVERTYUVTOARGBROW_SSSE3
// TODO(fbarchard): ARGBToRGB565Row_SSSE3
void FastConvertYUVToRGB565Row_SSSE3(const uint8* y_buf,
                                     const uint8* u_buf,
                                     const uint8* v_buf,
                                     uint8* rgb_buf,
                                     int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_SSSE3(y_buf, u_buf, v_buf, row, width);
  ARGBToRGB565Row_C(row, rgb_buf, width);
}

void FastConvertYUVToARGB1555Row_SSSE3(const uint8* y_buf,
                                       const uint8* u_buf,
                                       const uint8* v_buf,
                                       uint8* rgb_buf,
                                       int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_SSSE3(y_buf, u_buf, v_buf, row, width);
  ARGBToARGB1555Row_C(row, rgb_buf, width);
}

void FastConvertYUVToARGB4444Row_SSSE3(const uint8* y_buf,
                                       const uint8* u_buf,
                                       const uint8* v_buf,
                                       uint8* rgb_buf,
                                       int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_SSSE3(y_buf, u_buf, v_buf, row, width);
  ARGBToARGB4444Row_C(row, rgb_buf, width);
}

void FastConvertYUVToRGB24Row_SSSE3(const uint8* y_buf,
                                    const uint8* u_buf,
                                    const uint8* v_buf,
                                    uint8* rgb_buf,
                                    int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_SSSE3(y_buf, u_buf, v_buf, row, width);
  ARGBToRGB24Row_C(row, rgb_buf, width);
}

void FastConvertYUVToRAWRow_SSSE3(const uint8* y_buf,
                                  const uint8* u_buf,
                                  const uint8* v_buf,
                                  uint8* rgb_buf,
                                  int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_SSSE3(y_buf, u_buf, v_buf, row, width);
  ARGBToRAWRow_C(row, rgb_buf, width);
}
#endif
#ifdef HAS_FASTCONVERTYUVTOARGBROW_NEON
// TODO(fbarchard): ARGBToRGB565Row_NEON
void FastConvertYUVToRGB565Row_NEON(const uint8* y_buf,
                                     const uint8* u_buf,
                                     const uint8* v_buf,
                                     uint8* rgb_buf,
                                     int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_NEON(y_buf, u_buf, v_buf, row, width);
  ARGBToRGB565Row_C(row, rgb_buf, width);
}

void FastConvertYUVToARGB1555Row_NEON(const uint8* y_buf,
                                       const uint8* u_buf,
                                       const uint8* v_buf,
                                       uint8* rgb_buf,
                                       int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_NEON(y_buf, u_buf, v_buf, row, width);
  ARGBToARGB1555Row_C(row, rgb_buf, width);
}

void FastConvertYUVToARGB4444Row_NEON(const uint8* y_buf,
                                       const uint8* u_buf,
                                       const uint8* v_buf,
                                       uint8* rgb_buf,
                                       int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_NEON(y_buf, u_buf, v_buf, row, width);
  ARGBToARGB4444Row_C(row, rgb_buf, width);
}

void FastConvertYUVToRGB24Row_NEON(const uint8* y_buf,
                                    const uint8* u_buf,
                                    const uint8* v_buf,
                                    uint8* rgb_buf,
                                    int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_NEON(y_buf, u_buf, v_buf, row, width);
  ARGBToRGB24Row_C(row, rgb_buf, width);
}

void FastConvertYUVToRAWRow_NEON(const uint8* y_buf,
                                  const uint8* u_buf,
                                  const uint8* v_buf,
                                  uint8* rgb_buf,
                                  int width) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  FastConvertYUVToARGBRow_NEON(y_buf, u_buf, v_buf, row, width);
  ARGBToRAWRow_C(row, rgb_buf, width);
}
#endif

void I400ToARGBRow_C(const uint8* src_y, uint8* dst_argb, int pix) {
  // Copy a Y to RGB.
  for (int x = 0; x < pix; ++x) {
    uint8 y = src_y[0];
    dst_argb[2] = dst_argb[1] = dst_argb[0] = y;
    dst_argb[3] = 255u;
    dst_argb += 4;
    ++src_y;
  }
}

// C reference code that mimics the YUV assembly.

#define YG 74 /* static_cast<int8>(1.164 * 64 + 0.5) */

#define UB 127 /* min(63,static_cast<int8>(2.018 * 64)) */
#define UG -25 /* static_cast<int8>(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* static_cast<int8>(-0.813 * 64 - 0.5) */
#define VR 102 /* static_cast<int8>(1.596 * 64 + 0.5) */

// Bias
#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

static __inline uint32 Clip(int32 val) {
  if (val < 0) {
    return (uint32) 0;
  } else if (val > 255){
    return (uint32) 255;
  }
  return (uint32) val;
}

static __inline void YuvPixel(uint8 y, uint8 u, uint8 v, uint8* rgb_buf,
                            int ashift, int rshift, int gshift, int bshift) {
  int32 y1 = (static_cast<int32>(y) - 16) * YG;
  uint32 b = Clip(static_cast<int32>((u * UB + v * VB) - (BB) + y1) >> 6);
  uint32 g = Clip(static_cast<int32>((u * UG + v * VG) - (BG) + y1) >> 6);
  uint32 r = Clip(static_cast<int32>((u * UR + v * VR) - (BR) + y1) >> 6);
  *reinterpret_cast<uint32*>(rgb_buf) = (b << bshift) |
                                        (g << gshift) |
                                        (r << rshift) |
                                        (255u << ashift);
}

void FastConvertYUVToARGBRow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 4, 24, 16, 8, 0);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 24, 16, 8, 0);
  }
}

void FastConvertYUVToBGRARow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 0, 8, 16, 24);
    YuvPixel(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 4, 0, 8, 16, 24);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf, 0, 8, 16, 24);
  }
}

void FastConvertYUVToABGRRow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 24, 0, 8, 16);
    YuvPixel(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 4, 24, 0, 8, 16);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 24, 0, 8, 16);
  }
}

void FastConvertYUV444ToARGBRow_C(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgb_buf,
                                   int width) {
  for (int x = 0; x < width; ++x) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf, 24, 16, 8, 0);
    y_buf += 1;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 4;  // Advance 1 pixel.
  }
}

static __inline void YuvPixel16(uint8 y, uint8 u, uint8 v, uint8* rgb_buf,
                                int ar, int rr,
                                int gr, int br,
                                int ashift, int rshift,
                                int gshift, int bshift) {
  int32 y1 = (static_cast<int32>(y) - 16) * YG;
  uint32 a = 255u >> ar;
  uint32 b = Clip(static_cast<int32>((u * UB + v * VB) - (BB) + y1) >> 6) >> br;
  uint32 g = Clip(static_cast<int32>((u * UG + v * VG) - (BG) + y1) >> 6) >> gr;
  uint32 r = Clip(static_cast<int32>((u * UR + v * VR) - (BR) + y1) >> 6) >> rr;
  *reinterpret_cast<uint16*>(rgb_buf) = (b << bshift) |
                                        (g << gshift) |
                                        (r << rshift) |
                                        (a << ashift);
}

void FastConvertYUVToRGB565Row_C(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel16(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0,
        8, 3, 2, 3,  0, 11, 5, 0);
    YuvPixel16(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 2,
        8, 3, 2, 3,  0, 11, 5, 0);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 4;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel16(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0,
        8, 3, 2, 3,  0, 11, 5, 0);
  }
}

void FastConvertYUVToARGB1555Row_C(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgb_buf,
                                   int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel16(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0,
        7, 3, 3, 3,  15, 10, 5, 0);
    YuvPixel16(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 2,
        7, 3, 3, 3,  15, 10, 5, 0);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 4;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel16(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0,
        7, 3, 3, 3,  15, 10, 5, 0);
  }
}

void FastConvertYUVToARGB4444Row_C(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgb_buf,
                                   int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel16(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0,
        4, 4, 4, 4,  12, 8, 4, 0);
    YuvPixel16(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 2,
        4, 4, 4, 4,  12, 8, 4, 0);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 4;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel16(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0,
        4, 4, 4, 4,  12, 8, 4, 0);
  }
}

static __inline void YuvPixel24(uint8 y, uint8 u, uint8 v, uint8* rgb_buf,
                                int roffset, int goffset, int boffset) {
  int32 y1 = (static_cast<int32>(y) - 16) * YG;
  uint32 b = Clip(static_cast<int32>((u * UB + v * VB) - (BB) + y1) >> 6);
  uint32 g = Clip(static_cast<int32>((u * UG + v * VG) - (BG) + y1) >> 6);
  uint32 r = Clip(static_cast<int32>((u * UR + v * VR) - (BR) + y1) >> 6);
  rgb_buf[boffset] = static_cast<uint8>(b);
  rgb_buf[goffset] = static_cast<uint8>(g);
  rgb_buf[roffset] = static_cast<uint8>(r);
}

void FastConvertYUVToRGB24Row_C(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* rgb_buf,
                                int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel24(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 2, 1, 0);
    YuvPixel24(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 3, 2, 1, 0);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 6;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel24(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 2, 1, 0);
  }
}

void FastConvertYUVToRAWRow_C(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel24(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 0, 1, 2);
    YuvPixel24(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 3, 0, 1, 2);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 6;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel24(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 0, 1, 2);
  }
}

void FastConvertYToARGBRow_C(const uint8* y_buf,
                             uint8* rgb_buf,
                             int width) {
  for (int x = 0; x < width; ++x) {
    YuvPixel(y_buf[0], 128, 128, rgb_buf, 24, 16, 8, 0);
    y_buf += 1;
    rgb_buf += 4;  // Advance 1 pixel.
  }
}

void ReverseRow_C(const uint8* src, uint8* dst, int width) {
  src += width - 1;
  for (int i = 0; i < width; ++i) {
    dst[i] = src[0];
    --src;
  }
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

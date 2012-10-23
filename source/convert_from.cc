/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/convert_from.h"

#include "libyuv/basic_types.h"
#include "libyuv/convert.h"  // For I420Copy
#include "libyuv/cpu_id.h"
#include "libyuv/format_conversion.h"
#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"
#include "libyuv/video_common.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

LIBYUV_API
int I420ToI422(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_y = dst_y + (height - 1) * dst_stride_y;
    dst_u = dst_u + (height - 1) * dst_stride_u;
    dst_v = dst_v + (height - 1) * dst_stride_v;
    dst_stride_y = -dst_stride_y;
    dst_stride_u = -dst_stride_u;
    dst_stride_v = -dst_stride_v;
  }
  int halfwidth = (width + 1) >> 1;
  void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;
#if defined(HAS_COPYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(halfwidth, 32)) {
    CopyRow = CopyRow_NEON;
  }
#elif defined(HAS_COPYROW_X86)
  if (IS_ALIGNED(halfwidth, 4)) {
    CopyRow = CopyRow_X86;
#if defined(HAS_COPYROW_SSE2)
    if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(halfwidth, 32) &&
        IS_ALIGNED(src_u, 16) && IS_ALIGNED(src_stride_u, 16) &&
        IS_ALIGNED(src_v, 16) && IS_ALIGNED(src_stride_v, 16) &&
        IS_ALIGNED(dst_u, 16) && IS_ALIGNED(dst_stride_u, 16) &&
        IS_ALIGNED(dst_v, 16) && IS_ALIGNED(dst_stride_v, 16)) {
      CopyRow = CopyRow_SSE2;
    }
#endif
  }
#endif

  // Copy Y plane
  if (dst_y) {
    CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }

  // UpSample U plane.
  int y;
  for (y = 0; y < height - 1; y += 2) {
    CopyRow(src_u, dst_u, halfwidth);
    CopyRow(src_u, dst_u + dst_stride_u, halfwidth);
    src_u += src_stride_u;
    dst_u += dst_stride_u * 2;
  }
  if (height & 1) {
    CopyRow(src_u, dst_u, halfwidth);
  }

  // UpSample V plane.
  for (y = 0; y < height - 1; y += 2) {
    CopyRow(src_v, dst_v, halfwidth);
    CopyRow(src_v, dst_v + dst_stride_v, halfwidth);
    src_v += src_stride_v;
    dst_v += dst_stride_v * 2;
  }
  if (height & 1) {
    CopyRow(src_v, dst_v, halfwidth);
  }
  return 0;
}

// use Bilinear for upsampling chroma
void ScalePlaneBilinear(int src_width, int src_height,
                        int dst_width, int dst_height,
                        int src_stride, int dst_stride,
                        const uint8* src_ptr, uint8* dst_ptr);

LIBYUV_API
int I420ToI444(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_y || !src_u|| !src_v ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_y = dst_y + (height - 1) * dst_stride_y;
    dst_u = dst_u + (height - 1) * dst_stride_u;
    dst_v = dst_v + (height - 1) * dst_stride_v;
    dst_stride_y = -dst_stride_y;
    dst_stride_u = -dst_stride_u;
    dst_stride_v = -dst_stride_v;
  }

  // Copy Y plane
  if (dst_y) {
    CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }

  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;

  // Upsample U plane.
  ScalePlaneBilinear(halfwidth, halfheight,
                     width, height,
                     src_stride_u,
                     dst_stride_u,
                     src_u, dst_u);

  // Upsample V plane.
  ScalePlaneBilinear(halfwidth, halfheight,
                     width, height,
                     src_stride_v,
                     dst_stride_v,
                     src_v, dst_v);
  return 0;
}

// 420 chroma is 1/2 width, 1/2 height
// 411 chroma is 1/4 width, 1x height
LIBYUV_API
int I420ToI411(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_y || !dst_u || !dst_v ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_y = dst_y + (height - 1) * dst_stride_y;
    dst_u = dst_u + (height - 1) * dst_stride_u;
    dst_v = dst_v + (height - 1) * dst_stride_v;
    dst_stride_y = -dst_stride_y;
    dst_stride_u = -dst_stride_u;
    dst_stride_v = -dst_stride_v;
  }

  // Copy Y plane
  if (dst_y) {
    CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }

  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;
  int quarterwidth = (width + 3) >> 2;

  // Resample U plane.
  ScalePlaneBilinear(halfwidth, halfheight,  // from 1/2 width, 1/2 height
                     quarterwidth, height,  // to 1/4 width, 1x height
                     src_stride_u,
                     dst_stride_u,
                     src_u, dst_u);

  // Resample V plane.
  ScalePlaneBilinear(halfwidth, halfheight,  // from 1/2 width, 1/2 height
                     quarterwidth, height,  // to 1/4 width, 1x height
                     src_stride_v,
                     dst_stride_v,
                     src_v, dst_v);
  return 0;
}

// Copy to I400. Source can be I420,422,444,400,NV12,NV21
LIBYUV_API
int I400Copy(const uint8* src_y, int src_stride_y,
             uint8* dst_y, int dst_stride_y,
             int width, int height) {
  if (!src_y || !dst_y ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_stride_y = -src_stride_y;
  }
  CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  return 0;
}

// Visual C x86 or GCC little endian.
#if defined(__x86_64__) || defined(_M_X64) || \
  defined(__i386__) || defined(_M_IX86) || \
  defined(__arm__) || defined(_M_ARM) || \
  (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define LIBYUV_LITTLE_ENDIAN
#endif

#ifdef LIBYUV_LITTLE_ENDIAN
#define WRITEWORD(p, v) *reinterpret_cast<uint32*>(p) = v
#else
static inline void WRITEWORD(uint8* p, uint32 v) {
  p[0] = (uint8)(v & 255);
  p[1] = (uint8)((v >> 8) & 255);
  p[2] = (uint8)((v >> 16) & 255);
  p[3] = (uint8)((v >> 24) & 255);
}
#endif

#define EIGHTTOTEN(x) (x << 2 | x >> 6)
static void UYVYToV210Row_C(const uint8* src_uyvy, uint8* dst_v210, int width) {
  for (int x = 0; x < width; x += 6) {
    WRITEWORD(dst_v210 + 0, (EIGHTTOTEN(src_uyvy[0])) |
                            (EIGHTTOTEN(src_uyvy[1]) << 10) |
                            (EIGHTTOTEN(src_uyvy[2]) << 20));
    WRITEWORD(dst_v210 + 4, (EIGHTTOTEN(src_uyvy[3])) |
                            (EIGHTTOTEN(src_uyvy[4]) << 10) |
                            (EIGHTTOTEN(src_uyvy[5]) << 20));
    WRITEWORD(dst_v210 + 8, (EIGHTTOTEN(src_uyvy[6])) |
                            (EIGHTTOTEN(src_uyvy[7]) << 10) |
                            (EIGHTTOTEN(src_uyvy[8]) << 20));
    WRITEWORD(dst_v210 + 12, (EIGHTTOTEN(src_uyvy[9])) |
                             (EIGHTTOTEN(src_uyvy[10]) << 10) |
                             (EIGHTTOTEN(src_uyvy[11]) << 20));
    src_uyvy += 12;
    dst_v210 += 16;
  }
}

LIBYUV_API
int I422ToYUY2(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_frame ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_frame = dst_frame + (height - 1) * dst_stride_frame;
    dst_stride_frame = -dst_stride_frame;
  }
  void (*I422ToYUY2Row)(const uint8* src_y, const uint8* src_u,
                        const uint8* src_v, uint8* dst_frame, int width) =
      I422ToYUY2Row_C;
#if defined(HAS_I422TOYUY2ROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 16 &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_frame, 16) && IS_ALIGNED(dst_stride_frame, 16)) {
    I422ToYUY2Row = I422ToYUY2Row_Any_SSE2;
    if (IS_ALIGNED(width, 16)) {
      I422ToYUY2Row = I422ToYUY2Row_SSE2;
    }
  }
#elif defined(HAS_I422TOYUY2ROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 16) {
    I422ToYUY2Row = I422ToYUY2Row_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToYUY2Row = I422ToYUY2Row_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToYUY2Row(src_y, src_u, src_y, dst_frame, width);
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
    dst_frame += dst_stride_frame;
  }
  return 0;
}

LIBYUV_API
int I420ToYUY2(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_frame ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_frame = dst_frame + (height - 1) * dst_stride_frame;
    dst_stride_frame = -dst_stride_frame;
  }
  void (*I422ToYUY2Row)(const uint8* src_y, const uint8* src_u,
                        const uint8* src_v, uint8* dst_frame, int width) =
      I422ToYUY2Row_C;
#if defined(HAS_I422TOYUY2ROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 16 &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_frame, 16) && IS_ALIGNED(dst_stride_frame, 16)) {
    I422ToYUY2Row = I422ToYUY2Row_Any_SSE2;
    if (IS_ALIGNED(width, 16)) {
      I422ToYUY2Row = I422ToYUY2Row_SSE2;
    }
  }
#elif defined(HAS_I422TOYUY2ROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 16) {
    I422ToYUY2Row = I422ToYUY2Row_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToYUY2Row = I422ToYUY2Row_NEON;
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    I422ToYUY2Row(src_y, src_u, src_v, dst_frame, width);
    I422ToYUY2Row(src_y + src_stride_y, src_u, src_v,
                  dst_frame + dst_stride_frame, width);
    src_y += src_stride_y * 2;
    src_u += src_stride_u;
    src_v += src_stride_v;
    dst_frame += dst_stride_frame * 2;
  }
  if (height & 1) {
    I422ToYUY2Row(src_y, src_u, src_v, dst_frame, width);
  }
  return 0;
}

// TODO(fbarchard): Deprecate, move or expand 422 support?
LIBYUV_API
int I422ToUYVY(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_frame ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_frame = dst_frame + (height - 1) * dst_stride_frame;
    dst_stride_frame = -dst_stride_frame;
  }
  void (*I422ToUYVYRow)(const uint8* src_y, const uint8* src_u,
                        const uint8* src_v, uint8* dst_frame, int width) =
      I422ToUYVYRow_C;
#if defined(HAS_I422TOUYVYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 16 &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_frame, 16) && IS_ALIGNED(dst_stride_frame, 16)) {
    I422ToUYVYRow = I422ToUYVYRow_Any_SSE2;
    if (IS_ALIGNED(width, 16)) {
      I422ToUYVYRow = I422ToUYVYRow_SSE2;
    }
  }
#elif defined(HAS_I422TOUYVYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 16) {
    I422ToUYVYRow = I422ToUYVYRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToUYVYRow = I422ToUYVYRow_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToUYVYRow(src_y, src_u, src_y, dst_frame, width);
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
    dst_frame += dst_stride_frame;
  }
  return 0;
}

LIBYUV_API
int I420ToUYVY(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_frame ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_frame = dst_frame + (height - 1) * dst_stride_frame;
    dst_stride_frame = -dst_stride_frame;
  }
  void (*I422ToUYVYRow)(const uint8* src_y, const uint8* src_u,
                        const uint8* src_v, uint8* dst_frame, int width) =
      I422ToUYVYRow_C;
#if defined(HAS_I422TOUYVYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && width >= 16 &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_frame, 16) && IS_ALIGNED(dst_stride_frame, 16)) {
    I422ToUYVYRow = I422ToUYVYRow_Any_SSE2;
    if (IS_ALIGNED(width, 16)) {
      I422ToUYVYRow = I422ToUYVYRow_SSE2;
    }
  }
#elif defined(HAS_I422TOUYVYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 16) {
    I422ToUYVYRow = I422ToUYVYRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToUYVYRow = I422ToUYVYRow_NEON;
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    I422ToUYVYRow(src_y, src_u, src_v, dst_frame, width);
    I422ToUYVYRow(src_y + src_stride_y, src_u, src_v,
                  dst_frame + dst_stride_frame, width);
    src_y += src_stride_y * 2;
    src_u += src_stride_u;
    src_v += src_stride_v;
    dst_frame += dst_stride_frame * 2;
  }
  if (height & 1) {
    I422ToUYVYRow(src_y, src_u, src_v, dst_frame, width);
  }
  return 0;
}

LIBYUV_API
int I420ToV210(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height) {
  if (width * 16 / 6 > kMaxStride ||
      !src_y || !src_u || !src_v || !dst_frame ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_frame = dst_frame + (height - 1) * dst_stride_frame;
    dst_stride_frame = -dst_stride_frame;
  }

  SIMD_ALIGNED(uint8 row[kMaxStride]);

  for (int y = 0; y < height - 1; y += 2) {
    I422ToUYVYRow_C(src_y, src_u, src_v, row, width);
    UYVYToV210Row_C(row, dst_frame, width);
    I422ToUYVYRow_C(src_y + src_stride_y, src_u, src_v, row, width);
    UYVYToV210Row_C(row, dst_frame + dst_stride_frame, width);
    src_y += src_stride_y * 2;
    src_u += src_stride_u;
    src_v += src_stride_v;
    dst_frame += dst_stride_frame * 2;
  }
  if (height & 1) {
    I422ToUYVYRow_C(src_y, src_u, src_v, row, width);
    UYVYToV210Row_C(row, dst_frame, width);
  }
  return 0;
}

LIBYUV_API
int I420ToNV12(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_uv, int dst_stride_uv,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_y || !dst_uv ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    int halfheight = (height + 1) >> 1;
    dst_y = dst_y + (height - 1) * dst_stride_y;
    dst_uv = dst_uv + (halfheight - 1) * dst_stride_uv;
    dst_stride_y = -dst_stride_y;
    dst_stride_uv = -dst_stride_uv;
  }

  int halfwidth = (width + 1) >> 1;
  void (*MergeUV)(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                  int width) = MergeUV_C;
#if defined(HAS_SPLITUV_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(halfwidth, 16)) {
    MergeUV = MergeUV_NEON;
  }
#endif
  CopyPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  int halfheight = (height + 1) >> 1;
  for (int y = 0; y < halfheight; ++y) {
    // Copy a row of UV.
    MergeUV_C(src_u, src_v, dst_uv, halfwidth);
    src_u += src_stride_u;
    src_v += src_stride_v;
    dst_uv += dst_stride_uv;
  }
  return 0;
}

LIBYUV_API
int I420ToNV21(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_vu, int dst_stride_vu,
               int width, int height) {
  return I420ToNV12(src_y, src_stride_y,
                    src_v, src_stride_v,
                    src_u, src_stride_u,
                    dst_y, src_stride_y,
                    dst_vu, dst_stride_vu,
                    width, height);
}

// Convert I420 to ARGB.
LIBYUV_API
int I420ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_argb ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*I422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToARGBRow_C;
#if defined(HAS_I422TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToARGBRow = I422ToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToARGBRow = I422ToARGBRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
        I422ToARGBRow = I422ToARGBRow_SSSE3;
      }
    }
  }
#elif defined(HAS_I422TOARGBROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    I422ToARGBRow = I422ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      I422ToARGBRow = I422ToARGBRow_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToARGBRow(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to BGRA.
LIBYUV_API
int I420ToBGRA(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_bgra, int dst_stride_bgra,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_bgra ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_bgra = dst_bgra + (height - 1) * dst_stride_bgra;
    dst_stride_bgra = -dst_stride_bgra;
  }
  void (*I422ToBGRARow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToBGRARow_C;
#if defined(HAS_I422TOBGRAROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToBGRARow = I422ToBGRARow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToBGRARow = I422ToBGRARow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_bgra, 16) && IS_ALIGNED(dst_stride_bgra, 16)) {
        I422ToBGRARow = I422ToBGRARow_SSSE3;
      }
    }
  }
#elif defined(HAS_I422TOBGRAROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    I422ToBGRARow = I422ToBGRARow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      I422ToBGRARow = I422ToBGRARow_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToBGRARow(src_y, src_u, src_v, dst_bgra, width);
    dst_bgra += dst_stride_bgra;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to ABGR.
LIBYUV_API
int I420ToABGR(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_abgr, int dst_stride_abgr,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_abgr ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_abgr = dst_abgr + (height - 1) * dst_stride_abgr;
    dst_stride_abgr = -dst_stride_abgr;
  }
  void (*I422ToABGRRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToABGRRow_C;
#if defined(HAS_I422TOABGRROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToABGRRow = I422ToABGRRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToABGRRow = I422ToABGRRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_abgr, 16) && IS_ALIGNED(dst_stride_abgr, 16)) {
        I422ToABGRRow = I422ToABGRRow_SSSE3;
      }
    }
  }
#elif defined(HAS_I422TOABGRROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    I422ToABGRRow = I422ToABGRRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      I422ToABGRRow = I422ToABGRRow_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToABGRRow(src_y, src_u, src_v, dst_abgr, width);
    dst_abgr += dst_stride_abgr;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to RGBA.
LIBYUV_API
int I420ToRGBA(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_rgba, int dst_stride_rgba,
               int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_rgba ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_rgba = dst_rgba + (height - 1) * dst_stride_rgba;
    dst_stride_rgba = -dst_stride_rgba;
  }
  void (*I422ToRGBARow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToRGBARow_C;
#if defined(HAS_I422TORGBAROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToRGBARow = I422ToRGBARow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToRGBARow = I422ToRGBARow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_rgba, 16) && IS_ALIGNED(dst_stride_rgba, 16)) {
        I422ToRGBARow = I422ToRGBARow_SSSE3;
      }
    }
  }
#elif defined(HAS_I422TORGBAROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    I422ToRGBARow = I422ToRGBARow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      I422ToRGBARow = I422ToRGBARow_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToRGBARow(src_y, src_u, src_v, dst_rgba, width);
    dst_rgba += dst_stride_rgba;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to RGB24.
LIBYUV_API
int I420ToRGB24(const uint8* src_y, int src_stride_y,
                const uint8* src_u, int src_stride_u,
                const uint8* src_v, int src_stride_v,
                uint8* dst_rgb24, int dst_stride_rgb24,
                int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_rgb24 ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_rgb24 = dst_rgb24 + (height - 1) * dst_stride_rgb24;
    dst_stride_rgb24 = -dst_stride_rgb24;
  }
  void (*I422ToRGB24Row)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToRGB24Row_C;
#if defined(HAS_I422TORGB24ROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToRGB24Row = I422ToRGB24Row_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToRGB24Row = I422ToRGB24Row_SSSE3;
    }
  }
#elif defined(HAS_I422TORGB24ROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    I422ToRGB24Row = I422ToRGB24Row_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      I422ToRGB24Row = I422ToRGB24Row_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToRGB24Row(src_y, src_u, src_v, dst_rgb24, width);
    dst_rgb24 += dst_stride_rgb24;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to RAW.
LIBYUV_API
int I420ToRAW(const uint8* src_y, int src_stride_y,
                const uint8* src_u, int src_stride_u,
                const uint8* src_v, int src_stride_v,
                uint8* dst_raw, int dst_stride_raw,
                int width, int height) {
  if (!src_y || !src_u || !src_v || !dst_raw ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_raw = dst_raw + (height - 1) * dst_stride_raw;
    dst_stride_raw = -dst_stride_raw;
  }
  void (*I422ToRAWRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToRAWRow_C;
#if defined(HAS_I422TORAWROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToRAWRow = I422ToRAWRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToRAWRow = I422ToRAWRow_SSSE3;
    }
  }
#elif defined(HAS_I422TORAWROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && width >= 8) {
    I422ToRAWRow = I422ToRAWRow_Any_NEON;
    if (IS_ALIGNED(width, 8)) {
      I422ToRAWRow = I422ToRAWRow_NEON;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToRAWRow(src_y, src_u, src_v, dst_raw, width);
    dst_raw += dst_stride_raw;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to RGB565.
LIBYUV_API
int I420ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_u, int src_stride_u,
                 const uint8* src_v, int src_stride_v,
                 uint8* dst_rgb, int dst_stride_rgb,
                 int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_rgb ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_rgb = dst_rgb + (height - 1) * dst_stride_rgb;
    dst_stride_rgb = -dst_stride_rgb;
  }
  void (*I422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToARGBRow_C;
#if defined(HAS_I422TOARGBROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I422ToARGBRow = I422ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToARGBRow = I422ToARGBRow_NEON;
    }
  }
#elif defined(HAS_I422TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToARGBRow = I422ToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToARGBRow = I422ToARGBRow_SSSE3;
    }
  }
#endif

  SIMD_ALIGNED(uint8 row[kMaxStride]);
  void (*ARGBToRGB565Row)(const uint8* src_rgb, uint8* dst_rgb, int pix) =
      ARGBToRGB565Row_C;
#if defined(HAS_ARGBTORGB565ROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    if (width * 2 <= kMaxStride) {
      ARGBToRGB565Row = ARGBToRGB565Row_Any_SSE2;
    }
    if (IS_ALIGNED(width, 4)) {
      ARGBToRGB565Row = ARGBToRGB565Row_SSE2;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToARGBRow(src_y, src_u, src_v, row, width);
    ARGBToRGB565Row(row, dst_rgb, width);
    dst_rgb += dst_stride_rgb;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to ARGB1555.
LIBYUV_API
int I420ToARGB1555(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_argb ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*I422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToARGBRow_C;
#if defined(HAS_I422TOARGBROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I422ToARGBRow = I422ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToARGBRow = I422ToARGBRow_NEON;
    }
  }
#elif defined(HAS_I422TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToARGBRow = I422ToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToARGBRow = I422ToARGBRow_SSSE3;
    }
  }
#endif

  SIMD_ALIGNED(uint8 row[kMaxStride]);
  void (*ARGBToARGB1555Row)(const uint8* src_argb, uint8* dst_rgb, int pix) =
      ARGBToARGB1555Row_C;
#if defined(HAS_ARGBTOARGB1555ROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    if (width * 2 <= kMaxStride) {
      ARGBToARGB1555Row = ARGBToARGB1555Row_Any_SSE2;
    }
    if (IS_ALIGNED(width, 4)) {
      ARGBToARGB1555Row = ARGBToARGB1555Row_SSE2;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToARGBRow(src_y, src_u, src_v, row, width);
    ARGBToARGB1555Row(row, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to ARGB4444.
LIBYUV_API
int I420ToARGB4444(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_argb, int dst_stride_argb,
                   int width, int height) {
  if (!src_y || !src_u || !src_v ||
      !dst_argb ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*I422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToARGBRow_C;
#if defined(HAS_I422TOARGBROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I422ToARGBRow = I422ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I422ToARGBRow = I422ToARGBRow_NEON;
    }
  }
#elif defined(HAS_I422TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I422ToARGBRow = I422ToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8)) {
      I422ToARGBRow = I422ToARGBRow_SSSE3;
    }
  }
#endif

  SIMD_ALIGNED(uint8 row[kMaxStride]);
  void (*ARGBToARGB4444Row)(const uint8* src_argb, uint8* dst_rgb, int pix) =
     ARGBToARGB4444Row_C;
#if defined(HAS_ARGBTOARGB4444ROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    if (width * 2 <= kMaxStride) {
      ARGBToARGB4444Row = ARGBToARGB4444Row_Any_SSE2;
    }
    if (IS_ALIGNED(width, 4)) {
      ARGBToARGB4444Row = ARGBToARGB4444Row_SSE2;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I422ToARGBRow(src_y, src_u, src_v, row, width);
    ARGBToARGB4444Row(row, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    if (y & 1) {
      src_u += src_stride_u;
      src_v += src_stride_v;
    }
  }
  return 0;
}

// Convert I420 to specified format
LIBYUV_API
int ConvertFromI420(const uint8* y, int y_stride,
                    const uint8* u, int u_stride,
                    const uint8* v, int v_stride,
                    uint8* dst_sample, int dst_sample_stride,
                    int width, int height,
                    uint32 format) {
  if (!y || !u|| !v || !dst_sample ||
      width <= 0 || height == 0) {
    return -1;
  }
  int r = 0;
  switch (format) {
    // Single plane formats
    case FOURCC_YUY2:
      r = I420ToYUY2(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample,
                     dst_sample_stride ? dst_sample_stride : width * 2,
                     width, height);
      break;
    case FOURCC_UYVY:
      r = I420ToUYVY(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample,
                     dst_sample_stride ? dst_sample_stride : width * 2,
                     width, height);
      break;
    case FOURCC_V210:
      r = I420ToV210(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample,
                     dst_sample_stride ? dst_sample_stride :
                         (width + 47) / 48 * 128,
                     width, height);
      break;
    case FOURCC_RGBP:
      r = I420ToRGB565(y, y_stride,
                       u, u_stride,
                       v, v_stride,
                       dst_sample,
                       dst_sample_stride ? dst_sample_stride : width * 2,
                       width, height);
      break;
    case FOURCC_RGBO:
      r = I420ToARGB1555(y, y_stride,
                         u, u_stride,
                         v, v_stride,
                         dst_sample,
                         dst_sample_stride ? dst_sample_stride : width * 2,
                         width, height);
      break;
    case FOURCC_R444:
      r = I420ToARGB4444(y, y_stride,
                         u, u_stride,
                         v, v_stride,
                         dst_sample,
                         dst_sample_stride ? dst_sample_stride : width * 2,
                         width, height);
      break;
    case FOURCC_24BG:
      r = I420ToRGB24(y, y_stride,
                      u, u_stride,
                      v, v_stride,
                      dst_sample,
                      dst_sample_stride ? dst_sample_stride : width * 3,
                      width, height);
      break;
    case FOURCC_RAW:
      r = I420ToRAW(y, y_stride,
                    u, u_stride,
                    v, v_stride,
                    dst_sample,
                    dst_sample_stride ? dst_sample_stride : width * 3,
                    width, height);
      break;
    case FOURCC_ARGB:
      r = I420ToARGB(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample,
                     dst_sample_stride ? dst_sample_stride : width * 4,
                     width, height);
      break;
    case FOURCC_BGRA:
      r = I420ToBGRA(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample,
                     dst_sample_stride ? dst_sample_stride : width * 4,
                     width, height);
      break;
    case FOURCC_ABGR:
      r = I420ToABGR(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample,
                     dst_sample_stride ? dst_sample_stride : width * 4,
                     width, height);
      break;
    case FOURCC_RGBA:
      r = I420ToRGBA(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample,
                     dst_sample_stride ? dst_sample_stride : width * 4,
                     width, height);
      break;
    case FOURCC_BGGR:
      r = I420ToBayerBGGR(y, y_stride,
                          u, u_stride,
                          v, v_stride,
                          dst_sample,
                          dst_sample_stride ? dst_sample_stride : width,
                          width, height);
      break;
    case FOURCC_GBRG:
      r = I420ToBayerGBRG(y, y_stride,
                          u, u_stride,
                          v, v_stride,
                          dst_sample,
                          dst_sample_stride ? dst_sample_stride : width,
                          width, height);
      break;
    case FOURCC_GRBG:
      r = I420ToBayerGRBG(y, y_stride,
                          u, u_stride,
                          v, v_stride,
                          dst_sample,
                          dst_sample_stride ? dst_sample_stride : width,
                          width, height);
      break;
    case FOURCC_RGGB:
      r = I420ToBayerRGGB(y, y_stride,
                          u, u_stride,
                          v, v_stride,
                          dst_sample,
                          dst_sample_stride ? dst_sample_stride : width,
                          width, height);
      break;
    case FOURCC_I400:
      r = I400Copy(y, y_stride,
                   dst_sample,
                   dst_sample_stride ? dst_sample_stride : width,
                   width, height);
      break;
    // Triplanar formats
    // TODO(fbarchard): halfstride instead of halfwidth
    case FOURCC_I420:
    case FOURCC_YU12:
    case FOURCC_YV12: {
      int halfwidth = (width + 1) / 2;
      int halfheight = (height + 1) / 2;
      uint8* dst_u;
      uint8* dst_v;
      if (format == FOURCC_YV12) {
        dst_v = dst_sample + width * height;
        dst_u = dst_v + halfwidth * halfheight;
      } else {
        dst_u = dst_sample + width * height;
        dst_v = dst_u + halfwidth * halfheight;
      }
      r = I420Copy(y, y_stride,
                   u, u_stride,
                   v, v_stride,
                   dst_sample, width,
                   dst_u, halfwidth,
                   dst_v, halfwidth,
                   width, height);
      break;
    }
    case FOURCC_I422:
    case FOURCC_YV16: {
      int halfwidth = (width + 1) / 2;
      uint8* dst_u;
      uint8* dst_v;
      if (format == FOURCC_YV16) {
        dst_v = dst_sample + width * height;
        dst_u = dst_v + halfwidth * height;
      } else {
        dst_u = dst_sample + width * height;
        dst_v = dst_u + halfwidth * height;
      }
      r = I420ToI422(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample, width,
                     dst_u, halfwidth,
                     dst_v, halfwidth,
                     width, height);
      break;
    }
    case FOURCC_I444:
    case FOURCC_YV24: {
      uint8* dst_u;
      uint8* dst_v;
      if (format == FOURCC_YV24) {
        dst_v = dst_sample + width * height;
        dst_u = dst_v + width * height;
      } else {
        dst_u = dst_sample + width * height;
        dst_v = dst_u + width * height;
      }
      r = I420ToI444(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample, width,
                     dst_u, width,
                     dst_v, width,
                     width, height);
      break;
    }
    case FOURCC_I411: {
      int quarterwidth = (width + 3) / 4;
      uint8* dst_u = dst_sample + width * height;
      uint8* dst_v = dst_u + quarterwidth * height;
      r = I420ToI411(y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_sample, width,
                     dst_u, quarterwidth,
                     dst_v, quarterwidth,
                     width, height);
      break;
    }

    // Formats not supported - MJPG, biplanar, some rgb formats.
    default:
      return -1;  // unknown fourcc - return failure code.
  }
  return r;
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

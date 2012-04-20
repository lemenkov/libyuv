/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/planar_functions.h"

#include <stdio.h>  // printf()
#include <string.h>  // for memset()

#include "libyuv/cpu_id.h"
#include "source/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Copy a plane of data
void CopyPlane(const uint8* src_y, int src_stride_y,
               uint8* dst_y, int dst_stride_y,
               int width, int height) {
  void (*CopyRow)(const uint8* src, uint8* dst, int width) = CopyRow_C;
#if defined(HAS_COPYROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 64)) {
    CopyRow = CopyRow_NEON;
  }
#endif
#if defined(HAS_COPYROW_X86)
  if (TestCpuFlag(kCpuHasX86) && IS_ALIGNED(width, 4)) {
    CopyRow = CopyRow_X86;
  }
#endif
#if defined(HAS_COPYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 32) &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
    CopyRow = CopyRow_SSE2;
  }
#endif

  // Copy plane
  for (int y = 0; y < height; ++y) {
    CopyRow(src_y, dst_y, width);
    src_y += src_stride_y;
    dst_y += dst_stride_y;
  }
}

// Mirror a plane of data
void MirrorPlane(const uint8* src_y, int src_stride_y,
                 uint8* dst_y, int dst_stride_y,
                 int width, int height) {
  void (*MirrorRow)(const uint8* src, uint8* dst, int width) = MirrorRow_C;
#if defined(HAS_MIRRORROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 16)) {
    MirrorRow = MirrorRow_NEON;
  }
#endif
#if defined(HAS_MIRRORROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 16)) {
    MirrorRow = MirrorRow_SSE2;
#if defined(HAS_MIRRORROW_SSSE3)
    if (TestCpuFlag(kCpuHasSSSE3) &&
        IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16)) {
      MirrorRow = MirrorRow_SSSE3;
    }
#endif
  }
#endif

  // Mirror plane
  for (int y = 0; y < height; ++y) {
    MirrorRow(src_y, dst_y, width);
    src_y += src_stride_y;
    dst_y += dst_stride_y;
  }
}

// Mirror I420 with optional flipping
int I420Mirror(const uint8* src_y, int src_stride_y,
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
    int halfheight = (height + 1) >> 1;
    src_y = src_y + (height - 1) * src_stride_y;
    src_u = src_u + (halfheight - 1) * src_stride_u;
    src_v = src_v + (halfheight - 1) * src_stride_v;
    src_stride_y = -src_stride_y;
    src_stride_u = -src_stride_u;
    src_stride_v = -src_stride_v;
  }

  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;
  if (dst_y) {
    MirrorPlane(src_y, src_stride_y, dst_y, dst_stride_y, width, height);
  }
  MirrorPlane(src_u, src_stride_u, dst_u, dst_stride_u, halfwidth, halfheight);
  MirrorPlane(src_v, src_stride_v, dst_v, dst_stride_v, halfwidth, halfheight);
  return 0;
}

// Copy ARGB with optional flipping
int ARGBCopy(const uint8* src_argb, int src_stride_argb,
             uint8* dst_argb, int dst_stride_argb,
             int width, int height) {
  if (!src_argb ||
      !dst_argb ||
      width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }

  CopyPlane(src_argb, src_stride_argb, dst_argb, dst_stride_argb,
            width * 4, height);
  return 0;
}

// Get a blender that optimized for the CPU, alignment and pixel count.
// As there are 6 blenders to choose from, the caller should try to use
// the same blend function for all pixels if possible.
ARGBBlendRow GetARGBBlend(uint8* dst_argb, int dst_stride_argb, int width) {
  void (*ARGBBlendRow)(const uint8* src_argb, const uint8* src_argb1,
                       uint8* dst_argb, int width) = ARGBBlendRow_C;
#if defined(HAS_ARGBBLENDROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    ARGBBlendRow = ARGBBlendRow1_SSE2;
    if (width >= 4) {
      ARGBBlendRow = ARGBBlendRow_Any_SSE2;
      if (IS_ALIGNED(width, 4) &&
          IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
        ARGBBlendRow = ARGBBlendRow_Aligned_SSE2;
      }
    }
  }
#endif
#if defined(HAS_ARGBBLENDROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 4) {
    ARGBBlendRow = ARGBBlendRow_Any_SSSE3;
    if (IS_ALIGNED(width, 4) &&
        IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
      ARGBBlendRow = ARGBBlendRow_Aligned_SSSE3;
    }
  }
#endif
  return ARGBBlendRow;
}

// Alpha Blend 2 ARGB images and store to destination.
int ARGBBlend(const uint8* src_argb0, int src_stride_argb0,
               const uint8* src_argb1, int src_stride_argb1,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  if (!src_argb0 || !src_argb1 || !dst_argb || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*ARGBBlendRow)(const uint8* src_argb, const uint8* src_argb1,
                       uint8* dst_argb, int width) =
      GetARGBBlend(dst_argb, dst_stride_argb, width);

  for (int y = 0; y < height; ++y) {
    ARGBBlendRow(src_argb0, src_argb1, dst_argb, width);
    src_argb0 += src_stride_argb0;
    src_argb1 += src_stride_argb1;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Convert I422 to ARGB.
int I422ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*I420ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I420ToARGBRow_C;
#if defined(HAS_I420TOARGBROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I420ToARGBRow = I420ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I420ToARGBRow = I420ToARGBRow_NEON;
    }
  }
#elif defined(HAS_I420TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I420ToARGBRow = I420ToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8) &&
        IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
      I420ToARGBRow = I420ToARGBRow_SSSE3;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    I420ToARGBRow(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
  }
  return 0;
}

// Convert I444 to ARGB.
int I444ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*I444ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I444ToARGBRow_C;
#if defined(HAS_I444TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      IS_ALIGNED(width, 8) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    I444ToARGBRow = I444ToARGBRow_SSSE3;
  }
#endif

  for (int y = 0; y < height; ++y) {
    I444ToARGBRow(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
  }
  return 0;
}

// Convert I400 to ARGB.
int I400ToARGB_Reference(const uint8* src_y, int src_stride_y,
                         uint8* dst_argb, int dst_stride_argb,
                         int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*YToARGBRow)(const uint8* y_buf,
                     uint8* rgb_buf,
                     int width) = YToARGBRow_C;
#if defined(HAS_YTOARGBROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(width, 8) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    YToARGBRow = YToARGBRow_SSE2;
  }
#endif

  for (int y = 0; y < height; ++y) {
    YToARGBRow(src_y, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
  }
  return 0;
}

// Convert I400 to ARGB.
int I400ToARGB(const uint8* src_y, int src_stride_y,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_stride_y = -src_stride_y;
  }
  void (*I400ToARGBRow)(const uint8* src_y, uint8* dst_argb, int pix) =
      I400ToARGBRow_C;
#if defined(HAS_I400TOARGBROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(width, 8) &&
      IS_ALIGNED(src_y, 8) && IS_ALIGNED(src_stride_y, 8) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    I400ToARGBRow = I400ToARGBRow_SSE2;
  }
#endif

  for (int y = 0; y < height; ++y) {
    I400ToARGBRow(src_y, dst_argb, width);
    src_y += src_stride_y;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

int ABGRToARGB(const uint8* src_abgr, int src_stride_abgr,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_abgr = src_abgr + (height - 1) * src_stride_abgr;
    src_stride_abgr = -src_stride_abgr;
  }
  void (*ABGRToARGBRow)(const uint8* src_abgr, uint8* dst_argb, int pix) =
      ABGRToARGBRow_C;
#if defined(HAS_ABGRTOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_abgr, 16) && IS_ALIGNED(src_stride_abgr, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ABGRToARGBRow = ABGRToARGBRow_SSSE3;
  }
#endif

  for (int y = 0; y < height; ++y) {
    ABGRToARGBRow(src_abgr, dst_argb, width);
    src_abgr += src_stride_abgr;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Convert BGRA to ARGB.
int BGRAToARGB(const uint8* src_bgra, int src_stride_bgra,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_bgra = src_bgra + (height - 1) * src_stride_bgra;
    src_stride_bgra = -src_stride_bgra;
  }
  void (*BGRAToARGBRow)(const uint8* src_bgra, uint8* dst_argb, int pix) =
      BGRAToARGBRow_C;
#if defined(HAS_BGRATOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_bgra, 16) && IS_ALIGNED(src_stride_bgra, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    BGRAToARGBRow = BGRAToARGBRow_SSSE3;
  }
#endif

  for (int y = 0; y < height; ++y) {
    BGRAToARGBRow(src_bgra, dst_argb, width);
    src_bgra += src_stride_bgra;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Convert ARGB to I400.
int ARGBToI400(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix) =
      ARGBToYRow_C;
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
    ARGBToYRow = ARGBToYRow_SSSE3;
  }
#endif

  for (int y = 0; y < height; ++y) {
    ARGBToYRow(src_argb, dst_y, width);
    src_argb += src_stride_argb;
    dst_y += dst_stride_y;
  }
  return 0;
}

// Convert RAW to ARGB.
int RAWToARGB(const uint8* src_raw, int src_stride_raw,
              uint8* dst_argb, int dst_stride_argb,
              int width, int height) {
  if (height < 0) {
    height = -height;
    src_raw = src_raw + (height - 1) * src_stride_raw;
    src_stride_raw = -src_stride_raw;
  }
  void (*RAWToARGBRow)(const uint8* src_raw, uint8* dst_argb, int pix) =
      RAWToARGBRow_C;
#if defined(HAS_RAWTOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      IS_ALIGNED(width, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    RAWToARGBRow = RAWToARGBRow_SSSE3;
  }
#endif

  for (int y = 0; y < height; ++y) {
    RAWToARGBRow(src_raw, dst_argb, width);
    src_raw += src_stride_raw;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Convert RGB24 to ARGB.
int RGB24ToARGB(const uint8* src_rgb24, int src_stride_rgb24,
                uint8* dst_argb, int dst_stride_argb,
                int width, int height) {
  if (height < 0) {
    height = -height;
    src_rgb24 = src_rgb24 + (height - 1) * src_stride_rgb24;
    src_stride_rgb24 = -src_stride_rgb24;
  }
  void (*RGB24ToARGBRow)(const uint8* src_rgb24, uint8* dst_argb, int pix) =
      RGB24ToARGBRow_C;
#if defined(HAS_RGB24TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      IS_ALIGNED(width, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    RGB24ToARGBRow = RGB24ToARGBRow_SSSE3;
  }
#endif

  for (int y = 0; y < height; ++y) {
    RGB24ToARGBRow(src_rgb24, dst_argb, width);
    src_rgb24 += src_stride_rgb24;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Convert ARGB To RGB24.
int ARGBToRGB24(const uint8* src_argb, int src_stride_argb,
                uint8* dst_rgb24, int dst_stride_rgb24,
                int width, int height) {
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBToRGB24Row)(const uint8* src_argb, uint8* dst_rgb, int pix) =
      ARGBToRGB24Row_C;
#if defined(HAS_ARGBTORGB24ROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16)) {
    if (width * 3 <= kMaxStride) {
      ARGBToRGB24Row = ARGBToRGB24Row_Any_SSSE3;
    }
    if (IS_ALIGNED(width, 16) &&
        IS_ALIGNED(dst_rgb24, 16) && IS_ALIGNED(dst_stride_rgb24, 16)) {
      ARGBToRGB24Row = ARGBToRGB24Row_SSSE3;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    ARGBToRGB24Row(src_argb, dst_rgb24, width);
    src_argb += src_stride_argb;
    dst_rgb24 += dst_stride_rgb24;
  }
  return 0;
}

// Convert ARGB To RAW.
int ARGBToRAW(const uint8* src_argb, int src_stride_argb,
              uint8* dst_raw, int dst_stride_raw,
              int width, int height) {
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBToRAWRow)(const uint8* src_argb, uint8* dst_rgb, int pix) =
      ARGBToRAWRow_C;
#if defined(HAS_ARGBTORAWROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16)) {
    if (width * 3 <= kMaxStride) {
      ARGBToRAWRow = ARGBToRAWRow_Any_SSSE3;
    }
    if (IS_ALIGNED(width, 16) &&
        IS_ALIGNED(dst_raw, 16) && IS_ALIGNED(dst_stride_raw, 16)) {
      ARGBToRAWRow = ARGBToRAWRow_SSSE3;
    }
  }
#endif

  for (int y = 0; y < height; ++y) {
    ARGBToRAWRow(src_argb, dst_raw, width);
    src_argb += src_stride_argb;
    dst_raw += dst_stride_raw;
  }
  return 0;
}

// Convert NV12 to ARGB.
int NV12ToARGB(const uint8* src_y, int src_stride_y,
               const uint8* src_uv, int src_stride_uv,
               uint8* dst_argb, int dst_stride_argb,
               int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_argb = dst_argb + (height - 1) * dst_stride_argb;
    dst_stride_argb = -dst_stride_argb;
  }
  void (*I420ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* argb_buf,
                        int width) = I420ToARGBRow_C;
#if defined(HAS_I420TOARGBROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I420ToARGBRow = I420ToARGBRow_Any_NEON;
    if (IS_ALIGNED(width, 16)) {
      I420ToARGBRow = I420ToARGBRow_NEON;
    }
  }
#elif defined(HAS_I420TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && width >= 8) {
    I420ToARGBRow = I420ToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(width, 8) &&
        IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
      I420ToARGBRow = I420ToARGBRow_SSSE3;
    }
  }
#endif

  int halfwidth = (width + 1) >> 1;
  void (*SplitUV)(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) =
      SplitUV_C;
#if defined(HAS_SPLITUV_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    SplitUV = SplitUV_NEON;
  }
#elif defined(HAS_SPLITUV_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(src_uv, 16) && IS_ALIGNED(src_stride_uv, 16)) {
    SplitUV = SplitUV_SSE2;
  }
#endif
  SIMD_ALIGNED(uint8 rowuv[kMaxStride * 2]);

  for (int y = 0; y < height; ++y) {
    if ((y & 1) == 0) {
      // Copy a row of UV.
      SplitUV(src_uv, rowuv, rowuv + kMaxStride, halfwidth);
      src_uv += src_stride_uv;
    }
    I420ToARGBRow(src_y, rowuv, rowuv + kMaxStride, dst_argb, width);
    dst_argb += dst_stride_argb;
    src_y += src_stride_y;
  }
  return 0;
}

// Convert NV12 to RGB565.
int NV12ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_uv, int src_stride_uv,
                 uint8* dst_rgb, int dst_stride_rgb,
                 int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_rgb = dst_rgb + (height - 1) * dst_stride_rgb;
    dst_stride_rgb = -dst_stride_rgb;
  }
  void (*I420ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I420ToARGBRow_C;
#if defined(HAS_I420TOARGBROW_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    I420ToARGBRow = I420ToARGBRow_NEON;
  }
#elif defined(HAS_I420TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    I420ToARGBRow = I420ToARGBRow_SSSE3;
  }
#endif
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  void (*ARGBToRGB565Row)(const uint8* src_argb, uint8* dst_rgb, int pix) =
      ARGBToRGB565Row_C;
#if defined(HAS_ARGBTORGB565ROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 4)) {
    ARGBToRGB565Row = ARGBToRGB565Row_SSE2;
  }
#endif

  int halfwidth = (width + 1) >> 1;
  void (*SplitUV)(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) =
      SplitUV_C;
#if defined(HAS_SPLITUV_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    SplitUV = SplitUV_NEON;
  }
#elif defined(HAS_SPLITUV_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(src_uv, 16) && IS_ALIGNED(src_stride_uv, 16)) {
    SplitUV = SplitUV_SSE2;
  }
#endif
  SIMD_ALIGNED(uint8 rowuv[kMaxStride * 2]);

  for (int y = 0; y < height; ++y) {
    if ((y & 1) == 0) {
      // Copy a row of UV.
      SplitUV(src_uv, rowuv, rowuv + kMaxStride, halfwidth);
      src_uv += src_stride_uv;
    }
    I420ToARGBRow(src_y, rowuv, rowuv + kMaxStride, row, width);
    ARGBToRGB565Row(row, dst_rgb, width);
    dst_rgb += dst_stride_rgb;
    src_y += src_stride_y;
  }
  return 0;
}

// SetRow8 writes 'count' bytes using a 32 bit value repeated
// SetRow32 writes 'count' words using a 32 bit value repeated

#if !defined(YUV_DISABLE_ASM) && defined(__ARM_NEON__)
#define HAS_SETROW_NEON
static void SetRow8_NEON(uint8* dst, uint32 v32, int count) {
  asm volatile (
    "vdup.u32  q0, %2                          \n"  // duplicate 4 ints
    "1:                                        \n"
    "subs      %1, %1, #16                     \n"  // 16 bytes per loop
    "vst1.u32  {q0}, [%0]!                     \n"  // store
    "bgt       1b                              \n"
    : "+r"(dst),   // %0
      "+r"(count)  // %1
    : "r"(v32)     // %2
    : "q0", "memory", "cc");
}

// TODO(fbarchard): Make fully assembler
static void SetRows32_NEON(uint8* dst, uint32 v32, int width,
                           int dst_stride, int height) {
  for (int y = 0; y < height; ++y) {
    SetRow8_NEON(dst, v32, width << 2);
    dst += dst_stride;
  }
}

#elif !defined(YUV_DISABLE_ASM) && defined(_M_IX86)
#define HAS_SETROW_X86
__declspec(naked) __declspec(align(16))
static void SetRow8_X86(uint8* dst, uint32 v32, int count) {
  __asm {
    mov        edx, edi
    mov        edi, [esp + 4]   // dst
    mov        eax, [esp + 8]   // v32
    mov        ecx, [esp + 12]  // count
    shr        ecx, 2
    rep stosd
    mov        edi, edx
    ret
  }
}

__declspec(naked) __declspec(align(16))
static void SetRows32_X86(uint8* dst, uint32 v32, int width,
                         int dst_stride, int height) {
  __asm {
    push       edi
    push       ebp
    mov        edi, [esp + 8 + 4]   // dst
    mov        eax, [esp + 8 + 8]   // v32
    mov        ebp, [esp + 8 + 12]  // width
    mov        edx, [esp + 8 + 16]  // dst_stride
    mov        ebx, [esp + 8 + 20]  // height
    lea        ecx, [ebp * 4]
    sub        edx, ecx             // stride - width * 4

    align      16
  convertloop:
    mov        ecx, ebp
    rep stosd
    add        edi, edx
    sub        ebx, 1
    jg         convertloop

    pop        ebp
    pop        edi
    ret
  }
}

#elif !defined(YUV_DISABLE_ASM) && (defined(__x86_64__) || defined(__i386__))
#define HAS_SETROW_X86
static void SetRow8_X86(uint8* dst, uint32 v32, int width) {
  size_t width_tmp = static_cast<size_t>(width);
  asm volatile (
    "shr       $0x2,%1                         \n"
    "rep stosl                                 \n"
    : "+D"(dst),       // %0
      "+c"(width_tmp)  // %1
    : "a"(v32)         // %2
    : "memory", "cc");
}

static void SetRows32_X86(uint8* dst, uint32 v32, int width,
                         int dst_stride, int height) {
  for (int y = 0; y < height; ++y) {
    size_t width_tmp = static_cast<size_t>(width);
    uint32* d = reinterpret_cast<uint32*>(dst);
    asm volatile (
      "rep stosl                               \n"
      : "+D"(d),         // %0
        "+c"(width_tmp)  // %1
      : "a"(v32)         // %2
      : "memory", "cc");
    dst += dst_stride;
  }
}
#endif

static void SetRow8_C(uint8* dst, uint32 v8, int count) {
#ifdef _MSC_VER
  for (int x = 0; x < count; ++x) {
    dst[x] = v8;
  }
#else
  memset(dst, v8, count);
#endif
}

static void SetRows32_C(uint8* dst, uint32 v32, int width,
                        int dst_stride, int height) {
  for (int y = 0; y < height; ++y) {
    uint32* d = reinterpret_cast<uint32*>(dst);
    for (int x = 0; x < width; ++x) {
      d[x] = v32;
    }
    dst += dst_stride;
  }
}

void SetPlane(uint8* dst_y, int dst_stride_y,
              int width, int height,
              uint32 value) {
  void (*SetRow)(uint8* dst, uint32 value, int pix) = SetRow8_C;
#if defined(HAS_SETROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) &&
      IS_ALIGNED(width, 16) &&
      IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
    SetRow = SetRow8_NEON;
  }
#endif
#if defined(HAS_SETROW_X86)
  if (TestCpuFlag(kCpuHasX86) && IS_ALIGNED(width, 4)) {
    SetRow = SetRow8_X86;
  }
#endif
#if defined(HAS_SETROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(width, 16) &&
      IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
    SetRow = SetRow8_SSE2;
  }
#endif

  uint32 v32 = value | (value << 8) | (value << 16) | (value << 24);
  // Set plane
  for (int y = 0; y < height; ++y) {
    SetRow(dst_y, v32, width);
    dst_y += dst_stride_y;
  }
}

// Draw a rectangle into I420
int I420Rect(uint8* dst_y, int dst_stride_y,
             uint8* dst_u, int dst_stride_u,
             uint8* dst_v, int dst_stride_v,
             int x, int y,
             int width, int height,
             int value_y, int value_u, int value_v) {
  if (!dst_y || !dst_u || !dst_v ||
      width <= 0 || height <= 0 ||
      x < 0 || y < 0 ||
      value_y < 0 || value_y > 255 ||
      value_u < 0 || value_u > 255 ||
      value_v < 0 || value_v > 255) {
    return -1;
  }
  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;
  uint8* start_y = dst_y + y * dst_stride_y + x;
  uint8* start_u = dst_u + (y / 2) * dst_stride_u + (x / 2);
  uint8* start_v = dst_v + (y / 2) * dst_stride_v + (x / 2);

  SetPlane(start_y, dst_stride_y, width, height, value_y);
  SetPlane(start_u, dst_stride_u, halfwidth, halfheight, value_u);
  SetPlane(start_v, dst_stride_v, halfwidth, halfheight, value_v);
  return 0;
}

// Draw a rectangle into ARGB
int ARGBRect(uint8* dst_argb, int dst_stride_argb,
             int dst_x, int dst_y,
             int width, int height,
             uint32 value) {
  if (!dst_argb ||
      width <= 0 || height <= 0 ||
      dst_x < 0 || dst_y < 0) {
    return -1;
  }
  uint8* dst = dst_argb + dst_y * dst_stride_argb + dst_x * 4;
#if defined(HAS_SETROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(width, 16) &&
      IS_ALIGNED(dst, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    SetRows32_NEON(dst, value, width, dst_stride_argb, height);
    return 0;
  }
#endif
#if defined(HAS_SETROW_X86)
  if (TestCpuFlag(kCpuHasX86)) {
    SetRows32_X86(dst, value, width, dst_stride_argb, height);
    return 0;
  }
#endif
  SetRows32_C(dst, value, width, dst_stride_argb, height);
  return 0;
}

// Convert unattentuated ARGB values to preattenuated ARGB.
// An unattenutated ARGB alpha blend uses the formula
// p = a * f + (1 - a) * b
// where
//   p is output pixel
//   f is foreground pixel
//   b is background pixel
//   a is alpha value from foreground pixel
// An preattenutated ARGB alpha blend uses the formula
// p = f + (1 - a) * b
// where
//   f is foreground pixel premultiplied by alpha

int ARGBAttenuate(const uint8* src_argb, int src_stride_argb,
                  uint8* dst_argb, int dst_stride_argb,
                  int width, int height) {
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBAttenuateRow)(const uint8* src_argb, uint8* dst_argb,
                           int width) = ARGBAttenuateRow_C;
#if defined(HAS_ARGBATTENUATE_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBAttenuateRow = ARGBAttenuateRow_SSE2;
  }
#endif
#if defined(HAS_ARGBATTENUATE_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBAttenuateRow = ARGBAttenuateRow_SSSE3;
  }
#endif

  for (int y = 0; y < height; ++y) {
    ARGBAttenuateRow(src_argb, dst_argb, width);
    src_argb += src_stride_argb;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

// Convert unattentuated ARGB values to preattenuated ARGB.
int ARGBUnattenuate(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_argb, int dst_stride_argb,
                    int width, int height) {
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBUnattenuateRow)(const uint8* src_argb, uint8* dst_argb,
                             int width) = ARGBUnattenuateRow_C;
#if defined(HAS_ARGBUNATTENUATE_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 4) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
    ARGBUnattenuateRow = ARGBUnattenuateRow_SSE2;
  }
#endif

  for (int y = 0; y < height; ++y) {
    ARGBUnattenuateRow(src_argb, dst_argb, width);
    src_argb += src_stride_argb;
    dst_argb += dst_stride_argb;
  }
  return 0;
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

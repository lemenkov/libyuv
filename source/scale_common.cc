/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/scale.h"

#include <assert.h>
#include <string.h>

#include "libyuv/cpu_id.h"
#include "libyuv/planar_functions.h"  // For CopyARGB
#include "libyuv/row.h"
#include "../source/scale_row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Scale ARGB vertically with bilinear interpolation.
void ScaleARGBBilinearVertical(int src_height,
                               int dst_width, int dst_height,
                               int src_stride, int dst_stride,
                               const uint8* src_argb, uint8* dst_argb,
                               int x, int y, int dy,
                               int bpp, FilterMode filtering) {
  int dst_widthx4 = dst_width * bpp;
  src_argb += (x >> 16) * bpp;
  assert(src_height > 0);
  assert(dst_width > 0);
  assert(dst_height > 0);
  void (*InterpolateRow)(uint8* dst_argb, const uint8* src_argb,
      ptrdiff_t src_stride, int dst_width, int source_y_fraction) =
      InterpolateRow_C;
#if defined(HAS_INTERPOLATEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && dst_widthx4 >= 16) {
    InterpolateRow = InterpolateRow_Any_SSE2;
    if (IS_ALIGNED(dst_widthx4, 16)) {
      InterpolateRow = InterpolateRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride, 16) &&
          IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride, 16)) {
        InterpolateRow = InterpolateRow_SSE2;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && dst_widthx4 >= 16) {
    InterpolateRow = InterpolateRow_Any_SSSE3;
    if (IS_ALIGNED(dst_widthx4, 16)) {
      InterpolateRow = InterpolateRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride, 16) &&
          IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride, 16)) {
        InterpolateRow = InterpolateRow_SSSE3;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && dst_widthx4 >= 16) {
    InterpolateRow = InterpolateRow_Any_NEON;
    if (IS_ALIGNED(dst_widthx4, 16)) {
      InterpolateRow = InterpolateRow_NEON;
    }
  }
#endif
#if defined(HAS_INTERPOLATEROWS_MIPS_DSPR2)
  if (TestCpuFlag(kCpuHasMIPS_DSPR2) && dst_widthx4 >= 4 &&
      IS_ALIGNED(src_argb, 4) && IS_ALIGNED(src_stride, 4) &&
      IS_ALIGNED(dst_argb, 4) && IS_ALIGNED(dst_stride, 4)) {
    InterpolateRow = InterpolateRow_Any_MIPS_DSPR2;
    if (IS_ALIGNED(dst_widthx4, 4)) {
      InterpolateRow = InterpolateRow_MIPS_DSPR2;
    }
  }
#endif
  const int max_y = (src_height > 1) ? ((src_height - 1) << 16) - 1 : 0;
  for (int j = 0; j < dst_height; ++j) {
    if (y > max_y) {
      y = max_y;
    }
    int yi = y >> 16;
    int yf = filtering ? ((y >> 8) & 255) : 0;
    const uint8* src = src_argb + yi * src_stride;
    InterpolateRow(dst_argb, src, src_stride, dst_widthx4, yf);
    dst_argb += dst_stride;
    y += dy;
  }
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

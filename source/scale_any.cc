/*
 *  Copyright 2015 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/scale.h"
#include "libyuv/scale_row.h"

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Definition for ScaleFilterCols, ScaleARGBCols and ScaleARGBFilterCols
#define CANY(NAMEANY, TERP_SIMD, TERP_C, BPP, MASK)                            \
    void NAMEANY(uint8* dst_ptr, const uint8* src_ptr,                         \
                 int dst_width, int x, int dx) {                               \
      int n = dst_width & ~MASK;                                               \
      if (n > 0) {                                                             \
        TERP_SIMD(dst_ptr, src_ptr, n, x, dx);                                 \
      }                                                                        \
      TERP_C(dst_ptr + n * BPP, src_ptr,                                       \
             dst_width & MASK, x + n * dx, dx);                                \
    }

#ifdef HAS_SCALEFILTERCOLS_NEON
CANY(ScaleFilterCols_Any_NEON, ScaleFilterCols_NEON, ScaleFilterCols_C, 1, 7)
#endif
#undef CANY

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif


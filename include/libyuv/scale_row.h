/*
 *  Copyright 2013 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef INCLUDE_LIBYUV_ROW_H_  // NOLINT
#define INCLUDE_LIBYUV_ROW_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Scale ARGB vertically with bilinear interpolation.
void ScalePlaneVertical(int src_height,
                        int dst_width, int dst_height,
                        int src_stride, int dst_stride,
                        const uint8* src_argb, uint8* dst_argb,
                        int x, int y, int dy,
                        int bpp, FilterMode filtering);

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // INCLUDE_LIBYUV_ROW_H_  NOLINT

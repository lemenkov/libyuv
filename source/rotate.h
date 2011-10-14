/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef LIBYUV_SOURCE_ROTATE_H_
#define LIBYUV_SOURCE_ROTATE_H_

#include "libyuv/basic_types.h"

namespace libyuv {

void Rotate90(const uint8* src, int src_stride,
              uint8* dst, int dst_stride,
              int width, int height);
void Rotate180(const uint8* src, int src_stride,
               uint8* dst, int dst_stride,
               int width, int height);
void Rotate270(const uint8* src, int src_stride,
               uint8* dst, int dst_stride,
               int width, int height);

void Rotate90_deinterleave(const uint8* src, int src_stride,
                           uint8* dst_a, int dst_stride_a,
                           uint8* dst_b, int dst_stride_b,
                           int width, int height);
void Rotate180_deinterleave(const uint8* src, int src_stride,
                            uint8* dst_a, int dst_stride_a,
                            uint8* dst_b, int dst_stride_b,
                            int width, int height);
void Rotate270_deinterleave(const uint8* src, int src_stride,
                            uint8* dst_a, int dst_stride_a,
                            uint8* dst_b, int dst_stride_b,
                            int width, int height);

void Transpose(const uint8* src, int src_stride,
               uint8* dst, int dst_stride,
               int width, int height);
}  // namespace libyuv

#endif  // LIBYUV_SOURCE_ROTATE_H_

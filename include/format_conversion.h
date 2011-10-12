/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef LIBYUV_INCLUDE_FORMATCONVERSION_H_
#define LIBYUV_INCLUDE_FORMATCONVERSION_H_

#include "basic_types.h"

namespace libyuv {

// Converts any Bayer RGB format to I420.
void BayerRGBToI420(const uint8* src_bayer, int src_pitch_bayer,
                    uint32 src_fourcc_bayer,
                    uint8* dst_y, int dst_pitch_y,
                    uint8* dst_u, int dst_pitch_u,
                    uint8* dst_v, int dst_pitch_v,
                    int width, int height);

// Converts any Bayer RGB format to ARGB.
void BayerRGBToARGB(const uint8* src_bayer, int src_pitch_bayer,
                    uint32 src_fourcc_bayer,
                    uint8* dst_rgb, int dst_pitch_rgb,
                    int width, int height);

// Converts ARGB to any Bayer RGB format.
void ARGBToBayerRGB(const uint8* src_rgb, int src_pitch_rgb,
                    uint8* dst_bayer, int dst_pitch_bayer,
                    uint32 dst_fourcc_bayer,
                    int width, int height);

}  // namespace libyuv

#endif  // LIBYUV_INCLUDE_FORMATCONVERSION_H_

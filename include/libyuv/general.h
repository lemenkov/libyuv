/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*
 * General operations on YUV images.
 */

#ifndef INCLUDE_LIBYUV_GENERAL_H_
#define INCLUDE_LIBYUV_GENERAL_H_

#include "libyuv/basic_types.h"

namespace libyuv {

// I420 mirror
int
I420Mirror(const uint8* src_yplane, int src_ystride,
           const uint8* src_uplane, int src_ustride,
           const uint8* src_vplane, int src_vstride,
           uint8* dst_yplane, int dst_ystride,
           uint8* dst_uplane, int dst_ustride,
           uint8* dst_vplane, int dst_vstride,
           int width, int height);

// Crop/Pad I420 frame to match required dimensions.
int
I420CropPad(const uint8* src_frame, int src_width,
           int src_height, uint8* dst_frame,
           int dst_width, int dst_height);

// I420 Crop - crop a rectangle from image
int
I420Crop(uint8* frame,
         int src_width, int src_height,
         int dst_width, int dst_height);

} // namespace libyuv

#endif // INCLUDE_LIBYUV_GENERAL_H_

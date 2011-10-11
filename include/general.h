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

#ifndef LIBYUV_INCLUDE_GENERAL_H_
#define LIBYUV_INCLUDE_GENERAL_H_

#include "basic_types.h"

namespace libyuv {

// Supported rotation
enum VideoRotationMode
{
  kRotateNone = 0,
  kRotateClockwise = 90,
  kRotateCounterClockwise = -90,
  kRotate180 = 180,
};

// I420  mirror
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

// I420 Crop - make a center cut
int
I420Cut(uint8* frame,
        int src_width, int src_height,
        int dst_width, int dst_height);


} // namespace libyuv


#endif // LIBYUV_INCLUDE_GENERAL_H_

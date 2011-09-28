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
    kRotateAntiClockwise = -90,
    kRotate180 = 180,
};

// Mirror functions
// The following 2 functions perform mirroring on an image (LeftRight/UpDown)
// Input:
//    - width       : Image width in pixels.
//    - height      : Image height in pixels.
//    - inFrame     : Reference to input image.
//    - outFrame    : Reference to converted image.
// Return value: 0 if OK, < 0 otherwise.
int
MirrorI420LeftRight(const uint8* src_frame, int src_stride,
                     uint8* dst_frame, int dst_stride,
                     int src_width, int src_height);

// Cut/Pad I420 frame to match desird dimensions.
int
CutPadI420Frame(const uint8* inFrame, int inWidth,
                  int inHeight, uint8* outFrame,
                  int outWidth, int outHeight);

// I420 Cut - make a center cut
int
CutI420Frame(uint8* frame, int fromWidth,
             int fromHeight, int toWidth,
             int toHeight);


} // namespace libyuv


#endif // LIBYUV_INCLUDE_GENERAL_H_

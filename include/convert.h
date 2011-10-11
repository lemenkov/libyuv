/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef LIBYUV_INCLUDE_CONVERT_H_
#define LIBYUV_INCLUDE_CONVERT_H_

#include "basic_types.h"

namespace libyuv
{
class Convert {
 public:
  static int
  I420ToRGB24(const uint8* src_yplane, int src_ystride,
              const uint8* src_uplane, int src_ustride,
              const uint8* src_vplane, int src_vstride,
              uint8* dst_frame, int dst_stride,
              int src_width, int src_height);

  static int
  I420ToARGB(const uint8* src_yplane, int src_ystride,
             const uint8* src_uplane, int src_ustride,
             const uint8* src_vplane, int src_vstride,
             uint8* dst_frame, int dst_stride,
             int src_width, int src_height);

  static int
  I420ToARGB4444(const uint8* src_yplane, int src_ystride,
                 const uint8* src_uplane, int src_ustride,
                 const uint8* src_vplane, int src_vstride,
                 uint8* dst_frame, int dst_stride,
                 int src_width, int src_height);

  static int
  I420ToRGB565(const uint8* src_yplane, int src_ystride,
               const uint8* src_uplane, int src_ustride,
               const uint8* src_vplane, int src_vstride,
               uint8* dst_frame, int dst_stride,
               int src_width, int src_height);

  static int
  I420ToARGB1555(const uint8* src_yplane, int src_ystride,
                 const uint8* src_uplane, int src_ustride,
                 const uint8* src_vplane, int src_vstride,
                 uint8* dst_frame, int dst_stride,
                 int src_width, int src_height);

   static int
  I420ToYUY2(const uint8* src_yplane, int src_ystride,
             const uint8* src_uplane, int src_ustride,
             const uint8* src_vplane, int src_vstride,
             uint8* dst_frame, int dst_stride,
             int src_width, int src_height);
  static int
  I420ToUYVY(const uint8* src_yplane, int src_ystride,
             const uint8* src_uplane, int src_ustride,
             const uint8* src_vplane, int src_vstride,
             uint8* dst_frame, int dst_stride,
             int src_width, int src_height);
  static int
  UYVYToI420(const uint8* src_frame, int src_stride,
             uint8* dst_yplane, int dst_ystride,
             uint8* dst_uplane, int dst_ustride,
             uint8* dst_vplane, int dst_vstride,
             int src_width, int src_height);

  static int
  RGB24ToARGB(const uint8* src_frame, int src_stride,
              uint8* dst_frame, int dst_stride,
              int src_width, int src_height);

  static int
  RGB24ToI420(const uint8* src_frame, int src_stride,
              uint8* dst_yplane, int dst_ystride,
              uint8* dst_uplane, int dst_ustride,
              uint8* dst_vplane, int dst_vstride,
              int src_width, int src_height);

  static int
  RAWToI420(const uint8* src_frame, int src_stride,
            uint8* dst_yplane, int dst_ystride,
            uint8* dst_uplane, int dst_ustride,
            uint8* dst_vplane, int dst_vstride,
            int src_width, int src_height);

  static int
  ABGRToI420(const uint8* src_frame, int src_stride,
             uint8* dst_yplane, int dst_ystride,
             uint8* dst_uplane, int dst_ustride,
             uint8* dst_vplane, int dst_vstride,
             int src_width, int src_height);
  static int
  I420ToABGR(const uint8* src_yplane, int src_ystride,
             const uint8* src_uplane, int src_ustride,
             const uint8* src_vplane, int src_vstride,
             uint8* dst_frame, int dst_stride,
             int src_width, int src_height);
  int
  I420ToBGRA(const uint8* src_yplane, int src_ystride,
             const uint8* src_uplane, int src_ustride,
             const uint8* src_vplane, int src_vstride,
             uint8* dst_frame, int dst_stride,
             int src_width, int src_height);

  static int
  NV12ToRGB565(const uint8* src_yplane, int src_ystride,
               const  uint8* src_uvplane, int src_uvstride,
               uint8* dst_frame, int dst_stride,
               int src_width, int src_height);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Convert);

};
} //  namespace libyuv

#endif // LIBYUV_INCLUDE_CONVERT_H_

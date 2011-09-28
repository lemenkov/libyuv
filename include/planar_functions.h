/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef LIBYUV_INCLUDE_PLANAR_FUNCTIONS_H_
#define LIBYUV_INCLUDE_PLANAR_FUNCTIONS_H_

#include "basic_types.h"

namespace libyuv {

class PlanarFunctions {
 public:

  // Copy I420 to I420.
  static void I420Copy(const uint8* src_y, int src_pitch_y,
                       const uint8* src_u, int src_pitch_u,
                       const uint8* src_v, int src_pitch_v,
                       uint8* dst_y, int dst_pitch_y,
                       uint8* dst_u, int dst_pitch_u,
                       uint8* dst_v, int dst_pitch_v,
                       int width, int height);

  // Convert I422 to I420.  Used by MJPG.
  static void I422ToI420(const uint8* src_y, int src_pitch_y,
                         const uint8* src_u, int src_pitch_u,
                         const uint8* src_v, int src_pitch_v,
                         uint8* dst_y, int dst_pitch_y,
                         uint8* dst_u, int dst_pitch_u,
                         uint8* dst_v, int dst_pitch_v,
                         int width, int height);

  // Convert M420 to I420.
  static void M420ToI420(uint8* dst_y, int dst_pitch_y,
                         uint8* dst_u, int dst_pitch_u,
                         uint8* dst_v, int dst_pitch_v,
                         const uint8* m420, int pitch_m420,
                         int width, int height);

  // Convert NV12 to I420.  Also used for NV21.
  static void NV12ToI420(uint8* dst_y, int dst_pitch_y,
                         uint8* dst_u, int dst_pitch_u,
                         uint8* dst_v, int dst_pitch_v,
                         const uint8* src_y,
                         const uint8* src_uv,
                         int src_pitch,
                         int width, int height);

  DISALLOW_IMPLICIT_CONSTRUCTORS(PlanarFunctions);
};

}  // namespace libyuv

#endif  // LIBYUV_INCLUDE_PLANAR_FUNCTIONS_H_

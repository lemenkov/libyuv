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
    static void M420ToI420(const uint8* src_m420, int src_pitch_m420,
                           uint8* dst_y, int dst_pitch_y,
                           uint8* dst_u, int dst_pitch_u,
                           uint8* dst_v, int dst_pitch_v,
                           int width, int height);

    // Convert Q420 to I420.
    static void Q420ToI420(const uint8* src_y, int src_pitch_y,
                           const uint8* src_yuy2, int src_pitch_yuy2,
                           uint8* dst_y, int dst_pitch_y,
                           uint8* dst_u, int dst_pitch_u,
                           uint8* dst_v, int dst_pitch_v,
                           int width, int height);

    // Convert NV12 to I420.  Also used for NV21.
    static void NV12ToI420(const uint8* src_y,
                           const uint8* src_uv, int src_pitch,
                           uint8* dst_y, int dst_pitch_y,
                           uint8* dst_u, int dst_pitch_u,
                           uint8* dst_v, int dst_pitch_v,
                           int width, int height);

    // Convert YUY2 to I420.
    static void YUY2ToI420(const uint8* src_yuy2, int src_pitch_yuy2,
                           uint8* dst_y, int dst_pitch_y,
                           uint8* dst_u, int dst_pitch_u,
                           uint8* dst_v, int dst_pitch_v,
                           int width, int height);

    // Convert UYVY to I420.
    static void UYVYToI420(const uint8* src_uyvy, int src_pitch_uyvy,
                           uint8* dst_y, int dst_pitch_y,
                           uint8* dst_u, int dst_pitch_u,
                           uint8* dst_v, int dst_pitch_v,
                           int width, int height);

    // Convert I420 to ARGB.
    static void I420ToARGB(const uint8* src_y, int src_pitch_y,
                           const uint8* src_u, int src_pitch_u,
                           const uint8* src_v, int src_pitch_v,
                           uint8* dst_argb, int dst_pitch_argb,
                           int width, int height);

    // Convert I422 to ARGB.
    static void I422ToARGB(const uint8* src_y, int src_pitch_y,
                           const uint8* src_u, int src_pitch_u,
                           const uint8* src_v, int src_pitch_v,
                           uint8* dst_argb, int dst_pitch_argb,
                           int width, int height);

    // Convert I444 to ARGB.
    static void I444ToARGB(const uint8* src_y, int src_pitch_y,
                           const uint8* src_u, int src_pitch_u,
                           const uint8* src_v, int src_pitch_v,
                           uint8* dst_argb, int dst_pitch_argb,
                           int width, int height);

    // Convert I400 to ARGB.
    static void I400ToARGB(const uint8* src_y, int src_pitch_y,
                           uint8* dst_argb, int dst_pitch_argb,
                           int width, int height);

    // Convert I400 to ARGB.
    static void I400ToARGB_Reference(const uint8* src_y, int src_pitch_y,
                                     uint8* dst_argb, int dst_pitch_argb,
                                     int width, int height);

    // Convert RAW to ARGB.
    static void RAWToARGB(const uint8* src_raw, int src_pitch_raw,
                          uint8* dst_argb, int dst_pitch_argb,
                          int width, int height);

    // Convert BG24 to ARGB.
    static void BG24ToARGB(const uint8* src_bg24, int src_pitch_bg24,
                           uint8* dst_argb, int dst_pitch_argb,
                           int width, int height);

    // Convert ABGR to ARGB.
    static void ABGRToARGB(const uint8* src_abgr, int src_pitch_abgr,
                           uint8* dst_argb, int dst_pitch_argb,
                           int width, int height);

    DISALLOW_IMPLICIT_CONSTRUCTORS(PlanarFunctions);
  };

}  // namespace libyuv

#endif  // LIBYUV_INCLUDE_PLANAR_FUNCTIONS_H_

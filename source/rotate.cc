/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rotate.h"

namespace libyuv {

typedef void (*reverse_func)(const uint8*, uint8*, int);
typedef void (*rotate_wx8func)(const uint8*, int, uint8*, int, int);
typedef void (*rotate_wxhfunc)(const uint8*, int, uint8*, int, int, int);

#ifdef __ARM_NEON__
extern "C" {
void ReverseLine_NEON(const uint8* src, uint8* dst, int width);
void Transpose_wx8_NEON(const uint8* src, int src_pitch,
                        uint8* dst, int dst_pitch, int width);
}  // extern "C"
#endif

static void Transpose_wx8_C(const uint8* src, int src_pitch,
                            uint8* dst, int dst_pitch,
                            int w) {
  int i, j;
  for (i = 0; i < w; ++i)
    for (j = 0; j < 8; ++j)
      dst[i * dst_pitch + j] = src[j * src_pitch + i];
}

static void Transpose_wxh_C(const uint8* src, int src_pitch,
                            uint8* dst, int dst_pitch,
                            int width, int height) {
  int i, j;
  for (i = 0; i < width; ++i)
    for (j = 0; j < height; ++j)
      dst[i * dst_pitch + j] = src[j * src_pitch + i];
}

void Transpose(const uint8* src, int src_pitch,
               uint8* dst, int dst_pitch,
               int width, int height) {
  int i = height;
  rotate_wx8func Transpose_wx8;
  rotate_wxhfunc Transpose_wxh;

  // do processor detection here.
#ifdef __ARM_NEON__
  Transpose_wx8 = Transpose_wx8_NEON;
  Transpose_wxh = Transpose_wxh_C;
#else
  Transpose_wx8 = Transpose_wx8_C;
  Transpose_wxh = Transpose_wxh_C;
#endif

  // work across the source in 8x8 tiles
  do {
    Transpose_wx8(src, src_pitch, dst, dst_pitch, width);

    src += 8 * src_pitch;
    dst += 8;
    i   -= 8;
  } while (i >= 8);

// TODO(frkoenig): Have wx4 and maybe wx2
  Transpose_wxh(src, src_pitch, dst, dst_pitch, width, i);
}

void Rotate90(const uint8* src, int src_pitch,
              uint8* dst, int dst_pitch,
              int width, int height) {
  src += src_pitch*(height-1);
  src_pitch = -src_pitch;

  Transpose(src, src_pitch, dst, dst_pitch, width, height);
}

void Rotate270(const uint8* src, int src_pitch,
               uint8* dst, int dst_pitch,
               int width, int height) {
  dst += dst_pitch*(width-1);
  dst_pitch = -dst_pitch;

  Transpose(src, src_pitch, dst, dst_pitch, width, height);
}

void ReverseLine_C(const uint8* src, uint8* dst, int width) {
  int i;
  for (i = 0; i < width; ++i)
    dst[width-1 - i] = src[i];
}

void Rotate180(const uint8* src, int src_pitch,
               uint8* dst, int dst_pitch,
               int width, int height) {
  int i;
  reverse_func ReverseLine;

  // do processor detection here.
#ifdef __ARM_NEON__
  ReverseLine = ReverseLine_NEON;
#else
  ReverseLine = ReverseLine_C;
#endif

  dst += dst_pitch*(height-1);

  for (i = 0; i < height; ++i) {
    ReverseLine(src, dst, width);

    src += src_pitch;
    dst -= dst_pitch;
  }
}

}  // namespace libyuv

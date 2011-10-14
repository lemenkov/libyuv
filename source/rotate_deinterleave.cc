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

typedef void (*reverse_func)(const uint8*, uint8*, uint8*, int);
typedef void (*rotate_wx8func)(const uint8*, int,
                               uint8*, int,
                               uint8*, int, int);
typedef void (*rotate_wxhfunc)(const uint8*, int,
                               uint8*, int,
                               uint8*, int, int, int);

#ifdef __ARM_NEON__
extern "C" {
void RestoreRegisters_NEON(unsigned long long *restore);
void ReverseLine_di_NEON(const uint8* src,
                         uint8* dst_a, uint8* dst_b,
                         int width);
void SaveRegisters_NEON(unsigned long long *store);
void Transpose_di_wx8_NEON(const uint8* src, int src_stride,
                           uint8* dst_a, int dst_stride_a,
                           uint8* dst_b, int dst_stride_b,
                           int width);
}  // extern "C"
#endif

static void Transpose_di_wx8_C(const uint8* src, int src_stride,
                               uint8* dst_a, int dst_stride_a,
                               uint8* dst_b, int dst_stride_b,
                               int w) {
  int i, j;
  for (i = 0; i < w*2; i += 2)
    for (j = 0; j < 8; ++j) {
      dst_a[j + (i>>1)*dst_stride_a] = src[i + j*src_stride];
      dst_b[j + (i>>1)*dst_stride_b] = src[i + j*src_stride + 1];
    }
}

static void Transpose_di_wxh_C(const uint8* src, int src_stride,
                               uint8* dst_a, int dst_stride_a,
                               uint8* dst_b, int dst_stride_b,
                               int w, int h) {
  int i, j;
  for (i = 0; i < w*2; i += 2)
    for (j = 0; j < h; ++j) {
      dst_a[j + (i>>1)*dst_stride_a] = src[i + j*src_stride];
      dst_b[j + (i>>1)*dst_stride_b] = src[i + j*src_stride + 1];
    }
}

void Transpose_deinterleave(const uint8* src, int src_stride,
                            uint8* dst_a, int dst_stride_a,
                            uint8* dst_b, int dst_stride_b,
                            int width, int height) {
  int i = height;
  rotate_wx8func Transpose_wx8;
  rotate_wxhfunc Transpose_wxh;

  // do processor detection here.
#ifdef __ARM_NEON__
  unsigned long long store_reg[8];
  SaveRegisters_NEON(store_reg);
  Transpose_wx8 = Transpose_di_wx8_NEON;
  Transpose_wxh = Transpose_di_wxh_C;
#else
  Transpose_wx8 = Transpose_di_wx8_C;
  Transpose_wxh = Transpose_di_wxh_C;
#endif

  width >>= 1;

  // work across the source in 8x8 tiles
  do {
    Transpose_wx8(src, src_stride,
                  dst_a, dst_stride_a,
                  dst_b, dst_stride_b,
                  width);

    src   += 8 * src_stride;
    dst_a += 8;
    dst_b += 8;
    i     -= 8;
  } while (i >= 8);

  Transpose_wxh(src, src_stride,
                dst_a, dst_stride_a,
                dst_b, dst_stride_b,
                width, i);

#ifdef __ARM_NEON__
  RestoreRegisters_NEON(store_reg);
#endif
}

void Rotate90_deinterleave(const uint8* src, int src_stride,
                           uint8* dst_a, int dst_stride_a,
                           uint8* dst_b, int dst_stride_b,
                            int width, int height) {
  src += src_stride*(height-1);
  src_stride = -src_stride;

  Transpose_deinterleave(src, src_stride,
                         dst_a, dst_stride_a,
                         dst_b, dst_stride_b,
                         width, height);
}

void Rotate270_deinterleave(const uint8* src, int src_stride,
                            uint8* dst_a, int dst_stride_a,
                            uint8* dst_b, int dst_stride_b,
                            int width, int height) {
  dst_a += dst_stride_a*((width>>1)-1);
  dst_b += dst_stride_b*((width>>1)-1);
  dst_stride_a = -dst_stride_a;
  dst_stride_b = -dst_stride_b;

  Transpose_deinterleave(src, src_stride,
                         dst_a, dst_stride_a,
                         dst_b, dst_stride_b,
                         width, height);
}

static void ReverseLine_di_C(const uint8* src,
                             uint8* dst_a, uint8* dst_b,
                             int width) {
  int i;
  for (i = 0; i < width*2; i += 2) {
    dst_a[width-1 - (i>>1)] = src[i];
    dst_b[width-1 - (i>>1)] = src[i+1];
  }
}

void Rotate180_deinterleave(const uint8* src, int src_stride,
                            uint8* dst_a, int dst_stride_a,
                            uint8* dst_b, int dst_stride_b,
                            int width, int height) {
  int i;
  reverse_func ReverseLine;

  // do processor detection here.
#ifdef __ARM_NEON__
  ReverseLine = ReverseLine_di_NEON;
#else
  ReverseLine = ReverseLine_di_C;
#endif

  dst_a += dst_stride_a*(height-1);
  dst_b += dst_stride_b*(height-1);

  width >>= 1;

  for (i = 0; i < height; ++i) {
    ReverseLine(src, dst_a, dst_b, width);

    src   += src_stride;
    dst_a -= dst_stride_a;
    dst_b -= dst_stride_b;
  }
}

}  // namespace libyuv

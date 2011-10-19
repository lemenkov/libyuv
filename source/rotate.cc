/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"
#include "rotate_priv.h"

namespace libyuv {

typedef void (*reverse_uv_func)(const uint8*, uint8*, uint8*, int);
typedef void (*reverse_func)(const uint8*, uint8*, int);
typedef void (*rotate_uv_wx8_func)(const uint8*, int,
                                   uint8*, int,
                                   uint8*, int, int);
typedef void (*rotate_uv_wxh_func)(const uint8*, int,
                                   uint8*, int,
                                   uint8*, int, int, int);
typedef void (*rotate_wx8_func)(const uint8*, int, uint8*, int, int);
typedef void (*rotate_wxh_func)(const uint8*, int, uint8*, int, int, int);

#ifdef __ARM_NEON__
extern "C" {
void RestoreRegisters_NEON(unsigned long long *restore);
void SaveRegisters_NEON(unsigned long long *store);
void ReverseLine_NEON(const uint8* src, uint8* dst, int width);
void ReverseLineUV_NEON(const uint8* src,
                        uint8* dst_a, uint8* dst_b,
                        int width);
void TransposeWx8_NEON(const uint8* src, int src_stride,
                       uint8* dst, int dst_stride, int width);
void TransposeUVWx8_NEON(const uint8* src, int src_stride,
                         uint8* dst_a, int dst_stride_a,
                         uint8* dst_b, int dst_stride_b,
                         int width);
}  // extern "C"
#endif

static void TransposeWx8_C(const uint8* src, int src_stride,
                           uint8* dst, int dst_stride,
                           int w) {
  int i, j;
  for (i = 0; i < w; ++i)
    for (j = 0; j < 8; ++j)
      dst[i * dst_stride + j] = src[j * src_stride + i];
}

static void TransposeWxH_C(const uint8* src, int src_stride,
                           uint8* dst, int dst_stride,
                           int width, int height) {
  int i, j;
  for (i = 0; i < width; ++i)
    for (j = 0; j < height; ++j)
      dst[i * dst_stride + j] = src[j * src_stride + i];
}

void TransposePlane(const uint8* src, int src_stride,
                    uint8* dst, int dst_stride,
                    int width, int height) {
  int i = height;
  rotate_wx8_func TransposeWx8;
  rotate_wxh_func TransposeWxH;

  // do processor detection here.
#ifdef __ARM_NEON__
  TransposeWx8 = TransposeWx8_NEON;
  TransposeWxH = TransposeWxH_C;
#else
  TransposeWx8 = TransposeWx8_C;
  TransposeWxH = TransposeWxH_C;
#endif

  // work across the source in 8x8 tiles
  while (i >= 8) {
    TransposeWx8(src, src_stride, dst, dst_stride, width);

    src += 8 * src_stride;    // go down 8 rows
    dst += 8;                 // move over 8 columns
    i   -= 8;
  }

  TransposeWxH(src, src_stride, dst, dst_stride, width, i);
}

void RotatePlane90(const uint8* src, int src_stride,
                   uint8* dst, int dst_stride,
                   int width, int height) {
  // Rotate by 90 is a transpose with the source read
  // from bottom to top.  So set the source pointer to the end
  // of the buffer and flip the sign of the source stride.
  src += src_stride * (height - 1);
  src_stride = -src_stride;

  TransposePlane(src, src_stride, dst, dst_stride, width, height);
}

void RotatePlane270(const uint8* src, int src_stride,
                    uint8* dst, int dst_stride,
                    int width, int height) {
  // Rotate by 270 is a transpose with the destination written
  // from bottom to top.  So set the destination pointer to the end
  // of the buffer and flip the sign of the destination stride.
  dst += dst_stride * (width - 1);
  dst_stride = -dst_stride;

  TransposePlane(src, src_stride, dst, dst_stride, width, height);
}

void ReverseLine_C(const uint8* src, uint8* dst, int width) {
  int i;
  src += width;
  for (i = 0; i < width; ++i) {
    --src;
    dst[i] = src[0];
  }
}

void RotatePlane180(const uint8* src, int src_stride,
                    uint8* dst, int dst_stride,
                    int width, int height) {
  int i;
  reverse_func ReverseLine;

  // TODO(frkoenig): do processor detection here.
#ifdef __ARM_NEON__
  ReverseLine = ReverseLine_NEON;
#else
  ReverseLine = ReverseLine_C;
#endif

  // Rotate by 180 is a mirror with the destination
  // written in reverse.
  dst += dst_stride * (height - 1);

  for (i = 0; i < height; ++i) {
    ReverseLine(src, dst, width);

    src += src_stride;
    dst -= dst_stride;
  }
}

static void TransposeUVWx8_C(const uint8* src, int src_stride,
                             uint8* dst_a, int dst_stride_a,
                             uint8* dst_b, int dst_stride_b,
                             int w) {
  int i, j;
  for (i = 0; i < w * 2; i += 2)
    for (j = 0; j < 8; ++j) {
      dst_a[j + ((i >> 1) * dst_stride_a)] = src[i + (j * src_stride)];
      dst_b[j + ((i >> 1) * dst_stride_b)] = src[i + (j * src_stride) + 1];
    }
}

static void TransposeUVWxH_C(const uint8* src, int src_stride,
                             uint8* dst_a, int dst_stride_a,
                             uint8* dst_b, int dst_stride_b,
                             int w, int h) {
  int i, j;
  for (i = 0; i < w*2; i += 2)
    for (j = 0; j < h; ++j) {
      dst_a[j + ((i >> 1) * dst_stride_a)] = src[i + (j * src_stride)];
      dst_b[j + ((i >> 1) * dst_stride_b)] = src[i + (j * src_stride) + 1];
    }
}

void TransposeUV(const uint8* src, int src_stride,
                 uint8* dst_a, int dst_stride_a,
                 uint8* dst_b, int dst_stride_b,
                 int width, int height) {
  int i = height;
  rotate_uv_wx8_func TransposeWx8;
  rotate_uv_wxh_func TransposeWxH;

  // do processor detection here.
#ifdef __ARM_NEON__
  unsigned long long store_reg[8];
  SaveRegisters_NEON(store_reg);
  TransposeWx8 = TransposeUVWx8_NEON;
  TransposeWxH = TransposeUVWxH_C;
#else
  TransposeWx8 = TransposeUVWx8_C;
  TransposeWxH = TransposeUVWxH_C;
#endif

  // work through the source in 8x8 tiles
  while (i >= 8) {
    TransposeWx8(src, src_stride,
                 dst_a, dst_stride_a,
                 dst_b, dst_stride_b,
                 width);

    src   += 8 * src_stride;    // go down 8 rows
    dst_a += 8;                 // move over 8 columns
    dst_b += 8;                 // move over 8 columns
    i     -= 8;
  }

  TransposeWxH(src, src_stride,
               dst_a, dst_stride_a,
               dst_b, dst_stride_b,
               width, i);

#ifdef __ARM_NEON__
  RestoreRegisters_NEON(store_reg);
#endif
}

void RotateUV90(const uint8* src, int src_stride,
                uint8* dst_a, int dst_stride_a,
                uint8* dst_b, int dst_stride_b,
                int width, int height) {
  src += src_stride * (height - 1);
  src_stride = -src_stride;

  TransposeUV(src, src_stride,
              dst_a, dst_stride_a,
              dst_b, dst_stride_b,
              width, height);
}

void RotateUV270(const uint8* src, int src_stride,
                 uint8* dst_a, int dst_stride_a,
                 uint8* dst_b, int dst_stride_b,
                 int width, int height) {
  dst_a += dst_stride_a * (width - 1);
  dst_b += dst_stride_b * (width - 1);
  dst_stride_a = -dst_stride_a;
  dst_stride_b = -dst_stride_b;

  TransposeUV(src, src_stride,
              dst_a, dst_stride_a,
              dst_b, dst_stride_b,
              width, height);
}

static void ReverseLineUV_C(const uint8* src,
                            uint8* dst_a, uint8* dst_b,
                            int width) {
  int i;
  src += width << 1;
  for (i = 0; i < width; ++i) {
    src -= 2;
    dst_a[i] = src[0];
    dst_b[i] = src[1];
  }
}

void RotateUV180(const uint8* src, int src_stride,
                 uint8* dst_a, int dst_stride_a,
                 uint8* dst_b, int dst_stride_b,
                 int width, int height) {
  int i;
  reverse_uv_func ReverseLine;

  // TODO(frkoenig) : do processor detection here.
#ifdef __ARM_NEON__
  ReverseLine = ReverseLineUV_NEON;
#else
  ReverseLine = ReverseLineUV_C;
#endif

  dst_a += dst_stride_a * (height - 1);
  dst_b += dst_stride_b * (height - 1);

  for (i = 0; i < height; ++i) {
    ReverseLine(src, dst_a, dst_b, width);

    src   += src_stride;      // down one line at a time
    dst_a -= dst_stride_a;    // nominally up one line at a time
    dst_b -= dst_stride_b;    // nominally up one line at a time
  }
}

int I420Rotate(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height,
               RotationMode mode) {
  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;

  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    halfheight = (height + 1) >> 1;
    src_y = src_y + (height - 1) * src_stride_y;
    src_u = src_u + (halfheight - 1) * src_stride_u;
    src_v = src_v + (halfheight - 1) * src_stride_v;
    src_stride_y = -src_stride_y;
    src_stride_u = -src_stride_u;
    src_stride_v = -src_stride_v;
  }

  switch (mode) {
    case kRotateNone:
      // copy frame
      return I420Copy(src_y, src_stride_y,
                      src_u, src_stride_u,
                      src_v, src_stride_v,
                      dst_y, dst_stride_y,
                      dst_u, dst_stride_u,
                      dst_v, dst_stride_v,
                      width, height);
    case kRotateClockwise:
      RotatePlane90(src_y, src_stride_y,
                    dst_y, dst_stride_y,
                    width, height);
      RotatePlane90(src_u, src_stride_u,
                    dst_u, dst_stride_u,
                    halfwidth, halfheight);
      RotatePlane90(src_v, src_stride_v,
                    dst_v, dst_stride_v,
                    halfwidth, halfheight);
      return 0;
    case kRotateCounterClockwise:
      RotatePlane270(src_y, src_stride_y,
                     dst_y, dst_stride_y,
                     width, height);
      RotatePlane270(src_u, src_stride_u,
                     dst_u, dst_stride_u,
                     halfwidth, halfheight);
      RotatePlane270(src_v, src_stride_v,
                     dst_v, dst_stride_v,
                     halfwidth, halfheight);
      return 0;
    case kRotate180:
      RotatePlane180(src_y, src_stride_y,
                     dst_y, dst_stride_y,
                     width, height);
      RotatePlane180(src_u, src_stride_u,
                     dst_u, dst_stride_u,
                     halfwidth, halfheight);
      RotatePlane180(src_v, src_stride_v,
                     dst_v, dst_stride_v,
                     halfwidth, halfheight);
      return 0;
    default:
      break;
  }
  return -1;
}

int NV12ToI420Rotate(const uint8* src_y, int src_stride_y,
                     const uint8* src_uv, int src_stride_uv,
                     uint8* dst_y, int dst_stride_y,
                     uint8* dst_u, int dst_stride_u,
                     uint8* dst_v, int dst_stride_v,
                     int width, int height,
                     RotationMode mode) {
  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;

  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    halfheight = (height + 1) >> 1;
    src_y = src_y + (height - 1) * src_stride_y;
    src_uv = src_uv + (halfheight - 1) * src_stride_uv;
    src_stride_y = -src_stride_y;
    src_stride_uv = -src_stride_uv;
  }

  switch (mode) {
    case kRotateNone:
      // copy frame
      return NV12ToI420(src_y, src_uv, src_stride_y,
                        dst_y, dst_stride_y,
                        dst_u, dst_stride_u,
                        dst_v, dst_stride_v,
                        width, height);
    case kRotateClockwise:
      RotatePlane90(src_y, src_stride_y,
                    dst_y, dst_stride_y,
                    width, height);
      RotateUV90(src_uv, src_stride_uv,
                 dst_u, dst_stride_u,
                 dst_v, dst_stride_v,
                 halfwidth, halfheight);
      return 0;
    case kRotateCounterClockwise:
      RotatePlane270(src_y, src_stride_y,
                     dst_y, dst_stride_y,
                     width, height);
      RotateUV270(src_uv, src_stride_uv,
                  dst_u, dst_stride_u,
                  dst_v, dst_stride_v,
                  halfwidth, halfheight);
      return 0;
    case kRotate180:
      RotatePlane180(src_y, src_stride_y,
                     dst_y, dst_stride_y,
                     width, height);
      RotateUV180(src_uv, src_stride_uv,
                  dst_u, dst_stride_u,
                  dst_v, dst_stride_v,
                  halfwidth, halfheight);
      return 0;
    default:
      break;
  }
  return -1;
}

}  // namespace libyuv

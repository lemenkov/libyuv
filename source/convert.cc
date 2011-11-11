/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/convert.h"

//#define SCALEOPT //Currently for windows only. June 2010

#ifdef SCALEOPT
#include <emmintrin.h>
#endif

#include "conversion_tables.h"
#include "libyuv/basic_types.h"
#include "libyuv/cpu_id.h"
#include "libyuv/format_conversion.h"
#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"
#include "row.h"
#include "video_common.h"

namespace libyuv {

static inline uint8 Clip(int32 val) {
  if (val < 0) {
    return (uint8) 0;
  } else if (val > 255){
    return (uint8) 255;
  }
  return (uint8) val;
}

int I420ToRGB24(const uint8* src_y, int src_stride_y,
                const uint8* src_u, int src_stride_u,
                const uint8* src_v, int src_stride_v,
                uint8* dst_frame, int dst_stride_frame,
                int width, int height) {
  if (src_y == NULL || src_u == NULL || src_v == NULL || dst_frame == NULL) {
    return -1;
  }

  // RGB orientation - bottom up
  // TODO(fbarchard): support inversion
  uint8* out = dst_frame + dst_stride_frame * height - dst_stride_frame;
  uint8* out2 = out - dst_stride_frame;
  int h, w;
  int tmp_r, tmp_g, tmp_b;
  const uint8 *y1, *y2 ,*u, *v;
  y1 = src_y;
  y2 = y1 + src_stride_y;
  u = src_u;
  v = src_v;
  for (h = ((height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((width + 1) >> 1); w++){
      // Vertical and horizontal sub-sampling
      tmp_r = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
      out[0] = Clip(tmp_b);
      out[1] = Clip(tmp_g);
      out[2] = Clip(tmp_r);

      tmp_r = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
      out[3] = Clip(tmp_b);
      out[4] = Clip(tmp_g);
      out[5] = Clip(tmp_r);

      tmp_r = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
      out2[0] = Clip(tmp_b);
      out2[1] = Clip(tmp_g);
      out2[2] = Clip(tmp_r);

      tmp_r = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
      out2[3] = Clip(tmp_b);
      out2[4] = Clip(tmp_g);
      out2[5] = Clip(tmp_r);

      out += 6;
      out2 += 6;
      y1 += 2;
      y2 += 2;
      u++;
      v++;
    }
    y1 += src_stride_y + src_stride_y - width;
    y2 += src_stride_y + src_stride_y - width;
    u += src_stride_u - ((width + 1) >> 1);
    v += src_stride_v - ((width + 1) >> 1);
    out -= dst_stride_frame * 3;
    out2 -= dst_stride_frame * 3;
  } // end height for
  return 0;
}

// Little Endian...
int I420ToARGB4444(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_frame, int dst_stride_frame,
                   int width, int height) {
  if (src_y == NULL || src_u == NULL || src_v == NULL || dst_frame == NULL) {
    return -1;
  }

  // RGB orientation - bottom up
  uint8* out = dst_frame + dst_stride_frame * (height - 1);
  uint8* out2 = out - dst_stride_frame;
  int tmp_r, tmp_g, tmp_b;
  const uint8 *y1,*y2, *u, *v;
  y1 = src_y;
  y2 = y1 + src_stride_y;
  u = src_u;
  v = src_v;
  int h, w;

  for (h = ((height + 1) >> 1); h > 0; h--) {
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((width + 1) >> 1); w++) {
        // Vertical and horizontal sub-sampling
        // Convert to RGB888 and re-scale to 4 bits
        tmp_r = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
        tmp_g = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
        tmp_b = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
        out[0] =(uint8)((Clip(tmp_g) & 0xf0) + (Clip(tmp_b) >> 4));
        out[1] = (uint8)(0xf0 + (Clip(tmp_r) >> 4));

        tmp_r = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
        tmp_g = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
        tmp_b = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
        out[2] = (uint8)((Clip(tmp_g) & 0xf0 ) + (Clip(tmp_b) >> 4));
        out[3] = (uint8)(0xf0 + (Clip(tmp_r) >> 4));

        tmp_r = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
        tmp_g = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
        tmp_b = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
        out2[0] = (uint8)((Clip(tmp_g) & 0xf0 ) + (Clip(tmp_b) >> 4));
        out2[1] = (uint8) (0xf0 + (Clip(tmp_r) >> 4));

        tmp_r = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
        tmp_g = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
        tmp_b = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
        out2[2] = (uint8)((Clip(tmp_g) & 0xf0 ) + (Clip(tmp_b) >> 4));
        out2[3] = (uint8)(0xf0 + (Clip(tmp_r) >> 4));

        out += 4;
        out2 += 4;
        y1 += 2;
        y2 += 2;
        u++;
        v++;
    }
    y1 += 2 * src_stride_y - width;
    y2 += 2 * src_stride_y - width;
    u += src_stride_u - ((width + 1) >> 1);
    v += src_stride_v - ((width + 1) >> 1);
    out -= (dst_stride_frame + width) * 2;
    out2 -= (dst_stride_frame + width) * 2;
  } // end height for
  return 0;
}


int I420ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_u, int src_stride_u,
                 const uint8* src_v, int src_stride_v,
                 uint8* dst_frame, int dst_stride_frame,
                 int width, int height) {
  if (src_y == NULL || src_u == NULL || src_v == NULL || dst_frame == NULL) {
    return -1;
  }

  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_stride_y;
    src_u = src_u + (height - 1) * src_stride_u;
    src_v = src_v + (height - 1) * src_stride_v;
    src_stride_y = -src_stride_y;
    src_stride_u = -src_stride_u;
    src_stride_v = -src_stride_v;
  }
  uint16* out = (uint16*)(dst_frame) + dst_stride_frame * (height - 1);
  uint16* out2 = out - dst_stride_frame;

  int tmp_r, tmp_g, tmp_b;
  const uint8* y1,* y2, * u, * v;
  y1 = src_y;
  y2 = y1 + src_stride_y;
  u = src_u;
  v = src_v;
  int h, w;

  for (h = ((height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((width + 1) >> 1); w++){
      // Vertical and horizontal sub-sampling
      // 1. Convert to RGB888
      // 2. Shift to adequate location (in the 16 bit word) - RGB 565

      tmp_r = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
      out[0]  = (uint16)((Clip(tmp_r) & 0xf8) << 8) + ((Clip(tmp_g)
                          & 0xfc) << 3) + (Clip(tmp_b) >> 3);

      tmp_r = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
      out[1] = (uint16)((Clip(tmp_r) & 0xf8) << 8) + ((Clip(tmp_g)
                         & 0xfc) << 3) + (Clip(tmp_b ) >> 3);

      tmp_r = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
      out2[0] = (uint16)((Clip(tmp_r) & 0xf8) << 8) + ((Clip(tmp_g)
                          & 0xfc) << 3) + (Clip(tmp_b) >> 3);

      tmp_r = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
      out2[1] = (uint16)((Clip(tmp_r) & 0xf8) << 8) + ((Clip(tmp_g)
                          & 0xfc) << 3) + (Clip(tmp_b) >> 3);

      y1 += 2;
      y2 += 2;
      out += 2;
      out2 += 2;
      u++;
      v++;
    }
    y1 += 2 * src_stride_y - width;
    y2 += 2 * src_stride_y - width;
    u += src_stride_u - ((width + 1) >> 1);
    v += src_stride_v - ((width + 1) >> 1);
    out -= 2 * dst_stride_frame + width;
    out2 -=  2 * dst_stride_frame + width;
  }
  return 0;
}


int I420ToARGB1555(const uint8* src_y, int src_stride_y,
                   const uint8* src_u, int src_stride_u,
                   const uint8* src_v, int src_stride_v,
                   uint8* dst_frame, int dst_stride_frame,
                   int width, int height) {
  if (src_y == NULL || src_u == NULL || src_v == NULL || dst_frame == NULL) {
    return -1;
  }
  uint16* out = (uint16*)(dst_frame) + dst_stride_frame * (height - 1);
  uint16* out2 = out - dst_stride_frame ;
  int32 tmp_r, tmp_g, tmp_b;
  const uint8 *y1,*y2, *u, *v;
  int h, w;

  y1 = src_y;
  y2 = y1 + src_stride_y;
  u = src_u;
  v = src_v;

  for (h = ((height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((width + 1) >> 1); w++){
      // Vertical and horizontal sub-sampling
      // 1. Convert to RGB888
      // 2. Shift to adequate location (in the 16 bit word) - RGB 555
      // 3. Add 1 for alpha value
      tmp_r = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
      out[0]  = (uint16)(0x8000 + ((Clip(tmp_r) & 0xf8) << 10) +
                ((Clip(tmp_g) & 0xf8) << 3) + (Clip(tmp_b) >> 3));

      tmp_r = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]]  + 128) >> 8);
      tmp_b = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
      out[1]  = (uint16)(0x8000 + ((Clip(tmp_r) & 0xf8) << 10) +
                ((Clip(tmp_g) & 0xf8) << 3)  + (Clip(tmp_b) >> 3));

      tmp_r = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
      out2[0]  = (uint16)(0x8000 + ((Clip(tmp_r) & 0xf8) << 10) +
                 ((Clip(tmp_g) & 0xf8) << 3) + (Clip(tmp_b) >> 3));

      tmp_r = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
      out2[1]  = (uint16)(0x8000 + ((Clip(tmp_r) & 0xf8) << 10) +
                 ((Clip(tmp_g) & 0xf8) << 3)  + (Clip(tmp_b) >> 3));

      y1 += 2;
      y2 += 2;
      out += 2;
      out2 += 2;
      u++;
      v++;
    }
    y1 += 2 * src_stride_y - width;
    y2 += 2 * src_stride_y - width;
    u += src_stride_u - ((width + 1) >> 1);
    v += src_stride_v - ((width + 1) >> 1);
    out -= 2 * dst_stride_frame + width;
    out2 -=  2 * dst_stride_frame + width;
  }
  return 0;
}


int I420ToYUY2(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height) {
  if (src_y == NULL || src_u == NULL || src_v == NULL || dst_frame == NULL) {
    return -1;
  }

  const uint8* in1 = src_y;
  const uint8* in2 = src_y + src_stride_y;

  uint8* out1 = dst_frame;
  uint8* out2 = dst_frame + dst_stride_frame;

  // YUY2 - Macro-pixel = 2 image pixels
  // Y0U0Y1V0....Y2U2Y3V2...Y4U4Y5V4....
#ifndef SCALEOPT
  for (int i = 0; i < ((height + 1) >> 1); i++){
    for (int j = 0; j < ((width + 1) >> 1); j++){
      out1[0] = in1[0];
      out1[1] = *src_u;
      out1[2] = in1[1];
      out1[3] = *src_v;

      out2[0] = in2[0];
      out2[1] = *src_u;
      out2[2] = in2[1];
      out2[3] = *src_v;
      out1 += 4;
      out2 += 4;
      src_u++;
      src_v++;
      in1 += 2;
      in2 += 2;
    }
    in1 += 2 * src_stride_y - width;
    in2 += 2 * src_stride_y - width;
    src_u += src_stride_u - ((width + 1) >> 1);
    src_v += src_stride_v - ((width + 1) >> 1);
    out1 += dst_stride_frame + dst_stride_frame - 2 * width;
    out2 += dst_stride_frame + dst_stride_frame - 2 * width;
  }
#else
  for (WebRtc_UWord32 i = 0; i < ((height + 1) >> 1);i++) {
    int32 width__ = (width >> 4);
    _asm
    {
      ;pusha
      mov       eax, DWORD PTR [in1]                       ;1939.33
      mov       ecx, DWORD PTR [in2]                       ;1939.33
      mov       ebx, DWORD PTR [src_u]                       ;1939.33
      mov       edx, DWORD PTR [src_v]                       ;1939.33
      loop0:
      movq      xmm6, QWORD PTR [ebx]          ;src_u
      movq      xmm0, QWORD PTR [edx]          ;src_v
      punpcklbw xmm6, xmm0                     ;src_u, src_v mix
      ;movdqa    xmm1, xmm6
      ;movdqa    xmm2, xmm6
      ;movdqa    xmm4, xmm6

      movdqu    xmm3, XMMWORD PTR [eax]        ;in1
      movdqa    xmm1, xmm3
      punpcklbw xmm1, xmm6                     ;in1, src_u, in1, src_v
      mov       esi, DWORD PTR [out1]
      movdqu    XMMWORD PTR [esi], xmm1        ;write to out1

      movdqu    xmm5, XMMWORD PTR [ecx]        ;in2
      movdqa    xmm2, xmm5
      punpcklbw xmm2, xmm6                     ;in2, src_u, in2, src_v
      mov       edi, DWORD PTR [out2]
      movdqu    XMMWORD PTR [edi], xmm2        ;write to out2

      punpckhbw xmm3, xmm6                     ;in1, src_u, in1, src_v again
      movdqu    XMMWORD PTR [esi+16], xmm3     ;write to out1 again
      add       esi, 32
      mov       DWORD PTR [out1], esi

      punpckhbw xmm5, xmm6                     ;src_u, in2, src_v again
      movdqu    XMMWORD PTR [edi+16], xmm5     ;write to out2 again
      add       edi, 32
      mov       DWORD PTR [out2], edi

      add       ebx, 8
      add       edx, 8
      add       eax, 16
      add       ecx, 16

      mov       esi, DWORD PTR [width__]
      sub       esi, 1
      mov       DWORD PTR [width__], esi
      jg        loop0

      mov       DWORD PTR [in1], eax                       ;1939.33
      mov       DWORD PTR [in2], ecx                       ;1939.33
      mov       DWORD PTR [src_u], ebx                       ;1939.33
      mov       DWORD PTR [src_v], edx                       ;1939.33

      ;popa
      emms
    }
    in1 += 2 * src_stride_y - width;
    in2 += 2 * src_stride_y - width;
    out1 += dst_stride_frame + dst_stride_frame - 2 * width;
    out2 += dst_stride_frame + dst_stride_frame - 2 * width;
  }
#endif
  return 0;
}

int I420ToUYVY(const uint8* src_y, int src_stride_y,
               const uint8* src_u, int src_stride_u,
               const uint8* src_v, int src_stride_v,
               uint8* dst_frame, int dst_stride_frame,
               int width, int height) {
  if (src_y == NULL || src_u == NULL || src_v == NULL || dst_frame == NULL) {
    return -1;
  }

  int i = 0;
  const uint8* y1 = src_y;
  const uint8* y2 = y1 + src_stride_y;
  const uint8* u = src_u;
  const uint8* v = src_v;

  uint8* out1 = dst_frame;
  uint8* out2 = dst_frame + dst_stride_frame;

  // Macro-pixel = 2 image pixels
  // U0Y0V0Y1....U2Y2V2Y3...U4Y4V4Y5.....

#ifndef SCALEOPT
  for (; i < ((height + 1) >> 1); i++) {
    for (int j = 0; j < ((width + 1) >> 1); j++) {
      out1[0] = *u;
      out1[1] = y1[0];
      out1[2] = *v;
      out1[3] = y1[1];

      out2[0] = *u;
      out2[1] = y2[0];
      out2[2] = *v;
      out2[3] = y2[1];
      out1 += 4;
      out2 += 4;
      u++;
      v++;
      y1 += 2;
      y2 += 2;
    }
    y1 += 2 * src_stride_y - width;
    y2 += 2 * src_stride_y - width;
    u += src_stride_u - ((width + 1) >> 1);
    v += src_stride_v - ((width + 1) >> 1);
    out1 += 2 * (dst_stride_frame - width);
    out2 += 2 * (dst_stride_frame - width);
  }
#else
  for (; i < (height >> 1);i++) {
    int32 width__ = (width >> 4);
    _asm
    {
      ;pusha
      mov       eax, DWORD PTR [in1]                       ;1939.33
      mov       ecx, DWORD PTR [in2]                       ;1939.33
      mov       ebx, DWORD PTR [src_u]                       ;1939.33
      mov       edx, DWORD PTR [src_v]                       ;1939.33
loop0:
      movq      xmm6, QWORD PTR [ebx]          ;src_u
      movq      xmm0, QWORD PTR [edx]          ;src_v
      punpcklbw xmm6, xmm0                     ;src_u, src_v mix
      movdqa    xmm1, xmm6
      movdqa    xmm2, xmm6
      movdqa    xmm4, xmm6

      movdqu    xmm3, XMMWORD PTR [eax]        ;in1
      punpcklbw xmm1, xmm3                     ;src_u, in1, src_v
      mov       esi, DWORD PTR [out1]
      movdqu    XMMWORD PTR [esi], xmm1        ;write to out1

      movdqu    xmm5, XMMWORD PTR [ecx]        ;in2
      punpcklbw xmm2, xmm5                     ;src_u, in2, src_v
      mov       edi, DWORD PTR [out2]
      movdqu    XMMWORD PTR [edi], xmm2        ;write to out2

      punpckhbw xmm4, xmm3                     ;src_u, in1, src_v again
      movdqu    XMMWORD PTR [esi+16], xmm4     ;write to out1 again
      add       esi, 32
      mov       DWORD PTR [out1], esi

      punpckhbw xmm6, xmm5                     ;src_u, in2, src_v again
      movdqu    XMMWORD PTR [edi+16], xmm6     ;write to out2 again
      add       edi, 32
      mov       DWORD PTR [out2], edi

      add       ebx, 8
      add       edx, 8
      add       eax, 16
      add       ecx, 16

      mov       esi, DWORD PTR [width__]
      sub       esi, 1
      mov       DWORD PTR [width__], esi
      jg        loop0

      mov       DWORD PTR [in1], eax                       ;1939.33
      mov       DWORD PTR [in2], ecx                       ;1939.33
      mov       DWORD PTR [src_u], ebx                       ;1939.33
      mov       DWORD PTR [src_v], edx                       ;1939.33

      ;popa
      emms
    }
    in1 += width;
    in2 += width;
    out1 += 2 * (dst_stride_frame - width);
    out2 += 2 * (dst_stride_frame - width);
  }
#endif
  return 0;
}


int NV12ToRGB565(const uint8* src_y, int src_stride_y,
                 const uint8* src_uv, int src_stride_uv,
                 uint8* dst_frame, int dst_stride_frame,
                 int width, int height) {
  if (src_y == NULL || src_uv == NULL || dst_frame == NULL) {
    return -1;
  }

  // Bi-Planar: Y plane followed by an interlaced U and V plane
  const uint8* interlacedSrc = src_uv;
  uint16* out = (uint16*)(src_y) + dst_stride_frame * (height - 1);
  uint16* out2 = out - dst_stride_frame;
  int32 tmp_r, tmp_g, tmp_b;
  const uint8 *y1,*y2;
  y1 = src_y;
  y2 = y1 + src_stride_y;
  int h, w;

  for (h = ((height + 1) >> 1); h > 0; h--) {
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((width + 1) >> 1); w++) {
      // Vertical and horizontal sub-sampling
      // 1. Convert to RGB888
      // 2. Shift to adequate location (in the 16 bit word) - RGB 565

      tmp_r = (int32)((mapYc[y1[0]] + mapVcr[interlacedSrc[1]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y1[0]] + mapUcg[interlacedSrc[0]]
                      + mapVcg[interlacedSrc[1]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y1[0]] + mapUcb[interlacedSrc[0]] + 128) >> 8);
      out[0]  = (uint16)((Clip(tmp_r) & 0xf8) << 8) + ((Clip(tmp_g)
                          & 0xfc) << 3) + (Clip(tmp_b) >> 3);

      tmp_r = (int32)((mapYc[y1[1]] + mapVcr[interlacedSrc[1]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y1[1]] + mapUcg[interlacedSrc[0]]
                      + mapVcg[interlacedSrc[1]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y1[1]] + mapUcb[interlacedSrc[0]] + 128) >> 8);
      out[1] = (uint16)((Clip(tmp_r) & 0xf8) << 8) + ((Clip(tmp_g)
                         & 0xfc) << 3) + (Clip(tmp_b ) >> 3);

      tmp_r = (int32)((mapYc[y2[0]] + mapVcr[interlacedSrc[1]] + 128) >> 8);
      tmp_g = (int32)((mapYc[y2[0]] + mapUcg[interlacedSrc[0]]
                      + mapVcg[interlacedSrc[1]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y2[0]] + mapUcb[interlacedSrc[0]] + 128) >> 8);
      out2[0] = (uint16)((Clip(tmp_r) & 0xf8) << 8) + ((Clip(tmp_g)
                          & 0xfc) << 3) + (Clip(tmp_b) >> 3);

      tmp_r = (int32)((mapYc[y2[1]] + mapVcr[interlacedSrc[1]]
                      + 128) >> 8);
      tmp_g = (int32)((mapYc[y2[1]] + mapUcg[interlacedSrc[0]]
                      + mapVcg[interlacedSrc[1]] + 128) >> 8);
      tmp_b = (int32)((mapYc[y2[1]] + mapUcb[interlacedSrc[0]] + 128) >> 8);
      out2[1] = (uint16)((Clip(tmp_r) & 0xf8) << 8) + ((Clip(tmp_g)
                          & 0xfc) << 3) + (Clip(tmp_b) >> 3);

      y1 += 2;
      y2 += 2;
      out += 2;
      out2 += 2;
      interlacedSrc += 2;
    }
    y1 += 2 * src_stride_y - width;
    y2 += 2 * src_stride_y - width;
    interlacedSrc += src_stride_uv - ((width + 1) >> 1);
    out -= 3 * dst_stride_frame + dst_stride_frame - width;
    out2 -= 3 * dst_stride_frame + dst_stride_frame - width;
  }
  return 0;
}

// TODO(fbarchard): Deprecated - this is same as BG24ToARGB with -height
int RGB24ToARGB(const uint8* src_frame, int src_stride_frame,
                uint8* dst_frame, int dst_stride_frame,
                int width, int height) {
  if (src_frame == NULL || dst_frame == NULL) {
    return -1;
  }

  int i, j, offset;
  uint8* outFrame = dst_frame;
  const uint8* inFrame = src_frame;

  outFrame += dst_stride_frame * (height - 1) * 4;
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      offset = j * 4;
      outFrame[0 + offset] = inFrame[0];
      outFrame[1 + offset] = inFrame[1];
      outFrame[2 + offset] = inFrame[2];
      outFrame[3 + offset] = 0xff;
      inFrame += 3;
    }
    outFrame -= 4 * (dst_stride_frame - width);
    inFrame += src_stride_frame - width;
  }
  return 0;
}

int ARGBToI420(const uint8* src_frame, int src_stride_frame,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_frame = src_frame + (height - 1) * src_stride_frame;
    src_stride_frame = -src_stride_frame;
  }
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_y, 16) && (dst_stride_y % 16 == 0)) {
    ARGBToYRow = ARGBToYRow_SSSE3;
  } else
#endif
  {
    ARGBToYRow = ARGBToYRow_C;
  }
#if defined(HAS_ARGBTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_u, 8) && (dst_stride_u % 8 == 0) &&
      IS_ALIGNED(dst_v, 8) && (dst_stride_v % 8 == 0)) {
    ARGBToUVRow = ARGBToUVRow_SSSE3;
  } else
#endif
  {
    ARGBToUVRow = ARGBToUVRow_C;
  }

  for (int y = 0; y < (height - 1); y += 2) {
    ARGBToUVRow(src_frame, src_stride_frame, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
    ARGBToYRow(src_frame + src_stride_frame, dst_y + dst_stride_y, width);
    src_frame += src_stride_frame * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow(src_frame, 0, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
  }
  return 0;
}

int BGRAToI420(const uint8* src_frame, int src_stride_frame,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_frame = src_frame + (height - 1) * src_stride_frame;
    src_stride_frame = -src_stride_frame;
  }
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);
#if defined(HAS_BGRATOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_y, 16) && (dst_stride_y % 16 == 0)) {
    ARGBToYRow = BGRAToYRow_SSSE3;
  } else
#endif
  {
    ARGBToYRow = BGRAToYRow_C;
  }
#if defined(HAS_BGRATOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_u, 8) && (dst_stride_u % 8 == 0) &&
      IS_ALIGNED(dst_v, 8) && (dst_stride_v % 8 == 0)) {
    ARGBToUVRow = BGRAToUVRow_SSSE3;
  } else
#endif
  {
    ARGBToUVRow = BGRAToUVRow_C;
  }

  for (int y = 0; y < (height - 1); y += 2) {
    ARGBToUVRow(src_frame, src_stride_frame, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
    ARGBToYRow(src_frame + src_stride_frame, dst_y + dst_stride_y, width);
    src_frame += src_stride_frame * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow(src_frame, 0, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
  }
  return 0;
}

int ABGRToI420(const uint8* src_frame, int src_stride_frame,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_frame = src_frame + (height - 1) * src_stride_frame;
    src_stride_frame = -src_stride_frame;
  }
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);
#if defined(HAS_ABGRTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_y, 16) && (dst_stride_y % 16 == 0)) {
    ARGBToYRow = ABGRToYRow_SSSE3;
  } else
#endif
  {
    ARGBToYRow = ABGRToYRow_C;
  }
#if defined(HAS_ABGRTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_u, 8) && (dst_stride_u % 8 == 0) &&
      IS_ALIGNED(dst_v, 8) && (dst_stride_v % 8 == 0)) {
    ARGBToUVRow = ABGRToUVRow_SSSE3;
  } else
#endif
  {
    ARGBToUVRow = ABGRToUVRow_C;
  }

  for (int y = 0; y < (height - 1); y += 2) {
    ARGBToUVRow(src_frame, src_stride_frame, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
    ARGBToYRow(src_frame + src_stride_frame, dst_y + dst_stride_y, width);
    src_frame += src_stride_frame * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow(src_frame, 0, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
  }
  return 0;
}

int RGB24ToI420(const uint8* src_frame, int src_stride_frame,
                uint8* dst_y, int dst_stride_y,
                uint8* dst_u, int dst_stride_u,
                uint8* dst_v, int dst_stride_v,
                int width, int height) {
  if (height < 0) {
    height = -height;
    src_frame = src_frame + (height - 1) * src_stride_frame;
    src_stride_frame = -src_stride_frame;
  }
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);
#if defined(HAS_RGB24TOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_y, 16) && (dst_stride_y % 16 == 0)) {
    ARGBToYRow = RGB24ToYRow_SSSE3;
  } else
#endif
  {
    ARGBToYRow = RGB24ToYRow_C;
  }
#if defined(HAS_RGB24TOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_u, 8) && (dst_stride_u % 8 == 0) &&
      IS_ALIGNED(dst_v, 8) && (dst_stride_v % 8 == 0)) {
    ARGBToUVRow = RGB24ToUVRow_SSSE3;
  } else
#endif
  {
    ARGBToUVRow = RGB24ToUVRow_C;
  }

  for (int y = 0; y < (height - 1); y += 2) {
    ARGBToUVRow(src_frame, src_stride_frame, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
    ARGBToYRow(src_frame + src_stride_frame, dst_y + dst_stride_y, width);
    src_frame += src_stride_frame * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow(src_frame, 0, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
  }
  return 0;
}

int RAWToI420(const uint8* src_frame, int src_stride_frame,
                uint8* dst_y, int dst_stride_y,
                uint8* dst_u, int dst_stride_u,
                uint8* dst_v, int dst_stride_v,
                int width, int height) {
  if (height < 0) {
    height = -height;
    src_frame = src_frame + (height - 1) * src_stride_frame;
    src_stride_frame = -src_stride_frame;
  }
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);
#if defined(HAS_RAWTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_y, 16) && (dst_stride_y % 16 == 0)) {
    ARGBToYRow = RAWToYRow_SSSE3;
  } else
#endif
  {
    ARGBToYRow = RAWToYRow_C;
  }
#if defined(HAS_RAWTOUVROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_frame, 16) && (src_stride_frame % 16 == 0) &&
      IS_ALIGNED(dst_u, 8) && (dst_stride_u % 8 == 0) &&
      IS_ALIGNED(dst_v, 8) && (dst_stride_v % 8 == 0)) {
    ARGBToUVRow = RAWToUVRow_SSSE3;
  } else
#endif
  {
    ARGBToUVRow = RAWToUVRow_C;
  }

  for (int y = 0; y < (height - 1); y += 2) {
    ARGBToUVRow(src_frame, src_stride_frame, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
    ARGBToYRow(src_frame + src_stride_frame, dst_y + dst_stride_y, width);
    src_frame += src_stride_frame * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow(src_frame, 0, dst_u, dst_v, width);
    ARGBToYRow(src_frame, dst_y, width);
  }
  return 0;
}

// Convert camera sample to I420 with cropping, rotation and vertical flip.
// src_width is used for source stride computation
// src_height is used to compute location of planes, and indicate inversion
int ConvertToI420(const uint8* sample, size_t sample_size,
                  uint8* y, int y_stride,
                  uint8* u, int u_stride,
                  uint8* v, int v_stride,
                  int crop_x, int crop_y,
                  int src_width, int src_height,
                  int dst_width, int dst_height,
                  RotationMode rotation,
                  uint32 format) {
  if (y == NULL || u == NULL || v == NULL || sample == NULL) {
    return -1;
  }
  int aligned_src_width = (src_width + 1) & ~1;
  const uint8* src;
  const uint8* src_uv;
  int abs_src_height = (src_height < 0) ? -src_height : src_height;
  int inv_dst_height = (dst_height < 0) ? -dst_height : dst_height;
  if (src_height < 0) {
    inv_dst_height = -inv_dst_height;
  }

  switch (format) {
    // Single plane formats
    case FOURCC_YUY2:
      src = sample + (aligned_src_width * crop_y + crop_x) * 2 ;
      YUY2ToI420(src, aligned_src_width * 2,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    case FOURCC_UYVY:
      src = sample + (aligned_src_width * crop_y + crop_x) * 2;
      UYVYToI420(src, aligned_src_width * 2,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    case FOURCC_24BG:
      src = sample + (src_width * crop_y + crop_x) * 3;
      RGB24ToI420(src, src_width * 3,
                  y, y_stride,
                  u, u_stride,
                  v, v_stride,
                  dst_width, inv_dst_height);
      break;
    case FOURCC_RAW:
      src = sample + (src_width * crop_y + crop_x) * 3;
      RAWToI420(src, src_width * 3,
                y, y_stride,
                u, u_stride,
                v, v_stride,
                dst_width, inv_dst_height);
      break;
    case FOURCC_ARGB:
      src = sample + (src_width * crop_y + crop_x) * 4;
      ARGBToI420(src, src_width * 4,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    case FOURCC_BGRA:
      src = sample + (src_width * crop_y + crop_x) * 4;
      BGRAToI420(src, src_width * 4,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    case FOURCC_ABGR:
      src = sample + (src_width * crop_y + crop_x) * 4;
      ABGRToI420(src, src_width * 4,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    case FOURCC_BGGR:
    case FOURCC_RGGB:
    case FOURCC_GRBG:
    case FOURCC_GBRG:
      // TODO(fbarchard): We could support cropping by odd numbers by
      // adjusting fourcc.
      src = sample + (src_width * crop_y + crop_x);
      BayerRGBToI420(src, src_width, format,
                     y, y_stride, u, u_stride, v, v_stride,
                     dst_width, inv_dst_height);
      break;
    // Biplanar formats
    case FOURCC_M420:
      src = sample + (src_width * crop_y) * 12 / 8 + crop_x;
      M420ToI420(src, src_width,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    case FOURCC_NV12:
      src = sample + (src_width * crop_y + crop_x);
      src_uv = sample + aligned_src_width * (src_height + crop_y / 2) + crop_x;
      NV12ToI420Rotate(src, src_width,
                       src_uv, aligned_src_width,
                       y, y_stride,
                       u, u_stride,
                       v, v_stride,
                       dst_width, inv_dst_height, rotation);
      break;
    case FOURCC_NV21:
      src = sample + (src_width * crop_y + crop_x);
      src_uv = sample + aligned_src_width * (src_height + crop_y / 2) + crop_x;
      // Call NV12 but with u and v parameters swapped.
      NV12ToI420Rotate(src, src_width,
                       src_uv, aligned_src_width,
                       y, y_stride,
                       u, u_stride,
                       v, v_stride,
                       dst_width, inv_dst_height, rotation);
      break;
    case FOURCC_Q420:
      src = sample + (src_width + aligned_src_width * 2) * crop_y + crop_x;
      src_uv = sample + (src_width + aligned_src_width * 2) * crop_y +
               src_width + crop_x * 2;
      Q420ToI420(src, src_width * 3,
                 src_uv, src_width * 3,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    // Triplanar formats
    case FOURCC_I420:
    case FOURCC_YV12: {
      const uint8* src_y = sample + (src_width * crop_y + crop_x);
      const uint8* src_u;
      const uint8* src_v;
      int halfwidth = (src_width + 1) / 2;
      int halfheight = (abs_src_height + 1) / 2;
      if (format == FOURCC_I420) {
        src_u = sample + src_width * abs_src_height +
            (halfwidth * crop_y + crop_x) / 2;
        src_v = sample + src_width * abs_src_height +
            halfwidth * (halfheight + crop_y / 2) + crop_x / 2;
      } else {
        src_v = sample + src_width * abs_src_height +
            (halfwidth * crop_y + crop_x) / 2;
        src_u = sample + src_width * abs_src_height +
            halfwidth * (halfheight + crop_y / 2) + crop_x / 2;
      }
      I420Rotate(src_y, src_width,
                 src_u, halfwidth,
                 src_v, halfwidth,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height, rotation);
      break;
    }
    // Formats not supported
    case FOURCC_MJPG:
    default:
      return -1;  // unknown fourcc - return failure code.
  }
  return 0;
}

} // namespace libyuv

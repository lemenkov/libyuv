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

#include "libyuv/basic_types.h"
#include "conversion_tables.h"

//#define SCALEOPT //Currently for windows only. June 2010

#ifdef SCALEOPT
#include <emmintrin.h>
#endif

namespace libyuv {

static inline uint8 Clip(int32 val) {
  if (val < 0) {
    return (uint8) 0;
  } else if (val > 255){
    return (uint8) 255;
  }
  return (uint8) val;
}

int I420ToRGB24(const uint8* src_yplane, int src_ystride,
                const uint8* src_uplane, int src_ustride,
                const uint8* src_vplane, int src_vstride,
                uint8* dst_frame, int dst_stride,
                int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL)
    return -1;

  // RGB orientation - bottom up
  uint8* out = dst_frame + dst_stride * src_height - dst_stride;
  uint8* out2 = out - dst_stride;
  int h, w;
  int tmp_r, tmp_g, tmp_b;
  const uint8 *y1, *y2 ,*u, *v;
  y1 = src_yplane;
  y2 = y1 + src_ystride;
  u = src_uplane;
  v = src_vplane;
  for (h = ((src_height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((src_width + 1) >> 1); w++){
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
    y1 += src_ystride + src_ystride - src_width;
    y2 += src_ystride + src_ystride - src_width;
    u += src_ustride - ((src_width + 1) >> 1);
    v += src_vstride - ((src_width + 1) >> 1);
    out -= dst_stride * 3;
    out2 -= dst_stride * 3;
  } // end height for
  return 0;
}

// Little Endian...
int I420ToARGB4444(const uint8* src_yplane, int src_ystride,
                   const uint8* src_uplane, int src_ustride,
                   const uint8* src_vplane, int src_vstride,
                   uint8* dst_frame, int dst_stride,
                   int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL)
    return -1;

  // RGB orientation - bottom up
  uint8* out = dst_frame + dst_stride * (src_height - 1);
  uint8* out2 = out - dst_stride;
  int tmp_r, tmp_g, tmp_b;
  const uint8 *y1,*y2, *u, *v;
  y1 = src_yplane;
  y2 = y1 + src_ystride;
  u = src_uplane;
  v = src_vplane;
  int h, w;

  for (h = ((src_height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((src_width + 1) >> 1); w++){
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
    y1 += 2 * src_ystride - src_width;
    y2 += 2 * src_ystride - src_width;
    u += src_ustride - ((src_width + 1) >> 1);
    v += src_vstride - ((src_width + 1) >> 1);
    out -= (dst_stride + src_width) * 2;
    out2 -= (dst_stride + src_width) * 2;
  } // end height for
  return 0;
}


int I420ToRGB565(const uint8* src_yplane, int src_ystride,
                 const uint8* src_uplane, int src_ustride,
                 const uint8* src_vplane, int src_vstride,
                 uint8* dst_frame, int dst_stride,
                 int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL)
    return -1;

  // Negative height means invert the image.
  if (src_height < 0) {
    src_height = -src_height;
    src_yplane = src_yplane + (src_height - 1) * src_ystride;
    src_uplane = src_uplane + (src_height - 1) * src_ustride;
    src_vplane = src_vplane + (src_height - 1) * src_vstride;
    src_ystride = -src_ystride;
    src_ustride = -src_ustride;
    src_vstride = -src_vstride;
  }
  uint16* out = (uint16*)(dst_frame) + dst_stride * (src_height - 1);
  uint16* out2 = out - dst_stride;

  int tmp_r, tmp_g, tmp_b;
  const uint8 *y1,*y2, *u, *v;
  y1 = src_yplane;
  y2 = y1 + src_ystride;
  u = src_uplane;
  v = src_vplane;
  int h, w;

  for (h = ((src_height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((src_width + 1) >> 1); w++){
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
    y1 += 2 * src_ystride - src_width;
    y2 += 2 * src_ystride - src_width;
    u += src_ustride - ((src_width + 1) >> 1);
    v += src_vstride - ((src_width + 1) >> 1);
    out -= 2 * dst_stride + src_width;
    out2 -=  2 * dst_stride + src_width;
  }
  return 0;
}


int I420ToARGB1555(const uint8* src_yplane, int src_ystride,
                   const uint8* src_uplane, int src_ustride,
                   const uint8* src_vplane, int src_vstride,
                   uint8* dst_frame, int dst_stride,
                   int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL){
     return -1;
  }
  uint16* out = (uint16*)(dst_frame) + dst_stride * (src_height - 1);
  uint16* out2 = out - dst_stride ;
  int32 tmp_r, tmp_g, tmp_b;
  const uint8 *y1,*y2, *u, *v;
  int h, w;

  y1 = src_yplane;
  y2 = y1 + src_ystride;
  u = src_uplane;
  v = src_vplane;

  for (h = ((src_height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((src_width + 1) >> 1); w++){
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
    y1 += 2 * src_ystride - src_width;
    y2 += 2 * src_ystride - src_width;
    u += src_ustride - ((src_width + 1) >> 1);
    v += src_vstride - ((src_width + 1) >> 1);
    out -= 2 * dst_stride + src_width;
    out2 -=  2 * dst_stride + src_width;
  }
  return 0;
}


int I420ToYUY2(const uint8* src_yplane, int src_ystride,
               const uint8* src_uplane, int src_ustride,
               const uint8* src_vplane, int src_vstride,
               uint8* dst_frame, int dst_stride,
               int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL){
    return -1;
  }

  const uint8* in1 = src_yplane;
  const uint8* in2 = src_yplane + src_ystride ;
  const uint8* src_u = src_uplane;
  const uint8* src_v = src_vplane;

  uint8* out1 = dst_frame;
  uint8* out2 = dst_frame + dst_stride;

  // YUY2 - Macro-pixel = 2 image pixels
  // Y0U0Y1V0....Y2U2Y3V2...Y4U4Y5V4....
#ifndef SCALEOPT
  for (int i = 0; i < ((src_height + 1) >> 1); i++){
    for (int j = 0; j < ((src_width + 1) >> 1); j++){
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
    in1 += 2 * src_ystride - src_width;
    in2 += 2 * src_ystride - src_width;
    src_u += src_ustride - ((src_width + 1) >> 1);
    src_v += src_vstride - ((src_width + 1) >> 1);
    out1 += dst_stride + dst_stride - 2 * src_width;
    out2 += dst_stride + dst_stride - 2 * src_width;
  }
#else
  for (WebRtc_UWord32 i = 0; i < ((height + 1) >> 1);i++)
  {
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
    in1 += 2 * src_ystride - src_width;
    in2 += 2 * src_ystride - src_width;
    out1 += dst_stride + dst_stride - 2 * width;
    out2 += dst_stride + dst_stride - 2 * width;
  }
#endif
  return 0;
}

int I420ToUYVY(const uint8* src_yplane, int src_ystride,
               const uint8* src_uplane, int src_ustride,
               const uint8* src_vplane, int src_vstride,
               uint8* dst_frame, int dst_stride,
               int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL)
    return -1;

  int i = 0;
  const uint8* y1 = src_yplane;
  const uint8* y2 = y1 + src_ystride;
  const uint8* u = src_uplane;
  const uint8* v = src_vplane;

  uint8* out1 = dst_frame;
  uint8* out2 = dst_frame + dst_stride;

  // Macro-pixel = 2 image pixels
  // U0Y0V0Y1....U2Y2V2Y3...U4Y4V4Y5.....

#ifndef SCALEOPT
  for (; i < ((src_height + 1) >> 1);i++){
    for (int j = 0; j < ((src_width + 1) >> 1) ;j++){
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
    y1 += 2 * src_ystride - src_width;
    y2 += 2 * src_ystride - src_width;
    u += src_ustride - ((src_width + 1) >> 1);
    v += src_vstride - ((src_width + 1) >> 1);
    out1 += 2 * (dst_stride - src_width);
    out2 += 2 * (dst_stride - src_width);
  }
#else
  for (; i < (height >> 1);i++)
  {
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
    out1 += 2 * (dst_stride - width);
    out2 += 2 * (dst_stride - width);
  }
#endif
  return 0;
}


int NV12ToRGB565(const uint8* src_yplane, int src_ystride,
                 const  uint8* src_uvplane, int src_uvstride,
                 uint8* dst_frame, int dst_stride,
                 int src_width, int src_height)
{
  if (src_yplane == NULL || src_uvplane == NULL || dst_frame == NULL)
     return -1;

  // Bi-Planar: Y plane followed by an interlaced U and V plane
  const uint8* interlacedSrc = src_uvplane;
  uint16* out = (uint16*)(src_yplane) + dst_stride * (src_height - 1);
  uint16* out2 = out - dst_stride;
  int32 tmp_r, tmp_g, tmp_b;
  const uint8 *y1,*y2;
  y1 = src_yplane;
  y2 = y1 + src_ystride;
  int h, w;

  for (h = ((src_height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((src_width + 1) >> 1); w++){
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
    y1 += 2 * src_ystride - src_width;
    y2 += 2 * src_ystride - src_width;
    interlacedSrc += src_uvstride - ((src_width + 1) >> 1);
    out -= 3 * dst_stride + dst_stride - src_width;
    out2 -= 3 * dst_stride + dst_stride - src_width;
  }
  return 0;
}

int RGB24ToARGB(const uint8* src_frame, int src_stride,
                uint8* dst_frame, int dst_stride,
                int src_width, int src_height)
{
  if (src_frame == NULL || dst_frame == NULL)
    return -1;

  int i, j, offset;
  uint8* outFrame = dst_frame;
  const uint8* inFrame = src_frame;

  outFrame += dst_stride * (src_height - 1) * 4;
  for (i = 0; i < src_height; i++){
    for (j = 0; j < src_width; j++){
      offset = j * 4;
      outFrame[0 + offset] = inFrame[0];
      outFrame[1 + offset] = inFrame[1];
      outFrame[2 + offset] = inFrame[2];
      outFrame[3 + offset] = 0xff;
      inFrame += 3;
    }
    outFrame -= 4 * (dst_stride - src_width);
    inFrame += src_stride - src_width;
  }
  return 0;
}

// ARGBToI420Row_C etc row functions use the following macro, generating
// code with RGB offsets/strides different for each version.  Less error
// prone than duplicating the code.
// template could be used, but macro method works for C and asm and this is
// performance critical code.

#define MAKEROWRGBTOI420(NAME,R,G,B,BPP) \
static void \
NAME(const uint8* src_row0, const uint8* src_row1, \
         uint8* dst_yplane0, uint8* dst_yplane1, \
         uint8* dst_uplane, \
         uint8* dst_vplane, \
         int src_width) { \
  for (int x = 0; x < src_width - 1; x += 2) { \
    dst_yplane0[0] = (uint8)((src_row0[R] * 66 + \
                              src_row0[G] * 129 + \
                              src_row0[B] * 25 + 128) >> 8) + 16; \
    dst_yplane0[1] = (uint8)((src_row0[R + BPP] * 66 + \
                              src_row0[G + BPP] * 129 + \
                              src_row0[B + BPP] * 25 + 128) >> 8) + 16; \
    dst_yplane1[0] = (uint8)((src_row1[R] * 66 + \
                              src_row1[G] * 129 + \
                              src_row1[B] * 25 + 128) >> 8) + 16; \
    dst_yplane1[1] = (uint8)((src_row1[R + BPP] * 66 + \
                              src_row1[G + BPP] * 129 + \
                              src_row1[B + BPP] * 25 + 128) >> 8) + 16; \
    dst_uplane[0] = (uint8)(((src_row0[R] + src_row0[R + BPP] + \
                              src_row1[R] + src_row1[R + BPP]) * -38 + \
                             (src_row0[G] + src_row0[G + BPP] + \
                              src_row1[G] + src_row1[G + BPP]) * -74 + \
                             (src_row0[B] + src_row0[B + BPP] + \
                              src_row1[B] + src_row1[B + BPP]) * 112 + \
                              + 512) >> 10) + 128; \
    dst_vplane[0] = (uint8)(((src_row0[R] + src_row0[R + BPP] + \
                              src_row1[R] + src_row1[R + BPP]) * 112 + \
                             (src_row0[G] + src_row0[G + BPP] + \
                              src_row1[G] + src_row1[G + BPP]) * -94 + \
                             (src_row0[B] + src_row0[B + BPP] + \
                              src_row1[B] + src_row1[B + BPP]) * -18 + \
                              + 512) >> 10) + 128; \
    dst_yplane0 += 2; \
    dst_yplane1 += 2; \
    ++dst_uplane; \
    ++dst_vplane; \
    src_row0 += BPP * 2; \
    src_row1 += BPP * 2; \
  } \
  if (src_width & 1) { \
    dst_yplane0[0] = (uint8)((src_row0[R] * 66 + \
                              src_row0[G] * 129 + \
                              src_row0[B] * 25 + 128) >> 8) + 16; \
    dst_yplane1[0] = (uint8)((src_row1[R] * 66 + \
                              src_row1[G] * 129 + \
                              src_row1[B] * 25 + 128) >> 8) + 16; \
    dst_uplane[0] = (uint8)(((src_row0[R] + \
                              src_row1[R]) * -38 + \
                             (src_row0[G] + \
                              src_row1[G]) * -74 + \
                             (src_row0[B] + \
                              src_row1[B]) * 112 + \
                              + 256) >> 9) + 128; \
    dst_vplane[0] = (uint8)(((src_row0[R] + \
                              src_row1[R]) * 112 + \
                             (src_row0[G] + \
                              src_row1[G]) * -94 + \
                             (src_row0[B] + \
                              src_row1[B]) * -18 + \
                              + 256) >> 9) + 128; \
  } \
}

// Generate variations of RGBToI420.  Parameters are r,g,b offsets within a
// pixel, and number of bytes per pixel.
MAKEROWRGBTOI420(ARGBToI420Row_C, 2, 1, 0, 4)
MAKEROWRGBTOI420(BGRAToI420Row_C, 1, 2, 3, 4)
MAKEROWRGBTOI420(ABGRToI420Row_C, 0, 1, 2, 4)
MAKEROWRGBTOI420(RGB24ToI420Row_C, 2, 1, 0, 3)
MAKEROWRGBTOI420(RAWToI420Row_C, 0, 1, 2, 3)

static int RGBToI420(const uint8* src_frame, int src_stride,
                     uint8* dst_yplane, int dst_ystride,
                     uint8* dst_uplane, int dst_ustride,
                     uint8* dst_vplane, int dst_vstride,
                     int src_width, int src_height,
                     void (*RGBToI420Row)(const uint8* src_row0,
                                          const uint8* src_row1,
                                          uint8* dst_yplane0,
                                          uint8* dst_yplane1,
                                          uint8* dst_uplane,
                                          uint8* dst_vplane,
                                          int src_width)) {
  if (src_frame == NULL || dst_yplane == NULL ||
      dst_vplane == NULL || dst_vplane == NULL)
    return -1;

  if (src_height < 0) {
    src_height = -src_height;
    src_frame = src_frame + src_stride * (src_height -1);
    src_stride = -src_stride;
  }
  for (int y = 0; y < src_height - 1; y += 2) {
    RGBToI420Row(src_frame, src_frame + src_stride,
                 dst_yplane, dst_yplane + dst_ystride,
                 dst_uplane, dst_vplane,
                 src_width);
    src_frame += src_stride * 2;
    dst_yplane += dst_ystride * 2;
    dst_uplane += dst_ustride;
    dst_vplane += dst_vstride;
  }
  if (src_height & 1) {
    RGBToI420Row(src_frame, src_frame,
                 dst_yplane, dst_yplane,
                 dst_uplane, dst_vplane,
                 src_width);
  }
  return 0;
}

int ARGBToI420(const uint8* src_frame, int src_stride,
               uint8* dst_yplane, int dst_ystride,
               uint8* dst_uplane, int dst_ustride,
               uint8* dst_vplane, int dst_vstride,
               int src_width, int src_height) {
  return RGBToI420(src_frame, src_stride,
                   dst_yplane, dst_ystride,
                   dst_uplane, dst_ustride,
                   dst_vplane, dst_vstride,
                   src_width, src_height, ARGBToI420Row_C);
}

int BGRAToI420(const uint8* src_frame, int src_stride,
               uint8* dst_yplane, int dst_ystride,
               uint8* dst_uplane, int dst_ustride,
               uint8* dst_vplane, int dst_vstride,
               int src_width, int src_height) {
  return RGBToI420(src_frame, src_stride,
                   dst_yplane, dst_ystride,
                   dst_uplane, dst_ustride,
                   dst_vplane, dst_vstride,
                   src_width, src_height, BGRAToI420Row_C);
}

int ABGRToI420(const uint8* src_frame, int src_stride,
               uint8* dst_yplane, int dst_ystride,
               uint8* dst_uplane, int dst_ustride,
               uint8* dst_vplane, int dst_vstride,
               int src_width, int src_height) {
  return RGBToI420(src_frame, src_stride,
                   dst_yplane, dst_ystride,
                   dst_uplane, dst_ustride,
                   dst_vplane, dst_vstride,
                   src_width, src_height, ABGRToI420Row_C);
}

int RGB24ToI420(const uint8* src_frame, int src_stride,
                uint8* dst_yplane, int dst_ystride,
                uint8* dst_uplane, int dst_ustride,
                uint8* dst_vplane, int dst_vstride,
                int src_width, int src_height) {
  return RGBToI420(src_frame, src_stride,
                   dst_yplane, dst_ystride,
                   dst_uplane, dst_ustride,
                   dst_vplane, dst_vstride,
                   src_width, src_height, RGB24ToI420Row_C);
}

int RAWToI420(const uint8* src_frame, int src_stride,
              uint8* dst_yplane, int dst_ystride,
              uint8* dst_uplane, int dst_ustride,
              uint8* dst_vplane, int dst_vstride,
              int src_width, int src_height) {
  return RGBToI420(src_frame, src_stride,
                   dst_yplane, dst_ystride,
                   dst_uplane, dst_ustride,
                   dst_vplane, dst_vstride,
                   src_width, src_height, RAWToI420Row_C);
}

} // namespace libyuv

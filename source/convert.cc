/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "convert.h"

#include <string.h>     // memcpy(), memset()
#include <assert.h>
#include <stdlib.h>     // abs

//#define SCALEOPT //Currently for windows only. June 2010

#ifdef SCALEOPT
#include <emmintrin.h>
#endif

#include "conversion_tables.h"

namespace libyuv
{


// Clip value to [0,255]
inline uint8 Clip(int32 val);

#ifdef SCALEOPT
void *memcpy_16(void * dest, const void * src, size_t n);
void *memcpy_8(void * dest, const void * src, size_t n);
#endif


int
Convert::I420ToRGB24(const uint8* src_yplane, int src_ystride,
                     const uint8* src_uplane, int src_ustride,
                     const uint8* src_vplane, int src_vstride,
                     uint8* dst_frame, int dst_stride,
                     int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL)
    return -1;

  // RGB orientation - bottom up
  uint8* out = dst_frame + dst_stride * src_height * 3 - dst_stride * 3;
  uint8* out2 = out - dst_stride * 3;
  int h, w;
  int tmpR, tmpG, tmpB;
  const uint8 *y1, *y2 ,*u, *v;
  y1 = src_yplane;
  y2 = y1 + src_ystride;
  u = src_uplane;
  v = src_vplane;
  for (h = ((src_height + 1) >> 1); h > 0; h--){
    // 2 rows at a time, 2 y's at a time
    for (w = 0; w < ((src_width + 1) >> 1); w++){
      // Vertical and horizontal sub-sampling
      tmpR = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
      out[0] = Clip(tmpB);
      out[1] = Clip(tmpG);
      out[2] = Clip(tmpR);

      tmpR = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
      out[3] = Clip(tmpB);
      out[4] = Clip(tmpG);
      out[5] = Clip(tmpR);

      tmpR = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
      out2[0] = Clip(tmpB);
      out2[1] = Clip(tmpG);
      out2[2] = Clip(tmpR);

      tmpR = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
      out2[3] = Clip(tmpB);
      out2[4] = Clip(tmpG);
      out2[5] = Clip(tmpR);

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
    out -= dst_stride * 9;
    out2 -= dst_stride * 9;
  } // end height for
  return 0;
}


int
Convert::I420ToARGB(const uint8* src_yplane, int src_ystride,
                    const uint8* src_uplane, int src_ustride,
                    const uint8* src_vplane, int src_vstride,
                    uint8* dst_frame, int dst_stride,
                    int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL){
      return -1;
    }

  uint8* out1 = dst_frame;
  uint8* out2 = out1 + dst_stride * 4;
  const uint8 *y1,*y2, *u, *v;
  y1 = src_yplane;
  y2 = src_yplane + src_ystride;
  u = src_uplane;
  v = src_vplane;
  int h, w;
  int tmpR, tmpG, tmpB;

  for (h = ((src_height + 1) >> 1); h > 0; h--){
    // Do 2 rows at the time
    for (w = 0; w < ((src_width + 1) >> 1); w++){
      // Vertical and horizontal sub-sampling

      tmpR = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
      out1[0] = Clip(tmpB);
      out1[1] = Clip(tmpG);
      out1[2] = Clip(tmpR);
      out1[3] = 0xff;

      tmpR = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
      out1[4] = Clip(tmpB);
      out1[5] = Clip(tmpG);
      out1[6] = Clip(tmpR);
      out1[7] = 0xff;

      tmpR = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
      out2[0] = Clip(tmpB);
      out2[1] = Clip(tmpG);
      out2[2] = Clip(tmpR);
      out2[3] = 0xff;

      tmpR = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
      out2[4] = Clip(tmpB);
      out2[5] = Clip(tmpG);
      out2[6] = Clip(tmpR);
      out2[7] = 0xff;

      out1 += 8;
      out2 += 8;
      y1 += 2;
      y2 += 2;
      u++;
      v++;
    }
  y1 += 2 * src_ystride - src_width;
  y2 += 2 * src_ystride - src_width;
  u += src_ustride - ((src_width + 1) >> 1);
  v += src_vstride - ((src_width + 1) >> 1);
  out1 += (2 * dst_stride - src_width) * 4;
  out2 += (2 * dst_stride - src_width) * 4;
  } // end height for
  return 0;
}


int
Convert::I420ToBGRA(const uint8* src_yplane, int src_ystride,
                    const uint8* src_uplane, int src_ustride,
                    const uint8* src_vplane, int src_vstride,
                    uint8* dst_frame, int dst_stride,
                    int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL){
      return -1;
    }

  uint8* out1 = dst_frame;
  uint8* out2 = out1 + dst_stride * 4;
  const uint8 *y1,*y2, *u, *v;
  y1 = src_yplane;
  y2 = src_yplane + src_ystride;
  u = src_uplane;
  v = src_vplane;
  int h, w;
  int tmpR, tmpG, tmpB;

  for (h = ((src_height + 1) >> 1); h > 0; h--){
    // Do 2 rows at the time
    for (w = 0; w < ((src_width + 1) >> 1); w++){
      // Vertical and horizontal sub-sampling

      tmpR = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
      out1[0] = 0xff;
      out1[1] = Clip(tmpR);
      out1[2] = Clip(tmpG);
      out1[3] = Clip(tmpB);

      tmpR = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
      out1[4] = 0xff;
      out1[5] = Clip(tmpR);
      out1[6] = Clip(tmpG);
      out1[7] = Clip(tmpB);

      tmpR = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
      out2[0] = 0xff;
      out2[1] = Clip(tmpR);
      out2[2] = Clip(tmpG);
      out2[3] = Clip(tmpB);

      tmpR = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
      out2[4] = 0xff;
      out2[5] = Clip(tmpR);
      out2[6] = Clip(tmpG);
      out2[7] = Clip(tmpB);

      out1 += 8;
      out2 += 8;
      y1 += 2;
      y2 += 2;
      u++;
      v++;
    }
  y1 += 2 * src_ystride - src_width;
  y2 += 2 * src_ystride - src_width;
  u += src_ustride - ((src_width + 1) >> 1);
  v += src_vstride - ((src_width + 1) >> 1);
  out1 += (2 * dst_stride - src_width) * 4;
  out2 += (2 * dst_stride - src_width) * 4;
  } // end height for
  return 0;
}


// Little Endian...
int
Convert::I420ToARGB4444(const uint8* src_yplane, int src_ystride,
                        const uint8* src_uplane, int src_ustride,
                        const uint8* src_vplane, int src_vstride,
                        uint8* dst_frame, int dst_stride,
                        int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL){
    return -1;
  }
  // RGB orientation - bottom up
  uint8* out = dst_frame + dst_stride * (src_height - 1) * 2;
  uint8* out2 = out - (2 * dst_stride);
  int tmpR, tmpG, tmpB;
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
        tmpR = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
        tmpG = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
        tmpB = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
        out[0] =(uint8)((Clip(tmpG) & 0xf0) + (Clip(tmpB) >> 4));
        out[1] = (uint8)(0xf0 + (Clip(tmpR) >> 4));

        tmpR = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
        tmpG = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
        tmpB = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
        out[2] = (uint8)((Clip(tmpG) & 0xf0 ) + (Clip(tmpB) >> 4));
        out[3] = (uint8)(0xf0 + (Clip(tmpR) >> 4));

        tmpR = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
        tmpG = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
        tmpB = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
        out2[0] = (uint8)((Clip(tmpG) & 0xf0 ) + (Clip(tmpB) >> 4));
        out2[1] = (uint8) (0xf0 + (Clip(tmpR) >> 4));

        tmpR = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
        tmpG = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
        tmpB = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
        out2[2] = (uint8)((Clip(tmpG) & 0xf0 ) + (Clip(tmpB) >> 4));
        out2[3] = (uint8)(0xf0 + (Clip(tmpR) >> 4));

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
    out -= (2 * dst_stride + src_width) * 2;
    out2 -= (2 * dst_stride + src_width) * 2;
  } // end height for
  return 0;
}


int
Convert::I420ToRGB565(const uint8* src_yplane, int src_ystride,
                      const uint8* src_uplane, int src_ustride,
                      const uint8* src_vplane, int src_vstride,
                      uint8* dst_frame, int dst_stride,
                      int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL){
    return -1;
  }

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

  int tmpR, tmpG, tmpB;
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

      tmpR = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
      out[0]  = (uint16)((Clip(tmpR) & 0xf8) << 8) + ((Clip(tmpG)
                          & 0xfc) << 3) + (Clip(tmpB) >> 3);

      tmpR = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
      out[1] = (uint16)((Clip(tmpR) & 0xf8) << 8) + ((Clip(tmpG)
                         & 0xfc) << 3) + (Clip(tmpB ) >> 3);

      tmpR = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
      out2[0] = (uint16)((Clip(tmpR) & 0xf8) << 8) + ((Clip(tmpG)
                          & 0xfc) << 3) + (Clip(tmpB) >> 3);

      tmpR = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
      out2[1] = (uint16)((Clip(tmpR) & 0xf8) << 8) + ((Clip(tmpG)
                          & 0xfc) << 3) + (Clip(tmpB) >> 3);

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
  } // end height for
  return 0;
}


int
Convert::I420ToARGB1555(const uint8* src_yplane, int src_ystride,
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
  int32 tmpR, tmpG, tmpB;
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
      tmpR = (int32)((mapYc[y1[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[0]] + mapUcb[u[0]] + 128) >> 8);
      out[0]  = (uint16)(0x8000 + ((Clip(tmpR) & 0xf8) << 10) +
                ((Clip(tmpG) & 0xf8) << 3) + (Clip(tmpB) >> 3));

      tmpR = (int32)((mapYc[y1[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[1]] + mapUcg[u[0]] + mapVcg[v[0]]  + 128) >> 8);
      tmpB = (int32)((mapYc[y1[1]] + mapUcb[u[0]] + 128) >> 8);
      out[1]  = (uint16)(0x8000 + ((Clip(tmpR) & 0xf8) << 10) +
                ((Clip(tmpG) & 0xf8) << 3)  + (Clip(tmpB) >> 3));

      tmpR = (int32)((mapYc[y2[0]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[0]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[0]] + mapUcb[u[0]] + 128) >> 8);
      out2[0]  = (uint16)(0x8000 + ((Clip(tmpR) & 0xf8) << 10) +
                 ((Clip(tmpG) & 0xf8) << 3) + (Clip(tmpB) >> 3));

      tmpR = (int32)((mapYc[y2[1]] + mapVcr[v[0]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[1]] + mapUcg[u[0]] + mapVcg[v[0]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[1]] + mapUcb[u[0]] + 128) >> 8);
      out2[1]  = (uint16)(0x8000 + ((Clip(tmpR) & 0xf8) << 10) +
                 ((Clip(tmpG) & 0xf8) << 3)  + (Clip(tmpB) >> 3));

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
  } // end height for
  return 0;
}


int
Convert::I420ToYUY2(const uint8* src_yplane, int src_ystride,
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
  const uint8* inU = src_uplane;
  const uint8* inV = src_vplane;

  uint8* out1 = dst_frame;
  uint8* out2 = dst_frame + 2 * dst_stride;

  // YUY2 - Macro-pixel = 2 image pixels
  // Y0U0Y1V0....Y2U2Y3V2...Y4U4Y5V4....
#ifndef SCALEOPT
  for (int i = 0; i < ((src_height + 1) >> 1); i++){
    for (int j = 0; j < ((src_width + 1) >> 1); j++){
      out1[0] = in1[0];
      out1[1] = *inU;
      out1[2] = in1[1];
      out1[3] = *inV;

      out2[0] = in2[0];
      out2[1] = *inU;
      out2[2] = in2[1];
      out2[3] = *inV;
      out1 += 4;
      out2 += 4;
      inU++;
      inV++;
      in1 += 2;
      in2 += 2;
    }
    in1 += 2 * src_ystride - src_width;
    in2 += 2 * src_ystride - src_width;
    inU += src_ustride - ((src_width + 1) >> 1);
    inV += src_vstride - ((src_width + 1) >> 1);
    out1 += 2 * dst_stride + 2 * (dst_stride - src_width);
    out2 += 2 * dst_stride + 2 * (dst_stride - src_width);
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
      mov       ebx, DWORD PTR [inU]                       ;1939.33
      mov       edx, DWORD PTR [inV]                       ;1939.33
      loop0:
      movq      xmm6, QWORD PTR [ebx]          ;inU
      movq      xmm0, QWORD PTR [edx]          ;inV
      punpcklbw xmm6, xmm0                     ;inU, inV mix
      ;movdqa    xmm1, xmm6
      ;movdqa    xmm2, xmm6
      ;movdqa    xmm4, xmm6

      movdqu    xmm3, XMMWORD PTR [eax]        ;in1
      movdqa    xmm1, xmm3
      punpcklbw xmm1, xmm6                     ;in1, inU, in1, inV
      mov       esi, DWORD PTR [out1]
      movdqu    XMMWORD PTR [esi], xmm1        ;write to out1

      movdqu    xmm5, XMMWORD PTR [ecx]        ;in2
      movdqa    xmm2, xmm5
      punpcklbw xmm2, xmm6                     ;in2, inU, in2, inV
      mov       edi, DWORD PTR [out2]
      movdqu    XMMWORD PTR [edi], xmm2        ;write to out2

      punpckhbw xmm3, xmm6                     ;in1, inU, in1, inV again
      movdqu    XMMWORD PTR [esi+16], xmm3     ;write to out1 again
      add       esi, 32
      mov       DWORD PTR [out1], esi

      punpckhbw xmm5, xmm6                     ;inU, in2, inV again
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
      mov       DWORD PTR [inU], ebx                       ;1939.33
      mov       DWORD PTR [inV], edx                       ;1939.33

      ;popa
      emms
    }
    in1 += 2 * src_ystride - src_width;
    in2 += 2 * src_ystride - src_width;
    out1 += 2 * strideOut + 2 * (strideOut - width);
    out2 += 2 * strideOut + 2 * (strideOut - width);
  }
#endif
  return 0;
}

int
Convert::I420ToUYVY(const uint8* src_yplane, int src_ystride,
                    const uint8* src_uplane, int src_ustride,
                    const uint8* src_vplane, int src_vstride,
                    uint8* dst_frame, int dst_stride,
                    int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_frame == NULL){
    return -1;
  }
  int i = 0;
  const uint8* y1 = src_yplane;
  const uint8* y2 = y1 + src_ystride;
  const uint8* u = src_uplane;
  const uint8* v = src_vplane;

  uint8* out1 = dst_frame;
  uint8* out2 = dst_frame + 2 * dst_stride;

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
    out1 += 2 * (dst_stride + (dst_stride - src_width));
    out2 += 2 * (dst_stride + (dst_stride - src_width));
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
      mov       ebx, DWORD PTR [inU]                       ;1939.33
      mov       edx, DWORD PTR [inV]                       ;1939.33
loop0:
      movq      xmm6, QWORD PTR [ebx]          ;inU
      movq      xmm0, QWORD PTR [edx]          ;inV
      punpcklbw xmm6, xmm0                     ;inU, inV mix
      movdqa    xmm1, xmm6
      movdqa    xmm2, xmm6
      movdqa    xmm4, xmm6

      movdqu    xmm3, XMMWORD PTR [eax]        ;in1
      punpcklbw xmm1, xmm3                     ;inU, in1, inV
      mov       esi, DWORD PTR [out1]
      movdqu    XMMWORD PTR [esi], xmm1        ;write to out1

      movdqu    xmm5, XMMWORD PTR [ecx]        ;in2
      punpcklbw xmm2, xmm5                     ;inU, in2, inV
      mov       edi, DWORD PTR [out2]
      movdqu    XMMWORD PTR [edi], xmm2        ;write to out2

      punpckhbw xmm4, xmm3                     ;inU, in1, inV again
      movdqu    XMMWORD PTR [esi+16], xmm4     ;write to out1 again
      add       esi, 32
      mov       DWORD PTR [out1], esi

      punpckhbw xmm6, xmm5                     ;inU, in2, inV again
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
      mov       DWORD PTR [inU], ebx                       ;1939.33
      mov       DWORD PTR [inV], edx                       ;1939.33

      ;popa
      emms
    }
    in1 += width;
    in2 += width;
    out1 += 2 * (strideOut + (strideOut - width));
    out2 += 2 * (strideOut + (strideOut - width));
  }
#endif
  return 0;
}


int
Convert::NV12ToRGB565(const uint8* src_yplane, int src_ystride,
                      const  uint8* src_uvplane, int src_uvstride,
                      uint8* dst_frame, int dst_stride,
                      int src_width, int src_height)
{
  if (src_yplane == NULL || src_uvplane == NULL || dst_frame == NULL){
     return -1;
  }

  // Bi-Planar: Y plane followed by an interlaced U and V plane
  const uint8* interlacedSrc = src_uvplane;
  uint16* out = (uint16*)(src_yplane) + dst_stride * (src_height - 1);
  uint16* out2 = out - dst_stride;
  int32 tmpR, tmpG, tmpB;
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

      tmpR = (int32)((mapYc[y1[0]] + mapVcr[interlacedSrc[1]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[0]] + mapUcg[interlacedSrc[0]]
                      + mapVcg[interlacedSrc[1]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[0]] + mapUcb[interlacedSrc[0]] + 128) >> 8);
      out[0]  = (uint16)((Clip(tmpR) & 0xf8) << 8) + ((Clip(tmpG)
                          & 0xfc) << 3) + (Clip(tmpB) >> 3);

      tmpR = (int32)((mapYc[y1[1]] + mapVcr[interlacedSrc[1]] + 128) >> 8);
      tmpG = (int32)((mapYc[y1[1]] + mapUcg[interlacedSrc[0]]
                      + mapVcg[interlacedSrc[1]] + 128) >> 8);
      tmpB = (int32)((mapYc[y1[1]] + mapUcb[interlacedSrc[0]] + 128) >> 8);
      out[1] = (uint16)((Clip(tmpR) & 0xf8) << 8) + ((Clip(tmpG)
                         & 0xfc) << 3) + (Clip(tmpB ) >> 3);

      tmpR = (int32)((mapYc[y2[0]] + mapVcr[interlacedSrc[1]] + 128) >> 8);
      tmpG = (int32)((mapYc[y2[0]] + mapUcg[interlacedSrc[0]]
                      + mapVcg[interlacedSrc[1]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[0]] + mapUcb[interlacedSrc[0]] + 128) >> 8);
      out2[0] = (uint16)((Clip(tmpR) & 0xf8) << 8) + ((Clip(tmpG)
                          & 0xfc) << 3) + (Clip(tmpB) >> 3);

      tmpR = (int32)((mapYc[y2[1]] + mapVcr[interlacedSrc[1]]
                      + 128) >> 8);
      tmpG = (int32)((mapYc[y2[1]] + mapUcg[interlacedSrc[0]]
                      + mapVcg[interlacedSrc[1]] + 128) >> 8);
      tmpB = (int32)((mapYc[y2[1]] + mapUcb[interlacedSrc[0]] + 128) >> 8);
      out2[1] = (uint16)((Clip(tmpR) & 0xf8) << 8) + ((Clip(tmpG)
                          & 0xfc) << 3) + (Clip(tmpB) >> 3);

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
  } // end height for
  return 0;
}

int
Convert::I420ToABGR(const uint8* src_yplane, int src_ystride,
                    const uint8* src_uplane, int src_ustride,
                    const uint8* src_vplane, int src_vstride,
                    uint8* dst_frame, int dst_stride,
                    int src_width, int src_height)
{
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
       dst_frame == NULL){
     return -1;
  }

  // RGB orientation - bottom up
  uint8* out = dst_frame + 4 * dst_stride * (src_height - 1);
  uint8* out2 = out - dst_stride * 4;
  int32 tmpR, tmpG, tmpB;
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
      tmpR = (int32)((298 * (y1[0] - 16) + 409 * (v[0] - 128) + 128) >> 8);
      tmpG = (int32)((298 * (y1[0] - 16) - 100 * (u[0] - 128)
                      - 208 * (v[0] - 128) + 128 ) >> 8);
      tmpB = (int32)((298 * (y1[0] - 16) + 516 * (u[0] - 128) + 128 ) >> 8);

      out[0] = Clip(tmpR);
      out[1] = Clip(tmpG);
      out[2] = Clip(tmpB);
      out[3] = 0xff;

      tmpR = (int32)((298 * (y1[1] - 16) + 409 * (v[0] - 128) + 128 ) >> 8);
      tmpG = (int32)((298 * (y1[1] - 16) - 100 * (u[0] - 128)
                      - 208 * (v[0] - 128) + 128 ) >> 8);
      tmpB = (int32)((298 * (y1[1] - 16) + 516 * (u[0] - 128) + 128) >> 8);

      out[4] = Clip(tmpR);
      out[5] = Clip(tmpG);
      out[6] = Clip(tmpB);
      out[7] = 0xff;

      tmpR = (int32)((298 * (y2[0] - 16) + 409 * (v[0] - 128) + 128) >> 8);
      tmpG = (int32)((298 * (y2[0] - 16) - 100 * (u[0] - 128)
                      - 208 * (v[0] - 128) + 128) >> 8);
      tmpB = (int32)((298 * (y2[0] - 16) + 516 * (u[0] - 128) + 128) >> 8);

      out2[0] = Clip(tmpR);
      out2[1] = Clip(tmpG);
      out2[2] = Clip(tmpB);
      out2[3] = 0xff;

      tmpR = (int32)((298 * (y2[1] - 16) + 409 * (v[0] - 128) + 128) >> 8);
      tmpG = (int32)((298 * (y2[1] - 16) - 100 * (u[0] - 128)
                      - 208 * (v[0] - 128) + 128) >> 8);
      tmpB = (int32)((298 * (y2[1] - 16) + 516 * (u[0] - 128) + 128 ) >> 8);

      out2[4] = Clip(tmpR);
      out2[5] = Clip(tmpG);
      out2[6] = Clip(tmpB);
      out2[7] = 0xff;

      out += 8;
      out2 += 8;
      y1 += 2;
      y2 += 2;
      u++;
      v++;
    }
    y1 += src_ystride + src_ystride - src_width;
    y2 += src_ystride + src_ystride - src_width;
    u += src_ustride - ((src_width + 1) >> 1);
    v += src_vstride - ((src_width + 1) >> 1);
    out -= (2 * dst_stride + src_width) * 4;
    out2 -= (2 * dst_stride + src_width) * 4;
  } // end height for
  return 0;
}


int
Convert::UYVYToI420(const uint8* src_frame, int src_stride,
                    uint8* dst_yplane, int dst_ystride,
                    uint8* dst_uplane, int dst_ustride,
                    uint8* dst_vplane, int dst_vstride,
                    int src_width, int src_height)
{
  if (dst_yplane == NULL || dst_uplane == NULL || dst_vplane == NULL ||
      src_frame == NULL){
     return -1;
  }

  int i, j;
  uint8* outI = dst_yplane;
  uint8* outU = dst_uplane;
  uint8* outV = dst_vplane;

  // U0Y0V0Y1..U2Y2V2Y3.....

  for (i = 0; i < ((src_height + 1) >> 1); i++){
    for (j = 0; j < ((src_width + 1) >> 1); j++){
      outI[0] = src_frame[1];
      *outU = src_frame[0];
      outI[1] = src_frame[3];
      *outV = src_frame[2];
      src_frame += 4;
      outI += 2;
      outU++;
      outV++;
    }
    for (j = 0; j < ((src_width + 1) >> 1); j++)
    {
      outI[0] = src_frame[1];
      outI[1] = src_frame[3];
      src_frame += 4;
      outI += 2;
    }
    outI += dst_ystride - src_width;
    outU += dst_ustride - ((src_width + 1) << 1);
    outV += dst_vstride - ((src_width + 1) << 1);
  }
  return 0;
}


int
Convert::RGB24ToARGB(const uint8* src_frame, int src_stride,
                     uint8* dst_frame, int dst_stride,
                     int src_width, int src_height)
{
  if (src_frame == NULL || dst_frame == NULL){
    return -1;
  }

  int i, j, offset;
  uint8* outFrame = dst_frame;
  const uint8* inFrame = src_frame;

  outFrame += dst_stride * (src_height - 1) * 4;
  for (i = 0; i < src_height; i++)
  {
    for (j = 0; j < src_width; j++)
    {
      offset = j*4;
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


int
Convert::RGB24ToI420(const uint8* src_frame, int src_stride,
                     uint8* dst_yplane, int dst_ystride,
                     uint8* dst_uplane, int dst_ustride,
                     uint8* dst_vplane, int dst_vstride,
                     int src_width, int src_height)
{
  if (src_frame == NULL || dst_yplane == NULL ||
      dst_vplane == NULL || dst_vplane == NULL)
    return -1;

  uint8* yStartPtr;
  uint8* yStartPtr2;
  uint8* uStartPtr;
  uint8* vStartPtr;
  const uint8* inpPtr;
  const uint8* inpPtr2;
  int h, w;

  // Assuming RGB in a bottom up orientation.
  yStartPtr = dst_yplane;
  yStartPtr2 = yStartPtr + dst_ystride;
  uStartPtr = dst_uplane;
  vStartPtr = dst_vplane;
  inpPtr = src_frame + src_stride * src_height * 3 - 3 * src_height;
  inpPtr2 = inpPtr - 3 * src_stride;

  for (h = 0; h < ((src_height + 1) >> 1); h++ ){
    for (w = 0; w < ((src_width + 1) >> 1); w++){
      // Y
      yStartPtr[0] =  (uint8)((66 * inpPtr[2] + 129 * inpPtr[1]
                                      + 25 * inpPtr[0] + 128) >> 8) + 16;
      yStartPtr2[0] = (uint8)((66 * inpPtr2[2] + 129 * inpPtr2[1]
                                      + 25 * inpPtr2[0] + 128) >> 8) + 16;
      // Moving to next column
      yStartPtr[1] = (uint8)((66 * inpPtr[5] + 129 * inpPtr[4]
                                     + 25 * inpPtr[3]  + 128) >> 8) + 16;
      yStartPtr2[1] = (uint8)((66 * inpPtr2[5] + 129 * inpPtr2[4]
                                      + 25 * inpPtr2[3] + 128) >> 8 ) + 16;
      // U
      uStartPtr[0] = (uint8)((-38 * inpPtr[2] - 74 * inpPtr[1] +
                                      112 * inpPtr[0] + 128) >> 8) + 128;
      // V
      vStartPtr[0] = (uint8)((112 * inpPtr[2] -94 * inpPtr[1] -
                                     18 * inpPtr[0] + 128) >> 8) + 128;

      yStartPtr += 2;
      yStartPtr2 += 2;
      uStartPtr++;
      vStartPtr++;
      inpPtr += 6;
      inpPtr2 += 6;
    } // end for w
    yStartPtr += dst_ystride + dst_ystride - src_width;
    yStartPtr2 += dst_ystride + dst_ystride - src_width;
    uStartPtr += dst_ustride + dst_ustride - ((src_width + 1) >> 1);
    vStartPtr += dst_vstride + dst_vstride - ((src_width + 1) >> 1);
    inpPtr -= 3 * (2 * src_stride + src_width);
    inpPtr2 -= 3 * (2 * src_stride + src_width);
  } // end for h
  return 0;
}

int
Convert::RAWToI420(const uint8* src_frame, int src_stride,
                   uint8* dst_yplane, int dst_ystride,
                   uint8* dst_uplane, int dst_ustride,
                   uint8* dst_vplane, int dst_vstride,
                   int src_width, int src_height)
{
  if (src_frame == NULL || dst_yplane == NULL ||
      dst_vplane == NULL || dst_vplane == NULL)
    return -1;

  uint8* yStartPtr;
  uint8* yStartPtr2;
  uint8* uStartPtr;
  uint8* vStartPtr;
  const uint8* inpPtr;
  const uint8* inpPtr2;
  int h, w;

  // Assuming RGB in a bottom up orientation.
  yStartPtr = dst_yplane;
  yStartPtr2 = yStartPtr + dst_ystride;
  uStartPtr = dst_uplane;
  vStartPtr = dst_vplane;
  inpPtr = src_frame + src_stride * src_height * 3 - 3 * src_height;
  inpPtr2 = inpPtr - 3 * src_stride;

  // Same as RGB24 - reverse ordering

  for (h = 0; h < ((src_height + 1) >> 1); h++){
    for (w = 0; w < ((src_width + 1) >> 1); w++){
      // Y
      yStartPtr[0] =  (uint8)((66 * inpPtr[0] + 129 * inpPtr[1]
                                      + 25 * inpPtr[2] + 128) >> 8) + 16;
      yStartPtr2[0] = (uint8)((66 * inpPtr2[2] + 129 * inpPtr2[1]
                                      + 25 * inpPtr2[0] + 128) >> 8) + 16;
      // Moving to next column
      yStartPtr[1] = (uint8)((66 * inpPtr[3] + 129 * inpPtr[4]
                                     + 25 * inpPtr[5]  + 128) >> 8) + 16;
      yStartPtr2[1] = (uint8)((66 * inpPtr2[3] + 129 * inpPtr2[4]
                                      + 25 * inpPtr2[5] + 128) >> 8 ) + 16;
      // U
      uStartPtr[0] = (uint8)((-38 * inpPtr[0] - 74 * inpPtr[1] +
                                      112 * inpPtr[2] + 128) >> 8) + 128;
      // V
      vStartPtr[0] = (uint8)((112 * inpPtr[0] -94 * inpPtr[1] -
                                     18 * inpPtr[2] + 128) >> 8) + 128;

      yStartPtr += 2;
      yStartPtr2 += 2;
      uStartPtr++;
      vStartPtr++;
      inpPtr += 6;
      inpPtr2 += 6;
    } // end for w
    yStartPtr += dst_ystride + dst_ystride - src_width;
    yStartPtr2 += dst_ystride + dst_ystride - src_width;
    uStartPtr += dst_ustride + dst_ustride - ((src_width + 1) >> 1);
    vStartPtr += dst_vstride + dst_vstride - ((src_width + 1) >> 1);
    inpPtr -= 3 * (2 * src_stride + src_width);
    inpPtr2 -= 3 * (2 * src_stride + src_width);
  } // end for h
  return 0;
}


int
Convert::ABGRToI420(const uint8* src_frame, int src_stride,
                    uint8* dst_yplane, int dst_ystride,
                    uint8* dst_uplane, int dst_ustride,
                    uint8* dst_vplane, int dst_vstride,
                    int src_width, int src_height)
{
  if (src_frame == NULL || dst_yplane == NULL ||
      dst_vplane == NULL || dst_vplane == NULL){
    return -1;
  }

  uint8* yStartPtr;
  uint8* yStartPtr2;
  uint8* uStartPtr;
  uint8* vStartPtr;
  const uint8* inpPtr;
  const uint8* inpPtr2;

  yStartPtr = dst_yplane;
  yStartPtr2 = yStartPtr + dst_ystride;
  uStartPtr = dst_uplane;
  vStartPtr = dst_vplane;
  inpPtr = src_frame;
  inpPtr2 = inpPtr + 4 * src_stride;
  int h, w;
  for (h = 0; h < ((src_height + 1) >> 1); h++){
    for (w = 0; w < ((src_width + 1) >> 1); w++){
      // Y
      yStartPtr[0]  = (uint8)((66 * inpPtr[1] + 129 * inpPtr[2]
                               + 25 * inpPtr[3] + 128) >> 8) + 16;
      yStartPtr2[0] = (uint8)((66 * inpPtr2[1] + 129 * inpPtr2[2]
                               + 25 * inpPtr2[3] + 128) >> 8) + 16;
      // Moving to next column
      yStartPtr[1] =  (uint8)((66 * inpPtr[5] + 129 * inpPtr[6]
                               + 25 * inpPtr[7] + 128) >> 8) + 16;
      yStartPtr2[1] = (uint8)((66 * inpPtr2[5] + 129 * inpPtr2[6]
                                + 25 * inpPtr2[7] + 128) >> 8) + 16;
      // U
      uStartPtr[0] = (uint8)((-38 * inpPtr[1] - 74 * inpPtr[2]
                              + 112 * inpPtr[3] + 128) >> 8) + 128;
      // V
      vStartPtr[0] = (uint8)((112 * inpPtr[1] - 94 * inpPtr[2]
                              - 18 * inpPtr[3] + 128) >> 8) + 128;

      yStartPtr += 2;
      yStartPtr2 += 2;
      uStartPtr++;
      vStartPtr++;
      inpPtr += 8;
      inpPtr2 += 8;
    }

    yStartPtr += 2 * dst_ystride - src_width;
    yStartPtr2 += 2 * dst_ystride - src_width;
    uStartPtr += dst_ustride + dst_ustride - ((src_width + 1) >> 1);
    vStartPtr += dst_vstride + dst_vstride - ((src_width + 1) >> 1);
    inpPtr += 4 * (2 * src_stride - src_width);
    inpPtr2 += 4 * (2 * src_stride - src_width);
  }
  return 0;
}

inline
uint8 Clip(int32 val)
{
  if (val < 0){
    return (uint8)0;
  } else if (val > 255){
    return (uint8)255;
  }
  return (uint8)val;
}

#ifdef SCALEOPT
//memcpy_16 assumes that width is an integer multiple of 16!
void
*memcpy_16(void * dest, const void * src, size_t n)
{
  _asm
  {
    mov eax, dword ptr [src]
    mov ebx, dword ptr [dest]
    mov ecx, dword ptr [n]

  loop0:

    movdqu    xmm0, XMMWORD PTR [eax]
    movdqu    XMMWORD PTR [ebx], xmm0
    add       eax, 16
    add       ebx, 16
    sub       ecx, 16
    jg        loop0
  }
}

// memcpy_8 assumes that width is an integer multiple of 8!
void
*memcpy_8(void * dest, const void * src, size_t n)
{
  _asm
  {
    mov eax, dword ptr [src]
    mov ebx, dword ptr [dest]
    mov ecx, dword ptr [n]

  loop0:

    movq    mm0, QWORD PTR [eax]
    movq    QWORD PTR [ebx], mm0
    add       eax, 8
    add       ebx, 8
    sub       ecx, 8
    jg        loop0
  emms
  }

}

#endif

} // namespace libyuv


/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/general.h"

#include <string.h>     // memcpy(), memset()

#include "libyuv/planar_functions.h"

namespace libyuv {

int I420Mirror(const uint8* src_yplane, int src_ystride,
               const uint8* src_uplane, int src_ustride,
               const uint8* src_vplane, int src_vstride,
               uint8* dst_yplane, int dst_ystride,
               uint8* dst_uplane, int dst_ustride,
               uint8* dst_vplane, int dst_vstride,
               int width, int height) {
  if (src_yplane == NULL || src_uplane == NULL || src_vplane == NULL ||
      dst_yplane == NULL || dst_uplane == NULL || dst_vplane == NULL)
    return -1;

  // Only accepts positive dimensions
  if (height < 0 || width < 0 ||  dst_ystride < 0 || dst_ustride < 0 ||
      dst_vstride < 0)
    return -1;

  int indO = 0;
  int indS  = 0;
  int wind, hind;
  uint8 tmp_val;
  // Will swap two values per iteration
  const int half_width = (width + 1) >> 1;

  // Y
  for (wind = 0; wind < half_width; ++wind) {
   for (hind = 0; hind < height; ++hind) {
     indO = hind * src_ystride + wind;
     indS = hind * dst_ystride + (width - wind - 1);
     tmp_val = src_yplane[indO];
     dst_yplane[indO] = src_yplane[indS];
     dst_yplane[indS] = tmp_val;
    }
  }

  const int half_height = (height + 1) >> 1;
  const int half_uv_width = (width + 1) >> 1;

  for (wind = 0; wind < half_uv_width; ++wind) {
   for (hind = 0; hind < half_height; ++hind) {
     // U
     indO = hind * dst_ustride + wind;
     indS = hind * dst_ustride + (half_uv_width - wind - 1);
     tmp_val = src_uplane[indO];
     dst_uplane[indO] = src_uplane[indS];
     dst_uplane[indS] = tmp_val;
     // V
     indO = hind * dst_vstride + wind;
     indS = hind * dst_vstride + (half_uv_width - wind - 1);
     tmp_val = src_vplane[indO];
     dst_vplane[indO] = src_vplane[indS];
     dst_vplane[indS] = tmp_val;
   }
  }
  return 0;
}

// Make a center cut
int I420Crop(uint8* frame,
             int src_width, int src_height,
             int dst_width, int dst_height)
{
  if (frame == NULL)
    return -1;

  if (src_width == dst_width && src_height == dst_height) {
      // Nothing to do
    return 0;
  }
  if (dst_width > src_width || dst_height > src_height) {
      // error
      return -1;
  }
  int i = 0;
  int m = 0;
  int loop = 0;
  int half_dst_width = dst_width / 2;
  int halfdst_height = dst_height / 2;
  int halfsrc_width = src_width / 2;
  int half_dst_height= src_height / 2;
  int crop_height = ( src_height - dst_height ) / 2;
  int crop_width = ( src_width - dst_width ) / 2;

  for (i = src_width * crop_height + crop_width; loop < dst_height ;
      loop++, i += src_width) {
    memcpy(&frame[m],&frame[i],dst_width);
    m += dst_width;
  }
  i = src_width * src_height; // ilum
  loop = 0;
  for ( i += (halfsrc_width * crop_height / 2 + crop_width / 2);
        loop < halfdst_height; loop++,i += halfsrc_width) {
    memcpy(&frame[m],&frame[i],half_dst_width);
    m += half_dst_width;
  }
  loop = 0;
  i = src_width * src_height + half_dst_height * halfsrc_width; // ilum + Cr
  for ( i += (halfsrc_width * crop_height / 2 + crop_width / 2);
        loop < halfdst_height; loop++, i += halfsrc_width) {
    memcpy(&frame[m],&frame[i],half_dst_width);
    m += half_dst_width;
  }
  return 0;
}


int I420CropPad(const uint8* src_frame, int src_width,
                int src_height, uint8* dst_frame,
                int dst_width, int dst_height)
{
  if (src_width < 1 || dst_width < 1 || src_height < 1 || dst_height < 1) {
    return -1;
  }
  if (src_width == dst_width && src_height == dst_height) {
    memcpy(dst_frame, src_frame, 3 * dst_width * (dst_height >> 1));
  } else {
    if (src_height < dst_height) {
      // pad height
      int pad_height = dst_height - src_height;
      int i = 0;
      int pad_width = 0;
      int crop_width = 0;
      int width = src_width;
      if (src_width < dst_width) {
        // pad width
        pad_width = dst_width - src_width;
      } else {
        // cut width
        crop_width = src_width - dst_width;
        width = dst_width;
      }
      if (pad_height) {
        memset(dst_frame, 0, dst_width * (pad_height >> 1));
        dst_frame +=  dst_width * (pad_height >> 1);
      }
      for (i = 0; i < src_height;i++) {
        if (pad_width) {
            memset(dst_frame, 0, pad_width / 2);
            dst_frame +=  pad_width / 2;
        }
        src_frame += crop_width >> 1; // in case we have a cut
        memcpy(dst_frame,src_frame ,width);
        src_frame += crop_width >> 1;
        dst_frame += width;
        src_frame += width;
        if (pad_width) {
          memset(dst_frame, 0, pad_width / 2);
          dst_frame +=  pad_width / 2;
        }
      }
      if (pad_height) {
        memset(dst_frame, 0, dst_width * (pad_height >> 1));
        dst_frame +=  dst_width * (pad_height >> 1);
      }
      if (pad_height) {
        memset(dst_frame, 127, (dst_width >> 2) * (pad_height >> 1));
        dst_frame +=  (dst_width >> 2) * (pad_height >> 1);
      }
      for (i = 0; i < (src_height >> 1); i++) {
        if (pad_width) {
          memset(dst_frame, 127, pad_width >> 2);
          dst_frame +=  pad_width >> 2;
        }
        src_frame += crop_width >> 2; // in case we have a cut
        memcpy(dst_frame, src_frame,width >> 1);
        src_frame += crop_width >> 2;
        dst_frame += width >> 1;
        src_frame += width >> 1;
        if (pad_width) {
          memset(dst_frame, 127, pad_width >> 2);
          dst_frame +=  pad_width >> 2;
        }
      }
      if (pad_height) {
        memset(dst_frame, 127, (dst_width >> 1) * (pad_height >> 1));
        dst_frame +=  (dst_width >> 1) * (pad_height >> 1);
      }
      for (i = 0; i < (src_height >> 1); i++) {
        if (pad_width) {
          memset(dst_frame, 127, pad_width >> 2);
          dst_frame +=  pad_width >> 2;
        }
        src_frame += crop_width >> 2; // in case we have a cut
        memcpy(dst_frame, src_frame,width >> 1);
        src_frame += crop_width >> 2;
        dst_frame += width >> 1;
        src_frame += width >> 1;
        if (pad_width) {
          memset(dst_frame, 127, pad_width >> 2);
          dst_frame += pad_width >> 2;
        }
      }
      if (pad_height) {
        memset(dst_frame, 127, (dst_width >> 2) * (pad_height >> 1));
        dst_frame +=  (dst_width >> 2) * (pad_height >> 1);
      }
    } else {
      // cut height
      int i = 0;
      int pad_width = 0;
      int crop_width = 0;
      int width = src_width;

      if (src_width < dst_width) {
        // pad width
        pad_width = dst_width - src_width;
      } else {
        // cut width
        crop_width = src_width - dst_width;
        width = dst_width;
      }
      int diff_height = src_height - dst_height;
      src_frame += src_width * (diff_height >> 1);  // skip top I

      for (i = 0; i < dst_height; i++) {
        if (pad_width) {
          memset(dst_frame, 0, pad_width / 2);
          dst_frame +=  pad_width / 2;
        }
        src_frame += crop_width >> 1; // in case we have a cut
        memcpy(dst_frame,src_frame ,width);
        src_frame += crop_width >> 1;
        dst_frame += width;
        src_frame += width;
        if (pad_width) {
          memset(dst_frame, 0, pad_width / 2);
          dst_frame +=  pad_width / 2;
        }
      }
      src_frame += src_width * (diff_height >> 1);  // skip end I
      src_frame += (src_width >> 2) * (diff_height >> 1); // skip top of Cr
      for (i = 0; i < (dst_height >> 1); i++) {
        if (pad_width) {
          memset(dst_frame, 127, pad_width >> 2);
          dst_frame +=  pad_width >> 2;
        }
        src_frame += crop_width >> 2; // in case we have a cut
        memcpy(dst_frame, src_frame,width >> 1);
        src_frame += crop_width >> 2;
        dst_frame += width >> 1;
        src_frame += width >> 1;
        if (pad_width) {
          memset(dst_frame, 127, pad_width >> 2);
          dst_frame +=  pad_width >> 2;
        }
      }
      src_frame += (src_width >> 2) * (diff_height >> 1); // skip end of Cr
      src_frame += (src_width >> 2) * (diff_height >> 1); // skip top of Cb
      for (i = 0; i < (dst_height >> 1); i++) {
        if (pad_width) {
          memset(dst_frame, 127, pad_width >> 2);
          dst_frame +=  pad_width >> 2;
        }
        src_frame += crop_width >> 2; // in case we have a cut
        memcpy(dst_frame, src_frame, width >> 1);
        src_frame += crop_width >> 2;
        dst_frame += width >> 1;
        src_frame += width >> 1;
        if (pad_width) {
          memset(dst_frame, 127, pad_width >> 2);
          dst_frame +=  pad_width >> 2;
        }
      }
    }
  }
  return 0;
}

} // namespace libyuv

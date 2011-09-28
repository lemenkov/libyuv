/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "general.h"

#include <assert.h>
#include <string.h>     // memcpy(), memset()

#include "video_common.h"

namespace libyuv {


int
MirrorI420LeftRight( const uint8* src_frame,uint8* dst_frame,
                     int src_width, int src_height)
{
    if (src_width < 1 || src_height < 1)
    {
        return -1;
    }

    assert(src_width % 2 == 0 &&  src_height % 2 == 0);

    int indO = 0;
    int indS  = 0;
    int wind, hind;
    uint8 tmpVal;
    // Will swap two values per iteration
    const int halfW = src_width >> 1;
    const int halfStride = src_width >> 1;
    // Y
    for (wind = 0; wind < halfW; wind++ )
    {
        for (hind = 0; hind < src_height; hind++ )
        {
            indO = hind * src_width + wind;
            indS = hind * src_width + (src_width - wind - 1); // swapping index
            tmpVal = src_frame[indO];
            dst_frame[indO] = src_frame[indS];
            dst_frame[indS] = tmpVal;
        } // end for (height)
    } // end for(width)
    const int lengthW = src_width >> 2;
    const int lengthH = src_height >> 1;
    // V
    int zeroInd = src_width * src_height;
    for (wind = 0; wind < lengthW; wind++ )
    {
        for (hind = 0; hind < lengthH; hind++ )
        {
            indO = zeroInd + hind * halfW + wind;
            indS = zeroInd + hind * halfW + (halfW - wind - 1);// swapping index
            tmpVal = src_frame[indO];
            dst_frame[indO] = src_frame[indS];
            dst_frame[indS] = tmpVal;
        } // end for (height)
    } // end for(width)

    // U
    zeroInd += src_width * src_height >> 2;
    for (wind = 0; wind < lengthW; wind++ )
    {
        for (hind = 0; hind < lengthH; hind++ )
        {
            indO = zeroInd + hind * halfW + wind;
            indS = zeroInd + hind * halfW + (halfW - wind - 1);// swapping index
            tmpVal = src_frame[indO];
            dst_frame[indO] = src_frame[indS];
            dst_frame[indS] = tmpVal;
        } // end for (height)
    } // end for(width)

    return 0;
}


// Make a center cut
int
CutI420Frame(uint8* frame,
             int fromWidth, int fromHeight,
             int toWidth, int toHeight)
{
    if (toWidth < 1 || fromWidth < 1 || toHeight < 1 || fromHeight < 1 )
    {
        return -1;
    }
    if (toWidth == fromWidth && toHeight == fromHeight)
    {
        // Nothing to do
      return 3 * toHeight * toWidth / 2;
    }
    if (toWidth > fromWidth || toHeight > fromHeight)
    {
        // error
        return -1;
    }
    int i = 0;
    int m = 0;
    int loop = 0;
    int halfToWidth = toWidth / 2;
    int halfToHeight = toHeight / 2;
    int halfFromWidth = fromWidth / 2;
    int halfFromHeight= fromHeight / 2;
    int cutHeight = ( fromHeight - toHeight ) / 2;
    int cutWidth = ( fromWidth - toWidth ) / 2;

    for (i = fromWidth * cutHeight + cutWidth; loop < toHeight ;
        loop++, i += fromWidth)
    {
        memcpy(&frame[m],&frame[i],toWidth);
        m += toWidth;
    }
    i = fromWidth * fromHeight; // ilum
    loop = 0;
    for ( i += (halfFromWidth * cutHeight / 2 + cutWidth / 2);
          loop < halfToHeight; loop++,i += halfFromWidth)
    {
        memcpy(&frame[m],&frame[i],halfToWidth);
        m += halfToWidth;
    }
    loop = 0;
    i = fromWidth * fromHeight + halfFromHeight * halfFromWidth; // ilum + Cr
    for ( i += (halfFromWidth * cutHeight / 2 + cutWidth / 2);
          loop < halfToHeight; loop++, i += halfFromWidth)
    {
        memcpy(&frame[m],&frame[i],halfToWidth);
        m += halfToWidth;
    }
    return halfToWidth * toHeight * 3;
}

int
CutPadI420Frame(const uint8* inFrame, int inWidth,
                int inHeight, uint8* outFrame,
                int outWidth, int outHeight)
{
    if (inWidth < 1 || outWidth < 1 || inHeight < 1 || outHeight < 1 )
    {
        return -1;
    }
    if (inWidth == outWidth && inHeight == outHeight)
    {
        memcpy(outFrame, inFrame, 3 * outWidth * (outHeight >> 1));
    }
    else
    {
        if ( inHeight < outHeight)
        {
            // pad height
            int padH = outHeight - inHeight;
            int i = 0;
            int padW = 0;
            int cutW = 0;
            int width = inWidth;
            if (inWidth < outWidth)
            {
                // pad width
                padW = outWidth - inWidth;
            }
            else
            {
              // cut width
              cutW = inWidth - outWidth;
              width = outWidth;
            }
            if (padH)
            {
                memset(outFrame, 0, outWidth * (padH >> 1));
                outFrame +=  outWidth * (padH >> 1);
            }
            for (i = 0; i < inHeight;i++)
            {
                if (padW)
                {
                    memset(outFrame, 0, padW / 2);
                    outFrame +=  padW / 2;
                }
                inFrame += cutW >> 1; // in case we have a cut
                memcpy(outFrame,inFrame ,width);
                inFrame += cutW >> 1;
                outFrame += width;
                inFrame += width;
                if (padW)
                {
                    memset(outFrame, 0, padW / 2);
                    outFrame +=  padW / 2;
                }
            }
            if (padH)
            {
                memset(outFrame, 0, outWidth * (padH >> 1));
                outFrame +=  outWidth * (padH >> 1);
            }
            if (padH)
            {
                memset(outFrame, 127, (outWidth >> 2) * (padH >> 1));
                outFrame +=  (outWidth >> 2) * (padH >> 1);
            }
            for (i = 0; i < (inHeight >> 1); i++)
            {
                if (padW)
                {
                    memset(outFrame, 127, padW >> 2);
                    outFrame +=  padW >> 2;
                }
                inFrame += cutW >> 2; // in case we have a cut
                memcpy(outFrame, inFrame,width >> 1);
                inFrame += cutW >> 2;
                outFrame += width >> 1;
                inFrame += width >> 1;
                if (padW)
                {
                    memset(outFrame, 127, padW >> 2);
                    outFrame +=  padW >> 2;
                }
            }
            if (padH)
            {
                memset(outFrame, 127, (outWidth >> 1) * (padH >> 1));
                outFrame +=  (outWidth >> 1) * (padH >> 1);
            }
            for (i = 0; i < (inHeight >> 1); i++)
            {
                if (padW)
                {
                    memset(outFrame, 127, padW >> 2);
                    outFrame +=  padW >> 2;
                }
                inFrame += cutW >> 2; // in case we have a cut
                memcpy(outFrame, inFrame,width >> 1);
                inFrame += cutW >> 2;
                outFrame += width >> 1;
                inFrame += width >> 1;
                if (padW)
                {
                    memset(outFrame, 127, padW >> 2);
                    outFrame += padW >> 2;
                }
            }
            if (padH)
            {
                memset(outFrame, 127, (outWidth >> 2) * (padH >> 1));
                outFrame +=  (outWidth >> 2) * (padH >> 1);
            }
        }
        else
        {
            // cut height
            int i = 0;
            int padW = 0;
            int cutW = 0;
            int width = inWidth;

            if (inWidth < outWidth)
            {
                // pad width
                padW = outWidth - inWidth;
            } else
            {
                // cut width
                cutW = inWidth - outWidth;
                width = outWidth;
            }
            int diffH = inHeight - outHeight;
            inFrame += inWidth * (diffH >> 1);  // skip top I

            for (i = 0; i < outHeight; i++)
            {
                if (padW)
                {
                    memset(outFrame, 0, padW / 2);
                    outFrame +=  padW / 2;
                }
                inFrame += cutW >> 1; // in case we have a cut
                memcpy(outFrame,inFrame ,width);
                inFrame += cutW >> 1;
                outFrame += width;
                inFrame += width;
                if (padW)
                {
                    memset(outFrame, 0, padW / 2);
                    outFrame +=  padW / 2;
                }
            }
            inFrame += inWidth * (diffH >> 1);  // skip end I
            inFrame += (inWidth >> 2) * (diffH >> 1); // skip top of Cr
            for (i = 0; i < (outHeight >> 1); i++)
            {
                if (padW)
                {
                    memset(outFrame, 127, padW >> 2);
                    outFrame +=  padW >> 2;
                }
                inFrame += cutW >> 2; // in case we have a cut
                memcpy(outFrame, inFrame,width >> 1);
                inFrame += cutW >> 2;
                outFrame += width >> 1;
                inFrame += width >> 1;
                if (padW)
                {
                    memset(outFrame, 127, padW >> 2);
                    outFrame +=  padW >> 2;
                }
            }
            inFrame += (inWidth >> 2) * (diffH >> 1); // skip end of Cr
            inFrame += (inWidth >> 2) * (diffH >> 1); // skip top of Cb
            for (i = 0; i < (outHeight >> 1); i++)
            {
                if (padW)
                {
                    memset(outFrame, 127, padW >> 2);
                    outFrame +=  padW >> 2;
                }
                inFrame += cutW >> 2; // in case we have a cut
                memcpy(outFrame, inFrame, width >> 1);
                inFrame += cutW >> 2;
                outFrame += width >> 1;
                inFrame += width >> 1;
                if (padW)
                {
                    memset(outFrame, 127, padW >> 2);
                    outFrame +=  padW >> 2;
                }
            }
        }
    }
    return 3 * outWidth * (outHeight >> 1);
}

} // nmaespace libyuv

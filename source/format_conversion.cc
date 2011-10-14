/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>

#include "libyuv/cpu_id.h"
#include "video_common.h"

namespace libyuv {

// Most code in here is inspired by the material at
// http://www.siliconimaging.com/RGB%20Bayer.htm

// Forces compiler to inline, even against its better judgement. Use wisely.
#if defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline))
#elif defined(WIN32)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE
#endif

enum {
  RED = 0,
  BLUE = 1,
  GREEN_BETWEEN_RED = 2,
  GREEN_BETWEEN_BLUE = 3,
};

enum Position {
  LEFT = 0,
  RIGHT = 1,
  TOP = 2,
  BOTTOM = 4,
  CENTER = 6,

  // Due to the choice of the above values, these are all distinct and the
  // corner values and edge values are each contiguous. This allows us to
  // figure out the position type of a pixel with a single addition operation
  // using the above values, rather than having to use a 3x3 nested switch
  // statement.
  TOP_LEFT = TOP + LEFT,          // 2
  TOP_RIGHT = TOP + RIGHT,        // 3
  BOTTOM_LEFT = BOTTOM + LEFT,    // 4
  BOTTOM_RIGHT = BOTTOM + RIGHT,  // 5
  LEFT_EDGE = CENTER + LEFT,      // 6
  RIGHT_EDGE = CENTER + RIGHT,    // 7
  TOP_EDGE = TOP + CENTER,        // 8
  BOTTOM_EDGE = BOTTOM + CENTER,  // 10
  MIDDLE = CENTER + CENTER,       // 12
};

static FORCE_INLINE Position GetPosition(int x, int y, int width, int height) {
  Position xpos = CENTER;
  Position ypos = CENTER;
  if (x == 0) {
    xpos = LEFT;
  } else if (x == width - 1) {
    xpos = RIGHT;
  }
  if (y == 0) {
    ypos = TOP;
  } else if (y == height - 1) {
    ypos = BOTTOM;
  }
  return static_cast<Position>(xpos + ypos);
}

static FORCE_INLINE bool IsRedBlue(uint8 colour) {
  return colour <= BLUE;
}

static FORCE_INLINE uint32 FourCcToBayerPixelColourMap(uint32 fourcc) {
  // The colour map is a 4-byte array-as-uint32 containing the colours for the
  // four pixels in each 2x2 grid, in left-to-right and top-to-bottom order.
  switch (fourcc) {
    default:
      assert(false);
    case FOURCC_RGGB:
      return FOURCC(RED, GREEN_BETWEEN_RED, GREEN_BETWEEN_BLUE, BLUE);
    case FOURCC_BGGR:
      return FOURCC(BLUE, GREEN_BETWEEN_BLUE, GREEN_BETWEEN_RED, RED);
    case FOURCC_GRBG:
      return FOURCC(GREEN_BETWEEN_RED, RED, BLUE, GREEN_BETWEEN_BLUE);
    case FOURCC_GBRG:
      return FOURCC(GREEN_BETWEEN_BLUE, BLUE, RED, GREEN_BETWEEN_RED);
  }
}

static FORCE_INLINE void RGBToYUV(uint8 r, uint8 g, uint8 b,
                                  uint8* y, uint8* u, uint8* v) {
  // Taken from http://en.wikipedia.org/wiki/YUV
  *y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
  *u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
  *v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
}

static FORCE_INLINE void InterpolateBayerRGBCorner(uint8* r,
                                                   uint8* g,
                                                   uint8* b,
                                                   const uint8* src,
                                                   int src_stride,
                                                   Position pos,
                                                   uint8 colour) {

  // Compute the offsets to use for fetching the adjacent pixels.

  int adjacent_row;
  int adjacent_column;
  switch (pos) {
    case TOP_LEFT:
      adjacent_row = src_stride;
      adjacent_column = 1;
      break;
    case TOP_RIGHT:
      adjacent_row = src_stride;
      adjacent_column = -1;
      break;
    case BOTTOM_LEFT:
      adjacent_row = -src_stride;
      adjacent_column = 1;
      break;
    case BOTTOM_RIGHT:
    default:
      adjacent_row = -src_stride;
      adjacent_column = -1;
      break;
  }

  // Now interpolate.

  if (IsRedBlue(colour)) {
    uint8 current_pixel = src[0];
    // Average of the adjacent green pixels (there's only two).
    *g = (src[adjacent_column] + src[adjacent_row]) / 2;
    // Average of the oppositely-coloured corner pixels (there's only one).
    uint8 corner_average = src[adjacent_row + adjacent_column];
    if (colour == RED) {
      *r = current_pixel;
      *b = corner_average;
    } else {  // i.e., BLUE
      *b = current_pixel;
      *r = corner_average;
    }
  } else {  // i.e., GREEN_BETWEEN_*
    *g = src[0];
    // Average of the adjacent same-row pixels (there's only one).
    uint8 row_average = src[adjacent_column];
    // Average of the adjacent same-column pixels (there's only one).
    uint8 column_average = src[adjacent_row];
    if (colour == GREEN_BETWEEN_RED) {
      *r = row_average;
      *b = column_average;
    } else {  // i.e., GREEN_BETWEEN_BLUE
      *b = row_average;
      *r = column_average;
    }
  }
}

static FORCE_INLINE void InterpolateBayerRGBEdge(uint8* r,
                                                 uint8* g,
                                                 uint8* b,
                                                 const uint8* src,
                                                 int src_stride,
                                                 Position pos,
                                                 uint8 colour) {

  // Compute the offsets to use for fetching the adjacent pixels.

  // Goes one pixel "in" to the image (i.e. towards the center)
  int inner;
  // Goes one pixel to the side (i.e. along the edge) in either the clockwise or
  // counter-clockwise direction, and its negative value goes in the other
  // direction.
  int side;

  switch (pos) {
    case TOP_EDGE:
      inner = src_stride;
      side = 1;
      break;
    case RIGHT_EDGE:
      inner = -1;
      side = src_stride;
      break;
    case BOTTOM_EDGE:
      inner = -src_stride;
      side = 1;
      break;
    case LEFT_EDGE:
    default:
      inner = 1;
      side = src_stride;
      break;
  }

  // Now interpolate.

  if (IsRedBlue(colour)) {
    uint8 current_pixel = src[0];
    // Average of the adjacent green pixels (there's only three).
    *g = (src[inner] + src[side] + src[-side]) / 3;
    // Average of the oppositely-coloured corner pixels (there's only two).
    uint8 corner_average = (src[inner + side] + src[inner - side]) / 2;
    if (colour == RED) {
      *r = current_pixel;
      *b = corner_average;
    } else {  // i.e., BLUE
      *b = current_pixel;
      *r = corner_average;
    }
  } else {  // i.e., GREEN_BETWEEN_*
    *g = src[0];
    // Average of the adjacent side-ways pixels (there's only two).
    uint8 side_average = (src[side] + src[-side]) / 2;
    // Average of the adjacent inner-ways pixels (there's only one).
    uint8 inner_pixel = src[inner];
    // Including && side == 1 effectively transposes the colour logic for
    // processing the left/right sides, which is needed since the "T" shape
    // formed by the pixels is transposed.
    if (colour == GREEN_BETWEEN_RED && side == 1) {
      *r = side_average;
      *b = inner_pixel;
    } else {  // i.e., GREEN_BETWEEN_BLUE || side != 1
      *b = side_average;
      *r = inner_pixel;
    }
  }
}

// We inline this one because it runs 99% of the time, so inlining it is
// probably beneficial.
static FORCE_INLINE void InterpolateBayerRGBCenter(uint8* r,
                                                   uint8* g,
                                                   uint8* b,
                                                   const uint8* src,
                                                   int src_stride,
                                                   uint8 colour) {

  if (IsRedBlue(colour)) {
    uint8 current_pixel = src[0];
    // Average of the adjacent green pixels (there's four).
    // NOTE(tschmelcher): The material at
    // http://www.siliconimaging.com/RGB%20Bayer.htm discusses a way to improve
    // quality here by using only two of the green pixels based on the
    // correlation to the nearby red/blue pixels, but that is slower and would
    // result in more edge cases.
    *g = (src[1] + src[-1] + src[src_stride] + src[-src_stride]) / 4;
    // Average of the oppositely-coloured corner pixels (there's four).
    uint8 corner_average = (src[src_stride + 1] +
                            src[src_stride - 1] +
                            src[-src_stride + 1] +
                            src[-src_stride - 1]) / 4;
    if (colour == RED) {
      *r = current_pixel;
      *b = corner_average;
    } else {  // i.e., BLUE
      *b = current_pixel;
      *r = corner_average;
    }
  } else {  // i.e., GREEN_BETWEEN_*
    *g = src[0];
    // Average of the adjacent same-row pixels (there's two).
    uint8 row_adjacent = (src[1] + src[-1]) / 2;
    // Average of the adjacent same-column pixels (there's two).
    uint8 column_adjacent = (src[src_stride] + src[-src_stride]) / 2;
    if (colour == GREEN_BETWEEN_RED) {
      *r = row_adjacent;
      *b = column_adjacent;
    } else {  // i.e., GREEN_BETWEEN_BLUE
      *b = row_adjacent;
      *r = column_adjacent;
    }
  }
}

// Converts any Bayer RGB format to ARGB.
int BayerRGBToARGB(const uint8* src, int src_stride, uint32 src_fourcc,
                   uint8* dst, int dst_stride,
                   int width, int height) {
  assert(width % 2 == 0);
  assert(height % 2 == 0);

  uint32 colour_map = FourCcToBayerPixelColourMap(src_fourcc);
  int src_row_inc = src_stride * 2 - width;
  int dst_row_inc = dst_stride * 2 - width * 4;

  // Iterate over the 2x2 grids.
  for (int y1 = 0; y1 < height; y1 += 2) {
    for (int x1 = 0; x1 < width; x1 += 2) {
      uint32 colours = colour_map;
      // Iterate over the four pixels within them.
      for (int y2 = 0; y2 < 2; ++y2) {
        for (int x2 = 0; x2 < 2; ++x2) {
          uint8 r, g, b;
          // The low-order byte of the colour map is the current colour.
          uint8 current_colour = static_cast<uint8>(colours);
          colours >>= 8;
          Position pos = GetPosition(x1 + x2, y1 + y2, width, height);
          const uint8* src_pixel = &src[y2 * src_stride + x2];
          uint8* dst_pixel = &dst[y2 * dst_stride + x2 * 4];

          // Convert from Bayer RGB to regular RGB.
          if (pos == MIDDLE) {
            // 99% of the image is the middle.
            InterpolateBayerRGBCenter(&r, &g, &b,
                                      src_pixel, src_stride,
                                      current_colour);
          } else if (pos >= LEFT_EDGE) {
            // Next most frequent is edges.
            InterpolateBayerRGBEdge(&r, &g, &b,
                                    src_pixel, src_stride, pos,
                                    current_colour);
          } else {
            // Last is the corners. There are only 4.
            InterpolateBayerRGBCorner(&r, &g, &b,
                                      src_pixel, src_stride, pos,
                                      current_colour);
          }

          // Store ARGB
          dst_pixel[0] = b;
          dst_pixel[1] = g;
          dst_pixel[2] = r;
          dst_pixel[3] = 255u;
        }
      }
      src += 2;
      dst += 2 * 4;
    }
    src += src_row_inc;
    dst += dst_row_inc;
  }
  return 0;
}

// Converts any Bayer RGB format to I420.
int BayerRGBToI420(const uint8* src, int src_stride, uint32 src_fourcc,
                   uint8* y, int y_stride,
                   uint8* u, int u_stride,
                   uint8* v, int v_stride,
                   int width, int height) {
  assert(width % 2 == 0);
  assert(height % 2 == 0);

  uint32 colour_map = FourCcToBayerPixelColourMap(src_fourcc);

  int src_row_inc = src_stride * 2 - width;
  int y_row_inc = y_stride * 2 - width;
  int u_row_inc = u_stride - width / 2;
  int v_row_inc = v_stride - width / 2;

  // Iterate over the 2x2 grids.
  for (int y1 = 0; y1 < height; y1 += 2) {
    for (int x1 = 0; x1 < width; x1 += 2) {
      uint32 colours = colour_map;
      int total_u = 0;
      int total_v = 0;
      // Iterate over the four pixels within them.
      for (int y2 = 0; y2 < 2; ++y2) {
        for (int x2 = 0; x2 < 2; ++x2) {
          uint8 r, g, b;
          // The low-order byte of the colour map is the current colour.
          uint8 current_colour = static_cast<uint8>(colours);
          colours >>= 8;
          Position pos = GetPosition(x1 + x2, y1 + y2, width, height);
          const uint8* src_pixel = &src[y2 * src_stride + x2];
          uint8* y_pixel = &y[y2 * y_stride + x2];

          // Convert from Bayer RGB to regular RGB.

          if (pos == MIDDLE) {
            // 99% of the image is the middle.
            InterpolateBayerRGBCenter(&r, &g, &b,
                                      src_pixel, src_stride,
                                      current_colour);
          } else if (pos >= LEFT_EDGE) {
            // Next most frequent is edges.
            InterpolateBayerRGBEdge(&r, &g, &b,
                                    src_pixel, src_stride, pos,
                                    current_colour);
          } else {
            // Last is the corners. There are only 4.
            InterpolateBayerRGBCorner(&r, &g, &b,
                                      src_pixel, src_stride, pos,
                                      current_colour);
          }

          // Convert from RGB to YUV.

          uint8 tmp_u, tmp_v;
          RGBToYUV(r, g, b, y_pixel, &tmp_u, &tmp_v);
          total_u += tmp_u;
          total_v += tmp_v;
        }
      }
      src += 2;
      y += 2;
      *u = total_u / 4;
      *v = total_v / 4;
      ++u;
      ++v;
    }
    src += src_row_inc;
    y += y_row_inc;
    u += u_row_inc;
    v += v_row_inc;
  }
  return 0;
}

// Note: to do this with Neon vld4.8 would load ARGB values into 4 registers
// and vst would select which 2 components to write.  The low level would need
// to be ARGBToBG, ARGBToGB, ARGBToRG, ARGBToGR

#if defined(WIN32) && !defined(COVERAGE_ENABLED)
#define HAS_ARGBTOBAYERROW_SSSE3
__declspec(naked)
static void ARGBToBayerRow_SSSE3(const uint8* src_argb,
                                 uint8* dst_bayer, uint32 selector, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_argb
    mov        edx, [esp + 8]    // dst_bayer
    movd       xmm0, [esp + 12]  // selector
    mov        ecx, [esp + 16]   // pix
    pshufd     xmm0, xmm0, 0

  wloop:
    movdqa     xmm1, [eax]
    lea        eax, [eax + 16]
    pshufb     xmm1, xmm0
    movd       [edx], xmm1
    lea        edx, [edx + 4]
    sub        ecx, 4
    ja         wloop
    ret
  }
}

#elif defined(__i386__) && !defined(COVERAGE_ENABLED) && \
    !TARGET_IPHONE_SIMULATOR

#define HAS_ARGBTOBAYERROW_SSSE3
extern "C" void ARGBToBayerRow_SSSE3(const uint8* src_argb, uint8* dst_bayer,
                                     uint32 selector, int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ARGBToBayerRow_SSSE3\n"
"_ARGBToBayerRow_SSSE3:\n"
#else
    ".global ARGBToBayerRow_SSSE3\n"
"ARGBToBayerRow_SSSE3:\n"
#endif
    "mov    0x4(%esp),%eax\n"
    "mov    0x8(%esp),%edx\n"
    "movd   0xc(%esp),%xmm0\n"
    "mov    0x10(%esp),%ecx\n"
    "pshufd $0x0,%xmm0,%xmm0\n"

"1:"
    "movdqa (%eax),%xmm1\n"
    "lea    0x10(%eax),%eax\n"
    "pshufb %xmm0,%xmm1\n"
    "movd   %xmm1,(%edx)\n"
    "lea    0x4(%edx),%edx\n"
    "sub    $0x4,%ecx\n"
    "ja     1b\n"
    "ret\n"
);
#endif

static void ARGBToBayerRow_C(const uint8* src_argb,
                             uint8* dst_bayer, uint32 selector, int pix) {
  int index0 = selector & 0xff;
  int index1 = (selector >> 8) & 0xff;
  // Copy a row of Bayer.
  for (int x = 0; x < pix; x += 2) {
    dst_bayer[0] = src_argb[index0];
    dst_bayer[1] = src_argb[index1];
    src_argb += 8;
    dst_bayer += 2;
  }
}

// generate a selector mask useful for pshufb
static uint32 GenerateSelector(int select0, int select1) {
  return static_cast<uint32>(select0) |
         static_cast<uint32>((select1 + 4) << 8) |
         static_cast<uint32>((select0 + 8) << 16) |
         static_cast<uint32>((select1 + 12) << 24);
}

// Converts 32 bit ARGB to any Bayer RGB format.
int ARGBToBayerRGB(const uint8* src_rgb, int src_stride_rgb,
                   uint8* dst_bayer, int dst_stride_bayer,
                   uint32 dst_fourcc_bayer,
                   int width, int height) {
  assert(width % 2 == 0);
  void (*ARGBToBayerRow)(const uint8* src_argb,
                         uint8* dst_bayer, uint32 selector, int pix);
#if defined(HAS_ARGBTOBAYERROW_SSSE3)
  if (libyuv::TestCpuFlag(libyuv::kCpuHasSSSE3) &&
      (width % 4 == 0) &&
      IS_ALIGNED(src_rgb, 16) && (src_stride_rgb % 16 == 0) &&
      IS_ALIGNED(dst_bayer, 4) && (dst_stride_bayer % 4 == 0)) {
    ARGBToBayerRow = ARGBToBayerRow_SSSE3;
  } else
#endif
  {
    ARGBToBayerRow = ARGBToBayerRow_C;
  }

  int blue_index = 0;
  int green_index = 1;
  int red_index = 2;

  // Now build a lookup table containing the indices for the four pixels in each
  // 2x2 Bayer grid.
  uint32 index_map[2];
  switch (dst_fourcc_bayer) {
    default:
      assert(false);
    case FOURCC_RGGB:
      index_map[0] = GenerateSelector(red_index, green_index);
      index_map[1] = GenerateSelector(green_index, blue_index);
      break;
    case FOURCC_BGGR:
      index_map[0] = GenerateSelector(blue_index, green_index);
      index_map[1] = GenerateSelector(green_index, red_index);
      break;
    case FOURCC_GRBG:
      index_map[0] = GenerateSelector(green_index, red_index);
      index_map[1] = GenerateSelector(blue_index, green_index);
      break;
    case FOURCC_GBRG:
      index_map[0] = GenerateSelector(green_index, blue_index);
      index_map[1] = GenerateSelector(red_index, green_index);
      break;
  }

  // Now convert.
  for (int y = 0; y < height; ++y) {
    ARGBToBayerRow(src_rgb, dst_bayer, index_map[y & 1], width);
    src_rgb += src_stride_rgb;
    dst_bayer += dst_stride_bayer;
  }
  return 0;
}

}  // namespace libyuv

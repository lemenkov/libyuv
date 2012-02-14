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
#include <emmintrin.h>  // Not currently used
#endif

#include "libyuv/basic_types.h"
#include "libyuv/cpu_id.h"
#include "libyuv/format_conversion.h"
#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"
#include "libyuv/video_common.h"
#include "row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// YUY2 - Macro-pixel = 2 image pixels
// Y0U0Y1V0....Y2U2Y3V2...Y4U4Y5V4....

#if defined(_M_IX86) && !defined(YUV_DISABLE_ASM)
#define HAS_I42XTOYUY2ROW_SSE2
__declspec(naked)
static void I42xToYUY2Row_SSE2(const uint8* src_y,
                               const uint8* src_u,
                               const uint8* src_v,
                               uint8* dst_frame, int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    // src_y
    mov        esi, [esp + 8 + 8]    // src_u
    mov        edx, [esp + 8 + 12]   // src_v
    mov        edi, [esp + 8 + 16]   // dst_frame
    mov        ecx, [esp + 8 + 20]   // width
    sub        edx, esi

  convertloop:
    movdqa     xmm0, [eax] // Y
    lea        eax, [eax + 16]
    movq       xmm2, qword ptr [esi] // U
    movq       xmm3, qword ptr [esi + edx] // V
    lea        esi, [esi + 8]
    punpcklbw  xmm2, xmm3 // UV
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm2 // YUYV
    punpckhbw  xmm1, xmm2
    movdqa     [edi], xmm0
    movdqa     [edi + 16], xmm1
    lea        edi, [edi + 32]
    sub        ecx, 16
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}
#elif (defined(__x86_64__) || defined(__i386__)) && !defined(YUV_DISABLE_ASM)
#define HAS_I42XTOYUY2ROW_SSE2
static void I42xToYUY2Row_SSE2(const uint8* src_y,
                               const uint8* src_u,
                               const uint8* src_v,
                               uint8* dst_frame, int width) {
 asm volatile (
  "sub        %1,%2                            \n"
"1:                                            \n"
  "movdqa    (%0),%%xmm0                       \n"
  "lea       0x10(%0),%0                       \n"
  "movq      (%1),%%xmm2                       \n"
  "movq      (%1,%2,1),%%xmm3                  \n"
  "lea       0x8(%1),%1                        \n"
  "punpcklbw %%xmm3,%%xmm2                     \n"
  "movdqa    %%xmm0,%%xmm1                     \n"
  "punpcklbw %%xmm2,%%xmm0                     \n"
  "punpckhbw %%xmm2,%%xmm1                     \n"
  "movdqa    %%xmm0,(%3)                       \n"
  "movdqa    %%xmm1,0x10(%3)                   \n"
  "lea       0x20(%3),%3                       \n"
  "sub       $0x10,%4                          \n"
  "ja         1b                               \n"
  : "+r"(src_y),  // %0
    "+r"(src_u),  // %1
    "+r"(src_v),  // %2
    "+r"(dst_frame),  // %3
    "+rm"(width)  // %4
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
);
}
#endif

void I42xToYUY2Row_C(const uint8* src_y, const uint8* src_u, const uint8* src_v,
                     uint8* dst_frame, int width) {
    for (int x = 0; x < width - 1; x += 2) {
      dst_frame[0] = src_y[0];
      dst_frame[1] = src_u[0];
      dst_frame[2] = src_y[1];
      dst_frame[3] = src_v[0];
      dst_frame += 4;
      src_y += 2;
      src_u += 1;
      src_v += 1;
    }
    if (width & 1) {
      dst_frame[0] = src_y[0];
      dst_frame[1] = src_u[0];
      dst_frame[2] = src_y[0];  // duplicate last y
      dst_frame[3] = src_v[0];
    }
}

int I422ToYUY2(const uint8* src_y, int src_stride_y,
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
    dst_frame = dst_frame + (height - 1) * dst_stride_frame;
    dst_stride_frame = -dst_stride_frame;
  }
  void (*I42xToYUY2Row)(const uint8* src_y, const uint8* src_u,
                        const uint8* src_v, uint8* dst_frame, int width);
#if defined(HAS_I42XTOYUY2ROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(width, 16) &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_frame, 16) && IS_ALIGNED(dst_stride_frame, 16)) {
    I42xToYUY2Row = I42xToYUY2Row_SSE2;
  } else
#endif
  {
    I42xToYUY2Row = I42xToYUY2Row_C;
  }
  for (int y = 0; y < height; ++y) {
    I42xToYUY2Row(src_y, src_u, src_y, dst_frame, width);
    src_y += src_stride_y;
    src_u += src_stride_u;
    src_v += src_stride_v;
    dst_frame += dst_stride_frame;
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
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_frame = dst_frame + (height - 1) * dst_stride_frame;
    dst_stride_frame = -dst_stride_frame;
  }
  void (*I42xToYUY2Row)(const uint8* src_y, const uint8* src_u,
                        const uint8* src_v, uint8* dst_frame, int width);
#if defined(HAS_I42XTOYUY2ROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(width, 16) &&
      IS_ALIGNED(src_y, 16) && IS_ALIGNED(src_stride_y, 16) &&
      IS_ALIGNED(dst_frame, 16) && IS_ALIGNED(dst_stride_frame, 16)) {
    I42xToYUY2Row = I42xToYUY2Row_SSE2;
  } else
#endif
  {
    I42xToYUY2Row = I42xToYUY2Row_C;
  }
  for (int y = 0; y < height - 1; y += 2) {
    I42xToYUY2Row(src_y, src_u, src_v, dst_frame, width);
    I42xToYUY2Row(src_y + src_stride_y, src_u, src_v,
                  dst_frame + dst_stride_frame, width);
    src_y += src_stride_y * 2;
    src_u += src_stride_u;
    src_v += src_stride_v;
    dst_frame += dst_stride_frame * 2;
  }
  if (height & 1) {
    I42xToYUY2Row(src_y, src_u, src_v, dst_frame, width);
  }
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

// Test if over reading on source is safe.
// TODO(fbarchard): Find more efficient solution to safely do odd sizes.
// Macros to control read policy, from slowest to fastest:
// READSAFE_NEVER - disables read ahead on systems with strict memory reads
// READSAFE_ODDHEIGHT - last row of odd height done with C.
//   This policy assumes that the caller handles the last row of an odd height
//   image using C.
// READSAFE_PAGE - enable read ahead within same page.
//   A page is 4096 bytes.  When reading ahead, if the last pixel is near the
//   end the page, and a read spans the page into the next page, a memory
//   exception can occur if that page has not been allocated, or is a guard
//   page.  This setting ensures the overread is within the same page.
// READSAFE_ALWAYS - enables read ahead on systems without memory exceptions
//   or where buffers are padded by 64 bytes.

#define READSAFE_ODDHEIGHT

static bool TestReadSafe(const uint8* src_yuy2, int src_stride_yuy2,
                        int width, int height, int bpp, int overread) {
  if (width > kMaxStride) {
    return false;
  }
#if defined(READSAFE_ALWAYS)
  return true;
#elif defined(READSAFE_NEVER)
  return false;
#elif defined(READSAFE_ODDHEIGHT)
  if (!(width & 15) ||
      (src_stride_yuy2 >= 0 && (height & 1) && width * bpp >= overread)) {
    return true;
  }
  return false;
#elif defined(READSAFE_PAGE)
  if (src_stride_yuy2 >= 0) {
    src_yuy2 += (height - 1) * src_stride_yuy2;
  }
  uintptr_t last_adr = (uintptr_t)(src_yuy2) + width * bpp - 1;
  uintptr_t last_read_adr = last_adr + overread - 1;
  if (((last_adr ^ last_read_adr) & ~4095) == 0) {
    return true;
  }
  return false;
#endif
}

// Convert YUY2 to I420.
int YUY2ToI420(const uint8* src_yuy2, int src_stride_yuy2,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_yuy2 = src_yuy2 + (height - 1) * src_stride_yuy2;
    src_stride_yuy2 = -src_stride_yuy2;
  }
  void (*YUY2ToUVRow)(const uint8* src_yuy2, int src_stride_yuy2,
                      uint8* dst_u, uint8* dst_v, int pix);
  void (*YUY2ToYRow)(const uint8* src_yuy2,
                     uint8* dst_y, int pix);
  YUY2ToYRow = YUY2ToYRow_C;
  YUY2ToUVRow = YUY2ToUVRow_C;
#if defined(HAS_YUY2TOYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    if (width > 16) {
      YUY2ToUVRow = YUY2ToUVRow_Any_SSE2;
      YUY2ToYRow = YUY2ToYRow_Any_SSE2;
    }
    if (IS_ALIGNED(width, 16)) {
      YUY2ToUVRow = YUY2ToUVRow_Unaligned_SSE2;
      YUY2ToYRow = YUY2ToYRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_yuy2, 16) && IS_ALIGNED(src_stride_yuy2, 16)) {
        YUY2ToUVRow = YUY2ToUVRow_SSE2;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          YUY2ToYRow = YUY2ToYRow_SSE2;
        }
      }
    }
  }
#endif
  for (int y = 0; y < height - 1; y += 2) {
    YUY2ToUVRow(src_yuy2, src_stride_yuy2, dst_u, dst_v, width);
    YUY2ToYRow(src_yuy2, dst_y, width);
    YUY2ToYRow(src_yuy2 + src_stride_yuy2, dst_y + dst_stride_y, width);
    src_yuy2 += src_stride_yuy2 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    YUY2ToUVRow(src_yuy2, 0, dst_u, dst_v, width);
    YUY2ToYRow(src_yuy2, dst_y, width);
  }
  return 0;
}

// Convert UYVY to I420.
int UYVYToI420(const uint8* src_uyvy, int src_stride_uyvy,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_uyvy = src_uyvy + (height - 1) * src_stride_uyvy;
    src_stride_uyvy = -src_stride_uyvy;
  }
  void (*UYVYToUVRow)(const uint8* src_uyvy, int src_stride_uyvy,
                      uint8* dst_u, uint8* dst_v, int pix);
  void (*UYVYToYRow)(const uint8* src_uyvy,
                     uint8* dst_y, int pix);
  UYVYToYRow = UYVYToYRow_C;
  UYVYToUVRow = UYVYToUVRow_C;
#if defined(HAS_UYVYTOYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    if (width > 16) {
      UYVYToUVRow = UYVYToUVRow_Any_SSE2;
      UYVYToYRow = UYVYToYRow_Any_SSE2;
    }
    if (IS_ALIGNED(width, 16)) {
      UYVYToUVRow = UYVYToUVRow_Unaligned_SSE2;
      UYVYToYRow = UYVYToYRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_uyvy, 16) && IS_ALIGNED(src_stride_uyvy, 16)) {
        UYVYToUVRow = UYVYToUVRow_SSE2;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          UYVYToYRow = UYVYToYRow_SSE2;
        }
      }
    }
  }
#endif
  for (int y = 0; y < height - 1; y += 2) {
    UYVYToUVRow(src_uyvy, src_stride_uyvy, dst_u, dst_v, width);
    UYVYToYRow(src_uyvy, dst_y, width);
    UYVYToYRow(src_uyvy + src_stride_uyvy, dst_y + dst_stride_y, width);
    src_uyvy += src_stride_uyvy * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    UYVYToUVRow(src_uyvy, 0, dst_u, dst_v, width);
    UYVYToYRow(src_uyvy, dst_y, width);
  }
  return 0;
}

// gcc provided macros
#if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && defined(__BIG_ENDIAN)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define LIBYUV_LITTLE_ENDIAN
#endif
// Visual C for x86 defines these
#elif defined(_M_X64) || defined(_M_IX86)
#define LIBYUV_LITTLE_ENDIAN
#endif

#ifdef LIBYUV_LITTLE_ENDIAN
#define READWORD(p) (*((uint32*) (p)))
#else
uint32 READWORD(const uint8* p) {
  return (uint32) p[0] |
         ((uint32) (p[1]) << 8) |
         ((uint32) (p[2]) << 16) |
         ((uint32) (p[3]) << 24);
}
#endif

// Must be multiple of 6 pixels.  Will over convert to handle remainder.
// https://developer.apple.com/quicktime/icefloe/dispatch019.html#v210
void V210ToUYVYRow_C(const uint8* src_v210, uint8* dst_uyvy, int width) {
  for (int x = 0; x < width; x += 6) {
    uint32 w = READWORD(src_v210 + 0);
    dst_uyvy[0] = (w >> 2) & 0xff;
    dst_uyvy[1] = (w >> 12) & 0xff;
    dst_uyvy[2] = (w >> 22) & 0xff;

    w = READWORD(src_v210 + 4);
    dst_uyvy[3] = (w >> 2) & 0xff;
    dst_uyvy[4] = (w >> 12) & 0xff;
    dst_uyvy[5] = (w >> 22) & 0xff;

    w = READWORD(src_v210 + 8);
    dst_uyvy[6] = (w >> 2) & 0xff;
    dst_uyvy[7] = (w >> 12) & 0xff;
    dst_uyvy[8] = (w >> 22) & 0xff;

    w = READWORD(src_v210 + 12);
    dst_uyvy[9] = (w >> 2) & 0xff;
    dst_uyvy[10] = (w >> 12) & 0xff;
    dst_uyvy[11] = (w >> 22) & 0xff;

    dst_uyvy += 12;
    src_v210 += 16;
  }
}

// Convert V210 to I420.
// V210 is 10 bit version of UYVY.  16 bytes to store 6 pixels.
// With is multiple of 48.
int V210ToI420(const uint8* src_v210, int src_stride_v210,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (width * 16 / 6 > kMaxStride) {  // row buffer is required
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_v210 = src_v210 + (height - 1) * src_stride_v210;
    src_stride_v210 = -src_stride_v210;
  }
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*V210ToUYVYRow)(const uint8* src_v210, uint8* dst_uyvy, int pix);
  V210ToUYVYRow = V210ToUYVYRow_C;

  void (*UYVYToUVRow)(const uint8* src_uyvy, int src_stride_uyvy,
                      uint8* dst_u, uint8* dst_v, int pix);
  void (*UYVYToYRow)(const uint8* src_uyvy,
                     uint8* dst_y, int pix);
  UYVYToYRow = UYVYToYRow_C;
  UYVYToUVRow = UYVYToUVRow_C;
#if defined(HAS_UYVYTOYROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(width, 16)) {
    UYVYToUVRow = UYVYToUVRow_SSE2;
    UYVYToYRow = UYVYToYRow_Unaligned_SSE2;
    if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
      UYVYToYRow = UYVYToYRow_SSE2;
    }
  }
#endif
  for (int y = 0; y < height - 1; y += 2) {
    V210ToUYVYRow(src_v210, row, width);
    V210ToUYVYRow(src_v210 + src_stride_v210, row + kMaxStride, width);
    UYVYToUVRow(row, kMaxStride, dst_u, dst_v, width);
    UYVYToYRow(row, dst_y, width);
    UYVYToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
    src_v210 += src_stride_v210 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    V210ToUYVYRow(src_v210, row, width);
    UYVYToUVRow(row, 0, dst_u, dst_v, width);
    UYVYToYRow(row, dst_y, width);
  }
  return 0;
}

int ARGBToI420(const uint8* src_argb, int src_stride_argb,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }
  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);

  ARGBToYRow = ARGBToYRow_C;
  ARGBToUVRow = ARGBToUVRow_C;
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    if (width > 16) {
      ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
      ARGBToYRow = ARGBToYRow_Any_SSSE3;
    }
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_Unaligned_SSSE3;
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride_argb, 16)) {
        ARGBToUVRow = ARGBToUVRow_SSSE3;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          ARGBToYRow = ARGBToYRow_SSSE3;
        }
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    ARGBToUVRow(src_argb, src_stride_argb, dst_u, dst_v, width);
    ARGBToYRow(src_argb, dst_y, width);
    ARGBToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
    src_argb += src_stride_argb * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow(src_argb, 0, dst_u, dst_v, width);
    ARGBToYRow(src_argb, dst_y, width);
  }
  return 0;
}

int BGRAToI420(const uint8* src_bgra, int src_stride_bgra,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_bgra = src_bgra + (height - 1) * src_stride_bgra;
    src_stride_bgra = -src_stride_bgra;
  }
  void (*BGRAToYRow)(const uint8* src_bgra, uint8* dst_y, int pix);
  void (*BGRAToUVRow)(const uint8* src_bgra0, int src_stride_bgra,
                      uint8* dst_u, uint8* dst_v, int width);

  BGRAToYRow = BGRAToYRow_C;
  BGRAToUVRow = BGRAToUVRow_C;
#if defined(HAS_BGRATOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    if (width > 16) {
      BGRAToUVRow = BGRAToUVRow_Any_SSSE3;
      BGRAToYRow = BGRAToYRow_Any_SSSE3;
    }
    if (IS_ALIGNED(width, 16)) {
      BGRAToUVRow = BGRAToUVRow_Unaligned_SSSE3;
      BGRAToYRow = BGRAToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_bgra, 16) && IS_ALIGNED(src_stride_bgra, 16)) {
        BGRAToUVRow = BGRAToUVRow_SSSE3;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          BGRAToYRow = BGRAToYRow_SSSE3;
        }
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    BGRAToUVRow(src_bgra, src_stride_bgra, dst_u, dst_v, width);
    BGRAToYRow(src_bgra, dst_y, width);
    BGRAToYRow(src_bgra + src_stride_bgra, dst_y + dst_stride_y, width);
    src_bgra += src_stride_bgra * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    BGRAToUVRow(src_bgra, 0, dst_u, dst_v, width);
    BGRAToYRow(src_bgra, dst_y, width);
  }
  return 0;
}

int ABGRToI420(const uint8* src_abgr, int src_stride_abgr,
               uint8* dst_y, int dst_stride_y,
               uint8* dst_u, int dst_stride_u,
               uint8* dst_v, int dst_stride_v,
               int width, int height) {
  if (height < 0) {
    height = -height;
    src_abgr = src_abgr + (height - 1) * src_stride_abgr;
    src_stride_abgr = -src_stride_abgr;
  }
  void (*ABGRToYRow)(const uint8* src_abgr, uint8* dst_y, int pix);
  void (*ABGRToUVRow)(const uint8* src_abgr0, int src_stride_abgr,
                      uint8* dst_u, uint8* dst_v, int width);

  ABGRToYRow = ABGRToYRow_C;
  ABGRToUVRow = ABGRToUVRow_C;
#if defined(HAS_ABGRTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    if (width > 16) {
      ABGRToUVRow = ABGRToUVRow_Any_SSSE3;
      ABGRToYRow = ABGRToYRow_Any_SSSE3;
    }
    if (IS_ALIGNED(width, 16)) {
      ABGRToUVRow = ABGRToUVRow_Unaligned_SSSE3;
      ABGRToYRow = ABGRToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_abgr, 16) && IS_ALIGNED(src_stride_abgr, 16)) {
        ABGRToUVRow = ABGRToUVRow_SSSE3;
        if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
          ABGRToYRow = ABGRToYRow_SSSE3;
        }
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    ABGRToUVRow(src_abgr, src_stride_abgr, dst_u, dst_v, width);
    ABGRToYRow(src_abgr, dst_y, width);
    ABGRToYRow(src_abgr + src_stride_abgr, dst_y + dst_stride_y, width);
    src_abgr += src_stride_abgr * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ABGRToUVRow(src_abgr, 0, dst_u, dst_v, width);
    ABGRToYRow(src_abgr, dst_y, width);
  }
  return 0;
}

int RGB24ToI420(const uint8* src_rgb24, int src_stride_rgb24,
                uint8* dst_y, int dst_stride_y,
                uint8* dst_u, int dst_stride_u,
                uint8* dst_v, int dst_stride_v,
                int width, int height) {
  if (width * 4 > kMaxStride) {  // row buffer is required
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_rgb24 = src_rgb24 + (height - 1) * src_stride_rgb24;
    src_stride_rgb24 = -src_stride_rgb24;
  }
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*RGB24ToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix);

  RGB24ToARGBRow = RGB24ToARGBRow_C;
#if defined(HAS_RGB24TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      TestReadSafe(src_rgb24, src_stride_rgb24, width, height, 3, 48)) {
    RGB24ToARGBRow = RGB24ToARGBRow_SSSE3;
  }
#endif

  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);

  ARGBToYRow = ARGBToYRow_C;
  ARGBToUVRow = ARGBToUVRow_C;
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    if (width > 16) {
      ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    }
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    RGB24ToARGBRow(src_rgb24, row, width);
    RGB24ToARGBRow(src_rgb24 + src_stride_rgb24, row + kMaxStride, width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
    src_rgb24 += src_stride_rgb24 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    RGB24ToARGBRow_C(src_rgb24, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
  }
  return 0;
}

int RAWToI420(const uint8* src_raw, int src_stride_raw,
              uint8* dst_y, int dst_stride_y,
              uint8* dst_u, int dst_stride_u,
              uint8* dst_v, int dst_stride_v,
              int width, int height) {
  if (width * 4 > kMaxStride) {  // row buffer is required
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_raw = src_raw + (height - 1) * src_stride_raw;
    src_stride_raw = -src_stride_raw;
  }
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*RAWToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix);

  RAWToARGBRow = RAWToARGBRow_C;
#if defined(HAS_RAWTOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) &&
      TestReadSafe(src_raw, src_stride_raw, width, height, 3, 48)) {
    RAWToARGBRow = RAWToARGBRow_SSSE3;
  }
#endif

  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);

  ARGBToYRow = ARGBToYRow_C;
  ARGBToUVRow = ARGBToUVRow_C;
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    if (width > 16) {
      ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    }
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    RAWToARGBRow(src_raw, row, width);
    RAWToARGBRow(src_raw + src_stride_raw, row + kMaxStride, width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
    src_raw += src_stride_raw * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    RAWToARGBRow_C(src_raw, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
  }
  return 0;
}

int RGB565ToI420(const uint8* src_rgb565, int src_stride_rgb565,
                 uint8* dst_y, int dst_stride_y,
                 uint8* dst_u, int dst_stride_u,
                 uint8* dst_v, int dst_stride_v,
                 int width, int height) {
  if (width * 4 > kMaxStride) {  // row buffer is required
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_rgb565 = src_rgb565 + (height - 1) * src_stride_rgb565;
    src_stride_rgb565 = -src_stride_rgb565;
  }
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*RGB565ToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix);

  RGB565ToARGBRow = RGB565ToARGBRow_C;
#if defined(HAS_RGB565TOARGBROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      TestReadSafe(src_rgb565, src_stride_rgb565, width, height, 2, 16)) {
    RGB565ToARGBRow = RGB565ToARGBRow_SSE2;
  }
#endif

  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);

  ARGBToYRow = ARGBToYRow_C;
  ARGBToUVRow = ARGBToUVRow_C;
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    if (width > 16) {
      ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    }
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    RGB565ToARGBRow(src_rgb565, row, width);
    RGB565ToARGBRow(src_rgb565 + src_stride_rgb565, row + kMaxStride, width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
    src_rgb565 += src_stride_rgb565 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    RGB565ToARGBRow_C(src_rgb565, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
  }
  return 0;
}

int ARGB1555ToI420(const uint8* src_argb1555, int src_stride_argb1555,
                 uint8* dst_y, int dst_stride_y,
                 uint8* dst_u, int dst_stride_u,
                 uint8* dst_v, int dst_stride_v,
                 int width, int height) {
  if (width * 4 > kMaxStride) {  // row buffer is required
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_argb1555 = src_argb1555 + (height - 1) * src_stride_argb1555;
    src_stride_argb1555 = -src_stride_argb1555;
  }
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*ARGB1555ToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix);

  ARGB1555ToARGBRow = ARGB1555ToARGBRow_C;
#if defined(HAS_ARGB1555TOARGBROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      TestReadSafe(src_argb1555, src_stride_argb1555, width, height, 2, 16)) {
    ARGB1555ToARGBRow = ARGB1555ToARGBRow_SSE2;
  }
#endif

  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);

  ARGBToYRow = ARGBToYRow_C;
  ARGBToUVRow = ARGBToUVRow_C;
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    if (width > 16) {
      ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    }
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    ARGB1555ToARGBRow(src_argb1555, row, width);
    ARGB1555ToARGBRow(src_argb1555 + src_stride_argb1555,
                      row + kMaxStride, width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
    src_argb1555 += src_stride_argb1555 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGB1555ToARGBRow_C(src_argb1555, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
  }
  return 0;
}

int ARGB4444ToI420(const uint8* src_argb4444, int src_stride_argb4444,
                   uint8* dst_y, int dst_stride_y,
                   uint8* dst_u, int dst_stride_u,
                   uint8* dst_v, int dst_stride_v,
                   int width, int height) {
  if (width * 4 > kMaxStride) {  // row buffer is required
    return -1;
  }
  if (height < 0) {
    height = -height;
    src_argb4444 = src_argb4444 + (height - 1) * src_stride_argb4444;
    src_stride_argb4444 = -src_stride_argb4444;
  }
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  void (*ARGB4444ToARGBRow)(const uint8* src_rgb, uint8* dst_argb, int pix);

  ARGB4444ToARGBRow = ARGB4444ToARGBRow_C;
#if defined(HAS_ARGB4444TOARGBROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      TestReadSafe(src_argb4444, src_stride_argb4444, width, height, 2, 16)) {
    ARGB4444ToARGBRow = ARGB4444ToARGBRow_SSE2;
  }
#endif

  void (*ARGBToYRow)(const uint8* src_argb, uint8* dst_y, int pix);
  void (*ARGBToUVRow)(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width);

  ARGBToYRow = ARGBToYRow_C;
  ARGBToUVRow = ARGBToUVRow_C;
#if defined(HAS_ARGBTOYROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    if (width > 16) {
      ARGBToUVRow = ARGBToUVRow_Any_SSSE3;
    }
    ARGBToYRow = ARGBToYRow_Any_SSSE3;
    if (IS_ALIGNED(width, 16)) {
      ARGBToUVRow = ARGBToUVRow_SSSE3;
      ARGBToYRow = ARGBToYRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_y, 16) && IS_ALIGNED(dst_stride_y, 16)) {
        ARGBToYRow = ARGBToYRow_SSSE3;
      }
    }
  }
#endif

  for (int y = 0; y < height - 1; y += 2) {
    ARGB4444ToARGBRow(src_argb4444, row, width);
    ARGB4444ToARGBRow(src_argb4444 + src_stride_argb4444,
                      row + kMaxStride, width);
    ARGBToUVRow(row, kMaxStride, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
    ARGBToYRow(row + kMaxStride, dst_y + dst_stride_y, width);
    src_argb4444 += src_stride_argb4444 * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGB4444ToARGBRow_C(src_argb4444, row, width);
    ARGBToUVRow(row, 0, dst_u, dst_v, width);
    ARGBToYRow(row, dst_y, width);
  }
  return 0;
}

// Convert camera sample to I420 with cropping, rotation and vertical flip.
// src_width is used for source stride computation
// src_height is used to compute location of planes, and indicate inversion
// TODO(fbarchard): sample_size should be used to ensure the low levels do
// not read outside the buffer provided.  It is measured in bytes and is the
// size of the frame.  With MJPEG it is the compressed size of the frame.
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
    case FOURCC_V210:
      // TODO(fbarchard): Confirm stride is 16 bytes per 6 pixels.
      src = sample + (aligned_src_width * crop_y + crop_x) * 16 / 6;
      V210ToI420(src, aligned_src_width * 16 / 6,
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
    case FOURCC_RGBP:
      src = sample + (src_width * crop_y + crop_x) * 2;
      RGB565ToI420(src, src_width * 2,
                   y, y_stride,
                   u, u_stride,
                   v, v_stride,
                   dst_width, inv_dst_height);
      break;
//  V4L2_PIX_FMT_RGB555 'RGBO'
//               Byte 0                    Byte 1
//  Bit   7  6  5  4  3  2  1  0    7  6  5  4  3  2  1  0
//       g2 g1 g0 b4 b3 b2 b1 b0    a r4 r3 r2 r1 r0 g4 g3
//  Bit 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
//       a r4 r3 r2 r1 r0 g4 g3 g2 g1 g0 b4 b3 b2 b1 b0
    case FOURCC_RGBO:
      src = sample + (src_width * crop_y + crop_x) * 2;
      ARGB1555ToI420(src, src_width * 2,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
//  V4L2_PIX_FMT_RGB444 'R444'
//               Byte 0                    Byte 1
//  Bit   7  6  5  4  3  2  1  0    7  6  5  4  3  2  1  0
//       g3 g2 g1 g0 b3 b2 b1 b0   a3 a2 a1 a0 r3 r2 r1 r0
//  Bit 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
//      a3 a2 a1 a0 r3 r2 r1 r0 g3 g2 g1 g0 b3 b2 b1 b0
    case FOURCC_R444:
      src = sample + (src_width * crop_y + crop_x) * 2;
      ARGB4444ToI420(src, src_width * 2,
                     y, y_stride,
                     u, u_stride,
                     v, v_stride,
                     dst_width, inv_dst_height);
      break;
    // TODO(fbarchard): Support cropping Bayer by odd numbers
    // by adjusting fourcc.
    case FOURCC_BGGR:
      src = sample + (src_width * crop_y + crop_x);
      BayerBGGRToI420(src, src_width,
                      y, y_stride,
                      u, u_stride,
                      v, v_stride,
                      dst_width, inv_dst_height);
      break;

    case FOURCC_GBRG:
      src = sample + (src_width * crop_y + crop_x);
      BayerGBRGToI420(src, src_width,
                      y, y_stride,
                      u, u_stride,
                      v, v_stride,
                      dst_width, inv_dst_height);
      break;

    case FOURCC_GRBG:
      src = sample + (src_width * crop_y + crop_x);
      BayerGRBGToI420(src, src_width,
                      y, y_stride,
                      u, u_stride,
                      v, v_stride,
                      dst_width, inv_dst_height);
      break;

    case FOURCC_RGGB:
      src = sample + (src_width * crop_y + crop_x);
      BayerRGGBToI420(src, src_width,
                      y, y_stride,
                      u, u_stride,
                      v, v_stride,
                      dst_width, inv_dst_height);
      break;

    case FOURCC_I400:
      src = sample + src_width * crop_y + crop_x;
      I400ToI420(src, src_width,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;

    // Biplanar formats
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
    case FOURCC_M420:
      src = sample + (src_width * crop_y) * 12 / 8 + crop_x;
      M420ToI420(src, src_width,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
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
    case FOURCC_I422:
    case FOURCC_YV16: {
      const uint8* src_y = sample + src_width * crop_y + crop_x;
      const uint8* src_u;
      const uint8* src_v;
      int halfwidth = (src_width + 1) / 2;
      if (format == FOURCC_I422) {
        src_u = sample + src_width * abs_src_height +
            halfwidth * crop_y + crop_x / 2;
        src_v = sample + src_width * abs_src_height +
            halfwidth * (abs_src_height + crop_y) + crop_x / 2;
      } else {
        src_v = sample + src_width * abs_src_height +
            halfwidth * crop_y + crop_x / 2;
        src_u = sample + src_width * abs_src_height +
            halfwidth * (abs_src_height + crop_y) + crop_x / 2;
      }
      I422ToI420(src_y, src_width,
                 src_u, halfwidth,
                 src_v, halfwidth,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    }
    case FOURCC_I444:
    case FOURCC_YV24: {
      const uint8* src_y = sample + src_width * crop_y + crop_x;
      const uint8* src_u;
      const uint8* src_v;
      if (format == FOURCC_I444) {
        src_u = sample + src_width * (abs_src_height + crop_y) + crop_x;
        src_v = sample + src_width * (abs_src_height * 2 + crop_y) + crop_x;
      } else {
        src_v = sample + src_width * (abs_src_height + crop_y) + crop_x;
        src_u = sample + src_width * (abs_src_height * 2 + crop_y) + crop_x;
      }
      I444ToI420(src_y, src_width,
                 src_u, src_width,
                 src_v, src_width,
                 y, y_stride,
                 u, u_stride,
                 v, v_stride,
                 dst_width, inv_dst_height);
      break;
    }
    // Formats not supported
    case FOURCC_MJPG:
    default:
      return -1;  // unknown fourcc - return failure code.
  }
  return 0;
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

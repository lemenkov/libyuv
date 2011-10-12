/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "planar_functions.h"

#include <string.h>

#include "cpu_id.h"
#include "row.h"

namespace libyuv {

#if defined(__ARM_NEON__) && !defined(COVERAGE_ENABLED)
#define HAS_SPLITUV_NEON
// Reads 16 pairs of UV and write even values to dst_u and odd to dst_v
// Alignment requirement: 16 bytes for pointers, and multiple of 16 pixels.
static void SplitUV_NEON(const uint8* src_uv,
                         uint8* dst_u, uint8* dst_v, int pix) {
  __asm__ volatile
  (
    "1:\n"
    "vld2.u8    {q0,q1}, [%0]!    \n"  // load 16 pairs of UV
    "vst1.u8    {q0}, [%1]!       \n"  // store U
    "vst1.u8    {q1}, [%2]!       \n"  // Store V
    "subs       %3, %3, #16       \n"  // 16 processed per loop
    "bhi        1b                \n"
    :                                                // Output registers
    : "r"(src_uv), "r"(dst_u), "r"(dst_v), "r"(pix)  // Input registers
    : "q0", "q1"                                     // Clobber List
  );
}

#elif (defined(WIN32) || defined(__i386__)) && !defined(COVERAGE_ENABLED) && \
    !defined(__PIC__) && !TARGET_IPHONE_SIMULATOR
#if defined(_MSC_VER)
#define TALIGN16(t, var) static __declspec(align(16)) t _ ## var
#elif defined(OSX)
#define TALIGN16(t, var) t var __attribute__((aligned(16)))
#else
#define TALIGN16(t, var) t _ ## var __attribute__((aligned(16)))
#endif

// shuffle constant to put even bytes in low 8 and odd bytes in high 8 bytes
extern "C" TALIGN16(const uint8, shufevenodd[16]) =
  { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };

#if defined(WIN32) && !defined(COVERAGE_ENABLED)
#define HAS_SPLITUV_SSE2
__declspec(naked)
static void SplitUV_SSE2(const uint8* src_uv,
                         uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    // src_uv
    mov        edx, [esp + 4 + 8]    // dst_u
    mov        edi, [esp + 4 + 12]   // dst_v
    mov        ecx, [esp + 4 + 16]   // pix
    pcmpeqb    xmm7, xmm7            // generate mask 0x00ff00ff
    psrlw      xmm7, 8

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm0
    movdqa     xmm3, xmm1
    pand       xmm0, xmm7   // even bytes
    pand       xmm1, xmm7
    packuswb   xmm0, xmm1
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    psrlw      xmm2, 8      // odd bytes
    psrlw      xmm3, 8
    packuswb   xmm2, xmm3
    movdqa     [edi], xmm2
    lea        edi, [edi + 16]
    sub        ecx, 16
    ja         wloop
    pop        edi
    ret
  }
}

#define HAS_SPLITUV_SSSE3
__declspec(naked)
static void SplitUV_SSSE3(const uint8* src_uv,
                          uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]       // src_uv
    mov        edx, [esp + 4 + 8]       // dst_u
    mov        edi, [esp + 4 + 12]      // dst_v
    mov        ecx, [esp + 4 + 16]      // pix
    movdqa     xmm7, _shufevenodd

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax, [eax + 32]
    pshufb     xmm0, xmm7               // 8 u's and 8 v's
    pshufb     xmm1, xmm7               // 8 u's and 8 v's
    movdqa     xmm2, xmm0
    punpcklqdq xmm0, xmm1               // 16 u's
    punpckhqdq xmm2, xmm1               // 16 v's
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    movdqa     [edi], xmm2
    lea        edi, [edi + 16]
    sub        ecx, 16
    ja         wloop
    pop        edi
    ret
  }
}
#elif defined(__i386__) && !defined(COVERAGE_ENABLED) && \
    !TARGET_IPHONE_SIMULATOR
#define HAS_SPLITUV_SSE2
extern "C" void SplitUV_SSE2(const uint8* src_uv,
                             uint8* dst_u, uint8* dst_v, int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _SplitUV_SSE2\n"
"_SplitUV_SSE2:\n"
#else
    ".global SplitUV_SSE2\n"
"SplitUV_SSE2:\n"
#endif
    "push   %edi\n"
    "mov    0x8(%esp),%eax\n"
    "mov    0xc(%esp),%edx\n"
    "mov    0x10(%esp),%edi\n"
    "mov    0x14(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlw  $0x8,%xmm7\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "lea    0x20(%eax),%eax\n"
    "movdqa %xmm0,%xmm2\n"
    "movdqa %xmm1,%xmm3\n"
    "pand   %xmm7,%xmm0\n"
    "pand   %xmm7,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,(%edx)\n"
    "lea    0x10(%edx),%edx\n"
    "psrlw  $0x8,%xmm2\n"
    "psrlw  $0x8,%xmm3\n"
    "packuswb %xmm3,%xmm2\n"
    "movdqa %xmm2,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "pop    %edi\n"
    "ret\n"
);

#define HAS_SPLITUV_SSSE3
extern "C" void SplitUV_SSSE3(const uint8* src_uv,
                             uint8* dst_u, uint8* dst_v, int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _SplitUV_SSSE3\n"
"_SplitUV_SSSE3:\n"
#else
    ".global SplitUV_SSSE3\n"
"SplitUV_SSSE3:\n"
#endif
    "push   %edi\n"
    "mov    0x8(%esp),%eax\n"
    "mov    0xc(%esp),%edx\n"
    "mov    0x10(%esp),%edi\n"
    "mov    0x14(%esp),%ecx\n"
    "movdqa _shufevenodd,%xmm7\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "lea    0x20(%eax),%eax\n"
    "pshufb %xmm7,%xmm0\n"
    "pshufb %xmm7,%xmm1\n"
    "movdqa %xmm0,%xmm2\n"
    "punpcklqdq %xmm1,%xmm0\n"
    "punpckhqdq %xmm1,%xmm2\n"
    "movdqa %xmm0,(%edx)\n"
    "lea    0x10(%edx),%edx\n"
    "movdqa %xmm2,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "pop    %edi\n"
    "ret\n"
);
#endif
#endif

static void SplitUV_C(const uint8* src_uv,
                      uint8* dst_u, uint8* dst_v, int pix) {
  // Copy a row of UV.
  for (int x = 0; x < pix; ++x) {
    dst_u[0] = src_uv[0];
    dst_v[0] = src_uv[1];
    src_uv += 2;
    dst_u += 1;
    dst_v += 1;
  }
}

static void I420CopyPlane(const uint8* src_y, int src_pitch_y,
                          uint8* dst_y, int dst_pitch_y,
                          int width, int height) {
  // Copy plane
  for (int y = 0; y < height; ++y) {
    memcpy(dst_y, src_y, width);
    src_y += src_pitch_y;
    dst_y += dst_pitch_y;
  }
}

static void I420CopyPlane2(const uint8* src, int src_pitch_0, int src_pitch_1,
                           uint8* dst, int dst_pitch,
                           int width, int height) {
  // Copy plane
  for (int y = 0; y < height; y += 2) {
    memcpy(dst, src, width);
    src += src_pitch_0;
    dst += dst_pitch;
    memcpy(dst, src, width);
    src += src_pitch_1;
    dst += dst_pitch;
  }
}

// TODO(fbarchard): For biplanar formats (ie NV21), the Y plane is the same
// as I420, and only the chroma plane varies. Copy the Y plane by reference,
// and just convert the UV.  This method can be used for NV21, NV12, I420,
// I422, M422.  8 of the 12 bits is Y, so this would copy 3 times less data,
// which is approximately how much faster it would be.

// Helper function to copy yuv data without scaling.  Used
// by our jpeg conversion callbacks to incrementally fill a yuv image.
void I420Copy(const uint8* src_y, int src_pitch_y,
              const uint8* src_u, int src_pitch_u,
              const uint8* src_v, int src_pitch_v,
              uint8* dst_y, int dst_pitch_y,
              uint8* dst_u, int dst_pitch_u,
              uint8* dst_v, int dst_pitch_v,
              int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_pitch_y;
    src_u = src_u + (height - 1) * src_pitch_u;
    src_v = src_v + (height - 1) * src_pitch_v;
    src_pitch_y = -src_pitch_y;
    src_pitch_u = -src_pitch_u;
    src_pitch_v = -src_pitch_v;
  }

  int halfwidth = (width + 1) >> 1;
  int halfheight = (height + 1) >> 1;
  I420CopyPlane(src_y, src_pitch_y, dst_y, dst_pitch_y, width, height);
  I420CopyPlane(src_u, src_pitch_u, dst_u, dst_pitch_u, halfwidth, halfheight);
  I420CopyPlane(src_v, src_pitch_v, dst_v, dst_pitch_v, halfwidth, halfheight);
}

// Helper function to copy yuv data without scaling.  Used
// by our jpeg conversion callbacks to incrementally fill a yuv image.
void I422ToI420(const uint8* src_y, int src_pitch_y,
                const uint8* src_u, int src_pitch_u,
                const uint8* src_v, int src_pitch_v,
                uint8* dst_y, int dst_pitch_y,
                uint8* dst_u, int dst_pitch_u,
                uint8* dst_v, int dst_pitch_v,
                int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_y = src_y + (height - 1) * src_pitch_y;
    src_u = src_u + (height - 1) * src_pitch_u;
    src_v = src_v + (height - 1) * src_pitch_v;
    src_pitch_y = -src_pitch_y;
    src_pitch_u = -src_pitch_u;
    src_pitch_v = -src_pitch_v;
  }

  // Copy Y plane
  I420CopyPlane(src_y, src_pitch_y, dst_y, dst_pitch_y, width, height);

  // SubSample UV planes.
  int x, y;
  int halfwidth = (width + 1) >> 1;
  for (y = 0; y < height; y += 2) {
    const uint8* u0 = src_u;
    const uint8* u1 = src_u + src_pitch_u;
    if ((y + 1) >= height) {
      u1 = u0;
    }
    for (x = 0; x < halfwidth; ++x) {
      dst_u[x] = (u0[x] + u1[x] + 1) >> 1;
    }
    src_u += src_pitch_u * 2;
    dst_u += dst_pitch_u;
  }
  for (y = 0; y < height; y += 2) {
    const uint8* v0 = src_v;
    const uint8* v1 = src_v + src_pitch_v;
    if ((y + 1) >= height) {
      v1 = v0;
    }
    for (x = 0; x < halfwidth; ++x) {
      dst_v[x] = (v0[x] + v1[x] + 1) >> 1;
    }
    src_v += src_pitch_v * 2;
    dst_v += dst_pitch_v;
  }
}

// Support converting from FOURCC_M420
// Useful for bandwidth constrained transports like USB 1.0 and 2.0 and for
// easy conversion to I420.
// M420 format description:
// M420 is row biplanar 420: 2 rows of Y and 1 row of VU.
// Chroma is half width / half height. (420)
// src_pitch_m420 is row planar.  Normally this will be the width in pixels.
//   The UV plane is half width, but 2 values, so src_pitch_m420 applies to this
//   as well as the two Y planes.
// TODO(fbarchard): Do NV21/NV12 formats with this function
static void X420ToI420(const uint8* src_y,
                       int src_pitch_y0, int src_pitch_y1,
                       const uint8* src_uv, int src_pitch_uv,
                       uint8* dst_y, int dst_pitch_y,
                       uint8* dst_u, int dst_pitch_u,
                       uint8* dst_v, int dst_pitch_v,
                       int width, int height) {
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_y = dst_y + (height - 1) * dst_pitch_y;
    dst_u = dst_u + (height - 1) * dst_pitch_u;
    dst_v = dst_v + (height - 1) * dst_pitch_v;
    dst_pitch_y = -dst_pitch_y;
    dst_pitch_u = -dst_pitch_u;
    dst_pitch_v = -dst_pitch_v;
  }

  int halfwidth = (width + 1) >> 1;
  void (*SplitUV)(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix);
#if defined(HAS_SPLITUV_NEON)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasNEON) &&
      (halfwidth % 16 == 0) &&
      IS_ALIGNED(src_uv, 16) && (src_pitch_uv % 16 == 0) &&
      IS_ALIGNED(dst_u, 16) && (dst_pitch_u % 16 == 0) &&
      IS_ALIGNED(dst_v, 16) && (dst_pitch_v % 16 == 0)) {
    SplitUV = SplitUV_NEON;
  } else
#elif defined(HAS_SPLITUV_SSSE3)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSSE3) &&
      (halfwidth % 16 == 0) &&
      IS_ALIGNED(src_uv, 16) && (src_pitch_uv % 16 == 0) &&
      IS_ALIGNED(dst_u, 16) && (dst_pitch_u % 16 == 0) &&
      IS_ALIGNED(dst_v, 16) && (dst_pitch_v % 16 == 0)) {
    SplitUV = SplitUV_SSSE3;
  } else
#elif defined(HAS_SPLITUV_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (halfwidth % 16 == 0) &&
      IS_ALIGNED(src_uv, 16) && (src_pitch_uv % 16 == 0) &&
      IS_ALIGNED(dst_u, 16) && (dst_pitch_u % 16 == 0) &&
      IS_ALIGNED(dst_v, 16) && (dst_pitch_v % 16 == 0)) {
    SplitUV = SplitUV_SSE2;
  } else
#endif
  {
    SplitUV = SplitUV_C;
  }

  I420CopyPlane2(src_y, src_pitch_y0, src_pitch_y1, dst_y, dst_pitch_y,
                 width, height);

  int halfheight = (height + 1) >> 1;
  for (int y = 0; y < halfheight; ++y) {
    // Copy a row of UV.
    SplitUV(src_uv, dst_u, dst_v, halfwidth);
    dst_u += dst_pitch_u;
    dst_v += dst_pitch_v;
    src_uv += src_pitch_uv;
  }
}

// Convert M420 to I420.
void M420ToI420(const uint8* src_m420, int src_pitch_m420,
                uint8* dst_y, int dst_pitch_y,
                uint8* dst_u, int dst_pitch_u,
                uint8* dst_v, int dst_pitch_v,
                int width, int height) {
  X420ToI420(src_m420, src_pitch_m420, src_pitch_m420 * 2,
             src_m420 + src_pitch_m420 * 2, src_pitch_m420 * 3,
             dst_y, dst_pitch_y, dst_u, dst_pitch_u, dst_v, dst_pitch_v,
             width, height);
}

// Convert NV12 to I420.
void NV12ToI420(const uint8* src_y,
                const uint8* src_uv,
                int src_pitch,
                uint8* dst_y, int dst_pitch_y,
                uint8* dst_u, int dst_pitch_u,
                uint8* dst_v, int dst_pitch_v,
                int width, int height) {
  X420ToI420(src_y, src_pitch, src_pitch,
             src_uv, src_pitch,
             dst_y, dst_pitch_y, dst_u, dst_pitch_u, dst_v, dst_pitch_v,
             width, height);
}

#if defined(WIN32) && !defined(COVERAGE_ENABLED)
#define HAS_SPLITYUY2_SSE2
__declspec(naked)
static void SplitYUY2_SSE2(const uint8* src_yuy2,
                           uint8* dst_y, uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    // src_yuy2
    mov        edx, [esp + 8 + 8]    // dst_y
    mov        esi, [esp + 8 + 12]   // dst_u
    mov        edi, [esp + 8 + 16]   // dst_v
    mov        ecx, [esp + 8 + 20]   // pix
    pcmpeqb    xmm7, xmm7            // generate mask 0x00ff00ff
    psrlw      xmm7, 8

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm0
    movdqa     xmm3, xmm1
    pand       xmm2, xmm7   // even bytes are Y
    pand       xmm3, xmm7
    packuswb   xmm2, xmm3
    movdqa     [edx], xmm2
    lea        edx, [edx + 16]
    psrlw      xmm0, 8      // YUYV -> UVUV
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm7  // U
    packuswb   xmm0, xmm0
    movq       qword ptr [esi], xmm0
    lea        esi, [esi + 8]
    psrlw      xmm1, 8     // V
    packuswb   xmm1, xmm1
    movq       qword ptr [edi], xmm1
    lea        edi, [edi + 8]
    sub        ecx, 16
    ja         wloop

    pop        edi
    pop        esi
    ret
  }
}
#elif defined(__i386__) && !defined(COVERAGE_ENABLED) && \
    !TARGET_IPHONE_SIMULATOR
#define HAS_SPLITYUY2_SSE2
extern "C" void SplitYUY2_SSE2(const uint8* src_yuy2, uint8* dst_y,
                               uint8* dst_u, uint8* dst_v, int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _SplitYUY2_SSE2\n"
"_SplitYUY2_SSE2:\n"
#else
    ".global SplitYUY2_SSE2\n"
"SplitYUY2_SSE2:\n"
#endif
    "push   %esi\n"
    "push   %edi\n"
    "mov    0xc(%esp),%eax\n"
    "mov    0x10(%esp),%edx\n"
    "mov    0x14(%esp),%esi\n"
    "mov    0x18(%esp),%edi\n"
    "mov    0x1c(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlw  $0x8,%xmm7\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "lea    0x20(%eax),%eax\n"
    "movdqa %xmm0,%xmm2\n"
    "movdqa %xmm1,%xmm3\n"
    "pand   %xmm7,%xmm2\n"
    "pand   %xmm7,%xmm3\n"
    "packuswb %xmm3,%xmm2\n"
    "movdqa %xmm2,(%edx)\n"
    "lea    0x10(%edx),%edx\n"
    "psrlw  $0x8,%xmm0\n"
    "psrlw  $0x8,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,%xmm1\n"
    "pand   %xmm7,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,(%esi)\n"
    "lea    0x8(%esi),%esi\n"
    "psrlw  $0x8,%xmm1\n"
    "packuswb %xmm1,%xmm1\n"
    "movq   %xmm1,(%edi)\n"
    "lea    0x8(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"
);
#endif

static void SplitYUY2_C(const uint8* src_yuy2,
                        uint8* dst_y, uint8* dst_u, uint8* dst_v, int pix) {
  // Copy a row of YUY2.
  for (int x = 0; x < pix; x += 2) {
    dst_y[0] = src_yuy2[0];
    dst_y[1] = src_yuy2[2];
    dst_u[0] = src_yuy2[1];
    dst_v[0] = src_yuy2[3];
    src_yuy2 += 4;
    dst_y += 2;
    dst_u += 1;
    dst_v += 1;
  }
}

// Convert Q420 to I420.
// Format is rows of YY/YUYV
void Q420ToI420(const uint8* src_y, int src_pitch_y,
                const uint8* src_yuy2, int src_pitch_yuy2,
                uint8* dst_y, int dst_pitch_y,
                uint8* dst_u, int dst_pitch_u,
                uint8* dst_v, int dst_pitch_v,
                int width, int height) {
  void (*SplitYUY2)(const uint8* src_yuy2,
                    uint8* dst_y, uint8* dst_u, uint8* dst_v, int pix);
#if defined(HAS_SPLITYUY2_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_yuy2, 16) && (src_pitch_yuy2 % 16 == 0) &&
      IS_ALIGNED(dst_y, 16) && (dst_pitch_y % 16 == 0) &&
      IS_ALIGNED(dst_u, 8) && (dst_pitch_u % 8 == 0) &&
      IS_ALIGNED(dst_v, 8) && (dst_pitch_v % 8 == 0)) {
    SplitYUY2 = SplitYUY2_SSE2;
  } else
#endif
  {
    SplitYUY2 = SplitYUY2_C;
  }
  for (int y = 0; y < height; y += 2) {
    memcpy(dst_y, src_y, width);
    dst_y += dst_pitch_y;
    src_y += src_pitch_y;

    // Copy a row of YUY2.
    SplitYUY2(src_yuy2, dst_y, dst_u, dst_v, width);
    dst_y += dst_pitch_y;
    dst_u += dst_pitch_u;
    dst_v += dst_pitch_v;
    src_yuy2 += src_pitch_yuy2;
  }
}

#if defined(WIN32) && !defined(COVERAGE_ENABLED)
#define HAS_YUY2TOI420ROW_SSE2
__declspec(naked)
void YUY2ToI420RowY_SSE2(const uint8* src_yuy2,
                         uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_yuy2
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix
    pcmpeqb    xmm7, xmm7        // generate mask 0x00ff00ff
    psrlw      xmm7, 8

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm7   // even bytes are Y
    pand       xmm1, xmm7
    packuswb   xmm0, xmm1
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         wloop
    ret
  }
}

__declspec(naked)
void YUY2ToI420RowUV_SSE2(const uint8* src_yuy2, int pitch_yuy2,
                          uint8* dst_u, uint8* dst_y, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    // src_yuy2
    mov        esi, [esp + 8 + 8]    // pitch_yuy2
    mov        edx, [esp + 8 + 12]   // dst_u
    mov        edi, [esp + 8 + 16]   // dst_v
    mov        ecx, [esp + 8 + 20]   // pix
    pcmpeqb    xmm7, xmm7            // generate mask 0x00ff00ff
    psrlw      xmm7, 8

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    psrlw      xmm0, 8      // YUYV -> UVUV
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm7  // U
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    psrlw      xmm1, 8     // V
    packuswb   xmm1, xmm1
    movq       qword ptr [edi], xmm1
    lea        edi, [edi + 8]
    sub        ecx, 16
    ja         wloop

    pop        edi
    pop        esi
    ret
  }
}

#define HAS_UYVYTOI420ROW_SSE2
__declspec(naked)
void UYVYToI420RowY_SSE2(const uint8* src_uyvy,
                         uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_uyvy
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8    // odd bytes are Y
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         wloop
    ret
  }
}

__declspec(naked)
void UYVYToI420RowUV_SSE2(const uint8* src_uyvy, int pitch_uyvy,
                          uint8* dst_u, uint8* dst_y, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    // src_yuy2
    mov        esi, [esp + 8 + 8]    // pitch_yuy2
    mov        edx, [esp + 8 + 12]   // dst_u
    mov        edi, [esp + 8 + 16]   // dst_v
    mov        ecx, [esp + 8 + 20]   // pix
    pcmpeqb    xmm7, xmm7            // generate mask 0x00ff00ff
    psrlw      xmm7, 8

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    pand       xmm0, xmm7   // UYVY -> UVUV
    pand       xmm1, xmm7
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm7  // U
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    psrlw      xmm1, 8     // V
    packuswb   xmm1, xmm1
    movq       qword ptr [edi], xmm1
    lea        edi, [edi + 8]
    sub        ecx, 16
    ja         wloop

    pop        edi
    pop        esi
    ret
  }
}
#elif defined(__i386__) && !defined(COVERAGE_ENABLED) && \
    !TARGET_IPHONE_SIMULATOR

#define HAS_YUY2TOI420ROW_SSE2
extern "C" void YUY2ToI420RowY_SSE2(const uint8* src_yuy2,
                                    uint8* dst_y, int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _YUY2ToI420RowY_SSE2\n"
"_YUY2ToI420RowY_SSE2:\n"
#else
    ".global YUY2ToI420RowY_SSE2\n"
"YUY2ToI420RowY_SSE2:\n"
#endif
    "mov    0x4(%esp),%eax\n"
    "mov    0x8(%esp),%edx\n"
    "mov    0xc(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlw  $0x8,%xmm7\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "lea    0x20(%eax),%eax\n"
    "pand   %xmm7,%xmm0\n"
    "pand   %xmm7,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,(%edx)\n"
    "lea    0x10(%edx),%edx\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "ret\n"
);

extern "C" void YUY2ToI420RowUV_SSE2(const uint8* src_yuy2, int pitch_yuy2,
                                     uint8* dst_u, uint8* dst_y, int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _YUY2ToI420RowUV_SSE2\n"
"_YUY2ToI420RowUV_SSE2:\n"
#else
    ".global YUY2ToI420RowUV_SSE2\n"
"YUY2ToI420RowUV_SSE2:\n"
#endif
    "push   %esi\n"
    "push   %edi\n"
    "mov    0xc(%esp),%eax\n"
    "mov    0x10(%esp),%esi\n"
    "mov    0x14(%esp),%edx\n"
    "mov    0x18(%esp),%edi\n"
    "mov    0x1c(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlw  $0x8,%xmm7\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "movdqa (%eax,%esi,1),%xmm2\n"
    "movdqa 0x10(%eax,%esi,1),%xmm3\n"
    "lea    0x20(%eax),%eax\n"
    "pavgb  %xmm2,%xmm0\n"
    "pavgb  %xmm3,%xmm1\n"
    "psrlw  $0x8,%xmm0\n"
    "psrlw  $0x8,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,%xmm1\n"
    "pand   %xmm7,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,(%edx)\n"
    "lea    0x8(%edx),%edx\n"
    "psrlw  $0x8,%xmm1\n"
    "packuswb %xmm1,%xmm1\n"
    "movq   %xmm1,(%edi)\n"
    "lea    0x8(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"
);

#define HAS_UYVYTOI420ROW_SSE2
extern "C" void UYVYToI420RowY_SSE2(const uint8* src_uyvy,
                                    uint8* dst_y, int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _UYVYToI420RowY_SSE2\n"
"_UYVYToI420RowY_SSE2:\n"
#else
    ".global UYVYToI420RowY_SSE2\n"
"UYVYToI420RowY_SSE2:\n"
#endif
    "mov    0x4(%esp),%eax\n"
    "mov    0x8(%esp),%edx\n"
    "mov    0xc(%esp),%ecx\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "lea    0x20(%eax),%eax\n"
    "psrlw  $0x8,%xmm0\n"
    "psrlw  $0x8,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,(%edx)\n"
    "lea    0x10(%edx),%edx\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "ret\n"
);

extern "C" void UYVYToI420RowUV_SSE2(const uint8* src_uyvy, int pitch_uyvy,
                                     uint8* dst_u, uint8* dst_y, int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _UYVYToI420RowUV_SSE2\n"
"_UYVYToI420RowUV_SSE2:\n"
#else
    ".global UYVYToI420RowUV_SSE2\n"
"UYVYToI420RowUV_SSE2:\n"
#endif
    "push   %esi\n"
    "push   %edi\n"
    "mov    0xc(%esp),%eax\n"
    "mov    0x10(%esp),%esi\n"
    "mov    0x14(%esp),%edx\n"
    "mov    0x18(%esp),%edi\n"
    "mov    0x1c(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlw  $0x8,%xmm7\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "movdqa (%eax,%esi,1),%xmm2\n"
    "movdqa 0x10(%eax,%esi,1),%xmm3\n"
    "lea    0x20(%eax),%eax\n"
    "pavgb  %xmm2,%xmm0\n"
    "pavgb  %xmm3,%xmm1\n"
    "pand   %xmm7,%xmm0\n"
    "pand   %xmm7,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,%xmm1\n"
    "pand   %xmm7,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,(%edx)\n"
    "lea    0x8(%edx),%edx\n"
    "psrlw  $0x8,%xmm1\n"
    "packuswb %xmm1,%xmm1\n"
    "movq   %xmm1,(%edi)\n"
    "lea    0x8(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"
);
#endif

void YUY2ToI420RowUV_C(const uint8* src_yuy2, int src_pitch_yuy2,
                       uint8* dst_u, uint8* dst_v, int pix) {
  // Copy a row of yuy2 UV values
  for (int x = 0; x < pix; x += 2) {
    dst_u[0] = (src_yuy2[1] + src_yuy2[src_pitch_yuy2 + 1] + 1) >> 1;
    dst_v[0] = (src_yuy2[3] + src_yuy2[src_pitch_yuy2 + 3] + 1) >> 1;
    src_yuy2 += 4;
    dst_u += 1;
    dst_v += 1;
  }
}

void YUY2ToI420RowY_C(const uint8* src_yuy2,
                      uint8* dst_y, int pix) {
  // Copy a row of yuy2 Y values
  for (int x = 0; x < pix; ++x) {
    dst_y[0] = src_yuy2[0];
    src_yuy2 += 2;
    dst_y += 1;
  }
}

void UYVYToI420RowUV_C(const uint8* src_uyvy, int src_pitch_uyvy,
                       uint8* dst_u, uint8* dst_v, int pix) {
  // Copy a row of uyvy UV values
  for (int x = 0; x < pix; x += 2) {
    dst_u[0] = (src_uyvy[0] + src_uyvy[src_pitch_uyvy + 0] + 1) >> 1;
    dst_v[0] = (src_uyvy[2] + src_uyvy[src_pitch_uyvy + 2] + 1) >> 1;
    src_uyvy += 4;
    dst_u += 1;
    dst_v += 1;
  }
}

void UYVYToI420RowY_C(const uint8* src_uyvy,
                      uint8* dst_y, int pix) {
  // Copy a row of uyvy Y values
  for (int x = 0; x < pix; ++x) {
    dst_y[0] = src_uyvy[1];
    src_uyvy += 2;
    dst_y += 1;
  }
}

// Convert YUY2 to I420.
void YUY2ToI420(const uint8* src_yuy2, int src_pitch_yuy2,
                uint8* dst_y, int dst_pitch_y,
                uint8* dst_u, int dst_pitch_u,
                uint8* dst_v, int dst_pitch_v,
                int width, int height) {
  void (*YUY2ToI420RowUV)(const uint8* src_yuy2, int src_pitch_yuy2,
                          uint8* dst_u, uint8* dst_v, int pix);
  void (*YUY2ToI420RowY)(const uint8* src_yuy2,
                         uint8* dst_y, int pix);
#if defined(HAS_YUY2TOI420ROW_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_yuy2, 16) && (src_pitch_yuy2 % 16 == 0) &&
      IS_ALIGNED(dst_y, 16) && (dst_pitch_y % 16 == 0) &&
      IS_ALIGNED(dst_u, 8) && (dst_pitch_u % 8 == 0) &&
      IS_ALIGNED(dst_v, 8) && (dst_pitch_v % 8 == 0)) {
    YUY2ToI420RowY = YUY2ToI420RowY_SSE2;
    YUY2ToI420RowUV = YUY2ToI420RowUV_SSE2;
  } else
#endif
  {
    YUY2ToI420RowY = YUY2ToI420RowY_C;
    YUY2ToI420RowUV = YUY2ToI420RowUV_C;
  }
  for (int y = 0; y < height; ++y) {
    if ((y & 1) == 0) {
      if (y >= (height - 1) ) {  // last chroma on odd height clamp height
        src_pitch_yuy2 = 0;
      }
      YUY2ToI420RowUV(src_yuy2, src_pitch_yuy2, dst_u, dst_v, width);
      dst_u += dst_pitch_u;
      dst_v += dst_pitch_v;
    }
    YUY2ToI420RowY(src_yuy2, dst_y, width);
    dst_y += dst_pitch_y;
    src_yuy2 += src_pitch_yuy2;
  }
}

// Convert UYVY to I420.
void UYVYToI420(const uint8* src_uyvy, int src_pitch_uyvy,
                uint8* dst_y, int dst_pitch_y,
                uint8* dst_u, int dst_pitch_u,
                uint8* dst_v, int dst_pitch_v,
                int width, int height) {
  void (*UYVYToI420RowUV)(const uint8* src_uyvy, int src_pitch_uyvy,
                          uint8* dst_u, uint8* dst_v, int pix);
  void (*UYVYToI420RowY)(const uint8* src_uyvy,
                         uint8* dst_y, int pix);
#if defined(HAS_UYVYTOI420ROW_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (width % 16 == 0) &&
      IS_ALIGNED(src_uyvy, 16) && (src_pitch_uyvy % 16 == 0) &&
      IS_ALIGNED(dst_y, 16) && (dst_pitch_y % 16 == 0) &&
      IS_ALIGNED(dst_u, 8) && (dst_pitch_u % 8 == 0) &&
      IS_ALIGNED(dst_v, 8) && (dst_pitch_v % 8 == 0)) {
    UYVYToI420RowY = UYVYToI420RowY_SSE2;
    UYVYToI420RowUV = UYVYToI420RowUV_SSE2;
  } else
#endif
  {
    UYVYToI420RowY = UYVYToI420RowY_C;
    UYVYToI420RowUV = UYVYToI420RowUV_C;
  }
  for (int y = 0; y < height; ++y) {
    if ((y & 1) == 0) {
      if (y >= (height - 1) ) {  // last chroma on odd height clamp height
        src_pitch_uyvy = 0;
      }
      UYVYToI420RowUV(src_uyvy, src_pitch_uyvy, dst_u, dst_v, width);
      dst_u += dst_pitch_u;
      dst_v += dst_pitch_v;
    }
    UYVYToI420RowY(src_uyvy, dst_y, width);
    dst_y += dst_pitch_y;
    src_uyvy += src_pitch_uyvy;
  }
}

// Convert I420 to ARGB.
// TODO(fbarchard): Add SSSE3 version and supply C version for fallback.
void I420ToARGB(const uint8* src_y, int src_pitch_y,
                const uint8* src_u, int src_pitch_u,
                const uint8* src_v, int src_pitch_v,
                uint8* dst_argb, int dst_pitch_argb,
                int width, int height) {
  for (int y = 0; y < height; ++y) {
    FastConvertYUVToRGB32Row(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_pitch_argb;
    src_y += src_pitch_y;
    if (y & 1) {
      src_u += src_pitch_u;
      src_v += src_pitch_v;
    }
  }
  // MMX used for FastConvertYUVToRGB32Row requires an emms instruction.
  EMMS();
}

// Convert I420 to BGRA.
void I420ToBGRA(const uint8* src_y, int src_pitch_y,
                const uint8* src_u, int src_pitch_u,
                const uint8* src_v, int src_pitch_v,
                uint8* dst_argb, int dst_pitch_argb,
                int width, int height) {
  for (int y = 0; y < height; ++y) {
    FastConvertYUVToBGRARow(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_pitch_argb;
    src_y += src_pitch_y;
    if (y & 1) {
      src_u += src_pitch_u;
      src_v += src_pitch_v;
    }
  }
  EMMS();
}

// Convert I420 to BGRA.
void I420ToABGR(const uint8* src_y, int src_pitch_y,
                const uint8* src_u, int src_pitch_u,
                const uint8* src_v, int src_pitch_v,
                uint8* dst_argb, int dst_pitch_argb,
                int width, int height) {
  for (int y = 0; y < height; ++y) {
    FastConvertYUVToABGRRow(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_pitch_argb;
    src_y += src_pitch_y;
    if (y & 1) {
      src_u += src_pitch_u;
      src_v += src_pitch_v;
    }
  }
  EMMS();
}

// Convert I422 to ARGB.
void I422ToARGB(const uint8* src_y, int src_pitch_y,
                const uint8* src_u, int src_pitch_u,
                const uint8* src_v, int src_pitch_v,
                uint8* dst_argb, int dst_pitch_argb,
                int width, int height) {
  for (int y = 0; y < height; ++y) {
    FastConvertYUVToRGB32Row(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_pitch_argb;
    src_y += src_pitch_y;
    src_u += src_pitch_u;
    src_v += src_pitch_v;
  }
  // MMX used for FastConvertYUVToRGB32Row requires an emms instruction.
  EMMS();
}

// Convert I444 to ARGB.
void I444ToARGB(const uint8* src_y, int src_pitch_y,
                const uint8* src_u, int src_pitch_u,
                const uint8* src_v, int src_pitch_v,
                uint8* dst_argb, int dst_pitch_argb,
                int width, int height) {
  for (int y = 0; y < height; ++y) {
    FastConvertYUV444ToRGB32Row(src_y, src_u, src_v, dst_argb, width);
    dst_argb += dst_pitch_argb;
    src_y += src_pitch_y;
    src_u += src_pitch_u;
    src_v += src_pitch_v;
  }
  // MMX used for FastConvertYUVToRGB32Row requires an emms instruction.
  EMMS();
}

// Convert I400 to ARGB.
void I400ToARGB_Reference(const uint8* src_y, int src_pitch_y,
                          uint8* dst_argb, int dst_pitch_argb,
                          int width, int height) {
  for (int y = 0; y < height; ++y) {
    FastConvertYToRGB32Row(src_y, dst_argb, width);
    dst_argb += dst_pitch_argb;
    src_y += src_pitch_y;
  }
  // MMX used for FastConvertYUVToRGB32Row requires an emms instruction.
  EMMS();
}

// TODO(fbarchard): 64 bit version
#if defined(WIN32) && !defined(COVERAGE_ENABLED)

#define HAS_I400TOARGBROW_SSE2
__declspec(naked)
static void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  __asm {
    mov        eax, [esp + 4]        // src_y
    mov        edx, [esp + 8]        // dst_argb
    mov        ecx, [esp + 12]       // pix
    pcmpeqb    xmm7, xmm7            // generate mask 0xff000000
    pslld      xmm7, 24

  wloop:
    movq       xmm0, qword ptr [eax]
    lea        eax,  [eax + 8]
    punpcklbw  xmm0, xmm0
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm0
    punpckhwd  xmm1, xmm1
    por        xmm0, xmm7
    por        xmm1, xmm7
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx, [edx + 32]
    sub        ecx, 8
    ja         wloop
    ret
  }
}

#elif defined(__i386__) && !defined(COVERAGE_ENABLED) && \
    !TARGET_IPHONE_SIMULATOR

#define HAS_I400TOARGBROW_SSE2
extern "C" void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb,
                                   int pix);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _I400ToARGBRow_SSE2\n"
"_I400ToARGBRow_SSE2:\n"
#else
    ".global I400ToARGBRow_SSE2\n"
"I400ToARGBRow_SSE2:\n"
#endif
    "mov    0x4(%esp),%eax\n"
    "mov    0x8(%esp),%edx\n"
    "mov    0xc(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "pslld  $0x18,%xmm7\n"
"1:"
    "movq   (%eax),%xmm0\n"
    "lea    0x8(%eax),%eax\n"
    "punpcklbw %xmm0,%xmm0\n"
    "movdqa %xmm0,%xmm1\n"
    "punpcklwd %xmm0,%xmm0\n"
    "punpckhwd %xmm1,%xmm1\n"
    "por    %xmm7,%xmm0\n"
    "por    %xmm7,%xmm1\n"
    "movdqa %xmm0,(%edx)\n"
    "movdqa %xmm1,0x10(%edx)\n"
    "lea    0x20(%edx),%edx\n"
    "sub    $0x8,%ecx\n"
    "ja     1b\n"
    "ret\n"
);
#endif

static void I400ToARGBRow_C(const uint8* src_y, uint8* dst_argb, int pix) {
  // Copy a Y to RGB.
  for (int x = 0; x < pix; ++x) {
    dst_argb[2] = dst_argb[1] = dst_argb[0] = src_y[0];
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_y += 1;
  }
}

// Convert I400 to ARGB.
void I400ToARGB(const uint8* src_y, int src_pitch_y,
                uint8* dst_argb, int dst_pitch_argb,
                int width, int height) {
  void (*I400ToARGBRow)(const uint8* src_y, uint8* dst_argb, int pix);
#if defined(HAS_I400TOARGBROW_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (width % 8 == 0) &&
      IS_ALIGNED(src_y, 8) && (src_pitch_y % 8 == 0) &&
      IS_ALIGNED(dst_argb, 16) && (dst_pitch_argb % 16 == 0)) {
    I400ToARGBRow = I400ToARGBRow_SSE2;
  } else
#endif
  {
    I400ToARGBRow = I400ToARGBRow_C;
  }

  for (int y = 0; y < height; ++y) {
    I400ToARGBRow(src_y, dst_argb, width);
    src_y += src_pitch_y;
    dst_argb += dst_pitch_argb;
  }
}

static void RAWToARGBRow_C(const uint8* src_raw, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    dst_argb[0] = src_raw[2];
    dst_argb[1] = src_raw[1];
    dst_argb[2] = src_raw[0];
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_raw += 3;
  }
}

// Convert RAW to ARGB.
void RAWToARGB(const uint8* src_raw, int src_pitch_raw,
               uint8* dst_argb, int dst_pitch_argb,
               int width, int height) {
  for (int y = 0; y < height; ++y) {
    RAWToARGBRow_C(src_raw, dst_argb, width);
    src_raw += src_pitch_raw;
    dst_argb += dst_pitch_argb;
  }
}

static void BG24ToARGBRow_C(const uint8* src_bg24, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    dst_argb[0] = src_bg24[0];
    dst_argb[1] = src_bg24[1];
    dst_argb[2] = src_bg24[2];
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_bg24 += 3;
  }
}

// Convert BG24 to ARGB.
void BG24ToARGB(const uint8* src_bg24, int src_pitch_bg24,
                uint8* dst_argb, int dst_pitch_argb,
                int width, int height) {
  for (int y = 0; y < height; ++y) {
    BG24ToARGBRow_C(src_bg24, dst_argb, width);
    src_bg24 += src_pitch_bg24;
    dst_argb += dst_pitch_argb;
  }
}

static void ABGRToARGBRow_C(const uint8* src_abgr, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    dst_argb[0] = src_abgr[2];
    dst_argb[1] = src_abgr[1];
    dst_argb[2] = src_abgr[0];
    dst_argb[3] = src_abgr[3];
    dst_argb += 4;
    src_abgr += 4;
  }
}

// Convert ABGR to ARGB.
void ABGRToARGB(const uint8* src_abgr, int src_pitch_abgr,
                uint8* dst_argb, int dst_pitch_argb,
                int width, int height) {
  for (int y = 0; y < height; ++y) {
    ABGRToARGBRow_C(src_abgr, dst_argb, width);
    src_abgr += src_pitch_abgr;
    dst_argb += dst_pitch_argb;
  }
}

}  // namespace libyuv

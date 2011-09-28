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

#elif defined(WIN32) && !defined(COVERAGE_ENABLED)
#define HAS_SPLITUV_SSE2
static void SplitUV_SSE2(const uint8* src_uv,
                         uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    mov        esi, src_uv
    mov        edi, dst_u
    mov        edx, dst_v
    mov        ecx, pix
    mov        eax, 0x00ff00ff       // mask for isolating low bytes
    movd       xmm7, eax
    pshufd     xmm7, xmm7, 0

  wloop:
    movdqa     xmm0, [esi]
    movdqa     xmm1, [esi + 16]
    lea        esi,  [esi + 32]
    movdqa     xmm2, xmm0
    movdqa     xmm3, xmm1
    pand       xmm0, xmm7   // even bytes
    pand       xmm1, xmm7
    packuswb   xmm0, xmm1
    movdqa     [edi], xmm0
    lea        edi, [edi + 16]
    psrlw      xmm2, 8      // odd bytes
    psrlw      xmm3, 8
    packuswb   xmm2, xmm3
    movdqa     [edx], xmm2
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         wloop
  }
}

#elif defined(__i386__) && !defined(COVERAGE_ENABLED) && \
    !TARGET_IPHONE_SIMULATOR

// GCC version is same as Visual C

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
    "push   %ebp\n"
    "mov    %esp,%ebp\n"
    "push   %esi\n"
    "push   %edi\n"
    "mov    0x8(%ebp),%esi\n"
    "mov    0xc(%ebp),%edi\n"
    "mov    0x10(%ebp),%edx\n"
    "mov    0x14(%ebp),%ecx\n"
    "mov    $0xff00ff,%eax\n"
    "movd   %eax,%xmm7\n"
    "pshufd $0x0,%xmm7,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa 0x10(%esi),%xmm1\n"
    "lea    0x20(%esi),%esi\n"
    "movdqa %xmm0,%xmm2\n"
    "movdqa %xmm1,%xmm3\n"
    "pand   %xmm7,%xmm0\n"
    "pand   %xmm7,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "psrlw  $0x8,%xmm2\n"
    "psrlw  $0x8,%xmm3\n"
    "packuswb %xmm3,%xmm2\n"
    "movdqa %xmm2,(%edx)\n"
    "lea    0x10(%edx),%edx\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "pop    %ebp\n"
    "ret\n"
);
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

// Support converting from FOURCC_M420
// Useful for bandwidth constrained transports like USB 1.0 and 2.0 and for
// easy conversion to I420.
// M420 format description:
// M420 is row biplanar 420: 2 rows of Y and 1 row of VU.
// Chroma is half width / half height. (420)
// pitch_m420 is row planar.  Normally this will be the width in pixels.
//   The UV plane is half width, but 2 values, so pitch_m420 applies to this
//   as well as the two Y planes.
// TODO(fbarchard): Do NV21/NV12 formats with this function
static void X420ToI420(uint8* dst_y, int dst_pitch_y,
                       uint8* dst_u, int dst_pitch_u,
                       uint8* dst_v, int dst_pitch_v,
                       const uint8* src_y,
                       int src_pitch_y0, int src_pitch_y1,
                       const uint8* src_uv, int src_pitch_uv,
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

// TODO(fbarchard): For biplanar formats (ie NV21), the Y plane is the same
// as I420, and only the chroma plane varies. Copy the Y plane by reference,
// and just convert the UV.  This method can be used for NV21, NV12, I420,
// I422, M422.  8 of the 12 bits is Y, so this would copy 3 times less data,
// which is approximately how much faster it would be.

// Helper function to copy yuv data without scaling.  Used
// by our jpeg conversion callbacks to incrementally fill a yuv image.
void PlanarFunctions::I420Copy(const uint8* src_y, int src_pitch_y,
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
void PlanarFunctions::I422ToI420(const uint8* src_y, int src_pitch_y,
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

// Convert M420 to I420.
void PlanarFunctions::M420ToI420(uint8* dst_y, int dst_pitch_y,
                                 uint8* dst_u, int dst_pitch_u,
                                 uint8* dst_v, int dst_pitch_v,
                                 const uint8* m420, int pitch_m420,
                                 int width, int height) {
  X420ToI420(dst_y, dst_pitch_y, dst_u, dst_pitch_u, dst_v, dst_pitch_v,
             m420, pitch_m420, pitch_m420 * 2,
             m420 + pitch_m420 * 2, pitch_m420 * 3,
             width, height);
}

// Convert NV12 to I420.
void PlanarFunctions::NV12ToI420(uint8* dst_y, int dst_pitch_y,
                                 uint8* dst_u, int dst_pitch_u,
                                 uint8* dst_v, int dst_pitch_v,
                                 const uint8* src_y,
                                 const uint8* src_uv,
                                 int src_pitch,
                                 int width, int height) {
  X420ToI420(dst_y, dst_pitch_y, dst_u, dst_pitch_u, dst_v, dst_pitch_v,
             src_y, src_pitch, src_pitch,
             src_uv, src_pitch,
             width, height);
}

}  // namespace libyuv

/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#ifdef HAS_ARGBTOYROW_SSSE3

// Constant multiplication table for converting ARGB to I400.
static const vec8 kARGBToY = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};

static const vec8 kARGBToU = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

static const vec8 kARGBToV = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};

// Constants for BGRA
static const vec8 kBGRAToY = {
  0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13
};

static const vec8 kBGRAToU = {
  0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112
};

static const vec8 kBGRAToV = {
  0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18
};

// Constants for ABGR
static const vec8 kABGRToY = {
  33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0
};

static const vec8 kABGRToU = {
  -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0
};

static const vec8 kABGRToV = {
  112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0
};

static const uvec8 kAddY16 = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u
};

static const uvec8 kAddUV128 = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};

// Shuffle table for converting RGB24 to ARGB.
static const uvec8 kShuffleMaskRGB24ToARGB = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};

// Shuffle table for converting RAW to ARGB.
static const uvec8 kShuffleMaskRAWToARGB = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};

// Shuffle table for converting ABGR to ARGB.
static const uvec8 kShuffleMaskABGRToARGB = {
  2u, 1u, 0u, 3u, 6u, 5u, 4u, 7u, 10u, 9u, 8u, 11u, 14u, 13u, 12u, 15u
};

// Shuffle table for converting BGRA to ARGB.
static const uvec8 kShuffleMaskBGRAToARGB = {
  3u, 2u, 1u, 0u, 7u, 6u, 5u, 4u, 11u, 10u, 9u, 8u, 15u, 14u, 13u, 12u
};

// Shuffle table for converting ARGB to RGB24.
static const uvec8 kShuffleMaskARGBToRGB24 = {
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u, 128u, 128u, 128u, 128u };


// Shuffle table for converting ARGB to RAW.
static const uvec8 kShuffleMaskARGBToRAW = {
  2u, 1u,0u, 6u, 5u, 4u, 10u, 9u, 8u, 14u, 13u, 12u, 128u, 128u, 128u, 128u };

__declspec(naked)
void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  __asm {
    mov        eax, [esp + 4]        // src_y
    mov        edx, [esp + 8]        // dst_argb
    mov        ecx, [esp + 12]       // pix
    pcmpeqb    xmm5, xmm5            // generate mask 0xff000000
    pslld      xmm5, 24

  convertloop:
    movq       xmm0, qword ptr [eax]
    lea        eax,  [eax + 8]
    punpcklbw  xmm0, xmm0
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm0
    punpckhwd  xmm1, xmm1
    por        xmm0, xmm5
    por        xmm1, xmm5
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx, [edx + 32]
    sub        ecx, 8
    ja         convertloop
    ret
  }
}

__declspec(naked)
void ABGRToARGBRow_SSSE3(const uint8* src_abgr, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_abgr
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    movdqa    xmm5, kShuffleMaskABGRToARGB

 convertloop:
    movdqa    xmm0, [eax]
    lea       eax, [eax + 16]
    pshufb    xmm0, xmm5
    movdqa    [edx], xmm0
    lea       edx, [edx + 16]
    sub       ecx, 4
    ja        convertloop
    ret
  }
}

__declspec(naked)
void BGRAToARGBRow_SSSE3(const uint8* src_bgra, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_bgra
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    movdqa    xmm5, kShuffleMaskBGRAToARGB

 convertloop:
    movdqa    xmm0, [eax]
    lea       eax, [eax + 16]
    pshufb    xmm0, xmm5
    movdqa    [edx], xmm0
    lea       edx, [edx + 16]
    sub       ecx, 4
    ja        convertloop
    ret
  }
}

__declspec(naked)
void RGB24ToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_rgb24
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm5, xmm5       // generate mask 0xff000000
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskRGB24ToARGB

 convertloop:
    movdqu    xmm0, [eax]
    movdqu    xmm1, [eax + 16]
    movdqu    xmm3, [eax + 32]
    lea       eax, [eax + 48]
    movdqa    xmm2, xmm3
    palignr   xmm2, xmm1, 8    // xmm2 = { xmm3[0:3] xmm1[8:15]}
    pshufb    xmm2, xmm4
    por       xmm2, xmm5
    palignr   xmm1, xmm0, 12   // xmm1 = { xmm3[0:7] xmm0[12:15]}
    pshufb    xmm0, xmm4
    movdqa    [edx + 32], xmm2
    por       xmm0, xmm5
    pshufb    xmm1, xmm4
    movdqa    [edx], xmm0
    por       xmm1, xmm5
    palignr   xmm3, xmm3, 4    // xmm3 = { xmm3[4:15]}
    pshufb    xmm3, xmm4
    movdqa    [edx + 16], xmm1
    por       xmm3, xmm5
    movdqa    [edx + 48], xmm3
    lea       edx, [edx + 64]
    sub       ecx, 16
    ja        convertloop
    ret
  }
}

__declspec(naked)
void RAWToARGBRow_SSSE3(const uint8* src_raw, uint8* dst_argb,
                        int pix) {
__asm {
    mov       eax, [esp + 4]   // src_raw
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm5, xmm5       // generate mask 0xff000000
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskRAWToARGB

 convertloop:
    movdqu    xmm0, [eax]
    movdqu    xmm1, [eax + 16]
    movdqu    xmm3, [eax + 32]
    lea       eax, [eax + 48]
    movdqa    xmm2, xmm3
    palignr   xmm2, xmm1, 8    // xmm2 = { xmm3[0:3] xmm1[8:15]}
    pshufb    xmm2, xmm4
    por       xmm2, xmm5
    palignr   xmm1, xmm0, 12   // xmm1 = { xmm3[0:7] xmm0[12:15]}
    pshufb    xmm0, xmm4
    movdqa    [edx + 32], xmm2
    por       xmm0, xmm5
    pshufb    xmm1, xmm4
    movdqa    [edx], xmm0
    por       xmm1, xmm5
    palignr   xmm3, xmm3, 4    // xmm3 = { xmm3[4:15]}
    pshufb    xmm3, xmm4
    movdqa    [edx + 16], xmm1
    por       xmm3, xmm5
    movdqa    [edx + 48], xmm3
    lea       edx, [edx + 64]
    sub       ecx, 16
    ja        convertloop
    ret
  }
}

// pmul method to replicate bits
// Math to replicate bits
// (v << 8) | (v << 3)
// v * 256 + v * 8
// v * (256 + 8)
// G shift of 5 is incorporated, so shift is 5 + 8 and 5 + 3
// 20 instructions
__declspec(naked)
void RGB565ToARGBRow_SSE2(const uint8* src_rgb565, uint8* dst_argb,
                          int pix) {
__asm {
    mov       eax, 0x01080108  // generate multiplier to repeat 5 bits
    movd      xmm5, eax
    pshufd    xmm5, xmm5, 0
    mov       eax, 0x20082008  // multiplier shift by 5 and then repeat 6 bits
    movd      xmm6, eax
    pshufd    xmm6, xmm6, 0
    pcmpeqb   xmm3, xmm3       // generate mask 0xf800f800 for Red
    psllw     xmm3, 11
    pcmpeqb   xmm4, xmm4       // generate mask 0x07e007e0 for Green
    psllw     xmm4, 10
    psrlw     xmm4, 5
    pcmpeqb   xmm7, xmm7       // generate mask 0xff00ff00 for Alpha
    psllw     xmm7, 8

    mov       eax, [esp + 4]   // src_rgb565
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    sub       edx, eax
    sub       edx, eax

 convertloop:
    movdqu    xmm0, [eax]   // fetch 8 pixels of bgr565
    movdqa    xmm1, xmm0
    movdqa    xmm2, xmm0
    pand      xmm1, xmm3    // R in upper 5 bits
    psllw     xmm2, 11      // B in upper 5 bits
    pmulhuw   xmm1, xmm5    // * (256 + 8)
    pmulhuw   xmm2, xmm5    // * (256 + 8)
    psllw     xmm1, 8
    por       xmm1, xmm2    // RB
    pand      xmm0, xmm4    // G in middle 6 bits
    pmulhuw   xmm0, xmm6    // << 5 * (256 + 4)
    por       xmm0, xmm7    // AG
    movdqa    xmm2, xmm1
    punpcklbw xmm1, xmm0
    punpckhbw xmm2, xmm0
    movdqa    [eax * 2 + edx], xmm1  // store 4 pixels of ARGB
    movdqa    [eax * 2 + edx + 16], xmm2  // store next 4 pixels of ARGB
    lea       eax, [eax + 16]
    sub       ecx, 8
    ja        convertloop
    ret
  }
}

// TODO(fbarchard): Port ARGB1555ToARGBRow_SSE2 to gcc
// 24 instructions
__declspec(naked)
void ARGB1555ToARGBRow_SSE2(const uint8* src_argb1555, uint8* dst_argb,
                            int pix) {
__asm {
    mov       eax, 0x01080108  // generate multiplier to repeat 5 bits
    movd      xmm5, eax
    pshufd    xmm5, xmm5, 0
    mov       eax, 0x42004200  // multiplier shift by 6 and then repeat 5 bits
    movd      xmm6, eax
    pshufd    xmm6, xmm6, 0
    pcmpeqb   xmm3, xmm3       // generate mask 0xf800f800 for Red
    psllw     xmm3, 11
    movdqa    xmm4, xmm3       // generate mask 0x03e003e0 for Green
    psrlw     xmm4, 6
    pcmpeqb   xmm7, xmm7       // generate mask 0xff00ff00 for Alpha
    psllw     xmm7, 8

    mov       eax, [esp + 4]   // src_argb1555
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    sub       edx, eax
    sub       edx, eax

 convertloop:
    movdqu    xmm0, [eax]   // fetch 8 pixels of 1555
    movdqa    xmm1, xmm0
    movdqa    xmm2, xmm0
    psllw     xmm1, 1       // R in upper 5 bits
    psllw     xmm2, 11      // B in upper 5 bits
    pand      xmm1, xmm3
    pmulhuw   xmm2, xmm5    // * (256 + 8)
    pmulhuw   xmm1, xmm5    // * (256 + 8)
    psllw     xmm1, 8
    por       xmm1, xmm2    // RB
    movdqa    xmm2, xmm0
    pand      xmm0, xmm4    // G in middle 5 bits
    psraw     xmm2, 8       // A
    pmulhuw   xmm0, xmm6    // << 6 * (256 + 8)
    pand      xmm2, xmm7
    por       xmm0, xmm2    // AG
    movdqa    xmm2, xmm1
    punpcklbw xmm1, xmm0
    punpckhbw xmm2, xmm0
    movdqa    [eax * 2 + edx], xmm1  // store 4 pixels of ARGB
    movdqa    [eax * 2 + edx + 16], xmm2  // store next 4 pixels of ARGB
    lea       eax, [eax + 16]
    sub       ecx, 8
    ja        convertloop
    ret
  }
}

// TODO(fbarchard): Port ARGB4444ToARGBRow_SSE2 to gcc
// 18 instructions
__declspec(naked)
void ARGB4444ToARGBRow_SSE2(const uint8* src_argb4444, uint8* dst_argb,
                            int pix) {
__asm {
    mov       eax, 0x0f0f0f0f  // generate mask 0x0f0f0f0f
    movd      xmm4, eax
    pshufd    xmm4, xmm4, 0
    movdqa    xmm5, xmm4       // 0xf0f0f0f0 for high nibbles
    pslld     xmm5, 4
    mov       eax, [esp + 4]   // src_argb4444
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    sub       edx, eax
    sub       edx, eax

 convertloop:
    movdqu    xmm0, [eax]   // fetch 8 pixels of bgra4444
    movdqa    xmm2, xmm0
    pand      xmm0, xmm4    // mask low nibbles
    pand      xmm2, xmm5    // mask high nibbles
    movdqa    xmm1, xmm0
    movdqa    xmm3, xmm2
    psllw     xmm1, 4
    psrlw     xmm3, 4
    por       xmm0, xmm1
    por       xmm2, xmm3
    movdqa    xmm1, xmm0
    punpcklbw xmm0, xmm2
    punpckhbw xmm1, xmm2
    movdqa    [eax * 2 + edx], xmm0  // store 4 pixels of ARGB
    movdqa    [eax * 2 + edx + 16], xmm1  // store next 4 pixels of ARGB
    lea       eax, [eax + 16]
    sub       ecx, 8
    ja        convertloop
    ret
  }
}

// TODO(fbarchard): Port to gcc
__declspec(naked)
void ARGBToRGB24Row_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_argb
    mov       edx, [esp + 8]   // dst_rgb
    mov       ecx, [esp + 12]  // pix
    movdqa    xmm6, kShuffleMaskARGBToRGB24

 convertloop:
    movdqa    xmm0, [eax]   // fetch 16 pixels of argb
    movdqa    xmm1, [eax + 16]
    movdqa    xmm2, [eax + 32]
    movdqa    xmm3, [eax + 48]
    lea       eax, [eax + 64]
    pshufb    xmm0, xmm6    // pack 16 bytes of ARGB to 12 bytes of RGB
    pshufb    xmm1, xmm6
    pshufb    xmm2, xmm6
    pshufb    xmm3, xmm6
    movdqa    xmm4, xmm1   // 4 bytes from 1 for 0
    psrldq    xmm1, 4      // 8 bytes from 1
    pslldq    xmm4, 12     // 4 bytes from 1 for 0
    movdqa    xmm5, xmm2   // 8 bytes from 2 for 1
    por       xmm0, xmm4   // 4 bytes from 1 for 0
    pslldq    xmm5, 8      // 8 bytes from 2 for 1
    movdqa    [edx], xmm0  // store 0
    por       xmm1, xmm5   // 8 bytes from 2 for 1
    psrldq    xmm2, 8      // 4 bytes from 2
    pslldq    xmm3, 4      // 12 bytes from 3 for 2
    por       xmm2, xmm3   // 12 bytes from 3 for 2
    movdqa    [edx + 16], xmm1   // store 1
    movdqa    [edx + 32], xmm2   // store 2
    lea       edx, [edx + 48]
    sub       ecx, 16
    ja        convertloop
    ret
  }
}

// TODO(fbarchard): Port to gcc
__declspec(naked)
void ARGBToRAWRow_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_argb
    mov       edx, [esp + 8]   // dst_rgb
    mov       ecx, [esp + 12]  // pix
    movdqa    xmm6, kShuffleMaskARGBToRAW

 convertloop:
    movdqa    xmm0, [eax]   // fetch 16 pixels of argb
    movdqa    xmm1, [eax + 16]
    movdqa    xmm2, [eax + 32]
    movdqa    xmm3, [eax + 48]
    lea       eax, [eax + 64]
    pshufb    xmm0, xmm6    // pack 16 bytes of ARGB to 12 bytes of RGB
    pshufb    xmm1, xmm6
    pshufb    xmm2, xmm6
    pshufb    xmm3, xmm6
    movdqa    xmm4, xmm1   // 4 bytes from 1 for 0
    psrldq    xmm1, 4      // 8 bytes from 1
    pslldq    xmm4, 12     // 4 bytes from 1 for 0
    movdqa    xmm5, xmm2   // 8 bytes from 2 for 1
    por       xmm0, xmm4   // 4 bytes from 1 for 0
    pslldq    xmm5, 8      // 8 bytes from 2 for 1
    movdqa    [edx], xmm0  // store 0
    por       xmm1, xmm5   // 8 bytes from 2 for 1
    psrldq    xmm2, 8      // 4 bytes from 2
    pslldq    xmm3, 4      // 12 bytes from 3 for 2
    por       xmm2, xmm3   // 12 bytes from 3 for 2
    movdqa    [edx + 16], xmm1   // store 1
    movdqa    [edx + 32], xmm2   // store 2
    lea       edx, [edx + 48]
    sub       ecx, 16
    ja        convertloop
    ret
  }
}

// TODO(fbarchard): Port to gcc
__declspec(naked)
void ARGBToRGB565Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_argb
    mov       edx, [esp + 8]   // dst_rgb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm3, xmm3       // generate mask 0x0000001f
    psrld     xmm3, 27
    pcmpeqb   xmm4, xmm4       // generate mask 0x000007e0
    psrld     xmm4, 26
    pslld     xmm4, 5
    pcmpeqb   xmm5, xmm5       // generate mask 0xfffff800
    pslld     xmm5, 11

 convertloop:
    movdqa    xmm0, [eax]   // fetch 4 pixels of argb
    movdqa    xmm1, xmm0    // B
    movdqa    xmm2, xmm0    // G
    pslld     xmm0, 8       // R
    psrld     xmm1, 3       // B
    psrld     xmm2, 5       // G
    psrad     xmm0, 16      // R
    pand      xmm1, xmm3    // B
    pand      xmm2, xmm4    // G
    pand      xmm0, xmm5    // R
    por       xmm1, xmm2    // BG
    por       xmm0, xmm1    // BGR
    packssdw  xmm0, xmm0
    lea       eax, [eax + 16]
    movq      qword ptr [edx], xmm0  // store 4 pixels of ARGB1555
    lea       edx, [edx + 8]
    sub       ecx, 4
    ja        convertloop
    ret
  }
}

// TODO(fbarchard): Port to gcc
// TODO(fbarchard): Improve sign extension/packing
__declspec(naked)
void ARGBToARGB1555Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_argb
    mov       edx, [esp + 8]   // dst_rgb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm4, xmm4       // generate mask 0x0000001f
    psrld     xmm4, 27
    movdqa    xmm5, xmm4       // generate mask 0x000003e0
    pslld     xmm5, 5
    movdqa    xmm6, xmm4       // generate mask 0x00007c00
    pslld     xmm6, 10
    pcmpeqb   xmm7, xmm7       // generate mask 0xffff8000
    pslld     xmm7, 15

 convertloop:
    movdqa    xmm0, [eax]   // fetch 4 pixels of argb
    movdqa    xmm1, xmm0    // B
    movdqa    xmm2, xmm0    // G
    movdqa    xmm3, xmm0    // R
    psrad     xmm0, 16      // A
    psrld     xmm1, 3       // B
    psrld     xmm2, 6       // G
    psrld     xmm3, 9       // R
    pand      xmm0, xmm7    // A
    pand      xmm1, xmm4    // B
    pand      xmm2, xmm5    // G
    pand      xmm3, xmm6    // R
    por       xmm0, xmm1    // BA
    por       xmm2, xmm3    // GR
    por       xmm0, xmm2    // BGRA
    packssdw  xmm0, xmm0
    lea       eax, [eax + 16]
    movq      qword ptr [edx], xmm0  // store 4 pixels of ARGB1555
    lea       edx, [edx + 8]
    sub       ecx, 4
    ja        convertloop
    ret
  }
}

// TODO(fbarchard): Port to gcc
__declspec(naked)
void ARGBToARGB4444Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_argb
    mov       edx, [esp + 8]   // dst_rgb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm4, xmm4       // generate mask 0xf000f000
    psllw     xmm4, 12
    movdqa    xmm3, xmm4       // generate mask 0x00f000f0
    psrlw     xmm3, 8

 convertloop:
    movdqa    xmm0, [eax]   // fetch 4 pixels of argb
    movdqa    xmm1, xmm0
    pand      xmm0, xmm3    // low nibble
    pand      xmm1, xmm4    // high nibble
    psrl      xmm0, 4
    psrl      xmm1, 8
    por       xmm0, xmm1
    packuswb  xmm0, xmm0
    lea       eax, [eax + 16]
    movq      qword ptr [edx], xmm0  // store 4 pixels of ARGB4444
    lea       edx, [edx + 8]
    sub       ecx, 4
    ja        convertloop
    ret
  }
}

// Convert 16 ARGB pixels (64 bytes) to 16 Y values
__declspec(naked)
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

 convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void ARGBToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

 convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void BGRAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

 convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void BGRAToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

 convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void ABGRToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

 convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void ABGRToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

 convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void ARGBToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // src_argb
    mov        esi, [esp + 8 + 8]   // src_stride_argb
    mov        edx, [esp + 8 + 12]  // dst_u
    mov        edi, [esp + 8 + 16]  // dst_v
    mov        ecx, [esp + 8 + 20]  // pix
    movdqa     xmm7, kARGBToU
    movdqa     xmm6, kARGBToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             // stride from u to v

 convertloop:
    /* step 1 - subsample 16x2 argb pixels to 8x1 */
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pavgb      xmm0, [eax + esi]
    pavgb      xmm1, [eax + esi + 16]
    pavgb      xmm2, [eax + esi + 32]
    pavgb      xmm3, [eax + esi + 48]
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    // step 2 - convert to U and V
    // from here down is very similar to Y code except
    // instead of 16 different pixels, its 8 pixels of U and 8 of V
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  // U
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  // V
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            // -> unsigned

    // step 3 - store 8 U and 8 V values
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop
    pop        edi
    pop        esi
    ret
  }
}


__declspec(naked)
void ARGBToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // src_argb
    mov        esi, [esp + 8 + 8]   // src_stride_argb
    mov        edx, [esp + 8 + 12]  // dst_u
    mov        edi, [esp + 8 + 16]  // dst_v
    mov        ecx, [esp + 8 + 20]  // pix
    movdqa     xmm7, kARGBToU
    movdqa     xmm6, kARGBToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             // stride from u to v

 convertloop:
    /* step 1 - subsample 16x2 argb pixels to 8x1 */
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    movdqu     xmm4, [eax + esi]
    pavgb      xmm0, xmm4
    movdqu     xmm4, [eax + esi + 16]
    pavgb      xmm1, xmm4
    movdqu     xmm4, [eax + esi + 32]
    pavgb      xmm2, xmm4
    movdqu     xmm4, [eax + esi + 48]
    pavgb      xmm3, xmm4
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    // step 2 - convert to U and V
    // from here down is very similar to Y code except
    // instead of 16 different pixels, its 8 pixels of U and 8 of V
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  // U
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  // V
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            // -> unsigned

    // step 3 - store 8 U and 8 V values
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop
    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void BGRAToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // src_argb
    mov        esi, [esp + 8 + 8]   // src_stride_argb
    mov        edx, [esp + 8 + 12]  // dst_u
    mov        edi, [esp + 8 + 16]  // dst_v
    mov        ecx, [esp + 8 + 20]  // pix
    movdqa     xmm7, kBGRAToU
    movdqa     xmm6, kBGRAToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             // stride from u to v

 convertloop:
    /* step 1 - subsample 16x2 argb pixels to 8x1 */
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pavgb      xmm0, [eax + esi]
    pavgb      xmm1, [eax + esi + 16]
    pavgb      xmm2, [eax + esi + 32]
    pavgb      xmm3, [eax + esi + 48]
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    // step 2 - convert to U and V
    // from here down is very similar to Y code except
    // instead of 16 different pixels, its 8 pixels of U and 8 of V
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  // U
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  // V
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            // -> unsigned

    // step 3 - store 8 U and 8 V values
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop
    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void BGRAToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // src_argb
    mov        esi, [esp + 8 + 8]   // src_stride_argb
    mov        edx, [esp + 8 + 12]  // dst_u
    mov        edi, [esp + 8 + 16]  // dst_v
    mov        ecx, [esp + 8 + 20]  // pix
    movdqa     xmm7, kBGRAToU
    movdqa     xmm6, kBGRAToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             // stride from u to v

 convertloop:
    /* step 1 - subsample 16x2 argb pixels to 8x1 */
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    movdqu     xmm4, [eax + esi]
    pavgb      xmm0, xmm4
    movdqu     xmm4, [eax + esi + 16]
    pavgb      xmm1, xmm4
    movdqu     xmm4, [eax + esi + 32]
    pavgb      xmm2, xmm4
    movdqu     xmm4, [eax + esi + 48]
    pavgb      xmm3, xmm4
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    // step 2 - convert to U and V
    // from here down is very similar to Y code except
    // instead of 16 different pixels, its 8 pixels of U and 8 of V
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  // U
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  // V
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            // -> unsigned

    // step 3 - store 8 U and 8 V values
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop
    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void ABGRToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // src_argb
    mov        esi, [esp + 8 + 8]   // src_stride_argb
    mov        edx, [esp + 8 + 12]  // dst_u
    mov        edi, [esp + 8 + 16]  // dst_v
    mov        ecx, [esp + 8 + 20]  // pix
    movdqa     xmm7, kABGRToU
    movdqa     xmm6, kABGRToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             // stride from u to v

 convertloop:
    /* step 1 - subsample 16x2 argb pixels to 8x1 */
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pavgb      xmm0, [eax + esi]
    pavgb      xmm1, [eax + esi + 16]
    pavgb      xmm2, [eax + esi + 32]
    pavgb      xmm3, [eax + esi + 48]
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    // step 2 - convert to U and V
    // from here down is very similar to Y code except
    // instead of 16 different pixels, its 8 pixels of U and 8 of V
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  // U
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  // V
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            // -> unsigned

    // step 3 - store 8 U and 8 V values
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop
    pop        edi
    pop        esi
    ret
  }
}


__declspec(naked)
void ABGRToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // src_argb
    mov        esi, [esp + 8 + 8]   // src_stride_argb
    mov        edx, [esp + 8 + 12]  // dst_u
    mov        edi, [esp + 8 + 16]  // dst_v
    mov        ecx, [esp + 8 + 20]  // pix
    movdqa     xmm7, kABGRToU
    movdqa     xmm6, kABGRToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             // stride from u to v

 convertloop:
    /* step 1 - subsample 16x2 argb pixels to 8x1 */
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    movdqu     xmm4, [eax + esi]
    pavgb      xmm0, xmm4
    movdqu     xmm4, [eax + esi + 16]
    pavgb      xmm1, xmm4
    movdqu     xmm4, [eax + esi + 32]
    pavgb      xmm2, xmm4
    movdqu     xmm4, [eax + esi + 48]
    pavgb      xmm3, xmm4
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    // step 2 - convert to U and V
    // from here down is very similar to Y code except
    // instead of 16 different pixels, its 8 pixels of U and 8 of V
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  // U
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  // V
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            // -> unsigned

    // step 3 - store 8 U and 8 V values
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop
    pop        edi
    pop        esi
    ret
  }
}

#ifdef HAS_I420TOARGBROW_SSSE3

#define YG 74 /* static_cast<int8>(1.164 * 64 + 0.5) */

#define UB 127 /* min(63,static_cast<int8>(2.018 * 64)) */
#define UG -25 /* static_cast<int8>(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* static_cast<int8>(-0.813 * 64 - 0.5) */
#define VR 102 /* static_cast<int8>(1.596 * 64 + 0.5) */

// Bias
#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

static const vec8 kUVToB = {
  UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB
};

static const vec8 kUVToR = {
  UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR
};

static const vec8 kUVToG = {
  UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG
};

static const vec16 kYToRgb = { YG, YG, YG, YG, YG, YG, YG, YG };
static const vec16 kYSub16 = { 16, 16, 16, 16, 16, 16, 16, 16 };
static const vec16 kUVBiasB = { BB, BB, BB, BB, BB, BB, BB, BB };
static const vec16 kUVBiasG = { BG, BG, BG, BG, BG, BG, BG, BG };
static const vec16 kUVBiasR = { BR, BR, BR, BR, BR, BR, BR, BR };

#define YUVTORGB __asm {                                                       \
    /* Step 1: Find 4 UV contributions to 8 R,G,B values */                    \
    __asm movd       xmm0, [esi]          /* U */                              \
    __asm movd       xmm1, [esi + edi]    /* V */                              \
    __asm lea        esi,  [esi + 4]                                           \
    __asm punpcklbw  xmm0, xmm1           /* UV */                             \
    __asm punpcklwd  xmm0, xmm0           /* UVUV (upsample) */                \
    __asm movdqa     xmm1, xmm0                                                \
    __asm movdqa     xmm2, xmm0                                                \
    __asm pmaddubsw  xmm0, kUVToB        /* scale B UV */                      \
    __asm pmaddubsw  xmm1, kUVToG        /* scale G UV */                      \
    __asm pmaddubsw  xmm2, kUVToR        /* scale R UV */                      \
    __asm psubw      xmm0, kUVBiasB      /* unbias back to signed */           \
    __asm psubw      xmm1, kUVBiasG                                            \
    __asm psubw      xmm2, kUVBiasR                                            \
    /* Step 2: Find Y contribution to 8 R,G,B values */                        \
    __asm movq       xmm3, qword ptr [eax]                                     \
    __asm lea        eax, [eax + 8]                                            \
    __asm punpcklbw  xmm3, xmm4                                                \
    __asm psubsw     xmm3, kYSub16                                             \
    __asm pmullw     xmm3, kYToRgb                                             \
    __asm paddsw     xmm0, xmm3           /* B += Y */                         \
    __asm paddsw     xmm1, xmm3           /* G += Y */                         \
    __asm paddsw     xmm2, xmm3           /* R += Y */                         \
    __asm psraw      xmm0, 6                                                   \
    __asm psraw      xmm1, 6                                                   \
    __asm psraw      xmm2, 6                                                   \
    __asm packuswb   xmm0, xmm0           /* B */                              \
    __asm packuswb   xmm1, xmm1           /* G */                              \
    __asm packuswb   xmm2, xmm2           /* R */                              \
  }

__declspec(naked)
void I420ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* rgb_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // Y
    mov        esi, [esp + 8 + 8]   // U
    mov        edi, [esp + 8 + 12]  // V
    mov        edx, [esp + 8 + 16]  // rgb
    mov        ecx, [esp + 8 + 20]  // width
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           // generate 0xffffffff for alpha
    pxor       xmm4, xmm4

 convertloop:
    YUVTORGB

    // Step 3: Weave into ARGB
    punpcklbw  xmm0, xmm1           // BG
    punpcklbw  xmm2, xmm5           // RA
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           // BGRA first 4 pixels
    punpckhwd  xmm1, xmm2           // BGRA next 4 pixels
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]

    sub        ecx, 8
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void I420ToBGRARow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* rgb_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // Y
    mov        esi, [esp + 8 + 8]   // U
    mov        edi, [esp + 8 + 12]  // V
    mov        edx, [esp + 8 + 16]  // rgb
    mov        ecx, [esp + 8 + 20]  // width
    sub        edi, esi
    pxor       xmm4, xmm4

 convertloop:
    YUVTORGB

    // Step 3: Weave into BGRA
    pcmpeqb    xmm5, xmm5           // generate 0xffffffff for alpha
    punpcklbw  xmm1, xmm0           // GB
    punpcklbw  xmm5, xmm2           // AR
    movdqa     xmm0, xmm5
    punpcklwd  xmm5, xmm1           // BGRA first 4 pixels
    punpckhwd  xmm0, xmm1           // BGRA next 4 pixels
    movdqa     [edx], xmm5
    movdqa     [edx + 16], xmm0
    lea        edx,  [edx + 32]

    sub        ecx, 8
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void I420ToABGRRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* rgb_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // Y
    mov        esi, [esp + 8 + 8]   // U
    mov        edi, [esp + 8 + 12]  // V
    mov        edx, [esp + 8 + 16]  // rgb
    mov        ecx, [esp + 8 + 20]  // width
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           // generate 0xffffffff for alpha
    pxor       xmm4, xmm4

 convertloop:
    YUVTORGB

    // Step 3: Weave into ARGB
    punpcklbw  xmm2, xmm1           // RG
    punpcklbw  xmm0, xmm5           // BA
    movdqa     xmm1, xmm2
    punpcklwd  xmm2, xmm0           // RGBA first 4 pixels
    punpckhwd  xmm1, xmm0           // RGBA next 4 pixels
    movdqa     [edx], xmm2
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]

    sub        ecx, 8
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void I444ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* rgb_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // Y
    mov        esi, [esp + 8 + 8]   // U
    mov        edi, [esp + 8 + 12]  // V
    mov        edx, [esp + 8 + 16]  // rgb
    mov        ecx, [esp + 8 + 20]  // width
    sub        edi, esi
    pcmpeqb    xmm5, xmm5            // generate 0xffffffff for alpha
    pxor       xmm4, xmm4

 convertloop:
    // Step 1: Find 4 UV contributions to 4 R,G,B values
    movd       xmm0, [esi]          // U
    movd       xmm1, [esi + edi]    // V
    lea        esi,  [esi + 4]
    punpcklbw  xmm0, xmm1           // UV
    movdqa     xmm1, xmm0
    movdqa     xmm2, xmm0
    pmaddubsw  xmm0, kUVToB        // scale B UV
    pmaddubsw  xmm1, kUVToG        // scale G UV
    pmaddubsw  xmm2, kUVToR        // scale R UV
    psubw      xmm0, kUVBiasB      // unbias back to signed
    psubw      xmm1, kUVBiasG
    psubw      xmm2, kUVBiasR

    // Step 2: Find Y contribution to 4 R,G,B values
    movd       xmm3, [eax]
    lea        eax, [eax + 4]
    punpcklbw  xmm3, xmm4
    psubsw     xmm3, kYSub16
    pmullw     xmm3, kYToRgb
    paddsw     xmm0, xmm3           // B += Y
    paddsw     xmm1, xmm3           // G += Y
    paddsw     xmm2, xmm3           // R += Y
    psraw      xmm0, 6
    psraw      xmm1, 6
    psraw      xmm2, 6
    packuswb   xmm0, xmm0           // B
    packuswb   xmm1, xmm1           // G
    packuswb   xmm2, xmm2           // R

    // Step 3: Weave into ARGB
    punpcklbw  xmm0, xmm1           // BG
    punpcklbw  xmm2, xmm5           // RA
    punpcklwd  xmm0, xmm2           // BGRA 4 pixels
    movdqa     [edx], xmm0
    lea        edx,  [edx + 16]

    sub        ecx, 4
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}
#endif

#ifdef HAS_YTOARGBROW_SSE2
__declspec(naked)
void YToARGBRow_SSE2(const uint8* y_buf,
                     uint8* rgb_buf,
                     int width) {
  __asm {
    pcmpeqb    xmm4, xmm4           // generate mask 0xff000000
    pslld      xmm4, 24
    mov        eax,0x10001000
    movd       xmm3,eax
    pshufd     xmm3,xmm3,0
    mov        eax,0x012a012a
    movd       xmm2,eax
    pshufd     xmm2,xmm2,0
    mov        eax, [esp + 4]       // Y
    mov        edx, [esp + 8]       // rgb
    mov        ecx, [esp + 12]      // width

 convertloop:
    // Step 1: Scale Y contribution to 8 G values. G = (y - 16) * 1.164
    movq       xmm0, qword ptr [eax]
    lea        eax, [eax + 8]
    punpcklbw  xmm0, xmm0           // Y.Y
    psubusw    xmm0, xmm3
    pmulhuw    xmm0, xmm2
    packuswb   xmm0, xmm0           // G

    // Step 2: Weave into ARGB
    punpcklbw  xmm0, xmm0           // GG
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm0           // BGRA first 4 pixels
    punpckhwd  xmm1, xmm1           // BGRA next 4 pixels
    por        xmm0, xmm4
    por        xmm1, xmm4
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]

    sub        ecx, 8
    ja         convertloop

    ret
  }
}
#endif
#endif

#ifdef HAS_MIRRORROW_SSSE3

// Shuffle table for reversing the bytes.
static const uvec8 kShuffleMirror = {
  15u, 14u, 13u, 12u, 11u, 10u, 9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u
};

__declspec(naked)
void MirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
__asm {
    mov       eax, [esp + 4]   // src
    mov       edx, [esp + 8]   // dst
    mov       ecx, [esp + 12]  // width
    movdqa    xmm5, kShuffleMirror
    lea       eax, [eax - 16]
 convertloop:
    movdqa    xmm0, [eax + ecx]
    pshufb    xmm0, xmm5
    sub       ecx, 16
    movdqa    [edx], xmm0
    lea       edx, [edx + 16]
    ja        convertloop
    ret
  }
}
#endif

#ifdef HAS_MIRRORROW_SSE2

// SSE2 version has movdqu so it can be used on misaligned buffers when SSSE3
// version can not.
__declspec(naked)
void MirrorRow_SSE2(const uint8* src, uint8* dst, int width) {
__asm {
    mov       eax, [esp + 4]   // src
    mov       edx, [esp + 8]   // dst
    mov       ecx, [esp + 12]  // width
    lea       eax, [eax - 16]
 convertloop:
    movdqu    xmm0, [eax + ecx]
    movdqu    xmm1, xmm0        // swap bytes
    psllw     xmm0, 8
    psrlw     xmm1, 8
    por       xmm0, xmm1
    pshuflw   xmm0, xmm0, 0x1b  // swap words
    pshufhw   xmm0, xmm0, 0x1b
    pshufd    xmm0, xmm0, 0x4e  // swap qwords
    sub       ecx, 16
    movdqu    [edx], xmm0
    lea       edx, [edx + 16]
    ja        convertloop
    ret
  }
}
#endif

#ifdef HAS_YUY2TOYROW_SSE2
__declspec(naked)
void YUY2ToYRow_SSE2(const uint8* src_yuy2,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_yuy2
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix
    pcmpeqb    xmm5, xmm5        // generate mask 0x00ff00ff
    psrlw      xmm5, 8

  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5   // even bytes are Y
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void YUY2ToUVRow_SSE2(const uint8* src_yuy2, int stride_yuy2,
                      uint8* dst_u, uint8* dst_y, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    // src_yuy2
    mov        esi, [esp + 8 + 8]    // stride_yuy2
    mov        edx, [esp + 8 + 12]   // dst_u
    mov        edi, [esp + 8 + 16]   // dst_v
    mov        ecx, [esp + 8 + 20]   // pix
    pcmpeqb    xmm5, xmm5            // generate mask 0x00ff00ff
    psrlw      xmm5, 8
    sub        edi, edx

  convertloop:
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
    pand       xmm0, xmm5  // U
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     // V
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void YUY2ToYRow_Unaligned_SSE2(const uint8* src_yuy2,
                               uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_yuy2
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix
    pcmpeqb    xmm5, xmm5        // generate mask 0x00ff00ff
    psrlw      xmm5, 8

  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5   // even bytes are Y
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void YUY2ToUVRow_Unaligned_SSE2(const uint8* src_yuy2, int stride_yuy2,
                                uint8* dst_u, uint8* dst_y, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    // src_yuy2
    mov        esi, [esp + 8 + 8]    // stride_yuy2
    mov        edx, [esp + 8 + 12]   // dst_u
    mov        edi, [esp + 8 + 16]   // dst_v
    mov        ecx, [esp + 8 + 20]   // pix
    pcmpeqb    xmm5, xmm5            // generate mask 0x00ff00ff
    psrlw      xmm5, 8
    sub        edi, edx

  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + esi]
    movdqu     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    psrlw      xmm0, 8      // YUYV -> UVUV
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  // U
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     // V
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void UYVYToYRow_SSE2(const uint8* src_uyvy,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_uyvy
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix

  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8    // odd bytes are Y
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void UYVYToUVRow_SSE2(const uint8* src_uyvy, int stride_uyvy,
                      uint8* dst_u, uint8* dst_y, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    // src_yuy2
    mov        esi, [esp + 8 + 8]    // stride_yuy2
    mov        edx, [esp + 8 + 12]   // dst_u
    mov        edi, [esp + 8 + 16]   // dst_v
    mov        ecx, [esp + 8 + 20]   // pix
    pcmpeqb    xmm5, xmm5            // generate mask 0x00ff00ff
    psrlw      xmm5, 8
    sub        edi, edx

  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    pand       xmm0, xmm5   // UYVY -> UVUV
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  // U
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     // V
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void UYVYToYRow_Unaligned_SSE2(const uint8* src_uyvy,
                               uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_uyvy
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix

  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8    // odd bytes are Y
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         convertloop
    ret
  }
}

__declspec(naked)
void UYVYToUVRow_Unaligned_SSE2(const uint8* src_uyvy, int stride_uyvy,
                                uint8* dst_u, uint8* dst_y, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    // src_yuy2
    mov        esi, [esp + 8 + 8]    // stride_yuy2
    mov        edx, [esp + 8 + 12]   // dst_u
    mov        edi, [esp + 8 + 16]   // dst_v
    mov        ecx, [esp + 8 + 20]   // pix
    pcmpeqb    xmm5, xmm5            // generate mask 0x00ff00ff
    psrlw      xmm5, 8
    sub        edi, edx

  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + esi]
    movdqu     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    pand       xmm0, xmm5   // UYVY -> UVUV
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  // U
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     // V
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    ja         convertloop

    pop        edi
    pop        esi
    ret
  }
}
#endif  // HAS_YUY2TOYROW_SSE2

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

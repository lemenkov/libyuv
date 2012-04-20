/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "source/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// This module is for Visual C x86
#if !defined(YUV_DISABLE_ASM) && defined(_M_IX86)

#ifdef HAS_ARGBTOYROW_SSSE3

// Constants for ARGB
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
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u, 128u, 128u, 128u, 128u
};


// Shuffle table for converting ARGB to RAW.
static const uvec8 kShuffleMaskARGBToRAW = {
  2u, 1u, 0u, 6u, 5u, 4u, 10u, 9u, 8u, 14u, 13u, 12u, 128u, 128u, 128u, 128u
};

__declspec(naked) __declspec(align(16))
void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  __asm {
    mov        eax, [esp + 4]        // src_y
    mov        edx, [esp + 8]        // dst_argb
    mov        ecx, [esp + 12]       // pix
    pcmpeqb    xmm5, xmm5            // generate mask 0xff000000
    pslld      xmm5, 24

    align      16
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
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ABGRToARGBRow_SSSE3(const uint8* src_abgr, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_abgr
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    movdqa    xmm5, kShuffleMaskABGRToARGB
    sub       edx, eax

    align      16
 convertloop:
    movdqa    xmm0, [eax]
    pshufb    xmm0, xmm5
    sub       ecx, 4
    movdqa    [eax + edx], xmm0
    lea       eax, [eax + 16]
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void BGRAToARGBRow_SSSE3(const uint8* src_bgra, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_bgra
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    movdqa    xmm5, kShuffleMaskBGRAToARGB
    sub       edx, eax

    align      16
 convertloop:
    movdqa    xmm0, [eax]
    pshufb    xmm0, xmm5
    sub       ecx, 4
    movdqa    [eax + edx], xmm0
    lea       eax, [eax + 16]
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RGB24ToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_rgb24
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm5, xmm5       // generate mask 0xff000000
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskRGB24ToARGB

    align      16
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
    sub       ecx, 16
    movdqa    [edx + 48], xmm3
    lea       edx, [edx + 64]
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RAWToARGBRow_SSSE3(const uint8* src_raw, uint8* dst_argb,
                        int pix) {
__asm {
    mov       eax, [esp + 4]   // src_raw
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm5, xmm5       // generate mask 0xff000000
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskRAWToARGB

    align      16
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
    sub       ecx, 16
    movdqa    [edx + 48], xmm3
    lea       edx, [edx + 64]
    jg        convertloop
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
__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg        convertloop
    ret
  }
}

// 24 instructions
__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg        convertloop
    ret
  }
}

// 18 instructions
__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToRGB24Row_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_argb
    mov       edx, [esp + 8]   // dst_rgb
    mov       ecx, [esp + 12]  // pix
    movdqa    xmm6, kShuffleMaskARGBToRGB24

    align      16
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
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToRAWRow_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_argb
    mov       edx, [esp + 8]   // dst_rgb
    mov       ecx, [esp + 12]  // pix
    movdqa    xmm6, kShuffleMaskARGBToRAW

    align      16
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
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg        convertloop
    ret
  }
}

// TODO(fbarchard): Improve sign extension/packing
__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToARGB4444Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_argb
    mov       edx, [esp + 8]   // dst_rgb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm4, xmm4       // generate mask 0xf000f000
    psllw     xmm4, 12
    movdqa    xmm3, xmm4       // generate mask 0x00f000f0
    psrlw     xmm3, 8

    align      16
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
    jg        convertloop
    ret
  }
}

// Convert 16 ARGB pixels (64 bytes) to 16 Y values
__declspec(naked) __declspec(align(16))
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

    align      16
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
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

    align      16
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
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void BGRAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

    align      16
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
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void BGRAToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

    align      16
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
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ABGRToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

    align      16
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
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ABGRToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

    align      16
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
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 // U
    movhps     qword ptr [edx + edi], xmm0 // V
    lea        edx, [edx + 8]
    jg         convertloop

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
    __asm movq       xmm3, qword ptr [eax]                        /* NOLINT */ \
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

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I420ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
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

    align      16
 convertloop:
    YUVTORGB

    // Step 3: Weave into ARGB
    punpcklbw  xmm0, xmm1           // BG
    punpcklbw  xmm2, xmm5           // RA
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           // BGRA first 4 pixels
    punpckhwd  xmm1, xmm2           // BGRA next 4 pixels
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I420ToBGRARow_Unaligned_SSSE3(const uint8* y_buf,
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

    align      16
 convertloop:
    YUVTORGB

    // Step 3: Weave into BGRA
    pcmpeqb    xmm5, xmm5           // generate 0xffffffff for alpha
    punpcklbw  xmm1, xmm0           // GB
    punpcklbw  xmm5, xmm2           // AR
    movdqa     xmm0, xmm5
    punpcklwd  xmm5, xmm1           // BGRA first 4 pixels
    punpckhwd  xmm0, xmm1           // BGRA next 4 pixels
    movdqu     [edx], xmm5
    movdqu     [edx + 16], xmm0
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I420ToABGRRow_Unaligned_SSSE3(const uint8* y_buf,
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

    align      16
 convertloop:
    YUVTORGB

    // Step 3: Weave into ARGB
    punpcklbw  xmm2, xmm1           // RG
    punpcklbw  xmm0, xmm5           // BA
    movdqa     xmm1, xmm2
    punpcklwd  xmm2, xmm0           // RGBA first 4 pixels
    punpckhwd  xmm1, xmm0           // RGBA next 4 pixels
    movdqu     [edx], xmm2
    movdqu     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}
#endif

#ifdef HAS_YTOARGBROW_SSE2
__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

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

__declspec(naked) __declspec(align(16))
void MirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
__asm {
    mov       eax, [esp + 4]   // src
    mov       edx, [esp + 8]   // dst
    mov       ecx, [esp + 12]  // width
    movdqa    xmm5, kShuffleMirror
    lea       eax, [eax - 16]

    align      16
 convertloop:
    movdqa    xmm0, [eax + ecx]
    pshufb    xmm0, xmm5
    sub       ecx, 16
    movdqa    [edx], xmm0
    lea       edx, [edx + 16]
    jg        convertloop
    ret
  }
}
#endif

#ifdef HAS_MIRRORROW_SSE2
// SSE2 version has movdqu so it can be used on unaligned buffers when SSSE3
// version can not.
__declspec(naked) __declspec(align(16))
void MirrorRow_SSE2(const uint8* src, uint8* dst, int width) {
__asm {
    mov       eax, [esp + 4]   // src
    mov       edx, [esp + 8]   // dst
    mov       ecx, [esp + 12]  // width
    lea       eax, [eax - 16]

    align      16
 convertloop:
    movdqu    xmm0, [eax + ecx]
    movdqa    xmm1, xmm0        // swap bytes
    psllw     xmm0, 8
    psrlw     xmm1, 8
    por       xmm0, xmm1
    pshuflw   xmm0, xmm0, 0x1b  // swap words
    pshufhw   xmm0, xmm0, 0x1b
    pshufd    xmm0, xmm0, 0x4e  // swap qwords
    sub       ecx, 16
    movdqu    [edx], xmm0
    lea       edx, [edx + 16]
    jg        convertloop
    ret
  }
}
#endif

#ifdef HAS_MIRRORROW_UV_SSSE3
// Shuffle table for reversing the bytes of UV channels.
static const uvec8 kShuffleMirrorUV = {
  14u, 12u, 10u, 8u, 6u, 4u, 2u, 0u, 15u, 13u, 11u, 9u, 7u, 5u, 3u, 1u
};

__declspec(naked) __declspec(align(16))
void MirrorRowUV_SSSE3(const uint8* src, uint8* dst_u, uint8* dst_v,
                       int width) {
  __asm {
    push      edi
    mov       eax, [esp + 4 + 4]   // src
    mov       edx, [esp + 4 + 8]   // dst_u
    mov       edi, [esp + 4 + 12]  // dst_v
    mov       ecx, [esp + 4 + 16]  // width
    movdqa    xmm1, kShuffleMirrorUV
    lea       eax, [eax + ecx * 2 - 16]
    sub       edi, edx

    align      16
 convertloop:
    movdqa    xmm0, [eax]
    lea       eax, [eax - 16]
    pshufb    xmm0, xmm1
    sub       ecx, 8
    movlpd    qword ptr [edx], xmm0
    movhpd    qword ptr [edx + edi], xmm0
    lea       edx, [edx + 8]
    jg        convertloop

    pop       edi
    ret
  }
}
#endif

#ifdef HAS_SPLITUV_SSE2
__declspec(naked) __declspec(align(16))
void SplitUV_SSE2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    // src_uv
    mov        edx, [esp + 4 + 8]    // dst_u
    mov        edi, [esp + 4 + 12]   // dst_v
    mov        ecx, [esp + 4 + 16]   // pix
    pcmpeqb    xmm5, xmm5            // generate mask 0x00ff00ff
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm0
    movdqa     xmm3, xmm1
    pand       xmm0, xmm5   // even bytes
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    psrlw      xmm2, 8      // odd bytes
    psrlw      xmm3, 8
    packuswb   xmm2, xmm3
    movdqa     [edx], xmm0
    movdqa     [edx + edi], xmm2
    lea        edx, [edx + 16]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}
#endif

#ifdef HAS_COPYROW_SSE2
// CopyRow copys 'count' bytes using a 16 byte load/store, 32 bytes at time
__declspec(naked) __declspec(align(16))
void CopyRow_SSE2(const uint8* src, uint8* dst, int count) {
  __asm {
    mov        eax, [esp + 4]   // src
    mov        edx, [esp + 8]   // dst
    mov        ecx, [esp + 12]  // count
    sub        edx, eax

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     [eax + edx], xmm0
    movdqa     [eax + edx + 16], xmm1
    lea        eax, [eax + 32]
    sub        ecx, 32
    jg         convertloop
    ret
  }
}
#endif  // HAS_COPYROW_SSE2

#ifdef HAS_COPYROW_X86
__declspec(naked) __declspec(align(16))
void CopyRow_X86(const uint8* src, uint8* dst, int count) {
  __asm {
    mov        eax, esi
    mov        edx, edi
    mov        esi, [esp + 4]   // src
    mov        edi, [esp + 8]   // dst
    mov        ecx, [esp + 12]  // count
    shr        ecx, 2
    rep movsd
    mov        edi, edx
    mov        esi, eax
    ret
  }
}
#endif

#ifdef HAS_YUY2TOYROW_SSE2
__declspec(naked) __declspec(align(16))
void YUY2ToYRow_SSE2(const uint8* src_yuy2,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_yuy2
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix
    pcmpeqb    xmm5, xmm5        // generate mask 0x00ff00ff
    psrlw      xmm5, 8

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5   // even bytes are Y
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void YUY2ToYRow_Unaligned_SSE2(const uint8* src_yuy2,
                               uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_yuy2
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix
    pcmpeqb    xmm5, xmm5        // generate mask 0x00ff00ff
    psrlw      xmm5, 8

    align      16
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5   // even bytes are Y
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void UYVYToYRow_SSE2(const uint8* src_uyvy,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_uyvy
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8    // odd bytes are Y
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void UYVYToYRow_Unaligned_SSE2(const uint8* src_uyvy,
                               uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    // src_uyvy
    mov        edx, [esp + 8]    // dst_y
    mov        ecx, [esp + 12]   // pix

    align      16
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8    // odd bytes are Y
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
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

    align      16
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
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}
#endif  // HAS_YUY2TOYROW_SSE2

#ifdef HAS_ARGBBLENDROW_SSE2
// Blend 8 pixels at a time.
// src_argb0 unaligned.
// src_argb1 and dst_argb aligned to 16 bytes.
// width must be multiple of 4 pixels.
__declspec(naked) __declspec(align(16))
void ARGBBlendRow_Aligned_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                               uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   // src_argb0
    mov        esi, [esp + 4 + 8]   // src_argb1
    mov        edx, [esp + 4 + 12]  // dst_argb
    mov        ecx, [esp + 4 + 16]  // width
    pcmpeqb    xmm7, xmm7       // generate constant 1
    psrlw      xmm7, 15
    pcmpeqb    xmm6, xmm6       // generate mask 0x00ff00ff
    psrlw      xmm6, 8
    pcmpeqb    xmm5, xmm5       // generate mask 0xff00ff00
    psllw      xmm5, 8
    pcmpeqb    xmm4, xmm4       // generate mask 0xff000000
    pslld      xmm4, 24

    align      16
 convertloop:
    movdqu     xmm3, [eax]
    movdqa     xmm0, xmm3       // src argb
    pxor       xmm3, xmm4       // ~alpha
    movdqu     xmm2, [esi]      // _r_b
    psrlw      xmm3, 8          // alpha
    pshufhw    xmm3, xmm3,0F5h  // 8 alpha words
    pshuflw    xmm3, xmm3,0F5h
    pand       xmm2, xmm6       // _r_b
    paddw      xmm3, xmm7       // 256 - alpha
    pmullw     xmm2, xmm3       // _r_b * alpha
    movdqu     xmm1, [esi]      // _a_g
    psrlw      xmm1, 8          // _a_g
    por        xmm0, xmm4       // set alpha to 255
    pmullw     xmm1, xmm3       // _a_g * alpha
    movdqu     xmm3, [eax + 16]
    lea        eax, [eax + 32]
    psrlw      xmm2, 8          // _r_b convert to 8 bits again
    paddusb    xmm0, xmm2       // + src argb
    pand       xmm1, xmm5       // a_g_ convert to 8 bits again
    paddusb    xmm0, xmm1       // + src argb
    sub        ecx, 4
    movdqa     [edx], xmm0
    jle        done

    movdqa     xmm0, xmm3       // src argb
    pxor       xmm3, xmm4       // ~alpha
    movdqu     xmm2, [esi + 16] // _r_b
    psrlw      xmm3, 8          // alpha
    pshufhw    xmm3, xmm3,0F5h  // 8 alpha words
    pshuflw    xmm3, xmm3,0F5h
    pand       xmm2, xmm6       // _r_b
    paddw      xmm3, xmm7       // 256 - alpha
    pmullw     xmm2, xmm3       // _r_b * alpha
    movdqu     xmm1, [esi + 16] // _a_g
    lea        esi, [esi + 32]
    psrlw      xmm1, 8          // _a_g
    por        xmm0, xmm4       // set alpha to 255
    pmullw     xmm1, xmm3       // _a_g * alpha
    psrlw      xmm2, 8          // _r_b convert to 8 bits again
    paddusb    xmm0, xmm2       // + src argb
    pand       xmm1, xmm5       // a_g_ convert to 8 bits again
    paddusb    xmm0, xmm1       // + src argb
    sub        ecx, 4
    movdqa     [edx + 16], xmm0
    lea        edx, [edx + 32]
    jg         convertloop

 done:
    pop        esi
    ret
  }
}

// Blend 1 pixel at a time, unaligned.
__declspec(naked) __declspec(align(16))
void ARGBBlendRow1_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                         uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   // src_argb0
    mov        esi, [esp + 4 + 8]   // src_argb1
    mov        edx, [esp + 4 + 12]  // dst_argb
    mov        ecx, [esp + 4 + 16]  // width
    pcmpeqb    xmm7, xmm7       // generate constant 1
    psrlw      xmm7, 15
    pcmpeqb    xmm6, xmm6       // generate mask 0x00ff00ff
    psrlw      xmm6, 8
    pcmpeqb    xmm5, xmm5       // generate mask 0xff00ff00
    psllw      xmm5, 8
    pcmpeqb    xmm4, xmm4       // generate mask 0xff000000
    pslld      xmm4, 24

    align      16
 convertloop:
    movd       xmm3, [eax]
    lea        eax, [eax + 4]
    movdqa     xmm0, xmm3       // src argb
    pxor       xmm3, xmm4       // ~alpha
    movd       xmm2, [esi]      // _r_b
    psrlw      xmm3, 8          // alpha
    pshufhw    xmm3, xmm3,0F5h  // 8 alpha words
    pshuflw    xmm3, xmm3,0F5h
    pand       xmm2, xmm6       // _r_b
    paddw      xmm3, xmm7       // 256 - alpha
    pmullw     xmm2, xmm3       // _r_b * alpha
    movd       xmm1, [esi]      // _a_g
    lea        esi, [esi + 4]
    psrlw      xmm1, 8          // _a_g
    por        xmm0, xmm4       // set alpha to 255
    pmullw     xmm1, xmm3       // _a_g * alpha
    psrlw      xmm2, 8          // _r_b convert to 8 bits again
    paddusb    xmm0, xmm2       // + src argb
    pand       xmm1, xmm5       // a_g_ convert to 8 bits again
    paddusb    xmm0, xmm1       // + src argb
    sub        ecx, 1
    movd       [edx], xmm0
    lea        edx, [edx + 4]
    jg         convertloop

    pop        esi
    ret
  }
}

#endif  // HAS_ARGBBLENDROW_SSE2
#ifdef HAS_ARGBBLENDROW_SSSE3
// Shuffle table for reversing the bytes.
static const uvec8 kShuffleAlpha = {
  3u, 0x80, 3u, 0x80, 7u, 0x80, 7u, 0x80,
  11u, 0x80, 11u, 0x80, 15u, 0x80, 15u, 0x80
};

// Blend 8 pixels at a time
// Shuffle table for reversing the bytes.

// Same as SSE2, but replaces
//    psrlw      xmm3, 8          // alpha
//    pshufhw    xmm3, xmm3,0F5h  // 8 alpha words
//    pshuflw    xmm3, xmm3,0F5h
// with..
//    pshufb     xmm3, kShuffleAlpha // alpha

// Destination aligned to 16 bytes, multiple of 4 pixels.
__declspec(naked) __declspec(align(16))
void ARGBBlendRow_Aligned_SSSE3(const uint8* src_argb0, const uint8* src_argb1,
                                 uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   // src_argb0
    mov        esi, [esp + 4 + 8]   // src_argb1
    mov        edx, [esp + 4 + 12]  // dst_argb
    mov        ecx, [esp + 4 + 16]  // width
    pcmpeqb    xmm7, xmm7       // generate constant 1
    psrlw      xmm7, 15
    pcmpeqb    xmm6, xmm6       // generate mask 0x00ff00ff
    psrlw      xmm6, 8
    pcmpeqb    xmm5, xmm5       // generate mask 0xff00ff00
    psllw      xmm5, 8
    pcmpeqb    xmm4, xmm4       // generate mask 0xff000000
    pslld      xmm4, 24

    align      16
 convertloop:
    movdqu     xmm3, [eax]
    movdqa     xmm0, xmm3       // src argb
    pxor       xmm3, xmm4       // ~alpha
    pshufb     xmm3, kShuffleAlpha // alpha
    movdqu     xmm2, [esi]      // _r_b
    pand       xmm2, xmm6       // _r_b
    paddw      xmm3, xmm7       // 256 - alpha
    pmullw     xmm2, xmm3       // _r_b * alpha
    movdqu     xmm1, [esi]      // _a_g
    psrlw      xmm1, 8          // _a_g
    por        xmm0, xmm4       // set alpha to 255
    pmullw     xmm1, xmm3       // _a_g * alpha
    movdqu     xmm3, [eax + 16]
    lea        eax, [eax + 32]
    psrlw      xmm2, 8          // _r_b convert to 8 bits again
    paddusb    xmm0, xmm2       // + src argb
    pand       xmm1, xmm5       // a_g_ convert to 8 bits again
    paddusb    xmm0, xmm1       // + src argb
    sub        ecx, 4
    movdqa     [edx], xmm0
    jle        done

    movdqa     xmm0, xmm3       // src argb
    pxor       xmm3, xmm4       // ~alpha
    movdqu     xmm2, [esi + 16] // _r_b
    pshufb     xmm3, kShuffleAlpha // alpha
    pand       xmm2, xmm6       // _r_b
    paddw      xmm3, xmm7       // 256 - alpha
    pmullw     xmm2, xmm3       // _r_b * alpha
    movdqu     xmm1, [esi + 16] // _a_g
    lea        esi, [esi + 32]
    psrlw      xmm1, 8          // _a_g
    por        xmm0, xmm4       // set alpha to 255
    pmullw     xmm1, xmm3       // _a_g * alpha
    psrlw      xmm2, 8          // _r_b convert to 8 bits again
    paddusb    xmm0, xmm2       // + src argb
    pand       xmm1, xmm5       // a_g_ convert to 8 bits again
    paddusb    xmm0, xmm1       // + src argb
    sub        ecx, 4
    movdqa     [edx + 16], xmm0
    lea        edx, [edx + 32]
    jg         convertloop

 done:
    pop        esi
    ret
  }
}
#endif  // HAS_ARGBBLENDROW_SSSE3

#ifdef HAS_ARGBATTENUATE_SSE2
// Attenuate 4 pixels at a time.
// aligned to 16 bytes
__declspec(naked) __declspec(align(16))
void ARGBAttenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   // src_argb0
    mov        edx, [esp + 8]   // dst_argb
    mov        ecx, [esp + 12]  // width
    sub        edx, eax
    pcmpeqb    xmm4, xmm4       // generate mask 0xff000000
    pslld      xmm4, 24
    pcmpeqb    xmm5, xmm5       // generate mask 0x00ffffff
    psrld      xmm5, 8

    align      16
 convertloop:
    movdqa     xmm0, [eax]      // read 4 pixels
    punpcklbw  xmm0, xmm0       // first 2
    pshufhw    xmm2, xmm0,0FFh  // 8 alpha words
    pshuflw    xmm2, xmm2,0FFh
    pmulhuw    xmm0, xmm2       // rgb * a
    movdqa     xmm1, [eax]      // read 4 pixels
    punpckhbw  xmm1, xmm1       // next 2 pixels
    pshufhw    xmm2, xmm1,0FFh  // 8 alpha words
    pshuflw    xmm2, xmm2,0FFh
    pmulhuw    xmm1, xmm2       // rgb * a
    movdqa     xmm2, [eax]      // alphas
    psrlw      xmm0, 8
    pand       xmm2, xmm4
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    pand       xmm0, xmm5       // keep original alphas
    por        xmm0, xmm2
    sub        ecx, 4
    movdqa     [eax + edx], xmm0
    lea        eax, [eax + 16]
    jg         convertloop

    ret
  }
}
#endif  // HAS_ARGBATTENUATE_SSE2

#ifdef HAS_ARGBATTENUATE_SSSE3
// Shuffle table duplicating alpha
static const uvec8 kShuffleAlpha0 = {
  3u, 3u, 3u, 3u, 3u, 3u, 128u, 128u, 7u, 7u, 7u, 7u, 7u, 7u, 128u, 128u,
};
static const uvec8 kShuffleAlpha1 = {
  11u, 11u, 11u, 11u, 11u, 11u, 128u, 128u,
  15u, 15u, 15u, 15u, 15u, 15u, 128u, 128u,
};
__declspec(naked) __declspec(align(16))
void ARGBAttenuateRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   // src_argb0
    mov        edx, [esp + 8]   // dst_argb
    mov        ecx, [esp + 12]  // width
    sub        edx, eax
    pcmpeqb    xmm3, xmm3       // generate mask 0xff000000
    pslld      xmm3, 24
    movdqa     xmm4, kShuffleAlpha0
    movdqa     xmm5, kShuffleAlpha1

    align      16
 convertloop:
    movdqa     xmm0, [eax]      // read 4 pixels
    pshufb     xmm0, xmm4       // isolate first 2 alphas
    movdqa     xmm1, [eax]      // read 4 pixels
    punpcklbw  xmm1, xmm1       // first 2 pixel rgbs
    pmulhuw    xmm0, xmm1       // rgb * a
    movdqa     xmm1, [eax]      // read 4 pixels
    pshufb     xmm1, xmm5       // isolate next 2 alphas
    movdqa     xmm2, [eax]      // read 4 pixels
    punpckhbw  xmm2, xmm2       // next 2 pixel rgbs
    pmulhuw    xmm1, xmm2       // rgb * a
    movdqa     xmm2, [eax]      // mask original alpha
    pand       xmm2, xmm3
    psrlw      xmm0, 8
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    por        xmm0, xmm2       // copy original alpha
    sub        ecx, 4
    movdqa     [eax + edx], xmm0
    lea        eax, [eax + 16]
    jg         convertloop

    ret
  }
}
#endif  // HAS_ARGBATTENUATE_SSSE3

#ifdef HAS_ARGBUNATTENUATE_SSE2
// Divide source RGB by alpha and store to destination.
// b = (b * 255 + (a / 2)) / a;
// g = (g * 255 + (a / 2)) / a;
// r = (r * 255 + (a / 2)) / a;
// Reciprocal method is off by 1 on some values. ie 125
// 8.16 fixed point inverse table
#define T(a) 0x10000 / a
static uint32 fixed_invtbl8[256] = {
  0x100, 0xffff, T(0x02), T(0x03), T(0x04), T(0x05), T(0x06), T(0x07),
  T(0x08), T(0x09), T(0x0a), T(0x0b), T(0x0c), T(0x0d), T(0x0e), T(0x0f),
  T(0x10), T(0x11), T(0x12), T(0x13), T(0x14), T(0x15), T(0x16), T(0x17),
  T(0x18), T(0x19), T(0x1a), T(0x1b), T(0x1c), T(0x1d), T(0x1e), T(0x1f),
  T(0x20), T(0x21), T(0x22), T(0x23), T(0x24), T(0x25), T(0x26), T(0x27),
  T(0x28), T(0x29), T(0x2a), T(0x2b), T(0x2c), T(0x2d), T(0x2e), T(0x2f),
  T(0x30), T(0x31), T(0x32), T(0x33), T(0x34), T(0x35), T(0x36), T(0x37),
  T(0x38), T(0x39), T(0x3a), T(0x3b), T(0x3c), T(0x3d), T(0x3e), T(0x3f),
  T(0x40), T(0x41), T(0x42), T(0x43), T(0x44), T(0x45), T(0x46), T(0x47),
  T(0x48), T(0x49), T(0x4a), T(0x4b), T(0x4c), T(0x4d), T(0x4e), T(0x4f),
  T(0x50), T(0x51), T(0x52), T(0x53), T(0x54), T(0x55), T(0x56), T(0x57),
  T(0x58), T(0x59), T(0x5a), T(0x5b), T(0x5c), T(0x5d), T(0x5e), T(0x5f),
  T(0x60), T(0x61), T(0x62), T(0x63), T(0x64), T(0x65), T(0x66), T(0x67),
  T(0x68), T(0x69), T(0x6a), T(0x6b), T(0x6c), T(0x6d), T(0x6e), T(0x6f),
  T(0x70), T(0x71), T(0x72), T(0x73), T(0x74), T(0x75), T(0x76), T(0x77),
  T(0x78), T(0x79), T(0x7a), T(0x7b), T(0x7c), T(0x7d), T(0x7e), T(0x7f),
  T(0x80), T(0x81), T(0x82), T(0x83), T(0x84), T(0x85), T(0x86), T(0x87),
  T(0x88), T(0x89), T(0x8a), T(0x8b), T(0x8c), T(0x8d), T(0x8e), T(0x8f),
  T(0x90), T(0x91), T(0x92), T(0x93), T(0x94), T(0x95), T(0x96), T(0x97),
  T(0x98), T(0x99), T(0x9a), T(0x9b), T(0x9c), T(0x9d), T(0x9e), T(0x9f),
  T(0xa0), T(0xa1), T(0xa2), T(0xa3), T(0xa4), T(0xa5), T(0xa6), T(0xa7),
  T(0xa8), T(0xa9), T(0xaa), T(0xab), T(0xac), T(0xad), T(0xae), T(0xaf),
  T(0xb0), T(0xb1), T(0xb2), T(0xb3), T(0xb4), T(0xb5), T(0xb6), T(0xb7),
  T(0xb8), T(0xb9), T(0xba), T(0xbb), T(0xbc), T(0xbd), T(0xbe), T(0xbf),
  T(0xc0), T(0xc1), T(0xc2), T(0xc3), T(0xc4), T(0xc5), T(0xc6), T(0xc7),
  T(0xc8), T(0xc9), T(0xca), T(0xcb), T(0xcc), T(0xcd), T(0xce), T(0xcf),
  T(0xd0), T(0xd1), T(0xd2), T(0xd3), T(0xd4), T(0xd5), T(0xd6), T(0xd7),
  T(0xd8), T(0xd9), T(0xda), T(0xdb), T(0xdc), T(0xdd), T(0xde), T(0xdf),
  T(0xe0), T(0xe1), T(0xe2), T(0xe3), T(0xe4), T(0xe5), T(0xe6), T(0xe7),
  T(0xe8), T(0xe9), T(0xea), T(0xeb), T(0xec), T(0xed), T(0xee), T(0xef),
  T(0xf0), T(0xf1), T(0xf2), T(0xf3), T(0xf4), T(0xf5), T(0xf6), T(0xf7),
  T(0xf8), T(0xf9), T(0xfa), T(0xfb), T(0xfc), T(0xfd), T(0xfe), 0x100 };
#undef T

// Unattenuate 4 pixels at a time.
// aligned to 16 bytes
__declspec(naked) __declspec(align(16))
void ARGBUnattenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb,
                             int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   // src_argb0
    mov        edx, [esp + 8 + 8]   // dst_argb
    mov        ecx, [esp + 8 + 12]  // width
    sub        edx, eax
    pcmpeqb    xmm4, xmm4       // generate mask 0xff000000
    pslld      xmm4, 24

    align      16
 convertloop:
    movdqa     xmm0, [eax]      // read 4 pixels
    movzx      esi, byte ptr [eax + 3]  // first alpha
    movzx      edi, byte ptr [eax + 7]  // second alpha
    punpcklbw  xmm0, xmm0       // first 2
    movd       xmm2, dword ptr fixed_invtbl8[esi * 4]
    movd       xmm3, dword ptr fixed_invtbl8[edi * 4]
    pshuflw    xmm2, xmm2,0C0h  // first 4 inv_alpha words
    pshuflw    xmm3, xmm3,0C0h  // next 4 inv_alpha words
    movlhps    xmm2, xmm3
    pmulhuw    xmm0, xmm2       // rgb * a

    movdqa     xmm1, [eax]      // read 4 pixels
    movzx      esi, byte ptr [eax + 11]  // third alpha
    movzx      edi, byte ptr [eax + 15]  // forth alpha
    punpckhbw  xmm1, xmm1       // next 2
    movd       xmm2, dword ptr fixed_invtbl8[esi * 4]
    movd       xmm3, dword ptr fixed_invtbl8[edi * 4]
    pshuflw    xmm2, xmm2,0C0h  // first 4 inv_alpha words
    pshuflw    xmm3, xmm3,0C0h  // next 4 inv_alpha words
    movlhps    xmm2, xmm3
    pmulhuw    xmm1, xmm2       // rgb * a

    movdqa     xmm2, [eax]      // alphas
    pand       xmm2, xmm4
    packuswb   xmm0, xmm1
    por        xmm0, xmm2
    sub        ecx, 4
    movdqa     [eax + edx], xmm0
    lea        eax, [eax + 16]
    jg         convertloop
    pop        edi
    pop        esi
    ret
  }
}
#endif  // HAS_ARGBUNATTENUATE_SSE2

#endif  // _M_IX86

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

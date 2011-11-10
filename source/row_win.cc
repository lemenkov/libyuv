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

extern "C" {

#ifdef HAS_ARGBTOYROW_SSSE3

// Constant multiplication table for converting ARGB to I400.
SIMD_ALIGNED(const int8 kARGBToY[16]) = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};

SIMD_ALIGNED(const int8 kARGBToU[16]) = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

SIMD_ALIGNED(const int8 kARGBToV[16]) = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};

// Constants for BGRA
SIMD_ALIGNED(const int8 kBGRAToY[16]) = {
  0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13
};

SIMD_ALIGNED(const int8 kBGRAToU[16]) = {
  0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112
};

SIMD_ALIGNED(const int8 kBGRAToV[16]) = {
  0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18
};

// Constants for ABGR
SIMD_ALIGNED(const int8 kABGRToY[16]) = {
  33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0
};

SIMD_ALIGNED(const int8 kABGRToU[16]) = {
  -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0
};

SIMD_ALIGNED(const int8 kABGRToV[16]) = {
  112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0
};

SIMD_ALIGNED(const uint8 kAddY16[16]) = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u,
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u,
};

SIMD_ALIGNED(const uint8 kAddUV128[16]) = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};

// Shuffle table for converting BG24 to ARGB.
SIMD_ALIGNED(const uint8 kShuffleMaskBG24ToARGB[16]) = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};

// Shuffle table for converting RAW to ARGB.
SIMD_ALIGNED(const uint8 kShuffleMaskRAWToARGB[16]) = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};

// Shuffle table for converting ABGR to ARGB.
SIMD_ALIGNED(const uint8 kShuffleMaskABGRToARGB[16]) = {
  2u, 1u, 0u, 3u, 6u, 5u, 4u, 7u, 10u, 9u, 8u, 11u, 14u, 13u, 12u, 15u
};

// Shuffle table for converting BGRA to ARGB.
SIMD_ALIGNED(const uint8 kShuffleMaskBGRAToARGB[16]) = {
  3u, 2u, 1u, 0u, 7u, 6u, 5u, 4u, 11u, 10u, 9u, 8u, 15u, 14u, 13u, 12u
};

__declspec(naked)
void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  __asm {
    mov        eax, [esp + 4]        // src_y
    mov        edx, [esp + 8]        // dst_argb
    mov        ecx, [esp + 12]       // pix
    pcmpeqb    xmm5, xmm5            // generate mask 0xff000000
    pslld      xmm5, 24

  wloop:
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
    ja         wloop
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

 convertloop :
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

 convertloop :
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
void BG24ToARGBRow_SSSE3(const uint8* src_bg24, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_bg24
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm5, xmm5       // generate mask 0xff000000
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskBG24ToARGB

 convertloop :
    movdqa    xmm0, [eax]
    movdqa    xmm1, [eax + 16]
    movdqa    xmm3, [eax + 32]
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

 convertloop :
    movdqa    xmm0, [eax]
    movdqa    xmm1, [eax + 16]
    movdqa    xmm3, [eax + 32]
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

// Convert 16 ARGB pixels (64 bytes) to 16 Y values
__declspec(naked)
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

 convertloop :
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
void BGRAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

 convertloop :
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
void ABGRToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

 convertloop :
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

 convertloop :
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

 convertloop :
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

 convertloop :
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

#define YUVTORGB_MMX(TABLE) __asm {                                            \
    __asm convertloop :                                                        \
    __asm movzx     eax, byte ptr [edi]                                        \
    __asm lea       edi, [edi + 1]                                             \
    __asm movzx     ebx, byte ptr [esi]                                        \
    __asm lea       esi, [esi + 1]                                             \
    __asm movq      mm0, [TABLE + 2048 + 8 * eax]                              \
    __asm movzx     eax, byte ptr [edx]                                        \
    __asm paddsw    mm0, [TABLE + 4096 + 8 * ebx]                              \
    __asm movzx     ebx, byte ptr [edx + 1]                                    \
    __asm movq      mm1, [TABLE + 8 * eax]                                     \
    __asm lea       edx, [edx + 2]                                             \
    __asm movq      mm2, [TABLE + 8 * ebx]                                     \
    __asm paddsw    mm1, mm0                                                   \
    __asm paddsw    mm2, mm0                                                   \
    __asm psraw     mm1, 6                                                     \
    __asm psraw     mm2, 6                                                     \
    __asm packuswb  mm1, mm2                                                   \
    __asm movq      [ebp], mm1                                                 \
    __asm lea       ebp, [ebp + 8]                                             \
    __asm sub       ecx, 2                                                     \
    __asm ja        convertloop                                                \
  }

__declspec(naked)
void FastConvertYUVToARGBRow_MMX(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width) {
  __asm {
    push      ebx
    push      esi
    push      edi
    push      ebp
    mov       edx, [esp + 16 + 4]
    mov       edi, [esp + 16 + 8]
    mov       esi, [esp + 16 + 12]
    mov       ebp, [esp + 16 + 16]
    mov       ecx, [esp + 16 + 20]

    YUVTORGB_MMX(kCoefficientsRgbY)

    pop       ebp
    pop       edi
    pop       esi
    pop       ebx
    ret
  }
}

__declspec(naked)
void FastConvertYUVToBGRARow_MMX(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width) {
  __asm {
    push      ebx
    push      esi
    push      edi
    push      ebp
    mov       edx, [esp + 16 + 4]
    mov       edi, [esp + 16 + 8]
    mov       esi, [esp + 16 + 12]
    mov       ebp, [esp + 16 + 16]
    mov       ecx, [esp + 16 + 20]

    YUVTORGB_MMX(kCoefficientsBgraY)

    pop       ebp
    pop       edi
    pop       esi
    pop       ebx
    ret
  }
}

__declspec(naked)
void FastConvertYUVToABGRRow_MMX(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width) {
  __asm {
    push      ebx
    push      esi
    push      edi
    push      ebp
    mov       edx, [esp + 16 + 4]
    mov       edi, [esp + 16 + 8]
    mov       esi, [esp + 16 + 12]
    mov       ebp, [esp + 16 + 16]
    mov       ecx, [esp + 16 + 20]

    YUVTORGB_MMX(kCoefficientsAbgrY)

    pop       ebp
    pop       edi
    pop       esi
    pop       ebx
    ret
  }
}

__declspec(naked)
void FastConvertYUV444ToARGBRow_MMX(const uint8* y_buf,
                                    const uint8* u_buf,
                                    const uint8* v_buf,
                                    uint8* rgb_buf,
                                    int width) {
  __asm {
    push      ebx
    push      esi
    push      edi
    push      ebp
    mov       edx, [esp + 16 + 4]
    mov       edi, [esp + 16 + 8]
    mov       esi, [esp + 16 + 12]
    mov       ebp, [esp + 16 + 16]
    mov       ecx, [esp + 16 + 20]

 convertloop :
    movzx     eax, byte ptr [edi]
    lea       edi, [edi + 1]
    movzx     ebx, byte ptr [esi]
    lea       esi, [esi + 1]
    movq      mm0, [kCoefficientsRgbY + 2048 + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [kCoefficientsRgbY + 4096 + 8 * ebx]
    lea       edx, [edx + 1]
    paddsw    mm0, [kCoefficientsRgbY + 8 * eax]
    psraw     mm0, 6
    packuswb  mm0, mm0
    movd      [ebp], mm0
    lea       ebp, [ebp + 4]
    sub       ecx, 1
    ja        convertloop

    pop       ebp
    pop       edi
    pop       esi
    pop       ebx
    ret
  }
}

__declspec(naked)
void FastConvertYToARGBRow_MMX(const uint8* y_buf,
                               uint8* rgb_buf,
                               int width) {
  __asm {
    push      ebx
    mov       eax, [esp + 4 + 4]   // Y
    mov       edx, [esp + 4 + 8]   // rgb
    mov       ecx, [esp + 4 + 12]  // width

 convertloop :
    movzx     ebx, byte ptr [eax]
    movq      mm0, [kCoefficientsRgbY + 8 * ebx]
    psraw     mm0, 6
    movzx     ebx, byte ptr [eax + 1]
    movq      mm1, [kCoefficientsRgbY + 8 * ebx]
    psraw     mm1, 6
    packuswb  mm0, mm1
    lea       eax, [eax + 2]
    movq      [edx], mm0
    lea       edx, [edx + 8]
    sub       ecx, 2
    ja        convertloop

    pop       ebx
    ret
  }
}

#ifdef HAS_FASTCONVERTYUVTOARGBROW_SSSE3

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

SIMD_ALIGNED(const int8 kUVToB[16]) = {
  UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB
};

SIMD_ALIGNED(const int8 kUVToR[16]) = {
  UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR
};

SIMD_ALIGNED(const int8 kUVToG[16]) = {
  UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG
};

SIMD_ALIGNED(const int16 kYToRgb[8]) = {
  YG, YG, YG, YG, YG, YG, YG, YG
};

SIMD_ALIGNED(const int16 kYSub16[8]) = {
  16, 16, 16, 16, 16, 16, 16, 16
};

SIMD_ALIGNED(const int16 kUVBiasB[8]) = {
  BB, BB, BB, BB, BB, BB, BB, BB
};

SIMD_ALIGNED(const int16 kUVBiasG[8]) = {
  BG, BG, BG, BG, BG, BG, BG, BG
};

SIMD_ALIGNED(const int16 kUVBiasR[8]) = {
  BR, BR, BR, BR, BR, BR, BR, BR
};

#define YUVTORGB_SSSE3 __asm {                                                 \
    /* Step 1: Find 4 UV contributions to 8 R,G,B values */                    \
    __asm movd       xmm0, [esi]          /* U */                              \
    __asm movd       xmm1, [esi + edi]    /* V */                              \
    __asm lea        esi,  [esi + 4]                                           \
    __asm punpcklbw  xmm0, xmm1           /* UV */                             \
    __asm punpcklwd  xmm0, xmm0           /* UVUV (upsample) */                \
    __asm movdqa     xmm1, xmm0                                                \
    __asm movdqa     xmm2, xmm0                                                \
    __asm pmaddubsw  xmm0, kUVToB        /* scale B UV */                     \
    __asm pmaddubsw  xmm1, kUVToG        /* scale G UV */                     \
    __asm pmaddubsw  xmm2, kUVToR        /* scale R UV */                     \
    __asm psubw      xmm0, kUVBiasB      /* unbias back to signed */          \
    __asm psubw      xmm1, kUVBiasG                                           \
    __asm psubw      xmm2, kUVBiasR                                           \
    /* Step 2: Find Y contribution to 8 R,G,B values */                        \
    __asm movq       xmm3, qword ptr [eax]                                     \
    __asm lea        eax, [eax + 8]                                            \
    __asm punpcklbw  xmm3, xmm4                                                \
    __asm psubsw     xmm3, kYSub16                                            \
    __asm pmullw     xmm3, kYToRgb                                            \
    __asm paddw      xmm0, xmm3           /* B += Y */                         \
    __asm paddw      xmm1, xmm3           /* G += Y */                         \
    __asm paddw      xmm2, xmm3           /* R += Y */                         \
    __asm psraw      xmm0, 6                                                   \
    __asm psraw      xmm1, 6                                                   \
    __asm psraw      xmm2, 6                                                   \
    __asm packuswb   xmm0, xmm0           /* B */                              \
    __asm packuswb   xmm1, xmm1           /* G */                              \
    __asm packuswb   xmm2, xmm2           /* R */                              \
  }

__declspec(naked)
void FastConvertYUVToARGBRow_SSSE3(const uint8* y_buf,
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

 convertloop :
    YUVTORGB_SSSE3

    // Step 3: Weave into ARGB
    punpcklbw  xmm0, xmm1           // BG
    punpcklbw  xmm2, xmm5           // RA
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           // BGRA first 4 pixels
    movdqa     [edx], xmm0
    punpckhwd  xmm1, xmm2           // BGRA next 4 pixels
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
void FastConvertYUVToBGRARow_SSSE3(const uint8* y_buf,
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

 convertloop :
    YUVTORGB_SSSE3

    // Step 3: Weave into BGRA
    pcmpeqb    xmm5, xmm5           // generate 0xffffffff for alpha
    punpcklbw  xmm1, xmm0           // GB
    punpcklbw  xmm5, xmm2           // AR
    movdqa     xmm0, xmm5
    punpcklwd  xmm5, xmm1           // BGRA first 4 pixels
    movdqa     [edx], xmm5
    punpckhwd  xmm0, xmm1           // BGRA next 4 pixels
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
void FastConvertYUVToABGRRow_SSSE3(const uint8* y_buf,
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

 convertloop :
    YUVTORGB_SSSE3

    // Step 3: Weave into ARGB
    punpcklbw  xmm2, xmm1           // RG
    punpcklbw  xmm0, xmm5           // BA
    movdqa     xmm1, xmm2
    punpcklwd  xmm2, xmm0           // RGBA first 4 pixels
    movdqa     [edx], xmm2
    punpckhwd  xmm1, xmm0           // RGBA next 4 pixels
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
void FastConvertYUV444ToARGBRow_SSSE3(const uint8* y_buf,
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

 convertloop :
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
    paddw      xmm0, xmm3           // B += Y
    paddw      xmm1, xmm3           // G += Y
    paddw      xmm2, xmm3           // R += Y
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

#ifdef HAS_FASTCONVERTYTOARGBROW_SSE2

__declspec(naked)
void FastConvertYToARGBRow_SSE2(const uint8* y_buf,
                                uint8* rgb_buf,
                                int width) {
  __asm {
    mov        eax, [esp + 4]       // Y
    mov        edx, [esp + 8]       // rgb
    mov        ecx, [esp + 12]      // width
    pcmpeqb    xmm5, xmm5           // generate mask 0xff000000
    pslld      xmm5, 24
    pxor       xmm4, xmm4
    movdqa     xmm3, kYSub16
    movdqa     xmm2, kYToRgb

 convertloop :
    // Step 1: Scale Y contribution to 8 G values. G = (y - 16) * 1.164
    movq       xmm0, qword ptr [eax]
    lea        eax, [eax + 8]
    punpcklbw  xmm0, xmm4
    psubsw     xmm0, xmm3
    pmullw     xmm0, xmm2
    psraw      xmm0, 6
    packuswb   xmm0, xmm0           // G

    // Step 2: Weave into ARGB
    punpcklbw  xmm0, xmm0           // GG
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm0           // BGRA first 4 pixels
    por        xmm0, xmm5
    movdqa     [edx], xmm0
    punpckhwd  xmm1, xmm1           // BGRA next 4 pixels
    por        xmm1, xmm5
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]

    sub        ecx, 8
    ja         convertloop

    ret
  }
}

#endif
#endif

}  // extern "C"




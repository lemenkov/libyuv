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
#define TALIGN16(t, var) static __declspec(align(16)) t _ ## var

// Constant multiplication table for converting ARGB to I400.
extern "C" TALIGN16(const int8, kARGBToY[16]) = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};

extern "C" TALIGN16(const int8, kARGBToU[16]) = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

extern "C" TALIGN16(const int8, kARGBToV[16]) = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};

// Constants for BGRA
extern "C" TALIGN16(const int8, kBGRAToY[16]) = {
  0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13
};

extern "C" TALIGN16(const int8, kBGRAToU[16]) = {
  0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112
};

extern "C" TALIGN16(const int8, kBGRAToV[16]) = {
  0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18
};

// Constants for ABGR
extern "C" TALIGN16(const int8, kABGRToY[16]) = {
  33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0
};

extern "C" TALIGN16(const int8, kABGRToU[16]) = {
  -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0
};

extern "C" TALIGN16(const int8, kABGRToV[16]) = {
  112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0
};

extern "C" TALIGN16(const uint8, kAddY16[16]) = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u,
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u,
};

extern "C" TALIGN16(const uint8, kAddUV128[16]) = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};

// Shuffle table for converting BG24 to ARGB.
extern "C" TALIGN16(const uint8, kShuffleMaskBG24ToARGB[16]) = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};

// Shuffle table for converting RAW to ARGB.
extern "C" TALIGN16(const uint8, kShuffleMaskRAWToARGB[16]) = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};

// Convert 16 ARGB pixels (64 bytes) to 16 Y values
__declspec(naked)
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   /* src_argb */
    mov        edx, [esp + 8]   /* dst_y */
    mov        ecx, [esp + 12]  /* pix */
    movdqa     xmm7, _kARGBToY
    movdqa     xmm6, _kAddY16

 convertloop :
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm7
    pmaddubsw  xmm1, xmm7
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm3, xmm7
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm6
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
    movdqa     xmm7, _kBGRAToY
    movdqa     xmm6, _kAddY16

 convertloop :
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm7
    pmaddubsw  xmm1, xmm7
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm3, xmm7
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm6
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
    movdqa     xmm7, _kABGRToY
    movdqa     xmm6, _kAddY16

 convertloop :
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm7
    pmaddubsw  xmm1, xmm7
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm3, xmm7
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm6
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
    movdqa     xmm7, _kARGBToU
    movdqa     xmm6, _kARGBToV
    movdqa     xmm5, _kAddUV128
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
    movdqa     xmm7, _kBGRAToU
    movdqa     xmm6, _kBGRAToV
    movdqa     xmm5, _kAddUV128
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
    movdqa     xmm7, _kABGRToU
    movdqa     xmm6, _kABGRToV
    movdqa     xmm5, _kAddUV128
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
void BG24ToARGBRow_SSSE3(const uint8* src_bg24, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   // src_bg24
    mov       edx, [esp + 8]   // dst_argb
    mov       ecx, [esp + 12]  // pix
    pcmpeqb   xmm7, xmm7       // generate mask 0xff000000
    pslld     xmm7, 24
    movdqa    xmm6, _kShuffleMaskBG24ToARGB

 convertloop :
    movdqa    xmm0, [eax]
    movdqa    xmm1, [eax + 16]
    movdqa    xmm3, [eax + 32]
    lea       eax, [eax + 48]
    movdqa    xmm2, xmm3
    palignr   xmm2, xmm1, 8    // xmm2 = { xmm3[0:3] xmm1[8:15]}
    pshufb    xmm2, xmm6
    por       xmm2, xmm7
    palignr   xmm1, xmm0, 12   // xmm1 = { xmm3[0:7] xmm0[12:15]}
    pshufb    xmm0, xmm6
    movdqa    [edx + 32], xmm2
    por       xmm0, xmm7
    pshufb    xmm1, xmm6
    movdqa    [edx], xmm0
    por       xmm1, xmm7
    palignr   xmm3, xmm3, 4    // xmm3 = { xmm3[4:15]}
    pshufb    xmm3, xmm6
    movdqa    [edx + 16], xmm1
    por       xmm3, xmm7
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
    pcmpeqb   xmm7, xmm7       // generate mask 0xff000000
    pslld     xmm7, 24
    movdqa    xmm6, _kShuffleMaskRAWToARGB

 convertloop :
    movdqa    xmm0, [eax]
    movdqa    xmm1, [eax + 16]
    movdqa    xmm3, [eax + 32]
    lea       eax, [eax + 48]
    movdqa    xmm2, xmm3
    palignr   xmm2, xmm1, 8    // xmm2 = { xmm3[0:3] xmm1[8:15]}
    pshufb    xmm2, xmm6
    por       xmm2, xmm7
    palignr   xmm1, xmm0, 12   // xmm1 = { xmm3[0:7] xmm0[12:15]}
    pshufb    xmm0, xmm6
    movdqa    [edx + 32], xmm2
    por       xmm0, xmm7
    pshufb    xmm1, xmm6
    movdqa    [edx], xmm0
    por       xmm1, xmm7
    palignr   xmm3, xmm3, 4    // xmm3 = { xmm3[4:15]}
    pshufb    xmm3, xmm6
    movdqa    [edx + 16], xmm1
    por       xmm3, xmm7
    movdqa    [edx + 48], xmm3
    lea       edx, [edx + 64]
    sub       ecx, 16
    ja        convertloop
    ret
  }
}

__declspec(naked)
void FastConvertYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]
    mov       edi, [esp + 32 + 8]
    mov       esi, [esp + 32 + 12]
    mov       ebp, [esp + 32 + 16]
    mov       ecx, [esp + 32 + 20]

 convertloop :
    movzx     eax, byte ptr [edi]
    lea       edi, [edi + 1]
    movzx     ebx, byte ptr [esi]
    lea       esi, [esi + 1]
    movq      mm0, [_kCoefficientsRgbY + 2048 + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [_kCoefficientsRgbY + 4096 + 8 * ebx]
    movzx     ebx, byte ptr [edx + 1]
    movq      mm1, [_kCoefficientsRgbY + 8 * eax]
    lea       edx, [edx + 2]
    movq      mm2, [_kCoefficientsRgbY + 8 * ebx]
    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 6
    psraw     mm2, 6
    packuswb  mm1, mm2
    movntq    [ebp], mm1
    lea       ebp, [ebp + 8]
    sub       ecx, 2
    ja        convertloop

    popad
    ret
  }
}

__declspec(naked)
void FastConvertYUVToBGRARow(const uint8* y_buf,
                             const uint8* u_buf,
                             const uint8* v_buf,
                             uint8* rgb_buf,
                             int width) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]
    mov       edi, [esp + 32 + 8]
    mov       esi, [esp + 32 + 12]
    mov       ebp, [esp + 32 + 16]
    mov       ecx, [esp + 32 + 20]

 convertloop :
    movzx     eax, byte ptr [edi]
    lea       edi, [edi + 1]
    movzx     ebx, byte ptr [esi]
    lea       esi, [esi + 1]
    movq      mm0, [_kCoefficientsBgraY + 2048 + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [_kCoefficientsBgraY + 4096 + 8 * ebx]
    movzx     ebx, byte ptr [edx + 1]
    movq      mm1, [_kCoefficientsBgraY + 8 * eax]
    lea       edx, [edx + 2]
    movq      mm2, [_kCoefficientsBgraY + 8 * ebx]
    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 6
    psraw     mm2, 6
    packuswb  mm1, mm2
    movntq    [ebp], mm1
    lea       ebp, [ebp + 8]
    sub       ecx, 2
    ja        convertloop

    popad
    ret
  }
}

__declspec(naked)
void FastConvertYUVToABGRRow(const uint8* y_buf,
                             const uint8* u_buf,
                             const uint8* v_buf,
                             uint8* rgb_buf,
                             int width) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]
    mov       edi, [esp + 32 + 8]
    mov       esi, [esp + 32 + 12]
    mov       ebp, [esp + 32 + 16]
    mov       ecx, [esp + 32 + 20]

 convertloop :
    movzx     eax, byte ptr [edi]
    lea       edi, [edi + 1]
    movzx     ebx, byte ptr [esi]
    lea       esi, [esi + 1]
    movq      mm0, [_kCoefficientsAbgrY + 2048 + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [_kCoefficientsAbgrY + 4096 + 8 * ebx]
    movzx     ebx, byte ptr [edx + 1]
    movq      mm1, [_kCoefficientsAbgrY + 8 * eax]
    lea       edx, [edx + 2]
    movq      mm2, [_kCoefficientsAbgrY + 8 * ebx]
    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 6
    psraw     mm2, 6
    packuswb  mm1, mm2
    movntq    [ebp], mm1
    lea       ebp, [ebp + 8]
    sub       ecx, 2
    ja        convertloop

    popad
    ret
  }
}

__declspec(naked)
void FastConvertYUV444ToRGB32Row(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]   // Y
    mov       edi, [esp + 32 + 8]   // U
    mov       esi, [esp + 32 + 12]  // V
    mov       ebp, [esp + 32 + 16]  // rgb
    mov       ecx, [esp + 32 + 20]  // width

 convertloop :
    movzx     eax, byte ptr [edi]
    lea       edi, [edi + 1]
    movzx     ebx, byte ptr [esi]
    lea       esi, [esi + 1]
    movq      mm0, [_kCoefficientsRgbY + 2048 + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [_kCoefficientsRgbY + 4096 + 8 * ebx]
    lea       edx, [edx + 1]
    paddsw    mm0, [_kCoefficientsRgbY + 8 * eax]
    psraw     mm0, 6
    packuswb  mm0, mm0
    movd      [ebp], mm0
    lea       ebp, [ebp + 4]
    sub       ecx, 1
    ja        convertloop

    popad
    ret
  }
}

__declspec(naked)
void FastConvertYToRGB32Row(const uint8* y_buf,
                            uint8* rgb_buf,
                            int width) {
  __asm {
    push      ebx
    mov       eax, [esp + 4 + 4]   // Y
    mov       edx, [esp + 4 + 8]   // rgb
    mov       ecx, [esp + 4 + 12]  // width

 convertloop :
    movzx     ebx, byte ptr [eax]
    movq      mm0, [_kCoefficientsRgbY + 8 * ebx]
    psraw     mm0, 6
    movzx     ebx, byte ptr [eax + 1]
    movq      mm1, [_kCoefficientsRgbY + 8 * ebx]
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

#endif

}  // extern "C"

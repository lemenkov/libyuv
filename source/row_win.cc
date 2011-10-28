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
extern "C" TALIGN16(const int8, kRGBToY[16]) = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};

extern "C" TALIGN16(const int8, kRGBToU[16]) = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

extern "C" TALIGN16(const int8, kRGBToV[16]) = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};

extern "C" TALIGN16(const uint8, kAddY16[16]) = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u,
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u,
};

extern "C" TALIGN16(const uint8, kAddUV128[16]) = {
  128u, 0u, 128u, 0u, 128u, 0u, 128u, 0u,
  128u, 0u, 128u, 0u, 128u, 0u, 128u, 0u
};

__declspec(naked)
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   // src_argb
    mov        edx, [esp + 8]   // dst_y
    mov        ecx, [esp + 12]  // pix
    movdqa     xmm7, _kRGBToY
    movdqa     xmm6, _kAddY16
    pcmpeqb    xmm5, xmm5      // Generate mask 0x0000ffff
    psrld      xmm5, 16

 convertloop :
    movdqa    xmm0, [eax]
    movdqa    xmm1, [eax + 16]
    pmaddubsw xmm0, xmm7
    lea       eax, [eax + 32]
    pmaddubsw xmm1, xmm7            // BG ra BG ra BG ra BG ra
    palignr   xmm2, xmm0, 2         // AR xx AR xx AR xx AR xx
    paddw     xmm2, xmm0            // BGRA xx BGRA xx BGRA xx BGRA xx
    pand      xmm2, xmm5            // BGRA 00 BGRA 00 BGRA 00 BGRA 00
    palignr   xmm3, xmm1, 2
    paddw     xmm3, xmm1
    pand      xmm3, xmm5            // BGRA 00 BGRA 00 BGRA 00 BGRA 00
    packssdw  xmm2, xmm3            // BGRA BGRA BGRA BGRA BGRA BGRA BGRA BGRA
    psrlw     xmm2, 7               // 0B xx 0B xx 0B xx 0B xx
    packuswb  xmm2, xmm2
    paddb     xmm2, xmm6
    movq      qword ptr [edx], xmm2
    lea       edx, [edx + 8]
    sub       ecx, 8
    ja        convertloop
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
    movdqa     xmm7, _kRGBToU
    movdqa     xmm6, _kRGBToV
    movdqa     xmm5, _kAddUV128
    pcmpeqb    xmm4, xmm4      // Generate mask 0x0000ffff
    psrld      xmm4, 16

 convertloop :
    // step 1 - subsample 8x2 argb pixels to 4x1
    movdqa     xmm0, [eax]           // 32x2 -> 32x1
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3

    movdqa     xmm2, xmm0            // 32x1 -> 16x1
    shufps     xmm0, xmm1, 0x88
    shufps     xmm2, xmm1, 0xdd
    pavgb      xmm0, xmm2

    // step 2 - convert to U and V
    // from here down is very similar to Y code except
    // instead of 8 different pixels, its 4 pixels of U and 4 of V
    movdqa     xmm1, xmm0
    pmaddubsw  xmm0, xmm7            // U
    pmaddubsw  xmm1, xmm6            // V

    palignr    xmm2, xmm0, 2         // AR xx AR xx AR xx AR xx
    paddw      xmm2, xmm0            // BGRA xx BGRA xx BGRA xx BGRA xx
    pand       xmm2, xmm4            // BGRA 00 BGRA 00 BGRA 00 BGRA 00

    palignr    xmm3, xmm1, 2
    paddw      xmm3, xmm1
    pand       xmm3, xmm4            // BGRA 00 BGRA 00 BGRA 00 BGRA 00

    psraw      xmm2, 8
    psraw      xmm3, 8
    packsswb   xmm2, xmm3            // BGRA BGRA BGRA BGRA BGRA BGRA BGRA BGRA
    paddb      xmm2, xmm5            // -> unsigned
    packuswb   xmm2, xmm2            // 8 bytes. 4 U, 4 V

    // step 3 - store 4 U and 4 V values
    movd       dword ptr [edx], xmm2 // U
    lea        edx, [edx + 4]
    pshufd     xmm0, xmm2, 0x55      // V
    movd       dword ptr [edi], xmm0
    lea        edi, [edi + 4]
    sub        ecx, 8
    ja         convertloop
    pop        edi
    pop        esi
    ret
  }
}

static inline int RGBToY(uint8 r, uint8 g, uint8 b) {
  return (( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
}

static inline int RGBToU(uint8 r, uint8 g, uint8 b) {
  return ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
}
static inline int RGBToV(uint8 r, uint8 g, uint8 b) {
  return ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
}

void ARGBToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {
  for (int x = 0; x < width; ++x) {
    dst_y[0] = RGBToY(src_argb0[2], src_argb0[1], src_argb0[0]);
    src_argb0 += 4;
    dst_y += 1;
  }
}

void ARGBToUVRow_C(const uint8* src_argb0, int src_stride_argb,
                   uint8* dst_u, uint8* dst_v, int width) {
  const uint8* src_argb1 = src_argb0 + src_stride_argb;
  for (int x = 0; x < width - 1; x += 2) {
    uint8 ab = (src_argb0[0] + src_argb0[4] + src_argb1[0] + src_argb1[4]) >> 2;
    uint8 ag = (src_argb0[1] + src_argb0[5] + src_argb1[1] + src_argb1[5]) >> 2;
    uint8 ar = (src_argb0[2] + src_argb0[6] + src_argb1[2] + src_argb1[6]) >> 2;
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
    src_argb0 += 8;
    src_argb1 += 8;
    dst_u += 1;
    dst_v += 1;
  }
  if (width & 1) {
    uint8 ab = (src_argb0[0] + src_argb1[0]) >> 1;
    uint8 ag = (src_argb0[1] + src_argb1[1]) >> 1;
    uint8 ar = (src_argb0[2] + src_argb1[2]) >> 1;
    dst_u[0] = RGBToU(ar, ag, ab);
    dst_v[0] = RGBToV(ar, ag, ab);
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

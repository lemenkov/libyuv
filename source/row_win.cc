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

#define kCoefficientsRgbY _kCoefficientsRgbY + 0
#define kCoefficientsRgbU _kCoefficientsRgbY + 2048
#define kCoefficientsRgbV _kCoefficientsRgbY + 4096

extern "C" {

__declspec(naked)
void FastConvertYUVToRGB32Row(const uint8* y_buf,
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
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [kCoefficientsRgbV + 8 * ebx]
    movzx     ebx, byte ptr [edx + 1]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    lea       edx, [edx + 2]
    movq      mm2, [kCoefficientsRgbY + 8 * ebx]
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
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [kCoefficientsRgbV + 8 * ebx]
    lea       edx, [edx + 1]
    paddsw    mm0, [kCoefficientsRgbY + 8 * eax]
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

}  // extern "C"

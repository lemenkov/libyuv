/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(YUV_DISABLE_ASM) && defined(_M_IX86)

__declspec(naked) __declspec(align(16))
uint32 SumSquareError_SSE2(const uint8* src_a, const uint8* src_b, int count) {
  __asm {
    mov        eax, [esp + 4]    // src_a
    mov        edx, [esp + 8]    // src_b
    mov        ecx, [esp + 12]   // count
    pxor       xmm0, xmm0
    pxor       xmm5, xmm5
    sub        edx, eax

    align      16
  wloop:
    movdqa     xmm1, [eax]
    movdqa     xmm2, [eax + edx]
    lea        eax,  [eax + 16]
    sub        ecx, 16
    movdqa     xmm3, xmm1  // abs trick
    psubusb    xmm1, xmm2
    psubusb    xmm2, xmm3
    por        xmm1, xmm2
    movdqa     xmm2, xmm1
    punpcklbw  xmm1, xmm5
    punpckhbw  xmm2, xmm5
    pmaddwd    xmm1, xmm1
    pmaddwd    xmm2, xmm2
    paddd      xmm0, xmm1
    paddd      xmm0, xmm2
    jg         wloop

    pshufd     xmm1, xmm0, 0EEh
    paddd      xmm0, xmm1
    pshufd     xmm1, xmm0, 01h
    paddd      xmm0, xmm1
    movd       eax, xmm0
    ret
  }
}

#endif  // _M_IX86

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif


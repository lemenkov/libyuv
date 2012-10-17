;*
;* Copyright 2012 The LibYuv Project Authors. All rights reserved.
;*
;* Use of this source code is governed by a BSD-style license
;* that can be found in the LICENSE file in the root of the source
;* tree. An additional intellectual property rights grant can be found
;* in the file PATENTS.  All contributing project authors may
;* be found in the AUTHORS file in the root of the source tree.
;*

%include "x86inc.asm"

SECTION .text

; void YUY2ToYRow_SSE2(const uint8* src_yuy2,
;                      uint8* dst_y, int pix);

%macro YUY2TOYROW 2-3
cglobal %1ToYRow%3, 3, 3, 3, src_yuy2, dst_y, pix
%ifidn %1,YUY2
    pcmpeqb    m2, m2        ; generate mask 0x00ff00ff
    psrlw      m2, 8
%endif

    ALIGN      16
.convertloop:
    mov%2      m0, [src_yuy2q]
    mov%2      m1, [src_yuy2q + mmsize]
    lea        src_yuy2q, [src_yuy2q + mmsize * 2]
%ifidn %1,YUY2
    pand       m0, m2   ; YUY2 even bytes are Y
    pand       m1, m2
%else
    psrlw      m0, 8    ; UYVY odd bytes are Y
    psrlw      m1, 8
%endif
    packuswb   m0, m1
    sub        pixd, mmsize
    mov%2      [dst_yq], m0
    lea        dst_yq, [dst_yq + mmsize]
    jg         .convertloop
    RET
%endmacro

; TODO(fbarchard): Remove MMX when SSE2 is required.
INIT_MMX MMX
YUY2TOYROW YUY2,a,
YUY2TOYROW YUY2,u,_Unaligned
YUY2TOYROW UYVY,a,
YUY2TOYROW UYVY,u,_Unaligned
INIT_XMM SSE2
YUY2TOYROW YUY2,a,
YUY2TOYROW YUY2,u,_Unaligned
YUY2TOYROW UYVY,a,
YUY2TOYROW UYVY,u,_Unaligned
INIT_YMM AVX2
YUY2TOYROW YUY2,a,
YUY2TOYROW YUY2,u,_Unaligned
YUY2TOYROW UYVY,a,
YUY2TOYROW UYVY,u,_Unaligned


/*
 *  Copyright 2013 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"
#include "libyuv/scale_row.h"

#if !defined(LIBYUV_DISABLE_X86) &&                                 \
    (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || \
     defined(_M_IX86)) &&                                           \
    ((defined(_MSC_VER) && !defined(__clang__)) ||                  \
     defined(LIBYUV_ENABLE_ROWWIN))
#include <emmintrin.h>
#include <immintrin.h>
#include <tmmintrin.h>
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif
#endif

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// This module is for 32 bit Visual C x86
#if !defined(LIBYUV_DISABLE_X86) && defined(_MSC_VER) && defined(_M_IX86) && \
    (!defined(__clang__) || defined(LIBYUV_ENABLE_ROWWIN))

// Offsets for source bytes 0 to 9
static const uvec8 kShuf0 = {0,   1,   3,   4,   5,   7,   8,   9,
                             128, 128, 128, 128, 128, 128, 128, 128};

// Offsets for source bytes 11 to 20 with 8 subtracted = 3 to 12.
static const uvec8 kShuf1 = {3,   4,   5,   7,   8,   9,   11,  12,
                             128, 128, 128, 128, 128, 128, 128, 128};

// Offsets for source bytes 21 to 31 with 16 subtracted = 5 to 31.
static const uvec8 kShuf2 = {5,   7,   8,   9,   11,  12,  13,  15,
                             128, 128, 128, 128, 128, 128, 128, 128};

// Offsets for source bytes 0 to 10
static const uvec8 kShuf01 = {0, 1, 1, 2, 2, 3, 4, 5, 5, 6, 6, 7, 8, 9, 9, 10};

// Offsets for source bytes 10 to 21 with 8 subtracted = 3 to 13.
static const uvec8 kShuf11 = {2, 3, 4, 5,  5,  6,  6,  7,
                              8, 9, 9, 10, 10, 11, 12, 13};

// Offsets for source bytes 21 to 31 with 16 subtracted = 5 to 31.
static const uvec8 kShuf21 = {5,  6,  6,  7,  8,  9,  9,  10,
                              10, 11, 12, 13, 13, 14, 14, 15};

// Coefficients for source bytes 0 to 10
static const uvec8 kMadd01 = {3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2};

// Coefficients for source bytes 10 to 21
static const uvec8 kMadd11 = {1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1};

// Coefficients for source bytes 21 to 31
static const uvec8 kMadd21 = {2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3};

// Coefficients for source bytes 21 to 31
static const vec16 kRound34 = {2, 2, 2, 2, 2, 2, 2, 2};

// Reads 32 pixels, throws half away and writes 16 pixels.
__declspec(naked) void ScaleRowDown2_SSSE3(const uint8_t* src_ptr,
                                           ptrdiff_t src_stride,
                                           uint8_t* dst_ptr,
                                           int dst_width) {
  __asm {
    mov        eax, [esp + 4]  // src_ptr
    // src_stride ignored
    mov        edx, [esp + 12]  // dst_ptr
    mov        ecx, [esp + 16]  // dst_width

  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8       // isolate odd pixels.
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    jg         wloop

    ret
  }
}

// Blends 32x1 rectangle to 16x1.
__declspec(naked) void ScaleRowDown2Linear_SSSE3(const uint8_t* src_ptr,
                                                 ptrdiff_t src_stride,
                                                 uint8_t* dst_ptr,
                                                 int dst_width) {
  __asm {
    mov        eax, [esp + 4]  // src_ptr
    // src_stride
    mov        edx, [esp + 12]  // dst_ptr
    mov        ecx, [esp + 16]  // dst_width

    pcmpeqb    xmm4, xmm4  // constant 0x0101
    psrlw      xmm4, 15
    packuswb   xmm4, xmm4
    pxor       xmm5, xmm5  // constant 0

  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pmaddubsw  xmm0, xmm4  // horizontal add
    pmaddubsw  xmm1, xmm4
    pavgw      xmm0, xmm5    // (x + 1) / 2
    pavgw      xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    jg         wloop

    ret
  }
}

// Blends 32x2 rectangle to 16x1.
__declspec(naked) void ScaleRowDown2Box_SSSE3(const uint8_t* src_ptr,
                                              ptrdiff_t src_stride,
                                              uint8_t* dst_ptr,
                                              int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]  // src_ptr
    mov        esi, [esp + 4 + 8]  // src_stride
    mov        edx, [esp + 4 + 12]  // dst_ptr
    mov        ecx, [esp + 4 + 16]  // dst_width

    pcmpeqb    xmm4, xmm4  // constant 0x0101
    psrlw      xmm4, 15
    packuswb   xmm4, xmm4
    pxor       xmm5, xmm5  // constant 0

  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + esi]
    movdqu     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pmaddubsw  xmm0, xmm4  // horizontal add
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    paddw      xmm0, xmm2  // vertical add
    paddw      xmm1, xmm3
    psrlw      xmm0, 1
    psrlw      xmm1, 1
    pavgw      xmm0, xmm5  // (x + 1) / 2
    pavgw      xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    jg         wloop

    pop        esi
    ret
  }
}

#ifdef HAS_SCALEROWDOWN2_AVX2
// Reads 64 pixels, throws half away and writes 32 pixels.
__declspec(naked) void ScaleRowDown2_AVX2(const uint8_t* src_ptr,
                                          ptrdiff_t src_stride,
                                          uint8_t* dst_ptr,
                                          int dst_width) {
  __asm {
    mov        eax, [esp + 4]  // src_ptr
    // src_stride ignored
    mov        edx, [esp + 12]  // dst_ptr
    mov        ecx, [esp + 16]  // dst_width

  wloop:
    vmovdqu     ymm0, [eax]
    vmovdqu     ymm1, [eax + 32]
    lea         eax,  [eax + 64]
    vpsrlw      ymm0, ymm0, 8  // isolate odd pixels.
    vpsrlw      ymm1, ymm1, 8
    vpackuswb   ymm0, ymm0, ymm1
    vpermq      ymm0, ymm0, 0xd8    // unmutate vpackuswb
    vmovdqu     [edx], ymm0
    lea         edx, [edx + 32]
    sub         ecx, 32
    jg          wloop

    vzeroupper
    ret
  }
}

// Blends 64x1 rectangle to 32x1.
__declspec(naked) void ScaleRowDown2Linear_AVX2(const uint8_t* src_ptr,
                                                ptrdiff_t src_stride,
                                                uint8_t* dst_ptr,
                                                int dst_width) {
  __asm {
    mov         eax, [esp + 4]  // src_ptr
    // src_stride
    mov         edx, [esp + 12]  // dst_ptr
    mov         ecx, [esp + 16]  // dst_width

    vpcmpeqb    ymm4, ymm4, ymm4  // '1' constant, 8b
    vpsrlw      ymm4, ymm4, 15
    vpackuswb   ymm4, ymm4, ymm4
    vpxor       ymm5, ymm5, ymm5  // constant 0

  wloop:
    vmovdqu     ymm0, [eax]
    vmovdqu     ymm1, [eax + 32]
    lea         eax,  [eax + 64]
    vpmaddubsw  ymm0, ymm0, ymm4  // horizontal add
    vpmaddubsw  ymm1, ymm1, ymm4
    vpavgw      ymm0, ymm0, ymm5  // (x + 1) / 2
    vpavgw      ymm1, ymm1, ymm5
    vpackuswb   ymm0, ymm0, ymm1
    vpermq      ymm0, ymm0, 0xd8    // unmutate vpackuswb
    vmovdqu     [edx], ymm0
    lea         edx, [edx + 32]
    sub         ecx, 32
    jg          wloop

    vzeroupper
    ret
  }
}

// For rounding, average = (sum + 2) / 4
// becomes average((sum >> 1), 0)
// Blends 64x2 rectangle to 32x1.
__declspec(naked) void ScaleRowDown2Box_AVX2(const uint8_t* src_ptr,
                                             ptrdiff_t src_stride,
                                             uint8_t* dst_ptr,
                                             int dst_width) {
  __asm {
    push        esi
    mov         eax, [esp + 4 + 4]  // src_ptr
    mov         esi, [esp + 4 + 8]  // src_stride
    mov         edx, [esp + 4 + 12]  // dst_ptr
    mov         ecx, [esp + 4 + 16]  // dst_width

    vpcmpeqb    ymm4, ymm4, ymm4  // '1' constant, 8b
    vpsrlw      ymm4, ymm4, 15
    vpackuswb   ymm4, ymm4, ymm4
    vpxor       ymm5, ymm5, ymm5  // constant 0

  wloop:
    vmovdqu     ymm0, [eax]
    vmovdqu     ymm1, [eax + 32]
    vmovdqu     ymm2, [eax + esi]
    vmovdqu     ymm3, [eax + esi + 32]
    lea         eax,  [eax + 64]
    vpmaddubsw  ymm0, ymm0, ymm4  // horizontal add
    vpmaddubsw  ymm1, ymm1, ymm4
    vpmaddubsw  ymm2, ymm2, ymm4
    vpmaddubsw  ymm3, ymm3, ymm4
    vpaddw      ymm0, ymm0, ymm2  // vertical add
    vpaddw      ymm1, ymm1, ymm3
    vpsrlw      ymm0, ymm0, 1  // (x + 2) / 4 = (x / 2 + 1) / 2
    vpsrlw      ymm1, ymm1, 1
    vpavgw      ymm0, ymm0, ymm5  // (x + 1) / 2
    vpavgw      ymm1, ymm1, ymm5
    vpackuswb   ymm0, ymm0, ymm1
    vpermq      ymm0, ymm0, 0xd8  // unmutate vpackuswb
    vmovdqu     [edx], ymm0
    lea         edx, [edx + 32]
    sub         ecx, 32
    jg          wloop

    pop         esi
    vzeroupper
    ret
  }
}
#endif  // HAS_SCALEROWDOWN2_AVX2

// Point samples 32 pixels to 8 pixels.
__declspec(naked) void ScaleRowDown4_SSSE3(const uint8_t* src_ptr,
                                           ptrdiff_t src_stride,
                                           uint8_t* dst_ptr,
                                           int dst_width) {
  __asm {
    mov        eax, [esp + 4]  // src_ptr
    // src_stride ignored
    mov        edx, [esp + 12]  // dst_ptr
    mov        ecx, [esp + 16]  // dst_width
    pcmpeqb    xmm5, xmm5     // generate mask 0x00ff0000
    psrld      xmm5, 24
    pslld      xmm5, 16

  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    psrlw      xmm0, 8
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    sub        ecx, 8
    jg         wloop

    ret
  }
}

// Blends 32x4 rectangle to 8x1.
__declspec(naked) void ScaleRowDown4Box_SSSE3(const uint8_t* src_ptr,
                                              ptrdiff_t src_stride,
                                              uint8_t* dst_ptr,
                                              int dst_width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]  // src_ptr
    mov        esi, [esp + 8 + 8]  // src_stride
    mov        edx, [esp + 8 + 12]  // dst_ptr
    mov        ecx, [esp + 8 + 16]  // dst_width
    lea        edi, [esi + esi * 2]  // src_stride * 3
    pcmpeqb    xmm4, xmm4  // constant 0x0101
    psrlw      xmm4, 15
    movdqa     xmm5, xmm4
    packuswb   xmm4, xmm4
    psllw      xmm5, 3  // constant 0x0008

  wloop:
    movdqu     xmm0, [eax]  // average rows
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + esi]
    movdqu     xmm3, [eax + esi + 16]
    pmaddubsw  xmm0, xmm4  // horizontal add
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    paddw      xmm0, xmm2  // vertical add rows 0, 1
    paddw      xmm1, xmm3
    movdqu     xmm2, [eax + esi * 2]
    movdqu     xmm3, [eax + esi * 2 + 16]
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    paddw      xmm0, xmm2  // add row 2
    paddw      xmm1, xmm3
    movdqu     xmm2, [eax + edi]
    movdqu     xmm3, [eax + edi + 16]
    lea        eax, [eax + 32]
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    paddw      xmm0, xmm2  // add row 3
    paddw      xmm1, xmm3
    phaddw     xmm0, xmm1
    paddw      xmm0, xmm5  // + 8 for round
    psrlw      xmm0, 4  // /16 for average of 4 * 4
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    sub        ecx, 8
    jg         wloop

    pop        edi
    pop        esi
    ret
  }
}

#ifdef HAS_SCALEROWDOWN4_AVX2
// Point samples 64 pixels to 16 pixels.
__declspec(naked) void ScaleRowDown4_AVX2(const uint8_t* src_ptr,
                                          ptrdiff_t src_stride,
                                          uint8_t* dst_ptr,
                                          int dst_width) {
  __asm {
    mov         eax, [esp + 4]  // src_ptr
    // src_stride ignored
    mov         edx, [esp + 12]  // dst_ptr
    mov         ecx, [esp + 16]  // dst_width
    vpcmpeqb    ymm5, ymm5, ymm5  // generate mask 0x00ff0000
    vpsrld      ymm5, ymm5, 24
    vpslld      ymm5, ymm5, 16

  wloop:
    vmovdqu     ymm0, [eax]
    vmovdqu     ymm1, [eax + 32]
    lea         eax,  [eax + 64]
    vpand       ymm0, ymm0, ymm5
    vpand       ymm1, ymm1, ymm5
    vpackuswb   ymm0, ymm0, ymm1
    vpermq      ymm0, ymm0, 0xd8  // unmutate vpackuswb
    vpsrlw      ymm0, ymm0, 8
    vpackuswb   ymm0, ymm0, ymm0
    vpermq      ymm0, ymm0, 0xd8    // unmutate vpackuswb
    vmovdqu     [edx], xmm0
    lea         edx, [edx + 16]
    sub         ecx, 16
    jg          wloop

    vzeroupper
    ret
  }
}

// Blends 64x4 rectangle to 16x1.
__declspec(naked) void ScaleRowDown4Box_AVX2(const uint8_t* src_ptr,
                                             ptrdiff_t src_stride,
                                             uint8_t* dst_ptr,
                                             int dst_width) {
  __asm {
    push        esi
    push        edi
    mov         eax, [esp + 8 + 4]  // src_ptr
    mov         esi, [esp + 8 + 8]  // src_stride
    mov         edx, [esp + 8 + 12]  // dst_ptr
    mov         ecx, [esp + 8 + 16]  // dst_width
    lea         edi, [esi + esi * 2]  // src_stride * 3
    vpcmpeqb    ymm4, ymm4, ymm4  // constant 0x0101
    vpsrlw      ymm4, ymm4, 15
    vpsllw      ymm5, ymm4, 3  // constant 0x0008
    vpackuswb   ymm4, ymm4, ymm4

  wloop:
    vmovdqu     ymm0, [eax]  // average rows
    vmovdqu     ymm1, [eax + 32]
    vmovdqu     ymm2, [eax + esi]
    vmovdqu     ymm3, [eax + esi + 32]
    vpmaddubsw  ymm0, ymm0, ymm4  // horizontal add
    vpmaddubsw  ymm1, ymm1, ymm4
    vpmaddubsw  ymm2, ymm2, ymm4
    vpmaddubsw  ymm3, ymm3, ymm4
    vpaddw      ymm0, ymm0, ymm2  // vertical add rows 0, 1
    vpaddw      ymm1, ymm1, ymm3
    vmovdqu     ymm2, [eax + esi * 2]
    vmovdqu     ymm3, [eax + esi * 2 + 32]
    vpmaddubsw  ymm2, ymm2, ymm4
    vpmaddubsw  ymm3, ymm3, ymm4
    vpaddw      ymm0, ymm0, ymm2  // add row 2
    vpaddw      ymm1, ymm1, ymm3
    vmovdqu     ymm2, [eax + edi]
    vmovdqu     ymm3, [eax + edi + 32]
    lea         eax,  [eax + 64]
    vpmaddubsw  ymm2, ymm2, ymm4
    vpmaddubsw  ymm3, ymm3, ymm4
    vpaddw      ymm0, ymm0, ymm2  // add row 3
    vpaddw      ymm1, ymm1, ymm3
    vphaddw     ymm0, ymm0, ymm1  // mutates
    vpermq      ymm0, ymm0, 0xd8  // unmutate vphaddw
    vpaddw      ymm0, ymm0, ymm5  // + 8 for round
    vpsrlw      ymm0, ymm0, 4  // /32 for average of 4 * 4
    vpackuswb   ymm0, ymm0, ymm0
    vpermq      ymm0, ymm0, 0xd8  // unmutate vpackuswb
    vmovdqu     [edx], xmm0
    lea         edx, [edx + 16]
    sub         ecx, 16
    jg          wloop

    pop        edi
    pop        esi
    vzeroupper
    ret
  }
}
#endif  // HAS_SCALEROWDOWN4_AVX2

// Point samples 32 pixels to 24 pixels.
// Produces three 8 byte values. For each 8 bytes, 16 bytes are read.
// Then shuffled to do the scaling.

__declspec(naked) void ScaleRowDown34_SSSE3(const uint8_t* src_ptr,
                                            ptrdiff_t src_stride,
                                            uint8_t* dst_ptr,
                                            int dst_width) {
  __asm {
    mov        eax, [esp + 4]   // src_ptr
    // src_stride ignored
    mov        edx, [esp + 12]  // dst_ptr
    mov        ecx, [esp + 16]  // dst_width
    movdqa     xmm3, xmmword ptr kShuf0
    movdqa     xmm4, xmmword ptr kShuf1
    movdqa     xmm5, xmmword ptr kShuf2

  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm1
    palignr    xmm1, xmm0, 8
    pshufb     xmm0, xmm3
    pshufb     xmm1, xmm4
    pshufb     xmm2, xmm5
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + 8], xmm1
    movq       qword ptr [edx + 16], xmm2
    lea        edx, [edx + 24]
    sub        ecx, 24
    jg         wloop

    ret
  }
}

// Blends 32x2 rectangle to 24x1
// Produces three 8 byte values. For each 8 bytes, 16 bytes are read.
// Then shuffled to do the scaling.

// Register usage:
// xmm0 src_row 0
// xmm1 src_row 1
// xmm2 shuf 0
// xmm3 shuf 1
// xmm4 shuf 2
// xmm5 madd 0
// xmm6 madd 1
// xmm7 kRound34

// Note that movdqa+palign may be better than movdqu.
__declspec(naked) void ScaleRowDown34_1_Box_SSSE3(const uint8_t* src_ptr,
                                                  ptrdiff_t src_stride,
                                                  uint8_t* dst_ptr,
                                                  int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]  // src_ptr
    mov        esi, [esp + 4 + 8]  // src_stride
    mov        edx, [esp + 4 + 12]  // dst_ptr
    mov        ecx, [esp + 4 + 16]  // dst_width
    movdqa     xmm2, xmmword ptr kShuf01
    movdqa     xmm3, xmmword ptr kShuf11
    movdqa     xmm4, xmmword ptr kShuf21
    movdqa     xmm5, xmmword ptr kMadd01
    movdqa     xmm6, xmmword ptr kMadd11
    movdqa     xmm7, xmmword ptr kRound34

  wloop:
    movdqu     xmm0, [eax]  // pixels 0..7
    movdqu     xmm1, [eax + esi]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm2
    pmaddubsw  xmm0, xmm5
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    movdqu     xmm0, [eax + 8]  // pixels 8..15
    movdqu     xmm1, [eax + esi + 8]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm3
    pmaddubsw  xmm0, xmm6
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx + 8], xmm0
    movdqu     xmm0, [eax + 16]  // pixels 16..23
    movdqu     xmm1, [eax + esi + 16]
    lea        eax, [eax + 32]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm4
    movdqa     xmm1, xmmword ptr kMadd21
    pmaddubsw  xmm0, xmm1
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx + 16], xmm0
    lea        edx, [edx + 24]
    sub        ecx, 24
    jg         wloop

    pop        esi
    ret
  }
}

// Note that movdqa+palign may be better than movdqu.
__declspec(naked) void ScaleRowDown34_0_Box_SSSE3(const uint8_t* src_ptr,
                                                  ptrdiff_t src_stride,
                                                  uint8_t* dst_ptr,
                                                  int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]  // src_ptr
    mov        esi, [esp + 4 + 8]  // src_stride
    mov        edx, [esp + 4 + 12]  // dst_ptr
    mov        ecx, [esp + 4 + 16]  // dst_width
    movdqa     xmm2, xmmword ptr kShuf01
    movdqa     xmm3, xmmword ptr kShuf11
    movdqa     xmm4, xmmword ptr kShuf21
    movdqa     xmm5, xmmword ptr kMadd01
    movdqa     xmm6, xmmword ptr kMadd11
    movdqa     xmm7, xmmword ptr kRound34

  wloop:
    movdqu     xmm0, [eax]  // pixels 0..7
    movdqu     xmm1, [eax + esi]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm2
    pmaddubsw  xmm0, xmm5
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    movdqu     xmm0, [eax + 8]  // pixels 8..15
    movdqu     xmm1, [eax + esi + 8]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm3
    pmaddubsw  xmm0, xmm6
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx + 8], xmm0
    movdqu     xmm0, [eax + 16]  // pixels 16..23
    movdqu     xmm1, [eax + esi + 16]
    lea        eax, [eax + 32]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm4
    movdqa     xmm1, xmmword ptr kMadd21
    pmaddubsw  xmm0, xmm1
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx + 16], xmm0
    lea        edx, [edx+24]
    sub        ecx, 24
    jg         wloop

    pop        esi
    ret
  }
}

static const uvec16 kFadd40 = {0x0040, 0x0040, 0x0040, 0x0040,
                               0x0040, 0x0040, 0x0040, 0x0040};

// Bilinear column filtering. SSSE3 version.
__declspec(naked) void ScaleFilterCols_SSSE3(uint8_t* dst_ptr,
                                             const uint8_t* src_ptr,
                                             int dst_width,
                                             int x,
                                             int dx) {
  __asm {
    push       ebx
    push       esi
    push       edi
    mov        edi, [esp + 12 + 4]  // dst_ptr
    mov        esi, [esp + 12 + 8]  // src_ptr
    mov        ecx, [esp + 12 + 12]  // dst_width
    movd       xmm2, [esp + 12 + 16]  // x
    movd       xmm3, [esp + 12 + 20]  // dx
    mov        eax, 0x04040000  // shuffle to line up fractions with pixel.
    movd       xmm5, eax
    pcmpeqb    xmm6, xmm6  // generate 0x007f for inverting fraction.
    psrlw      xmm6, 9
    pcmpeqb    xmm7, xmm7  // generate 0x00ff mask for extracting 'a'
    psrlw      xmm7, 8
    pextrw     eax, xmm2, 1  // get x0 integer. preroll
    sub        ecx, 2
    jl         xloop29

    movdqa     xmm0, xmm2  // x1 = x0 + dx
    paddd      xmm0, xmm3
    punpckldq  xmm2, xmm0  // x0 x1
    punpckldq  xmm3, xmm3  // dx dx
    paddd      xmm3, xmm3  // dx * 2, dx * 2
    pextrw     edx, xmm2, 3  // get x1 integer. preroll

    // 2 Pixel loop.
  xloop2:
    movdqa     xmm1, xmm2  // x0, x1 fractions.
    paddd      xmm2, xmm3  // x += dx
    movzx      ebx, word ptr [esi + eax]  // 2 source x0 pixels
    movd       xmm0, ebx
    psrlw      xmm1, 9  // 7 bit fractions.
    movzx      ebx, word ptr [esi + edx]  // 2 source x1 pixels
    movd       xmm4, ebx
    pshufb     xmm1, xmm5  // 0011
    punpcklwd  xmm0, xmm4
    pxor       xmm1, xmm6  // 0..7f and 7f..0
    movdqa     xmm4, xmm0  // Copy pixels.
    pmaddubsw  xmm0, xmm1  // a * (127 - f) + b * f
    pand       xmm4, xmm7  // Extract left pixels 'a'.
    paddw      xmm0, xmm4  // a * 128 + f * (b - a)
    pextrw     eax, xmm2, 1  // get x0 integer. next iteration.
    paddw      xmm0, xmmword ptr kFadd40  // add rounding bias 64.
    pextrw     edx, xmm2, 3  // get x1 integer. next iteration.
    psrlw      xmm0, 7  // 8.7 fixed point to low 8 bits.
    packuswb   xmm0, xmm0  // 8 bits, 2 pixels.
    movd       ebx, xmm0
    mov        [edi], bx
    lea        edi, [edi + 2]
    sub        ecx, 2  // 2 pixels
    jge        xloop2

 xloop29:
    add        ecx, 2 - 1
    jl         xloop99

             // 1 pixel remainder
    movzx      ebx, word ptr [esi + eax]  // 2 source x0 pixels
    movd       xmm0, ebx
    psrlw      xmm2, 9  // 7 bit fractions.
    pshufb     xmm2, xmm5  // 0011
    pxor       xmm2, xmm6  // 0..7f and 7f..0
    movdqa     xmm4, xmm0  // Copy pixels.
    pmaddubsw  xmm0, xmm2  // a * (127 - f) + b * f
    pand       xmm4, xmm7  // Extract left pixel 'a'.
    paddw      xmm0, xmm4  // a * 128 + f * (b - a)
    paddw      xmm0, xmmword ptr kFadd40  // add rounding bias 64.
    psrlw      xmm0, 7  // 8.7 fixed point to low 8 bits.
    packuswb   xmm0, xmm0  // 8 bits
    movd       ebx, xmm0
    mov        [edi], bl

 xloop99:

    pop        edi
    pop        esi
    pop        ebx
    ret
  }
}

// Reads 16 pixels, duplicates them and writes 32 pixels.
__declspec(naked) void ScaleColsUp2_SSE2(uint8_t* dst_ptr,
                                         const uint8_t* src_ptr,
                                         int dst_width,
                                         int x,
                                         int dx) {
  __asm {
    mov        edx, [esp + 4]  // dst_ptr
    mov        eax, [esp + 8]  // src_ptr
    mov        ecx, [esp + 12]  // dst_width

  wloop:
    movdqu     xmm0, [eax]
    lea        eax,  [eax + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm0
    punpckhbw  xmm1, xmm1
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx, [edx + 32]
    sub        ecx, 32
    jg         wloop

    ret
  }
}

// Reads 8 pixels, throws half away and writes 4 even pixels (0, 2, 4, 6)
__declspec(naked) void ScaleARGBRowDown2_SSE2(const uint8_t* src_argb,
                                              ptrdiff_t src_stride,
                                              uint8_t* dst_argb,
                                              int dst_width) {
  __asm {
    mov        eax, [esp + 4]   // src_argb
    // src_stride ignored
    mov        edx, [esp + 12]  // dst_argb
    mov        ecx, [esp + 16]  // dst_width

  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    shufps     xmm0, xmm1, 0xdd
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 4
    jg         wloop

    ret
  }
}

// Blends 8x1 rectangle to 4x1.
__declspec(naked) void ScaleARGBRowDown2Linear_SSE2(const uint8_t* src_argb,
                                                    ptrdiff_t src_stride,
                                                    uint8_t* dst_argb,
                                                    int dst_width) {
  __asm {
    mov        eax, [esp + 4]  // src_argb
    // src_stride ignored
    mov        edx, [esp + 12]  // dst_argb
    mov        ecx, [esp + 16]  // dst_width

  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm0
    shufps     xmm0, xmm1, 0x88  // even pixels
    shufps     xmm2, xmm1, 0xdd    // odd pixels
    pavgb      xmm0, xmm2
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 4
    jg         wloop

    ret
  }
}

// Blends 8x2 rectangle to 4x1.
__declspec(naked) void ScaleARGBRowDown2Box_SSE2(const uint8_t* src_argb,
                                                 ptrdiff_t src_stride,
                                                 uint8_t* dst_argb,
                                                 int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]  // src_argb
    mov        esi, [esp + 4 + 8]  // src_stride
    mov        edx, [esp + 4 + 12]  // dst_argb
    mov        ecx, [esp + 4 + 16]  // dst_width

  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + esi]
    movdqu     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2  // average rows
    pavgb      xmm1, xmm3
    movdqa     xmm2, xmm0  // average columns (8 to 4 pixels)
    shufps     xmm0, xmm1, 0x88  // even pixels
    shufps     xmm2, xmm1, 0xdd  // odd pixels
    pavgb      xmm0, xmm2
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 4
    jg         wloop

    pop        esi
    ret
  }
}

// Reads 4 pixels at a time.
__declspec(naked) void ScaleARGBRowDownEven_SSE2(const uint8_t* src_argb,
                                                 ptrdiff_t src_stride,
                                                 int src_stepx,
                                                 uint8_t* dst_argb,
                                                 int dst_width) {
  __asm {
    push       ebx
    push       edi
    mov        eax, [esp + 8 + 4]   // src_argb
    // src_stride ignored
    mov        ebx, [esp + 8 + 12]  // src_stepx
    mov        edx, [esp + 8 + 16]  // dst_argb
    mov        ecx, [esp + 8 + 20]  // dst_width
    lea        ebx, [ebx * 4]
    lea        edi, [ebx + ebx * 2]

  wloop:
    movd       xmm0, [eax]
    movd       xmm1, [eax + ebx]
    punpckldq  xmm0, xmm1
    movd       xmm2, [eax + ebx * 2]
    movd       xmm3, [eax + edi]
    lea        eax,  [eax + ebx * 4]
    punpckldq  xmm2, xmm3
    punpcklqdq xmm0, xmm2
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 4
    jg         wloop

    pop        edi
    pop        ebx
    ret
  }
}

// Blends four 2x2 to 4x1.
__declspec(naked) void ScaleARGBRowDownEvenBox_SSE2(const uint8_t* src_argb,
                                                    ptrdiff_t src_stride,
                                                    int src_stepx,
                                                    uint8_t* dst_argb,
                                                    int dst_width) {
  __asm {
    push       ebx
    push       esi
    push       edi
    mov        eax, [esp + 12 + 4]  // src_argb
    mov        esi, [esp + 12 + 8]  // src_stride
    mov        ebx, [esp + 12 + 12]  // src_stepx
    mov        edx, [esp + 12 + 16]  // dst_argb
    mov        ecx, [esp + 12 + 20]  // dst_width
    lea        esi, [eax + esi]  // row1 pointer
    lea        ebx, [ebx * 4]
    lea        edi, [ebx + ebx * 2]

  wloop:
    movq       xmm0, qword ptr [eax]  // row0 4 pairs
    movhps     xmm0, qword ptr [eax + ebx]
    movq       xmm1, qword ptr [eax + ebx * 2]
    movhps     xmm1, qword ptr [eax + edi]
    lea        eax,  [eax + ebx * 4]
    movq       xmm2, qword ptr [esi]  // row1 4 pairs
    movhps     xmm2, qword ptr [esi + ebx]
    movq       xmm3, qword ptr [esi + ebx * 2]
    movhps     xmm3, qword ptr [esi + edi]
    lea        esi,  [esi + ebx * 4]
    pavgb      xmm0, xmm2  // average rows
    pavgb      xmm1, xmm3
    movdqa     xmm2, xmm0  // average columns (8 to 4 pixels)
    shufps     xmm0, xmm1, 0x88  // even pixels
    shufps     xmm2, xmm1, 0xdd  // odd pixels
    pavgb      xmm0, xmm2
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 4
    jg         wloop

    pop        edi
    pop        esi
    pop        ebx
    ret
  }
}

// Column scaling unfiltered. SSE2 version.
__declspec(naked) void ScaleARGBCols_SSE2(uint8_t* dst_argb,
                                          const uint8_t* src_argb,
                                          int dst_width,
                                          int x,
                                          int dx) {
  __asm {
    push       edi
    push       esi
    mov        edi, [esp + 8 + 4]  // dst_argb
    mov        esi, [esp + 8 + 8]  // src_argb
    mov        ecx, [esp + 8 + 12]  // dst_width
    movd       xmm2, [esp + 8 + 16]  // x
    movd       xmm3, [esp + 8 + 20]  // dx

    pshufd     xmm2, xmm2, 0  // x0 x0 x0 x0
    pshufd     xmm0, xmm3, 0x11  // dx  0 dx  0
    paddd      xmm2, xmm0
    paddd      xmm3, xmm3  // 0, 0, 0,  dx * 2
    pshufd     xmm0, xmm3, 0x05  // dx * 2, dx * 2, 0, 0
    paddd      xmm2, xmm0  // x3 x2 x1 x0
    paddd      xmm3, xmm3  // 0, 0, 0,  dx * 4
    pshufd     xmm3, xmm3, 0  // dx * 4, dx * 4, dx * 4, dx * 4

    pextrw     eax, xmm2, 1  // get x0 integer.
    pextrw     edx, xmm2, 3  // get x1 integer.

    cmp        ecx, 0
    jle        xloop99
    sub        ecx, 4
    jl         xloop49

        // 4 Pixel loop.
 xloop4:
    movd       xmm0, [esi + eax * 4]  // 1 source x0 pixels
    movd       xmm1, [esi + edx * 4]  // 1 source x1 pixels
    pextrw     eax, xmm2, 5  // get x2 integer.
    pextrw     edx, xmm2, 7  // get x3 integer.
    paddd      xmm2, xmm3  // x += dx
    punpckldq  xmm0, xmm1  // x0 x1

    movd       xmm1, [esi + eax * 4]  // 1 source x2 pixels
    movd       xmm4, [esi + edx * 4]  // 1 source x3 pixels
    pextrw     eax, xmm2, 1  // get x0 integer. next iteration.
    pextrw     edx, xmm2, 3  // get x1 integer. next iteration.
    punpckldq  xmm1, xmm4  // x2 x3
    punpcklqdq xmm0, xmm1  // x0 x1 x2 x3
    movdqu     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 4  // 4 pixels
    jge        xloop4

 xloop49:
    test       ecx, 2
    je         xloop29

         // 2 Pixels.
    movd       xmm0, [esi + eax * 4]  // 1 source x0 pixels
    movd       xmm1, [esi + edx * 4]  // 1 source x1 pixels
    pextrw     eax, xmm2, 5  // get x2 integer.
    punpckldq  xmm0, xmm1  // x0 x1

    movq       qword ptr [edi], xmm0
    lea        edi, [edi + 8]

 xloop29:
    test       ecx, 1
    je         xloop99

         // 1 Pixels.
    movd       xmm0, [esi + eax * 4]  // 1 source x2 pixels
    movd       dword ptr [edi], xmm0
 xloop99:

    pop        esi
    pop        edi
    ret
  }
}

// Bilinear row filtering combines 2x1 -> 1x1. SSSE3 version.
// TODO(fbarchard): Port to Neon

// Shuffle table for arranging 2 pixels into pairs for pmaddubsw
static const uvec8 kShuffleColARGB = {
    0u, 4u,  1u, 5u,  2u,  6u,  3u,  7u,  // bbggrraa 1st pixel
    8u, 12u, 9u, 13u, 10u, 14u, 11u, 15u  // bbggrraa 2nd pixel
};

// Shuffle table for duplicating 2 fractions into 8 bytes each
static const uvec8 kShuffleFractions = {
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u,
};

__declspec(naked) void ScaleARGBFilterCols_SSSE3(uint8_t* dst_argb,
                                                 const uint8_t* src_argb,
                                                 int dst_width,
                                                 int x,
                                                 int dx) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]  // dst_argb
    mov        esi, [esp + 8 + 8]  // src_argb
    mov        ecx, [esp + 8 + 12]  // dst_width
    movd       xmm2, [esp + 8 + 16]  // x
    movd       xmm3, [esp + 8 + 20]  // dx
    movdqa     xmm4, xmmword ptr kShuffleColARGB
    movdqa     xmm5, xmmword ptr kShuffleFractions
    pcmpeqb    xmm6, xmm6  // generate 0x007f for inverting fraction.
    psrlw      xmm6, 9
    pextrw     eax, xmm2, 1  // get x0 integer. preroll
    sub        ecx, 2
    jl         xloop29

    movdqa     xmm0, xmm2  // x1 = x0 + dx
    paddd      xmm0, xmm3
    punpckldq  xmm2, xmm0  // x0 x1
    punpckldq  xmm3, xmm3  // dx dx
    paddd      xmm3, xmm3  // dx * 2, dx * 2
    pextrw     edx, xmm2, 3  // get x1 integer. preroll

    // 2 Pixel loop.
  xloop2:
    movdqa     xmm1, xmm2  // x0, x1 fractions.
    paddd      xmm2, xmm3  // x += dx
    movq       xmm0, qword ptr [esi + eax * 4]  // 2 source x0 pixels
    psrlw      xmm1, 9  // 7 bit fractions.
    movhps     xmm0, qword ptr [esi + edx * 4]  // 2 source x1 pixels
    pshufb     xmm1, xmm5  // 0000000011111111
    pshufb     xmm0, xmm4  // arrange pixels into pairs
    pxor       xmm1, xmm6  // 0..7f and 7f..0
    pmaddubsw  xmm0, xmm1  // argb_argb 16 bit, 2 pixels.
    pextrw     eax, xmm2, 1  // get x0 integer. next iteration.
    pextrw     edx, xmm2, 3  // get x1 integer. next iteration.
    psrlw      xmm0, 7  // argb 8.7 fixed point to low 8 bits.
    packuswb   xmm0, xmm0  // argb_argb 8 bits, 2 pixels.
    movq       qword ptr [edi], xmm0
    lea        edi, [edi + 8]
    sub        ecx, 2  // 2 pixels
    jge        xloop2

 xloop29:

    add        ecx, 2 - 1
    jl         xloop99

            // 1 pixel remainder
    psrlw      xmm2, 9  // 7 bit fractions.
    movq       xmm0, qword ptr [esi + eax * 4]  // 2 source x0 pixels
    pshufb     xmm2, xmm5  // 00000000
    pshufb     xmm0, xmm4  // arrange pixels into pairs
    pxor       xmm2, xmm6  // 0..7f and 7f..0
    pmaddubsw  xmm0, xmm2  // argb 16 bit, 1 pixel.
    psrlw      xmm0, 7
    packuswb   xmm0, xmm0  // argb 8 bits, 1 pixel.
    movd       [edi], xmm0

 xloop99:

    pop        edi
    pop        esi
    ret
  }
}

// Reads 4 pixels, duplicates them and writes 8 pixels.
__declspec(naked) void ScaleARGBColsUp2_SSE2(uint8_t* dst_argb,
                                             const uint8_t* src_argb,
                                             int dst_width,
                                             int x,
                                             int dx) {
  __asm {
    mov        edx, [esp + 4]  // dst_argb
    mov        eax, [esp + 8]  // src_argb
    mov        ecx, [esp + 12]  // dst_width

  wloop:
    movdqu     xmm0, [eax]
    lea        eax,  [eax + 16]
    movdqa     xmm1, xmm0
    punpckldq  xmm0, xmm0
    punpckhdq  xmm1, xmm1
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx, [edx + 32]
    sub        ecx, 8
    jg         wloop

    ret
  }
}

// Divide num by div and return as 16.16 fixed point result.
__declspec(naked) int FixedDiv_X86(int num, int div) {
  __asm {
    mov        eax, [esp + 4]  // num
    cdq  // extend num to 64 bits
    shld       edx, eax, 16  // 32.16
    shl        eax, 16
    idiv       dword ptr [esp + 8]
    ret
  }
}

// Divide num by div and return as 16.16 fixed point result.
__declspec(naked) int FixedDiv1_X86(int num, int div) {
  __asm {
    mov        eax, [esp + 4]  // num
    mov        ecx, [esp + 8]  // denom
    cdq  // extend num to 64 bits
    shld       edx, eax, 16  // 32.16
    shl        eax, 16
    sub        eax, 0x00010001
    sbb        edx, 0
    sub        ecx, 1
    idiv       ecx
    ret
  }
}
#endif  // !defined(LIBYUV_DISABLE_X86) && defined(_M_IX86)

// The following are available for Visual C 32/64 bit, and for clang with
// LIBYUV_ENABLE_ROWWIN:
#if !defined(LIBYUV_DISABLE_X86) &&                                 \
    (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || \
     defined(_M_IX86)) &&                                           \
    ((defined(_MSC_VER) && !defined(__clang__)) ||                  \
     defined(LIBYUV_ENABLE_ROWWIN))

#if defined(__clang__) || defined(__GNUC__)
#define LIBYUV_TARGET_SSSE3 __attribute__((target("ssse3")))
#define LIBYUV_TARGET_AVX2 __attribute__((target("avx2")))
#define LIBYUV_TARGET_AVX512BW \
  __attribute__((target("avx512bw,avx512vl,avx512f")))
#define LIBYUV_TARGET_AVX512VBMI \
  __attribute__((target("avx512vbmi,avx512bw,avx512vl,avx512f")))
#else
#define LIBYUV_TARGET_SSSE3
#define LIBYUV_TARGET_AVX2
#define LIBYUV_TARGET_AVX512BW
#define LIBYUV_TARGET_AVX512VBMI
#endif  // defined(__clang__) || defined(__GNUC__)

#ifdef HAS_SCALEROWDOWN38_SSSE3
static const uvec8 kShuf38a = {0,   3,   6,   8,   11,  14,  128, 128,
                               128, 128, 128, 128, 128, 128, 128, 128};

static const uvec8 kShuf38b = {128, 128, 128, 128, 128, 128, 0,   3,
                               6,   8,   11,  14,  128, 128, 128, 128};

// Arrange words 0,3,6 into 0,1,2
static const uvec8 kShufAc = {0,   1,   6,   7,   12,  13,  128, 128,
                              128, 128, 128, 128, 128, 128, 128, 128};

// Arrange words 0,3,6 into 3,4,5
static const uvec8 kShufAc3 = {128, 128, 128, 128, 128, 128, 0,   1,
                               6,   7,   12,  13,  128, 128, 128, 128};

// Scaling values for boxes of 3x3 and 2x3
static const uvec16 kScaleAc33 = {65536 / 9, 65536 / 9, 65536 / 6, 65536 / 9,
                                  65536 / 9, 65536 / 6, 0,         0};

// Arrange first value for pixels 0,1,2,3,4,5
static const uvec8 kShufAb0 = {0,  128, 3,  128, 6,   128, 8,   128,
                               11, 128, 14, 128, 128, 128, 128, 128};

// Arrange second value for pixels 0,1,2,3,4,5
static const uvec8 kShufAb1 = {1,  128, 4,  128, 7,   128, 9,   128,
                               12, 128, 15, 128, 128, 128, 128, 128};

// Arrange third value for pixels 0,1,2,3,4,5
static const uvec8 kShufAb2 = {2,  128, 5,   128, 128, 128, 10,  128,
                               13, 128, 128, 128, 128, 128, 128, 128};

// Scaling values for boxes of 3x2 and 2x2
static const uvec16 kScaleAb2 = {65536 / 3, 65536 / 3, 65536 / 2, 65536 / 3,
                                 65536 / 3, 65536 / 2, 0,         0};

// Scale 32 pixels to 12
LIBYUV_TARGET_SSSE3
void ScaleRowDown38_SSSE3(const uint8_t* src_ptr,
                          ptrdiff_t src_stride,
                          uint8_t* dst_ptr,
                          int dst_width) {
  (void)src_stride;
  const __m128i shuf38a = _mm_loadu_si128((const __m128i*)&kShuf38a);
  const __m128i shuf38b = _mm_loadu_si128((const __m128i*)&kShuf38b);
  while (dst_width > 0) {
    __m128i s0 = _mm_loadu_si128((const __m128i*)src_ptr);
    __m128i s1 = _mm_loadu_si128((const __m128i*)(src_ptr + 16));
    src_ptr += 32;
    s0 = _mm_shuffle_epi8(s0, shuf38a);
    s1 = _mm_shuffle_epi8(s1, shuf38b);
    s0 = _mm_adds_epu8(s0, s1);
    _mm_storel_epi64((__m128i*)dst_ptr, s0);
    *(uint32_t*)(dst_ptr + 8) =
        (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(s0, 8));
    dst_ptr += 12;
    dst_width -= 12;
  }
}

// Scale 16x3 pixels to 6x1 with interpolation
LIBYUV_TARGET_SSSE3
void ScaleRowDown38_3_Box_SSSE3(const uint8_t* src_ptr,
                                ptrdiff_t src_stride,
                                uint8_t* dst_ptr,
                                int dst_width) {
  const __m128i shuf_ac = _mm_loadu_si128((const __m128i*)&kShufAc);
  const __m128i shuf_ac3 = _mm_loadu_si128((const __m128i*)&kShufAc3);
  const __m128i scale_ac33 = _mm_loadu_si128((const __m128i*)&kScaleAc33);
  const __m128i zero = _mm_setzero_si128();
  while (dst_width > 0) {
    __m128i r0 = _mm_loadu_si128((const __m128i*)src_ptr);
    __m128i r1 = _mm_loadu_si128((const __m128i*)(src_ptr + src_stride));
    __m128i r2 = _mm_loadu_si128((const __m128i*)(src_ptr + src_stride * 2));
    src_ptr += 16;
    __m128i s0 = _mm_adds_epu16(_mm_adds_epu16(_mm_unpacklo_epi8(r0, zero),
                                               _mm_unpacklo_epi8(r1, zero)),
                                _mm_unpacklo_epi8(r2, zero));
    __m128i s1 = _mm_adds_epu16(_mm_adds_epu16(_mm_unpackhi_epi8(r0, zero),
                                               _mm_unpackhi_epi8(r1, zero)),
                                _mm_unpackhi_epi8(r2, zero));
    __m128i t0 = _mm_adds_epu16(_mm_adds_epu16(s0, _mm_srli_si128(s0, 2)),
                                _mm_srli_si128(s0, 4));
    __m128i t1 = _mm_adds_epu16(_mm_adds_epu16(s1, _mm_srli_si128(s1, 2)),
                                _mm_srli_si128(s1, 4));
    t0 = _mm_shuffle_epi8(t0, shuf_ac);
    t1 = _mm_shuffle_epi8(t1, shuf_ac3);
    t0 = _mm_adds_epu16(t0, t1);
    t0 = _mm_mulhi_epu16(t0, scale_ac33);
    t0 = _mm_packus_epi16(t0, t0);
    *(uint32_t*)dst_ptr = (uint32_t)_mm_cvtsi128_si32(t0);
    *(uint16_t*)(dst_ptr + 4) =
        (uint16_t)_mm_cvtsi128_si32(_mm_srli_si128(t0, 4));
    dst_ptr += 6;
    dst_width -= 6;
  }
}

// Scale 16x2 pixels to 6x1 with interpolation
LIBYUV_TARGET_SSSE3
void ScaleRowDown38_2_Box_SSSE3(const uint8_t* src_ptr,
                                ptrdiff_t src_stride,
                                uint8_t* dst_ptr,
                                int dst_width) {
  const __m128i shuf_ab0 = _mm_loadu_si128((const __m128i*)&kShufAb0);
  const __m128i shuf_ab1 = _mm_loadu_si128((const __m128i*)&kShufAb1);
  const __m128i shuf_ab2 = _mm_loadu_si128((const __m128i*)&kShufAb2);
  const __m128i scale_ab2 = _mm_loadu_si128((const __m128i*)&kScaleAb2);
  while (dst_width > 0) {
    __m128i r0 = _mm_loadu_si128((const __m128i*)src_ptr);
    __m128i r1 = _mm_loadu_si128((const __m128i*)(src_ptr + src_stride));
    src_ptr += 16;
    r0 = _mm_avg_epu8(r0, r1);
    __m128i t0 = _mm_shuffle_epi8(r0, shuf_ab0);
    __m128i t1 = _mm_shuffle_epi8(r0, shuf_ab1);
    __m128i t2 = _mm_shuffle_epi8(r0, shuf_ab2);
    t0 = _mm_adds_epu16(_mm_adds_epu16(t0, t1), t2);
    t0 = _mm_mulhi_epu16(t0, scale_ab2);
    t0 = _mm_packus_epi16(t0, t0);
    *(uint32_t*)dst_ptr = (uint32_t)_mm_cvtsi128_si32(t0);
    *(uint16_t*)(dst_ptr + 4) =
        (uint16_t)_mm_cvtsi128_si32(_mm_srli_si128(t0, 4));
    dst_ptr += 6;
    dst_width -= 6;
  }
}
#endif  // HAS_SCALEROWDOWN38_SSSE3

#ifdef HAS_SCALEADDROW_SSE2
// Reads 16 bytes and accumulates to 16 shorts at a time.
void ScaleAddRow_SSE2(const uint8_t* src_ptr,
                      uint16_t* dst_ptr,
                      int src_width) {
  const __m128i zero = _mm_setzero_si128();
  while (src_width > 0) {
    __m128i s = _mm_loadu_si128((const __m128i*)src_ptr);
    src_ptr += 16;
    __m128i d0 = _mm_loadu_si128((const __m128i*)dst_ptr);
    __m128i d1 = _mm_loadu_si128((const __m128i*)(dst_ptr + 8));
    d0 = _mm_adds_epu16(d0, _mm_unpacklo_epi8(s, zero));
    d1 = _mm_adds_epu16(d1, _mm_unpackhi_epi8(s, zero));
    _mm_storeu_si128((__m128i*)dst_ptr, d0);
    _mm_storeu_si128((__m128i*)(dst_ptr + 8), d1);
    dst_ptr += 16;
    src_width -= 16;
  }
}
#endif  // HAS_SCALEADDROW_SSE2

#ifdef HAS_SCALEADDROW_AVX2
// Reads 32 bytes and accumulates to 32 shorts at a time.
LIBYUV_TARGET_AVX2
void ScaleAddRow_AVX2(const uint8_t* src_ptr,
                      uint16_t* dst_ptr,
                      int src_width) {
  while (src_width > 0) {
    __m256i s0 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)src_ptr));
    __m256i s1 =
        _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)(src_ptr + 16)));
    src_ptr += 32;
    __m256i d0 = _mm256_loadu_si256((const __m256i*)dst_ptr);
    __m256i d1 = _mm256_loadu_si256((const __m256i*)(dst_ptr + 16));
    d0 = _mm256_adds_epu16(d0, s0);
    d1 = _mm256_adds_epu16(d1, s1);
    _mm256_storeu_si256((__m256i*)dst_ptr, d0);
    _mm256_storeu_si256((__m256i*)(dst_ptr + 16), d1);
    dst_ptr += 32;
    src_width -= 32;
  }
}
#endif  // HAS_SCALEADDROW_AVX2

#ifdef HAS_SCALEADDROW_AVX512BW
// Reads 64 bytes and accumulates to 64 shorts at a time, with masked tail.
LIBYUV_TARGET_AVX512BW
void ScaleAddRow_AVX512BW(const uint8_t* src_ptr,
                          uint16_t* dst_ptr,
                          int src_width) {
  while (src_width >= 64) {
    __m512i s0 =
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)src_ptr));
    __m512i s1 = _mm512_cvtepu8_epi16(
        _mm256_loadu_si256((const __m256i*)(src_ptr + 32)));
    src_ptr += 64;
    __m512i d0 = _mm512_loadu_si512((const __m512i*)dst_ptr);
    __m512i d1 = _mm512_loadu_si512((const __m512i*)(dst_ptr + 32));
    d0 = _mm512_adds_epu16(d0, s0);
    d1 = _mm512_adds_epu16(d1, s1);
    _mm512_storeu_si512((__m512i*)dst_ptr, d0);
    _mm512_storeu_si512((__m512i*)(dst_ptr + 32), d1);
    dst_ptr += 64;
    src_width -= 64;
  }
  if (src_width >= 32) {
    __m512i s0 =
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)src_ptr));
    src_ptr += 32;
    __m512i d0 = _mm512_loadu_si512((const __m512i*)dst_ptr);
    d0 = _mm512_adds_epu16(d0, s0);
    _mm512_storeu_si512((__m512i*)dst_ptr, d0);
    dst_ptr += 32;
    src_width -= 32;
  }
  if (src_width > 0) {
    __mmask32 k = (__mmask32)((1ULL << src_width) - 1);
    __m512i s0 = _mm512_maskz_cvtepu8_epi16(
        k, _mm256_maskz_loadu_epi8(k, (const void*)src_ptr));
    __m512i d0 = _mm512_maskz_loadu_epi16(k, (const void*)dst_ptr);
    d0 = _mm512_maskz_adds_epu16(k, d0, s0);
    _mm512_mask_storeu_epi16((void*)dst_ptr, k, d0);
  }
}
#endif  // HAS_SCALEADDROW_AVX512BW

#ifdef HAS_SCALEADDCOLS_AVX512BW
static const uint8_t kSequence0_15[16] = {0, 1, 2,  3,  4,  5,  6,  7,
                                          8, 9, 10, 11, 12, 13, 14, 15};

LIBYUV_TARGET_AVX512BW
void ScaleAddCols1_AVX512BW(int dst_width,
                            int boxheight,
                            int x,
                            int dx,
                            const uint16_t* src_ptr,
                            uint8_t* dst_ptr) {
  int boxwidth = dx >> 16;
  if (boxwidth < 1) {
    boxwidth = 1;
  }
  int scaleval = 65536 / (boxwidth * boxheight);
  src_ptr += (x >> 16);
  const __m512i mask_ffff = _mm512_set1_epi32(0x0000ffff);
  const __m512i vscale = _mm512_set1_epi32(scaleval);
  if (boxwidth <= 4) {
    __m512i seq =
        _mm512_cvtepu8_epi32(_mm_loadu_si128((const __m128i*)kSequence0_15));
    seq = _mm512_or_si512(seq, _mm512_slli_epi32(seq, 16));
    __m512i vbw = _mm512_set1_epi16((int16_t)boxwidth);
    __m512i base = _mm512_mullo_epi16(seq, vbw);
    __m512i c01 = _mm512_set1_epi32(0x00010000);
    __m512i c23 = _mm512_set1_epi32(0x00030002);
    __m512i idx01 = _mm512_add_epi16(base, c01);
    __m512i idx23 = _mm512_add_epi16(base, c23);
    __mmask32 k2 = _mm512_cmpgt_epu16_mask(vbw, c01);
    __mmask32 k3 = _mm512_cmpgt_epu16_mask(vbw, c23);
    int span = boxwidth * 16;
    uint64_t m64 = (span >= 64) ? ~0ULL : ((1ULL << span) - 1);
    __mmask32 k1 = (__mmask32)m64;
    __mmask32 k4 = (__mmask32)(m64 >> 32);
    while (dst_width >= 16) {
      __m512i s0 = _mm512_maskz_loadu_epi16(k1, src_ptr);
      __m512i s1 = _mm512_maskz_loadu_epi16(k4, src_ptr + 32);
      src_ptr += span;
      __m512i p01 = _mm512_maskz_permutex2var_epi16(k2, s0, idx01, s1);
      __m512i p23 = _mm512_maskz_permutex2var_epi16(k3, s0, idx23, s1);
      __m512i sum01 = _mm512_add_epi32(_mm512_and_si512(p01, mask_ffff),
                                       _mm512_srli_epi32(p01, 16));
      __m512i sum23 = _mm512_add_epi32(_mm512_and_si512(p23, mask_ffff),
                                       _mm512_srli_epi32(p23, 16));
      __m512i sum = _mm512_add_epi32(sum01, sum23);
      sum = _mm512_srli_epi32(_mm512_mullo_epi32(sum, vscale), 16);
      _mm_storeu_si128((__m128i*)dst_ptr, _mm512_cvtepi32_epi8(sum));
      dst_ptr += 16;
      dst_width -= 16;
    }
    if (dst_width > 0) {
      int rem_span = dst_width * boxwidth;
      uint64_t rm64 = (rem_span >= 64) ? ~0ULL : ((1ULL << rem_span) - 1);
      __m512i s0 = _mm512_maskz_loadu_epi16((__mmask32)rm64, src_ptr);
      __m512i s1 =
          _mm512_maskz_loadu_epi16((__mmask32)(rm64 >> 32), src_ptr + 32);
      __m512i p01 = _mm512_maskz_permutex2var_epi16(k2, s0, idx01, s1);
      __m512i p23 = _mm512_maskz_permutex2var_epi16(k3, s0, idx23, s1);
      __m512i sum01 = _mm512_add_epi32(_mm512_and_si512(p01, mask_ffff),
                                       _mm512_srli_epi32(p01, 16));
      __m512i sum23 = _mm512_add_epi32(_mm512_and_si512(p23, mask_ffff),
                                       _mm512_srli_epi32(p23, 16));
      __m512i sum = _mm512_add_epi32(sum01, sum23);
      sum = _mm512_srli_epi32(_mm512_mullo_epi32(sum, vscale), 16);
      __mmask16 km = (__mmask16)((1U << dst_width) - 1);
      _mm512_mask_cvtepi32_storeu_epi8(dst_ptr, km, sum);
    }
  } else if (boxwidth <= 8) {
    __m512i seq =
        _mm512_cvtepu8_epi64(_mm_loadu_si128((const __m128i*)kSequence0_15));
    seq = _mm512_or_si512(seq, _mm512_slli_epi64(seq, 16));
    seq = _mm512_or_si512(seq, _mm512_slli_epi64(seq, 32));
    __m512i vbw = _mm512_set1_epi16((int16_t)boxwidth);
    __m512i base = _mm512_mullo_epi16(seq, vbw);
    __m512i c03 = _mm512_set1_epi64(0x0003000200010000ULL);
    __m512i c47 = _mm512_set1_epi64(0x0007000600050004ULL);
    __m512i idx03 = _mm512_add_epi16(base, c03);
    __m512i idx47 = _mm512_add_epi16(base, c47);
    __mmask32 k3 = _mm512_cmpgt_epu16_mask(vbw, c47);
    int span = boxwidth * 8;
    uint64_t m64 = (span >= 64) ? ~0ULL : ((1ULL << span) - 1);
    __mmask32 k1 = (__mmask32)m64;
    __mmask32 k4 = (__mmask32)(m64 >> 32);
    while (dst_width >= 8) {
      __m512i s0 = _mm512_maskz_loadu_epi16(k1, src_ptr);
      __m512i s1 = _mm512_maskz_loadu_epi16(k4, src_ptr + 32);
      src_ptr += span;
      __m512i p03 = _mm512_permutex2var_epi16(s0, idx03, s1);
      __m512i p47 = _mm512_maskz_permutex2var_epi16(k3, s0, idx47, s1);
      __m512i sum03 = _mm512_add_epi32(_mm512_and_si512(p03, mask_ffff),
                                       _mm512_srli_epi32(p03, 16));
      __m512i sum47 = _mm512_add_epi32(_mm512_and_si512(p47, mask_ffff),
                                       _mm512_srli_epi32(p47, 16));
      __m512i sum = _mm512_add_epi32(sum03, sum47);
      sum = _mm512_add_epi32(sum, _mm512_srli_epi64(sum, 32));
      sum = _mm512_srli_epi32(_mm512_mullo_epi32(sum, vscale), 16);
      _mm_storel_epi64((__m128i*)dst_ptr, _mm512_cvtepi64_epi8(sum));
      dst_ptr += 8;
      dst_width -= 8;
    }
    if (dst_width > 0) {
      int rem_span = dst_width * boxwidth;
      uint64_t rm64 = (rem_span >= 64) ? ~0ULL : ((1ULL << rem_span) - 1);
      __m512i s0 = _mm512_maskz_loadu_epi16((__mmask32)rm64, src_ptr);
      __m512i s1 =
          _mm512_maskz_loadu_epi16((__mmask32)(rm64 >> 32), src_ptr + 32);
      __m512i p03 = _mm512_permutex2var_epi16(s0, idx03, s1);
      __m512i p47 = _mm512_maskz_permutex2var_epi16(k3, s0, idx47, s1);
      __m512i sum03 = _mm512_add_epi32(_mm512_and_si512(p03, mask_ffff),
                                       _mm512_srli_epi32(p03, 16));
      __m512i sum47 = _mm512_add_epi32(_mm512_and_si512(p47, mask_ffff),
                                       _mm512_srli_epi32(p47, 16));
      __m512i sum = _mm512_add_epi32(sum03, sum47);
      sum = _mm512_add_epi32(sum, _mm512_srli_epi64(sum, 32));
      sum = _mm512_srli_epi32(_mm512_mullo_epi32(sum, vscale), 16);
      __mmask8 km = (__mmask8)((1U << dst_width) - 1);
      _mm512_mask_cvtepi64_storeu_epi8(dst_ptr, km, sum);
    }
  } else {
    int rem = boxwidth & 31;
    __mmask32 k1 = (__mmask32)((1ULL << rem) - 1);
    while (dst_width > 0) {
      __m512i acc = _mm512_setzero_si512();
      const uint16_t* p = src_ptr;
      src_ptr += boxwidth;
      int n = boxwidth;
      while (n >= 32) {
        __m512i s = _mm512_loadu_si512((const __m512i*)p);
        p += 32;
        acc = _mm512_add_epi32(acc,
                               _mm512_add_epi32(_mm512_and_si512(s, mask_ffff),
                                                _mm512_srli_epi32(s, 16)));
        n -= 32;
      }
      if (n > 0) {
        __m512i s = _mm512_maskz_loadu_epi16(k1, p);
        acc = _mm512_add_epi32(acc,
                               _mm512_add_epi32(_mm512_and_si512(s, mask_ffff),
                                                _mm512_srli_epi32(s, 16)));
      }
      uint32_t total = (uint32_t)_mm512_reduce_add_epi32(acc);
      *dst_ptr++ = (uint8_t)((total * (uint32_t)scaleval) >> 16);
      --dst_width;
    }
  }
}

LIBYUV_TARGET_AVX512BW
void ScaleAddCols2_AVX512BW(int dst_width,
                            int boxheight,
                            int x,
                            int dx,
                            const uint16_t* src_ptr,
                            uint8_t* dst_ptr) {
  int minboxwidth = dx >> 16;
  int scale0 = 65536 / ((minboxwidth < 1 ? 1 : minboxwidth) * boxheight);
  int scale1 =
      65536 / (((minboxwidth + 1) < 1 ? 1 : (minboxwidth + 1)) * boxheight);
  const __m512i mask_ffff = _mm512_set1_epi32(0x0000ffff);
  const __m512i vscale0 = _mm512_set1_epi32(scale0);
  const __m512i vscale1 = _mm512_set1_epi32(scale1);
  if (minboxwidth >= 1 && minboxwidth <= 3) {
    __m512i seq =
        _mm512_cvtepu8_epi32(_mm_loadu_si128((const __m128i*)kSequence0_15));
    __m512i vdx = _mm512_set1_epi32(dx);
    __m512i step0 = _mm512_mullo_epi32(seq, vdx);
    __m512i step1 = _mm512_add_epi32(step0, vdx);
    __m512i c01 = _mm512_set1_epi32(0x00010000);
    __m512i c23 = _mm512_set1_epi32(0x00030002);
    __m512i vminbw = _mm512_set1_epi32(minboxwidth);
    while (dst_width >= 16) {
      int base_ix = x >> 16;
      __m512i xfrac = _mm512_set1_epi32(x & 0xffff);
      x += dx * 16;
      int span = (x >> 16) - base_ix;
      uint64_t m64 = (span >= 64) ? ~0ULL : ((1ULL << span) - 1);
      __m512i s0 = _mm512_maskz_loadu_epi16((__mmask32)m64, src_ptr + base_ix);
      __m512i s1 = _mm512_maskz_loadu_epi16((__mmask32)(m64 >> 32),
                                            src_ptr + base_ix + 32);
      __m512i ix0 = _mm512_srli_epi32(_mm512_add_epi32(xfrac, step0), 16);
      __m512i ix1 = _mm512_srli_epi32(_mm512_add_epi32(xfrac, step1), 16);
      __m512i bw = _mm512_sub_epi32(ix1, ix0);
      __mmask16 k4 = _mm512_cmpgt_epi32_mask(bw, vminbw);
      __m512i ix0_16 = _mm512_or_si512(ix0, _mm512_slli_epi32(ix0, 16));
      __m512i bw_16 = _mm512_or_si512(bw, _mm512_slli_epi32(bw, 16));
      __m512i idx01 = _mm512_add_epi16(ix0_16, c01);
      __m512i idx23 = _mm512_add_epi16(ix0_16, c23);
      __mmask32 k2 = _mm512_cmpgt_epu16_mask(bw_16, c01);
      __mmask32 k3 = _mm512_cmpgt_epu16_mask(bw_16, c23);
      __m512i p01 = _mm512_maskz_permutex2var_epi16(k2, s0, idx01, s1);
      __m512i p23 = _mm512_maskz_permutex2var_epi16(k3, s0, idx23, s1);
      __m512i sum01 = _mm512_add_epi32(_mm512_and_si512(p01, mask_ffff),
                                       _mm512_srli_epi32(p01, 16));
      __m512i sum23 = _mm512_add_epi32(_mm512_and_si512(p23, mask_ffff),
                                       _mm512_srli_epi32(p23, 16));
      __m512i sum = _mm512_add_epi32(sum01, sum23);
      __m512i vsc = _mm512_mask_blend_epi32(k4, vscale0, vscale1);
      sum = _mm512_srli_epi32(_mm512_mullo_epi32(sum, vsc), 16);
      _mm_storeu_si128((__m128i*)dst_ptr, _mm512_cvtepi32_epi8(sum));
      dst_ptr += 16;
      dst_width -= 16;
    }
    if (dst_width > 0) {
      int base_ix = x >> 16;
      __m512i xfrac = _mm512_set1_epi32(x & 0xffff);
      int end_x = x + dx * dst_width;
      int span = (end_x >> 16) - base_ix;
      uint64_t m64 = (span >= 64) ? ~0ULL : ((1ULL << span) - 1);
      __m512i s0 = _mm512_maskz_loadu_epi16((__mmask32)m64, src_ptr + base_ix);
      __m512i s1 = _mm512_maskz_loadu_epi16((__mmask32)(m64 >> 32),
                                            src_ptr + base_ix + 32);
      __m512i ix0 = _mm512_srli_epi32(_mm512_add_epi32(xfrac, step0), 16);
      __m512i ix1 = _mm512_srli_epi32(_mm512_add_epi32(xfrac, step1), 16);
      __m512i bw = _mm512_sub_epi32(ix1, ix0);
      __mmask16 k4 = _mm512_cmpgt_epi32_mask(bw, vminbw);
      __m512i ix0_16 = _mm512_or_si512(ix0, _mm512_slli_epi32(ix0, 16));
      __m512i bw_16 = _mm512_or_si512(bw, _mm512_slli_epi32(bw, 16));
      __m512i idx01 = _mm512_add_epi16(ix0_16, c01);
      __m512i idx23 = _mm512_add_epi16(ix0_16, c23);
      __mmask32 k2 = _mm512_cmpgt_epu16_mask(bw_16, c01);
      __mmask32 k3 = _mm512_cmpgt_epu16_mask(bw_16, c23);
      __m512i p01 = _mm512_maskz_permutex2var_epi16(k2, s0, idx01, s1);
      __m512i p23 = _mm512_maskz_permutex2var_epi16(k3, s0, idx23, s1);
      __m512i sum01 = _mm512_add_epi32(_mm512_and_si512(p01, mask_ffff),
                                       _mm512_srli_epi32(p01, 16));
      __m512i sum23 = _mm512_add_epi32(_mm512_and_si512(p23, mask_ffff),
                                       _mm512_srli_epi32(p23, 16));
      __m512i sum = _mm512_add_epi32(sum01, sum23);
      __m512i vsc = _mm512_mask_blend_epi32(k4, vscale0, vscale1);
      sum = _mm512_srli_epi32(_mm512_mullo_epi32(sum, vsc), 16);
      __mmask16 km = (__mmask16)((1U << dst_width) - 1);
      _mm512_mask_cvtepi32_storeu_epi8(dst_ptr, km, sum);
    }
  } else if (minboxwidth >= 4 && minboxwidth <= 7) {
    __m512i seq =
        _mm512_cvtepu8_epi64(_mm_loadu_si128((const __m128i*)kSequence0_15));
    __m512i vdx = _mm512_set1_epi64(dx);
    __m512i step0 = _mm512_mullo_epi32(seq, vdx);
    __m512i step1 = _mm512_add_epi64(step0, vdx);
    __m512i c03 = _mm512_set1_epi64(0x0003000200010000ULL);
    __m512i c47 = _mm512_set1_epi64(0x0007000600050004ULL);
    __m512i vminbw = _mm512_set1_epi64(minboxwidth);
    while (dst_width >= 8) {
      int base_ix = x >> 16;
      __m512i xfrac = _mm512_set1_epi64(x & 0xffff);
      x += dx * 8;
      int span = (x >> 16) - base_ix;
      uint64_t m64 = (span >= 64) ? ~0ULL : ((1ULL << span) - 1);
      __m512i s0 = _mm512_maskz_loadu_epi16((__mmask32)m64, src_ptr + base_ix);
      __m512i s1 = _mm512_maskz_loadu_epi16((__mmask32)(m64 >> 32),
                                            src_ptr + base_ix + 32);
      __m512i ix0 = _mm512_srli_epi32(_mm512_add_epi32(xfrac, step0), 16);
      __m512i ix1 = _mm512_srli_epi32(_mm512_add_epi32(xfrac, step1), 16);
      __m512i bw = _mm512_sub_epi32(ix1, ix0);
      __mmask8 k4 = _mm512_cmpgt_epi64_mask(bw, vminbw);
      __m512i ix0_16 = _mm512_or_si512(ix0, _mm512_slli_epi64(ix0, 16));
      ix0_16 = _mm512_or_si512(ix0_16, _mm512_slli_epi64(ix0_16, 32));
      __m512i bw_16 = _mm512_or_si512(bw, _mm512_slli_epi64(bw, 16));
      bw_16 = _mm512_or_si512(bw_16, _mm512_slli_epi64(bw_16, 32));
      __m512i idx03 = _mm512_add_epi16(ix0_16, c03);
      __m512i idx47 = _mm512_add_epi16(ix0_16, c47);
      __mmask32 k3 = _mm512_cmpgt_epu16_mask(bw_16, c47);
      __m512i p03 = _mm512_permutex2var_epi16(s0, idx03, s1);
      __m512i p47 = _mm512_maskz_permutex2var_epi16(k3, s0, idx47, s1);
      __m512i sum03 = _mm512_add_epi32(_mm512_and_si512(p03, mask_ffff),
                                       _mm512_srli_epi32(p03, 16));
      __m512i sum47 = _mm512_add_epi32(_mm512_and_si512(p47, mask_ffff),
                                       _mm512_srli_epi32(p47, 16));
      __m512i sum = _mm512_add_epi32(sum03, sum47);
      sum = _mm512_add_epi32(sum, _mm512_srli_epi64(sum, 32));
      __m512i vsc = _mm512_mask_blend_epi64(k4, vscale0, vscale1);
      sum = _mm512_srli_epi32(_mm512_mullo_epi32(sum, vsc), 16);
      _mm_storel_epi64((__m128i*)dst_ptr, _mm512_cvtepi64_epi8(sum));
      dst_ptr += 8;
      dst_width -= 8;
    }
    if (dst_width > 0) {
      int base_ix = x >> 16;
      __m512i xfrac = _mm512_set1_epi64(x & 0xffff);
      int end_x = x + dx * dst_width;
      int span = (end_x >> 16) - base_ix;
      uint64_t m64 = (span >= 64) ? ~0ULL : ((1ULL << span) - 1);
      __m512i s0 = _mm512_maskz_loadu_epi16((__mmask32)m64, src_ptr + base_ix);
      __m512i s1 = _mm512_maskz_loadu_epi16((__mmask32)(m64 >> 32),
                                            src_ptr + base_ix + 32);
      __m512i ix0 = _mm512_srli_epi32(_mm512_add_epi32(xfrac, step0), 16);
      __m512i ix1 = _mm512_srli_epi32(_mm512_add_epi32(xfrac, step1), 16);
      __m512i bw = _mm512_sub_epi32(ix1, ix0);
      __mmask8 k4 = _mm512_cmpgt_epi64_mask(bw, vminbw);
      __m512i ix0_16 = _mm512_or_si512(ix0, _mm512_slli_epi64(ix0, 16));
      ix0_16 = _mm512_or_si512(ix0_16, _mm512_slli_epi64(ix0_16, 32));
      __m512i bw_16 = _mm512_or_si512(bw, _mm512_slli_epi64(bw, 16));
      bw_16 = _mm512_or_si512(bw_16, _mm512_slli_epi64(bw_16, 32));
      __m512i idx03 = _mm512_add_epi16(ix0_16, c03);
      __m512i idx47 = _mm512_add_epi16(ix0_16, c47);
      __mmask32 k3 = _mm512_cmpgt_epu16_mask(bw_16, c47);
      __m512i p03 = _mm512_permutex2var_epi16(s0, idx03, s1);
      __m512i p47 = _mm512_maskz_permutex2var_epi16(k3, s0, idx47, s1);
      __m512i sum03 = _mm512_add_epi32(_mm512_and_si512(p03, mask_ffff),
                                       _mm512_srli_epi32(p03, 16));
      __m512i sum47 = _mm512_add_epi32(_mm512_and_si512(p47, mask_ffff),
                                       _mm512_srli_epi32(p47, 16));
      __m512i sum = _mm512_add_epi32(sum03, sum47);
      sum = _mm512_add_epi32(sum, _mm512_srli_epi64(sum, 32));
      __m512i vsc = _mm512_mask_blend_epi64(k4, vscale0, vscale1);
      sum = _mm512_srli_epi32(_mm512_mullo_epi32(sum, vsc), 16);
      __mmask8 km = (__mmask8)((1U << dst_width) - 1);
      _mm512_mask_cvtepi64_storeu_epi8(dst_ptr, km, sum);
    }
  } else {
    while (dst_width > 0) {
      int ix = x >> 16;
      x += dx;
      int bw = (x >> 16) - ix;
      if (bw < 1) {
        bw = 1;
      }
      int sc = (bw > minboxwidth) ? scale1 : scale0;
      __m512i acc = _mm512_setzero_si512();
      const uint16_t* p = src_ptr + ix;
      int n = bw;
      while (n >= 32) {
        __m512i s = _mm512_loadu_si512((const __m512i*)p);
        p += 32;
        acc = _mm512_add_epi32(acc,
                               _mm512_add_epi32(_mm512_and_si512(s, mask_ffff),
                                                _mm512_srli_epi32(s, 16)));
        n -= 32;
      }
      if (n > 0) {
        __mmask32 k1 = (__mmask32)((1ULL << n) - 1);
        __m512i s = _mm512_maskz_loadu_epi16(k1, p);
        acc = _mm512_add_epi32(acc,
                               _mm512_add_epi32(_mm512_and_si512(s, mask_ffff),
                                                _mm512_srli_epi32(s, 16)));
      }
      uint32_t total = (uint32_t)_mm512_reduce_add_epi32(acc);
      *dst_ptr++ = (uint8_t)((total * (uint32_t)sc) >> 16);
      --dst_width;
    }
  }
}
#endif  // HAS_SCALEADDCOLS_AVX512BW

#ifdef HAS_SCALEUVADDCOLS3_AVX2
static const ulvec32 kPermUV3_0 = {0, 3, 6, 1, 4, 7, 2, 5};
static const ulvec32 kPermUV3_1 = {3, 4, 5, 6, 7, 0, 1, 2};
static const ulvec32 kPermUV3_2 = {6, 7, 0, 1, 2, 3, 4, 5};

// Scale UV columns by 3 box filter.
// Reads 96 bytes (24 UV pixels) and writes 16 bytes (8 UV pixels) per block.
LIBYUV_TARGET_AVX2
void ScaleUVAddCols3_AVX2(int dst_width,
                          int scaleval,
                          const uint16_t* src_uv,
                          uint8_t* dst_uv) {
  const __m256i vscale = _mm256_set1_epi16((int16_t)scaleval);
  const __m256i perm0 = _mm256_loadu_si256((const __m256i*)&kPermUV3_0);
  const __m256i perm1 = _mm256_loadu_si256((const __m256i*)&kPermUV3_1);
  const __m256i perm2 = _mm256_loadu_si256((const __m256i*)&kPermUV3_2);
  while (dst_width >= 16) {
    __m256i a0 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)src_uv), perm0);
    __m256i a1 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)(src_uv + 16)), perm0);
    __m256i a2 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)(src_uv + 32)), perm0);
    __m256i c0 = _mm256_blend_epi32(_mm256_blend_epi32(a0, a1, 0x38), a2, 0xc0);
    __m256i c1 = _mm256_permutevar8x32_epi32(
        _mm256_blend_epi32(_mm256_blend_epi32(a0, a1, 0xc0), a2, 0x07), perm1);
    __m256i c2 = _mm256_permutevar8x32_epi32(
        _mm256_blend_epi32(_mm256_blend_epi32(a0, a1, 0x07), a2, 0x38), perm2);
    __m256i sum0 = _mm256_mulhi_epu16(
        _mm256_add_epi16(_mm256_add_epi16(c0, c1), c2), vscale);

    __m256i b0 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)(src_uv + 48)), perm0);
    __m256i b1 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)(src_uv + 64)), perm0);
    __m256i b2 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)(src_uv + 80)), perm0);
    src_uv += 96;
    __m256i d0 = _mm256_blend_epi32(_mm256_blend_epi32(b0, b1, 0x38), b2, 0xc0);
    __m256i d1 = _mm256_permutevar8x32_epi32(
        _mm256_blend_epi32(_mm256_blend_epi32(b0, b1, 0xc0), b2, 0x07), perm1);
    __m256i d2 = _mm256_permutevar8x32_epi32(
        _mm256_blend_epi32(_mm256_blend_epi32(b0, b1, 0x07), b2, 0x38), perm2);
    __m256i sum1 = _mm256_mulhi_epu16(
        _mm256_add_epi16(_mm256_add_epi16(d0, d1), d2), vscale);

    __m256i packed =
        _mm256_permute4x64_epi64(_mm256_packus_epi16(sum0, sum1), 0xd8);
    _mm256_storeu_si256((__m256i*)dst_uv, packed);
    dst_uv += 32;
    dst_width -= 16;
  }
  while (dst_width > 0) {
    __m256i a0 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)src_uv), perm0);
    __m256i a1 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)(src_uv + 16)), perm0);
    __m256i a2 = _mm256_permutevar8x32_epi32(
        _mm256_loadu_si256((const __m256i*)(src_uv + 32)), perm0);
    src_uv += 48;
    __m256i c0 = _mm256_blend_epi32(_mm256_blend_epi32(a0, a1, 0x38), a2, 0xc0);
    __m256i c1 = _mm256_permutevar8x32_epi32(
        _mm256_blend_epi32(_mm256_blend_epi32(a0, a1, 0xc0), a2, 0x07), perm1);
    __m256i c2 = _mm256_permutevar8x32_epi32(
        _mm256_blend_epi32(_mm256_blend_epi32(a0, a1, 0x07), a2, 0x38), perm2);
    __m256i sum = _mm256_mulhi_epu16(
        _mm256_add_epi16(_mm256_add_epi16(c0, c1), c2), vscale);
    __m128i lo = _mm256_castsi256_si128(sum);
    __m128i hi = _mm256_extracti128_si256(sum, 1);
    _mm_storeu_si128((__m128i*)dst_uv, _mm_packus_epi16(lo, hi));
    dst_uv += 16;
    dst_width -= 8;
  }
}
#endif  // HAS_SCALEUVADDCOLS3_AVX2

#ifdef HAS_SCALEUVADDCOLS3_AVX512VBMI
static const uint8_t kPerm512_Col0[16] = {0,  3,  6,  9, 12, 15, 18, 21,
                                          24, 27, 30, 1, 4,  7,  10, 13};
static const uint8_t kPerm512_Col1[16] = {1,  4,  7,  10, 13, 16, 19, 22,
                                          25, 28, 31, 2,  5,  8,  11, 14};
static const uint8_t kPerm512_Col2[16] = {2,  5,  8, 11, 14, 17, 20, 23,
                                          26, 29, 0, 3,  6,  9,  12, 15};
static const uint8_t kPackEvenBytes[64] = {
    0,   2,   4,   6,   8,   10,  12,  14,  16,  18,  20,  22,  24,
    26,  28,  30,  32,  34,  36,  38,  40,  42,  44,  46,  48,  50,
    52,  54,  56,  58,  60,  62,  64,  66,  68,  70,  72,  74,  76,
    78,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98,  100, 102,
    104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126};

// Scale UV columns by 3 box filter using AVX512VBMI.
// Processes 32 UV pixels (384 input bytes -> 64 output bytes) per main loop
// iteration, followed by a 16-pixel block and a k-masked tail (1..15 pixels).
LIBYUV_TARGET_AVX512VBMI
void ScaleUVAddCols3_AVX512VBMI(int dst_width,
                                int scaleval,
                                const uint16_t* src_uv,
                                uint8_t* dst_uv) {
  const __m512i vscale = _mm512_set1_epi16((int16_t)scaleval);
  const __m512i col0 =
      _mm512_cvtepu8_epi32(_mm_loadu_si128((const __m128i*)kPerm512_Col0));
  const __m512i col1 =
      _mm512_cvtepu8_epi32(_mm_loadu_si128((const __m128i*)kPerm512_Col1));
  const __m512i col2 =
      _mm512_cvtepu8_epi32(_mm_loadu_si128((const __m128i*)kPerm512_Col2));
  const __m512i pack_even = _mm512_loadu_si512((const __m512i*)kPackEvenBytes);
  const __mmask16 k5 = 0xf800;
  const __mmask16 k6 = 0xfc00;

  while (dst_width >= 32) {
    __m512i s0 = _mm512_loadu_si512((const __m512i*)src_uv);
    __m512i s1 = _mm512_loadu_si512((const __m512i*)(src_uv + 32));
    __m512i s2 = _mm512_loadu_si512((const __m512i*)(src_uv + 64));
    __m512i a0 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col0, s1), k5, col0, s2);
    __m512i a1 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col1, s1), k5, col1, s2);
    __m512i a2 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col2, s1), k6, col2, s2);
    __m512i sum0 = _mm512_mulhi_epu16(
        _mm512_add_epi16(_mm512_add_epi16(a0, a1), a2), vscale);

    __m512i s3 = _mm512_loadu_si512((const __m512i*)(src_uv + 96));
    __m512i s4 = _mm512_loadu_si512((const __m512i*)(src_uv + 128));
    __m512i s5 = _mm512_loadu_si512((const __m512i*)(src_uv + 160));
    src_uv += 192;
    __m512i b0 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s3, col0, s4), k5, col0, s5);
    __m512i b1 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s3, col1, s4), k5, col1, s5);
    __m512i b2 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s3, col2, s4), k6, col2, s5);
    __m512i sum1 = _mm512_mulhi_epu16(
        _mm512_add_epi16(_mm512_add_epi16(b0, b1), b2), vscale);

    __m512i out = _mm512_permutex2var_epi8(sum0, pack_even, sum1);
    _mm512_storeu_si512((__m512i*)dst_uv, out);
    dst_uv += 64;
    dst_width -= 32;
  }
  if (dst_width >= 16) {
    __m512i s0 = _mm512_loadu_si512((const __m512i*)src_uv);
    __m512i s1 = _mm512_loadu_si512((const __m512i*)(src_uv + 32));
    __m512i s2 = _mm512_loadu_si512((const __m512i*)(src_uv + 64));
    src_uv += 96;
    __m512i a0 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col0, s1), k5, col0, s2);
    __m512i a1 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col1, s1), k5, col1, s2);
    __m512i a2 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col2, s1), k6, col2, s2);
    __m512i sum0 = _mm512_mulhi_epu16(
        _mm512_add_epi16(_mm512_add_epi16(a0, a1), a2), vscale);
    _mm256_storeu_si256((__m256i*)dst_uv, _mm512_cvtepi16_epi8(sum0));
    dst_uv += 32;
    dst_width -= 16;
  }
  if (dst_width > 0) {
    uint64_t m64 = (1ULL << (dst_width * 3)) - 1;
    __mmask16 k1 = (__mmask16)m64;
    __mmask16 k2 = (__mmask16)(m64 >> 16);
    __mmask16 k3 = (__mmask16)(m64 >> 32);
    __mmask16 k4 = (__mmask16)((1U << dst_width) - 1);
    __m512i s0 = _mm512_maskz_loadu_epi32(k1, src_uv);
    __m512i s1 = _mm512_maskz_loadu_epi32(k2, src_uv + 32);
    __m512i s2 = _mm512_maskz_loadu_epi32(k3, src_uv + 64);
    __m512i a0 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col0, s1), k5, col0, s2);
    __m512i a1 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col1, s1), k5, col1, s2);
    __m512i a2 = _mm512_mask_permutexvar_epi32(
        _mm512_permutex2var_epi32(s0, col2, s1), k6, col2, s2);
    __m512i sum0 = _mm512_mulhi_epu16(
        _mm512_add_epi16(_mm512_add_epi16(a0, a1), a2), vscale);
    _mm256_mask_storeu_epi16(dst_uv, k4, _mm512_cvtepi16_epi8(sum0));
  }
}
#endif  // HAS_SCALEUVADDCOLS3_AVX512VBMI

#undef LIBYUV_TARGET_SSSE3
#undef LIBYUV_TARGET_AVX2
#undef LIBYUV_TARGET_AVX512BW
#undef LIBYUV_TARGET_AVX512VBMI

#endif  // !defined(LIBYUV_DISABLE_X86) && ...

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

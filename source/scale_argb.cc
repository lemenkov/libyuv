/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/scale.h"

#include <assert.h>
#include <string.h>

#include "libyuv/cpu_id.h"
#include "libyuv/planar_functions.h"  // For CopyARGB
#include "libyuv/row.h"
#include "../source/scale_row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

static __inline int Abs(int v) {
  return v >= 0 ? v : -v;
}

// ARGB scaling uses bilinear or point, but not box filter.
#if !defined(LIBYUV_DISABLE_NEON) && !defined(__native_client__) && \
    (defined(__ARM_NEON__) || defined(LIBYUV_NEON))
#define HAS_SCALEARGBROWDOWNEVEN_NEON
#define HAS_SCALEARGBROWDOWN2_NEON
void ScaleARGBRowDownEven_NEON(const uint8* src_argb, int src_stride,
                               int src_stepx,
                               uint8* dst_argb, int dst_width);
void ScaleARGBRowDownEvenBox_NEON(const uint8* src_argb, int src_stride,
                                  int src_stepx,
                                  uint8* dst_argb, int dst_width);
void ScaleARGBRowDown2_NEON(const uint8* src_ptr, ptrdiff_t /* src_stride */,
                            uint8* dst, int dst_width);
void ScaleARGBRowDown2Box_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                               uint8* dst, int dst_width);
#endif

#if !defined(LIBYUV_DISABLE_X86) && \
    defined(_M_IX86) && defined(_MSC_VER)
#define HAS_SCALEARGBROWDOWN2_SSE2
// Reads 8 pixels, throws half away and writes 4 even pixels (0, 2, 4, 6)
// Alignment requirement: src_argb 16 byte aligned, dst_argb 16 byte aligned.
__declspec(naked) __declspec(align(16))
static void ScaleARGBRowDown2_SSE2(const uint8* src_argb,
                                   ptrdiff_t /* src_stride */,
                                   uint8* dst_argb, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        // src_argb
                                     // src_stride ignored
    mov        edx, [esp + 12]       // dst_argb
    mov        ecx, [esp + 16]       // dst_width

    align      16
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    shufps     xmm0, xmm1, 0xdd
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    ret
  }
}

// Blends 8x2 rectangle to 4x1.
// Alignment requirement: src_argb 16 byte aligned, dst_argb 16 byte aligned.
__declspec(naked) __declspec(align(16))
static void ScaleARGBRowDown2Box_SSE2(const uint8* src_argb,
                                      ptrdiff_t src_stride,
                                      uint8* dst_argb, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    // src_argb
    mov        esi, [esp + 4 + 8]    // src_stride
    mov        edx, [esp + 4 + 12]   // dst_argb
    mov        ecx, [esp + 4 + 16]   // dst_width

    align      16
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2            // average rows
    pavgb      xmm1, xmm3
    movdqa     xmm2, xmm0            // average columns (8 to 4 pixels)
    shufps     xmm0, xmm1, 0x88      // even pixels
    shufps     xmm2, xmm1, 0xdd      // odd pixels
    pavgb      xmm0, xmm2
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    pop        esi
    ret
  }
}

#define HAS_SCALEARGBROWDOWNEVEN_SSE2
// Reads 4 pixels at a time.
// Alignment requirement: dst_argb 16 byte aligned.
__declspec(naked) __declspec(align(16))
void ScaleARGBRowDownEven_SSE2(const uint8* src_argb, ptrdiff_t src_stride,
                               int src_stepx,
                               uint8* dst_argb, int dst_width) {
  __asm {
    push       ebx
    push       edi
    mov        eax, [esp + 8 + 4]    // src_argb
                                     // src_stride ignored
    mov        ebx, [esp + 8 + 12]   // src_stepx
    mov        edx, [esp + 8 + 16]   // dst_argb
    mov        ecx, [esp + 8 + 20]   // dst_width
    lea        ebx, [ebx * 4]
    lea        edi, [ebx + ebx * 2]

    align      16
  wloop:
    movd       xmm0, [eax]
    movd       xmm1, [eax + ebx]
    punpckldq  xmm0, xmm1
    movd       xmm2, [eax + ebx * 2]
    movd       xmm3, [eax + edi]
    lea        eax,  [eax + ebx * 4]
    punpckldq  xmm2, xmm3
    punpcklqdq xmm0, xmm2
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    pop        edi
    pop        ebx
    ret
  }
}

// Blends four 2x2 to 4x1.
// Alignment requirement: dst_argb 16 byte aligned.
__declspec(naked) __declspec(align(16))
static void ScaleARGBRowDownEvenBox_SSE2(const uint8* src_argb,
                                         ptrdiff_t src_stride,
                                         int src_stepx,
                                         uint8* dst_argb, int dst_width) {
  __asm {
    push       ebx
    push       esi
    push       edi
    mov        eax, [esp + 12 + 4]    // src_argb
    mov        esi, [esp + 12 + 8]    // src_stride
    mov        ebx, [esp + 12 + 12]   // src_stepx
    mov        edx, [esp + 12 + 16]   // dst_argb
    mov        ecx, [esp + 12 + 20]   // dst_width
    lea        esi, [eax + esi]       // row1 pointer
    lea        ebx, [ebx * 4]
    lea        edi, [ebx + ebx * 2]

    align      16
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
    pavgb      xmm0, xmm2            // average rows
    pavgb      xmm1, xmm3
    movdqa     xmm2, xmm0            // average columns (8 to 4 pixels)
    shufps     xmm0, xmm1, 0x88      // even pixels
    shufps     xmm2, xmm1, 0xdd      // odd pixels
    pavgb      xmm0, xmm2
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    pop        edi
    pop        esi
    pop        ebx
    ret
  }
}

// Column scaling unfiltered. SSSE3 version.
// TODO(fbarchard): Port to Neon

#define HAS_SCALEARGBCOLS_SSE2
__declspec(naked) __declspec(align(16))
static void ScaleARGBCols_SSE2(uint8* dst_argb, const uint8* src_argb,
                               int dst_width, int x, int dx) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]    // dst_argb
    mov        esi, [esp + 8 + 8]    // src_argb
    mov        ecx, [esp + 8 + 12]   // dst_width
    movd       xmm2, [esp + 8 + 16]  // x
    movd       xmm3, [esp + 8 + 20]  // dx
    pextrw     eax, xmm2, 1          // get x0 integer. preroll
    sub        ecx, 2
    jl         xloop29

    movdqa     xmm0, xmm2           // x1 = x0 + dx
    paddd      xmm0, xmm3
    punpckldq  xmm2, xmm0           // x0 x1
    punpckldq  xmm3, xmm3           // dx dx
    paddd      xmm3, xmm3           // dx * 2, dx * 2
    pextrw     edx, xmm2, 3         // get x1 integer. preroll

    // 2 Pixel loop.
    align      16
  xloop2:
    paddd      xmm2, xmm3           // x += dx
    movd       xmm0, qword ptr [esi + eax * 4]  // 1 source x0 pixels
    movd       xmm1, qword ptr [esi + edx * 4]  // 1 source x1 pixels
    punpckldq  xmm0, xmm1           // x0 x1
    pextrw     eax, xmm2, 1         // get x0 integer. next iteration.
    pextrw     edx, xmm2, 3         // get x1 integer. next iteration.
    movq       qword ptr [edi], xmm0
    lea        edi, [edi + 8]
    sub        ecx, 2               // 2 pixels
    jge        xloop2

    align      16
 xloop29:

    add        ecx, 2 - 1
    jl         xloop99

    // 1 pixel remainder
    movd       xmm0, qword ptr [esi + eax * 4]  // 1 source x0 pixels
    movd       [edi], xmm0

    align      16
 xloop99:

    pop        edi
    pop        esi
    ret
  }
}

// Bilinear row filtering combines 2x1 -> 1x1. SSSE3 version.
// TODO(fbarchard): Port to Neon

// Shuffle table for arranging 2 pixels into pairs for pmaddubsw
static uvec8 kShuffleColARGB = {
  0u, 4u, 1u, 5u, 2u, 6u, 3u, 7u,  // bbggrraa 1st pixel
  8u, 12u, 9u, 13u, 10u, 14u, 11u, 15u  // bbggrraa 2nd pixel
};

// Shuffle table for duplicating 2 fractions into 8 bytes each
static uvec8 kShuffleFractions = {
  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u,
};

#define HAS_SCALEARGBFILTERCOLS_SSSE3
__declspec(naked) __declspec(align(16))
static void ScaleARGBFilterCols_SSSE3(uint8* dst_argb, const uint8* src_argb,
                                      int dst_width, int x, int dx) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]    // dst_argb
    mov        esi, [esp + 8 + 8]    // src_argb
    mov        ecx, [esp + 8 + 12]   // dst_width
    movd       xmm2, [esp + 8 + 16]  // x
    movd       xmm3, [esp + 8 + 20]  // dx
    movdqa     xmm4, kShuffleColARGB
    movdqa     xmm5, kShuffleFractions
    pcmpeqb    xmm6, xmm6           // generate 0x007f for inverting fraction.
    psrlw      xmm6, 9
    pextrw     eax, xmm2, 1         // get x0 integer. preroll
    sub        ecx, 2
    jl         xloop29

    movdqa     xmm0, xmm2           // x1 = x0 + dx
    paddd      xmm0, xmm3
    punpckldq  xmm2, xmm0           // x0 x1
    punpckldq  xmm3, xmm3           // dx dx
    paddd      xmm3, xmm3           // dx * 2, dx * 2
    pextrw     edx, xmm2, 3         // get x1 integer. preroll

    // 2 Pixel loop.
    align      16
  xloop2:
    movdqa     xmm1, xmm2           // x0, x1 fractions.
    paddd      xmm2, xmm3           // x += dx
    movq       xmm0, qword ptr [esi + eax * 4]  // 2 source x0 pixels
    psrlw      xmm1, 9              // 7 bit fractions.
    movhps     xmm0, qword ptr [esi + edx * 4]  // 2 source x1 pixels
    pshufb     xmm1, xmm5           // 0000000011111111
    pshufb     xmm0, xmm4           // arrange pixels into pairs
    pxor       xmm1, xmm6           // 0..7f and 7f..0
    pmaddubsw  xmm0, xmm1           // argb_argb 16 bit, 2 pixels.
    psrlw      xmm0, 7              // argb 8.7 fixed point to low 8 bits.
    pextrw     eax, xmm2, 1         // get x0 integer. next iteration.
    pextrw     edx, xmm2, 3         // get x1 integer. next iteration.
    packuswb   xmm0, xmm0           // argb_argb 8 bits, 2 pixels.
    movq       qword ptr [edi], xmm0
    lea        edi, [edi + 8]
    sub        ecx, 2               // 2 pixels
    jge        xloop2

    align      16
 xloop29:

    add        ecx, 2 - 1
    jl         xloop99

    // 1 pixel remainder
    psrlw      xmm2, 9              // 7 bit fractions.
    movq       xmm0, qword ptr [esi + eax * 4]  // 2 source x0 pixels
    pshufb     xmm2, xmm5           // 00000000
    pshufb     xmm0, xmm4           // arrange pixels into pairs
    pxor       xmm2, xmm6           // 0..7f and 7f..0
    pmaddubsw  xmm0, xmm2           // argb 16 bit, 1 pixel.
    psrlw      xmm0, 7
    packuswb   xmm0, xmm0           // argb 8 bits, 1 pixel.
    movd       [edi], xmm0

    align      16
 xloop99:

    pop        edi
    pop        esi
    ret
  }
}

#elif !defined(LIBYUV_DISABLE_X86) && \
    (defined(__x86_64__) || defined(__i386__))

// TODO(nfullagar): For Native Client: When new toolchain becomes available,
// take advantage of bundle lock / unlock feature. This will reduce the amount
// of manual bundle alignment done below, and bundle alignment could even be
// moved into each macro that doesn't use %%nacl: such as MEMOPREG.

#if defined(__native_client__) && defined(__x86_64__)
#define MEMACCESS(base) "%%nacl:(%%r15,%q" #base ")"
#define MEMACCESS2(offset, base) "%%nacl:" #offset "(%%r15,%q" #base ")"
#define MEMLEA(offset, base) #offset "(%q" #base ")"
#define MEMLEA3(offset, index, scale) \
    #offset "(,%q" #index "," #scale ")"
#define MEMLEA4(offset, base, index, scale) \
    #offset "(%q" #base ",%q" #index "," #scale ")"
#define MEMOPREG(opcode, offset, base, index, scale, reg) \
    "lea " #offset "(%q" #base ",%q" #index "," #scale "),%%r14d\n" \
    #opcode " (%%r15,%%r14),%%" #reg "\n"
#define MEMOPMEM(opcode, reg, offset, base, index, scale) \
    "lea " #offset "(%q" #base ",%q" #index "," #scale "),%%r14d\n" \
    #opcode " %%" #reg ",(%%r15,%%r14)\n"
#define BUNDLEALIGN ".p2align 5 \n"
#else
#define MEMACCESS(base) "(%" #base ")"
#define MEMACCESS2(offset, base) #offset "(%" #base ")"
#define MEMLEA(offset, base) #offset "(%" #base ")"
#define MEMLEA3(offset, index, scale) \
    #offset "(,%" #index "," #scale ")"
#define MEMLEA4(offset, base, index, scale) \
    #offset "(%" #base ",%" #index "," #scale ")"
#define MEMOPREG(opcode, offset, base, index, scale, reg) \
    #opcode " " #offset "(%" #base ",%" #index "," #scale "),%%" #reg "\n"
#define MEMOPMEM(opcode, reg, offset, base, index, scale) \
    #opcode " %%" #reg ","#offset "(%" #base ",%" #index "," #scale ")\n"
#define BUNDLEALIGN
#endif

// GCC versions of row functions are verbatim conversions from Visual C,
// with some additional macro injection for Native Client (see row_posix.cc
// for more details.)
// Generated using gcc disassembly on Visual C object file:
//   objdump -D yuvscaler.obj >yuvscaler.txt
#define HAS_SCALEARGBROWDOWN2_SSE2
static void ScaleARGBRowDown2_SSE2(const uint8* src_argb,
                                   ptrdiff_t /* src_stride */,
                                   uint8* dst_argb, int dst_width) {
  asm volatile (
    ".p2align  4                               \n"
    BUNDLEALIGN
  "1:                                          \n"
    "movdqa    "MEMACCESS(0)",%%xmm0           \n"
    "movdqa    "MEMACCESS2(0x10,0)",%%xmm1     \n"
    "lea       "MEMLEA(0x20,0)",%0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm0             \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,"MEMACCESS(1)"           \n"
    "lea       "MEMLEA(0x10,1)",%1             \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  // %0
    "+r"(dst_argb),  // %1
    "+r"(dst_width)  // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

static void ScaleARGBRowDown2Box_SSE2(const uint8* src_argb,
                                      ptrdiff_t src_stride,
                                      uint8* dst_argb, int dst_width) {
  asm volatile (
    ".p2align  4                               \n"
    BUNDLEALIGN
  "1:                                          \n"
    "movdqa    "MEMACCESS(0)",%%xmm0           \n"
    "movdqa    "MEMACCESS2(0x10,0)",%%xmm1     \n"
    BUNDLEALIGN
    MEMOPREG(movdqa,0x00,0,3,1,xmm2)           //  movdqa   (%0,%3,1),%%xmm2
    MEMOPREG(movdqa,0x10,0,3,1,xmm3)           //  movdqa   0x10(%0,%3,1),%%xmm3
    "lea       "MEMLEA(0x20,0)",%0             \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm2             \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,"MEMACCESS(1)"           \n"
    "lea       "MEMLEA(0x10,1)",%1             \n"
    "jg        1b                              \n"
  : "+r"(src_argb),   // %0
    "+r"(dst_argb),   // %1
    "+r"(dst_width)   // %2
  : "r"(static_cast<intptr_t>(src_stride))   // %3
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
  );
}

#define HAS_SCALEARGBROWDOWNEVEN_SSE2
// Reads 4 pixels at a time.
// Alignment requirement: dst_argb 16 byte aligned.
void ScaleARGBRowDownEven_SSE2(const uint8* src_argb, ptrdiff_t src_stride,
                               int src_stepx,
                               uint8* dst_argb, int dst_width) {
  intptr_t src_stepx_x4 = static_cast<intptr_t>(src_stepx);
  intptr_t src_stepx_x12 = 0;
  asm volatile (
    "lea       "MEMLEA3(0x00,1,4)",%1          \n"
    "lea       "MEMLEA4(0x00,1,1,2)",%4        \n"
    ".p2align  4                               \n"
    BUNDLEALIGN
  "1:                                          \n"
    "movd      "MEMACCESS(0)",%%xmm0           \n"
    MEMOPREG(movd,0x00,0,1,1,xmm1)             //  movd      (%0,%1,1),%%xmm1
    "punpckldq %%xmm1,%%xmm0                   \n"
    BUNDLEALIGN
    MEMOPREG(movd,0x00,0,1,2,xmm2)             //  movd      (%0,%1,2),%%xmm2
    MEMOPREG(movd,0x00,0,4,1,xmm3)             //  movd      (%0,%4,1),%%xmm3
    "lea       "MEMLEA4(0x00,0,1,4)",%0        \n"
    "punpckldq %%xmm3,%%xmm2                   \n"
    "punpcklqdq %%xmm2,%%xmm0                  \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,"MEMACCESS(2)"           \n"
    "lea       "MEMLEA(0x10,2)",%2             \n"
    "jg        1b                              \n"
  : "+r"(src_argb),      // %0
    "+r"(src_stepx_x4),  // %1
    "+r"(dst_argb),      // %2
    "+r"(dst_width),     // %3
    "+r"(src_stepx_x12)  // %4
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
  );
}

// Blends four 2x2 to 4x1.
// Alignment requirement: dst_argb 16 byte aligned.
static void ScaleARGBRowDownEvenBox_SSE2(const uint8* src_argb,
                                         ptrdiff_t src_stride, int src_stepx,
                                         uint8* dst_argb, int dst_width) {
  intptr_t src_stepx_x4 = static_cast<intptr_t>(src_stepx);
  intptr_t src_stepx_x12 = 0;
  intptr_t row1 = static_cast<intptr_t>(src_stride);
  asm volatile (
    "lea       "MEMLEA3(0x00,1,4)",%1          \n"
    "lea       "MEMLEA4(0x00,1,1,2)",%4        \n"
    "lea       "MEMLEA4(0x00,0,5,1)",%5        \n"
    ".p2align  4                               \n"
    BUNDLEALIGN
  "1:                                          \n"
    "movq      "MEMACCESS(0)",%%xmm0           \n"
    MEMOPREG(movhps,0x00,0,1,1,xmm0)           //  movhps    (%0,%1,1),%%xmm0
    MEMOPREG(movq,0x00,0,1,2,xmm1)             //  movq      (%0,%1,2),%%xmm1
    BUNDLEALIGN
    MEMOPREG(movhps,0x00,0,4,1,xmm1)           //  movhps    (%0,%4,1),%%xmm1
    "lea       "MEMLEA4(0x00,0,1,4)",%0        \n"
    "movq      "MEMACCESS(5)",%%xmm2           \n"
    BUNDLEALIGN
    MEMOPREG(movhps,0x00,5,1,1,xmm2)           //  movhps    (%5,%1,1),%%xmm2
    MEMOPREG(movq,0x00,5,1,2,xmm3)             //  movq      (%5,%1,2),%%xmm3
    MEMOPREG(movhps,0x00,5,4,1,xmm3)           //  movhps    (%5,%4,1),%%xmm3
    "lea       "MEMLEA4(0x00,5,1,4)",%5        \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm2             \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,"MEMACCESS(2)"           \n"
    "lea       "MEMLEA(0x10,2)",%2             \n"
    "jg        1b                              \n"
  : "+r"(src_argb),       // %0
    "+r"(src_stepx_x4),   // %1
    "+r"(dst_argb),       // %2
    "+rm"(dst_width),     // %3
    "+r"(src_stepx_x12),  // %4
    "+r"(row1)            // %5
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
  );
}

#define HAS_SCALEARGBCOLS_SSE2
// TODO(fbarchard): p2align 5 is for nacl branch targets.  Reduce using
// pseudoop, bundle or macro.
static void ScaleARGBCols_SSE2(uint8* dst_argb, const uint8* src_argb,
                               int dst_width, int x, int dx) {
  intptr_t x0 = 0, x1 = 0;
  asm volatile (
    "movd      %5,%%xmm2                       \n"
    "movd      %6,%%xmm3                       \n"
    "pextrw    $0x1,%%xmm2,%k3                 \n"
    "sub       $0x2,%2                         \n"
    "jl        29f                             \n"
    "movdqa    %%xmm2,%%xmm0                   \n"
    "paddd     %%xmm3,%%xmm0                   \n"
    "punpckldq %%xmm0,%%xmm2                   \n"
    "punpckldq %%xmm3,%%xmm3                   \n"
    "paddd     %%xmm3,%%xmm3                   \n"
    "pextrw    $0x3,%%xmm2,%k4                 \n"

    ".p2align  5                               \n"
    BUNDLEALIGN
  "2:                                          \n"
    "paddd     %%xmm3,%%xmm2                   \n"
    MEMOPREG(movd,0x00,1,3,4,xmm0)             //  movd      (%1,%3,4),%%xmm0
    MEMOPREG(movd,0x00,1,4,4,xmm1)             //  movd      (%1,%4,4),%%xmm1
    "punpckldq %%xmm1,%%xmm0                   \n"
    "pextrw    $0x1,%%xmm2,%k3                 \n"
    "pextrw    $0x3,%%xmm2,%k4                 \n"
    "movq      %%xmm0,"MEMACCESS(0)"           \n"
    "lea       "MEMLEA(0x8,0)",%0              \n"
    "sub       $0x2,%2                         \n"
    "jge       2b                              \n"

    ".p2align  5                               \n"
  "29:                                         \n"
    "add       $0x1,%2                         \n"
    "jl        99f                             \n"
    BUNDLEALIGN
    MEMOPREG(movd,0x00,1,3,4,xmm0)             //  movd      (%1,%3,4),%%xmm0
    "movd      %%xmm0,"MEMACCESS(0)"           \n"

    ".p2align  5                               \n"
  "99:                                         \n"
  : "+r"(dst_argb),    // %0
    "+r"(src_argb),    // %1
    "+rm"(dst_width),  // %2
    "+r"(x0),          // %3
    "+r"(x1)           // %4
  : "rm"(x),           // %5
    "rm"(dx)           // %6
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
  );
}

// Shuffle table for arranging 2 pixels into pairs for pmaddubsw
static uvec8 kShuffleColARGB = {
  0u, 4u, 1u, 5u, 2u, 6u, 3u, 7u,  // bbggrraa 1st pixel
  8u, 12u, 9u, 13u, 10u, 14u, 11u, 15u  // bbggrraa 2nd pixel
};

// Shuffle table for duplicating 2 fractions into 8 bytes each
static uvec8 kShuffleFractions = {
  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u,
};

// Bilinear row filtering combines 4x2 -> 4x1. SSSE3 version
#define HAS_SCALEARGBFILTERCOLS_SSSE3
static void ScaleARGBFilterCols_SSSE3(uint8* dst_argb, const uint8* src_argb,
                                      int dst_width, int x, int dx) {
  intptr_t x0 = 0, x1 = 0;
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm5                       \n"
  :
  : "m"(kShuffleColARGB),  // %0
    "m"(kShuffleFractions)  // %1
  );

  asm volatile (
    "movd      %5,%%xmm2                       \n"
    "movd      %6,%%xmm3                       \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrlw     $0x9,%%xmm6                     \n"
    "pextrw    $0x1,%%xmm2,%k3                 \n"
    "sub       $0x2,%2                         \n"
    "jl        29f                             \n"
    "movdqa    %%xmm2,%%xmm0                   \n"
    "paddd     %%xmm3,%%xmm0                   \n"
    "punpckldq %%xmm0,%%xmm2                   \n"
    "punpckldq %%xmm3,%%xmm3                   \n"
    "paddd     %%xmm3,%%xmm3                   \n"
    "pextrw    $0x3,%%xmm2,%k4                 \n"

    ".p2align  4                               \n"
    BUNDLEALIGN
  "2:                                          \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "paddd     %%xmm3,%%xmm2                   \n"
    MEMOPREG(movq,0x00,1,3,4,xmm0)             //  movq      (%1,%3,4),%%xmm0
    "psrlw     $0x9,%%xmm1                     \n"
    BUNDLEALIGN
    MEMOPREG(movhps,0x00,1,4,4,xmm0)           //  movhps    (%1,%4,4),%%xmm0
    "pshufb    %%xmm5,%%xmm1                   \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "pxor      %%xmm6,%%xmm1                   \n"
    "pmaddubsw %%xmm1,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "pextrw    $0x1,%%xmm2,%k3                 \n"
    "pextrw    $0x3,%%xmm2,%k4                 \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movq      %%xmm0,"MEMACCESS(0)"           \n"
    "lea       "MEMLEA(0x8,0)",%0              \n"
    "sub       $0x2,%2                         \n"
    "jge       2b                              \n"

    ".p2align  4                               \n"
    BUNDLEALIGN
  "29:                                         \n"
    "add       $0x1,%2                         \n"
    "jl        99f                             \n"
    "psrlw     $0x9,%%xmm2                     \n"
    BUNDLEALIGN
    MEMOPREG(movq,0x00,1,3,4,xmm0)             //  movq      (%1,%3,4),%%xmm0
    "pshufb    %%xmm5,%%xmm2                   \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "pxor      %%xmm6,%%xmm2                   \n"
    "pmaddubsw %%xmm2,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movd      %%xmm0,"MEMACCESS(0)"           \n"

    ".p2align  4                               \n"
  "99:                                         \n"
  : "+r"(dst_argb),    // %0
    "+r"(src_argb),    // %1
    "+rm"(dst_width),  // %2
    "+r"(x0),          // %3
    "+r"(x1)           // %4
  : "rm"(x),           // %5
    "rm"(dx)           // %6
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}
#endif  // defined(__x86_64__) || defined(__i386__)

static void ScaleARGBRowDown2_C(const uint8* src_argb,
                                ptrdiff_t /* src_stride */,
                                uint8* dst_argb, int dst_width) {
  const uint32* src = reinterpret_cast<const uint32*>(src_argb);
  uint32* dst = reinterpret_cast<uint32*>(dst_argb);

  for (int x = 0; x < dst_width - 1; x += 2) {
    dst[0] = src[1];
    dst[1] = src[3];
    src += 4;
    dst += 2;
  }
  if (dst_width & 1) {
    dst[0] = src[1];
  }
}

static void ScaleARGBRowDown2Box_C(const uint8* src_argb, ptrdiff_t src_stride,
                                   uint8* dst_argb, int dst_width) {
  for (int x = 0; x < dst_width; ++x) {
    dst_argb[0] = (src_argb[0] + src_argb[4] +
                  src_argb[src_stride] + src_argb[src_stride + 4] + 2) >> 2;
    dst_argb[1] = (src_argb[1] + src_argb[5] +
                  src_argb[src_stride + 1] + src_argb[src_stride + 5] + 2) >> 2;
    dst_argb[2] = (src_argb[2] + src_argb[6] +
                  src_argb[src_stride + 2] + src_argb[src_stride + 6] + 2) >> 2;
    dst_argb[3] = (src_argb[3] + src_argb[7] +
                  src_argb[src_stride + 3] + src_argb[src_stride + 7] + 2) >> 2;
    src_argb += 8;
    dst_argb += 4;
  }
}

void ScaleARGBRowDownEven_C(const uint8* src_argb, ptrdiff_t /* src_stride */,
                            int src_stepx,
                            uint8* dst_argb, int dst_width) {
  const uint32* src = reinterpret_cast<const uint32*>(src_argb);
  uint32* dst = reinterpret_cast<uint32*>(dst_argb);

  for (int x = 0; x < dst_width - 1; x += 2) {
    dst[0] = src[0];
    dst[1] = src[src_stepx];
    src += src_stepx * 2;
    dst += 2;
  }
  if (dst_width & 1) {
    dst[0] = src[0];
  }
}

static void ScaleARGBRowDownEvenBox_C(const uint8* src_argb,
                                      ptrdiff_t src_stride,
                                      int src_stepx,
                                      uint8* dst_argb, int dst_width) {
  for (int x = 0; x < dst_width; ++x) {
    dst_argb[0] = (src_argb[0] + src_argb[4] +
                  src_argb[src_stride] + src_argb[src_stride + 4] + 2) >> 2;
    dst_argb[1] = (src_argb[1] + src_argb[5] +
                  src_argb[src_stride + 1] + src_argb[src_stride + 5] + 2) >> 2;
    dst_argb[2] = (src_argb[2] + src_argb[6] +
                  src_argb[src_stride + 2] + src_argb[src_stride + 6] + 2) >> 2;
    dst_argb[3] = (src_argb[3] + src_argb[7] +
                  src_argb[src_stride + 3] + src_argb[src_stride + 7] + 2) >> 2;
    src_argb += src_stepx * 4;
    dst_argb += 4;
  }
}

// Mimics SSSE3 blender
#define BLENDER1(a, b, f) ((a) * (0x7f ^ f) + (b) * f) >> 7
#define BLENDERC(a, b, f, s) static_cast<uint32>( \
    BLENDER1(((a) >> s) & 255, ((b) >> s) & 255, f) << s)
#define BLENDER(a, b, f) \
    BLENDERC(a, b, f, 24) | BLENDERC(a, b, f, 16) | \
    BLENDERC(a, b, f, 8) | BLENDERC(a, b, f, 0)

static void ScaleARGBFilterCols_C(uint8* dst_argb, const uint8* src_argb,
                                  int dst_width, int x, int dx) {
  const uint32* src = reinterpret_cast<const uint32*>(src_argb);
  uint32* dst = reinterpret_cast<uint32*>(dst_argb);
  for (int j = 0; j < dst_width - 1; j += 2) {
    int xi = x >> 16;
    int xf = (x >> 9) & 0x7f;
    uint32 a = src[xi];
    uint32 b = src[xi + 1];
    dst[0] = BLENDER(a, b, xf);
    x += dx;
    xi = x >> 16;
    xf = (x >> 9) & 0x7f;
    a = src[xi];
    b = src[xi + 1];
    dst[1] = BLENDER(a, b, xf);
    x += dx;
    dst += 2;
  }
  if (dst_width & 1) {
    int xi = x >> 16;
    int xf = (x >> 9) & 0x7f;
    uint32 a = src[xi];
    uint32 b = src[xi + 1];
    dst[0] = BLENDER(a, b, xf);
  }
}

// ScaleARGB ARGB, 1/2
// This is an optimized version for scaling down a ARGB to 1/2 of
// its original size.

static void ScaleARGBDown2(int /* src_width */, int /* src_height */,
                           int dst_width, int dst_height,
                           int src_stride, int dst_stride,
                           const uint8* src_argb, uint8* dst_argb,
                           int x, int dx, int y, int dy,
                           FilterMode filtering) {
  assert(dx == 65536 * 2);  // Test scale factor of 2.
  assert((dy & 0x1ffff) == 0);  // Test vertical scale is multiple of 2.
  // Advance to odd row, even column.
  if (filtering) {
    src_argb += (y >> 16) * src_stride + (x >> 16) * 4;
  } else {
    src_argb += (y >> 16) * src_stride + ((x >> 16) - 1) * 4;
  }
  int row_stride = src_stride * (dy >> 16);
  void (*ScaleARGBRowDown2)(const uint8* src_argb, ptrdiff_t src_stride,
                            uint8* dst_argb, int dst_width) =
      filtering ? ScaleARGBRowDown2Box_C : ScaleARGBRowDown2_C;
#if defined(HAS_SCALEARGBROWDOWN2_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(dst_width, 4) &&
      IS_ALIGNED(src_argb, 16) && IS_ALIGNED(row_stride, 16) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride, 16)) {
    ScaleARGBRowDown2 = filtering ? ScaleARGBRowDown2Box_SSE2 :
        ScaleARGBRowDown2_SSE2;
  }
#elif defined(HAS_SCALEARGBROWDOWN2_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(dst_width, 8) &&
      IS_ALIGNED(src_argb, 4) && IS_ALIGNED(row_stride, 4)) {
    ScaleARGBRowDown2 = filtering ? ScaleARGBRowDown2Box_NEON :
        ScaleARGBRowDown2_NEON;
  }
#endif

  // TODO(fbarchard): Loop through source height to allow odd height.
  for (int y = 0; y < dst_height; ++y) {
    ScaleARGBRowDown2(src_argb, src_stride, dst_argb, dst_width);
    src_argb += row_stride;
    dst_argb += dst_stride;
  }
}

// ScaleARGB ARGB Even
// This is an optimized version for scaling down a ARGB to even
// multiple of its original size.
static void ScaleARGBDownEven(int src_width, int src_height,
                              int dst_width, int dst_height,
                              int src_stride, int dst_stride,
                              const uint8* src_argb, uint8* dst_argb,
                              int x, int dx, int y, int dy,
                              FilterMode filtering) {
  assert(IS_ALIGNED(src_width, 2));
  assert(IS_ALIGNED(src_height, 2));
  int col_step = dx >> 16;
  int row_stride = (dy >> 16) * src_stride;
  src_argb += (y >> 16) * src_stride + (x >> 16) * 4;
  void (*ScaleARGBRowDownEven)(const uint8* src_argb, ptrdiff_t src_stride,
                               int src_step, uint8* dst_argb, int dst_width) =
      filtering ? ScaleARGBRowDownEvenBox_C : ScaleARGBRowDownEven_C;
#if defined(HAS_SCALEARGBROWDOWNEVEN_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && IS_ALIGNED(dst_width, 4) &&
      IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride, 16)) {
    ScaleARGBRowDownEven = filtering ? ScaleARGBRowDownEvenBox_SSE2 :
        ScaleARGBRowDownEven_SSE2;
  }
#elif defined(HAS_SCALEARGBROWDOWNEVEN_NEON)
  if (TestCpuFlag(kCpuHasNEON) && IS_ALIGNED(dst_width, 4) &&
      IS_ALIGNED(src_argb, 4)) {
    ScaleARGBRowDownEven = filtering ? ScaleARGBRowDownEvenBox_NEON :
        ScaleARGBRowDownEven_NEON;
  }
#endif

  for (int y = 0; y < dst_height; ++y) {
    ScaleARGBRowDownEven(src_argb, src_stride, col_step, dst_argb, dst_width);
    src_argb += row_stride;
    dst_argb += dst_stride;
  }
}

// Scale ARGB down with bilinear interpolation.
static void ScaleARGBBilinearDown(int src_height,
                                  int dst_width, int dst_height,
                                  int src_stride, int dst_stride,
                                  const uint8* src_argb, uint8* dst_argb,
                                  int x, int dx, int y, int dy) {
  assert(src_height > 0);
  assert(dst_width > 0);
  assert(dst_height > 0);
  int xlast = x + (dst_width - 1) * dx;
  int xl = (dx >= 0) ? x : xlast;
  int xr = (dx >= 0) ? xlast : x;
  xl = (xl >> 16) & ~3;  // Left edge aligned.
  xr = (xr >> 16) + 1;  // Right most pixel used.
  int clip_src_width = (((xr - xl) + 1 + 3) & ~3) * 4;  // Width aligned to 4.
  src_argb += xl * 4;
  x -= (xl << 16);
  assert(clip_src_width <= kMaxStride);
  // TODO(fbarchard): Remove clip_src_width alignment checks.
  SIMD_ALIGNED(uint8 row[kMaxStride + 16]);
  void (*InterpolateRow)(uint8* dst_argb, const uint8* src_argb,
      ptrdiff_t src_stride, int dst_width, int source_y_fraction) =
      InterpolateRow_C;
#if defined(HAS_INTERPOLATEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && clip_src_width >= 16) {
    InterpolateRow = InterpolateRow_Any_SSE2;
    if (IS_ALIGNED(clip_src_width, 16)) {
      InterpolateRow = InterpolateRow_Unaligned_SSE2;
      if (IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride, 16)) {
        InterpolateRow = InterpolateRow_SSE2;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && clip_src_width >= 16) {
    InterpolateRow = InterpolateRow_Any_SSSE3;
    if (IS_ALIGNED(clip_src_width, 16)) {
      InterpolateRow = InterpolateRow_Unaligned_SSSE3;
      if (IS_ALIGNED(src_argb, 16) && IS_ALIGNED(src_stride, 16)) {
        InterpolateRow = InterpolateRow_SSSE3;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && clip_src_width >= 16) {
    InterpolateRow = InterpolateRow_Any_NEON;
    if (IS_ALIGNED(clip_src_width, 16)) {
      InterpolateRow = InterpolateRow_NEON;
    }
  }
#endif
#if defined(HAS_INTERPOLATEROWS_MIPS_DSPR2)
  if (TestCpuFlag(kCpuHasMIPS_DSPR2) && clip_src_width >= 4 &&
      IS_ALIGNED(src_argb, 4) && IS_ALIGNED(src_stride, 4)) {
    InterpolateRow = InterpolateRow_Any_MIPS_DSPR2;
    if (IS_ALIGNED(clip_src_width, 4)) {
      InterpolateRow = InterpolateRow_MIPS_DSPR2;
    }
  }
#endif
  void (*ScaleARGBFilterCols)(uint8* dst_argb, const uint8* src_argb,
      int dst_width, int x, int dx) = ScaleARGBFilterCols_C;
#if defined(HAS_SCALEARGBFILTERCOLS_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    ScaleARGBFilterCols = ScaleARGBFilterCols_SSSE3;
  }
#endif
  const int max_y = (src_height > 1) ? ((src_height - 1) << 16) - 1 : 0;
  for (int j = 0; j < dst_height; ++j) {
    if (y > max_y) {
      y = max_y;
    }
    int yi = y >> 16;
    int yf = (y >> 8) & 255;
    const uint8* src = src_argb + yi * src_stride;
    InterpolateRow(row, src, src_stride, clip_src_width, yf);
    ScaleARGBFilterCols(dst_argb, row, dst_width, x, dx);
    dst_argb += dst_stride;
    y += dy;
  }
}

// Scale ARGB up with bilinear interpolation.
static void ScaleARGBBilinearUp(int src_width, int src_height,
                                int dst_width, int dst_height,
                                int src_stride, int dst_stride,
                                const uint8* src_argb, uint8* dst_argb,
                                int x, int dx, int y, int dy) {
  assert(src_width > 0);
  assert(src_height > 0);
  assert(dst_width > 0);
  assert(dst_height > 0);
  assert(dst_width * 4 <= kMaxStride);
  void (*InterpolateRow)(uint8* dst_argb, const uint8* src_argb,
      ptrdiff_t src_stride, int dst_width, int source_y_fraction) =
      InterpolateRow_C;
#if defined(HAS_INTERPOLATEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && dst_width >= 4) {
    InterpolateRow = InterpolateRow_Any_SSE2;
    if (IS_ALIGNED(dst_width, 4)) {
      InterpolateRow = InterpolateRow_Unaligned_SSE2;
      if (IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride, 16)) {
        InterpolateRow = InterpolateRow_SSE2;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && dst_width >= 4) {
    InterpolateRow = InterpolateRow_Any_SSSE3;
    if (IS_ALIGNED(dst_width, 4)) {
      InterpolateRow = InterpolateRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride, 16)) {
        InterpolateRow = InterpolateRow_SSSE3;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && dst_width >= 4) {
    InterpolateRow = InterpolateRow_Any_NEON;
    if (IS_ALIGNED(dst_width, 4)) {
      InterpolateRow = InterpolateRow_NEON;
    }
  }
#endif
#if defined(HAS_INTERPOLATEROWS_MIPS_DSPR2)
  if (TestCpuFlag(kCpuHasMIPS_DSPR2) && dst_width >= 1 &&
      IS_ALIGNED(dst_argb, 4) && IS_ALIGNED(dst_stride, 4)) {
    InterpolateRow = InterpolateRow_MIPS_DSPR2;
  }
#endif
  void (*ScaleARGBFilterCols)(uint8* dst_argb, const uint8* src_argb,
      int dst_width, int x, int dx) = ScaleARGBFilterCols_C;
#if defined(HAS_SCALEARGBFILTERCOLS_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    ScaleARGBFilterCols = ScaleARGBFilterCols_SSSE3;
  }
#endif
  const int max_y = (src_height > 1) ? ((src_height - 1) << 16) - 1 : 0;
  if (y > max_y) {
    y = max_y;
  }
  int yi = y >> 16;
  const uint8* src = src_argb + yi * src_stride;
  SIMD_ALIGNED(uint8 row[2 * kMaxStride]);
  uint8* rowptr = row;
  int rowstride = kMaxStride;
  int lasty = yi;

  ScaleARGBFilterCols(rowptr, src, dst_width, x, dx);
  if (src_height > 1) {
    src += src_stride;
  }
  ScaleARGBFilterCols(rowptr + rowstride, src, dst_width, x, dx);
  src += src_stride;

  for (int j = 0; j < dst_height; ++j) {
    yi = y >> 16;
    if (yi != lasty) {
      if (y <= max_y) {
        ScaleARGBFilterCols(rowptr, src, dst_width, x, dx);
        rowptr += rowstride;
        rowstride = -rowstride;
        lasty = yi;
        src += src_stride;
      }
    }
    int yf = (y >> 8) & 255;
    InterpolateRow(dst_argb, rowptr, rowstride, dst_width * 4, yf);
    dst_argb += dst_stride;
    y += dy;
  }
}

#ifdef YUVSCALEUP
// Scale YUV to ARGB up with bilinear interpolation.
static void ScaleYUVToARGBBilinearUp(int src_width, int src_height,
                                     int dst_width, int dst_height,
                                     int src_stride_y,
                                     int src_stride_u,
                                     int src_stride_v,
                                     int dst_stride_argb,
                                     const uint8* src_y,
                                     const uint8* src_u,
                                     const uint8* src_v,
                                     uint8* dst_argb,
                                     int x, int dx, int y, int dy) {
  assert(src_width > 0);
  assert(src_height > 0);
  assert(dst_width > 0);
  assert(dst_height > 0);
  assert(dst_width * 4 <= kMaxStride);

  void (*I422ToARGBRow)(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) = I422ToARGBRow_C;
#if defined(HAS_I422TOARGBROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && src_width >= 8) {
    I422ToARGBRow = I422ToARGBRow_Any_SSSE3;
    if (IS_ALIGNED(src_width, 8)) {
      I422ToARGBRow = I422ToARGBRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
        I422ToARGBRow = I422ToARGBRow_SSSE3;
      }
    }
  }
#endif
#if defined(HAS_I422TOARGBROW_AVX2)
  if (TestCpuFlag(kCpuHasAVX2) && src_width >= 16) {
    I422ToARGBRow = I422ToARGBRow_Any_AVX2;
    if (IS_ALIGNED(src_width, 16)) {
      I422ToARGBRow = I422ToARGBRow_AVX2;
    }
  }
#endif
#if defined(HAS_I422TOARGBROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && src_width >= 8) {
    I422ToARGBRow = I422ToARGBRow_Any_NEON;
    if (IS_ALIGNED(src_width, 8)) {
      I422ToARGBRow = I422ToARGBRow_NEON;
    }
  }
#endif
#if defined(HAS_I422TOARGBROW_MIPS_DSPR2)
  if (TestCpuFlag(kCpuHasMIPS_DSPR2) && IS_ALIGNED(src_width, 4) &&
      IS_ALIGNED(src_y, 4) && IS_ALIGNED(src_stride_y, 4) &&
      IS_ALIGNED(src_u, 2) && IS_ALIGNED(src_stride_u, 2) &&
      IS_ALIGNED(src_v, 2) && IS_ALIGNED(src_stride_v, 2) &&
      IS_ALIGNED(dst_argb, 4) && IS_ALIGNED(dst_stride_argb, 4)) {
    I422ToARGBRow = I422ToARGBRow_MIPS_DSPR2;
  }
#endif

  void (*InterpolateRow)(uint8* dst_argb, const uint8* src_argb,
      ptrdiff_t src_stride, int dst_width, int source_y_fraction) =
      InterpolateRow_C;
#if defined(HAS_INTERPOLATEROW_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) && dst_width >= 4) {
    InterpolateRow = InterpolateRow_Any_SSE2;
    if (IS_ALIGNED(dst_width, 4)) {
      InterpolateRow = InterpolateRow_Unaligned_SSE2;
      if (IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
        InterpolateRow = InterpolateRow_SSE2;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3) && dst_width >= 4) {
    InterpolateRow = InterpolateRow_Any_SSSE3;
    if (IS_ALIGNED(dst_width, 4)) {
      InterpolateRow = InterpolateRow_Unaligned_SSSE3;
      if (IS_ALIGNED(dst_argb, 16) && IS_ALIGNED(dst_stride_argb, 16)) {
        InterpolateRow = InterpolateRow_SSSE3;
      }
    }
  }
#endif
#if defined(HAS_INTERPOLATEROW_NEON)
  if (TestCpuFlag(kCpuHasNEON) && dst_width >= 4) {
    InterpolateRow = InterpolateRow_Any_NEON;
    if (IS_ALIGNED(dst_width, 4)) {
      InterpolateRow = InterpolateRow_NEON;
    }
  }
#endif
#if defined(HAS_INTERPOLATEROWS_MIPS_DSPR2)
  if (TestCpuFlag(kCpuHasMIPS_DSPR2) && dst_width >= 1 &&
      IS_ALIGNED(dst_argb, 4) && IS_ALIGNED(dst_stride_argb, 4)) {
    InterpolateRow = InterpolateRow_MIPS_DSPR2;
  }
#endif
  void (*ScaleARGBFilterCols)(uint8* dst_argb, const uint8* src_argb,
      int dst_width, int x, int dx) = ScaleARGBFilterCols_C;
#if defined(HAS_SCALEARGBFILTERCOLS_SSSE3)
  if (TestCpuFlag(kCpuHasSSSE3)) {
    ScaleARGBFilterCols = ScaleARGBFilterCols_SSSE3;
  }
#endif
  const int max_y = (src_height > 1) ? ((src_height - 1) << 16) - 1 : 0;
  if (y > max_y) {
    y = max_y;
  }
  const int kYShift = 1;  // Shift Y by 1 to convert Y plane to UV coordinate.
  int yi = y >> 16;
  int uv_yi = yi >> kYShift;
  const uint8* src_row_y = src_y + yi * src_stride_y;
  const uint8* src_row_u = src_u + uv_yi * src_stride_u;
  const uint8* src_row_v = src_v + uv_yi * src_stride_v;
  SIMD_ALIGNED(uint8 row[2 * kMaxStride]);
  SIMD_ALIGNED(uint8 argb_row[kMaxStride * 4]);
  uint8* rowptr = row;
  int rowstride = kMaxStride;
  int lasty = yi;

  ScaleARGBFilterCols(rowptr, src_row_y, dst_width, x, dx);
  if (src_height > 1) {
    src_row_y += src_stride_y;
    if (yi & 1) {
      src_row_u += src_stride_u;
      src_row_v += src_stride_v;
    }
  }
  ScaleARGBFilterCols(rowptr + rowstride, src_row_y, dst_width, x, dx);
  if (src_height > 2) {
    src_row_y += src_stride_y;
    if (!(yi & 1)) {
      src_row_u += src_stride_u;
      src_row_v += src_stride_v;
    }
  }

  for (int j = 0; j < dst_height; ++j) {
    yi = y >> 16;
    if (yi != lasty) {
      if (y <= max_y) {
        // TODO(fbarchard): Convert the clipped region of row.
        I422ToARGBRow(src_row_y, src_row_u, src_row_v, argb_row, src_width);
        ScaleARGBFilterCols(rowptr, argb_row, dst_width, x, dx);
        rowptr += rowstride;
        rowstride = -rowstride;
        lasty = yi;
        src_row_y += src_stride_y;
        if (yi & 1) {
          src_row_u += src_stride_u;
          src_row_v += src_stride_v;
        }
      }
    }
    int yf = (y >> 8) & 255;
    InterpolateRow(dst_argb, rowptr, rowstride, dst_width * 4, yf);
    dst_argb += dst_stride_argb;
    y += dy;
  }
}
#endif

// Scales a single row of pixels using point sampling.
// Code is adapted from libyuv bilinear yuv scaling, but with bilinear
// interpolation off, and argb pixels instead of yuv.
static void ScaleARGBCols_C(uint8* dst_argb, const uint8* src_argb,
                            int dst_width, int x, int dx) {
  const uint32* src = reinterpret_cast<const uint32*>(src_argb);
  uint32* dst = reinterpret_cast<uint32*>(dst_argb);
  for (int j = 0; j < dst_width - 1; j += 2) {
    dst[0] = src[x >> 16];
    x += dx;
    dst[1] = src[x >> 16];
    x += dx;
    dst += 2;
  }
  if (dst_width & 1) {
    dst[0] = src[x >> 16];
  }
}

// ScaleARGB ARGB to/from any dimensions, without interpolation.
// Fixed point math is used for performance: The upper 16 bits
// of x and dx is the integer part of the source position and
// the lower 16 bits are the fixed decimal part.

static void ScaleARGBSimple(int src_width, int src_height,
                            int dst_width, int dst_height,
                            int src_stride, int dst_stride,
                            const uint8* src_argb, uint8* dst_argb,
                            int x, int dx, int y, int dy) {
  void (*ScaleARGBCols)(uint8* dst_argb, const uint8* src_argb,
      int dst_width, int x, int dx) = ScaleARGBCols_C;
#if defined(HAS_SCALEARGBCOLS_SSE2)
  if (TestCpuFlag(kCpuHasSSE2)) {
    ScaleARGBCols = ScaleARGBCols_SSE2;
  }
#endif

  for (int i = 0; i < dst_height; ++i) {
    ScaleARGBCols(dst_argb, src_argb + (y >> 16) * src_stride,
                  dst_width, x, dx);
    dst_argb += dst_stride;
    y += dy;
  }
}

// ScaleARGB ARGB to/from any dimensions.
static void ScaleARGBAnySize(int src_width, int src_height,
                             int dst_width, int dst_height,
                             int clip_width, int clip_height,
                             int src_stride, int dst_stride,
                             const uint8* src_argb, uint8* dst_argb,
                             int x, int dx, int y, int dy,
                             FilterMode filtering) {
  if (filtering && dy < 65536 && dst_width * 4 <= kMaxStride) {
    ScaleARGBBilinearUp(src_width, src_height,
                        clip_width, clip_height,
                        src_stride, dst_stride, src_argb, dst_argb,
                        x, dx, y, dy);
    return;
  }
  if (filtering && src_width * 4 < kMaxStride) {
    ScaleARGBBilinearDown(src_height,
                          clip_width, clip_height,
                          src_stride, dst_stride, src_argb, dst_argb,
                          x, dx, y, dy);
    return;
  }
  ScaleARGBSimple(src_width, src_height, clip_width, clip_height,
                  src_stride, dst_stride, src_argb, dst_argb,
                  x, dx, y, dy);
}

// ScaleARGB a ARGB.
// This function in turn calls a scaling function
// suitable for handling the desired resolutions.
static void ScaleARGB(const uint8* src, int src_stride,
                      int src_width, int src_height,
                      uint8* dst, int dst_stride,
                      int dst_width, int dst_height,
                      int clip_x, int clip_y, int clip_width, int clip_height,
                      FilterMode filtering) {
  // Negative src_height means invert the image.
  if (src_height < 0) {
    src_height = -src_height;
    src = src + (src_height - 1) * src_stride;
    src_stride = -src_stride;
  }
  // Initial source x/y coordinate and step values as 16.16 fixed point.
  int dx = 0;
  int dy = 0;
  int x = 0;
  int y = 0;
  if (filtering) {
    // Scale step for bilinear sampling renders last pixel once for upsample.
    if (dst_width <= Abs(src_width)) {
      dx = FixedDiv(Abs(src_width), dst_width);
      x = (dx >> 1) - 32768;
    } else if (dst_width > 1) {
      dx = FixedDiv(Abs(src_width) - 1, dst_width - 1);
    }
    if (dst_height <= src_height) {
      dy = FixedDiv(src_height,  dst_height);
      y = (dy >> 1) - 32768;
    } else if (dst_height > 1) {
      dy = FixedDiv(src_height - 1, dst_height - 1);
    }
  } else {
    // Scale step for point sampling duplicates all pixels equally.
    dx = FixedDiv(Abs(src_width), dst_width);
    dy = FixedDiv(src_height, dst_height);
    x = dx >> 1;
    y = dy >> 1;
  }
  // Negative src_width means horizontally mirror.
  if (src_width < 0) {
    x += (dst_width - 1) * dx;
    dx = -dx;
    src_width = -src_width;
  }
  if (clip_x) {
    x += clip_x * dx;
    dst += clip_x * 4;
  }
  if (clip_y) {
    y += clip_y * dy;
    dst += clip_y * dst_stride;
  }

  // Special case for integer step values.
  if (((dx | dy) & 0xffff) == 0) {
    if (!dx || !dy) {  // 1 pixel wide and/or tall.
      filtering = kFilterNone;
    } else {
      // Optimized even scale down. ie 2, 4, 6, 8, 10x.
      if (!(dx & 0x10000) && !(dy & 0x10000)) {
        if ((dx >> 16) == 2) {
          // Optimized 1/2 horizontal.
          ScaleARGBDown2(src_width, src_height,
                         clip_width, clip_height,
                         src_stride, dst_stride, src, dst,
                         x, dx, y, dy, filtering);
          return;
        }
        ScaleARGBDownEven(src_width, src_height,
                          clip_width, clip_height,
                          src_stride, dst_stride, src, dst,
                          x, dx, y, dy, filtering);
        return;
      }
      // Optimized odd scale down. ie 3, 5, 7, 9x.
      if ((dx & 0x10000) && (dy & 0x10000)) {
        filtering = kFilterNone;
        if (dx == 0x10000 && dy == 0x10000) {
          // Straight copy.
          ARGBCopy(src + (y >> 16) * src_stride + (x >> 16) * 4, src_stride,
                   dst, dst_stride, clip_width, clip_height);
          return;
        }
      }
    }
  }
  if (dx == 0x10000 && (x & 0xffff) == 0) {
    // Arbitrary scale vertically, but unscaled vertically.
    ScaleARGBBilinearVertical(src_height,
                              clip_width, clip_height,
                              src_stride, dst_stride, src, dst,
                              x, y, dy, 4, filtering);
    return;
  }

  // Arbitrary scale up and/or down.
  ScaleARGBAnySize(src_width, src_height,
                   dst_width, dst_height,
                   clip_width, clip_height,
                   src_stride, dst_stride, src, dst,
                   x, dx, y, dy, filtering);
}

LIBYUV_API
int ARGBScaleClip(const uint8* src_argb, int src_stride_argb,
                  int src_width, int src_height,
                  uint8* dst_argb, int dst_stride_argb,
                  int dst_width, int dst_height,
                  int clip_x, int clip_y, int clip_width, int clip_height,
                  enum FilterMode filtering) {
  if (!src_argb || src_width == 0 || src_height == 0 ||
      !dst_argb || dst_width <= 0 || dst_height <= 0 ||
      clip_x < 0 || clip_y < 0 ||
      (clip_x + clip_width) > dst_width ||
      (clip_y + clip_height) > dst_height) {
    return -1;
  }
  ScaleARGB(src_argb, src_stride_argb, src_width, src_height,
            dst_argb, dst_stride_argb, dst_width, dst_height,
            clip_x, clip_y, clip_width, clip_height, filtering);
  return 0;
}

// Scale an ARGB image.
LIBYUV_API
int ARGBScale(const uint8* src_argb, int src_stride_argb,
              int src_width, int src_height,
              uint8* dst_argb, int dst_stride_argb,
              int dst_width, int dst_height,
              FilterMode filtering) {
  if (!src_argb || src_width == 0 || src_height == 0 ||
      !dst_argb || dst_width <= 0 || dst_height <= 0) {
    return -1;
  }
  ScaleARGB(src_argb, src_stride_argb, src_width, src_height,
            dst_argb, dst_stride_argb, dst_width, dst_height,
            0, 0, dst_width, dst_height, filtering);
  return 0;
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/scale.h"

#include <assert.h>
#include <string.h>

#include "libyuv/cpu_id.h"

#if defined(_MSC_VER)
#define ALIGN16(var) __declspec(align(16)) var
#else
#define ALIGN16(var) var __attribute__((aligned(16)))
#endif

// Note: A Neon reference manual
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0204j/CJAJIIGG.html
// Note: Some SSE2 reference manuals
// cpuvol1.pdf agner_instruction_tables.pdf 253666.pdf 253667.pdf

namespace libyuv {

// Set the following flag to true to revert to only
// using the reference implementation ScalePlaneBox(), and
// NOT the optimized versions. Useful for debugging and
// when comparing the quality of the resulting YUV planes
// as produced by the optimized and non-optimized versions.

static bool use_reference_impl_ = false;

void SetUseReferenceImpl(bool use) {
  use_reference_impl_ = use;
}

/**
 * NEON downscalers with interpolation.
 *
 * Provided by Fritz Koenig
 *
 */

#if defined(__ARM_NEON__) && !defined(COVERAGE_ENABLED)
#define HAS_SCALEROWDOWN2_NEON
void ScaleRowDown2_NEON(const uint8* src_ptr, int /* src_stride */,
                        uint8* dst, int dst_width) {
  __asm__ volatile
  (
    "1:\n"
    "vld2.u8    {q0,q1}, [%0]!    \n"  // load even pixels into q0, odd into q1
    "vst1.u8    {q0}, [%1]!       \n"  // store even pixels
    "subs       %2, %2, #16       \n"  // 16 processed per loop
    "bhi        1b                \n"
    :                                    // Output registers
    : "r"(src_ptr), "r"(dst), "r"(dst_width)   // Input registers
    : "r4", "q0", "q1"                   // Clobber List
  );
}

void ScaleRowDown2Int_NEON(const uint8* src_ptr, int src_stride,
                           uint8* dst, int dst_width) {
  __asm__ volatile
  (
    "mov        r4, #2            \n"  // rounding constant
    "add        %1, %0            \n"  // l2
    "vdup.16    q4, r4            \n"
    "1:\n"
    "vld1.u8    {q0,q1}, [%0]!    \n"  // load l1 and post increment
    "vld1.u8    {q2,q3}, [%1]!    \n"  // load l2 and post increment
    "vpaddl.u8  q0, q0            \n"  // l1 add adjacent
    "vpaddl.u8  q1, q1            \n"
    "vpadal.u8  q0, q2            \n"  // l2 add adjacent and add l1 to l2
    "vpadal.u8  q1, q3            \n"
    "vadd.u16   q0, q4            \n"  // rounding
    "vadd.u16   q1, q4            \n"
    "vshrn.u16  d0, q0, #2        \n"  // downshift and pack
    "vshrn.u16  d1, q1, #2        \n"
    "vst1.u8    {q0}, [%2]!       \n"
    "subs       %3, %3, #16       \n"  // 16 processed per loop
    "bhi        1b                \n"
    :                                                 // Output registers
    : "r"(src_ptr), "r"(src_stride), "r"(dst), "r"(dst_width)  // Input registers
    : "r4", "q0", "q1", "q2", "q3", "q4"              // Clobber List
   );
}

/**
 * SSE2 downscalers with interpolation.
 *
 * Provided by Frank Barchard (fbarchard@google.com)
 *
 */

// Constants for SSE2 code
#elif (defined(WIN32) || defined(__i386__)) && !defined(COVERAGE_ENABLED) && \
    !defined(__PIC__) && !TARGET_IPHONE_SIMULATOR
#if defined(_MSC_VER)
#define TALIGN16(t, var) __declspec(align(16)) t _ ## var
#elif defined(OSX)
#define TALIGN16(t, var) t var __attribute__((aligned(16)))
#else
#define TALIGN16(t, var) t _ ## var __attribute__((aligned(16)))
#endif

// Offsets for source bytes 0 to 9
extern "C" TALIGN16(const uint8, shuf0[16]) =
  { 0, 1, 3, 4, 5, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128 };

// Offsets for source bytes 11 to 20 with 8 subtracted = 3 to 12.
extern "C" TALIGN16(const uint8, shuf1[16]) =
  { 3, 4, 5, 7, 8, 9, 11, 12, 128, 128, 128, 128, 128, 128, 128, 128 };

// Offsets for source bytes 21 to 31 with 16 subtracted = 5 to 31.
extern "C" TALIGN16(const uint8, shuf2[16]) =
  { 5, 7, 8, 9, 11, 12, 13, 15, 128, 128, 128, 128, 128, 128, 128, 128 };

// Offsets for source bytes 0 to 10
extern "C" TALIGN16(const uint8, shuf01[16]) =
  { 0, 1, 1, 2, 2, 3, 4, 5, 5, 6, 6, 7, 8, 9, 9, 10 };

// Offsets for source bytes 10 to 21 with 8 subtracted = 3 to 13.
extern "C" TALIGN16(const uint8, shuf11[16]) =
  { 2, 3, 4, 5, 5, 6, 6, 7, 8, 9, 9, 10, 10, 11, 12, 13 };

// Offsets for source bytes 21 to 31 with 16 subtracted = 5 to 31.
extern "C" TALIGN16(const uint8, shuf21[16]) =
  { 5, 6, 6, 7, 8, 9, 9, 10, 10, 11, 12, 13, 13, 14, 14, 15 };

// Coefficients for source bytes 0 to 10
extern "C" TALIGN16(const uint8, madd01[16]) =
  { 3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2 };

// Coefficients for source bytes 10 to 21
extern "C" TALIGN16(const uint8, madd11[16]) =
  { 1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1 };

// Coefficients for source bytes 21 to 31
extern "C" TALIGN16(const uint8, madd21[16]) =
  { 2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3 };

// Coefficients for source bytes 21 to 31
extern "C" TALIGN16(const int16, round34[8]) =
  { 2, 2, 2, 2, 2, 2, 2, 2 };

extern "C" TALIGN16(const uint8, shuf38a[16]) =
  { 0, 3, 6, 8, 11, 14, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 };

extern "C" TALIGN16(const uint8, shuf38b[16]) =
  { 128, 128, 128, 128, 128, 128, 0, 3, 6, 8, 11, 14, 128, 128, 128, 128 };

// Arrange words 0,3,6 into 0,1,2
extern "C" TALIGN16(const uint8, shufac0[16]) =
  { 0, 1, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 };

// Arrange words 0,3,6 into 3,4,5
extern "C" TALIGN16(const uint8, shufac3[16]) =
  { 128, 128, 128, 128, 128, 128, 0, 1, 6, 7, 12, 13, 128, 128, 128, 128 };

// Scaling values for boxes of 3x3 and 2x3
extern "C" TALIGN16(const uint16, scaleac3[8]) =
  { 65536 / 9, 65536 / 9, 65536 / 6, 65536 / 9, 65536 / 9, 65536 / 6, 0, 0 };

// Arrange first value for pixels 0,1,2,3,4,5
extern "C" TALIGN16(const uint8, shufab0[16]) =
  { 0, 128, 3, 128, 6, 128, 8, 128, 11, 128, 14, 128, 128, 128, 128, 128 };

// Arrange second value for pixels 0,1,2,3,4,5
extern "C" TALIGN16(const uint8, shufab1[16]) =
  { 1, 128, 4, 128, 7, 128, 9, 128, 12, 128, 15, 128, 128, 128, 128, 128 };

// Arrange third value for pixels 0,1,2,3,4,5
extern "C" TALIGN16(const uint8, shufab2[16]) =
  { 2, 128, 5, 128, 128, 128, 10, 128, 13, 128, 128, 128, 128, 128, 128, 128 };

// Scaling values for boxes of 3x2 and 2x2
extern "C" TALIGN16(const uint16, scaleab2[8]) =
  { 65536 / 3, 65536 / 3, 65536 / 2, 65536 / 3, 65536 / 3, 65536 / 2, 0, 0 };
#endif

#if defined(WIN32) && !defined(COVERAGE_ENABLED)

#define HAS_SCALEROWDOWN2_SSE2
// Reads 32 pixels, throws half away and writes 16 pixels.
// Alignment requirement: src_ptr 16 byte aligned, optr 16 byte aligned.
__declspec(naked)
static void ScaleRowDown2_SSE2(const uint8* src_ptr, int src_stride,
                               uint8* optr, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        // src_ptr
                                     // src_stride ignored
    mov        edx, [esp + 12]       // optr
    mov        ecx, [esp + 16]       // dst_width
    pcmpeqb    xmm7, xmm7            // generate mask 0x00ff00ff
    psrlw      xmm7, 8

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm7
    pand       xmm1, xmm7
    packuswb   xmm0, xmm1
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         wloop

    ret
  }
}
// Blends 32x2 rectangle to 16x1.
// Alignment requirement: src_ptr 16 byte aligned, optr 16 byte aligned.
__declspec(naked)
static void ScaleRowDown2Int_SSE2(const uint8* src_ptr, int src_stride,
                                  uint8* optr, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    // src_ptr
    mov        esi, [esp + 4 + 8]    // src_stride
    mov        edx, [esp + 4 + 12]   // optr
    mov        ecx, [esp + 4 + 16]   // dst_width
    pcmpeqb    xmm7, xmm7            // generate mask 0x00ff00ff
    psrlw      xmm7, 8

  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2            // average rows
    pavgb      xmm1, xmm3

    movdqa     xmm2, xmm0            // average columns (32 to 16 pixels)
    psrlw      xmm0, 8
    movdqa     xmm3, xmm1
    psrlw      xmm1, 8
    pand       xmm2, xmm7
    pand       xmm3, xmm7
    pavgw      xmm0, xmm2
    pavgw      xmm1, xmm3
    packuswb   xmm0, xmm1

    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    sub        ecx, 16
    ja         wloop

    pop        esi
    ret
  }
}

#define HAS_SCALEROWDOWN4_SSE2
// Point samples 32 pixels to 8 pixels.
// Alignment requirement: src_ptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown4_SSE2(const uint8* src_ptr, int src_stride,
                               uint8* orow, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
                                     // src_stride ignored
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // dst_width
    pcmpeqb    xmm7, xmm7            // generate mask 0x000000ff
    psrld      xmm7, 24

  wloop:
    movdqa     xmm0, [esi]
    movdqa     xmm1, [esi + 16]
    lea        esi,  [esi + 32]
    pand       xmm0, xmm7
    pand       xmm1, xmm7
    packuswb   xmm0, xmm1
    packuswb   xmm0, xmm0
    movq       qword ptr [edi], xmm0
    lea        edi, [edi + 8]
    sub        ecx, 8
    ja         wloop

    popad
    ret
  }
}

// Blends 32x4 rectangle to 8x1.
// Alignment requirement: src_ptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown4Int_SSE2(const uint8* src_ptr, int src_stride,
                                  uint8* orow, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
    mov        ebx, [esp + 32 + 8]   // src_stride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // dst_width
    pcmpeqb    xmm7, xmm7            // generate mask 0x00ff00ff
    psrlw      xmm7, 8
    lea        edx, [ebx + ebx * 2]  // src_stride * 3

  wloop:
    movdqa     xmm0, [esi]
    movdqa     xmm1, [esi + 16]
    movdqa     xmm2, [esi + ebx]
    movdqa     xmm3, [esi + ebx + 16]
    pavgb      xmm0, xmm2            // average rows
    pavgb      xmm1, xmm3
    movdqa     xmm2, [esi + ebx * 2]
    movdqa     xmm3, [esi + ebx * 2 + 16]
    movdqa     xmm4, [esi + edx]
    movdqa     xmm5, [esi + edx + 16]
    lea        esi, [esi + 32]
    pavgb      xmm2, xmm4
    pavgb      xmm3, xmm5
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3

    movdqa     xmm2, xmm0            // average columns (32 to 16 pixels)
    psrlw      xmm0, 8
    movdqa     xmm3, xmm1
    psrlw      xmm1, 8
    pand       xmm2, xmm7
    pand       xmm3, xmm7
    pavgw      xmm0, xmm2
    pavgw      xmm1, xmm3
    packuswb   xmm0, xmm1

    movdqa     xmm2, xmm0            // average columns (16 to 8 pixels)
    psrlw      xmm0, 8
    pand       xmm2, xmm7
    pavgw      xmm0, xmm2
    packuswb   xmm0, xmm0

    movq       qword ptr [edi], xmm0
    lea        edi, [edi + 8]
    sub        ecx, 8
    ja         wloop

    popad
    ret
  }
}

#define HAS_SCALEROWDOWN8_SSE2
// Point samples 32 pixels to 4 pixels.
// Alignment requirement: src_ptr 16 byte aligned, optr 4 byte aligned.
__declspec(naked)
static void ScaleRowDown8_SSE2(const uint8* src_ptr, int src_stride,
                               uint8* orow, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
                                     // src_stride ignored
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // dst_width
    pcmpeqb    xmm7, xmm7            // generate mask isolating 1 src 8 bytes
    psrlq      xmm7, 56

  wloop:
    movdqa     xmm0, [esi]
    movdqa     xmm1, [esi + 16]
    lea        esi,  [esi + 32]
    pand       xmm0, xmm7
    pand       xmm1, xmm7
    packuswb   xmm0, xmm1  // 32->16
    packuswb   xmm0, xmm0  // 16->8
    packuswb   xmm0, xmm0  // 8->4
    movd       dword ptr [edi], xmm0
    lea        edi, [edi + 4]
    sub        ecx, 4
    ja         wloop

    popad
    ret
  }
}

// Blends 32x8 rectangle to 4x1.
// Alignment requirement: src_ptr 16 byte aligned, optr 4 byte aligned.
__declspec(naked)
static void ScaleRowDown8Int_SSE2(const uint8* src_ptr, int src_stride,
                                  uint8* orow, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
    mov        ebx, [esp + 32 + 8]   // src_stride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // dst_width
    lea        edx, [ebx + ebx * 2]  // src_stride * 3
    pxor       xmm7, xmm7

  wloop:
    movdqa     xmm0, [esi]           // average 8 rows to 1
    movdqa     xmm1, [esi + 16]
    movdqa     xmm2, [esi + ebx]
    movdqa     xmm3, [esi + ebx + 16]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    movdqa     xmm2, [esi + ebx * 2]
    movdqa     xmm3, [esi + ebx * 2 + 16]
    movdqa     xmm4, [esi + edx]
    movdqa     xmm5, [esi + edx + 16]
    lea        ebp, [esi + ebx * 4]
    lea        esi, [esi + 32]
    pavgb      xmm2, xmm4
    pavgb      xmm3, xmm5
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3

    movdqa     xmm2, [ebp]
    movdqa     xmm3, [ebp + 16]
    movdqa     xmm4, [ebp + ebx]
    movdqa     xmm5, [ebp + ebx + 16]
    pavgb      xmm2, xmm4
    pavgb      xmm3, xmm5
    movdqa     xmm4, [ebp + ebx * 2]
    movdqa     xmm5, [ebp + ebx * 2 + 16]
    movdqa     xmm6, [ebp + edx]
    pavgb      xmm4, xmm6
    movdqa     xmm6, [ebp + edx + 16]
    pavgb      xmm5, xmm6
    pavgb      xmm2, xmm4
    pavgb      xmm3, xmm5
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3

    psadbw     xmm0, xmm7            // average 32 pixels to 4
    psadbw     xmm1, xmm7
    pshufd     xmm0, xmm0, 0xd8      // x1x0 -> xx01
    pshufd     xmm1, xmm1, 0x8d      // x3x2 -> 32xx
    por        xmm0, xmm1            //      -> 3201
    psrlw      xmm0, 3
    packuswb   xmm0, xmm0
    packuswb   xmm0, xmm0
    movd       dword ptr [edi], xmm0

    lea        edi, [edi + 4]
    sub        ecx, 4
    ja         wloop

    popad
    ret
  }
}

#define HAS_SCALEROWDOWN34_SSSE3
// Point samples 32 pixels to 24 pixels.
// Produces three 8 byte values.  For each 8 bytes, 16 bytes are read.
// Then shuffled to do the scaling.

// Note that movdqa+palign may be better than movdqu.
// Alignment requirement: src_ptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown34_SSSE3(const uint8* src_ptr, int src_stride,
                                 uint8* orow, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
                                     // src_stride ignored
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // dst_width
    movdqa     xmm3, _shuf0
    movdqa     xmm4, _shuf1
    movdqa     xmm5, _shuf2

  wloop:
    movdqa     xmm0, [esi]
    movdqa     xmm2, [esi + 16]
    lea        esi,  [esi + 32]
    movdqa     xmm1, xmm2
    palignr    xmm1, xmm0, 8
    pshufb     xmm0, xmm3
    pshufb     xmm1, xmm4
    pshufb     xmm2, xmm5
    movq       qword ptr [edi], xmm0
    movq       qword ptr [edi + 8], xmm1
    movq       qword ptr [edi + 16], xmm2
    lea        edi, [edi + 24]
    sub        ecx, 24
    ja         wloop

    popad
    ret
  }
}

// Blends 32x2 rectangle to 24x1
// Produces three 8 byte values.  For each 8 bytes, 16 bytes are read.
// Then shuffled to do the scaling.

// Register usage:
// xmm0 irow 0
// xmm1 irow 1
// xmm2 shuf 0
// xmm3 shuf 1
// xmm4 shuf 2
// xmm5 madd 0
// xmm6 madd 1
// xmm7 round34

// Note that movdqa+palign may be better than movdqu.
// Alignment requirement: src_ptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown34_1_Int_SSSE3(const uint8* src_ptr, int src_stride,
                                       uint8* orow, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
    mov        ebx, [esp + 32 + 8]   // src_stride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // dst_width
    movdqa     xmm2, _shuf01
    movdqa     xmm3, _shuf11
    movdqa     xmm4, _shuf21
    movdqa     xmm5, _madd01
    movdqa     xmm6, _madd11
    movdqa     xmm7, _round34

  wloop:
    movdqa     xmm0, [esi]           // pixels 0..7
    movdqa     xmm1, [esi+ebx]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm2
    pmaddubsw  xmm0, xmm5
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edi], xmm0
    movdqu     xmm0, [esi+8]         // pixels 8..15
    movdqu     xmm1, [esi+ebx+8]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm3
    pmaddubsw  xmm0, xmm6
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edi+8], xmm0
    movdqa     xmm0, [esi+16]        // pixels 16..23
    movdqa     xmm1, [esi+ebx+16]
    lea        esi, [esi+32]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm4
    movdqa     xmm1, _madd21
    pmaddubsw  xmm0, xmm1
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edi+16], xmm0
    lea        edi, [edi+24]
    sub        ecx, 24
    ja         wloop

    popad
    ret
  }
}

// Note that movdqa+palign may be better than movdqu.
// Alignment requirement: src_ptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown34_0_Int_SSSE3(const uint8* src_ptr, int src_stride,
                                       uint8* orow, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
    mov        ebx, [esp + 32 + 8]   // src_stride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // dst_width
    movdqa     xmm2, _shuf01
    movdqa     xmm3, _shuf11
    movdqa     xmm4, _shuf21
    movdqa     xmm5, _madd01
    movdqa     xmm6, _madd11
    movdqa     xmm7, _round34

  wloop:
    movdqa     xmm0, [esi]           // pixels 0..7
    movdqa     xmm1, [esi+ebx]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm2
    pmaddubsw  xmm0, xmm5
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edi], xmm0
    movdqu     xmm0, [esi+8]         // pixels 8..15
    movdqu     xmm1, [esi+ebx+8]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm3
    pmaddubsw  xmm0, xmm6
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edi+8], xmm0
    movdqa     xmm0, [esi+16]        // pixels 16..23
    movdqa     xmm1, [esi+ebx+16]
    lea        esi, [esi+32]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm4
    movdqa     xmm1, _madd21
    pmaddubsw  xmm0, xmm1
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edi+16], xmm0
    lea        edi, [edi+24]
    sub        ecx, 24
    ja         wloop

    popad
    ret
  }
}

#define HAS_SCALEROWDOWN38_SSSE3
// 3/8 point sampler

// Scale 32 pixels to 12
__declspec(naked)
static void ScaleRowDown38_SSSE3(const uint8* src_ptr, int src_stride,
                                 uint8* optr, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
    mov        edx, [esp + 32 + 8]   // src_stride
    mov        edi, [esp + 32 + 12]  // optr
    mov        ecx, [esp + 32 + 16]  // dst_width
    movdqa     xmm5, _shuf38a
    movdqa     xmm6, _shuf38b
    pxor       xmm7, xmm7

  xloop:
    movdqa     xmm0, [esi]           // 16 pixels -> 0,1,2,3,4,5
    movdqa     xmm1, [esi + 16]      // 16 pixels -> 6,7,8,9,10,11
    lea        esi, [esi + 32]
    pshufb     xmm0, xmm5
    pshufb     xmm1, xmm6
    paddusb    xmm0, xmm1

    movq       qword ptr [edi], xmm0 // write 12 pixels
    movhlps    xmm1, xmm0
    movd       [edi + 8], xmm1
    lea        edi, [edi + 12]
    sub        ecx, 12
    ja         xloop

    popad
    ret
  }
}

// Scale 16x3 pixels to 6x1 with interpolation
__declspec(naked)
static void ScaleRowDown38_3_Int_SSSE3(const uint8* src_ptr, int src_stride,
                                       uint8* optr, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
    mov        edx, [esp + 32 + 8]   // src_stride
    mov        edi, [esp + 32 + 12]  // optr
    mov        ecx, [esp + 32 + 16]  // dst_width
    movdqa     xmm4, _shufac0
    movdqa     xmm5, _shufac3
    movdqa     xmm6, _scaleac3
    pxor       xmm7, xmm7

  xloop:
    movdqa     xmm0, [esi]           // sum up 3 rows into xmm0/1
    movdqa     xmm2, [esi + edx]
    movhlps    xmm1, xmm0
    movhlps    xmm3, xmm2
    punpcklbw  xmm0, xmm7
    punpcklbw  xmm1, xmm7
    punpcklbw  xmm2, xmm7
    punpcklbw  xmm3, xmm7
    paddusw    xmm0, xmm2
    paddusw    xmm1, xmm3
    movdqa     xmm2, [esi + edx * 2]
    lea        esi, [esi + 16]
    movhlps    xmm3, xmm2
    punpcklbw  xmm2, xmm7
    punpcklbw  xmm3, xmm7
    paddusw    xmm0, xmm2
    paddusw    xmm1, xmm3

    movdqa     xmm2, xmm0            // 8 pixels -> 0,1,2 of xmm2
    psrldq     xmm0, 2
    paddusw    xmm2, xmm0
    psrldq     xmm0, 2
    paddusw    xmm2, xmm0
    pshufb     xmm2, xmm4

    movdqa     xmm3, xmm1            // 8 pixels -> 3,4,5 of xmm2
    psrldq     xmm1, 2
    paddusw    xmm3, xmm1
    psrldq     xmm1, 2
    paddusw    xmm3, xmm1
    pshufb     xmm3, xmm5
    paddusw    xmm2, xmm3

    pmulhuw    xmm2, xmm6            // divide by 9,9,6, 9,9,6
    packuswb   xmm2, xmm2

    movd       [edi], xmm2           // write 6 pixels
    pextrw     eax, xmm2, 2
    mov        [edi + 4], ax
    lea        edi, [edi + 6]
    sub        ecx, 6
    ja         xloop

    popad
    ret
  }
}

// Scale 16x2 pixels to 6x1 with interpolation
__declspec(naked)
static void ScaleRowDown38_2_Int_SSSE3(const uint8* src_ptr, int src_stride,
                                       uint8* optr, int dst_width) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
    mov        edx, [esp + 32 + 8]   // src_stride
    mov        edi, [esp + 32 + 12]  // optr
    mov        ecx, [esp + 32 + 16]  // dst_width
    movdqa     xmm4, _shufab0
    movdqa     xmm5, _shufab1
    movdqa     xmm6, _shufab2
    movdqa     xmm7, _scaleab2

  xloop:
    movdqa     xmm2, [esi]           // average 2 rows into xmm2
    pavgb      xmm2, [esi + edx]
    lea        esi, [esi + 16]

    movdqa     xmm0, xmm2            // 16 pixels -> 0,1,2,3,4,5 of xmm0
    pshufb     xmm0, xmm4
    movdqa     xmm1, xmm2
    pshufb     xmm1, xmm5
    paddusw    xmm0, xmm1
    pshufb     xmm2, xmm6
    paddusw    xmm0, xmm2

    pmulhuw    xmm0, xmm7            // divide by 3,3,2, 3,3,2
    packuswb   xmm0, xmm0

    movd       [edi], xmm0           // write 6 pixels
    pextrw     eax, xmm0, 2
    mov        [edi + 4], ax
    lea        edi, [edi + 6]
    sub        ecx, 6
    ja         xloop

    popad
    ret
  }
}

#define HAS_SCALEADDROWS_SSE2

// Reads 8xN bytes and produces 16 shorts at a time.
__declspec(naked)
static void ScaleAddRows_SSE2(const uint8* src_ptr, int src_stride,
                              uint16* orow, int src_width, int src_height) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // src_ptr
    mov        edx, [esp + 32 + 8]   // src_stride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // dst_width
    mov        ebx, [esp + 32 + 20]  // height
    pxor       xmm7, xmm7
    dec        ebx

  xloop:
    // first row
    movdqa     xmm2, [esi]
    lea        eax, [esi + edx]
    movhlps    xmm3, xmm2
    mov        ebp, ebx
    punpcklbw  xmm2, xmm7
    punpcklbw  xmm3, xmm7

    // sum remaining rows
  yloop:
    movdqa     xmm0, [eax]       // read 16 pixels
    lea        eax, [eax + edx]  // advance to next row
    movhlps    xmm1, xmm0
    punpcklbw  xmm0, xmm7
    punpcklbw  xmm1, xmm7
    paddusw    xmm2, xmm0        // sum 16 words
    paddusw    xmm3, xmm1
    sub        ebp, 1
    ja         yloop

    movdqa     [edi], xmm2
    movdqa     [edi + 16], xmm3
    lea        edi, [edi + 32]
    lea        esi, [esi + 16]

    sub        ecx, 16
    ja         xloop

    popad
    ret
  }
}

// Bilinear row filtering combines 16x2 -> 16x1. SSE2 version.
#define HAS_SCALEFILTERROWS_SSE2
__declspec(naked)
static void ScaleFilterRows_SSE2(uint8* optr, const uint8* iptr0, int src_stride,
                                 int dst_width, int source_y_fraction) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]   // optr
    mov        esi, [esp + 8 + 8]   // iptr0
    mov        edx, [esp + 8 + 12]  // src_stride
    mov        ecx, [esp + 8 + 16]  // dst_width
    mov        eax, [esp + 8 + 20]  // source_y_fraction (0..255)
    cmp        eax, 0
    je         xloop1
    cmp        eax, 128
    je         xloop2

    movd       xmm6, eax            // xmm6 = y fraction
    punpcklwd  xmm6, xmm6
    pshufd     xmm6, xmm6, 0
    neg        eax                  // xmm5 = 256 - y fraction
    add        eax, 256
    movd       xmm5, eax
    punpcklwd  xmm5, xmm5
    pshufd     xmm5, xmm5, 0
    pxor       xmm7, xmm7

  xloop:
    movdqa     xmm0, [esi]
    movdqa     xmm2, [esi + edx]
    lea        esi, [esi + 16]
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    punpcklbw  xmm0, xmm7
    punpcklbw  xmm2, xmm7
    punpckhbw  xmm1, xmm7
    punpckhbw  xmm3, xmm7
    pmullw     xmm0, xmm5           // scale row 0
    pmullw     xmm1, xmm5
    pmullw     xmm2, xmm6           // scale row 1
    pmullw     xmm3, xmm6
    paddusw    xmm0, xmm2           // sum rows
    paddusw    xmm1, xmm3
    psrlw      xmm0, 8
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 16
    ja         xloop

    mov        al, [edi - 1]
    mov        [edi], al
    pop        edi
    pop        esi
    ret

  xloop1:
    movdqa     xmm0, [esi]
    lea        esi, [esi + 16]
    movdqa     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 16
    ja         xloop1

    mov        al, [edi - 1]
    mov        [edi], al
    pop        edi
    pop        esi
    ret

  xloop2:
    movdqa     xmm0, [esi]
    movdqa     xmm2, [esi + edx]
    lea        esi, [esi + 16]
    pavgb      xmm0, xmm2
    movdqa     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 16
    ja         xloop2

    mov        al, [edi - 1]
    mov        [edi], al
    pop        edi
    pop        esi
    ret
  }
}

// Bilinear row filtering combines 16x2 -> 16x1. SSSE3 version.
#define HAS_SCALEFILTERROWS_SSSE3
__declspec(naked)
static void ScaleFilterRows_SSSE3(uint8* optr, const uint8* iptr0, int src_stride,
                                  int dst_width, int source_y_fraction) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]   // optr
    mov        esi, [esp + 8 + 8]   // iptr0
    mov        edx, [esp + 8 + 12]  // src_stride
    mov        ecx, [esp + 8 + 16]  // dst_width
    mov        eax, [esp + 8 + 20]  // source_y_fraction (0..255)
    cmp        eax, 0
    je         xloop1
    cmp        eax, 128
    je         xloop2

    shr        eax, 1
    mov        ah,al
    neg        al
    add        al, 128
    movd       xmm7, eax
    punpcklwd  xmm7, xmm7
    pshufd     xmm7, xmm7, 0

  xloop:
    movdqa     xmm0, [esi]
    movdqa     xmm2, [esi + edx]
    lea        esi, [esi + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm2
    punpckhbw  xmm1, xmm2
    pmaddubsw  xmm0, xmm7
    pmaddubsw  xmm1, xmm7
    psrlw      xmm0, 7
    psrlw      xmm1, 7
    packuswb   xmm0, xmm1
    movdqa     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 16
    ja         xloop

    mov        al, [edi - 1]
    mov        [edi], al
    pop        edi
    pop        esi
    ret

  xloop1:
    movdqa     xmm0, [esi]
    lea        esi, [esi + 16]
    movdqa     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 16
    ja         xloop1

    mov        al, [edi - 1]
    mov        [edi], al
    pop        edi
    pop        esi
    ret

  xloop2:
    movdqa     xmm0, [esi]
    movdqa     xmm2, [esi + edx]
    lea        esi, [esi + 16]
    pavgb      xmm0, xmm2
    movdqa     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 16
    ja         xloop2

    mov        al, [edi - 1]
    mov        [edi], al
    pop        edi
    pop        esi
    ret

  }
}

// Note that movdqa+palign may be better than movdqu.
// Alignment requirement: src_ptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleFilterCols34_SSSE3(uint8* optr, const uint8* src_ptr,
                                    int dst_width) {
  __asm {
    mov        edx, [esp + 4]    // optr
    mov        eax, [esp + 8]    // src_ptr
    mov        ecx, [esp + 12]   // dst_width
    movdqa     xmm1, _round34
    movdqa     xmm2, _shuf01
    movdqa     xmm3, _shuf11
    movdqa     xmm4, _shuf21
    movdqa     xmm5, _madd01
    movdqa     xmm6, _madd11
    movdqa     xmm7, _madd21

  wloop:
    movdqa     xmm0, [eax]           // pixels 0..7
    pshufb     xmm0, xmm2
    pmaddubsw  xmm0, xmm5
    paddsw     xmm0, xmm1
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    movdqu     xmm0, [eax+8]         // pixels 8..15
    pshufb     xmm0, xmm3
    pmaddubsw  xmm0, xmm6
    paddsw     xmm0, xmm1
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx+8], xmm0
    movdqa     xmm0, [eax+16]        // pixels 16..23
    lea        eax, [eax+32]
    pshufb     xmm0, xmm4
    pmaddubsw  xmm0, xmm7
    paddsw     xmm0, xmm1
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx+16], xmm0
    lea        edx, [edx+24]
    sub        ecx, 24
    ja         wloop
    ret
  }
}

#elif defined(__i386__) && !defined(COVERAGE_ENABLED) && \
    !TARGET_IPHONE_SIMULATOR

// GCC versions of row functions are verbatim conversions from Visual C.
// Generated using gcc disassembly on Visual C object file:
// objdump -D yuvscaler.obj >yuvscaler.txt
#define HAS_SCALEROWDOWN2_SSE2
extern "C" void ScaleRowDown2_SSE2(const uint8* src_ptr, int src_stride,
                                   uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown2_SSE2\n"
"_ScaleRowDown2_SSE2:\n"
#else
    ".global ScaleRowDown2_SSE2\n"
"ScaleRowDown2_SSE2:\n"
#endif
    "mov    0x4(%esp),%eax\n"
    "mov    0xc(%esp),%edx\n"
    "mov    0x10(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlw  $0x8,%xmm7\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "lea    0x20(%eax),%eax\n"
    "pand   %xmm7,%xmm0\n"
    "pand   %xmm7,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,(%edx)\n"
    "lea    0x10(%edx),%edx\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "ret\n"
);

extern "C" void ScaleRowDown2Int_SSE2(const uint8* src_ptr, int src_stride,
                                      uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown2Int_SSE2\n"
"_ScaleRowDown2Int_SSE2:\n"
#else
    ".global ScaleRowDown2Int_SSE2\n"
"ScaleRowDown2Int_SSE2:\n"
#endif
    "push   %esi\n"
    "mov    0x8(%esp),%eax\n"
    "mov    0xc(%esp),%esi\n"
    "mov    0x10(%esp),%edx\n"
    "mov    0x14(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlw  $0x8,%xmm7\n"

"1:"
    "movdqa (%eax),%xmm0\n"
    "movdqa 0x10(%eax),%xmm1\n"
    "movdqa (%eax,%esi,1),%xmm2\n"
    "movdqa 0x10(%eax,%esi,1),%xmm3\n"
    "lea    0x20(%eax),%eax\n"
    "pavgb  %xmm2,%xmm0\n"
    "pavgb  %xmm3,%xmm1\n"
    "movdqa %xmm0,%xmm2\n"
    "psrlw  $0x8,%xmm0\n"
    "movdqa %xmm1,%xmm3\n"
    "psrlw  $0x8,%xmm1\n"
    "pand   %xmm7,%xmm2\n"
    "pand   %xmm7,%xmm3\n"
    "pavgw  %xmm2,%xmm0\n"
    "pavgw  %xmm3,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,(%edx)\n"
    "lea    0x10(%edx),%edx\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "pop    %esi\n"
    "ret\n"
);

#define HAS_SCALEROWDOWN4_SSE2
extern "C" void ScaleRowDown4_SSE2(const uint8* src_ptr, int src_stride,
                                   uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown4_SSE2\n"
"_ScaleRowDown4_SSE2:\n"
#else
    ".global ScaleRowDown4_SSE2\n"
"ScaleRowDown4_SSE2:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrld  $0x18,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa 0x10(%esi),%xmm1\n"
    "lea    0x20(%esi),%esi\n"
    "pand   %xmm7,%xmm0\n"
    "pand   %xmm7,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,(%edi)\n"
    "lea    0x8(%edi),%edi\n"
    "sub    $0x8,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

extern "C" void ScaleRowDown4Int_SSE2(const uint8* src_ptr, int src_stride,
                                      uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown4Int_SSE2\n"
"_ScaleRowDown4Int_SSE2:\n"
#else
    ".global ScaleRowDown4Int_SSE2\n"
"ScaleRowDown4Int_SSE2:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x28(%esp),%ebx\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlw  $0x8,%xmm7\n"
    "lea    (%ebx,%ebx,2),%edx\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa 0x10(%esi),%xmm1\n"
    "movdqa (%esi,%ebx,1),%xmm2\n"
    "movdqa 0x10(%esi,%ebx,1),%xmm3\n"
    "pavgb  %xmm2,%xmm0\n"
    "pavgb  %xmm3,%xmm1\n"
    "movdqa (%esi,%ebx,2),%xmm2\n"
    "movdqa 0x10(%esi,%ebx,2),%xmm3\n"
    "movdqa (%esi,%edx,1),%xmm4\n"
    "movdqa 0x10(%esi,%edx,1),%xmm5\n"
    "lea    0x20(%esi),%esi\n"
    "pavgb  %xmm4,%xmm2\n"
    "pavgb  %xmm2,%xmm0\n"
    "pavgb  %xmm5,%xmm3\n"
    "pavgb  %xmm3,%xmm1\n"
    "movdqa %xmm0,%xmm2\n"
    "psrlw  $0x8,%xmm0\n"
    "movdqa %xmm1,%xmm3\n"
    "psrlw  $0x8,%xmm1\n"
    "pand   %xmm7,%xmm2\n"
    "pand   %xmm7,%xmm3\n"
    "pavgw  %xmm2,%xmm0\n"
    "pavgw  %xmm3,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,%xmm2\n"
    "psrlw  $0x8,%xmm0\n"
    "pand   %xmm7,%xmm2\n"
    "pavgw  %xmm2,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,(%edi)\n"
    "lea    0x8(%edi),%edi\n"
    "sub    $0x8,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

#define HAS_SCALEROWDOWN8_SSE2
extern "C" void ScaleRowDown8_SSE2(const uint8* src_ptr, int src_stride,
                                   uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown8_SSE2\n"
"_ScaleRowDown8_SSE2:\n"
#else
    ".global ScaleRowDown8_SSE2\n"
"ScaleRowDown8_SSE2:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "pcmpeqb %xmm7,%xmm7\n"
    "psrlq  $0x38,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa 0x10(%esi),%xmm1\n"
    "lea    0x20(%esi),%esi\n"
    "pand   %xmm7,%xmm0\n"
    "pand   %xmm7,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movd   %xmm0,(%edi)\n"
    "lea    0x4(%edi),%edi\n"
    "sub    $0x4,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

extern "C" void ScaleRowDown8Int_SSE2(const uint8* src_ptr, int src_stride,
                                      uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown8Int_SSE2\n"
"_ScaleRowDown8Int_SSE2:\n"
#else
    ".global ScaleRowDown8Int_SSE2\n"
"ScaleRowDown8Int_SSE2:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x28(%esp),%ebx\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "lea    (%ebx,%ebx,2),%edx\n"
    "pxor   %xmm7,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa 0x10(%esi),%xmm1\n"
    "movdqa (%esi,%ebx,1),%xmm2\n"
    "movdqa 0x10(%esi,%ebx,1),%xmm3\n"
    "pavgb  %xmm2,%xmm0\n"
    "pavgb  %xmm3,%xmm1\n"
    "movdqa (%esi,%ebx,2),%xmm2\n"
    "movdqa 0x10(%esi,%ebx,2),%xmm3\n"
    "movdqa (%esi,%edx,1),%xmm4\n"
    "movdqa 0x10(%esi,%edx,1),%xmm5\n"
    "lea    (%esi,%ebx,4),%ebp\n"
    "lea    0x20(%esi),%esi\n"
    "pavgb  %xmm4,%xmm2\n"
    "pavgb  %xmm5,%xmm3\n"
    "pavgb  %xmm2,%xmm0\n"
    "pavgb  %xmm3,%xmm1\n"
    "movdqa 0x0(%ebp),%xmm2\n"
    "movdqa 0x10(%ebp),%xmm3\n"
    "movdqa 0x0(%ebp,%ebx,1),%xmm4\n"
    "movdqa 0x10(%ebp,%ebx,1),%xmm5\n"
    "pavgb  %xmm4,%xmm2\n"
    "pavgb  %xmm5,%xmm3\n"
    "movdqa 0x0(%ebp,%ebx,2),%xmm4\n"
    "movdqa 0x10(%ebp,%ebx,2),%xmm5\n"
    "movdqa 0x0(%ebp,%edx,1),%xmm6\n"
    "pavgb  %xmm6,%xmm4\n"
    "movdqa 0x10(%ebp,%edx,1),%xmm6\n"
    "pavgb  %xmm6,%xmm5\n"
    "pavgb  %xmm4,%xmm2\n"
    "pavgb  %xmm5,%xmm3\n"
    "pavgb  %xmm2,%xmm0\n"
    "pavgb  %xmm3,%xmm1\n"
    "psadbw %xmm7,%xmm0\n"
    "psadbw %xmm7,%xmm1\n"
    "pshufd $0xd8,%xmm0,%xmm0\n"
    "pshufd $0x8d,%xmm1,%xmm1\n"
    "por    %xmm1,%xmm0\n"
    "psrlw  $0x3,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movd   %xmm0,(%edi)\n"
    "lea    0x4(%edi),%edi\n"
    "sub    $0x4,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

// fpic is used for magiccam plugin
#if !defined(__PIC__)
#define HAS_SCALEROWDOWN34_SSSE3
extern "C" void ScaleRowDown34_SSSE3(const uint8* src_ptr, int src_stride,
                                     uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown34_SSSE3\n"
"_ScaleRowDown34_SSSE3:\n"
#else
    ".global ScaleRowDown34_SSSE3\n"
"ScaleRowDown34_SSSE3:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "movdqa _shuf0,%xmm3\n"
    "movdqa _shuf1,%xmm4\n"
    "movdqa _shuf2,%xmm5\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa 0x10(%esi),%xmm2\n"
    "lea    0x20(%esi),%esi\n"
    "movdqa %xmm2,%xmm1\n"
    "palignr $0x8,%xmm0,%xmm1\n"
    "pshufb %xmm3,%xmm0\n"
    "pshufb %xmm4,%xmm1\n"
    "pshufb %xmm5,%xmm2\n"
    "movq   %xmm0,(%edi)\n"
    "movq   %xmm1,0x8(%edi)\n"
    "movq   %xmm2,0x10(%edi)\n"
    "lea    0x18(%edi),%edi\n"
    "sub    $0x18,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

extern "C" void ScaleRowDown34_1_Int_SSSE3(const uint8* src_ptr, int src_stride,
                                           uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown34_1_Int_SSSE3\n"
"_ScaleRowDown34_1_Int_SSSE3:\n"
#else
    ".global ScaleRowDown34_1_Int_SSSE3\n"
"ScaleRowDown34_1_Int_SSSE3:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x28(%esp),%ebp\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "movdqa _shuf01,%xmm2\n"
    "movdqa _shuf11,%xmm3\n"
    "movdqa _shuf21,%xmm4\n"
    "movdqa _madd01,%xmm5\n"
    "movdqa _madd11,%xmm6\n"
    "movdqa _round34,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa (%esi,%ebp),%xmm1\n"
    "pavgb  %xmm1,%xmm0\n"
    "pshufb %xmm2,%xmm0\n"
    "pmaddubsw %xmm5,%xmm0\n"
    "paddsw %xmm7,%xmm0\n"
    "psrlw  $0x2,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,(%edi)\n"
    "movdqu 0x8(%esi),%xmm0\n"
    "movdqu 0x8(%esi,%ebp),%xmm1\n"
    "pavgb  %xmm1,%xmm0\n"
    "pshufb %xmm3,%xmm0\n"
    "pmaddubsw %xmm6,%xmm0\n"
    "paddsw %xmm7,%xmm0\n"
    "psrlw  $0x2,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,0x8(%edi)\n"
    "movdqa 0x10(%esi),%xmm0\n"
    "movdqa 0x10(%esi,%ebp),%xmm1\n"
    "lea    0x20(%esi),%esi\n"
    "pavgb  %xmm1,%xmm0\n"
    "pshufb %xmm4,%xmm0\n"
    "movdqa  _madd21,%xmm1\n"
    "pmaddubsw %xmm1,%xmm0\n"
    "paddsw %xmm7,%xmm0\n"
    "psrlw  $0x2,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,0x10(%edi)\n"
    "lea    0x18(%edi),%edi\n"
    "sub    $0x18,%ecx\n"
    "ja     1b\n"

    "popa\n"
    "ret\n"
);

extern "C" void ScaleRowDown34_0_Int_SSSE3(const uint8* src_ptr, int src_stride,
                                           uint8* orow, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown34_0_Int_SSSE3\n"
"_ScaleRowDown34_0_Int_SSSE3:\n"
#else
    ".global ScaleRowDown34_0_Int_SSSE3\n"
"ScaleRowDown34_0_Int_SSSE3:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x28(%esp),%ebp\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "movdqa _shuf01,%xmm2\n"
    "movdqa _shuf11,%xmm3\n"
    "movdqa _shuf21,%xmm4\n"
    "movdqa _madd01,%xmm5\n"
    "movdqa _madd11,%xmm6\n"
    "movdqa _round34,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa (%esi,%ebp,1),%xmm1\n"
    "pavgb  %xmm0,%xmm1\n"
    "pavgb  %xmm1,%xmm0\n"
    "pshufb %xmm2,%xmm0\n"
    "pmaddubsw %xmm5,%xmm0\n"
    "paddsw %xmm7,%xmm0\n"
    "psrlw  $0x2,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,(%edi)\n"
    "movdqu 0x8(%esi),%xmm0\n"
    "movdqu 0x8(%esi,%ebp,1),%xmm1\n"
    "pavgb  %xmm0,%xmm1\n"
    "pavgb  %xmm1,%xmm0\n"
    "pshufb %xmm3,%xmm0\n"
    "pmaddubsw %xmm6,%xmm0\n"
    "paddsw %xmm7,%xmm0\n"
    "psrlw  $0x2,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,0x8(%edi)\n"
    "movdqa 0x10(%esi),%xmm0\n"
    "movdqa 0x10(%esi,%ebp,1),%xmm1\n"
    "lea    0x20(%esi),%esi\n"
    "pavgb  %xmm0,%xmm1\n"
    "pavgb  %xmm1,%xmm0\n"
    "pshufb %xmm4,%xmm0\n"
    "movdqa  _madd21,%xmm1\n"
    "pmaddubsw %xmm1,%xmm0\n"
    "paddsw %xmm7,%xmm0\n"
    "psrlw  $0x2,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movq   %xmm0,0x10(%edi)\n"
    "lea    0x18(%edi),%edi\n"
    "sub    $0x18,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

#define HAS_SCALEROWDOWN38_SSSE3
extern "C" void ScaleRowDown38_SSSE3(const uint8* src_ptr, int src_stride,
                                     uint8* optr, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown38_SSSE3\n"
"_ScaleRowDown38_SSSE3:\n"
#else
    ".global ScaleRowDown38_SSSE3\n"
"ScaleRowDown38_SSSE3:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x28(%esp),%edx\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "movdqa _shuf38a ,%xmm5\n"
    "movdqa _shuf38b ,%xmm6\n"
    "pxor   %xmm7,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa 0x10(%esi),%xmm1\n"
    "lea    0x20(%esi),%esi\n"
    "pshufb %xmm5,%xmm0\n"
    "pshufb %xmm6,%xmm1\n"
    "paddusb %xmm1,%xmm0\n"
    "movq   %xmm0,(%edi)\n"
    "movhlps %xmm0,%xmm1\n"
    "movd   %xmm1,0x8(%edi)\n"
    "lea    0xc(%edi),%edi\n"
    "sub    $0xc,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

extern "C" void ScaleRowDown38_3_Int_SSSE3(const uint8* src_ptr, int src_stride,
                                           uint8* optr, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown38_3_Int_SSSE3\n"
"_ScaleRowDown38_3_Int_SSSE3:\n"
#else
    ".global ScaleRowDown38_3_Int_SSSE3\n"
"ScaleRowDown38_3_Int_SSSE3:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x28(%esp),%edx\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "movdqa _shufac0,%xmm4\n"
    "movdqa _shufac3,%xmm5\n"
    "movdqa _scaleac3,%xmm6\n"
    "pxor   %xmm7,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa (%esi,%edx,1),%xmm2\n"
    "movhlps %xmm0,%xmm1\n"
    "movhlps %xmm2,%xmm3\n"
    "punpcklbw %xmm7,%xmm0\n"
    "punpcklbw %xmm7,%xmm1\n"
    "punpcklbw %xmm7,%xmm2\n"
    "punpcklbw %xmm7,%xmm3\n"
    "paddusw %xmm2,%xmm0\n"
    "paddusw %xmm3,%xmm1\n"
    "movdqa (%esi,%edx,2),%xmm2\n"
    "lea    0x10(%esi),%esi\n"
    "movhlps %xmm2,%xmm3\n"
    "punpcklbw %xmm7,%xmm2\n"
    "punpcklbw %xmm7,%xmm3\n"
    "paddusw %xmm2,%xmm0\n"
    "paddusw %xmm3,%xmm1\n"
    "movdqa %xmm0,%xmm2\n"
    "psrldq $0x2,%xmm0\n"
    "paddusw %xmm0,%xmm2\n"
    "psrldq $0x2,%xmm0\n"
    "paddusw %xmm0,%xmm2\n"
    "pshufb %xmm4,%xmm2\n"
    "movdqa %xmm1,%xmm3\n"
    "psrldq $0x2,%xmm1\n"
    "paddusw %xmm1,%xmm3\n"
    "psrldq $0x2,%xmm1\n"
    "paddusw %xmm1,%xmm3\n"
    "pshufb %xmm5,%xmm3\n"
    "paddusw %xmm3,%xmm2\n"
    "pmulhuw %xmm6,%xmm2\n"
    "packuswb %xmm2,%xmm2\n"
    "movd   %xmm2,(%edi)\n"
    "pextrw $0x2,%xmm2,%eax\n"
    "mov    %ax,0x4(%edi)\n"
    "lea    0x6(%edi),%edi\n"
    "sub    $0x6,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

extern "C" void ScaleRowDown38_2_Int_SSSE3(const uint8* src_ptr, int src_stride,
                                           uint8* optr, int dst_width);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleRowDown38_2_Int_SSSE3\n"
"_ScaleRowDown38_2_Int_SSSE3:\n"
#else
    ".global ScaleRowDown38_2_Int_SSSE3\n"
"ScaleRowDown38_2_Int_SSSE3:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x28(%esp),%edx\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "movdqa _shufab0,%xmm4\n"
    "movdqa _shufab1,%xmm5\n"
    "movdqa _shufab2,%xmm6\n"
    "movdqa _scaleab2,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm2\n"
    "pavgb  (%esi,%edx,1),%xmm2\n"
    "lea    0x10(%esi),%esi\n"
    "movdqa %xmm2,%xmm0\n"
    "pshufb %xmm4,%xmm0\n"
    "movdqa %xmm2,%xmm1\n"
    "pshufb %xmm5,%xmm1\n"
    "paddusw %xmm1,%xmm0\n"
    "pshufb %xmm6,%xmm2\n"
    "paddusw %xmm2,%xmm0\n"
    "pmulhuw %xmm7,%xmm0\n"
    "packuswb %xmm0,%xmm0\n"
    "movd   %xmm0,(%edi)\n"
    "pextrw $0x2,%xmm0,%eax\n"
    "mov    %ax,0x4(%edi)\n"
    "lea    0x6(%edi),%edi\n"
    "sub    $0x6,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);
#endif // __PIC__

#define HAS_SCALEADDROWS_SSE2
extern "C" void ScaleAddRows_SSE2(const uint8* src_ptr, int src_stride,
                                  uint16* orow, int src_width, int src_height);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleAddRows_SSE2\n"
"_ScaleAddRows_SSE2:\n"
#else
    ".global ScaleAddRows_SSE2\n"
"ScaleAddRows_SSE2:\n"
#endif
    "pusha\n"
    "mov    0x24(%esp),%esi\n"
    "mov    0x28(%esp),%edx\n"
    "mov    0x2c(%esp),%edi\n"
    "mov    0x30(%esp),%ecx\n"
    "mov    0x34(%esp),%ebx\n"
    "pxor   %xmm7,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm2\n"
    "lea    (%esi,%edx,1),%eax\n"
    "movhlps %xmm2,%xmm3\n"
    "lea    -0x1(%ebx),%ebp\n"
    "punpcklbw %xmm7,%xmm2\n"
    "punpcklbw %xmm7,%xmm3\n"

"2:"
    "movdqa (%eax),%xmm0\n"
    "lea    (%eax,%edx,1),%eax\n"
    "movhlps %xmm0,%xmm1\n"
    "punpcklbw %xmm7,%xmm0\n"
    "punpcklbw %xmm7,%xmm1\n"
    "paddusw %xmm0,%xmm2\n"
    "paddusw %xmm1,%xmm3\n"
    "sub    $0x1,%ebp\n"
    "ja     2b\n"

    "movdqa %xmm2,(%edi)\n"
    "movdqa %xmm3,0x10(%edi)\n"
    "lea    0x20(%edi),%edi\n"
    "lea    0x10(%esi),%esi\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "popa\n"
    "ret\n"
);

// Bilinear row filtering combines 16x2 -> 16x1. SSE2 version
#define HAS_SCALEFILTERROWS_SSE2
extern "C" void ScaleFilterRows_SSE2(uint8* optr,
                                     const uint8* iptr0, int src_stride,
                                     int dst_width, int source_y_fraction);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleFilterRows_SSE2\n"
"_ScaleFilterRows_SSE2:\n"
#else
    ".global ScaleFilterRows_SSE2\n"
"ScaleFilterRows_SSE2:\n"
#endif
    "push   %esi\n"
    "push   %edi\n"
    "mov    0xc(%esp),%edi\n"
    "mov    0x10(%esp),%esi\n"
    "mov    0x14(%esp),%edx\n"
    "mov    0x18(%esp),%ecx\n"
    "mov    0x1c(%esp),%eax\n"
    "cmp    $0x0,%eax\n"
    "je     2f\n"
    "cmp    $0x80,%eax\n"
    "je     3f\n"
    "movd   %eax,%xmm6\n"
    "punpcklwd %xmm6,%xmm6\n"
    "pshufd $0x0,%xmm6,%xmm6\n"
    "neg    %eax\n"
    "add    $0x100,%eax\n"
    "movd   %eax,%xmm5\n"
    "punpcklwd %xmm5,%xmm5\n"
    "pshufd $0x0,%xmm5,%xmm5\n"
    "pxor   %xmm7,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa (%esi,%edx,1),%xmm2\n"
    "lea    0x10(%esi),%esi\n"
    "movdqa %xmm0,%xmm1\n"
    "movdqa %xmm2,%xmm3\n"
    "punpcklbw %xmm7,%xmm0\n"
    "punpcklbw %xmm7,%xmm2\n"
    "punpckhbw %xmm7,%xmm1\n"
    "punpckhbw %xmm7,%xmm3\n"
    "pmullw %xmm5,%xmm0\n"
    "pmullw %xmm5,%xmm1\n"
    "pmullw %xmm6,%xmm2\n"
    "pmullw %xmm6,%xmm3\n"
    "paddusw %xmm2,%xmm0\n"
    "paddusw %xmm3,%xmm1\n"
    "psrlw  $0x8,%xmm0\n"
    "psrlw  $0x8,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "mov    -0x1(%edi),%al\n"
    "mov    %al,(%edi)\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"

"2:"
    "movdqa (%esi),%xmm0\n"
    "lea    0x10(%esi),%esi\n"
    "movdqa %xmm0,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     2b\n"

    "mov    -0x1(%edi),%al\n"
    "mov    %al,(%edi)\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"

"3:"
    "movdqa (%esi),%xmm0\n"
    "movdqa (%esi,%edx,1),%xmm2\n"
    "lea    0x10(%esi),%esi\n"
    "pavgb  %xmm2,%xmm0\n"
    "movdqa %xmm0,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     3b\n"

    "mov    -0x1(%edi),%al\n"
    "mov    %al,(%edi)\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"
);

// Bilinear row filtering combines 16x2 -> 16x1. SSSE3 version
#define HAS_SCALEFILTERROWS_SSSE3
extern "C" void ScaleFilterRows_SSSE3(uint8* optr,
                                      const uint8* iptr0, int src_stride,
                                      int dst_width, int source_y_fraction);
  asm(
    ".text\n"
#if defined(OSX)
    ".globl _ScaleFilterRows_SSSE3\n"
"_ScaleFilterRows_SSSE3:\n"
#else
    ".global ScaleFilterRows_SSSE3\n"
"ScaleFilterRows_SSSE3:\n"
#endif
    "push   %esi\n"
    "push   %edi\n"
    "mov    0xc(%esp),%edi\n"
    "mov    0x10(%esp),%esi\n"
    "mov    0x14(%esp),%edx\n"
    "mov    0x18(%esp),%ecx\n"
    "mov    0x1c(%esp),%eax\n"
    "cmp    $0x0,%eax\n"
    "je     2f\n"
    "cmp    $0x80,%eax\n"
    "je     3f\n"
    "shr    %eax\n"
    "mov    %al,%ah\n"
    "neg    %al\n"
    "add    $0x80,%al\n"
    "movd   %eax,%xmm7\n"
    "punpcklwd %xmm7,%xmm7\n"
    "pshufd $0x0,%xmm7,%xmm7\n"

"1:"
    "movdqa (%esi),%xmm0\n"
    "movdqa (%esi,%edx,1),%xmm2\n"
    "lea    0x10(%esi),%esi\n"
    "movdqa %xmm0,%xmm1\n"
    "punpcklbw %xmm2,%xmm0\n"
    "punpckhbw %xmm2,%xmm1\n"
    "pmaddubsw %xmm7,%xmm0\n"
    "pmaddubsw %xmm7,%xmm1\n"
    "psrlw  $0x7,%xmm0\n"
    "psrlw  $0x7,%xmm1\n"
    "packuswb %xmm1,%xmm0\n"
    "movdqa %xmm0,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     1b\n"
    "mov    -0x1(%edi),%al\n"
    "mov    %al,(%edi)\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"

"2:"
    "movdqa (%esi),%xmm0\n"
    "lea    0x10(%esi),%esi\n"
    "movdqa %xmm0,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     2b\n"
    "mov    -0x1(%edi),%al\n"
    "mov    %al,(%edi)\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"

"3:"
    "movdqa (%esi),%xmm0\n"
    "movdqa (%esi,%edx,1),%xmm2\n"
    "lea    0x10(%esi),%esi\n"
    "pavgb  %xmm2,%xmm0\n"
    "movdqa %xmm0,(%edi)\n"
    "lea    0x10(%edi),%edi\n"
    "sub    $0x10,%ecx\n"
    "ja     3b\n"
    "mov    -0x1(%edi),%al\n"
    "mov    %al,(%edi)\n"
    "pop    %edi\n"
    "pop    %esi\n"
    "ret\n"
);
#endif

// CPU agnostic row functions
static void ScaleRowDown2_C(const uint8* src_ptr, int,
                            uint8* dst, int dst_width) {
  for (int x = 0; x < dst_width; ++x) {
    *dst++ = *src_ptr;
    src_ptr += 2;
  }
}

static void ScaleRowDown2Int_C(const uint8* src_ptr, int src_stride,
                               uint8* dst, int dst_width) {
  for (int x = 0; x < dst_width; ++x) {
    *dst++ = (src_ptr[0] + src_ptr[1] +
              src_ptr[src_stride] + src_ptr[src_stride + 1] + 2) >> 2;
    src_ptr += 2;
  }
}

static void ScaleRowDown4_C(const uint8* src_ptr, int,
                            uint8* dst, int dst_width) {
  for (int x = 0; x < dst_width; ++x) {
    *dst++ = *src_ptr;
    src_ptr += 4;
  }
}

static void ScaleRowDown4Int_C(const uint8* src_ptr, int src_stride,
                               uint8* dst, int dst_width) {
  for (int x = 0; x < dst_width; ++x) {
    *dst++ = (src_ptr[0] + src_ptr[1] + src_ptr[2] + src_ptr[3] +
              src_ptr[src_stride + 0] + src_ptr[src_stride + 1] +
              src_ptr[src_stride + 2] + src_ptr[src_stride + 3] +
              src_ptr[src_stride * 2 + 0] + src_ptr[src_stride * 2 + 1] +
              src_ptr[src_stride * 2 + 2] + src_ptr[src_stride * 2 + 3] +
              src_ptr[src_stride * 3 + 0] + src_ptr[src_stride * 3 + 1] +
              src_ptr[src_stride * 3 + 2] + src_ptr[src_stride * 3 + 3] + 8) >> 4;
    src_ptr += 4;
  }
}

// 640 output pixels is enough to allow 5120 input pixels with 1/8 scale down.
// Keeping the total buffer under 4096 bytes avoids a stackcheck, saving 4% cpu.
static const int kMaxOutputWidth = 640;
static const int kMaxRow12 = kMaxOutputWidth * 2;

static void ScaleRowDown8_C(const uint8* src_ptr, int,
                            uint8* dst, int dst_width) {
  for (int x = 0; x < dst_width; ++x) {
    *dst++ = *src_ptr;
    src_ptr += 8;
  }
}

// Note calling code checks width is less than max and if not
// uses ScaleRowDown8_C instead.
static void ScaleRowDown8Int_C(const uint8* src_ptr, int src_stride,
                               uint8* dst, int dst_width) {
  ALIGN16(uint8 irow[kMaxRow12 * 2]);
  assert(dst_width <= kMaxOutputWidth);
  ScaleRowDown4Int_C(src_ptr, src_stride, irow, dst_width * 2);
  ScaleRowDown4Int_C(src_ptr + src_stride * 4, src_stride, irow + kMaxOutputWidth,
                     dst_width * 2);
  ScaleRowDown2Int_C(irow, kMaxOutputWidth, dst, dst_width);
}

static void ScaleRowDown34_C(const uint8* src_ptr, int,
                             uint8* dst, int dst_width) {
  assert((dst_width % 3 == 0) && (dst_width > 0));
  uint8* dend = dst + dst_width;
  do {
    dst[0] = src_ptr[0];
    dst[1] = src_ptr[1];
    dst[2] = src_ptr[3];
    dst += 3;
    src_ptr += 4;
  } while (dst < dend);
}

// Filter rows 0 and 1 together, 3 : 1
static void ScaleRowDown34_0_Int_C(const uint8* src_ptr, int src_stride,
                                   uint8* d, int dst_width) {
  assert((dst_width % 3 == 0) && (dst_width > 0));
  uint8* dend = d + dst_width;
  const uint8* s = src_ptr;
  const uint8* t = src_ptr + src_stride;
  do {
    uint8 a0 = (s[0] * 3 + s[1] * 1 + 2) >> 2;
    uint8 a1 = (s[1] * 1 + s[2] * 1 + 1) >> 1;
    uint8 a2 = (s[2] * 1 + s[3] * 3 + 2) >> 2;
    uint8 b0 = (t[0] * 3 + t[1] * 1 + 2) >> 2;
    uint8 b1 = (t[1] * 1 + t[2] * 1 + 1) >> 1;
    uint8 b2 = (t[2] * 1 + t[3] * 3 + 2) >> 2;
    d[0] = (a0 * 3 + b0 + 2) >> 2;
    d[1] = (a1 * 3 + b1 + 2) >> 2;
    d[2] = (a2 * 3 + b2 + 2) >> 2;
    d += 3;
    s += 4;
    t += 4;
  } while (d < dend);
}

// Filter rows 1 and 2 together, 1 : 1
static void ScaleRowDown34_1_Int_C(const uint8* src_ptr, int src_stride,
                                   uint8* d, int dst_width) {
  assert((dst_width % 3 == 0) && (dst_width > 0));
  uint8* dend = d + dst_width;
  const uint8* s = src_ptr;
  const uint8* t = src_ptr + src_stride;
  do {
    uint8 a0 = (s[0] * 3 + s[1] * 1 + 2) >> 2;
    uint8 a1 = (s[1] * 1 + s[2] * 1 + 1) >> 1;
    uint8 a2 = (s[2] * 1 + s[3] * 3 + 2) >> 2;
    uint8 b0 = (t[0] * 3 + t[1] * 1 + 2) >> 2;
    uint8 b1 = (t[1] * 1 + t[2] * 1 + 1) >> 1;
    uint8 b2 = (t[2] * 1 + t[3] * 3 + 2) >> 2;
    d[0] = (a0 + b0 + 1) >> 1;
    d[1] = (a1 + b1 + 1) >> 1;
    d[2] = (a2 + b2 + 1) >> 1;
    d += 3;
    s += 4;
    t += 4;
  } while (d < dend);
}

#if defined(HAS_SCALEFILTERROWS_SSE2)
// Filter row to 3/4
static void ScaleFilterCols34_C(uint8* optr, const uint8* src_ptr, int dst_width) {
  assert((dst_width % 3 == 0) && (dst_width > 0));
  uint8* dend = optr + dst_width;
  const uint8* s = src_ptr;
  do {
    optr[0] = (s[0] * 3 + s[1] * 1 + 2) >> 2;
    optr[1] = (s[1] * 1 + s[2] * 1 + 1) >> 1;
    optr[2] = (s[2] * 1 + s[3] * 3 + 2) >> 2;
    optr += 3;
    s += 4;
  } while (optr < dend);
}
#endif

static void ScaleFilterCols_C(uint8* optr, const uint8* src_ptr,
                              int dst_width, int dx) {
  int x = 0;
  for (int j = 0; j < dst_width; ++j) {
    int xi = x >> 16;
    int xf1 = x & 0xffff;
    int xf0 = 65536 - xf1;

    *optr++ = (src_ptr[xi] * xf0 + src_ptr[xi + 1] * xf1) >> 16;
    x += dx;
  }
}

static const int kMaxInputWidth = 2560;
#if defined(HAS_SCALEFILTERROWS_SSE2)
#define HAS_SCALEROWDOWN34_SSE2
// Filter rows 0 and 1 together, 3 : 1
static void ScaleRowDown34_0_Int_SSE2(const uint8* src_ptr, int src_stride,
                                      uint8* d, int dst_width) {
  assert((dst_width % 3 == 0) && (dst_width > 0));
  ALIGN16(uint8 row[kMaxInputWidth]);
  ScaleFilterRows_SSE2(row, src_ptr, src_stride, dst_width * 4 / 3, 256 / 4);
  ScaleFilterCols34_C(d, row, dst_width);
}

// Filter rows 1 and 2 together, 1 : 1
static void ScaleRowDown34_1_Int_SSE2(const uint8* src_ptr, int src_stride,
                                      uint8* d, int dst_width) {
  assert((dst_width % 3 == 0) && (dst_width > 0));
  ALIGN16(uint8 row[kMaxInputWidth]);
  ScaleFilterRows_SSE2(row, src_ptr, src_stride, dst_width * 4 / 3, 256 / 2);
  ScaleFilterCols34_C(d, row, dst_width);
}
#endif

static void ScaleRowDown38_C(const uint8* src_ptr, int,
                             uint8* dst, int dst_width) {
  assert(dst_width % 3 == 0);
  for (int x = 0; x < dst_width; x += 3) {
    dst[0] = src_ptr[0];
    dst[1] = src_ptr[3];
    dst[2] = src_ptr[6];
    dst += 3;
    src_ptr += 8;
  }
}

// 8x3 -> 3x1
static void ScaleRowDown38_3_Int_C(const uint8* src_ptr, int src_stride,
                                   uint8* optr, int dst_width) {
  assert((dst_width % 3 == 0) && (dst_width > 0));
  for (int i = 0; i < dst_width; i+=3) {
    optr[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] +
        src_ptr[src_stride + 0] + src_ptr[src_stride + 1] + src_ptr[src_stride + 2] +
        src_ptr[src_stride * 2 + 0] + src_ptr[src_stride * 2 + 1] + src_ptr[src_stride * 2 + 2]) *
        (65536 / 9) >> 16;
    optr[1] = (src_ptr[3] + src_ptr[4] + src_ptr[5] +
        src_ptr[src_stride + 3] + src_ptr[src_stride + 4] + src_ptr[src_stride + 5] +
        src_ptr[src_stride * 2 + 3] + src_ptr[src_stride * 2 + 4] + src_ptr[src_stride * 2 + 5]) *
        (65536 / 9) >> 16;
    optr[2] = (src_ptr[6] + src_ptr[7] +
        src_ptr[src_stride + 6] + src_ptr[src_stride + 7] +
        src_ptr[src_stride * 2 + 6] + src_ptr[src_stride * 2 + 7]) *
        (65536 / 6) >> 16;
    src_ptr += 8;
    optr += 3;
  }
}

// 8x2 -> 3x1
static void ScaleRowDown38_2_Int_C(const uint8* src_ptr, int src_stride,
                                   uint8* optr, int dst_width) {
  assert((dst_width % 3 == 0) && (dst_width > 0));
  for (int i = 0; i < dst_width; i+=3) {
    optr[0] = (src_ptr[0] + src_ptr[1] + src_ptr[2] +
        src_ptr[src_stride + 0] + src_ptr[src_stride + 1] + src_ptr[src_stride + 2]) *
        (65536 / 6) >> 16;
    optr[1] = (src_ptr[3] + src_ptr[4] + src_ptr[5] +
        src_ptr[src_stride + 3] + src_ptr[src_stride + 4] + src_ptr[src_stride + 5]) *
        (65536 / 6) >> 16;
    optr[2] = (src_ptr[6] + src_ptr[7] +
        src_ptr[src_stride + 6] + src_ptr[src_stride + 7]) *
        (65536 / 4) >> 16;
    src_ptr += 8;
    optr += 3;
  }
}

// C version 8x2 -> 8x1
static void ScaleFilterRows_C(uint8* optr,
                              const uint8* iptr0, int src_stride,
                              int dst_width, int source_y_fraction) {
  assert(dst_width > 0);
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint8* iptr1 = iptr0 + src_stride;
  uint8* end = optr + dst_width;
  do {
    optr[0] = (iptr0[0] * y0_fraction + iptr1[0] * y1_fraction) >> 8;
    optr[1] = (iptr0[1] * y0_fraction + iptr1[1] * y1_fraction) >> 8;
    optr[2] = (iptr0[2] * y0_fraction + iptr1[2] * y1_fraction) >> 8;
    optr[3] = (iptr0[3] * y0_fraction + iptr1[3] * y1_fraction) >> 8;
    optr[4] = (iptr0[4] * y0_fraction + iptr1[4] * y1_fraction) >> 8;
    optr[5] = (iptr0[5] * y0_fraction + iptr1[5] * y1_fraction) >> 8;
    optr[6] = (iptr0[6] * y0_fraction + iptr1[6] * y1_fraction) >> 8;
    optr[7] = (iptr0[7] * y0_fraction + iptr1[7] * y1_fraction) >> 8;
    iptr0 += 8;
    iptr1 += 8;
    optr += 8;
  } while (optr < end);
  optr[0] = optr[-1];
}

void ScaleAddRows_C(const uint8* src_ptr, int src_stride,
                    uint16* orow, int src_width, int src_height) {
  assert(src_width > 0);
  assert(src_height > 0);
  for (int x = 0; x < src_width; ++x) {
    const uint8* s = src_ptr + x;
    int sum = 0;
    for (int y = 0; y < src_height; ++y) {
      sum += s[0];
      s += src_stride;
    }
    orow[x] = sum;
  }
}

/**
 * Scale plane, 1/2
 *
 * This is an optimized version for scaling down a plane to 1/2 of
 * its original size.
 *
 */
static void ScalePlaneDown2(int src_width, int src_height,
                            int dst_width, int dst_height,
                            int src_stride, int ostride,
                            const uint8* src_ptr, uint8* optr,
                            FilterMode filtering) {
  assert(src_width % 2 == 0);
  assert(src_height % 2 == 0);
  void (*ScaleRowDown2)(const uint8* src_ptr, int src_stride,
                        uint8* orow, int dst_width);

#if defined(HAS_SCALEROWDOWN2_NEON)
  if (libyuv::TestCpuFlag(libyuv::kCpuHasNEON) &&
      (dst_width % 16 == 0) && (src_stride % 16 == 0) && (ostride % 16 == 0) &&
      IS_ALIGNED(src_ptr, 16) && IS_ALIGNED(optr, 16)) {
    ScaleRowDown2 = filtering ? ScaleRowDown2Int_NEON : ScaleRowDown2_NEON;
  } else
#endif
#if defined(HAS_SCALEROWDOWN2_SSE2)
  if (libyuv::TestCpuFlag(libyuv::kCpuHasSSE2) &&
      (dst_width % 16 == 0) && IS_ALIGNED(src_ptr, 16) && IS_ALIGNED(optr, 16)) {
    ScaleRowDown2 = filtering ? ScaleRowDown2Int_SSE2 : ScaleRowDown2_SSE2;
  } else
#endif
  {
    ScaleRowDown2 = filtering ? ScaleRowDown2Int_C : ScaleRowDown2_C;
  }

  for (int y = 0; y < dst_height; ++y) {
    ScaleRowDown2(src_ptr, src_stride, optr, dst_width);
    src_ptr += (src_stride << 1);
    optr += ostride;
  }
}

/**
 * Scale plane, 1/4
 *
 * This is an optimized version for scaling down a plane to 1/4 of
 * its original size.
 */
static void ScalePlaneDown4(int src_width, int src_height,
                            int dst_width, int dst_height,
                            int src_stride, int ostride,
                            const uint8* src_ptr, uint8* optr,
                            FilterMode filtering) {
  assert(src_width % 4 == 0);
  assert(src_height % 4 == 0);
  void (*ScaleRowDown4)(const uint8* src_ptr, int src_stride,
                        uint8* orow, int dst_width);

#if defined(HAS_SCALEROWDOWN4_SSE2)
  if (libyuv::TestCpuFlag(libyuv::kCpuHasSSE2) &&
      (dst_width % 8 == 0) && (src_stride % 16 == 0) && (ostride % 8 == 0) &&
      IS_ALIGNED(src_ptr, 16) && IS_ALIGNED(optr, 8)) {
    ScaleRowDown4 = filtering ? ScaleRowDown4Int_SSE2 : ScaleRowDown4_SSE2;
  } else
#endif
  {
    ScaleRowDown4 = filtering ? ScaleRowDown4Int_C : ScaleRowDown4_C;
  }

  for (int y = 0; y < dst_height; ++y) {
    ScaleRowDown4(src_ptr, src_stride, optr, dst_width);
    src_ptr += (src_stride << 2);
    optr += ostride;
  }
}

/**
 * Scale plane, 1/8
 *
 * This is an optimized version for scaling down a plane to 1/8
 * of its original size.
 *
 */
static void ScalePlaneDown8(int src_width, int src_height,
                            int dst_width, int dst_height,
                            int src_stride, int ostride,
                            const uint8* src_ptr, uint8* optr,
                            FilterMode filtering) {
  assert(src_width % 8 == 0);
  assert(src_height % 8 == 0);
  void (*ScaleRowDown8)(const uint8* src_ptr, int src_stride,
                        uint8* orow, int dst_width);
#if defined(HAS_SCALEROWDOWN8_SSE2)
  if (libyuv::TestCpuFlag(libyuv::kCpuHasSSE2) &&
      (dst_width % 16 == 0) && dst_width <= kMaxOutputWidth &&
      (src_stride % 16 == 0) && (ostride % 16 == 0) &&
      IS_ALIGNED(src_ptr, 16) && IS_ALIGNED(optr, 16)) {
    ScaleRowDown8 = filtering ? ScaleRowDown8Int_SSE2 : ScaleRowDown8_SSE2;
  } else
#endif
  {
    ScaleRowDown8 = filtering && (dst_width <= kMaxOutputWidth) ?
        ScaleRowDown8Int_C : ScaleRowDown8_C;
  }
  for (int y = 0; y < dst_height; ++y) {
    ScaleRowDown8(src_ptr, src_stride, optr, dst_width);
    src_ptr += (src_stride << 3);
    optr += ostride;
  }
}

/**
 * Scale plane down, 3/4
 *
 * Provided by Frank Barchard (fbarchard@google.com)
 *
 */
static void ScalePlaneDown34(int src_width, int src_height,
                             int dst_width, int dst_height,
                             int src_stride, int ostride,
                             const uint8* src_ptr, uint8* optr,
                             FilterMode filtering) {
  assert(dst_width % 3 == 0);
  void (*ScaleRowDown34_0)(const uint8* src_ptr, int src_stride,
                           uint8* orow, int dst_width);
  void (*ScaleRowDown34_1)(const uint8* src_ptr, int src_stride,
                           uint8* orow, int dst_width);
#if defined(HAS_SCALEROWDOWN34_SSSE3)
  if (libyuv::TestCpuFlag(libyuv::kCpuHasSSSE3) &&
      (dst_width % 24 == 0) && (src_stride % 16 == 0) && (ostride % 8 == 0) &&
      IS_ALIGNED(src_ptr, 16) && IS_ALIGNED(optr, 8)) {
    if (!filtering) {
      ScaleRowDown34_0 = ScaleRowDown34_SSSE3;
      ScaleRowDown34_1 = ScaleRowDown34_SSSE3;
    } else {
      ScaleRowDown34_0 = ScaleRowDown34_0_Int_SSSE3;
      ScaleRowDown34_1 = ScaleRowDown34_1_Int_SSSE3;
    }
  } else
#endif
#if defined(HAS_SCALEROWDOWN34_SSE2)
  if (libyuv::TestCpuFlag(libyuv::kCpuHasSSE2) &&
      (dst_width % 24 == 0) && (src_stride % 16 == 0) && (ostride % 8 == 0) &&
      IS_ALIGNED(src_ptr, 16) && IS_ALIGNED(optr, 8) &&
      filtering) {
    ScaleRowDown34_0 = ScaleRowDown34_0_Int_SSE2;
    ScaleRowDown34_1 = ScaleRowDown34_1_Int_SSE2;
  } else
#endif
  {
    if (!filtering) {
      ScaleRowDown34_0 = ScaleRowDown34_C;
      ScaleRowDown34_1 = ScaleRowDown34_C;
    } else {
      ScaleRowDown34_0 = ScaleRowDown34_0_Int_C;
      ScaleRowDown34_1 = ScaleRowDown34_1_Int_C;
    }
  }
  int irow = 0;
  for (int y = 0; y < dst_height; ++y) {
    switch (irow) {
      case 0:
        ScaleRowDown34_0(src_ptr, src_stride, optr, dst_width);
        break;

      case 1:
        ScaleRowDown34_1(src_ptr, src_stride, optr, dst_width);
        break;

      case 2:
        ScaleRowDown34_0(src_ptr + src_stride, -src_stride, optr, dst_width);
        break;
    }
    ++irow;
    src_ptr += src_stride;
    optr += ostride;
    if (irow >= 3) {
      src_ptr += src_stride;
      irow = 0;
    }
  }
}

/**
 * Scale plane, 3/8
 *
 * This is an optimized version for scaling down a plane to 3/8
 * of its original size.
 *
 * Reduces 16x3 to 6x1
 */
static void ScalePlaneDown38(int src_width, int src_height,
                             int dst_width, int dst_height,
                             int src_stride, int ostride,
                             const uint8* src_ptr, uint8* optr,
                             FilterMode filtering) {
  assert(dst_width % 3 == 0);
  void (*ScaleRowDown38_3)(const uint8* src_ptr, int src_stride,
                           uint8* orow, int dst_width);
  void (*ScaleRowDown38_2)(const uint8* src_ptr, int src_stride,
                           uint8* orow, int dst_width);
#if defined(HAS_SCALEROWDOWN38_SSSE3)
  if (libyuv::TestCpuFlag(libyuv::kCpuHasSSSE3) &&
      (dst_width % 24 == 0) && (src_stride % 16 == 0) && (ostride % 8 == 0) &&
      IS_ALIGNED(src_ptr, 16) && IS_ALIGNED(optr, 8)) {
    if (!filtering) {
      ScaleRowDown38_3 = ScaleRowDown38_SSSE3;
      ScaleRowDown38_2 = ScaleRowDown38_SSSE3;
    } else {
      ScaleRowDown38_3 = ScaleRowDown38_3_Int_SSSE3;
      ScaleRowDown38_2 = ScaleRowDown38_2_Int_SSSE3;
    }
  } else
#endif
  {
    if (!filtering) {
      ScaleRowDown38_3 = ScaleRowDown38_C;
      ScaleRowDown38_2 = ScaleRowDown38_C;
    } else {
      ScaleRowDown38_3 = ScaleRowDown38_3_Int_C;
      ScaleRowDown38_2 = ScaleRowDown38_2_Int_C;
    }
  }
  int irow = 0;
  for (int y = 0; y < dst_height; ++y) {
    switch (irow) {
      case 0:
      case 1:
        ScaleRowDown38_3(src_ptr, src_stride, optr, dst_width);
        src_ptr += src_stride * 3;
        ++irow;
        break;

      case 2:
        ScaleRowDown38_2(src_ptr, src_stride, optr, dst_width);
        src_ptr += src_stride * 2;
        irow = 0;
        break;
    }
    optr += ostride;
  }
}

inline static uint32 SumBox(int iboxwidth, int iboxheight,
                            int src_stride, const uint8* src_ptr) {
  assert(iboxwidth > 0);
  assert(iboxheight > 0);
  uint32 sum = 0u;
  for (int y = 0; y < iboxheight; ++y) {
    for (int x = 0; x < iboxwidth; ++x) {
      sum += src_ptr[x];
    }
    src_ptr += src_stride;
  }
  return sum;
}

static void ScalePlaneBoxRow(int dst_width, int boxheight,
                             int dx, int src_stride,
                             const uint8* src_ptr, uint8* optr) {
  int x = 0;
  for (int i = 0; i < dst_width; ++i) {
    int ix = x >> 16;
    x += dx;
    int boxwidth = (x >> 16) - ix;
    *optr++ = SumBox(boxwidth, boxheight, src_stride, src_ptr + ix) /
        (boxwidth * boxheight);
  }
}

inline static uint32 SumPixels(int iboxwidth, const uint16* src_ptr) {
  assert(iboxwidth > 0);
  uint32 sum = 0u;
  for (int x = 0; x < iboxwidth; ++x) {
    sum += src_ptr[x];
  }
  return sum;
}

static void ScaleAddCols2_C(int dst_width, int boxheight, int dx,
                            const uint16* src_ptr, uint8* optr) {
  int scaletbl[2];
  int minboxwidth = (dx >> 16);
  scaletbl[0] = 65536 / (minboxwidth * boxheight);
  scaletbl[1] = 65536 / ((minboxwidth + 1) * boxheight);
  int *scaleptr = scaletbl - minboxwidth;
  int x = 0;
  for (int i = 0; i < dst_width; ++i) {
    int ix = x >> 16;
    x += dx;
    int boxwidth = (x >> 16) - ix;
    *optr++ = SumPixels(boxwidth, src_ptr + ix) * scaleptr[boxwidth] >> 16;
  }
}

static void ScaleAddCols1_C(int dst_width, int boxheight, int dx,
                            const uint16* src_ptr, uint8* optr) {
  int boxwidth = (dx >> 16);
  int scaleval = 65536 / (boxwidth * boxheight);
  int x = 0;
  for (int i = 0; i < dst_width; ++i) {
    *optr++ = SumPixels(boxwidth, src_ptr + x) * scaleval >> 16;
    x += boxwidth;
  }
}

/**
 * Scale plane down to any dimensions, with interpolation.
 * (boxfilter).
 *
 * Same method as SimpleScale, which is fixed point, outputting
 * one pixel of destination using fixed point (16.16) to step
 * through source, sampling a box of pixel with simple
 * averaging.
 */
static void ScalePlaneBox(int src_width, int src_height,
                          int dst_width, int dst_height,
                          int src_stride, int ostride,
                          const uint8* src_ptr, uint8* optr) {
  assert(dst_width > 0);
  assert(dst_height > 0);
  int dy = (src_height << 16) / dst_height;
  int dx = (src_width << 16) / dst_width;
  if ((src_width % 16 != 0) || (src_width > kMaxInputWidth) ||
      dst_height * 2 > src_height) {
    uint8* dst = optr;
    int dy = (src_height << 16) / dst_height;
    int dx = (src_width << 16) / dst_width;
    int y = 0;
    for (int j = 0; j < dst_height; ++j) {
      int iy = y >> 16;
      const uint8* const src = src_ptr + iy * src_stride;
      y += dy;
      if (y > (src_height << 16)) {
        y = (src_height << 16);
      }
      int boxheight = (y >> 16) - iy;
      ScalePlaneBoxRow(dst_width, boxheight,
                       dx, src_stride,
                       src, dst);

      dst += ostride;
    }
  } else {
    ALIGN16(uint16 row[kMaxInputWidth]);
    void (*ScaleAddRows)(const uint8* src_ptr, int src_stride,
                         uint16* orow, int src_width, int src_height);
    void (*ScaleAddCols)(int dst_width, int boxheight, int dx,
                         const uint16* src_ptr, uint8* optr);
#if defined(HAS_SCALEADDROWS_SSE2)
    if (libyuv::TestCpuFlag(libyuv::kCpuHasSSE2) &&
        (src_stride % 16 == 0) && IS_ALIGNED(src_ptr, 16) && (src_width % 16) == 0) {
      ScaleAddRows = ScaleAddRows_SSE2;
    } else
#endif
    {
      ScaleAddRows = ScaleAddRows_C;
    }
    if (dx & 0xffff) {
      ScaleAddCols = ScaleAddCols2_C;
    } else {
      ScaleAddCols = ScaleAddCols1_C;
    }

    int y = 0;
    for (int j = 0; j < dst_height; ++j) {
      int iy = y >> 16;
      const uint8* const src = src_ptr + iy * src_stride;
      y += dy;
      if (y > (src_height << 16)) {
        y = (src_height << 16);
      }
      int boxheight = (y >> 16) - iy;
      ScaleAddRows(src, src_stride, row, src_width, boxheight);
      ScaleAddCols(dst_width, boxheight, dx, row, optr);
      optr += ostride;
    }
  }
}

/**
 * Scale plane to/from any dimensions, with interpolation.
 */
static void ScalePlaneBilinearSimple(int src_width, int src_height,
                                     int dst_width, int dst_height,
                                     int src_stride, int ostride,
                                     const uint8* src_ptr, uint8* optr) {
  uint8* dst = optr;
  int dx = (src_width << 16) / dst_width;
  int dy = (src_height << 16) / dst_height;
  int maxx = ((src_width - 1) << 16) - 1;
  int maxy = ((src_height - 1) << 16) - 1;
  int y = (dst_height < src_height) ? 32768 : (src_height << 16) / dst_height - 32768;
  for (int i = 0; i < dst_height; ++i) {
    int cy = (y < 0) ? 0 : y;
    int yi = cy >> 16;
    int yf = cy & 0xffff;
    const uint8* const src = src_ptr + yi * src_stride;
    int x = (dst_width < src_width) ? 32768 : (src_width << 16) / dst_width - 32768;
    for (int j = 0; j < dst_width; ++j) {
      int cx = (x < 0) ? 0 : x;
      int xi = cx >> 16;
      int xf = cx & 0xffff;
      int r0 = (src[xi] * (65536 - xf) + src[xi + 1] * xf) >> 16;
      int r1 = (src[xi + src_stride] * (65536 - xf) + src[xi + src_stride + 1] * xf)
                >> 16;
      *dst++ = (r0 * (65536 - yf) + r1 * yf) >> 16;
      x += dx;
      if (x > maxx)
        x = maxx;
    }
    dst += ostride - dst_width;
    y += dy;
    if (y > maxy)
      y = maxy;
  }
}

/**
 * Scale plane to/from any dimensions, with bilinear
 * interpolation.
 */
static void ScalePlaneBilinear(int src_width, int src_height,
                               int dst_width, int dst_height,
                               int src_stride, int ostride,
                               const uint8* src_ptr, uint8* optr) {
  assert(dst_width > 0);
  assert(dst_height > 0);
  int dy = (src_height << 16) / dst_height;
  int dx = (src_width << 16) / dst_width;
  if ((src_width % 8 != 0) || (src_width > kMaxInputWidth)) {
    ScalePlaneBilinearSimple(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                             src_ptr, optr);

  } else {
    ALIGN16(uint8 row[kMaxInputWidth + 1]);
    void (*ScaleFilterRows)(uint8* optr, const uint8* iptr0, int src_stride,
                            int dst_width, int source_y_fraction);
    void (*ScaleFilterCols)(uint8* optr, const uint8* src_ptr,
                            int dst_width, int dx);
#if defined(HAS_SCALEFILTERROWS_SSSE3)
    if (libyuv::TestCpuFlag(libyuv::kCpuHasSSSE3) &&
        (src_stride % 16 == 0) && IS_ALIGNED(src_ptr, 16) && (src_width % 16) == 0) {
      ScaleFilterRows = ScaleFilterRows_SSSE3;
    } else
#endif
#if defined(HAS_SCALEFILTERROWS_SSE2)
    if (libyuv::TestCpuFlag(libyuv::kCpuHasSSE2) &&
        (src_stride % 16 == 0) && IS_ALIGNED(src_ptr, 16) && (src_width % 16) == 0) {
      ScaleFilterRows = ScaleFilterRows_SSE2;
    } else
#endif
    {
      ScaleFilterRows = ScaleFilterRows_C;
    }
    ScaleFilterCols = ScaleFilterCols_C;

    int y = 0;
    int maxy = ((src_height - 1) << 16) - 1; // max is filter of last 2 rows.
    for (int j = 0; j < dst_height; ++j) {
      int iy = y >> 16;
      int fy = (y >> 8) & 255;
      const uint8* const src = src_ptr + iy * src_stride;
      ScaleFilterRows(row, src, src_stride, src_width, fy);
      ScaleFilterCols(optr, row, dst_width, dx);
      optr += ostride;
      y += dy;
      if (y > maxy) {
        y = maxy;
      }
    }
  }
}

/**
 * Scale plane to/from any dimensions, without interpolation.
 * Fixed point math is used for performance: The upper 16 bits
 * of x and dx is the integer part of the source position and
 * the lower 16 bits are the fixed decimal part.
 */
static void ScalePlaneSimple(int src_width, int src_height,
                             int dst_width, int dst_height,
                             int src_stride, int ostride,
                             const uint8* src_ptr, uint8* optr) {
  uint8* dst = optr;
  int dx = (src_width << 16) / dst_width;
  for (int y = 0; y < dst_height; ++y) {
    const uint8* const src = src_ptr + (y * src_height / dst_height) * src_stride;
    // TODO(fbarchard): Round X coordinate by setting x=0x8000.
    int x = 0;
    for (int i = 0; i < dst_width; ++i) {
      *dst++ = src[x >> 16];
      x += dx;
    }
    dst += ostride - dst_width;
  }
}

/**
 * Scale plane to/from any dimensions.
 */
static void ScalePlaneAnySize(int src_width, int src_height,
                              int dst_width, int dst_height,
                              int src_stride, int ostride,
                              const uint8* src_ptr, uint8* optr,
                              FilterMode filtering) {
  if (!filtering) {
    ScalePlaneSimple(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                     src_ptr, optr);
  } else {
    // fall back to non-optimized version
    ScalePlaneBilinear(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                       src_ptr, optr);
  }
}

/**
 * Scale plane down, any size
 *
 * This is an optimized version for scaling down a plane to any size.
 * The current implementation is ~10 times faster compared to the
 * reference implementation for e.g. XGA->LowResPAL
 *
 */
static void ScalePlaneDown(int src_width, int src_height,
                           int dst_width, int dst_height,
                           int src_stride, int ostride,
                           const uint8* src_ptr, uint8* optr,
                           FilterMode filtering) {
  if (!filtering) {
    ScalePlaneSimple(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                     src_ptr, optr);
  } else if (filtering == kFilterBilinear || src_height * 2 > dst_height) {
    // between 1/2x and 1x use bilinear
    ScalePlaneBilinear(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                       src_ptr, optr);
  } else {
    ScalePlaneBox(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                  src_ptr, optr);
  }
}

/**
 * Copy plane, no scaling
 *
 * This simply copies the given plane without scaling.
 * The current implementation is ~115 times faster
 * compared to the reference implementation.
 *
 */
static void CopyPlane(int src_width, int src_height,
                      int dst_width, int dst_height,
                      int src_stride, int ostride,
                      const uint8* src_ptr, uint8* optr) {
  if (src_stride == src_width && ostride == dst_width) {
    // All contiguous, so can use REALLY fast path.
    memcpy(optr, src_ptr, src_width * src_height);
  } else {
    // Not all contiguous; must copy scanlines individually
    const uint8* src = src_ptr;
    uint8* dst = optr;
    for (int i = 0; i < src_height; ++i) {
      memcpy(dst, src, src_width);
      dst += ostride;
      src += src_stride;
    }
  }
}

static void ScalePlane(const uint8* src, int src_stride,
                       int src_width, int src_height,
                       uint8* dst, int ostride,
                       int dst_width, int dst_height,
                       FilterMode filtering, bool use_ref) {
  // Use specialized scales to improve performance for common resolutions.
  // For example, all the 1/2 scalings will use ScalePlaneDown2()
  if (dst_width == src_width && dst_height == src_height) {
    // Straight copy.
    CopyPlane(src_width, src_height, dst_width, dst_height, src_stride, ostride, src, dst);
  } else if (dst_width <= src_width && dst_height <= src_height) {
    // Scale down.
    if (use_ref) {
      // For testing, allow the optimized versions to be disabled.
      ScalePlaneDown(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                     src, dst, filtering);
    } else if (4 * dst_width == 3 * src_width && 4 * dst_height == 3 * src_height) {
      // optimized, 3/4
      ScalePlaneDown34(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                       src, dst, filtering);
    } else if (2 * dst_width == src_width && 2 * dst_height == src_height) {
      // optimized, 1/2
      ScalePlaneDown2(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                      src, dst, filtering);
    // 3/8 rounded up for odd sized chroma height.
    } else if (8 * dst_width == 3 * src_width && dst_height == ((src_height * 3 + 7) / 8)) {
      // optimized, 3/8
      ScalePlaneDown38(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                       src, dst, filtering);
    } else if (4 * dst_width == src_width && 4 * dst_height == src_height) {
      // optimized, 1/4
      ScalePlaneDown4(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                      src, dst, filtering);
    } else if (8 * dst_width == src_width && 8 * dst_height == src_height) {
      // optimized, 1/8
      ScalePlaneDown8(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                      src, dst, filtering);
    } else {
      // Arbitrary downsample
      ScalePlaneDown(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                     src, dst, filtering);
    }
  } else {
    // Arbitrary scale up and/or down.
    ScalePlaneAnySize(src_width, src_height, dst_width, dst_height, src_stride, ostride,
                      src, dst, filtering);
  }
}

/**
 * Scale a plane.
 *
 * This function in turn calls a scaling function
 * suitable for handling the desired resolutions.
 *
 */

int I420Scale(const uint8* src_y, int src_stride_y,
              const uint8* src_u, int src_stride_u,
              const uint8* src_v, int src_stride_v,
              int src_width, int src_height,
              uint8* dst_y, int dst_stride_y,
              uint8* dst_u, int dst_stride_u,
              uint8* dst_v, int dst_stride_v,
              int dst_width, int dst_height,
              FilterMode filtering) {
  if (!src_y || !src_u || !src_v || src_width <= 0 || src_height <= 0 ||
      !dst_y || !dst_u || !dst_v || dst_width <= 0 || dst_height <= 0) {
    return -1;
  }
  int halfiwidth = (src_width + 1) >> 1;
  int halfiheight = (src_height + 1) >> 1;
  int halfowidth = (dst_width + 1) >> 1;
  int halfoheight = (dst_height + 1) >> 1;

  ScalePlane(src_y, src_stride_y, src_width, src_height,
             dst_y, dst_stride_y, dst_width, dst_height,
             filtering, use_reference_impl_);
  ScalePlane(src_u, src_stride_u, halfiwidth, halfiheight,
             dst_u, dst_stride_u, halfowidth, halfoheight,
             filtering, use_reference_impl_);
  ScalePlane(src_v, src_stride_v, halfiwidth, halfiheight,
             dst_v, dst_stride_v, halfowidth, halfoheight,
             filtering, use_reference_impl_);
  return 0;
}

int Scale(const uint8* src_y, const uint8* src_u, const uint8* src_v,
          int src_stride_y, int src_stride_u, int src_stride_v,
          int src_width, int src_height,
          uint8* dst_y, uint8* dst_u, uint8* dst_v,
          int dst_stride_y, int dst_stride_u, int dst_stride_v,
          int dst_width, int dst_height,
          bool interpolate) {
  if (!src_y || !src_u || !src_v || src_width <= 0 || src_height <= 0 ||
      !dst_y || !dst_u || !dst_v || dst_width <= 0 || dst_height <= 0) {
    return -1;
  }
  int halfiwidth = (src_width + 1) >> 1;
  int halfiheight = (src_height + 1) >> 1;
  int halfowidth = (dst_width + 1) >> 1;
  int halfoheight = (dst_height + 1) >> 1;
  FilterMode filtering = interpolate ? kFilterBox : kFilterNone;

  ScalePlane(src_y, src_stride_y, src_width, src_height,
             dst_y, dst_stride_y, dst_width, dst_height,
             filtering, use_reference_impl_);
  ScalePlane(src_u, src_stride_u, halfiwidth, halfiheight,
             dst_u, dst_stride_u, halfowidth, halfoheight,
             filtering, use_reference_impl_);
  ScalePlane(src_v, src_stride_v, halfiwidth, halfiheight,
             dst_v, dst_stride_v, halfowidth, halfoheight,
             filtering, use_reference_impl_);
  return 0;
}

int Scale(const uint8* src, int src_width, int src_height,
          uint8* dst, int dst_width, int dst_height, int ooffset,
          bool interpolate) {
  if (!src || src_width <= 0 || src_height <= 0 ||
      !dst || dst_width <= 0 || dst_height <= 0 || ooffset < 0 ||
      ooffset >= dst_height) {
    return -1;
  }
  ooffset = ooffset & ~1;  // chroma requires offset to multiple of 2.
  int halfiwidth = (src_width + 1) >> 1;
  int halfiheight = (src_height + 1) >> 1;
  int halfowidth = (dst_width + 1) >> 1;
  int halfoheight = (dst_height + 1) >> 1;
  int aheight = dst_height - ooffset * 2;  // actual output height
  const uint8* const iyptr = src;
  uint8* oyptr = dst + ooffset * dst_width;
  const uint8* const iuptr = src + src_width * src_height;
  uint8* ouptr = dst + dst_width * dst_height + (ooffset >> 1) * halfowidth;
  const uint8* const ivptr = src + src_width * src_height +
                             halfiwidth * halfiheight;
  uint8* ovptr = dst + dst_width * dst_height + halfowidth * halfoheight +
                 (ooffset >> 1) * halfowidth;
  return Scale(iyptr, iuptr, ivptr, src_width, halfiwidth, halfiwidth,
               src_width, src_height, oyptr, ouptr, ovptr, dst_width,
               halfowidth, halfowidth, dst_width, aheight, interpolate);
}

}  // namespace libyuv

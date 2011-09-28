/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "scale.h"

#include <string.h>
#include "common.h"

#include "cpu_id.h"

// Note: A Neon reference manual
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0204j/CJAJIIGG.html
// Note: Some SSE2 reference manuals
// cpuvol1.pdf agner_instruction_tables.pdf 253666.pdf 253667.pdf

// TODO(fbarchard): Remove once performance is known
//#define TEST_RSTSC

#if defined(TEST_RSTSC)
#include <iomanip>
#include <iostream>
#ifdef _MSC_VER
#include <emmintrin.h>
#endif

#if defined(__GNUC__) && defined(__i386__)
static inline uint64 __rdtsc(void) {
  uint32_t a, d;
  __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));
  return ((uint64)d << 32) + a;
}
#endif
#endif

namespace libyuv {

// Set the following flag to true to revert to only
// using the reference implementation ScalePlaneBox(), and
// NOT the optimized versions. Useful for debugging and
// when comparing the quality of the resulting YUV planes
// as produced by the optimized and non-optimized versions.
bool YuvScaler::use_reference_impl_ = false;


/**
 * NEON downscalers with interpolation.
 *
 * Provided by Fritz Koenig
 *
 */

#if defined(__ARM_NEON__) && !defined(COVERAGE_ENABLED)
#define HAS_SCALEROWDOWN2_NEON
void ScaleRowDown2_NEON(const uint8* iptr, int32 /* istride */,
                        uint8* dst, int32 owidth) {
  __asm__ volatile
  (
    "1:\n"
    "vld2.u8    {q0,q1}, [%0]!    \n"  // load even pixels into q0, odd into q1
    "vst1.u8    {q0}, [%1]!       \n"  // store even pixels
    "subs       %2, %2, #16       \n"  // 16 processed per loop
    "bhi        1b                \n"
    :                                    // Output registers
    : "r"(iptr), "r"(dst), "r"(owidth)   // Input registers
    : "r4", "q0", "q1"                   // Clobber List
  );
}

void ScaleRowDown2Int_NEON(const uint8* iptr, int32 istride,
                           uint8* dst, int32 owidth) {
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
    : "r"(iptr), "r"(istride), "r"(dst), "r"(owidth)  // Input registers
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
// Alignment requirement: iptr 16 byte aligned, optr 16 byte aligned.
__declspec(naked)
static void ScaleRowDown2_SSE2(const uint8* iptr, int32 istride,
                               uint8* optr, int32 owidth) {
  __asm {
    mov        eax, [esp + 4]        // iptr
                                     // istride ignored
    mov        edx, [esp + 12]       // optr
    mov        ecx, [esp + 16]       // owidth
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
// Alignment requirement: iptr 16 byte aligned, optr 16 byte aligned.
__declspec(naked)
static void ScaleRowDown2Int_SSE2(const uint8* iptr, int32 istride,
                                  uint8* optr, int32 owidth) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    // iptr
    mov        esi, [esp + 4 + 8]    // istride
    mov        edx, [esp + 4 + 12]   // optr
    mov        ecx, [esp + 4 + 16]   // owidth
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
// Alignment requirement: iptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown4_SSE2(const uint8* iptr, int32 istride,
                               uint8* orow, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
                                     // istride ignored
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // owidth
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
// Alignment requirement: iptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown4Int_SSE2(const uint8* iptr, int32 istride,
                                  uint8* orow, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
    mov        ebx, [esp + 32 + 8]   // istride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // owidth
    pcmpeqb    xmm7, xmm7            // generate mask 0x00ff00ff
    psrlw      xmm7, 8
    lea        edx, [ebx + ebx * 2]  // istride * 3

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
// Alignment requirement: iptr 16 byte aligned, optr 4 byte aligned.
__declspec(naked)
static void ScaleRowDown8_SSE2(const uint8* iptr, int32 istride,
                               uint8* orow, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
                                     // istride ignored
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // owidth
    pcmpeqb    xmm7, xmm7            // generate mask isolating 1 in 8 bytes
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
// Alignment requirement: iptr 16 byte aligned, optr 4 byte aligned.
__declspec(naked)
static void ScaleRowDown8Int_SSE2(const uint8* iptr, int32 istride,
                                  uint8* orow, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
    mov        ebx, [esp + 32 + 8]   // istride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // owidth
    lea        edx, [ebx + ebx * 2]  // istride * 3
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
// Alignment requirement: iptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown34_SSSE3(const uint8* iptr, int32 istride,
                                 uint8* orow, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
                                     // istride ignored
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // owidth
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
// Alignment requirement: iptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown34_1_Int_SSSE3(const uint8* iptr, int32 istride,
                                       uint8* orow, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
    mov        ebx, [esp + 32 + 8]   // istride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // owidth
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
// Alignment requirement: iptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleRowDown34_0_Int_SSSE3(const uint8* iptr, int32 istride,
                                       uint8* orow, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
    mov        ebx, [esp + 32 + 8]   // istride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // owidth
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
static void ScaleRowDown38_SSSE3(const uint8* iptr, int32 istride,
                                 uint8* optr, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
    mov        edx, [esp + 32 + 8]   // istride
    mov        edi, [esp + 32 + 12]  // optr
    mov        ecx, [esp + 32 + 16]  // owidth
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
static void ScaleRowDown38_3_Int_SSSE3(const uint8* iptr, int32 istride,
                                       uint8* optr, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
    mov        edx, [esp + 32 + 8]   // istride
    mov        edi, [esp + 32 + 12]  // optr
    mov        ecx, [esp + 32 + 16]  // owidth
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
static void ScaleRowDown38_2_Int_SSSE3(const uint8* iptr, int32 istride,
                                       uint8* optr, int32 owidth) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
    mov        edx, [esp + 32 + 8]   // istride
    mov        edi, [esp + 32 + 12]  // optr
    mov        ecx, [esp + 32 + 16]  // owidth
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
static void ScaleAddRows_SSE2(const uint8* iptr, int32 istride,
                              uint16* orow, int32 iwidth, int32 iheight) {
  __asm {
    pushad
    mov        esi, [esp + 32 + 4]   // iptr
    mov        edx, [esp + 32 + 8]   // istride
    mov        edi, [esp + 32 + 12]  // orow
    mov        ecx, [esp + 32 + 16]  // owidth
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
static void ScaleFilterRows_SSE2(uint8* optr, const uint8* iptr0, int32 istride,
                                 int owidth, int source_y_fraction) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]   // optr
    mov        esi, [esp + 8 + 8]   // iptr0
    mov        edx, [esp + 8 + 12]  // istride
    mov        ecx, [esp + 8 + 16]  // owidth
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
static void ScaleFilterRows_SSSE3(uint8* optr, const uint8* iptr0, int32 istride,
                                  int owidth, int source_y_fraction) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]   // optr
    mov        esi, [esp + 8 + 8]   // iptr0
    mov        edx, [esp + 8 + 12]  // istride
    mov        ecx, [esp + 8 + 16]  // owidth
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
// Alignment requirement: iptr 16 byte aligned, optr 8 byte aligned.
__declspec(naked)
static void ScaleFilterCols34_SSSE3(uint8* optr, const uint8* iptr,
                                    int owidth) {
  __asm {
    mov        edx, [esp + 4]    // optr
    mov        eax, [esp + 8]    // iptr
    mov        ecx, [esp + 12]   // owidth
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
extern "C" void ScaleRowDown2_SSE2(const uint8* iptr, int32 istride,
                                   uint8* orow, int32 owidth);
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

extern "C" void ScaleRowDown2Int_SSE2(const uint8* iptr, int32 istride,
                                      uint8* orow, int32 owidth);
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
extern "C" void ScaleRowDown4_SSE2(const uint8* iptr, int32 istride,
                                   uint8* orow, int32 owidth);
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

extern "C" void ScaleRowDown4Int_SSE2(const uint8* iptr, int32 istride,
                                      uint8* orow, int32 owidth);
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
extern "C" void ScaleRowDown8_SSE2(const uint8* iptr, int32 istride,
                                   uint8* orow, int32 owidth);
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

extern "C" void ScaleRowDown8Int_SSE2(const uint8* iptr, int32 istride,
                                      uint8* orow, int32 owidth);
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
extern "C" void ScaleRowDown34_SSSE3(const uint8* iptr, int32 istride,
                                     uint8* orow, int32 owidth);
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

extern "C" void ScaleRowDown34_1_Int_SSSE3(const uint8* iptr, int32 istride,
                                           uint8* orow, int32 owidth);
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

extern "C" void ScaleRowDown34_0_Int_SSSE3(const uint8* iptr, int32 istride,
                                           uint8* orow, int32 owidth);
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
extern "C" void ScaleRowDown38_SSSE3(const uint8* iptr, int32 istride,
                                     uint8* optr, int32 owidth);
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

extern "C" void ScaleRowDown38_3_Int_SSSE3(const uint8* iptr, int32 istride,
                                           uint8* optr, int32 owidth);
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

extern "C" void ScaleRowDown38_2_Int_SSSE3(const uint8* iptr, int32 istride,
                                           uint8* optr, int32 owidth);
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
extern "C" void ScaleAddRows_SSE2(const uint8* iptr, int32 istride,
                                  uint16* orow, int32 iwidth, int32 iheight);
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
                                     const uint8* iptr0, int32 istride,
                                     int owidth, int source_y_fraction);
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
                                      const uint8* iptr0, int32 istride,
                                      int owidth, int source_y_fraction);
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
static void ScaleRowDown2_C(const uint8* iptr, int32,
                            uint8* dst, int32 owidth) {
  for (int x = 0; x < owidth; ++x) {
    *dst++ = *iptr;
    iptr += 2;
  }
}

static void ScaleRowDown2Int_C(const uint8* iptr, int32 istride,
                               uint8* dst, int32 owidth) {
  for (int x = 0; x < owidth; ++x) {
    *dst++ = (iptr[0] + iptr[1] +
              iptr[istride] + iptr[istride + 1] + 2) >> 2;
    iptr += 2;
  }
}

static void ScaleRowDown4_C(const uint8* iptr, int32,
                            uint8* dst, int32 owidth) {
  for (int x = 0; x < owidth; ++x) {
    *dst++ = *iptr;
    iptr += 4;
  }
}

static void ScaleRowDown4Int_C(const uint8* iptr, int32 istride,
                               uint8* dst, int32 owidth) {
  for (int x = 0; x < owidth; ++x) {
    *dst++ = (iptr[0] + iptr[1] + iptr[2] + iptr[3] +
              iptr[istride + 0] + iptr[istride + 1] +
              iptr[istride + 2] + iptr[istride + 3] +
              iptr[istride * 2 + 0] + iptr[istride * 2 + 1] +
              iptr[istride * 2 + 2] + iptr[istride * 2 + 3] +
              iptr[istride * 3 + 0] + iptr[istride * 3 + 1] +
              iptr[istride * 3 + 2] + iptr[istride * 3 + 3] + 8) >> 4;
    iptr += 4;
  }
}

// 640 output pixels is enough to allow 5120 input pixels with 1/8 scale down.
// Keeping the total buffer under 4096 bytes avoids a stackcheck, saving 4% cpu.
static const int kMaxOutputWidth = 640;
static const int kMaxRow12 = kMaxOutputWidth * 2;

static void ScaleRowDown8_C(const uint8* iptr, int32,
                            uint8* dst, int32 owidth) {
  for (int x = 0; x < owidth; ++x) {
    *dst++ = *iptr;
    iptr += 8;
  }
}

// Note calling code checks width is less than max and if not
// uses ScaleRowDown8_C instead.
static void ScaleRowDown8Int_C(const uint8* iptr, int32 istride,
                               uint8* dst, int32 owidth) {
  ALIGN16(uint8 irow[kMaxRow12 * 2]);
  ASSERT(owidth <= kMaxOutputWidth);
  ScaleRowDown4Int_C(iptr, istride, irow, owidth * 2);
  ScaleRowDown4Int_C(iptr + istride * 4, istride, irow + kMaxOutputWidth,
                     owidth * 2);
  ScaleRowDown2Int_C(irow, kMaxOutputWidth, dst, owidth);
}

static void ScaleRowDown34_C(const uint8* iptr, int32,
                             uint8* dst, int32 owidth) {
  ASSERT((owidth % 3 == 0) && (owidth > 0));
  uint8* dend = dst + owidth;
  do {
    dst[0] = iptr[0];
    dst[1] = iptr[1];
    dst[2] = iptr[3];
    dst += 3;
    iptr += 4;
  } while (dst < dend);
}

// Filter rows 0 and 1 together, 3 : 1
static void ScaleRowDown34_0_Int_C(const uint8* iptr, int32 istride,
                                   uint8* d, int32 owidth) {
  ASSERT((owidth % 3 == 0) && (owidth > 0));
  uint8* dend = d + owidth;
  const uint8* s = iptr;
  const uint8* t = iptr + istride;
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
static void ScaleRowDown34_1_Int_C(const uint8* iptr, int32 istride,
                                   uint8* d, int32 owidth) {
  ASSERT((owidth % 3 == 0) && (owidth > 0));
  uint8* dend = d + owidth;
  const uint8* s = iptr;
  const uint8* t = iptr + istride;
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
static void ScaleFilterCols34_C(uint8* optr, const uint8* iptr, int owidth) {
  ASSERT((owidth % 3 == 0) && (owidth > 0));
  uint8* dend = optr + owidth;
  const uint8* s = iptr;
  do {
    optr[0] = (s[0] * 3 + s[1] * 1 + 2) >> 2;
    optr[1] = (s[1] * 1 + s[2] * 1 + 1) >> 1;
    optr[2] = (s[2] * 1 + s[3] * 3 + 2) >> 2;
    optr += 3;
    s += 4;
  } while (optr < dend);
}
#endif

static void ScaleFilterCols_C(uint8* optr, const uint8* iptr,
                              int owidth, int dx) {
  int x = 0;
  for (int j = 0; j < owidth; ++j) {
    int xi = x >> 16;
    int xf1 = x & 0xffff;
    int xf0 = 65536 - xf1;

    *optr++ = (iptr[xi] * xf0 + iptr[xi + 1] * xf1) >> 16;
    x += dx;
  }
}

#ifdef TEST_RSTSC
uint64 timers34[4] = { 0, };
#endif

static const int kMaxInputWidth = 2560;
#if defined(HAS_SCALEFILTERROWS_SSE2)
#define HAS_SCALEROWDOWN34_SSE2
// Filter rows 0 and 1 together, 3 : 1
static void ScaleRowDown34_0_Int_SSE2(const uint8* iptr, int32 istride,
                                      uint8* d, int32 owidth) {
  ASSERT((owidth % 3 == 0) && (owidth > 0));
  ALIGN16(uint8 row[kMaxInputWidth]);
#ifdef TEST_RSTSC
  uint64 t1 = __rdtsc();
#endif
  ScaleFilterRows_SSE2(row, iptr, istride, owidth * 4 / 3, 256 / 4);
#ifdef TEST_RSTSC
  uint64 t2 = __rdtsc();
#endif
  ScaleFilterCols34_C(d, row, owidth);

#ifdef TEST_RSTSC
  uint64 t3 = __rdtsc();
  timers34[0] += t2 - t1;
  timers34[1] += t3 - t2;
#endif
}

// Filter rows 1 and 2 together, 1 : 1
static void ScaleRowDown34_1_Int_SSE2(const uint8* iptr, int32 istride,
                                      uint8* d, int32 owidth) {
  ASSERT((owidth % 3 == 0) && (owidth > 0));
  ALIGN16(uint8 row[kMaxInputWidth]);
#ifdef TEST_RSTSC
  uint64 t1 = __rdtsc();
#endif
  ScaleFilterRows_SSE2(row, iptr, istride, owidth * 4 / 3, 256 / 2);
#ifdef TEST_RSTSC
  uint64 t2 = __rdtsc();
#endif
  ScaleFilterCols34_C(d, row, owidth);
#ifdef TEST_RSTSC
  uint64 t3 = __rdtsc();
  timers34[2] += t2 - t1;
  timers34[3] += t3 - t2;
#endif
}
#endif

static void ScaleRowDown38_C(const uint8* iptr, int32,
                             uint8* dst, int32 owidth) {
  ASSERT(owidth % 3 == 0);
  for (int x = 0; x < owidth; x += 3) {
    dst[0] = iptr[0];
    dst[1] = iptr[3];
    dst[2] = iptr[6];
    dst += 3;
    iptr += 8;
  }
}

// 8x3 -> 3x1
static void ScaleRowDown38_3_Int_C(const uint8* iptr, int32 istride,
                                   uint8* optr, int32 owidth) {
  ASSERT((owidth % 3 == 0) && (owidth > 0));
  for (int i = 0; i < owidth; i+=3) {
    optr[0] = (iptr[0] + iptr[1] + iptr[2] +
        iptr[istride + 0] + iptr[istride + 1] + iptr[istride + 2] +
        iptr[istride * 2 + 0] + iptr[istride * 2 + 1] + iptr[istride * 2 + 2]) *
        (65536 / 9) >> 16;
    optr[1] = (iptr[3] + iptr[4] + iptr[5] +
        iptr[istride + 3] + iptr[istride + 4] + iptr[istride + 5] +
        iptr[istride * 2 + 3] + iptr[istride * 2 + 4] + iptr[istride * 2 + 5]) *
        (65536 / 9) >> 16;
    optr[2] = (iptr[6] + iptr[7] +
        iptr[istride + 6] + iptr[istride + 7] +
        iptr[istride * 2 + 6] + iptr[istride * 2 + 7]) *
        (65536 / 6) >> 16;
    iptr += 8;
    optr += 3;
  }
}

// 8x2 -> 3x1
static void ScaleRowDown38_2_Int_C(const uint8* iptr, int32 istride,
                                   uint8* optr, int32 owidth) {
  ASSERT((owidth % 3 == 0) && (owidth > 0));
  for (int i = 0; i < owidth; i+=3) {
    optr[0] = (iptr[0] + iptr[1] + iptr[2] +
        iptr[istride + 0] + iptr[istride + 1] + iptr[istride + 2]) *
        (65536 / 6) >> 16;
    optr[1] = (iptr[3] + iptr[4] + iptr[5] +
        iptr[istride + 3] + iptr[istride + 4] + iptr[istride + 5]) *
        (65536 / 6) >> 16;
    optr[2] = (iptr[6] + iptr[7] +
        iptr[istride + 6] + iptr[istride + 7]) *
        (65536 / 4) >> 16;
    iptr += 8;
    optr += 3;
  }
}

// C version 8x2 -> 8x1
static void ScaleFilterRows_C(uint8* optr,
                              const uint8* iptr0, int32 istride,
                              int owidth, int source_y_fraction) {
  ASSERT(owidth > 0);
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint8* iptr1 = iptr0 + istride;
  uint8* end = optr + owidth;
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

void ScaleAddRows_C(const uint8* iptr, int32 istride,
                    uint16* orow, int32 iwidth, int32 iheight) {
  ASSERT(iwidth > 0);
  ASSERT(iheight > 0);
  for (int x = 0; x < iwidth; ++x) {
    const uint8* s = iptr + x;
    int sum = 0;
    for (int y = 0; y < iheight; ++y) {
      sum += s[0];
      s += istride;
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
static void ScalePlaneDown2(int32 iwidth, int32 iheight,
                            int32 owidth, int32 oheight,
                            int32 istride, int32 ostride,
                            const uint8 *iptr, uint8 *optr,
                            bool interpolate) {
  ASSERT(iwidth % 2 == 0);
  ASSERT(iheight % 2 == 0);
  void (*ScaleRowDown2)(const uint8* iptr, int32 istride,
                        uint8* orow, int32 owidth);

#if defined(HAS_SCALEROWDOWN2_NEON)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasNEON) &&
      (owidth % 16 == 0) && (istride % 16 == 0) && (ostride % 16 == 0) &&
      IS_ALIGNED(iptr, 16) && IS_ALIGNED(optr, 16)) {
    ScaleRowDown2 = interpolate ? ScaleRowDown2Int_NEON : ScaleRowDown2_NEON;
  } else
#endif
#if defined(HAS_SCALEROWDOWN2_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (owidth % 16 == 0) && IS_ALIGNED(iptr, 16) && IS_ALIGNED(optr, 16)) {
    ScaleRowDown2 = interpolate ? ScaleRowDown2Int_SSE2 : ScaleRowDown2_SSE2;
  } else
#endif
  {
    ScaleRowDown2 = interpolate ? ScaleRowDown2Int_C : ScaleRowDown2_C;
  }

  for (int y = 0; y < oheight; ++y) {
    ScaleRowDown2(iptr, istride, optr, owidth);
    iptr += (istride << 1);
    optr += ostride;
  }
}

/**
 * Scale plane, 1/4
 *
 * This is an optimized version for scaling down a plane to 1/4 of
 * its original size.
 */
static void ScalePlaneDown4(int32 iwidth, int32 iheight,
                            int32 owidth, int32 oheight,
                            int32 istride, int32 ostride,
                            const uint8 *iptr, uint8 *optr,
                            bool interpolate) {
  ASSERT(iwidth % 4 == 0);
  ASSERT(iheight % 4 == 0);
  void (*ScaleRowDown4)(const uint8* iptr, int32 istride,
                        uint8* orow, int32 owidth);

#if defined(HAS_SCALEROWDOWN4_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (owidth % 8 == 0) && (istride % 16 == 0) && (ostride % 8 == 0) &&
      IS_ALIGNED(iptr, 16) && IS_ALIGNED(optr, 8)) {
    ScaleRowDown4 = interpolate ? ScaleRowDown4Int_SSE2 : ScaleRowDown4_SSE2;
  } else
#endif
  {
    ScaleRowDown4 = interpolate ? ScaleRowDown4Int_C : ScaleRowDown4_C;
  }

  for (int y = 0; y < oheight; ++y) {
    ScaleRowDown4(iptr, istride, optr, owidth);
    iptr += (istride << 2);
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
static void ScalePlaneDown8(int32 iwidth, int32 iheight,
                            int32 owidth, int32 oheight,
                            int32 istride, int32 ostride,
                            const uint8 *iptr, uint8 *optr,
                            bool interpolate) {
  ASSERT(iwidth % 8 == 0);
  ASSERT(iheight % 8 == 0);
  void (*ScaleRowDown8)(const uint8* iptr, int32 istride,
                        uint8* orow, int32 owidth);
#if defined(HAS_SCALEROWDOWN8_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (owidth % 16 == 0) && owidth <= kMaxOutputWidth &&
      (istride % 16 == 0) && (ostride % 16 == 0) &&
      IS_ALIGNED(iptr, 16) && IS_ALIGNED(optr, 16)) {
    ScaleRowDown8 = interpolate ? ScaleRowDown8Int_SSE2 : ScaleRowDown8_SSE2;
  } else
#endif
  {
    ScaleRowDown8 = interpolate && (owidth <= kMaxOutputWidth) ?
        ScaleRowDown8Int_C : ScaleRowDown8_C;
  }
  for (int y = 0; y < oheight; ++y) {
    ScaleRowDown8(iptr, istride, optr, owidth);
    iptr += (istride << 3);
    optr += ostride;
  }
}

/**
 * Scale plane down, 3/4
 *
 * Provided by Frank Barchard (fbarchard@google.com)
 *
 */
static void ScalePlaneDown34(int32 iwidth, int32 iheight,
                             int32 owidth, int32 oheight,
                             int32 istride, int32 ostride,
                             const uint8* iptr, uint8* optr,
                             bool interpolate) {
  ASSERT(owidth % 3 == 0);
  void (*ScaleRowDown34_0)(const uint8* iptr, int32 istride,
                           uint8* orow, int32 owidth);
  void (*ScaleRowDown34_1)(const uint8* iptr, int32 istride,
                           uint8* orow, int32 owidth);
#if defined(HAS_SCALEROWDOWN34_SSSE3)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSSE3) &&
      (owidth % 24 == 0) && (istride % 16 == 0) && (ostride % 8 == 0) &&
      IS_ALIGNED(iptr, 16) && IS_ALIGNED(optr, 8)) {
    if (!interpolate) {
      ScaleRowDown34_0 = ScaleRowDown34_SSSE3;
      ScaleRowDown34_1 = ScaleRowDown34_SSSE3;
    } else {
      ScaleRowDown34_0 = ScaleRowDown34_0_Int_SSSE3;
      ScaleRowDown34_1 = ScaleRowDown34_1_Int_SSSE3;
    }
  } else
#endif
#if defined(HAS_SCALEROWDOWN34_SSE2)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
      (owidth % 24 == 0) && (istride % 16 == 0) && (ostride % 8 == 0) &&
      IS_ALIGNED(iptr, 16) && IS_ALIGNED(optr, 8) &&
      interpolate) {
    ScaleRowDown34_0 = ScaleRowDown34_0_Int_SSE2;
    ScaleRowDown34_1 = ScaleRowDown34_1_Int_SSE2;
  } else
#endif
  {
    if (!interpolate) {
      ScaleRowDown34_0 = ScaleRowDown34_C;
      ScaleRowDown34_1 = ScaleRowDown34_C;
    } else {
      ScaleRowDown34_0 = ScaleRowDown34_0_Int_C;
      ScaleRowDown34_1 = ScaleRowDown34_1_Int_C;
    }
  }
  int irow = 0;
  for (int y = 0; y < oheight; ++y) {
    switch (irow) {
      case 0:
        ScaleRowDown34_0(iptr, istride, optr, owidth);
        break;

      case 1:
        ScaleRowDown34_1(iptr, istride, optr, owidth);
        break;

      case 2:
        ScaleRowDown34_0(iptr + istride, -istride, optr, owidth);
        break;
    }
    ++irow;
    iptr += istride;
    optr += ostride;
    if (irow >= 3) {
      iptr += istride;
      irow = 0;
    }
  }

#ifdef TEST_RSTSC
  std::cout << "Timer34_0 Row " << std::setw(9) << timers34[0]
            << " Column " << std::setw(9) << timers34[1]
            << " Timer34_1 Row " << std::setw(9) << timers34[2]
            << " Column " << std::setw(9) << timers34[3] << std::endl;
#endif
}

/**
 * Scale plane, 3/8
 *
 * This is an optimized version for scaling down a plane to 3/8
 * of its original size.
 *
 * Reduces 16x3 to 6x1
 */
static void ScalePlaneDown38(int32 iwidth, int32 iheight,
                             int32 owidth, int32 oheight,
                             int32 istride, int32 ostride,
                             const uint8* iptr, uint8* optr,
                             bool interpolate) {
  ASSERT(owidth % 3 == 0);
  void (*ScaleRowDown38_3)(const uint8* iptr, int32 istride,
                           uint8* orow, int32 owidth);
  void (*ScaleRowDown38_2)(const uint8* iptr, int32 istride,
                           uint8* orow, int32 owidth);
#if defined(HAS_SCALEROWDOWN38_SSSE3)
  if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSSE3) &&
      (owidth % 24 == 0) && (istride % 16 == 0) && (ostride % 8 == 0) &&
      IS_ALIGNED(iptr, 16) && IS_ALIGNED(optr, 8)) {
    if (!interpolate) {
      ScaleRowDown38_3 = ScaleRowDown38_SSSE3;
      ScaleRowDown38_2 = ScaleRowDown38_SSSE3;
    } else {
      ScaleRowDown38_3 = ScaleRowDown38_3_Int_SSSE3;
      ScaleRowDown38_2 = ScaleRowDown38_2_Int_SSSE3;
    }
  } else
#endif
  {
    if (!interpolate) {
      ScaleRowDown38_3 = ScaleRowDown38_C;
      ScaleRowDown38_2 = ScaleRowDown38_C;
    } else {
      ScaleRowDown38_3 = ScaleRowDown38_3_Int_C;
      ScaleRowDown38_2 = ScaleRowDown38_2_Int_C;
    }
  }
  int irow = 0;
  for (int y = 0; y < oheight; ++y) {
    switch (irow) {
      case 0:
      case 1:
        ScaleRowDown38_3(iptr, istride, optr, owidth);
        iptr += istride * 3;
        ++irow;
        break;

      case 2:
        ScaleRowDown38_2(iptr, istride, optr, owidth);
        iptr += istride * 2;
        irow = 0;
        break;
    }
    optr += ostride;
  }
}

inline static uint32 SumBox(int32 iboxwidth, int32 iboxheight,
                            int32 istride, const uint8 *iptr) {
  ASSERT(iboxwidth > 0);
  ASSERT(iboxheight > 0);
  uint32 sum = 0u;
  for (int y = 0; y < iboxheight; ++y) {
    for (int x = 0; x < iboxwidth; ++x) {
      sum += iptr[x];
    }
    iptr += istride;
  }
  return sum;
}

static void ScalePlaneBoxRow(int32 owidth, int32 boxheight,
                             int dx, int32 istride,
                             const uint8 *iptr, uint8 *optr) {
  int x = 0;
  for (int i = 0; i < owidth; ++i) {
    int ix = x >> 16;
    x += dx;
    int boxwidth = (x >> 16) - ix;
    *optr++ = SumBox(boxwidth, boxheight, istride, iptr + ix) /
        (boxwidth * boxheight);
  }
}

inline static uint32 SumPixels(int32 iboxwidth, const uint16 *iptr) {
  ASSERT(iboxwidth > 0);
  uint32 sum = 0u;
  for (int x = 0; x < iboxwidth; ++x) {
    sum += iptr[x];
  }
  return sum;
}

static void ScaleAddCols2_C(int32 owidth, int32 boxheight, int dx,
                            const uint16 *iptr, uint8 *optr) {
  int scaletbl[2];
  int minboxwidth = (dx >> 16);
  scaletbl[0] = 65536 / (minboxwidth * boxheight);
  scaletbl[1] = 65536 / ((minboxwidth + 1) * boxheight);
  int *scaleptr = scaletbl - minboxwidth;
  int x = 0;
  for (int i = 0; i < owidth; ++i) {
    int ix = x >> 16;
    x += dx;
    int boxwidth = (x >> 16) - ix;
    *optr++ = SumPixels(boxwidth, iptr + ix) * scaleptr[boxwidth] >> 16;
  }
}

static void ScaleAddCols1_C(int32 owidth, int32 boxheight, int dx,
                            const uint16 *iptr, uint8 *optr) {
  int boxwidth = (dx >> 16);
  int scaleval = 65536 / (boxwidth * boxheight);
  int x = 0;
  for (int i = 0; i < owidth; ++i) {
    *optr++ = SumPixels(boxwidth, iptr + x) * scaleval >> 16;
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
static void ScalePlaneBox(int32 iwidth, int32 iheight,
                          int32 owidth, int32 oheight,
                          int32 istride, int32 ostride,
                          const uint8 *iptr, uint8 *optr) {
  ASSERT(owidth > 0);
  ASSERT(oheight > 0);
  int dy = (iheight << 16) / oheight;
  int dx = (iwidth << 16) / owidth;
  if ((iwidth % 16 != 0) || (iwidth > kMaxInputWidth) ||
      oheight * 2 > iheight) {
    uint8 *dst = optr;
    int dy = (iheight << 16) / oheight;
    int dx = (iwidth << 16) / owidth;
    int y = 0;
    for (int j = 0; j < oheight; ++j) {
      int iy = y >> 16;
      const uint8 *const src = iptr + iy * istride;
      y += dy;
      if (y > (iheight << 16)) {
        y = (iheight << 16);
      }
      int boxheight = (y >> 16) - iy;
      ScalePlaneBoxRow(owidth, boxheight,
                       dx, istride,
                       src, dst);

      dst += ostride;
    }
  } else {
    ALIGN16(uint16 row[kMaxInputWidth]);
    void (*ScaleAddRows)(const uint8* iptr, int32 istride,
                         uint16* orow, int32 iwidth, int32 iheight);
    void (*ScaleAddCols)(int32 owidth, int32 boxheight, int dx,
                         const uint16 *iptr, uint8 *optr);
#if defined(HAS_SCALEADDROWS_SSE2)
    if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
        (istride % 16 == 0) && IS_ALIGNED(iptr, 16) && (iwidth % 16) == 0) {
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
    for (int j = 0; j < oheight; ++j) {
      int iy = y >> 16;
      const uint8 *const src = iptr + iy * istride;
      y += dy;
      if (y > (iheight << 16)) {
        y = (iheight << 16);
      }
      int boxheight = (y >> 16) - iy;
      ScaleAddRows(src, istride, row, iwidth, boxheight);
      ScaleAddCols(owidth, boxheight, dx, row, optr);
      optr += ostride;
    }
  }
}

/**
 * Scale plane to/from any dimensions, with interpolation.
 */
static void ScalePlaneBilinearSimple(int32 iwidth, int32 iheight,
                                     int32 owidth, int32 oheight,
                                     int32 istride, int32 ostride,
                                     const uint8 *iptr, uint8 *optr) {
  uint8 *dst = optr;
  int dx = (iwidth << 16) / owidth;
  int dy = (iheight << 16) / oheight;
  int maxx = ((iwidth - 1) << 16) - 1;
  int maxy = ((iheight - 1) << 16) - 1;
  int y = (oheight < iheight) ? 32768 : (iheight << 16) / oheight - 32768;
  for (int i = 0; i < oheight; ++i) {
    int cy = (y < 0) ? 0 : y;
    int yi = cy >> 16;
    int yf = cy & 0xffff;
    const uint8 *const src = iptr + yi * istride;
    int x = (owidth < iwidth) ? 32768 : (iwidth << 16) / owidth - 32768;
    for (int j = 0; j < owidth; ++j) {
      int cx = (x < 0) ? 0 : x;
      int xi = cx >> 16;
      int xf = cx & 0xffff;
      int r0 = (src[xi] * (65536 - xf) + src[xi + 1] * xf) >> 16;
      int r1 = (src[xi + istride] * (65536 - xf) + src[xi + istride + 1] * xf)
                >> 16;
      *dst++ = (r0 * (65536 - yf) + r1 * yf) >> 16;
      x += dx;
      if (x > maxx)
        x = maxx;
    }
    dst += ostride - owidth;
    y += dy;
    if (y > maxy)
      y = maxy;
  }
}

/**
 * Scale plane to/from any dimensions, with bilinear
 * interpolation.
 */
static void ScalePlaneBilinear(int32 iwidth, int32 iheight,
                               int32 owidth, int32 oheight,
                               int32 istride, int32 ostride,
                               const uint8 *iptr, uint8 *optr) {
  ASSERT(owidth > 0);
  ASSERT(oheight > 0);
  int dy = (iheight << 16) / oheight;
  int dx = (iwidth << 16) / owidth;
  if ((iwidth % 8 != 0) || (iwidth > kMaxInputWidth)) {
    ScalePlaneBilinearSimple(iwidth, iheight, owidth, oheight, istride, ostride,
                             iptr, optr);

  } else {
    ALIGN16(uint8 row[kMaxInputWidth + 1]);
    void (*ScaleFilterRows)(uint8* optr, const uint8* iptr0, int32 istride,
                            int owidth, int source_y_fraction);
    void (*ScaleFilterCols)(uint8* optr, const uint8* iptr,
                            int owidth, int dx);
#if defined(HAS_SCALEFILTERROWS_SSSE3)
    if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSSE3) &&
        (istride % 16 == 0) && IS_ALIGNED(iptr, 16) && (iwidth % 16) == 0) {
      ScaleFilterRows = ScaleFilterRows_SSSE3;
    } else
#endif
#if defined(HAS_SCALEFILTERROWS_SSE2)
    if (libyuv::CpuInfo::TestCpuFlag(libyuv::CpuInfo::kCpuHasSSE2) &&
        (istride % 16 == 0) && IS_ALIGNED(iptr, 16) && (iwidth % 16) == 0) {
      ScaleFilterRows = ScaleFilterRows_SSE2;
    } else
#endif
    {
      ScaleFilterRows = ScaleFilterRows_C;
    }
    ScaleFilterCols = ScaleFilterCols_C;

    int y = 0;
    int maxy = ((iheight - 1) << 16) - 1; // max is filter of last 2 rows.
    for (int j = 0; j < oheight; ++j) {
      int iy = y >> 16;
      int fy = (y >> 8) & 255;
      const uint8 *const src = iptr + iy * istride;
      ScaleFilterRows(row, src, istride, iwidth, fy);
      ScaleFilterCols(optr, row, owidth, dx);
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
static void ScalePlaneSimple(int32 iwidth, int32 iheight,
                             int32 owidth, int32 oheight,
                             int32 istride, int32 ostride,
                             const uint8 *iptr, uint8 *optr) {
  uint8 *dst = optr;
  int dx = (iwidth << 16) / owidth;
  for (int y = 0; y < oheight; ++y) {
    const uint8 *const src = iptr + (y * iheight / oheight) * istride;
    // TODO(fbarchard): Round X coordinate by setting x=0x8000.
    int x = 0;
    for (int i = 0; i < owidth; ++i) {
      *dst++ = src[x >> 16];
      x += dx;
    }
    dst += ostride - owidth;
  }
}

/**
 * Scale plane to/from any dimensions.
 */
static void ScalePlaneAnySize(int32 iwidth, int32 iheight,
                              int32 owidth, int32 oheight,
                              int32 istride, int32 ostride,
                              const uint8 *iptr, uint8 *optr,
                              bool interpolate) {
  if (!interpolate) {
    ScalePlaneSimple(iwidth, iheight, owidth, oheight, istride, ostride,
                     iptr, optr);
  } else {
    // fall back to non-optimized version
    ScalePlaneBilinear(iwidth, iheight, owidth, oheight, istride, ostride,
                       iptr, optr);
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
static void ScalePlaneDown(int32 iwidth, int32 iheight,
                           int32 owidth, int32 oheight,
                           int32 istride, int32 ostride,
                           const uint8 *iptr, uint8 *optr,
                           bool interpolate) {
  if (!interpolate) {
    ScalePlaneSimple(iwidth, iheight, owidth, oheight, istride, ostride,
                     iptr, optr);
  } else if (iheight * 2 > oheight) {  // between 1/2x and 1x use bilinear
    ScalePlaneBilinear(iwidth, iheight, owidth, oheight, istride, ostride,
                       iptr, optr);
  } else {
    ScalePlaneBox(iwidth, iheight, owidth, oheight, istride, ostride,
                  iptr, optr);
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
static void CopyPlane(int32 iwidth, int32 iheight,
                      int32 owidth, int32 oheight,
                      int32 istride, int32 ostride,
                      const uint8 *iptr, uint8 *optr) {
  if (istride == iwidth && ostride == owidth) {
    // All contiguous, so can use REALLY fast path.
    memcpy(optr, iptr, iwidth * iheight);
  } else {
    // Not all contiguous; must copy scanlines individually
    const uint8 *src = iptr;
    uint8 *dst = optr;
    for (int i = 0; i < iheight; ++i) {
      memcpy(dst, src, iwidth);
      dst += ostride;
      src += istride;
    }
  }
}

static void ScalePlane(const uint8 *in, int32 istride,
                       int32 iwidth, int32 iheight,
                       uint8 *out, int32 ostride,
                       int32 owidth, int32 oheight,
                       bool interpolate, bool use_ref) {
  // Use specialized scales to improve performance for common resolutions.
  // For example, all the 1/2 scalings will use ScalePlaneDown2()
  if (owidth == iwidth && oheight == iheight) {
    // Straight copy.
    CopyPlane(iwidth, iheight, owidth, oheight, istride, ostride, in, out);
  } else if (owidth <= iwidth && oheight <= iheight) {
    // Scale down.
    if (use_ref) {
      // For testing, allow the optimized versions to be disabled.
      ScalePlaneDown(iwidth, iheight, owidth, oheight, istride, ostride,
                     in, out, interpolate);
    } else if (4 * owidth == 3 * iwidth && 4 * oheight == 3 * iheight) {
      // optimized, 3/4
      ScalePlaneDown34(iwidth, iheight, owidth, oheight, istride, ostride,
                       in, out, interpolate);
    } else if (2 * owidth == iwidth && 2 * oheight == iheight) {
      // optimized, 1/2
      ScalePlaneDown2(iwidth, iheight, owidth, oheight, istride, ostride,
                      in, out, interpolate);
    // 3/8 rounded up for odd sized chroma height.
    } else if (8 * owidth == 3 * iwidth && oheight == ((iheight * 3 + 7) / 8)) {
      // optimized, 3/8
      ScalePlaneDown38(iwidth, iheight, owidth, oheight, istride, ostride,
                       in, out, interpolate);
    } else if (4 * owidth == iwidth && 4 * oheight == iheight) {
      // optimized, 1/4
      ScalePlaneDown4(iwidth, iheight, owidth, oheight, istride, ostride,
                      in, out, interpolate);
    } else if (8 * owidth == iwidth && 8 * oheight == iheight) {
      // optimized, 1/8
      ScalePlaneDown8(iwidth, iheight, owidth, oheight, istride, ostride,
                      in, out, interpolate);
    } else {
      // Arbitrary downsample
      ScalePlaneDown(iwidth, iheight, owidth, oheight, istride, ostride,
                     in, out, interpolate);
    }
  } else {
    // Arbitrary scale up and/or down.
    ScalePlaneAnySize(iwidth, iheight, owidth, oheight, istride, ostride,
                      in, out, interpolate);
  }
}

/**
 * Scale a plane.
 *
 * This function in turn calls a scaling function
 * suitable for handling the desired resolutions.
 *
 */
bool YuvScaler::Scale(const uint8 *inY, const uint8 *inU, const uint8 *inV,
                      int32 istrideY, int32 istrideU, int32 istrideV,
                      int32 iwidth, int32 iheight,
                      uint8 *outY, uint8 *outU, uint8 *outV,
                      int32 ostrideY, int32 ostrideU, int32 ostrideV,
                      int32 owidth, int32 oheight,
                      bool interpolate) {
  if (!inY || !inU || !inV || iwidth <= 0 || iheight <= 0 ||
      !outY || !outU || !outV || owidth <= 0 || oheight <= 0) {
    return false;
  }
  int32 halfiwidth = (iwidth + 1) >> 1;
  int32 halfiheight = (iheight + 1) >> 1;
  int32 halfowidth = (owidth + 1) >> 1;
  int32 halfoheight = (oheight + 1) >> 1;

  ScalePlane(inY, istrideY, iwidth, iheight,
             outY, ostrideY, owidth, oheight,
             interpolate, use_reference_impl_);
  ScalePlane(inU, istrideU, halfiwidth, halfiheight,
             outU, ostrideU, halfowidth, halfoheight,
             interpolate, use_reference_impl_);
  ScalePlane(inV, istrideV, halfiwidth, halfiheight,
             outV, ostrideV, halfowidth, halfoheight,
             interpolate, use_reference_impl_);
  return true;
}

bool YuvScaler::Scale(const uint8 *in, int32 iwidth, int32 iheight,
                      uint8 *out, int32 owidth, int32 oheight, int32 ooffset,
                      bool interpolate) {
  if (!in || iwidth <= 0 || iheight <= 0 ||
      !out || owidth <= 0 || oheight <= 0 || ooffset < 0 ||
      ooffset >= oheight) {
    return false;
  }
  ooffset = ooffset & ~1;  // chroma requires offset to multiple of 2.
  int32 halfiwidth = (iwidth + 1) >> 1;
  int32 halfiheight = (iheight + 1) >> 1;
  int32 halfowidth = (owidth + 1) >> 1;
  int32 halfoheight = (oheight + 1) >> 1;
  int32 aheight = oheight - ooffset * 2;  // actual output height
  const uint8 *const iyptr = in;
  uint8 *oyptr = out + ooffset * owidth;
  const uint8 *const iuptr = in + iwidth * iheight;
  uint8 *ouptr = out + owidth * oheight + (ooffset >> 1) * halfowidth;
  const uint8 *const ivptr = in + iwidth * iheight +
                             halfiwidth * halfiheight;
  uint8 *ovptr = out + owidth * oheight + halfowidth * halfoheight +
                 (ooffset >> 1) * halfowidth;
  return Scale(iyptr, iuptr, ivptr, iwidth, halfiwidth, halfiwidth,
               iwidth, iheight, oyptr, ouptr, ovptr, owidth,
               halfowidth, halfowidth, owidth, aheight, interpolate);
}

}  // namespace libyuv

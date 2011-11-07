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
static const vec8 kARGBToY = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};

static const uvec8 kAddY16 = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u,
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u,
};

#ifdef HAS_ARGBTOUVROW_SSSE3
static const vec8 kARGBToU = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

static const uvec8 kARGBToV = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};
static const uvec8 kAddUV128 = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};
#endif

// Shuffle table for converting BG24 to ARGB.
static const uvec8 kShuffleMaskBG24ToARGB = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};

// Shuffle table for converting RAW to ARGB.
static const uvec8 kShuffleMaskRAWToARGB = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};

// Shuffle table for converting ABGR to ARGB.
static const uvec8 kShuffleMaskABGRToARGB = {
  2u, 1u, 0u, 3u, 6u, 5u, 4u, 7u, 10u, 9u, 8u, 11u, 14u, 13u, 12u, 15u
};

// Shuffle table for converting BGRA to ARGB.
static const uvec8 kShuffleMaskBGRAToARGB = {
  3u, 2u, 1u, 0u, 7u, 6u, 5u, 4u, 11u, 10u, 9u, 8u, 15u, 14u, 13u, 12u
};

void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  asm volatile(
  "pcmpeqb    %%xmm5,%%xmm5\n"
  "pslld      $0x18,%%xmm5\n"
"1:"
  "movq       (%0),%%xmm0\n"
  "lea        0x8(%0),%0\n"
  "punpcklbw  %%xmm0,%%xmm0\n"
  "movdqa     %%xmm0,%%xmm1\n"
  "punpcklwd  %%xmm0,%%xmm0\n"
  "punpckhwd  %%xmm1,%%xmm1\n"
  "por        %%xmm5,%%xmm0\n"
  "por        %%xmm5,%%xmm1\n"
  "movdqa     %%xmm0,(%1)\n"
  "movdqa     %%xmm1,0x10(%1)\n"
  "lea        0x20(%1),%1\n"
  "sub        $0x8,%2\n"
  "ja         1b\n"
  : "+r"(src_y),     // %0
    "+r"(dst_argb),  // %1
    "+r"(pix)        // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
);
}

void ABGRToARGBRow_SSSE3(const uint8* src_abgr, uint8* dst_argb, int pix) {
  asm volatile(
  "movdqa     %3,%%xmm5\n"
"1:"
  "movdqa     (%0),%%xmm0\n"
  "lea        0x10(%0),%0\n"
  "pshufb     %%xmm5,%%xmm0\n"
  "movdqa     %%xmm0,(%1)\n"
  "lea        0x10(%1),%1\n"
  "sub        $0x4,%2\n"
  "ja         1b\n"
  : "+r"(src_abgr),  // %0
    "+r"(dst_argb),  // %1
    "+r"(pix)        // %2
  : "m"(kShuffleMaskABGRToARGB)  // %3
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif

);
}

void BGRAToARGBRow_SSSE3(const uint8* src_bgra, uint8* dst_argb, int pix) {
  asm volatile(
  "movdqa     %3,%%xmm5\n"
"1:"
  "movdqa     (%0),%%xmm0\n"
  "lea        0x10(%0),%0\n"
  "pshufb     %%xmm5,%%xmm0\n"
  "movdqa     %%xmm0,(%1)\n"
  "lea        0x10(%1),%1\n"
  "sub        $0x4,%2\n"
  "ja         1b\n"
  : "+r"(src_bgra),  // %0
    "+r"(dst_argb),  // %1
    "+r"(pix)        // %2
  : "m"(kShuffleMaskBGRAToARGB)  // %3
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
);
}

void BG24ToARGBRow_SSSE3(const uint8* src_bg24, uint8* dst_argb, int pix) {
  asm volatile(
  "pcmpeqb    %%xmm5,%%xmm5\n"  // generate mask 0xff000000
  "pslld      $0x18,%%xmm5\n"
  "movdqa     %3,%%xmm4\n"
"1:"
  "movdqa     (%0),%%xmm0\n"
  "movdqa     0x10(%0),%%xmm1\n"
  "movdqa     0x20(%0),%%xmm3\n"
  "lea        0x30(%0),%0\n"
  "movdqa     %%xmm3,%%xmm2\n"
  "palignr    $0x8,%%xmm1,%%xmm2\n"  // xmm2 = { xmm3[0:3] xmm1[8:15] }
  "pshufb     %%xmm4,%%xmm2\n"
  "por        %%xmm5,%%xmm2\n"
  "palignr    $0xc,%%xmm0,%%xmm1\n"  // xmm1 = { xmm3[0:7] xmm0[12:15] }
  "pshufb     %%xmm4,%%xmm0\n"
  "movdqa     %%xmm2,0x20(%1)\n"
  "por        %%xmm5,%%xmm0\n"
  "pshufb     %%xmm4,%%xmm1\n"
  "movdqa     %%xmm0,(%1)\n"
  "por        %%xmm5,%%xmm1\n"
  "palignr    $0x4,%%xmm3,%%xmm3\n"  // xmm3 = { xmm3[4:15] }
  "pshufb     %%xmm4,%%xmm3\n"
  "movdqa     %%xmm1,0x10(%1)\n"
  "por        %%xmm5,%%xmm3\n"
  "movdqa     %%xmm3,0x30(%1)\n"
  "lea        0x40(%1),%1\n"
  "sub        $0x10,%2\n"
  "ja         1b\n"
  : "+r"(src_bg24),  // %0
    "+r"(dst_argb),  // %1
    "+r"(pix)        // %2
  : "m"(kShuffleMaskBG24ToARGB)  // %3
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
);
}

void RAWToARGBRow_SSSE3(const uint8* src_raw, uint8* dst_argb, int pix) {
  asm volatile(
  "pcmpeqb    %%xmm5,%%xmm5\n"  // generate mask 0xff000000
  "pslld      $0x18,%%xmm5\n"
  "movdqa     %3,%%xmm4\n"
"1:"
  "movdqa     (%0),%%xmm0\n"
  "movdqa     0x10(%0),%%xmm1\n"
  "movdqa     0x20(%0),%%xmm3\n"
  "lea        0x30(%0),%0\n"
  "movdqa     %%xmm3,%%xmm2\n"
  "palignr    $0x8,%%xmm1,%%xmm2\n"  // xmm2 = { xmm3[0:3] xmm1[8:15] }
  "pshufb     %%xmm4,%%xmm2\n"
  "por        %%xmm5,%%xmm2\n"
  "palignr    $0xc,%%xmm0,%%xmm1\n"  // xmm1 = { xmm3[0:7] xmm0[12:15] }
  "pshufb     %%xmm4,%%xmm0\n"
  "movdqa     %%xmm2,0x20(%1)\n"
  "por        %%xmm5,%%xmm0\n"
  "pshufb     %%xmm4,%%xmm1\n"
  "movdqa     %%xmm0,(%1)\n"
  "por        %%xmm5,%%xmm1\n"
  "palignr    $0x4,%%xmm3,%%xmm3\n"  // xmm3 = { xmm3[4:15] }
  "pshufb     %%xmm4,%%xmm3\n"
  "movdqa     %%xmm1,0x10(%1)\n"
  "por        %%xmm5,%%xmm3\n"
  "movdqa     %%xmm3,0x30(%1)\n"
  "lea        0x40(%1),%1\n"
  "sub        $0x10,%2\n"
  "ja         1b\n"
  : "+r"(src_raw),   // %0
    "+r"(dst_argb),  // %1
    "+r"(pix)        // %2
  : "m"(kShuffleMaskRAWToARGB)  // %3
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
);
}

void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile(
  "movdqa     %4,%%xmm5\n"
  "movdqa     %3,%%xmm4\n"
"1:"
  "movdqa     (%0),%%xmm0\n"
  "movdqa     0x10(%0),%%xmm1\n"
  "movdqa     0x20(%0),%%xmm2\n"
  "movdqa     0x30(%0),%%xmm3\n"
  "pmaddubsw  %%xmm4,%%xmm0\n"
  "pmaddubsw  %%xmm4,%%xmm1\n"
  "pmaddubsw  %%xmm4,%%xmm2\n"
  "pmaddubsw  %%xmm4,%%xmm3\n"
  "lea        0x40(%0),%0\n"
  "phaddw     %%xmm1,%%xmm0\n"
  "phaddw     %%xmm3,%%xmm2\n"
  "psrlw      $0x7,%%xmm0\n"
  "psrlw      $0x7,%%xmm2\n"
  "packuswb   %%xmm2,%%xmm0\n"
  "paddb      %%xmm5,%%xmm0\n"
  "movdqa     %%xmm0,(%1)\n"
  "lea        0x10(%1),%1\n"
  "sub        $0x10,%2\n"
  "ja         1b\n"
  : "+r"(src_argb),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  : "m"(kARGBToY),   // %3
    "m"(kAddY16)     // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif

);
}
#endif

#ifdef HAS_ARGBTOUVROW_SSSE3
void ARGBToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
  asm volatile(
  "movdqa     %5,%%xmm7\n"
  "movdqa     %6,%%xmm6\n"
  "movdqa     %7,%%xmm5\n"
  "sub        %1,%2\n"
"1:"
  "movdqa     (%0),%%xmm0\n"
  "movdqa     0x10(%0),%%xmm1\n"
  "movdqa     0x20(%0),%%xmm2\n"
  "movdqa     0x30(%0),%%xmm3\n"
  "pavgb      (%0,%4,1),%%xmm0\n"
  "pavgb      0x10(%0,%4,1),%%xmm1\n"
  "pavgb      0x20(%0,%4,1),%%xmm2\n"
  "pavgb      0x30(%0,%4,1),%%xmm3\n"
  "lea        0x40(%0),%0\n"
  "movdqa     %%xmm0,%%xmm4\n"
  "shufps     $0x88,%%xmm1,%%xmm0\n"
  "shufps     $0xdd,%%xmm1,%%xmm4\n"
  "pavgb      %%xmm4,%%xmm0\n"
  "movdqa     %%xmm2,%%xmm4\n"
  "shufps     $0x88,%%xmm3,%%xmm2\n"
  "shufps     $0xdd,%%xmm3,%%xmm4\n"
  "pavgb      %%xmm4,%%xmm2\n"
  "movdqa     %%xmm0,%%xmm1\n"
  "movdqa     %%xmm2,%%xmm3\n"
  "pmaddubsw  %%xmm7,%%xmm0\n"
  "pmaddubsw  %%xmm7,%%xmm2\n"
  "pmaddubsw  %%xmm6,%%xmm1\n"
  "pmaddubsw  %%xmm6,%%xmm3\n"
  "phaddw     %%xmm2,%%xmm0\n"
  "phaddw     %%xmm3,%%xmm1\n"
  "psraw      $0x8,%%xmm0\n"
  "psraw      $0x8,%%xmm1\n"
  "packsswb   %%xmm1,%%xmm0\n"
  "paddb      %%xmm5,%%xmm0\n"
  "movlps     %%xmm0,(%1)\n"
  "movhps     %%xmm0,(%1,%2,1)\n"
  "lea        0x8(%1),%1\n"
  "sub        $0x10,%3\n"
  "ja         1b\n"
  : "+r"(src_argb0),       // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"(static_cast<intptr_t>(src_stride_argb)), // %4
    "m"(kARGBToU),         // %5
    "m"(kARGBToV),         // %6
    "m"(kAddUV128)         // %7
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
);
}
#endif

// The following code requires 6 registers and prefers 7 registers.
// 7 registers requires -fpic to be off, and -fomit-frame-pointer
#ifdef HAS_FASTCONVERTYUVTOARGBROW_SSE2
#if defined(__x86_64__)
#define REG_a "rax"
#define REG_d "rdx"
#else
#define REG_a "eax"
#define REG_d "edx"
#endif
#if defined(__APPLE__) || defined(__x86_64__)
#define OMITFP
#else
#define OMITFP __attribute__((optimize("omit-frame-pointer")))
#endif

#if defined(__APPLE__)
// REG6 version uses 1 less register but is slower
#define REG6
#endif

#ifdef REG6
// 6 register version only has REG_a for temporary
#define CLOBBER "%"REG_a
#define YUVTORGB                                                               \
 "1:"                                                                          \
  "movzb  (%1),%%"REG_a"\n"                                                    \
  "lea    1(%1),%1\n"                                                          \
  "movq   2048(%5,%%"REG_a",8),%%xmm0\n"                                       \
  "movzb  (%2),%%"REG_a"\n"                                                    \
  "lea    1(%2),%2\n"                                                          \
  "movq   4096(%5,%%"REG_a",8),%%xmm1\n"                                       \
  "paddsw %%xmm1,%%xmm0\n"                                                     \
  "movzb  (%0),%%"REG_a"\n"                                                    \
  "movq   0(%5,%%"REG_a",8),%%xmm2\n"                                          \
  "movzb  0x1(%0),%%"REG_a"\n"                                                 \
  "movq   0(%5,%%"REG_a",8),%%xmm3\n"                                          \
  "lea    2(%0),%0\n"                                                          \
  "paddsw %%xmm0,%%xmm2\n"                                                     \
  "paddsw %%xmm0,%%xmm3\n"                                                     \
  "shufps $0x44,%%xmm3,%%xmm2\n"                                               \
  "psraw  $0x6,%%xmm2\n"                                                       \
  "packuswb %%xmm2,%%xmm2\n"                                                   \
  "movq   %%xmm2,0x0(%3)\n"                                                    \
  "lea    8(%3),%3\n"                                                          \
  "sub    $0x2,%4\n"                                                           \
  "ja     1b\n"
#else
#define CLOBBER "%"REG_a, "%"REG_d
// This version produces 2 pixels
#define YUVTORGB                                                               \
"1:"                                                                           \
  "movzb      (%1),%%"REG_a"\n"                                                \
  "lea        1(%1),%1\n"                                                      \
  "movzb      (%2),%%"REG_d"\n"                                                \
  "lea        1(%2),%2\n"                                                      \
  "movq       2048(%5,%%"REG_a",8),%%xmm0\n"                                   \
  "movzb      0(%0),%%"REG_a"\n"                                               \
  "movq       4096(%5,%%"REG_d",8),%%xmm1\n"                                   \
  "paddsw     %%xmm1,%%xmm0\n"                                                 \
  "movzb      1(%0),%%"REG_d"\n"                                               \
  "punpcklqdq %%xmm0,%%xmm0\n"                                                 \
  "lea        2(%0),%0\n"                                                      \
  "movq       0(%5,%%"REG_a",8),%%xmm1\n"                                      \
  "movhps     0(%5,%%"REG_d",8),%%xmm1\n"                                      \
  "paddsw     %%xmm0,%%xmm1\n"                                                 \
  "psraw      $6,%%xmm1\n"                                                     \
  "packuswb   %%xmm1,%%xmm1\n"                                                 \
  "movq       %%xmm1,0(%3)\n"                                                  \
  "lea        8(%3),%3\n"                                                      \
  "sub        $0x2,%4\n"                                                       \
  "ja         1b\n"
// This version produces 4 pixels
#define YUVTORGB4                                                              \
"1:"                                                                           \
  "movzb      0(%1),%%"REG_a"\n"                                               \
  "movzb      0(%2),%%"REG_d"\n"                                               \
  "movq       2048(%5,%%"REG_a",8),%%xmm0\n"                                   \
  "movzb      0(%0),%%"REG_a"\n"                                               \
  "movq       4096(%5,%%"REG_d",8),%%xmm1\n"                                   \
  "paddsw     %%xmm1,%%xmm0\n"                                                 \
  "movzb      1(%0),%%"REG_d"\n"                                               \
  "punpcklqdq %%xmm0,%%xmm0\n"                                                 \
  "movq       0(%5,%%"REG_a",8),%%xmm2\n"                                      \
  "movhps     0(%5,%%"REG_d",8),%%xmm2\n"                                      \
  "paddsw     %%xmm0,%%xmm2\n"                                                 \
  "psraw      $6,%%xmm2\n"                                                     \
  "movzb      1(%1),%%"REG_a"\n"                                               \
  "movzb      1(%2),%%"REG_d"\n"                                               \
  "movq       2048(%5,%%"REG_a",8),%%xmm0\n"                                   \
  "movzb      2(%0),%%"REG_a"\n"                                               \
  "movq       4096(%5,%%"REG_d",8),%%xmm1\n"                                   \
  "paddsw     %%xmm1,%%xmm0\n"                                                 \
  "movzb      3(%0),%%"REG_d"\n"                                               \
  "punpcklqdq %%xmm0,%%xmm0\n"                                                 \
  "movq       0(%5,%%"REG_a",8),%%xmm3\n"                                      \
  "movhps     0(%5,%%"REG_d",8),%%xmm3\n"                                      \
  "paddsw     %%xmm0,%%xmm3\n"                                                 \
  "psraw      $6,%%xmm3\n"                                                     \
  "lea        2(%1),%1\n"                                                      \
  "lea        2(%2),%2\n"                                                      \
  "lea        4(%0),%0\n"                                                      \
  "packuswb   %%xmm3,%%xmm2\n"                                                 \
  "movdqa     %%xmm2,0(%3)\n"                                                  \
  "lea        16(%3),%3\n"                                                     \
  "sub        $0x4,%4\n"                                                       \
  "ja         1b\n"
#endif

// 6 or 7 registers
void OMITFP FastConvertYUVToARGBRow_SSE2(const uint8* y_buf,  // rdi
                                         const uint8* u_buf,  // rsi
                                         const uint8* v_buf,  // rdx
                                         uint8* rgb_buf,      // rcx
                                         int width) {         // r8
  asm volatile(
    YUVTORGB
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r" (kCoefficientsRgbY)  // %5
  : "memory", "cc", CLOBBER
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
);
}

// 6 or 7 registers
void OMITFP FastConvertYUVToARGBRow4_SSE2(const uint8* y_buf,  // rdi
                                          const uint8* u_buf,  // rsi
                                          const uint8* v_buf,  // rdx
                                          uint8* rgb_buf,      // rcx
                                          int width) {         // r8
  asm volatile(
    YUVTORGB4
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r" (kCoefficientsRgbY)  // %5
  : "memory", "cc", CLOBBER
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
);
}

void OMITFP FastConvertYUVToBGRARow_SSE2(const uint8* y_buf,  // rdi
                                         const uint8* u_buf,  // rsi
                                         const uint8* v_buf,  // rdx
                                         uint8* rgb_buf,      // rcx
                                         int width) {         // r8
  asm volatile(
    YUVTORGB
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r" (kCoefficientsBgraY)  // %5
  : "memory", "cc", CLOBBER
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
);
}

void OMITFP FastConvertYUVToABGRRow_SSE2(const uint8* y_buf,  // rdi
                                         const uint8* u_buf,  // rsi
                                         const uint8* v_buf,  // rdx
                                         uint8* rgb_buf,      // rcx
                                         int width) {         // r8
  asm volatile(
    YUVTORGB
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r" (kCoefficientsAbgrY)  // %5
  : "memory", "cc", CLOBBER
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
);
}

// 6 registers
void OMITFP FastConvertYUV444ToARGBRow_SSE2(const uint8* y_buf,  // rdi
                                            const uint8* u_buf,  // rsi
                                            const uint8* v_buf,  // rdx
                                            uint8* rgb_buf,      // rcx
                                            int width) {         // r8
  asm volatile(
"1:"
  "movzb  (%1),%%"REG_a"\n"
  "lea    1(%1),%1\n"
  "movq   2048(%5,%%"REG_a",8),%%xmm0\n"
  "movzb  (%2),%%"REG_a"\n"
  "lea    1(%2),%2\n"
  "movq   4096(%5,%%"REG_a",8),%%xmm1\n"
  "paddsw %%xmm1,%%xmm0\n"
  "movzb  (%0),%%"REG_a"\n"
  "lea    1(%0),%0\n"
  "movq   0(%5,%%"REG_a",8),%%xmm2\n"
  "paddsw %%xmm0,%%xmm2\n"
  "shufps $0x44,%%xmm2,%%xmm2\n"
  "psraw  $0x6,%%xmm2\n"
  "packuswb %%xmm2,%%xmm2\n"
  "movd   %%xmm2,0x0(%3)\n"
  "lea    4(%3),%3\n"
  "sub    $0x1,%4\n"
  "ja     1b\n"
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r" (kCoefficientsRgbY)  // %5
  : "memory", "cc", "%"REG_a
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2"
#endif
);
}

// 5 registers
void FastConvertYToARGBRow_SSE2(const uint8* y_buf,  // rdi
                                uint8* rgb_buf,      // rcx
                                int width) {         // r8
  asm volatile(
"1:"
  "movzb  (%0),%%"REG_a"\n"
  "movzb  0x1(%0),%%"REG_d"\n"
  "movq   (%3,%%"REG_a",8),%%xmm2\n"
  "lea    2(%0),%0\n"
  "movhps (%3,%%"REG_d",8),%%xmm2\n"
  "psraw  $0x6,%%xmm2\n"
  "packuswb %%xmm2,%%xmm2\n"
  "movq   %%xmm2,0x0(%1)\n"
  "lea    8(%1),%1\n"
  "sub    $0x2,%2\n"
  "ja     1b\n"
  : "+r"(y_buf),    // %0
    "+r"(rgb_buf),  // %1
    "+rm"(width)    // %2
  : "r" (kCoefficientsRgbY)  // %3
  : "memory", "cc", "%"REG_a, "%"REG_d
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2"
#endif
);
}

#endif

#ifdef HAS_FASTCONVERTYUVTOARGBROW_MMX
// 32 bit mmx gcc version

#ifdef OSX
#define UNDERSCORE "_"
#else
#define UNDERSCORE ""
#endif

void FastConvertYUVToARGBRow_MMX(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width);
  asm(
  ".text\n"
#if defined(OSX) || defined(IOS)
  ".globl _FastConvertYUVToARGBRow_MMX\n"
"_FastConvertYUVToARGBRow_MMX:\n"
#else
  ".global FastConvertYUVToARGBRow_MMX\n"
"FastConvertYUVToARGBRow_MMX:\n"
#endif
  "pusha\n"
  "mov    0x24(%esp),%edx\n"
  "mov    0x28(%esp),%edi\n"
  "mov    0x2c(%esp),%esi\n"
  "mov    0x30(%esp),%ebp\n"
  "mov    0x34(%esp),%ecx\n"

"1:"
  "movzbl (%edi),%eax\n"
  "lea    1(%edi),%edi\n"
  "movzbl (%esi),%ebx\n"
  "lea    1(%esi),%esi\n"
  "movq   " UNDERSCORE "kCoefficientsRgbY+2048(,%eax,8),%mm0\n"
  "movzbl (%edx),%eax\n"
  "paddsw " UNDERSCORE "kCoefficientsRgbY+4096(,%ebx,8),%mm0\n"
  "movzbl 0x1(%edx),%ebx\n"
  "movq   " UNDERSCORE "kCoefficientsRgbY(,%eax,8),%mm1\n"
  "lea    2(%edx),%edx\n"
  "movq   " UNDERSCORE "kCoefficientsRgbY(,%ebx,8),%mm2\n"
  "paddsw %mm0,%mm1\n"
  "paddsw %mm0,%mm2\n"
  "psraw  $0x6,%mm1\n"
  "psraw  $0x6,%mm2\n"
  "packuswb %mm2,%mm1\n"
  "movq   %mm1,0x0(%ebp)\n"
  "lea    8(%ebp),%ebp\n"
  "sub    $0x2,%ecx\n"
  "ja     1b\n"
  "popa\n"
  "ret\n"
);

void FastConvertYUVToBGRARow_MMX(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width);
  asm(
  ".text\n"
#if defined(OSX) || defined(IOS)
  ".globl _FastConvertYUVToBGRARow_MMX\n"
"_FastConvertYUVToBGRARow_MMX:\n"
#else
  ".global FastConvertYUVToBGRARow_MMX\n"
"FastConvertYUVToBGRARow_MMX:\n"
#endif
  "pusha\n"
  "mov    0x24(%esp),%edx\n"
  "mov    0x28(%esp),%edi\n"
  "mov    0x2c(%esp),%esi\n"
  "mov    0x30(%esp),%ebp\n"
  "mov    0x34(%esp),%ecx\n"

"1:"
  "movzbl (%edi),%eax\n"
  "lea    1(%edi),%edi\n"
  "movzbl (%esi),%ebx\n"
  "lea    1(%esi),%esi\n"
  "movq   " UNDERSCORE "kCoefficientsBgraY+2048(,%eax,8),%mm0\n"
  "movzbl (%edx),%eax\n"
  "paddsw " UNDERSCORE "kCoefficientsBgraY+4096(,%ebx,8),%mm0\n"
  "movzbl 0x1(%edx),%ebx\n"
  "movq   " UNDERSCORE "kCoefficientsBgraY(,%eax,8),%mm1\n"
  "lea    2(%edx),%edx\n"
  "movq   " UNDERSCORE "kCoefficientsBgraY(,%ebx,8),%mm2\n"
  "paddsw %mm0,%mm1\n"
  "paddsw %mm0,%mm2\n"
  "psraw  $0x6,%mm1\n"
  "psraw  $0x6,%mm2\n"
  "packuswb %mm2,%mm1\n"
  "movq   %mm1,0x0(%ebp)\n"
  "lea    8(%ebp),%ebp\n"
  "sub    $0x2,%ecx\n"
  "ja     1b\n"
  "popa\n"
  "ret\n"
);

void FastConvertYUVToABGRRow_MMX(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* rgb_buf,
                                 int width);
  asm(
  ".text\n"
#if defined(OSX) || defined(IOS)
  ".globl _FastConvertYUVToABGRRow_MMX\n"
"_FastConvertYUVToABGRRow_MMX:\n"
#else
  ".global FastConvertYUVToABGRRow_MMX\n"
"FastConvertYUVToABGRRow_MMX:\n"
#endif
  "pusha\n"
  "mov    0x24(%esp),%edx\n"
  "mov    0x28(%esp),%edi\n"
  "mov    0x2c(%esp),%esi\n"
  "mov    0x30(%esp),%ebp\n"
  "mov    0x34(%esp),%ecx\n"

"1:"
  "movzbl (%edi),%eax\n"
  "lea    1(%edi),%edi\n"
  "movzbl (%esi),%ebx\n"
  "lea    1(%esi),%esi\n"
  "movq   " UNDERSCORE "kCoefficientsAbgrY+2048(,%eax,8),%mm0\n"
  "movzbl (%edx),%eax\n"
  "paddsw " UNDERSCORE "kCoefficientsAbgrY+4096(,%ebx,8),%mm0\n"
  "movzbl 0x1(%edx),%ebx\n"
  "movq   " UNDERSCORE "kCoefficientsAbgrY(,%eax,8),%mm1\n"
  "lea    2(%edx),%edx\n"
  "movq   " UNDERSCORE "kCoefficientsAbgrY(,%ebx,8),%mm2\n"
  "paddsw %mm0,%mm1\n"
  "paddsw %mm0,%mm2\n"
  "psraw  $0x6,%mm1\n"
  "psraw  $0x6,%mm2\n"
  "packuswb %mm2,%mm1\n"
  "movq   %mm1,0x0(%ebp)\n"
  "lea    8(%ebp),%ebp\n"
  "sub    $0x2,%ecx\n"
  "ja     1b\n"
  "popa\n"
  "ret\n"
);

void FastConvertYUV444ToARGBRow_MMX(const uint8* y_buf,
                                    const uint8* u_buf,
                                    const uint8* v_buf,
                                    uint8* rgb_buf,
                                    int width);
  asm(
  ".text\n"
#if defined(OSX) || defined(IOS)
  ".globl _FastConvertYUV444ToARGBRow_MMX\n"
"_FastConvertYUV444ToARGBRow_MMX:\n"
#else
  ".global FastConvertYUV444ToARGBRow_MMX\n"
"FastConvertYUV444ToARGBRow_MMX:\n"
#endif
  "pusha\n"
  "mov    0x24(%esp),%edx\n"
  "mov    0x28(%esp),%edi\n"
  "mov    0x2c(%esp),%esi\n"
  "mov    0x30(%esp),%ebp\n"
  "mov    0x34(%esp),%ecx\n"

"1:"
  "movzbl (%edi),%eax\n"
  "lea    1(%edi),%edi\n"
  "movzbl (%esi),%ebx\n"
  "lea    1(%esi),%esi\n"
  "movq   " UNDERSCORE "kCoefficientsRgbY+2048(,%eax,8),%mm0\n"
  "movzbl (%edx),%eax\n"
  "paddsw " UNDERSCORE "kCoefficientsRgbY+4096(,%ebx,8),%mm0\n"
  "lea    1(%edx),%edx\n"
  "paddsw " UNDERSCORE "kCoefficientsRgbY(,%eax,8),%mm0\n"
  "psraw  $0x6,%mm0\n"
  "packuswb %mm0,%mm0\n"
  "movd   %mm0,0x0(%ebp)\n"
  "lea    4(%ebp),%ebp\n"
  "sub    $0x1,%ecx\n"
  "ja     1b\n"
  "popa\n"
  "ret\n"
);

void FastConvertYToARGBRow_MMX(const uint8* y_buf,
                               uint8* rgb_buf,
                               int width);
  asm(
  ".text\n"
#if defined(OSX) || defined(IOS)
  ".globl _FastConvertYToARGBRow_MMX\n"
"_FastConvertYToARGBRow_MMX:\n"
#else
  ".global FastConvertYToARGBRow_MMX\n"
"FastConvertYToARGBRow_MMX:\n"
#endif
  "push   %ebx\n"
  "mov    0x8(%esp),%eax\n"
  "mov    0xc(%esp),%edx\n"
  "mov    0x10(%esp),%ecx\n"

"1:"
  "movzbl (%eax),%ebx\n"
  "movq   " UNDERSCORE "kCoefficientsRgbY(,%ebx,8),%mm0\n"
  "psraw  $0x6,%mm0\n"
  "movzbl 0x1(%eax),%ebx\n"
  "movq   " UNDERSCORE "kCoefficientsRgbY(,%ebx,8),%mm1\n"
  "psraw  $0x6,%mm1\n"
  "packuswb %mm1,%mm0\n"
  "lea    0x2(%eax),%eax\n"
  "movq   %mm0,(%edx)\n"
  "lea    0x8(%edx),%edx\n"
  "sub    $0x2,%ecx\n"
  "ja     1b\n"
  "pop    %ebx\n"
  "ret\n"
);

#endif

#ifdef HAS_ARGBTOYROW_SSSE3
void ABGRToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  ABGRToARGBRow_SSSE3(src_argb, row, pix);
  ARGBToYRow_SSSE3(row, dst_y, pix);
}

void BGRAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  BGRAToARGBRow_SSSE3(src_argb, row, pix);
  ARGBToYRow_SSSE3(row, dst_y, pix);
}
#endif

#ifdef HAS_ARGBTOUVROW_SSSE3
void ABGRToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  ABGRToARGBRow_SSSE3(src_argb, row, pix);
  ABGRToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}

void BGRAToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  BGRAToARGBRow_SSSE3(src_argb, row, pix);
  BGRAToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}
#endif

}  // extern "C"

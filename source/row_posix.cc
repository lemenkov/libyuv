/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "source/row.h"

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// This module is for GCC x86 and x64
#if !defined(YUV_DISABLE_ASM) && (defined(__x86_64__) || defined(__i386__))

// GCC 4.2 on OSX has link error when passing static or const to inline.
// TODO(fbarchard): Use static const when gcc 4.2 support is dropped.
#ifdef __APPLE__
#define CONST
#else
#define CONST static const
#endif

#ifdef HAS_ARGBTOYROW_SSSE3

// Constants for ARGB
CONST vec8 kARGBToY = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};

CONST vec8 kARGBToU = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

CONST vec8 kARGBToV = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};

// Constants for BGRA
CONST vec8 kBGRAToY = {
  0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13
};

CONST vec8 kBGRAToU = {
  0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112
};

CONST vec8 kBGRAToV = {
  0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18
};

// Constants for ABGR
CONST vec8 kABGRToY = {
  33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0
};

CONST vec8 kABGRToU = {
  -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0
};

CONST vec8 kABGRToV = {
  112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0
};

CONST uvec8 kAddY16 = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u
};

CONST uvec8 kAddUV128 = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};

// Shuffle table for converting RGB24 to ARGB.
CONST uvec8 kShuffleMaskRGB24ToARGB = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};

// Shuffle table for converting RAW to ARGB.
CONST uvec8 kShuffleMaskRAWToARGB = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};

// Shuffle table for converting ABGR to ARGB.
CONST uvec8 kShuffleMaskABGRToARGB = {
  2u, 1u, 0u, 3u, 6u, 5u, 4u, 7u, 10u, 9u, 8u, 11u, 14u, 13u, 12u, 15u
};

// Shuffle table for converting BGRA to ARGB.
CONST uvec8 kShuffleMaskBGRAToARGB = {
  3u, 2u, 1u, 0u, 7u, 6u, 5u, 4u, 11u, 10u, 9u, 8u, 15u, 14u, 13u, 12u
};

// Shuffle table for converting ARGB to RGB24.
CONST uvec8 kShuffleMaskARGBToRGB24 = {
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u, 128u, 128u, 128u, 128u
};

// Shuffle table for converting ARGB to RAW.
CONST uvec8 kShuffleMaskARGBToRAW = {
  2u, 1u, 0u, 6u, 5u, 4u, 10u, 9u, 8u, 14u, 13u, 12u, 128u, 128u, 128u, 128u
};

void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pslld     $0x18,%%xmm5                    \n"
  "1:                                          \n"
    "movq      (%0),%%xmm0                     \n"
    "lea       0x8(%0),%0                      \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm0                   \n"
    "punpckhwd %%xmm1,%%xmm1                   \n"
    "por       %%xmm5,%%xmm0                   \n"
    "por       %%xmm5,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "lea       0x20(%1),%1                     \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
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
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "sub       %0,%1                           \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"

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
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "sub       %0,%1                           \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
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

void RGB24ToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"  // generate mask 0xff000000
    "pslld     $0x18,%%xmm5                    \n"
    "movdqa    %3,%%xmm4                       \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm3                 \n"
    "lea       0x30(%0),%0                     \n"
    "movdqa    %%xmm3,%%xmm2                   \n"
    "palignr   $0x8,%%xmm1,%%xmm2              \n"
    "pshufb    %%xmm4,%%xmm2                   \n"
    "por       %%xmm5,%%xmm2                   \n"
    "palignr   $0xc,%%xmm0,%%xmm1              \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqa    %%xmm2,0x20(%1)                 \n"
    "por       %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "por       %%xmm5,%%xmm1                   \n"
    "palignr   $0x4,%%xmm3,%%xmm3              \n"
    "pshufb    %%xmm4,%%xmm3                   \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "por       %%xmm5,%%xmm3                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm3,0x30(%1)                 \n"
    "lea       0x40(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_rgb24),  // %0
    "+r"(dst_argb),  // %1
    "+r"(pix)        // %2
  : "m"(kShuffleMaskRGB24ToARGB)  // %3
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void RAWToARGBRow_SSSE3(const uint8* src_raw, uint8* dst_argb, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"  // generate mask 0xff000000
    "pslld     $0x18,%%xmm5                    \n"
    "movdqa    %3,%%xmm4                       \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm3                 \n"
    "lea       0x30(%0),%0                     \n"
    "movdqa    %%xmm3,%%xmm2                   \n"
    "palignr   $0x8,%%xmm1,%%xmm2              \n"
    "pshufb    %%xmm4,%%xmm2                   \n"
    "por       %%xmm5,%%xmm2                   \n"
    "palignr   $0xc,%%xmm0,%%xmm1              \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqa    %%xmm2,0x20(%1)                 \n"
    "por       %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "por       %%xmm5,%%xmm1                   \n"
    "palignr   $0x4,%%xmm3,%%xmm3              \n"
    "pshufb    %%xmm4,%%xmm3                   \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "por       %%xmm5,%%xmm3                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm3,0x30(%1)                 \n"
    "lea       0x40(%1),%1                     \n"
    "jg        1b                              \n"
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

void RGB565ToARGBRow_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "mov       $0x1080108,%%eax                \n"
    "movd      %%eax,%%xmm5                    \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    "mov       $0x20082008,%%eax               \n"
    "movd      %%eax,%%xmm6                    \n"
    "pshufd    $0x0,%%xmm6,%%xmm6              \n"
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "psllw     $0xb,%%xmm3                     \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psllw     $0xa,%%xmm4                     \n"
    "psrlw     $0x5,%%xmm4                     \n"
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psllw     $0x8,%%xmm7                     \n"
    "sub       %0,%1                           \n"
    "sub       %0,%1                           \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pand      %%xmm3,%%xmm1                   \n"
    "psllw     $0xb,%%xmm2                     \n"
    "pmulhuw   %%xmm5,%%xmm1                   \n"
    "pmulhuw   %%xmm5,%%xmm2                   \n"
    "psllw     $0x8,%%xmm1                     \n"
    "por       %%xmm2,%%xmm1                   \n"
    "pand      %%xmm4,%%xmm0                   \n"
    "pmulhuw   %%xmm6,%%xmm0                   \n"
    "por       %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpckhbw %%xmm0,%%xmm2                   \n"
    "movdqa    %%xmm1,(%1,%0,2)                \n"
    "movdqa    %%xmm2,0x10(%1,%0,2)            \n"
    "lea       0x10(%0),%0                     \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(pix)   // %2
  :
  : "memory", "cc", "eax"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ARGB1555ToARGBRow_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "mov       $0x1080108,%%eax                \n"
    "movd      %%eax,%%xmm5                    \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    "mov       $0x42004200,%%eax               \n"
    "movd      %%eax,%%xmm6                    \n"
    "pshufd    $0x0,%%xmm6,%%xmm6              \n"
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "psllw     $0xb,%%xmm3                     \n"
    "movdqa    %%xmm3,%%xmm4                   \n"
    "psrlw     $0x6,%%xmm4                     \n"
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psllw     $0x8,%%xmm7                     \n"
    "sub       %0,%1                           \n"
    "sub       %0,%1                           \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "psllw     $0x1,%%xmm1                     \n"
    "psllw     $0xb,%%xmm2                     \n"
    "pand      %%xmm3,%%xmm1                   \n"
    "pmulhuw   %%xmm5,%%xmm2                   \n"
    "pmulhuw   %%xmm5,%%xmm1                   \n"
    "psllw     $0x8,%%xmm1                     \n"
    "por       %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pand      %%xmm4,%%xmm0                   \n"
    "psraw     $0x8,%%xmm2                     \n"
    "pmulhuw   %%xmm6,%%xmm0                   \n"
    "pand      %%xmm7,%%xmm2                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpckhbw %%xmm0,%%xmm2                   \n"
    "movdqa    %%xmm1,(%1,%0,2)                \n"
    "movdqa    %%xmm2,0x10(%1,%0,2)            \n"
    "lea       0x10(%0),%0                     \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(pix)   // %2
  :
  : "memory", "cc", "eax"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ARGB4444ToARGBRow_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "mov       $0xf0f0f0f,%%eax                \n"
    "movd      %%eax,%%xmm4                    \n"
    "pshufd    $0x0,%%xmm4,%%xmm4              \n"
    "movdqa    %%xmm4,%%xmm5                   \n"
    "pslld     $0x4,%%xmm5                     \n"
    "sub       %0,%1                           \n"
    "sub       %0,%1                           \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pand      %%xmm4,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm3                   \n"
    "psllw     $0x4,%%xmm1                     \n"
    "psrlw     $0x4,%%xmm3                     \n"
    "por       %%xmm1,%%xmm0                   \n"
    "por       %%xmm3,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm0                   \n"
    "punpckhbw %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1,%0,2)                \n"
    "movdqa    %%xmm1,0x10(%1,%0,2)            \n"
    "lea       0x10(%0),%0                     \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(pix)   // %2
  :
  : "memory", "cc", "eax"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ARGBToRGB24Row_SSSE3(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm6                       \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "lea       0x40(%0),%0                     \n"
    "pshufb    %%xmm6,%%xmm0                   \n"
    "pshufb    %%xmm6,%%xmm1                   \n"
    "pshufb    %%xmm6,%%xmm2                   \n"
    "pshufb    %%xmm6,%%xmm3                   \n"
    "movdqa    %%xmm1,%%xmm4                   \n"
    "psrldq    $0x4,%%xmm1                     \n"
    "pslldq    $0xc,%%xmm4                     \n"
    "movdqa    %%xmm2,%%xmm5                   \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pslldq    $0x8,%%xmm5                     \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "por       %%xmm5,%%xmm1                   \n"
    "psrldq    $0x8,%%xmm2                     \n"
    "pslldq    $0x4,%%xmm3                     \n"
    "por       %%xmm3,%%xmm2                   \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "movdqa    %%xmm2,0x20(%1)                 \n"
    "lea       0x30(%1),%1                     \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(pix)   // %2
  : "m"(kShuffleMaskARGBToRGB24)  // %3
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}

void ARGBToRAWRow_SSSE3(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm6                       \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "lea       0x40(%0),%0                     \n"
    "pshufb    %%xmm6,%%xmm0                   \n"
    "pshufb    %%xmm6,%%xmm1                   \n"
    "pshufb    %%xmm6,%%xmm2                   \n"
    "pshufb    %%xmm6,%%xmm3                   \n"
    "movdqa    %%xmm1,%%xmm4                   \n"
    "psrldq    $0x4,%%xmm1                     \n"
    "pslldq    $0xc,%%xmm4                     \n"
    "movdqa    %%xmm2,%%xmm5                   \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pslldq    $0x8,%%xmm5                     \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "por       %%xmm5,%%xmm1                   \n"
    "psrldq    $0x8,%%xmm2                     \n"
    "pslldq    $0x4,%%xmm3                     \n"
    "por       %%xmm3,%%xmm2                   \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "movdqa    %%xmm2,0x20(%1)                 \n"
    "lea       0x30(%1),%1                     \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(pix)   // %2
  : "m"(kShuffleMaskARGBToRAW)  // %3
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}

void ARGBToRGB565Row_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "psrld     $0x1b,%%xmm3                    \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psrld     $0x1a,%%xmm4                    \n"
    "pslld     $0x5,%%xmm4                     \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pslld     $0xb,%%xmm5                     \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pslld     $0x8,%%xmm0                     \n"
    "psrld     $0x3,%%xmm1                     \n"
    "psrld     $0x5,%%xmm2                     \n"
    "psrad     $0x10,%%xmm0                    \n"
    "pand      %%xmm3,%%xmm1                   \n"
    "pand      %%xmm4,%%xmm2                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "por       %%xmm2,%%xmm1                   \n"
    "por       %%xmm1,%%xmm0                   \n"
    "packssdw  %%xmm0,%%xmm0                   \n"
    "lea       0x10(%0),%0                     \n"
    "movq      %%xmm0,(%1)                     \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x4,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(pix)   // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ARGBToARGB1555Row_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psrld     $0x1b,%%xmm4                    \n"
    "movdqa    %%xmm4,%%xmm5                   \n"
    "pslld     $0x5,%%xmm5                     \n"
    "movdqa    %%xmm4,%%xmm6                   \n"
    "pslld     $0xa,%%xmm6                     \n"
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "pslld     $0xf,%%xmm7                     \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm3                   \n"
    "psrad     $0x10,%%xmm0                    \n"
    "psrld     $0x3,%%xmm1                     \n"
    "psrld     $0x6,%%xmm2                     \n"
    "psrld     $0x9,%%xmm3                     \n"
    "pand      %%xmm7,%%xmm0                   \n"
    "pand      %%xmm4,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm2                   \n"
    "pand      %%xmm6,%%xmm3                   \n"
    "por       %%xmm1,%%xmm0                   \n"
    "por       %%xmm3,%%xmm2                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "packssdw  %%xmm0,%%xmm0                   \n"
    "lea       0x10(%0),%0                     \n"
    "movq      %%xmm0,(%1)                     \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x4,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(pix)   // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ARGBToARGB4444Row_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psllw     $0xc,%%xmm4                     \n"
    "movdqa    %%xmm4,%%xmm3                   \n"
    "psrlw     $0x8,%%xmm3                     \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm3,%%xmm0                   \n"
    "pand      %%xmm4,%%xmm1                   \n"
    "psrlq     $0x4,%%xmm0                     \n"
    "psrlq     $0x8,%%xmm1                     \n"
    "por       %%xmm1,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "lea       0x10(%0),%0                     \n"
    "movq      %%xmm0,(%1)                     \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x4,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(pix)   // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
#endif
  );
}

void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
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

void ARGBToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
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

// TODO(fbarchard): pass xmm constants to single block of assembly.
// fpic on GCC 4.2 for OSX runs out of GPR registers.  "m" effectively takes
// 3 registers - ebx, ebp and eax.  "m" can be passed with 3 normal registers,
// or 4 if stack frame is disabled.  Doing 2 assembly blocks is a work around
// and considered unsafe.
void ARGBToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kARGBToU),         // %0
    "m"(kARGBToV),         // %1
    "m"(kAddUV128)         // %2
  :
#if defined(__SSE2__)
    "xmm3", "xmm4", "xmm5"
#endif
  );
  asm volatile (
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm6                 \n"
    "pavgb     (%0,%4,1),%%xmm0                \n"
    "pavgb     0x10(%0,%4,1),%%xmm1            \n"
    "pavgb     0x20(%0,%4,1),%%xmm2            \n"
    "pavgb     0x30(%0,%4,1),%%xmm6            \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"(static_cast<intptr_t>(src_stride_argb))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ARGBToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kARGBToU),         // %0
    "m"(kARGBToV),         // %1
    "m"(kAddUV128)         // %2
  :
#if defined(__SSE2__)
    "xmm3", "xmm4", "xmm5"
#endif
  );
  asm volatile (
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm6                 \n"
    "movdqu    (%0,%4,1),%%xmm7                \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqu    0x10(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm1                   \n"
    "movdqu    0x20(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqu    0x30(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"(static_cast<intptr_t>(src_stride_argb))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void BGRAToYRow_SSSE3(const uint8* src_bgra, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_bgra),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  : "m"(kBGRAToY),   // %3
    "m"(kAddY16)     // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void BGRAToYRow_Unaligned_SSSE3(const uint8* src_bgra, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_bgra),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  : "m"(kBGRAToY),   // %3
    "m"(kAddY16)     // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void BGRAToUVRow_SSSE3(const uint8* src_bgra0, int src_stride_bgra,
                       uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kBGRAToU),         // %0
    "m"(kBGRAToV),         // %1
    "m"(kAddUV128)         // %2
  :
#if defined(__SSE2__)
    "xmm3", "xmm4", "xmm5"
#endif
  );
  asm volatile (
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm6                 \n"
    "pavgb     (%0,%4,1),%%xmm0                \n"
    "pavgb     0x10(%0,%4,1),%%xmm1            \n"
    "pavgb     0x20(%0,%4,1),%%xmm2            \n"
    "pavgb     0x30(%0,%4,1),%%xmm6            \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_bgra0),       // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"(static_cast<intptr_t>(src_stride_bgra))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void BGRAToUVRow_Unaligned_SSSE3(const uint8* src_bgra0, int src_stride_bgra,
                                 uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kBGRAToU),         // %0
    "m"(kBGRAToV),         // %1
    "m"(kAddUV128)         // %2
  :
#if defined(__SSE2__)
    "xmm3", "xmm4", "xmm5"
#endif
  );
  asm volatile (
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm6                 \n"
    "movdqu    (%0,%4,1),%%xmm7                \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqu    0x10(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm1                   \n"
    "movdqu    0x20(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqu    0x30(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_bgra0),       // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"(static_cast<intptr_t>(src_stride_bgra))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ABGRToYRow_SSSE3(const uint8* src_abgr, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_abgr),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  : "m"(kABGRToY),   // %3
    "m"(kAddY16)     // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ABGRToYRow_Unaligned_SSSE3(const uint8* src_abgr, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_abgr),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  : "m"(kABGRToY),   // %3
    "m"(kAddY16)     // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ABGRToUVRow_SSSE3(const uint8* src_abgr0, int src_stride_abgr,
                       uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kABGRToU),         // %0
    "m"(kABGRToV),         // %1
    "m"(kAddUV128)         // %2
  :
#if defined(__SSE2__)
    "xmm3", "xmm4", "xmm5"
#endif
  );
  asm volatile (
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm6                 \n"
    "pavgb     (%0,%4,1),%%xmm0                \n"
    "pavgb     0x10(%0,%4,1),%%xmm1            \n"
    "pavgb     0x20(%0,%4,1),%%xmm2            \n"
    "pavgb     0x30(%0,%4,1),%%xmm6            \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_abgr0),       // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"(static_cast<intptr_t>(src_stride_abgr))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ABGRToUVRow_Unaligned_SSSE3(const uint8* src_abgr0, int src_stride_abgr,
                                 uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kABGRToU),         // %0
    "m"(kABGRToV),         // %1
    "m"(kAddUV128)         // %2
  :
#if defined(__SSE2__)
    "xmm3", "xmm4", "xmm5"
#endif
  );
  asm volatile (
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm6                 \n"
    "movdqu    (%0,%4,1),%%xmm7                \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqu    0x10(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm1                   \n"
    "movdqu    0x20(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqu    0x30(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_abgr0),       // %0
    "+r"(dst_u),           // %1
    "+r"(dst_v),           // %2
    "+rm"(width)           // %3
  : "r"(static_cast<intptr_t>(src_stride_abgr))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

#endif  // HAS_ARGBTOYROW_SSSE3

#ifdef HAS_I420TOARGBROW_SSSE3
#define UB 127 /* min(63,static_cast<int8>(2.018 * 64)) */
#define UG -25 /* static_cast<int8>(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* static_cast<int8>(-0.813 * 64 - 0.5) */
#define VR 102 /* static_cast<int8>(1.596 * 64 + 0.5) */

// Bias
#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

#define YG 74 /* static_cast<int8>(1.164 * 64 + 0.5) */

#if defined(__APPLE__) || defined(__x86_64__)
#define OMITFP
#else
#define OMITFP __attribute__((optimize("omit-frame-pointer")))
#endif

struct {
  vec8 kUVToB;
  vec8 kUVToG;
  vec8 kUVToR;
  vec16 kUVBiasB;
  vec16 kUVBiasG;
  vec16 kUVBiasR;
  vec16 kYSub16;
  vec16 kYToRgb;
} CONST SIMD_ALIGNED(kYuvConstants) = {
  { UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB },
  { UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG },
  { UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR },
  { BB, BB, BB, BB, BB, BB, BB, BB },
  { BG, BG, BG, BG, BG, BG, BG, BG },
  { BR, BR, BR, BR, BR, BR, BR, BR },
  { 16, 16, 16, 16, 16, 16, 16, 16 },
  { YG, YG, YG, YG, YG, YG, YG, YG }
};

// Convert 8 pixels
#define YUVTORGB                                                               \
    "movd       (%1),%%xmm0                    \n"                             \
    "movd       (%1,%2,1),%%xmm1               \n"                             \
    "lea        0x4(%1),%1                     \n"                             \
    "punpcklbw  %%xmm1,%%xmm0                  \n"                             \
    "punpcklwd  %%xmm0,%%xmm0                  \n"                             \
    "movdqa     %%xmm0,%%xmm1                  \n"                             \
    "movdqa     %%xmm0,%%xmm2                  \n"                             \
    "pmaddubsw  (%5),%%xmm0                    \n"                             \
    "pmaddubsw  16(%5),%%xmm1                  \n"                             \
    "pmaddubsw  32(%5),%%xmm2                  \n"                             \
    "psubw      48(%5),%%xmm0                  \n"                             \
    "psubw      64(%5),%%xmm1                  \n"                             \
    "psubw      80(%5),%%xmm2                  \n"                             \
    "movq       (%0),%%xmm3                    \n"                             \
    "lea        0x8(%0),%0                     \n"                             \
    "punpcklbw  %%xmm4,%%xmm3                  \n"                             \
    "psubsw     96(%5),%%xmm3                  \n"                             \
    "pmullw     112(%5),%%xmm3                 \n"                             \
    "paddsw     %%xmm3,%%xmm0                  \n"                             \
    "paddsw     %%xmm3,%%xmm1                  \n"                             \
    "paddsw     %%xmm3,%%xmm2                  \n"                             \
    "psraw      $0x6,%%xmm0                    \n"                             \
    "psraw      $0x6,%%xmm1                    \n"                             \
    "psraw      $0x6,%%xmm2                    \n"                             \
    "packuswb   %%xmm0,%%xmm0                  \n"                             \
    "packuswb   %%xmm1,%%xmm1                  \n"                             \
    "packuswb   %%xmm2,%%xmm2                  \n"

void OMITFP I420ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* rgb_buf,
                                int width) {
  asm volatile (
    "sub       %1,%2                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
  "1:                                          \n"
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,(%3)                     \n"
    "movdqa    %%xmm1,0x10(%3)                 \n"
    "lea       0x20(%3),%3                     \n"
    "sub       $0x8,%4                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r"(&kYuvConstants.kUVToB) // %5
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I420ToBGRARow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* rgb_buf,
                                int width) {
  asm volatile (
    "sub       %1,%2                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
  "1:                                          \n"
    YUVTORGB
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm5                   \n"
    "movdqa    %%xmm5,%%xmm0                   \n"
    "punpcklwd %%xmm1,%%xmm5                   \n"
    "punpckhwd %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm5,(%3)                     \n"
    "movdqa    %%xmm0,0x10(%3)                 \n"
    "lea       0x20(%3),%3                     \n"
    "sub       $0x8,%4                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r"(&kYuvConstants.kUVToB) // %5
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I420ToABGRRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* rgb_buf,
                                int width) {
  asm volatile (
    "sub       %1,%2                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
  "1:                                          \n"
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm2                   \n"
    "punpckhwd %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,(%3)                     \n"
    "movdqa    %%xmm1,0x10(%3)                 \n"
    "lea       0x20(%3),%3                     \n"
    "sub       $0x8,%4                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r"(&kYuvConstants.kUVToB) // %5
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I420ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* rgb_buf,
                                          int width) {
  asm volatile (
    "sub       %1,%2                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
  "1:                                          \n"
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0,(%3)                     \n"
    "movdqu    %%xmm1,0x10(%3)                 \n"
    "lea       0x20(%3),%3                     \n"
    "sub       $0x8,%4                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r"(&kYuvConstants.kUVToB) // %5
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I420ToBGRARow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* rgb_buf,
                                          int width) {
  asm volatile (
    "sub       %1,%2                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
  "1:                                          \n"
    YUVTORGB
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm5                   \n"
    "movdqa    %%xmm5,%%xmm0                   \n"
    "punpcklwd %%xmm1,%%xmm5                   \n"
    "punpckhwd %%xmm1,%%xmm0                   \n"
    "movdqu    %%xmm5,(%3)                     \n"
    "movdqu    %%xmm0,0x10(%3)                 \n"
    "lea       0x20(%3),%3                     \n"
    "sub       $0x8,%4                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r"(&kYuvConstants.kUVToB) // %5
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I420ToABGRRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* rgb_buf,
                                          int width) {
  asm volatile (
    "sub       %1,%2                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
  "1:                                          \n"
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm2                   \n"
    "punpckhwd %%xmm0,%%xmm1                   \n"
    "movdqu    %%xmm2,(%3)                     \n"
    "movdqu    %%xmm1,0x10(%3)                 \n"
    "lea       0x20(%3),%3                     \n"
    "sub       $0x8,%4                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r"(&kYuvConstants.kUVToB) // %5
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I444ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* rgb_buf,
                                int width) {
  asm volatile (
    "sub       %1,%2                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
  "1:                                          \n"
    "movd      (%1),%%xmm0                     \n"
    "movd      (%1,%2,1),%%xmm1                \n"
    "lea       0x4(%1),%1                      \n"
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pmaddubsw (%5),%%xmm0                     \n"
    "pmaddubsw 16(%5),%%xmm1                   \n"
    "pmaddubsw 32(%5),%%xmm2                   \n"
    "psubw     48(%5),%%xmm0                   \n"
    "psubw     64(%5),%%xmm1                   \n"
    "psubw     80(%5),%%xmm2                   \n"
    "movd      (%0),%%xmm3                     \n"
    "lea       0x4(%0),%0                      \n"
    "punpcklbw %%xmm4,%%xmm3                   \n"
    "psubsw    96(%5),%%xmm3                   \n"
    "pmullw    112(%5),%%xmm3                  \n"
    "paddsw    %%xmm3,%%xmm0                   \n"
    "paddsw    %%xmm3,%%xmm1                   \n"
    "paddsw    %%xmm3,%%xmm2                   \n"
    "psraw     $0x6,%%xmm0                     \n"
    "psraw     $0x6,%%xmm1                     \n"
    "psraw     $0x6,%%xmm2                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "packuswb  %%xmm2,%%xmm2                   \n"
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%4                         \n"
    "movdqa    %%xmm0,(%3)                     \n"
    "lea       0x10(%3),%3                     \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    // %0
    "+r"(u_buf),    // %1
    "+r"(v_buf),    // %2
    "+r"(rgb_buf),  // %3
    "+rm"(width)    // %4
  : "r"(&kYuvConstants.kUVToB) // %5
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif

#ifdef HAS_YTOARGBROW_SSE2
void YToARGBRow_SSE2(const uint8* y_buf,
                     uint8* rgb_buf,
                     int width) {
  asm volatile (
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"
    "mov       $0x10001000,%%eax               \n"
    "movd      %%eax,%%xmm3                    \n"
    "pshufd    $0x0,%%xmm3,%%xmm3              \n"
    "mov       $0x012a012a,%%eax               \n"
    "movd      %%eax,%%xmm2                    \n"
    "pshufd    $0x0,%%xmm2,%%xmm2              \n"
  "1:                                          \n"
    // Step 1: Scale Y contribution to 8 G values. G = (y - 16) * 1.164
    "movq      (%0),%%xmm0                     \n"
    "lea       0x8(%0),%0                      \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "psubusw   %%xmm3,%%xmm0                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"

    // Step 2: Weave into ARGB
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm0                   \n"
    "punpckhwd %%xmm1,%%xmm1                   \n"
    "por       %%xmm4,%%xmm0                   \n"
    "por       %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "movdqa    %%xmm1,16(%1)                   \n"
    "lea       32(%1),%1                       \n"

    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    // %0
    "+r"(rgb_buf),  // %1
    "+rm"(width)    // %2
  :
  : "memory", "cc", "eax"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
#endif
  );
}
#endif

#ifdef HAS_MIRRORROW_SSSE3
// Shuffle table for reversing the bytes.
CONST uvec8 kShuffleMirror = {
  15u, 14u, 13u, 12u, 11u, 10u, 9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u
};

void MirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
  intptr_t temp_width = static_cast<intptr_t>(width);
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "lea       -0x10(%0),%0                    \n"
  "1:                                          \n"
    "movdqa    (%0,%2),%%xmm0                  \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(temp_width)  // %2
  : "m"(kShuffleMirror) // %3
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
  );
}
#endif

#ifdef HAS_MIRRORROW_SSE2
void MirrorRow_SSE2(const uint8* src, uint8* dst, int width) {
  intptr_t temp_width = static_cast<intptr_t>(width);
  asm volatile (
    "lea       -0x10(%0),%0                    \n"
  "1:                                          \n"
    "movdqu    (%0,%2),%%xmm0                  \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "psllw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm1,%%xmm0                   \n"
    "pshuflw   $0x1b,%%xmm0,%%xmm0             \n"
    "pshufhw   $0x1b,%%xmm0,%%xmm0             \n"
    "pshufd    $0x4e,%%xmm0,%%xmm0             \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src),  // %0
    "+r"(dst),  // %1
    "+r"(temp_width)  // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif

#ifdef HAS_MIRRORROW_UV_SSSE3
// Shuffle table for reversing the bytes of UV channels.
CONST uvec8 kShuffleMirrorUV = {
  14u, 12u, 10u, 8u, 6u, 4u, 2u, 0u, 15u, 13u, 11u, 9u, 7u, 5u, 3u, 1u
};
void MirrorRowUV_SSSE3(const uint8* src, uint8* dst_u, uint8* dst_v,
                       int width) {
  intptr_t temp_width = static_cast<intptr_t>(width);
  asm volatile (
    "movdqa    %4,%%xmm1                       \n"
    "lea       -16(%0,%3,2),%0                 \n"
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "lea       -16(%0),%0                      \n"
    "pshufb    %%xmm1,%%xmm0                   \n"
    "sub       $8,%3                           \n"
    "movlpd    %%xmm0,(%1)                     \n"
    "movhpd    %%xmm0,(%1,%2)                  \n"
    "lea       8(%1),%1                        \n"
    "jg        1b                              \n"
  : "+r"(src),      // %0
    "+r"(dst_u),    // %1
    "+r"(dst_v),    // %2
    "+r"(temp_width)  // %3
  : "m"(kShuffleMirrorUV) // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif

#ifdef HAS_SPLITUV_SSE2
void SplitUV_SSE2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb    %%xmm5,%%xmm5                    \n"
    "psrlw      $0x8,%%xmm5                      \n"
    "sub        %1,%2                            \n"
  "1:                                            \n"
    "movdqa     (%0),%%xmm0                      \n"
    "movdqa     0x10(%0),%%xmm1                  \n"
    "lea        0x20(%0),%0                      \n"
    "movdqa     %%xmm0,%%xmm2                    \n"
    "movdqa     %%xmm1,%%xmm3                    \n"
    "pand       %%xmm5,%%xmm0                    \n"
    "pand       %%xmm5,%%xmm1                    \n"
    "packuswb   %%xmm1,%%xmm0                    \n"
    "psrlw      $0x8,%%xmm2                      \n"
    "psrlw      $0x8,%%xmm3                      \n"
    "packuswb   %%xmm3,%%xmm2                    \n"
    "movdqa     %%xmm0,(%1)                      \n"
    "movdqa     %%xmm2,(%1,%2)                   \n"
    "lea        0x10(%1),%1                      \n"
    "sub        $0x10,%3                         \n"
    "jg         1b                               \n"
  : "+r"(src_uv),     // %0
    "+r"(dst_u),      // %1
    "+r"(dst_v),      // %2
    "+r"(pix)         // %3
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}
#endif

#ifdef HAS_COPYROW_SSE2
void CopyRow_SSE2(const uint8* src, uint8* dst, int count) {
  asm volatile (
    "sub        %0,%1                          \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    %%xmm0,(%0,%1)                  \n"
    "movdqa    %%xmm1,0x10(%0,%1)              \n"
    "lea       0x20(%0),%0                     \n"
    "sub       $0x20,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src),   // %0
    "+r"(dst),   // %1
    "+r"(count)  // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif  // HAS_COPYROW_SSE2

#ifdef HAS_COPYROW_X86
void CopyRow_X86(const uint8* src, uint8* dst, int width) {
  size_t width_tmp = static_cast<size_t>(width);
  asm volatile (
    "shr       $0x2,%2                         \n"
    "rep movsl                                 \n"
  : "+S"(src),  // %0
    "+D"(dst),  // %1
    "+c"(width_tmp) // %2
  :
  : "memory", "cc"
  );
}
#endif

#ifdef HAS_YUY2TOYROW_SSE2
void YUY2ToYRow_SSE2(const uint8* src_yuy2, uint8* dst_y, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void YUY2ToUVRow_SSE2(const uint8* src_yuy2, int stride_yuy2,
                      uint8* dst_u, uint8* dst_y, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    (%0,%4,1),%%xmm2                \n"
    "movdqa    0x10(%0,%4,1),%%xmm3            \n"
    "lea       0x20(%0),%0                     \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    // %0
    "+r"(dst_u),       // %1
    "+r"(dst_y),       // %2
    "+r"(pix)          // %3
  : "r"(static_cast<intptr_t>(stride_yuy2))  // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}


void YUY2ToYRow_Unaligned_SSE2(const uint8* src_yuy2,
                               uint8* dst_y, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void YUY2ToUVRow_Unaligned_SSE2(const uint8* src_yuy2,
                                int stride_yuy2,
                                uint8* dst_u, uint8* dst_y,
                                int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    (%0,%4,1),%%xmm2                \n"
    "movdqu    0x10(%0,%4,1),%%xmm3            \n"
    "lea       0x20(%0),%0                     \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    // %0
    "+r"(dst_u),       // %1
    "+r"(dst_y),       // %2
    "+r"(pix)          // %3
  : "r"(static_cast<intptr_t>(stride_yuy2))  // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void UYVYToYRow_SSE2(const uint8* src_uyvy, uint8* dst_y, int pix) {
  asm volatile (
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void UYVYToUVRow_SSE2(const uint8* src_uyvy, int stride_uyvy,
                      uint8* dst_u, uint8* dst_y, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    (%0,%4,1),%%xmm2                \n"
    "movdqa    0x10(%0,%4,1),%%xmm3            \n"
    "lea       0x20(%0),%0                     \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    // %0
    "+r"(dst_u),       // %1
    "+r"(dst_y),       // %2
    "+r"(pix)          // %3
  : "r"(static_cast<intptr_t>(stride_uyvy))  // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void UYVYToYRow_Unaligned_SSE2(const uint8* src_uyvy,
                               uint8* dst_y, int pix) {
  asm volatile (
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),  // %0
    "+r"(dst_y),     // %1
    "+r"(pix)        // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void UYVYToUVRow_Unaligned_SSE2(const uint8* src_uyvy, int stride_uyvy,
                                uint8* dst_u, uint8* dst_y, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    (%0,%4,1),%%xmm2                \n"
    "movdqu    0x10(%0,%4,1),%%xmm3            \n"
    "lea       0x20(%0),%0                     \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    // %0
    "+r"(dst_u),       // %1
    "+r"(dst_y),       // %2
    "+r"(pix)          // %3
  : "r"(static_cast<intptr_t>(stride_uyvy))  // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}
#endif  // HAS_YUY2TOYROW_SSE2

#ifdef HAS_ARGBBLENDROW_SSE2
// Blend 8 pixels at a time.
// src_argb0 unaligned.
// src_argb1 and dst_argb aligned to 16 bytes.
// width must be multiple of 4 pixels.
void ARGBBlendRow_Aligned_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                               uint8* dst_argb, int width) {
  asm volatile (
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psrlw     $0xf,%%xmm7                     \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrlw     $0x8,%%xmm6                     \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psllw     $0x8,%%xmm5                     \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"

  // 8 pixel loop
  "1:                                          \n"
    "movdqu    (%0),%%xmm3                     \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqu    (%1),%%xmm2                     \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqu    (%1),%%xmm1                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "movdqu    0x10(%0),%%xmm3                 \n"
    "lea       0x20(%0),%0                     \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,(%2)                     \n"
    "jle       9f                              \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqu    0x10(%1),%%xmm2                 \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqu    0x10(%1),%%xmm1                 \n"
    "lea       0x20(%1),%1                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,0x10(%2)                 \n"
    "lea       0x20(%2),%2                     \n"
    "jg        1b                              \n"
  "9:                                          \n"
  : "+r"(src_argb0),   // %0
    "+r"(src_argb1),   // %1
    "+r"(dst_argb),    // %2
    "+r"(width)        // %3
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

// Blend 1 pixel at a time, unaligned
void ARGBBlendRow1_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                        uint8* dst_argb, int width) {
  asm volatile (
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psrlw     $0xf,%%xmm7                     \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrlw     $0x8,%%xmm6                     \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psllw     $0x8,%%xmm5                     \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"

  // 1 pixel loop
  "1:                                          \n"
    "movd      (%0),%%xmm3                     \n"
    "lea       0x4(%0),%0                      \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      (%1),%%xmm2                     \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      (%1),%%xmm1                     \n"
    "lea       0x4(%1),%1                      \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0,(%2)                     \n"
    "lea       0x4(%2),%2                      \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),   // %0
    "+r"(src_argb1),   // %1
    "+r"(dst_argb),    // %2
    "+r"(width)        // %3
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}
#endif  // HAS_ARGBBLENDROW_SSE2

#ifdef HAS_ARGBBLENDROW_SSSE3
// Shuffle table for reversing the bytes.
CONST uvec8 kShuffleAlpha = {
  3u, 0x80, 3u, 0x80, 7u, 0x80, 7u, 0x80,
  11u, 0x80, 11u, 0x80, 15u, 0x80, 15u, 0x80
};
void ARGBBlendRow_Aligned_SSSE3(const uint8* src_argb0, const uint8* src_argb1,
                                uint8* dst_argb, int width) {
  asm volatile (
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psrlw     $0xf,%%xmm7                     \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrlw     $0x8,%%xmm6                     \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psllw     $0x8,%%xmm5                     \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"

  // 8 pixel loop
  "1:                                          \n"
    "movdqu    (%0),%%xmm3                     \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "pshufb    %4,%%xmm3                       \n"
    "movdqu    (%1),%%xmm2                     \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqu    (%1),%%xmm1                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "movdqu    0x10(%0),%%xmm3                 \n"
    "lea       0x20(%0),%0                     \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                  \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                  \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,(%2)                     \n"
    "jle       9f                              \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqu    0x10(%1),%%xmm2                 \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqu    0x10(%1),%%xmm1                 \n"
    "lea       0x20(%1),%1                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,0x10(%2)                 \n"
    "lea       0x20(%2),%2                     \n"
    "jg        1b                              \n"
  "9:                                          \n"
  : "+r"(src_argb0),    // %0
    "+r"(src_argb1),    // %1
    "+r"(dst_argb),     // %2
    "+r"(width)         // %3
  : "m"(kShuffleAlpha)  // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}
#endif  // HAS_ARGBBLENDROW_SSSE3

#ifdef HAS_ARGBATTENUATE_SSE2
// Attenuate 4 pixels at a time.
// aligned to 16 bytes
void ARGBAttenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrld     $0x8,%%xmm5                     \n"

  // 4 pixel loop
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "pshufhw   $0xff,%%xmm0,%%xmm2             \n"
    "pshuflw   $0xff,%%xmm2,%%xmm2             \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "pshufhw   $0xff,%%xmm1,%%xmm2             \n"
    "pshuflw   $0xff,%%xmm2,%%xmm2             \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "pand      %%xmm4,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),    // %0
    "+r"(dst_argb),    // %1
    "+r"(width)        // %2
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  // HAS_ARGBATTENUATE_SSE2

#ifdef HAS_ARGBATTENUATE_SSSE3
// Shuffle table duplicating alpha
CONST uvec8 kShuffleAlpha0 = {
  3u, 3u, 3u, 3u, 3u, 3u, 128u, 128u, 7u, 7u, 7u, 7u, 7u, 7u, 128u, 128u,
};
CONST uvec8 kShuffleAlpha1 = {
  11u, 11u, 11u, 11u, 11u, 11u, 128u, 128u,
  15u, 15u, 15u, 15u, 15u, 15u, 128u, 128u,
};
// Attenuate 4 pixels at a time.
// aligned to 16 bytes
void ARGBAttenuateRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "pslld     $0x18,%%xmm3                    \n"
    "movdqa    %3,%%xmm4                       \n"
    "movdqa    %4,%%xmm5                       \n"

  // 4 pixel loop
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "punpcklbw %%xmm1,%%xmm1                   \n"
    "pmulhuw   %%xmm1,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "pshufb    %%xmm5,%%xmm1                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "punpckhbw %%xmm2,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "pand      %%xmm3,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),    // %0
    "+r"(dst_argb),    // %1
    "+r"(width)        // %2
  : "m"(kShuffleAlpha0),  // %3
    "m"(kShuffleAlpha1)  // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  // HAS_ARGBATTENUATE_SSSE3

#ifdef HAS_ARGBUNATTENUATE_SSE2
// Divide source RGB by alpha and store to destination.
// b = (b * 255 + (a / 2)) / a;
// g = (g * 255 + (a / 2)) / a;
// r = (r * 255 + (a / 2)) / a;
// Reciprocal method is off by 1 on some values. ie 125
// 8.16 fixed point inverse table
#define T(a) 0x10000 / a
CONST uint32 fixed_invtbl8[256] = {
  0x100, 0xffff, T(0x02), T(0x03), T(0x04), T(0x05), T(0x06), T(0x07),
  T(0x08), T(0x09), T(0x0a), T(0x0b), T(0x0c), T(0x0d), T(0x0e), T(0x0f),
  T(0x10), T(0x11), T(0x12), T(0x13), T(0x14), T(0x15), T(0x16), T(0x17),
  T(0x18), T(0x19), T(0x1a), T(0x1b), T(0x1c), T(0x1d), T(0x1e), T(0x1f),
  T(0x20), T(0x21), T(0x22), T(0x23), T(0x24), T(0x25), T(0x26), T(0x27),
  T(0x28), T(0x29), T(0x2a), T(0x2b), T(0x2c), T(0x2d), T(0x2e), T(0x2f),
  T(0x30), T(0x31), T(0x32), T(0x33), T(0x34), T(0x35), T(0x36), T(0x37),
  T(0x38), T(0x39), T(0x3a), T(0x3b), T(0x3c), T(0x3d), T(0x3e), T(0x3f),
  T(0x40), T(0x41), T(0x42), T(0x43), T(0x44), T(0x45), T(0x46), T(0x47),
  T(0x48), T(0x49), T(0x4a), T(0x4b), T(0x4c), T(0x4d), T(0x4e), T(0x4f),
  T(0x50), T(0x51), T(0x52), T(0x53), T(0x54), T(0x55), T(0x56), T(0x57),
  T(0x58), T(0x59), T(0x5a), T(0x5b), T(0x5c), T(0x5d), T(0x5e), T(0x5f),
  T(0x60), T(0x61), T(0x62), T(0x63), T(0x64), T(0x65), T(0x66), T(0x67),
  T(0x68), T(0x69), T(0x6a), T(0x6b), T(0x6c), T(0x6d), T(0x6e), T(0x6f),
  T(0x70), T(0x71), T(0x72), T(0x73), T(0x74), T(0x75), T(0x76), T(0x77),
  T(0x78), T(0x79), T(0x7a), T(0x7b), T(0x7c), T(0x7d), T(0x7e), T(0x7f),
  T(0x80), T(0x81), T(0x82), T(0x83), T(0x84), T(0x85), T(0x86), T(0x87),
  T(0x88), T(0x89), T(0x8a), T(0x8b), T(0x8c), T(0x8d), T(0x8e), T(0x8f),
  T(0x90), T(0x91), T(0x92), T(0x93), T(0x94), T(0x95), T(0x96), T(0x97),
  T(0x98), T(0x99), T(0x9a), T(0x9b), T(0x9c), T(0x9d), T(0x9e), T(0x9f),
  T(0xa0), T(0xa1), T(0xa2), T(0xa3), T(0xa4), T(0xa5), T(0xa6), T(0xa7),
  T(0xa8), T(0xa9), T(0xaa), T(0xab), T(0xac), T(0xad), T(0xae), T(0xaf),
  T(0xb0), T(0xb1), T(0xb2), T(0xb3), T(0xb4), T(0xb5), T(0xb6), T(0xb7),
  T(0xb8), T(0xb9), T(0xba), T(0xbb), T(0xbc), T(0xbd), T(0xbe), T(0xbf),
  T(0xc0), T(0xc1), T(0xc2), T(0xc3), T(0xc4), T(0xc5), T(0xc6), T(0xc7),
  T(0xc8), T(0xc9), T(0xca), T(0xcb), T(0xcc), T(0xcd), T(0xce), T(0xcf),
  T(0xd0), T(0xd1), T(0xd2), T(0xd3), T(0xd4), T(0xd5), T(0xd6), T(0xd7),
  T(0xd8), T(0xd9), T(0xda), T(0xdb), T(0xdc), T(0xdd), T(0xde), T(0xdf),
  T(0xe0), T(0xe1), T(0xe2), T(0xe3), T(0xe4), T(0xe5), T(0xe6), T(0xe7),
  T(0xe8), T(0xe9), T(0xea), T(0xeb), T(0xec), T(0xed), T(0xee), T(0xef),
  T(0xf0), T(0xf1), T(0xf2), T(0xf3), T(0xf4), T(0xf5), T(0xf6), T(0xf7),
  T(0xf8), T(0xf9), T(0xfa), T(0xfb), T(0xfc), T(0xfd), T(0xfe), 0x100 };
#undef T

// Unattenuate 4 pixels at a time.
// aligned to 16 bytes
void ARGBUnattenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb,
                             int width) {
  uintptr_t alpha = 0;
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"

  // 4 pixel loop
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movzb     0x3(%0),%3                      \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movd      0x0(%4,%3,4),%%xmm2             \n"
    "movzb     0x7(%0),%3                      \n"
    "movd      0x0(%4,%3,4),%%xmm3             \n"
    "pshuflw   $0xc0,%%xmm2,%%xmm2             \n"
    "pshuflw   $0xc0,%%xmm3,%%xmm3             \n"
    "movlhps   %%xmm3,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "movzb     0xb(%0),%3                      \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "movd      0x0(%4,%3,4),%%xmm2             \n"
    "movzb     0xf(%0),%3                      \n"
    "movd      0x0(%4,%3,4),%%xmm3             \n"
    "pshuflw   $0xc0,%%xmm2,%%xmm2             \n"
    "pshuflw   $0xc0,%%xmm3,%%xmm3             \n"
    "movlhps   %%xmm3,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "pand      %%xmm4,%%xmm2                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),    // %0
    "+r"(dst_argb),    // %1
    "+r"(width),       // %2
    "+r"(alpha)        // %3
  : "r"(fixed_invtbl8)  // %4
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  // HAS_ARGBUNATTENUATE_SSE2

#endif  // defined(__x86_64__) || defined(__i386__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

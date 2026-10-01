/*
 *  Copyright 2015 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/rotate_row.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// This module is for GCC and clang x64. 32 bit x86 uses rotate_win.cc.
#if !defined(LIBYUV_DISABLE_X86) && defined(__x86_64__) && \
    !defined(LIBYUV_ENABLE_ROWWIN)

#if defined(HAS_TRANSPOSEWX8_SSSE3)
// Transpose 16x8. 64 bit.
void TransposeWx8_SSSE3(const uint8_t* src,
                        int src_stride,
                        uint8_t* dst,
                        int dst_stride,
                        int width) {
  asm volatile(
      // Read in the data from the source pointer.
      // First round of bit swap.
      LABELALIGN
      "1:          \n"
      "movdqu      (%0),%%xmm0                   \n"
      "movdqu      (%0,%3),%%xmm1                \n"
      "lea         (%0,%3,2),%0                  \n"
      "movdqa      %%xmm0,%%xmm8                 \n"
      "punpcklbw   %%xmm1,%%xmm0                 \n"
      "punpckhbw   %%xmm1,%%xmm8                 \n"
      "movdqu      (%0),%%xmm2                   \n"
      "movdqa      %%xmm0,%%xmm1                 \n"
      "movdqa      %%xmm8,%%xmm9                 \n"
      "palignr     $0x8,%%xmm1,%%xmm1            \n"
      "palignr     $0x8,%%xmm9,%%xmm9            \n"
      "movdqu      (%0,%3),%%xmm3                \n"
      "lea         (%0,%3,2),%0                  \n"
      "movdqa      %%xmm2,%%xmm10                \n"
      "punpcklbw   %%xmm3,%%xmm2                 \n"
      "punpckhbw   %%xmm3,%%xmm10                \n"
      "movdqa      %%xmm2,%%xmm3                 \n"
      "movdqa      %%xmm10,%%xmm11               \n"
      "movdqu      (%0),%%xmm4                   \n"
      "palignr     $0x8,%%xmm3,%%xmm3            \n"
      "palignr     $0x8,%%xmm11,%%xmm11          \n"
      "movdqu      (%0,%3),%%xmm5                \n"
      "lea         (%0,%3,2),%0                  \n"
      "movdqa      %%xmm4,%%xmm12                \n"
      "punpcklbw   %%xmm5,%%xmm4                 \n"
      "punpckhbw   %%xmm5,%%xmm12                \n"
      "movdqa      %%xmm4,%%xmm5                 \n"
      "movdqa      %%xmm12,%%xmm13               \n"
      "movdqu      (%0),%%xmm6                   \n"
      "palignr     $0x8,%%xmm5,%%xmm5            \n"
      "palignr     $0x8,%%xmm13,%%xmm13          \n"
      "movdqu      (%0,%3),%%xmm7                \n"
      "lea         (%0,%3,2),%0                  \n"
      "movdqa      %%xmm6,%%xmm14                \n"
      "punpcklbw   %%xmm7,%%xmm6                 \n"
      "punpckhbw   %%xmm7,%%xmm14                \n"
      "neg         %3                            \n"
      "movdqa      %%xmm6,%%xmm7                 \n"
      "movdqa      %%xmm14,%%xmm15               \n"
      "lea         0x10(%0,%3,8),%0              \n"
      "palignr     $0x8,%%xmm7,%%xmm7            \n"
      "palignr     $0x8,%%xmm15,%%xmm15          \n"
      "neg         %3                            \n"
      // Second round of bit swap.
      "punpcklwd   %%xmm2,%%xmm0                 \n"
      "punpcklwd   %%xmm3,%%xmm1                 \n"
      "movdqa      %%xmm0,%%xmm2                 \n"
      "movdqa      %%xmm1,%%xmm3                 \n"
      "palignr     $0x8,%%xmm2,%%xmm2            \n"
      "palignr     $0x8,%%xmm3,%%xmm3            \n"
      "punpcklwd   %%xmm6,%%xmm4                 \n"
      "punpcklwd   %%xmm7,%%xmm5                 \n"
      "movdqa      %%xmm4,%%xmm6                 \n"
      "movdqa      %%xmm5,%%xmm7                 \n"
      "palignr     $0x8,%%xmm6,%%xmm6            \n"
      "palignr     $0x8,%%xmm7,%%xmm7            \n"
      "punpcklwd   %%xmm10,%%xmm8                \n"
      "punpcklwd   %%xmm11,%%xmm9                \n"
      "movdqa      %%xmm8,%%xmm10                \n"
      "movdqa      %%xmm9,%%xmm11                \n"
      "palignr     $0x8,%%xmm10,%%xmm10          \n"
      "palignr     $0x8,%%xmm11,%%xmm11          \n"
      "punpcklwd   %%xmm14,%%xmm12               \n"
      "punpcklwd   %%xmm15,%%xmm13               \n"
      "movdqa      %%xmm12,%%xmm14               \n"
      "movdqa      %%xmm13,%%xmm15               \n"
      "palignr     $0x8,%%xmm14,%%xmm14          \n"
      "palignr     $0x8,%%xmm15,%%xmm15          \n"
      // Third round of bit swap.
      // Write to the destination pointer.
      "punpckldq   %%xmm4,%%xmm0                 \n"
      "movq        %%xmm0,(%1)                   \n"
      "movdqa      %%xmm0,%%xmm4                 \n"
      "palignr     $0x8,%%xmm4,%%xmm4            \n"
      "movq        %%xmm4,(%1,%4)                \n"
      "lea         (%1,%4,2),%1                  \n"
      "punpckldq   %%xmm6,%%xmm2                 \n"
      "movdqa      %%xmm2,%%xmm6                 \n"
      "movq        %%xmm2,(%1)                   \n"
      "palignr     $0x8,%%xmm6,%%xmm6            \n"
      "punpckldq   %%xmm5,%%xmm1                 \n"
      "movq        %%xmm6,(%1,%4)                \n"
      "lea         (%1,%4,2),%1                  \n"
      "movdqa      %%xmm1,%%xmm5                 \n"
      "movq        %%xmm1,(%1)                   \n"
      "palignr     $0x8,%%xmm5,%%xmm5            \n"
      "movq        %%xmm5,(%1,%4)                \n"
      "lea         (%1,%4,2),%1                  \n"
      "punpckldq   %%xmm7,%%xmm3                 \n"
      "movq        %%xmm3,(%1)                   \n"
      "movdqa      %%xmm3,%%xmm7                 \n"
      "palignr     $0x8,%%xmm7,%%xmm7            \n"
      "movq        %%xmm7,(%1,%4)                \n"
      "lea         (%1,%4,2),%1                  \n"
      "punpckldq   %%xmm12,%%xmm8                \n"
      "movq        %%xmm8,(%1)                   \n"
      "movdqa      %%xmm8,%%xmm12                \n"
      "palignr     $0x8,%%xmm12,%%xmm12          \n"
      "movq        %%xmm12,(%1,%4)               \n"
      "lea         (%1,%4,2),%1                  \n"
      "punpckldq   %%xmm14,%%xmm10               \n"
      "movdqa      %%xmm10,%%xmm14               \n"
      "movq        %%xmm10,(%1)                  \n"
      "palignr     $0x8,%%xmm14,%%xmm14          \n"
      "punpckldq   %%xmm13,%%xmm9                \n"
      "movq        %%xmm14,(%1,%4)               \n"
      "lea         (%1,%4,2),%1                  \n"
      "movdqa      %%xmm9,%%xmm13                \n"
      "movq        %%xmm9,(%1)                   \n"
      "palignr     $0x8,%%xmm13,%%xmm13          \n"
      "movq        %%xmm13,(%1,%4)               \n"
      "lea         (%1,%4,2),%1                  \n"
      "punpckldq   %%xmm15,%%xmm11               \n"
      "movq        %%xmm11,(%1)                  \n"
      "movdqa      %%xmm11,%%xmm15               \n"
      "palignr     $0x8,%%xmm15,%%xmm15          \n"
      "sub         $0x10,%2                      \n"
      "movq        %%xmm15,(%1,%4)               \n"
      "lea         (%1,%4,2),%1                  \n"
      "jg          1b                            \n"
      : "+r"(src),                     // %0
        "+r"(dst),                     // %1
        "+r"(width)                    // %2
      : "r"((ptrdiff_t)(src_stride)),  // %3
        "r"((ptrdiff_t)(dst_stride))   // %4
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14",
        "xmm15");
}
#endif  // defined(HAS_TRANSPOSEWX8_SSSE3)

#if defined(HAS_TRANSPOSEWX16_AVX512BW) || defined(HAS_TRANSPOSEUVWX16_AVX512BW)
// Dword permutes to gather 16 rows of a column from 4 lanes.
static const uint32_t kPermdTranspose_AVX512BW[32] = {
    0, 4, 8, 12, 2, 6, 10, 14, 16, 20, 24, 28, 18, 22, 26, 30,
    1, 5, 9, 13, 3, 7, 11, 15, 17, 21, 25, 29, 19, 23, 27, 31};

// Transpose 16x16 bytes held in 4 lanes of 4 zmm registers.
// Source row r is in lane r / 4 of zmm(r % 4). Outputs zmm0..zmm3 are 16
// destination rows, lane n of zmm0/zmm2 for dst_a and lane n of zmm1/zmm3
// for dst_b.
#define TRANSPOSE16X16_AVX512BW                  \
  "vpunpcklbw  %%zmm1,%%zmm0,%%zmm4          \n" \
  "vpunpckhbw  %%zmm1,%%zmm0,%%zmm5          \n" \
  "vpunpcklbw  %%zmm3,%%zmm2,%%zmm0          \n" \
  "vpunpckhbw  %%zmm3,%%zmm2,%%zmm1          \n" \
  "vpunpcklwd  %%zmm0,%%zmm4,%%zmm2          \n" \
  "vpunpckhwd  %%zmm0,%%zmm4,%%zmm3          \n" \
  "vpunpcklwd  %%zmm1,%%zmm5,%%zmm4          \n" \
  "vpunpckhwd  %%zmm1,%%zmm5,%%zmm5          \n" \
  "vmovdqa64   %%zmm2,%%zmm0                 \n" \
  "vpermt2d    %%zmm4,%%zmm6,%%zmm0          \n" \
  "vmovdqa64   %%zmm2,%%zmm1                 \n" \
  "vpermt2d    %%zmm4,%%zmm7,%%zmm1          \n" \
  "vmovdqa64   %%zmm3,%%zmm2                 \n" \
  "vpermt2d    %%zmm5,%%zmm6,%%zmm2          \n" \
  "vpermt2d    %%zmm5,%%zmm7,%%zmm3          \n"

// Transpose 16x16 bytes using 4 lanes of AVX512BW with tail masking.
// Can be used for 8-bit planar transpose (dst_a = even rows, dst_b = odd rows)
// or 16-bit UV split transpose (dst_a = U plane, dst_b = V plane).
// TODO(fbarchard): Port to rotate_win.cc using intrinsics.
// TODO(fbarchard): Use for ARGB (32 bit) and 16 bit channel transposes.
// TODO(fbarchard): Consider removing in favor of TransposeWx16_Byte_AVX2,
// which is within 10% of AVX512BW on most CPUs.
static void TransposeWx16_Byte_AVX512BW(const uint8_t* src,
                                        int src_stride,
                                        uint8_t* dst_a,
                                        int dst_stride_a,
                                        uint8_t* dst_b,
                                        int dst_stride_b,
                                        int byte_width) {
  uintptr_t temp;
  asm volatile(
      "vmovdqu32   (%[shuf]),%%zmm6              \n"
      "vmovdqu32   0x40(%[shuf]),%%zmm7          \n"
      "sub         $0x10,%[width]                \n"
      "jl          2f                            \n"

      // Main loop: 16 source rows x 16 bytes.
      LABELALIGN
      "1:          \n"
      "vmovdqu     (%[src]),%%xmm0 \n"
      "vmovdqu     (%[src],%[src_stride]),%%xmm1 \n"
      "lea         (%[src],%[src_stride],2),%[temp] \n"
      "lea         0x10(%[src]),%[src] \n"
      "vmovdqu     (%[temp]),%%xmm2 \n"
      "vmovdqu     (%[temp],%[src_stride]),%%xmm3 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti32x4 $1,(%[temp]),%%zmm0,%%zmm0 \n"
      "vinserti32x4 $1,(%[temp],%[src_stride]),%%zmm1,%%zmm1 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti32x4 $1,(%[temp]),%%zmm2,%%zmm2 \n"
      "vinserti32x4 $1,(%[temp],%[src_stride]),%%zmm3,%%zmm3 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti32x4 $2,(%[temp]),%%zmm0,%%zmm0 \n"
      "vinserti32x4 $2,(%[temp],%[src_stride]),%%zmm1,%%zmm1 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti32x4 $2,(%[temp]),%%zmm2,%%zmm2 \n"
      "vinserti32x4 $2,(%[temp],%[src_stride]),%%zmm3,%%zmm3 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti32x4 $3,(%[temp]),%%zmm0,%%zmm0 \n"
      "vinserti32x4 $3,(%[temp],%[src_stride]),%%zmm1,%%zmm1 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti32x4 $3,(%[temp]),%%zmm2,%%zmm2 \n"
      "vinserti32x4 $3,(%[temp],%[src_stride]),%%zmm3,%%zmm3 \n"

      TRANSPOSE16X16_AVX512BW

      "vmovdqu     %%xmm0,(%[dst_a])             \n"
      "vmovdqu     %%xmm1,(%[dst_b])             \n"
      "vextracti32x4 $1,%%zmm0,(%[dst_a],%[dst_stride_a]) \n"
      "vextracti32x4 $1,%%zmm1,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "vmovdqu     %%xmm2,(%[dst_a])             \n"
      "vmovdqu     %%xmm3,(%[dst_b])             \n"
      "vextracti32x4 $1,%%zmm2,(%[dst_a],%[dst_stride_a]) \n"
      "vextracti32x4 $1,%%zmm3,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "vextracti32x4 $2,%%zmm0,(%[dst_a])        \n"
      "vextracti32x4 $2,%%zmm1,(%[dst_b])        \n"
      "vextracti32x4 $3,%%zmm0,(%[dst_a],%[dst_stride_a]) \n"
      "vextracti32x4 $3,%%zmm1,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "vextracti32x4 $2,%%zmm2,(%[dst_a])        \n"
      "vextracti32x4 $2,%%zmm3,(%[dst_b])        \n"
      "vextracti32x4 $3,%%zmm2,(%[dst_a],%[dst_stride_a]) \n"
      "vextracti32x4 $3,%%zmm3,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "sub         $0x10,%[width]                \n"
      "jge         1b                            \n"

      // Remainder: 1 to 15 bytes, masked loads.
      "2:          \n"
      "add         $0x10,%[width]                \n"
      "je          99f                           \n"
      "mov         $-1,%k[temp]                  \n"
      "bzhi        %k[width],%k[temp],%k[temp]   \n"
      "kmovw       %k[temp],%%k1                 \n"
      "vmovdqu8    (%[src]),%%xmm0%{%%k1%}%{z%} \n"
      "vmovdqu8    (%[src],%[src_stride]),%%xmm1%{%%k1%}%{z%} \n"
      "lea         (%[src],%[src_stride],2),%[src] \n"
      "vmovdqu8    (%[src]),%%xmm2%{%%k1%}%{z%} \n"
      "vmovdqu8    (%[src],%[src_stride]),%%xmm3%{%%k1%}%{z%} \n"
      "lea         (%[src],%[src_stride],2),%[src] \n"
      "vmovdqu8    (%[src]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $1,%%xmm4,%%zmm0,%%zmm0 \n"
      "vmovdqu8    (%[src],%[src_stride]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $1,%%xmm4,%%zmm1,%%zmm1 \n"
      "lea         (%[src],%[src_stride],2),%[src] \n"
      "vmovdqu8    (%[src]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $1,%%xmm4,%%zmm2,%%zmm2 \n"
      "vmovdqu8    (%[src],%[src_stride]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $1,%%xmm4,%%zmm3,%%zmm3 \n"
      "lea         (%[src],%[src_stride],2),%[src] \n"
      "vmovdqu8    (%[src]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $2,%%xmm4,%%zmm0,%%zmm0 \n"
      "vmovdqu8    (%[src],%[src_stride]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $2,%%xmm4,%%zmm1,%%zmm1 \n"
      "lea         (%[src],%[src_stride],2),%[src] \n"
      "vmovdqu8    (%[src]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $2,%%xmm4,%%zmm2,%%zmm2 \n"
      "vmovdqu8    (%[src],%[src_stride]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $2,%%xmm4,%%zmm3,%%zmm3 \n"
      "lea         (%[src],%[src_stride],2),%[src] \n"
      "vmovdqu8    (%[src]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $3,%%xmm4,%%zmm0,%%zmm0 \n"
      "vmovdqu8    (%[src],%[src_stride]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $3,%%xmm4,%%zmm1,%%zmm1 \n"
      "lea         (%[src],%[src_stride],2),%[src] \n"
      "vmovdqu8    (%[src]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $3,%%xmm4,%%zmm2,%%zmm2 \n"
      "vmovdqu8    (%[src],%[src_stride]),%%xmm4%{%%k1%}%{z%} \n"
      "vinserti32x4 $3,%%xmm4,%%zmm3,%%zmm3 \n"

      TRANSPOSE16X16_AVX512BW

      // Store 1 destination row per remaining byte.
      "vmovdqu     %%xmm0,(%[dst_a])             \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vmovdqu     %%xmm1,(%[dst_b])             \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $1,%%zmm0,(%[dst_a],%[dst_stride_a]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $1,%%zmm1,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vmovdqu     %%xmm2,(%[dst_a])             \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vmovdqu     %%xmm3,(%[dst_b])             \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $1,%%zmm2,(%[dst_a],%[dst_stride_a]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $1,%%zmm3,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $2,%%zmm0,(%[dst_a])        \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $2,%%zmm1,(%[dst_b])        \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $3,%%zmm0,(%[dst_a],%[dst_stride_a]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $3,%%zmm1,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $2,%%zmm2,(%[dst_a])        \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $2,%%zmm3,(%[dst_b])        \n"
      "sub         $0x1,%[width]                 \n"
      "je          99f                           \n"
      "vextracti32x4 $3,%%zmm2,(%[dst_a],%[dst_stride_a]) \n"

      "99:         \n"
      "vzeroupper  \n"
      : [src] "+r"(src), [dst_a] "+r"(dst_a), [dst_b] "+r"(dst_b),
        [width] "+r"(byte_width), [temp] "=&r"(temp)
      : [src_stride] "r"((ptrdiff_t)src_stride),
        [dst_stride_a] "r"((ptrdiff_t)dst_stride_a),
        [dst_stride_b] "r"((ptrdiff_t)dst_stride_b),
        [shuf] "r"(kPermdTranspose_AVX512BW)
      : "memory", "cc", "k1", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5",
        "xmm6", "xmm7");
}
#undef TRANSPOSE16X16_AVX512BW
#endif  // defined(HAS_TRANSPOSEWX16_AVX512BW) ||
        // defined(HAS_TRANSPOSEUVWX16_AVX512BW)

#if defined(HAS_TRANSPOSEWX16_AVX512BW)
void TransposeWx16_AVX512BW(const uint8_t* src,
                            int src_stride,
                            uint8_t* dst,
                            int dst_stride,
                            int width) {
  TransposeWx16_Byte_AVX512BW(src, src_stride, dst, dst_stride * 2,
                              dst + (width > 1 ? dst_stride : 0),
                              dst_stride * 2, width);
}
#endif  // defined(HAS_TRANSPOSEWX16_AVX512BW)

#if defined(HAS_TRANSPOSEUVWX16_AVX512BW)
void TransposeUVWx16_AVX512BW(const uint8_t* src,
                              int src_stride,
                              uint8_t* dst_a,
                              int dst_stride_a,
                              uint8_t* dst_b,
                              int dst_stride_b,
                              int width) {
  TransposeWx16_Byte_AVX512BW(src, src_stride, dst_a, dst_stride_a, dst_b,
                              dst_stride_b, width * 2);
}
#endif  // defined(HAS_TRANSPOSEUVWX16_AVX512BW)

#if defined(HAS_TRANSPOSEWX16_AVX2) || defined(HAS_TRANSPOSEUVWX16_AVX2)
// Transpose 16x16 bytes using 2 lanes of AVX2. Source row r is in lane r / 8
// of ymm(r % 8). 3 unpack stages leave each lane with 2 columns x 8 rows, and
// vpermq joins the rows 0..7 and 8..15 halves. Lane 0 of each output is an
// even column for dst_a and lane 1 an odd column for dst_b.
// Width is a multiple of 16 bytes.
static void TransposeWx16_Byte_AVX2(const uint8_t* src,
                                    int src_stride,
                                    uint8_t* dst_a,
                                    int dst_stride_a,
                                    uint8_t* dst_b,
                                    int dst_stride_b,
                                    int byte_width) {
  uintptr_t temp;
  asm volatile(
      "1:          \n"
      "vmovdqu     (%[src]),%%xmm0               \n"
      "vmovdqu     (%[src],%[src_stride]),%%xmm1 \n"
      "lea         (%[src],%[src_stride],2),%[temp] \n"
      "lea         0x10(%[src]),%[src]           \n"
      "vmovdqu     (%[temp]),%%xmm2              \n"
      "vmovdqu     (%[temp],%[src_stride]),%%xmm3 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vmovdqu     (%[temp]),%%xmm4              \n"
      "vmovdqu     (%[temp],%[src_stride]),%%xmm5 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vmovdqu     (%[temp]),%%xmm6              \n"
      "vmovdqu     (%[temp],%[src_stride]),%%xmm7 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti128 $1,(%[temp]),%%ymm0,%%ymm0    \n"
      "vinserti128 $1,(%[temp],%[src_stride]),%%ymm1,%%ymm1 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti128 $1,(%[temp]),%%ymm2,%%ymm2    \n"
      "vinserti128 $1,(%[temp],%[src_stride]),%%ymm3,%%ymm3 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti128 $1,(%[temp]),%%ymm4,%%ymm4    \n"
      "vinserti128 $1,(%[temp],%[src_stride]),%%ymm5,%%ymm5 \n"
      "lea         (%[temp],%[src_stride],2),%[temp] \n"
      "vinserti128 $1,(%[temp]),%%ymm6,%%ymm6    \n"
      "vinserti128 $1,(%[temp],%[src_stride]),%%ymm7,%%ymm7 \n"

      "vpunpcklbw  %%ymm1,%%ymm0,%%ymm8          \n"
      "vpunpckhbw  %%ymm1,%%ymm0,%%ymm9          \n"
      "vpunpcklbw  %%ymm3,%%ymm2,%%ymm10         \n"
      "vpunpckhbw  %%ymm3,%%ymm2,%%ymm11         \n"
      "vpunpcklbw  %%ymm5,%%ymm4,%%ymm12         \n"
      "vpunpckhbw  %%ymm5,%%ymm4,%%ymm13         \n"
      "vpunpcklbw  %%ymm7,%%ymm6,%%ymm14         \n"
      "vpunpckhbw  %%ymm7,%%ymm6,%%ymm15         \n"

      "vpunpcklwd  %%ymm10,%%ymm8,%%ymm0         \n"
      "vpunpckhwd  %%ymm10,%%ymm8,%%ymm1         \n"
      "vpunpcklwd  %%ymm11,%%ymm9,%%ymm2         \n"
      "vpunpckhwd  %%ymm11,%%ymm9,%%ymm3         \n"
      "vpunpcklwd  %%ymm14,%%ymm12,%%ymm4        \n"
      "vpunpckhwd  %%ymm14,%%ymm12,%%ymm5        \n"
      "vpunpcklwd  %%ymm15,%%ymm13,%%ymm6        \n"
      "vpunpckhwd  %%ymm15,%%ymm13,%%ymm7        \n"

      "vpunpckldq  %%ymm4,%%ymm0,%%ymm8          \n"
      "vpunpckhdq  %%ymm4,%%ymm0,%%ymm9          \n"
      "vpunpckldq  %%ymm5,%%ymm1,%%ymm10         \n"
      "vpunpckhdq  %%ymm5,%%ymm1,%%ymm11         \n"
      "vpunpckldq  %%ymm6,%%ymm2,%%ymm12         \n"
      "vpunpckhdq  %%ymm6,%%ymm2,%%ymm13         \n"
      "vpunpckldq  %%ymm7,%%ymm3,%%ymm14         \n"
      "vpunpckhdq  %%ymm7,%%ymm3,%%ymm15         \n"

      "vpermq      $0xd8,%%ymm8,%%ymm8           \n"
      "vpermq      $0xd8,%%ymm9,%%ymm9           \n"
      "vpermq      $0xd8,%%ymm10,%%ymm10         \n"
      "vpermq      $0xd8,%%ymm11,%%ymm11         \n"
      "vpermq      $0xd8,%%ymm12,%%ymm12         \n"
      "vpermq      $0xd8,%%ymm13,%%ymm13         \n"
      "vpermq      $0xd8,%%ymm14,%%ymm14         \n"
      "vpermq      $0xd8,%%ymm15,%%ymm15         \n"

      "vmovdqu     %%xmm8,(%[dst_a])             \n"
      "vextracti128 $1,%%ymm8,(%[dst_b])         \n"
      "vmovdqu     %%xmm9,(%[dst_a],%[dst_stride_a]) \n"
      "vextracti128 $1,%%ymm9,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "vmovdqu     %%xmm10,(%[dst_a])            \n"
      "vextracti128 $1,%%ymm10,(%[dst_b])        \n"
      "vmovdqu     %%xmm11,(%[dst_a],%[dst_stride_a]) \n"
      "vextracti128 $1,%%ymm11,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "vmovdqu     %%xmm12,(%[dst_a])            \n"
      "vextracti128 $1,%%ymm12,(%[dst_b])        \n"
      "vmovdqu     %%xmm13,(%[dst_a],%[dst_stride_a]) \n"
      "vextracti128 $1,%%ymm13,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "vmovdqu     %%xmm14,(%[dst_a])            \n"
      "vextracti128 $1,%%ymm14,(%[dst_b])        \n"
      "vmovdqu     %%xmm15,(%[dst_a],%[dst_stride_a]) \n"
      "vextracti128 $1,%%ymm15,(%[dst_b],%[dst_stride_b]) \n"
      "lea         (%[dst_a],%[dst_stride_a],2),%[dst_a] \n"
      "lea         (%[dst_b],%[dst_stride_b],2),%[dst_b] \n"
      "sub         $0x10,%[width]                \n"
      "jg          1b                            \n"
      "vzeroupper  \n"
      : [src] "+r"(src), [dst_a] "+r"(dst_a), [dst_b] "+r"(dst_b),
        [width] "+r"(byte_width), [temp] "=&r"(temp)
      : [src_stride] "r"((ptrdiff_t)src_stride),
        [dst_stride_a] "r"((ptrdiff_t)dst_stride_a),
        [dst_stride_b] "r"((ptrdiff_t)dst_stride_b)
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14",
        "xmm15");
}
#endif  // defined(HAS_TRANSPOSEWX16_AVX2) || defined(HAS_TRANSPOSEUVWX16_AVX2)

#if defined(HAS_TRANSPOSEWX16_AVX2)
void TransposeWx16_AVX2(const uint8_t* src,
                        int src_stride,
                        uint8_t* dst,
                        int dst_stride,
                        int width) {
  TransposeWx16_Byte_AVX2(src, src_stride, dst, dst_stride * 2,
                          dst + dst_stride, dst_stride * 2, width);
}
#endif  // defined(HAS_TRANSPOSEWX16_AVX2)

#if defined(HAS_TRANSPOSEUVWX16_AVX2)
void TransposeUVWx16_AVX2(const uint8_t* src,
                          int src_stride,
                          uint8_t* dst_a,
                          int dst_stride_a,
                          uint8_t* dst_b,
                          int dst_stride_b,
                          int width) {
  TransposeWx16_Byte_AVX2(src, src_stride, dst_a, dst_stride_a, dst_b,
                          dst_stride_b, width * 2);
}
#endif  // defined(HAS_TRANSPOSEUVWX16_AVX2)

// Transpose UV 8x8.  64 bit.
#if defined(HAS_TRANSPOSEUVWX8_SSE2)
void TransposeUVWx8_SSE2(const uint8_t* src,
                         int src_stride,
                         uint8_t* dst_a,
                         int dst_stride_a,
                         uint8_t* dst_b,
                         int dst_stride_b,
                         int width) {
  asm volatile(
      // Read in the data from the source pointer.
      // First round of bit swap.
      LABELALIGN
      "1:          \n"
      "movdqu      (%0),%%xmm0                   \n"
      "movdqu      (%0,%4),%%xmm1                \n"
      "lea         (%0,%4,2),%0                  \n"
      "movdqa      %%xmm0,%%xmm8                 \n"
      "punpcklbw   %%xmm1,%%xmm0                 \n"
      "punpckhbw   %%xmm1,%%xmm8                 \n"
      "movdqa      %%xmm8,%%xmm1                 \n"
      "movdqu      (%0),%%xmm2                   \n"
      "movdqu      (%0,%4),%%xmm3                \n"
      "lea         (%0,%4,2),%0                  \n"
      "movdqa      %%xmm2,%%xmm8                 \n"
      "punpcklbw   %%xmm3,%%xmm2                 \n"
      "punpckhbw   %%xmm3,%%xmm8                 \n"
      "movdqa      %%xmm8,%%xmm3                 \n"
      "movdqu      (%0),%%xmm4                   \n"
      "movdqu      (%0,%4),%%xmm5                \n"
      "lea         (%0,%4,2),%0                  \n"
      "movdqa      %%xmm4,%%xmm8                 \n"
      "punpcklbw   %%xmm5,%%xmm4                 \n"
      "punpckhbw   %%xmm5,%%xmm8                 \n"
      "movdqa      %%xmm8,%%xmm5                 \n"
      "movdqu      (%0),%%xmm6                   \n"
      "movdqu      (%0,%4),%%xmm7                \n"
      "lea         (%0,%4,2),%0                  \n"
      "movdqa      %%xmm6,%%xmm8                 \n"
      "punpcklbw   %%xmm7,%%xmm6                 \n"
      "neg         %4                            \n"
      "lea         0x10(%0,%4,8),%0              \n"
      "punpckhbw   %%xmm7,%%xmm8                 \n"
      "movdqa      %%xmm8,%%xmm7                 \n"
      "neg         %4                            \n"
      // Second round of bit swap.
      "movdqa      %%xmm0,%%xmm8                 \n"
      "movdqa      %%xmm1,%%xmm9                 \n"
      "punpckhwd   %%xmm2,%%xmm8                 \n"
      "punpckhwd   %%xmm3,%%xmm9                 \n"
      "punpcklwd   %%xmm2,%%xmm0                 \n"
      "punpcklwd   %%xmm3,%%xmm1                 \n"
      "movdqa      %%xmm8,%%xmm2                 \n"
      "movdqa      %%xmm9,%%xmm3                 \n"
      "movdqa      %%xmm4,%%xmm8                 \n"
      "movdqa      %%xmm5,%%xmm9                 \n"
      "punpckhwd   %%xmm6,%%xmm8                 \n"
      "punpckhwd   %%xmm7,%%xmm9                 \n"
      "punpcklwd   %%xmm6,%%xmm4                 \n"
      "punpcklwd   %%xmm7,%%xmm5                 \n"
      "movdqa      %%xmm8,%%xmm6                 \n"
      "movdqa      %%xmm9,%%xmm7                 \n"
      // Third round of bit swap.
      // Write to the destination pointer.
      "movdqa      %%xmm0,%%xmm8                 \n"
      "punpckldq   %%xmm4,%%xmm0                 \n"
      "movlpd      %%xmm0,(%1)                   \n"  // Write back U channel
      "movhpd      %%xmm0,(%2)                   \n"  // Write back V channel
      "punpckhdq   %%xmm4,%%xmm8                 \n"
      "movlpd      %%xmm8,(%1,%5)                \n"
      "lea         (%1,%5,2),%1                  \n"
      "movhpd      %%xmm8,(%2,%6)                \n"
      "lea         (%2,%6,2),%2                  \n"
      "movdqa      %%xmm2,%%xmm8                 \n"
      "punpckldq   %%xmm6,%%xmm2                 \n"
      "movlpd      %%xmm2,(%1)                   \n"
      "movhpd      %%xmm2,(%2)                   \n"
      "punpckhdq   %%xmm6,%%xmm8                 \n"
      "movlpd      %%xmm8,(%1,%5)                \n"
      "lea         (%1,%5,2),%1                  \n"
      "movhpd      %%xmm8,(%2,%6)                \n"
      "lea         (%2,%6,2),%2                  \n"
      "movdqa      %%xmm1,%%xmm8                 \n"
      "punpckldq   %%xmm5,%%xmm1                 \n"
      "movlpd      %%xmm1,(%1)                   \n"
      "movhpd      %%xmm1,(%2)                   \n"
      "punpckhdq   %%xmm5,%%xmm8                 \n"
      "movlpd      %%xmm8,(%1,%5)                \n"
      "lea         (%1,%5,2),%1                  \n"
      "movhpd      %%xmm8,(%2,%6)                \n"
      "lea         (%2,%6,2),%2                  \n"
      "movdqa      %%xmm3,%%xmm8                 \n"
      "punpckldq   %%xmm7,%%xmm3                 \n"
      "movlpd      %%xmm3,(%1)                   \n"
      "movhpd      %%xmm3,(%2)                   \n"
      "punpckhdq   %%xmm7,%%xmm8                 \n"
      "sub         $0x8,%3                       \n"
      "movlpd      %%xmm8,(%1,%5)                \n"
      "lea         (%1,%5,2),%1                  \n"
      "movhpd      %%xmm8,(%2,%6)                \n"
      "lea         (%2,%6,2),%2                  \n"
      "jg          1b                            \n"
      : "+r"(src),                       // %0
        "+r"(dst_a),                     // %1
        "+r"(dst_b),                     // %2
        "+r"(width)                      // %3
      : "r"((ptrdiff_t)(src_stride)),    // %4
        "r"((ptrdiff_t)(dst_stride_a)),  // %5
        "r"((ptrdiff_t)(dst_stride_b))   // %6
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7", "xmm8", "xmm9");
}
#endif  // defined(HAS_TRANSPOSEUVWX8_SSE2)

#if defined(HAS_TRANSPOSE4X4_32_SSE2)
// 4 values, little endian view
// a b c d
// e f g h
// i j k l
// m n o p

// transpose 2x2
// a e b f   from row 0, 1
// i m j n   from row 2, 3
// c g d h   from row 0, 1
// k o l p   from row 2, 3

// transpose 4x4
// a e i m   from row 0, 1
// b f j n   from row 0, 1
// c g k o   from row 2, 3
// d h l p   from row 2, 3

// Transpose 32 bit values (ARGB)
void Transpose4x4_32_SSE2(const uint8_t* src,
                          int src_stride,
                          uint8_t* dst,
                          int dst_stride,
                          int width) {
  asm volatile(
      // Main loop transpose 4x4.  Read a column, write a row.
      "1:          \n"
      "movdqu      (%0),%%xmm0                   \n"  // a b c d
      "movdqu      (%0,%3),%%xmm1                \n"  // e f g h
      "lea         (%0,%3,2),%0                  \n"  // src += stride * 2
      "movdqu      (%0),%%xmm2                   \n"  // i j k l
      "movdqu      (%0,%3),%%xmm3                \n"  // m n o p
      "lea         (%0,%3,2),%0                  \n"  // src += stride * 2

      // Transpose 2x2
      "movdqa      %%xmm0,%%xmm4                 \n"
      "movdqa      %%xmm2,%%xmm5                 \n"
      "movdqa      %%xmm0,%%xmm6                 \n"
      "movdqa      %%xmm2,%%xmm7                 \n"
      "punpckldq   %%xmm1,%%xmm4                 \n"  // a e b f   from row 0, 1
      "punpckldq   %%xmm3,%%xmm5                 \n"  // i m j n   from row 2, 3
      "punpckhdq   %%xmm1,%%xmm6                 \n"  // c g d h   from row 0, 1
      "punpckhdq   %%xmm3,%%xmm7                 \n"  // k o l p   from row 2, 3

      // Transpose 4x4
      "movdqa      %%xmm4,%%xmm0                 \n"
      "movdqa      %%xmm4,%%xmm1                 \n"
      "movdqa      %%xmm6,%%xmm2                 \n"
      "movdqa      %%xmm6,%%xmm3                 \n"
      "punpcklqdq  %%xmm5,%%xmm0                 \n"  // a e i m   from row 0, 1
      "punpckhqdq  %%xmm5,%%xmm1                 \n"  // b f j n   from row 0, 1
      "punpcklqdq  %%xmm7,%%xmm2                 \n"  // c g k o   from row 2, 3
      "punpckhqdq  %%xmm7,%%xmm3                 \n"  // d h l p   from row 2, 3

      "movdqu      %%xmm0,(%1)                   \n"
      "lea         16(%1,%4),%1                  \n"  // dst += stride + 16
      "movdqu      %%xmm1,-16(%1)                \n"
      "movdqu      %%xmm2,-16(%1,%4)             \n"
      "movdqu      %%xmm3,-16(%1,%4,2)           \n"
      "sub         %4,%1                         \n"
      "sub         $0x4,%2                       \n"
      "jg          1b                            \n"
      : "+r"(src),                     // %0
        "+r"(dst),                     // %1
        "+rm"(width)                   // %2
      : "r"((ptrdiff_t)(src_stride)),  // %3
        "r"((ptrdiff_t)(dst_stride))   // %4
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7");
}
#endif  // defined(HAS_TRANSPOSE4X4_32_SSE2)

#if defined(HAS_TRANSPOSE4X4_32_AVX2)

// Transpose 32 bit values (ARGB)
void Transpose4x4_32_AVX2(const uint8_t* src,
                          int src_stride,
                          uint8_t* dst,
                          int dst_stride,
                          int width) {
  asm volatile(
      // Main loop transpose 2 blocks of 4x4.  Read a column, write a row.
      "1:          \n"
      "vmovdqu     (%0),%%xmm0                   \n"  // a b c d
      "vmovdqu     (%0,%3),%%xmm1                \n"  // e f g h
      "lea         (%0,%3,2),%0                  \n"  // src += stride * 2
      "vmovdqu     (%0),%%xmm2                   \n"  // i j k l
      "vmovdqu     (%0,%3),%%xmm3                \n"  // m n o p
      "lea         (%0,%3,2),%0                  \n"  // src += stride * 2

      "vinserti128 $1,(%0),%%ymm0,%%ymm0         \n"  // a b c d
      "vinserti128 $1,(%0,%3),%%ymm1,%%ymm1      \n"  // e f g h
      "lea         (%0,%3,2),%0                  \n"  // src += stride * 2
      "vinserti128 $1,(%0),%%ymm2,%%ymm2         \n"  // i j k l
      "vinserti128 $1,(%0,%3),%%ymm3,%%ymm3      \n"  // m n o p
      "lea         (%0,%3,2),%0                  \n"  // src += stride * 2

      // Transpose 2x2
      "vpunpckldq  %%ymm1,%%ymm0,%%ymm4          \n"  // a e b f   from row 0, 1
      "vpunpckldq  %%ymm3,%%ymm2,%%ymm5          \n"  // i m j n   from row 2, 3
      "vpunpckhdq  %%ymm1,%%ymm0,%%ymm6          \n"  // c g d h   from row 0, 1
      "vpunpckhdq  %%ymm3,%%ymm2,%%ymm7          \n"  // k o l p   from row 2, 3

      // Transpose 4x4
      "vpunpcklqdq %%ymm5,%%ymm4,%%ymm0          \n"  // a e i m   from row 0, 1
      "vpunpckhqdq %%ymm5,%%ymm4,%%ymm1          \n"  // b f j n   from row 0, 1
      "vpunpcklqdq %%ymm7,%%ymm6,%%ymm2          \n"  // c g k o   from row 2, 3
      "vpunpckhqdq %%ymm7,%%ymm6,%%ymm3          \n"  // d h l p   from row 2, 3

      "vmovdqu     %%ymm0,(%1)                   \n"
      "lea         32(%1,%4),%1                  \n"  // dst += stride + 32
      "vmovdqu     %%ymm1,-32(%1)                \n"
      "vmovdqu     %%ymm2,-32(%1,%4)             \n"
      "vmovdqu     %%ymm3,-32(%1,%4,2)           \n"
      "sub         %4,%1                         \n"
      "sub         $0x8,%2                       \n"
      "jg          1b                            \n"
      "vzeroupper  \n"
      : "+r"(src),                     // %0
        "+r"(dst),                     // %1
        "+rm"(width)                   // %2
      : "r"((ptrdiff_t)(src_stride)),  // %3
        "r"((ptrdiff_t)(dst_stride))   // %4
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7");
}
#endif  // defined(HAS_TRANSPOSE4X4_32_AVX2)

#endif  // !defined(LIBYUV_DISABLE_X86) && defined(__x86_64__) &&
        // !defined(LIBYUV_ENABLE_ROWWIN)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

/*
 *  Copyright 2014 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(LIBYUV_DISABLE_NEON) && defined(__aarch64__)
//this ifdef should be removed if TransposeWx8_NEON's aarch64 has
//been done
#ifdef HAS_TRANSPOSE_WX8_NEON
static uvec8 kVTbl4x4Transpose =
  { 0,  4,  8, 12,  1,  5,  9, 13,  2,  6, 10, 14,  3,  7, 11, 15 };

void TransposeWx8_NEON(const uint8* src, int src_stride,
                       uint8* dst, int dst_stride,
                       int width) {
  const uint8* src_temp = NULL;
  asm volatile (
    // loops are on blocks of 8. loop will stop when
    // counter gets to or below 0. starting the counter
    // at w-8 allow for this
    "sub         %5, #8                        \n"

    // handle 8x8 blocks. this should be the majority of the plane
    ".p2align  2                               \n"
    "1:                                        \n"
      "mov         %0, %1                      \n"

      MEMACCESS(0)
      "vld1.8      {d0}, [%0], %2              \n"
      MEMACCESS(0)
      "vld1.8      {d1}, [%0], %2              \n"
      MEMACCESS(0)
      "vld1.8      {d2}, [%0], %2              \n"
      MEMACCESS(0)
      "vld1.8      {d3}, [%0], %2              \n"
      MEMACCESS(0)
      "vld1.8      {d4}, [%0], %2              \n"
      MEMACCESS(0)
      "vld1.8      {d5}, [%0], %2              \n"
      MEMACCESS(0)
      "vld1.8      {d6}, [%0], %2              \n"
      MEMACCESS(0)
      "vld1.8      {d7}, [%0]                  \n"

      "vtrn.8      d1, d0                      \n"
      "vtrn.8      d3, d2                      \n"
      "vtrn.8      d5, d4                      \n"
      "vtrn.8      d7, d6                      \n"

      "vtrn.16     d1, d3                      \n"
      "vtrn.16     d0, d2                      \n"
      "vtrn.16     d5, d7                      \n"
      "vtrn.16     d4, d6                      \n"

      "vtrn.32     d1, d5                      \n"
      "vtrn.32     d0, d4                      \n"
      "vtrn.32     d3, d7                      \n"
      "vtrn.32     d2, d6                      \n"

      "vrev16.8    q0, q0                      \n"
      "vrev16.8    q1, q1                      \n"
      "vrev16.8    q2, q2                      \n"
      "vrev16.8    q3, q3                      \n"

      "mov         %0, %3                      \n"

    MEMACCESS(0)
      "vst1.8      {d1}, [%0], %4              \n"
    MEMACCESS(0)
      "vst1.8      {d0}, [%0], %4              \n"
    MEMACCESS(0)
      "vst1.8      {d3}, [%0], %4              \n"
    MEMACCESS(0)
      "vst1.8      {d2}, [%0], %4              \n"
    MEMACCESS(0)
      "vst1.8      {d5}, [%0], %4              \n"
    MEMACCESS(0)
      "vst1.8      {d4}, [%0], %4              \n"
    MEMACCESS(0)
      "vst1.8      {d7}, [%0], %4              \n"
    MEMACCESS(0)
      "vst1.8      {d6}, [%0]                  \n"

      "add         %1, #8                      \n"  // src += 8
      "add         %3, %3, %4, lsl #3          \n"  // dst += 8 * dst_stride
      "subs        %5,  #8                     \n"  // w   -= 8
      "bge         1b                          \n"

    // add 8 back to counter. if the result is 0 there are
    // no residuals.
    "adds        %5, #8                        \n"
    "beq         4f                            \n"

    // some residual, so between 1 and 7 lines left to transpose
    "cmp         %5, #2                        \n"
    "blt         3f                            \n"

    "cmp         %5, #4                        \n"
    "blt         2f                            \n"

    // 4x8 block
    "mov         %0, %1                        \n"
    MEMACCESS(0)
    "vld1.32     {d0[0]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.32     {d0[1]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.32     {d1[0]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.32     {d1[1]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.32     {d2[0]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.32     {d2[1]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.32     {d3[0]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.32     {d3[1]}, [%0]                 \n"

    "mov         %0, %3                        \n"

    MEMACCESS(6)
    "vld1.8      {q3}, [%6]                    \n"

    "vtbl.8      d4, {d0, d1}, d6              \n"
    "vtbl.8      d5, {d0, d1}, d7              \n"
    "vtbl.8      d0, {d2, d3}, d6              \n"
    "vtbl.8      d1, {d2, d3}, d7              \n"

    // TODO(frkoenig): Rework shuffle above to
    // write out with 4 instead of 8 writes.
    MEMACCESS(0)
    "vst1.32     {d4[0]}, [%0], %4             \n"
    MEMACCESS(0)
    "vst1.32     {d4[1]}, [%0], %4             \n"
    MEMACCESS(0)
    "vst1.32     {d5[0]}, [%0], %4             \n"
    MEMACCESS(0)
    "vst1.32     {d5[1]}, [%0]                 \n"

    "add         %0, %3, #4                    \n"
    MEMACCESS(0)
    "vst1.32     {d0[0]}, [%0], %4             \n"
    MEMACCESS(0)
    "vst1.32     {d0[1]}, [%0], %4             \n"
    MEMACCESS(0)
    "vst1.32     {d1[0]}, [%0], %4             \n"
    MEMACCESS(0)
    "vst1.32     {d1[1]}, [%0]                 \n"

    "add         %1, #4                        \n"  // src += 4
    "add         %3, %3, %4, lsl #2            \n"  // dst += 4 * dst_stride
    "subs        %5,  #4                       \n"  // w   -= 4
    "beq         4f                            \n"

    // some residual, check to see if it includes a 2x8 block,
    // or less
    "cmp         %5, #2                        \n"
    "blt         3f                            \n"

    // 2x8 block
    "2:                                        \n"
    "mov         %0, %1                        \n"
    MEMACCESS(0)
    "vld1.16     {d0[0]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.16     {d1[0]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.16     {d0[1]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.16     {d1[1]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.16     {d0[2]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.16     {d1[2]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.16     {d0[3]}, [%0], %2             \n"
    MEMACCESS(0)
    "vld1.16     {d1[3]}, [%0]                 \n"

    "vtrn.8      d0, d1                        \n"

    "mov         %0, %3                        \n"

    MEMACCESS(0)
    "vst1.64     {d0}, [%0], %4                \n"
    MEMACCESS(0)
    "vst1.64     {d1}, [%0]                    \n"

    "add         %1, #2                        \n"  // src += 2
    "add         %3, %3, %4, lsl #1            \n"  // dst += 2 * dst_stride
    "subs        %5,  #2                       \n"  // w   -= 2
    "beq         4f                            \n"

    // 1x8 block
    "3:                                        \n"
    MEMACCESS(1)
    "vld1.8      {d0[0]}, [%1], %2             \n"
    MEMACCESS(1)
    "vld1.8      {d0[1]}, [%1], %2             \n"
    MEMACCESS(1)
    "vld1.8      {d0[2]}, [%1], %2             \n"
    MEMACCESS(1)
    "vld1.8      {d0[3]}, [%1], %2             \n"
    MEMACCESS(1)
    "vld1.8      {d0[4]}, [%1], %2             \n"
    MEMACCESS(1)
    "vld1.8      {d0[5]}, [%1], %2             \n"
    MEMACCESS(1)
    "vld1.8      {d0[6]}, [%1], %2             \n"
    MEMACCESS(1)
    "vld1.8      {d0[7]}, [%1]                 \n"

    MEMACCESS(3)
    "vst1.64     {d0}, [%3]                    \n"

    "4:                                        \n"

    : "+r"(src_temp),          // %0
      "+r"(src),               // %1
      "+r"(src_stride),        // %2
      "+r"(dst),               // %3
      "+r"(dst_stride),        // %4
      "+r"(width)              // %5
    : "r"(&kVTbl4x4Transpose)  // %6
    : "memory", "cc", "q0", "q1", "q2", "q3"
  );
}
#endif //HAS_TRANSPOSE_WX8_NEON

//this ifdef should be removed if TransposeUVWx8_NEON's aarch64 has
//been done
#ifdef HAS_TRANSPOSE_UVWX8_NEON
static uint8 kVTbl4x4TransposeDi[32] =
  { 0,  16, 32, 48,  2, 18, 34, 50,  4, 20, 36, 52,  6, 22, 38, 54,
    1,  17, 33, 49,  3, 19, 35, 51,  5, 21, 37, 53,  7, 23, 39, 55};

void TransposeUVWx8_NEON(const uint8* src, int src_stride,
                         uint8* dst_a, int dst_stride_a,
                         uint8* dst_b, int dst_stride_b,
                         int width) {
  const uint8* src_temp = NULL;
  asm volatile (
    // loops are on blocks of 8. loop will stop when
    // counter gets to or below 0. starting the counter
    // at w-8 allow for this
    "sub       %4, %4, #8                      \n"

    // handle 8x8 blocks. this should be the majority of the plane
    "1:                                        \n"
    "mov       %0, %1                          \n"

    MEMACCESS(0)
    "ld1       {v0.16b}, [%0], %5              \n"
    MEMACCESS(0)
    "ld1       {v1.16b}, [%0], %5              \n"
    MEMACCESS(0)
    "ld1       {v2.16b}, [%0], %5              \n"
    MEMACCESS(0)
    "ld1       {v3.16b}, [%0], %5              \n"
    MEMACCESS(0)
    "ld1       {v4.16b}, [%0], %5              \n"
    MEMACCESS(0)
    "ld1       {v5.16b}, [%0], %5              \n"
    MEMACCESS(0)
    "ld1       {v6.16b}, [%0], %5              \n"
    MEMACCESS(0)
    "ld1       {v7.16b}, [%0]                  \n"

    "trn1      v16.16b, v0.16b, v1.16b         \n"
    "trn2      v17.16b, v0.16b, v1.16b         \n"
    "trn1      v18.16b, v2.16b, v3.16b         \n"
    "trn2      v19.16b, v2.16b, v3.16b         \n"
    "trn1      v20.16b, v4.16b, v5.16b         \n"
    "trn2      v21.16b, v4.16b, v5.16b         \n"
    "trn1      v22.16b, v6.16b, v7.16b         \n"
    "trn2      v23.16b, v6.16b, v7.16b         \n"

    "trn1      v0.8h, v16.8h, v18.8h           \n"
    "trn2      v1.8h, v16.8h, v18.8h           \n"
    "trn1      v2.8h, v20.8h, v22.8h           \n"
    "trn2      v3.8h, v20.8h, v22.8h           \n"
    "trn1      v4.8h, v17.8h, v19.8h           \n"
    "trn2      v5.8h, v17.8h, v19.8h           \n"
    "trn1      v6.8h, v21.8h, v23.8h           \n"
    "trn2      v7.8h, v21.8h, v23.8h           \n"

    "trn1      v16.4s, v0.4s, v2.4s            \n"
    "trn2      v17.4s, v0.4s, v2.4s            \n"
    "trn1      v18.4s, v1.4s, v3.4s            \n"
    "trn2      v19.4s, v1.4s, v3.4s            \n"
    "trn1      v20.4s, v4.4s, v6.4s            \n"
    "trn2      v21.4s, v4.4s, v6.4s            \n"
    "trn1      v22.4s, v5.4s, v7.4s            \n"
    "trn2      v23.4s, v5.4s, v7.4s            \n"

    "mov       %0, %2                          \n"

    MEMACCESS(0)
    "st1       {v16.d}[0], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v18.d}[0], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v17.d}[0], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v19.d}[0], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v16.d}[1], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v18.d}[1], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v17.d}[1], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v19.d}[1], [%0]                \n"

    "mov       %0, %3                          \n"

    MEMACCESS(0)
    "st1       {v20.d}[0], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v22.d}[0], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v21.d}[0], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v23.d}[0], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v20.d}[1], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v22.d}[1], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v21.d}[1], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v23.d}[1], [%0]                \n"

    "add       %1, %1, #16                     \n"  // src   += 8*2
    "add       %2, %2, %6, lsl #3              \n"  // dst_a += 8 * dst_stride_a
    "add       %3, %3, %7, lsl #3              \n"  // dst_b += 8 * dst_stride_b
    "subs      %4, %4,  #8                     \n"  // w     -= 8
    "bge       1b                              \n"

    // add 8 back to counter. if the result is 0 there are
    // no residuals.
    "adds      %4, %4, #8                      \n"
    "beq       4f                              \n"

    // some residual, so between 1 and 7 lines left to transpose
    "cmp       %4, #2                          \n"
    "blt       3f                              \n"

    "cmp       %4, #4                          \n"
    "blt       2f                              \n"

    // TODO(frkoenig): Clean this up
    // 4x8 block
    "mov       %0, %1                          \n"
    MEMACCESS(0)
    "ld1       {v0.8b}, [%0], %5               \n"
    MEMACCESS(0)
    "ld1       {v1.8b}, [%0], %5               \n"
    MEMACCESS(0)
    "ld1       {v2.8b}, [%0], %5               \n"
    MEMACCESS(0)
    "ld1       {v3.8b}, [%0], %5               \n"
    MEMACCESS(0)
    "ld1       {v4.8b}, [%0], %5               \n"
    MEMACCESS(0)
    "ld1       {v5.8b}, [%0], %5               \n"
    MEMACCESS(0)
    "ld1       {v6.8b}, [%0], %5               \n"
    MEMACCESS(0)
    "ld1       {v7.8b}, [%0]                   \n"

    MEMACCESS(8)
    "ld1       {v30.16b}, [%8], #16            \n"
    "ld1       {v31.16b}, [%8]                 \n"

    "tbl       v16.16b, {v0.16b, v1.16b, v2.16b, v3.16b}, v30.16b  \n"
    "tbl       v17.16b, {v0.16b, v1.16b, v2.16b, v3.16b}, v31.16b  \n"
    "tbl       v18.16b, {v4.16b, v5.16b, v6.16b, v7.16b}, v30.16b  \n"
    "tbl       v19.16b, {v4.16b, v5.16b, v6.16b, v7.16b}, v31.16b  \n"

    "mov       %0, %2                          \n"

    MEMACCESS(0)
    "st1       {v16.s}[0],  [%0], %6           \n"
    MEMACCESS(0)
    "st1       {v16.s}[1],  [%0], %6           \n"
    MEMACCESS(0)
    "st1       {v16.s}[2],  [%0], %6           \n"
    MEMACCESS(0)
    "st1       {v16.s}[3],  [%0], %6           \n"

    "add       %0, %2, #4                      \n"
    MEMACCESS(0)
    "st1       {v18.s}[0], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v18.s}[1], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v18.s}[2], [%0], %6            \n"
    MEMACCESS(0)
    "st1       {v18.s}[3], [%0]                \n"

    "mov       %0, %3                          \n"

    MEMACCESS(0)
    "st1       {v17.s}[0], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v17.s}[1], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v17.s}[2], [%0], %7            \n"
    MEMACCESS(0)
    "st1       {v17.s}[3], [%0], %7            \n"

    "add       %0, %3, #4                      \n"
    MEMACCESS(0)
    "st1       {v19.s}[0],  [%0], %7           \n"
    MEMACCESS(0)
    "st1       {v19.s}[1],  [%0], %7           \n"
    MEMACCESS(0)
    "st1       {v19.s}[2],  [%0], %7           \n"
    MEMACCESS(0)
    "st1       {v19.s}[3],  [%0]               \n"

    "add       %1, %1, #8                      \n"  // src   += 4 * 2
    "add       %2, %2, %6, lsl #2              \n"  // dst_a += 4 * dst_stride_a
    "add       %3, %3, %7, lsl #2              \n"  // dst_b += 4 * dst_stride_b
    "subs      %4,  %4,  #4                    \n"  // w     -= 4
    "beq       4f                              \n"

    // some residual, check to see if it includes a 2x8 block,
    // or less
    "cmp       %4, #2                          \n"
    "blt       3f                              \n"

    // 2x8 block
    "2:                                        \n"
    "mov       %0, %1                          \n"
    MEMACCESS(0)
    "ld2       {v0.h, v1.h}[0], [%0], %5       \n"
    MEMACCESS(0)
    "ld2       {v2.h, v3.h}[0], [%0], %5       \n"
    MEMACCESS(0)
    "ld2       {v0.h, v1.h}[1], [%0], %5       \n"
    MEMACCESS(0)
    "ld2       {v2.h, v3.h}[1], [%0], %5       \n"
    MEMACCESS(0)
    "ld2       {v0.h, v1.h}[2], [%0], %5       \n"
    MEMACCESS(0)
    "ld2       {v2.h, v3.h}[2], [%0], %5       \n"
    MEMACCESS(0)
    "ld2       {v0.h, v1.h}[3], [%0], %5       \n"
    MEMACCESS(0)
    "ld2       {v2.h, v3.h}[3], [%0]           \n"

    "trn1      v4.8b, v0.8b, v2.8b             \n"
    "trn2      v5.8b, v0.8b, v2.8b             \n"
    "trn1      v6.8b, v1.8b, v3.8b             \n"
    "trn2      v7.8b, v1.8b, v3.8b             \n"

    "mov       %0, %2                          \n"

    MEMACCESS(0)
    "st1       {v4.d}[0], [%0], %6             \n"
    MEMACCESS(0)
    "st1       {v6.d}[0], [%0]                 \n"

    "mov       %0, %3                          \n"

    MEMACCESS(0)
    "st1       {v5.d}[0], [%0], %7             \n"
    MEMACCESS(0)
    "st1       {v7.d}[0], [%0]                 \n"

    "add       %1, %1, #4                      \n"  // src   += 2 * 2
    "add       %2, %2, %6, lsl #1              \n"  // dst_a += 2 * dst_stride_a
    "add       %3, %3, %7, lsl #1              \n"  // dst_b += 2 * dst_stride_b
    "subs      %4,  %4,  #2                    \n"  // w     -= 2
    "beq       4f                              \n"

    // 1x8 block
    "3:                                        \n"
    MEMACCESS(1)
    "ld2       {v0.b, v1.b}[0], [%1], %5       \n"
    MEMACCESS(1)
    "ld2       {v0.b, v1.b}[1], [%1], %5       \n"
    MEMACCESS(1)
    "ld2       {v0.b, v1.b}[2], [%1], %5       \n"
    MEMACCESS(1)
    "ld2       {v0.b, v1.b}[3], [%1], %5       \n"
    MEMACCESS(1)
    "ld2       {v0.b, v1.b}[4], [%1], %5       \n"
    MEMACCESS(1)
    "ld2       {v0.b, v1.b}[5], [%1], %5       \n"
    MEMACCESS(1)
    "ld2       {v0.b, v1.b}[6], [%1], %5       \n"
    MEMACCESS(1)
    "ld2       {v0.b, v1.b}[7], [%1]           \n"

    MEMACCESS(2)
    "st1       {v0.d}[0], [%2]                 \n"
    MEMACCESS(3)
    "st1       {v1.d}[0], [%3]                 \n"

    "4:                                        \n"

    : "+r"(src_temp),                             // %0
      "+r"(src),                                  // %1
      "+r"(dst_a),                                // %2
      "+r"(dst_b),                                // %3
      "+r"(width)                                 // %4
    : "r"(static_cast<ptrdiff_t>(src_stride)),    // %5
      "r"(static_cast<ptrdiff_t>(dst_stride_a)),  // %6
      "r"(static_cast<ptrdiff_t>(dst_stride_b)),  // %7
      "r"(&kVTbl4x4TransposeDi)                   // %8
    : "memory", "cc",
      "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
      "v30", "v31"
  );
}
#endif // __aarch64__

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"
#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(YUV_DISABLE_ASM) && defined(__mips__)
#ifdef HAS_SPLITUV_MIPS_DSPR2
void SplitUV_MIPS_DSPR2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int width) {

    __asm__ __volatile__(
        ".set push                                     \n\t"
        ".set noreorder                                \n\t"

        "srl             $t4, %[width], 4              \n\t"  // how many multiplies of 16 8bits
        "blez            $t4, 2f                       \n\t"
        " andi           %[width], %[width], 0xf       \n\t"  // residual
        "andi            $t0, %[src_uv], 0x3           \n\t"
        "andi            $t1, %[dst_u], 0x3            \n\t"
        "andi            $t2, %[dst_v], 0x3            \n\t"
        "or              $t0, $t0, $t1                 \n\t"
        "or              $t0, $t0, $t2                 \n\t"

        "beqz            $t0, 12f                      \n\t"  // if src and dsts are aligned
        " nop                                          \n\t"

        // src and dst are unaligned
        "1:                                            \n\t"
        "addiu           $t4, $t4, -1                  \n\t"
        "lwr             $t0, 0(%[src_uv])             \n\t"
        "lwl             $t0, 3(%[src_uv])             \n\t"  // t0 = V1 | U1 | V0 | U0
        "lwr             $t1, 4(%[src_uv])             \n\t"
        "lwl             $t1, 7(%[src_uv])             \n\t"  // t1 = V3 | U3 | V2 | U2
        "lwr             $t2, 8(%[src_uv])             \n\t"
        "lwl             $t2, 11(%[src_uv])            \n\t"  // t2 = V5 | U5 | V4 | U4
        "lwr             $t3, 12(%[src_uv])            \n\t"
        "lwl             $t3, 15(%[src_uv])            \n\t"  // t3 = V7 | U7 | V6 | U6
        "lwr             $t5, 16(%[src_uv])            \n\t"
        "lwl             $t5, 19(%[src_uv])            \n\t"  // t5 = V9 | U9 | V8 | U8
        "lwr             $t6, 20(%[src_uv])            \n\t"
        "lwl             $t6, 23(%[src_uv])            \n\t"  // t6 = V11 | U11 | V10 | U10
        "lwr             $t7, 24(%[src_uv])            \n\t"
        "lwl             $t7, 27(%[src_uv])            \n\t"  // t7 = V13 | U13 | V12 | U12
        "lwr             $t8, 28(%[src_uv])            \n\t"
        "lwl             $t8, 31(%[src_uv])            \n\t"  // t8 = V15 | U15 | V14 | U14

        "precrq.qb.ph    $t9, $t1, $t0                 \n\t"  // t9 = V3 | V2 | V1 | V0
        "precr.qb.ph     $t0, $t1, $t0                 \n\t"  // t0 = U3 | U2 | U1 | U0
        "precrq.qb.ph    $t1, $t3, $t2                 \n\t"  // t1 = V7 | V6 | V5 | V4
        "precr.qb.ph     $t2, $t3, $t2                 \n\t"  // t2 = U7 | U6 | U5 | U4
        "precrq.qb.ph    $t3, $t6, $t5                 \n\t"  // t3 = V11 | V10 | V9 | V8
        "precr.qb.ph     $t5, $t6, $t5                 \n\t"  // t5 = U11 | U10 | U9 | U8
        "precrq.qb.ph    $t6, $t8, $t7                 \n\t"  // t6 = V15 | V14 | V13 | V12
        "precr.qb.ph     $t7, $t8, $t7                 \n\t"  // t7 = U15 | U14 | U13 | U12
        "addiu           %[src_uv], %[src_uv], 32      \n\t"

        "swr             $t9, 0(%[dst_v])              \n\t"
        "swl             $t9, 3(%[dst_v])              \n\t"
        "swr             $t0, 0(%[dst_u])              \n\t"
        "swl             $t0, 3(%[dst_u])              \n\t"
        "swr             $t1, 4(%[dst_v])              \n\t"
        "swl             $t1, 7(%[dst_v])              \n\t"
        "swr             $t2, 4(%[dst_u])              \n\t"
        "swl             $t2, 7(%[dst_u])              \n\t"
        "swr             $t3, 8(%[dst_v])              \n\t"
        "swl             $t3, 11(%[dst_v])             \n\t"
        "swr             $t5, 8(%[dst_u])              \n\t"
        "swl             $t5, 11(%[dst_u])             \n\t"
        "swr             $t6, 12(%[dst_v])             \n\t"
        "swl             $t6, 15(%[dst_v])             \n\t"
        "swr             $t7, 12(%[dst_u])             \n\t"
        "swl             $t7, 15(%[dst_u])             \n\t"
        "addiu           %[dst_u], %[dst_u], 16        \n\t"
        "bgtz            $t4, 1b                       \n\t"
        " addiu          %[dst_v], %[dst_v], 16        \n\t"

        "beqz            %[width], 3f                  \n\t"
        " nop                                          \n\t"
        "b               2f                            \n\t"
        " nop                                          \n\t"

        // src and dst are aligned
        "12:                                           \n\t"
        "addiu           $t4, $t4, -1                  \n\t"
        "lw              $t0, 0(%[src_uv])             \n\t"  // t0 = V1 | U1 | V0 | U0
        "lw              $t1, 4(%[src_uv])             \n\t"  // t1 = V3 | U3 | V2 | U2
        "lw              $t2, 8(%[src_uv])             \n\t"  // t2 = V5 | U5 | V4 | U4
        "lw              $t3, 12(%[src_uv])            \n\t"  // t3 = V7 | U7 | V6 | U6
        "lw              $t5, 16(%[src_uv])            \n\t"  // t5 = V9 | U9 | V8 | U8
        "lw              $t6, 20(%[src_uv])            \n\t"  // t6 = V11 | U11 | V10 | U10
        "lw              $t7, 24(%[src_uv])            \n\t"  // t7 = V13 | U13 | V12 | U12
        "lw              $t8, 28(%[src_uv])            \n\t"  // t8 = V15 | U15 | V14 | U14

        "addiu           %[src_uv], %[src_uv], 32      \n\t"
        "precrq.qb.ph    $t9, $t1, $t0                 \n\t"  // t9 = V3 | V2 | V1 | V0
        "precr.qb.ph     $t0, $t1, $t0                 \n\t"  // t0 = U3 | U2 | U1 | U0
        "precrq.qb.ph    $t1, $t3, $t2                 \n\t"  // t1 = V7 | V6 | V5 | V4
        "precr.qb.ph     $t2, $t3, $t2                 \n\t"  // t2 = U7 | U6 | U5 | U4
        "precrq.qb.ph    $t3, $t6, $t5                 \n\t"  // t3 = V11 | V10 | V9 | V8
        "precr.qb.ph     $t5, $t6, $t5                 \n\t"  // t5 = U11 | U10 | U9 | U8
        "precrq.qb.ph    $t6, $t8, $t7                 \n\t"  // t6 = V15 | V14 | V13 | V12
        "precr.qb.ph     $t7, $t8, $t7                 \n\t"  // t7 = U15 | U14 | U13 | U12

        "sw              $t9, 0(%[dst_v])              \n\t"
        "sw              $t0, 0(%[dst_u])              \n\t"
        "sw              $t1, 4(%[dst_v])              \n\t"
        "sw              $t2, 4(%[dst_u])              \n\t"
        "sw              $t3, 8(%[dst_v])              \n\t"
        "sw              $t5, 8(%[dst_u])              \n\t"
        "sw              $t6, 12(%[dst_v])             \n\t"
        "sw              $t7, 12(%[dst_u])             \n\t"
        "addiu           %[dst_v], %[dst_v], 16        \n\t"
        "bgtz            $t4, 12b                      \n\t"
        " addiu          %[dst_u], %[dst_u], 16        \n\t"

        "beqz            %[width], 3f                  \n\t"
        " nop                                          \n\t"

        "2:                                            \n\t"
        "lbu             $t0, 0(%[src_uv])             \n\t"
        "lbu             $t1, 1(%[src_uv])             \n\t"
        "addiu           %[src_uv], %[src_uv], 2       \n\t"
        "addiu           %[width], %[width], -1        \n\t"
        "sb              $t0, 0(%[dst_u])              \n\t"
        "sb              $t1, 0(%[dst_v])              \n\t"
        "addiu           %[dst_u], %[dst_u], 1         \n\t"
        "bgtz            %[width], 2b                  \n\t"
        " addiu          %[dst_v], %[dst_v], 1         \n\t"

        "3:                                            \n\t"
        ".set pop                                      \n\t"
         : [src_uv] "+r" (src_uv), [width] "+r" (width),
           [dst_u] "+r" (dst_u), [dst_v] "+r" (dst_v)
         :
         : "t0", "t1","t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9"
    );
}
#endif  // HAS_SPLITUV_MIPS_DSPR2


#ifdef HAS_SPLITUV_MIPS_DSPR2
// Reads 16 pairs of UV and write even values to dst_u and odd to dst_v
// Alignment requirement: 16 bytes for pointers, and multiple of 16 pixels.
void SplitUV_MIPS_DSPR2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    ".p2align  2                               \n"
  "1:                                          \n"
    "vld2.u8    {q0, q1}, [%0]!                \n"  // load 16 pairs of UV
    "subs       %3, %3, #16                    \n"  // 16 processed per loop
    "vst1.u8    {q0}, [%1]!                    \n"  // store U
    "vst1.u8    {q1}, [%2]!                    \n"  // Store V
    "bgt        1b                             \n"
    : "+r"(src_uv),  // %0
      "+r"(dst_u),   // %1
      "+r"(dst_v),   // %2
      "+r"(width)    // %3  // Output registers
    :                       // Input registers
    : "memory", "cc", "q0", "q1"  // Clobber List
  );
}
#endif  // HAS_SPLITUV_MIPS_DSPR2

#ifdef HAS_SPLITUV_MIPS_DSPR2
// Reads 4 pairs of UV and write even values to dst_u and odd to dst_v
// Alignment requirement: 4 bytes for pointers, and multiple of 4 pixels.
void SplitUV_MIPS_DSPR2(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                        int width) {
  asm volatile (
    ".set push                                 \n"
    ".set noreorder                            \n"
    ".p2align  2                               \n"
  "1:                                          \n"
    "lw            $t0, 0(%[src_uv])           \n"  // V1 | U1 | V0 | U0
    "lw            $t1, 4(%[src_uv])           \n"  // V3 | U3 | V2 | U2
    "addiu         %[width], %[width], -4      \n"
    "addiu         %[src_uv], %[src_uv], 8     \n"
    "precr.qb.ph   $t2, $t1, $t0               \n"  // U3 | U2 | U1 | U0
    "precrq.qb.ph  $t3, $t1, $t0               \n"  // V3 | V2 | V1 | V0
    "sw            $t2, 0(%[dst_u])            \n"
    "sw            $t3, 0(%[dst_v])            \n"
    "addiu         %[dst_u], %[dst_u], 4       \n"
    "bgtz          %[width], 1b                \n"
    " addiu        %[dst_v], %[dst_v], 4       \n"
    ".set pop                                  \n"
    : [src_uv] "+r" (src_uv),
      [width] "+r" (width),
      [dst_u] "+r" (dst_u),
      [dst_v] "+r" (dst_v)
    :
    : "t0", "t1","t2", "t3",
  );
}
#endif  // HAS_SPLITUV_MIPS_DSPR2
#endif  // __mips__

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

/*
 *  Copyright (c) 2012 The LibYuv project authors. All Rights Reserved.
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
void SplitUV_MIPS_DSPR2(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                        int width) {
  __asm__ __volatile__ (
    ".set push                                     \n"
    ".set noreorder                                \n"
    "srl             $t4, %[width], 4              \n"  // multiplies of 16
    "blez            $t4, 2f                       \n"
    " andi           %[width], %[width], 0xf       \n"  // residual

  "1:                                              \n"
    "addiu           $t4, $t4, -1                  \n"
    "lw              $t0, 0(%[src_uv])             \n"  // V1 | U1 | V0 | U0
    "lw              $t1, 4(%[src_uv])             \n"  // V3 | U3 | V2 | U2
    "lw              $t2, 8(%[src_uv])             \n"  // V5 | U5 | V4 | U4
    "lw              $t3, 12(%[src_uv])            \n"  // V7 | U7 | V6 | U6
    "lw              $t5, 16(%[src_uv])            \n"  // V9 | U9 | V8 | U8
    "lw              $t6, 20(%[src_uv])            \n"  // V11 | U11 | V10 | U10
    "lw              $t7, 24(%[src_uv])            \n"  // V13 | U13 | V12 | U12
    "lw              $t8, 28(%[src_uv])            \n"  // V15 | U15 | V14 | U14
    "addiu           %[src_uv], %[src_uv], 32      \n"
    "precrq.qb.ph    $t9, $t1, $t0                 \n"  // V3 | V2 | V1 | V0
    "precr.qb.ph     $t0, $t1, $t0                 \n"  // U3 | U2 | U1 | U0
    "precrq.qb.ph    $t1, $t3, $t2                 \n"  // V7 | V6 | V5 | V4
    "precr.qb.ph     $t2, $t3, $t2                 \n"  // U7 | U6 | U5 | U4
    "precrq.qb.ph    $t3, $t6, $t5                 \n"  // V11 | V10 | V9 | V8
    "precr.qb.ph     $t5, $t6, $t5                 \n"  // U11 | U10 | U9 | U8
    "precrq.qb.ph    $t6, $t8, $t7                 \n"  // V15 | V14 | V13 | V12
    "precr.qb.ph     $t7, $t8, $t7                 \n"  // U15 | U14 | U13 | U12
    "sw              $t9, 0(%[dst_v])              \n"
    "sw              $t0, 0(%[dst_u])              \n"
    "sw              $t1, 4(%[dst_v])              \n"
    "sw              $t2, 4(%[dst_u])              \n"
    "sw              $t3, 8(%[dst_v])              \n"
    "sw              $t5, 8(%[dst_u])              \n"
    "sw              $t6, 12(%[dst_v])             \n"
    "sw              $t7, 12(%[dst_u])             \n"
    "addiu           %[dst_v], %[dst_v], 16        \n"
    "bgtz            $t4, 1b                       \n"
    " addiu          %[dst_u], %[dst_u], 16        \n"

    "beqz            %[width], 3f                  \n"
    " nop                                          \n"

  "2:                                              \n"
    "lbu             $t0, 0(%[src_uv])             \n"
    "lbu             $t1, 1(%[src_uv])             \n"
    "addiu           %[src_uv], %[src_uv], 2       \n"
    "addiu           %[width], %[width], -1        \n"
    "sb              $t0, 0(%[dst_u])              \n"
    "sb              $t1, 0(%[dst_v])              \n"
    "addiu           %[dst_u], %[dst_u], 1         \n"
    "bgtz            %[width], 2b                  \n"
    " addiu          %[dst_v], %[dst_v], 1         \n"

  "3:                                              \n"
    ".set pop                                      \n"
     : [src_uv] "+r" (src_uv),
       [width] "+r" (width),
       [dst_u] "+r" (dst_u),
       [dst_v] "+r" (dst_v)
     :
     : "t0", "t1", "t2", "t3",
       "t4", "t5", "t6", "t7", "t8", "t9"
  );
}

void SplitUV_Unaligned_MIPS_DSPR2(const uint8* src_uv, uint8* dst_u,
                                  uint8* dst_v, int width) {
  __asm__ __volatile__ (
    ".set push                                     \n"
    ".set noreorder                                \n"
    "srl             $t4, %[width], 4              \n"  // multiplies of 16
    "blez            $t4, 2f                       \n"
    " andi           %[width], %[width], 0xf       \n"  // residual

  "1:                                              \n"
    "addiu           $t4, $t4, -1                  \n"
    "lwr             $t0, 0(%[src_uv])             \n"
    "lwl             $t0, 3(%[src_uv])             \n"  // V1 | U1 | V0 | U0
    "lwr             $t1, 4(%[src_uv])             \n"
    "lwl             $t1, 7(%[src_uv])             \n"  // V3 | U3 | V2 | U2
    "lwr             $t2, 8(%[src_uv])             \n"
    "lwl             $t2, 11(%[src_uv])            \n"  // V5 | U5 | V4 | U4
    "lwr             $t3, 12(%[src_uv])            \n"
    "lwl             $t3, 15(%[src_uv])            \n"  // V7 | U7 | V6 | U6
    "lwr             $t5, 16(%[src_uv])            \n"
    "lwl             $t5, 19(%[src_uv])            \n"  // V9 | U9 | V8 | U8
    "lwr             $t6, 20(%[src_uv])            \n"
    "lwl             $t6, 23(%[src_uv])            \n"  // V11 | U11 | V10 | U10
    "lwr             $t7, 24(%[src_uv])            \n"
    "lwl             $t7, 27(%[src_uv])            \n"  // V13 | U13 | V12 | U12
    "lwr             $t8, 28(%[src_uv])            \n"
    "lwl             $t8, 31(%[src_uv])            \n"  // V15 | U15 | V14 | U14
    "precrq.qb.ph    $t9, $t1, $t0                 \n"  // V3 | V2 | V1 | V0
    "precr.qb.ph     $t0, $t1, $t0                 \n"  // U3 | U2 | U1 | U0
    "precrq.qb.ph    $t1, $t3, $t2                 \n"  // V7 | V6 | V5 | V4
    "precr.qb.ph     $t2, $t3, $t2                 \n"  // U7 | U6 | U5 | U4
    "precrq.qb.ph    $t3, $t6, $t5                 \n"  // V11 | V10 | V9 | V8
    "precr.qb.ph     $t5, $t6, $t5                 \n"  // U11 | U10 | U9 | U8
    "precrq.qb.ph    $t6, $t8, $t7                 \n"  // V15 | V14 | V13 | V12
    "precr.qb.ph     $t7, $t8, $t7                 \n"  // U15 | U14 | U13 | U12
    "addiu           %[src_uv], %[src_uv], 32      \n"
    "swr             $t9, 0(%[dst_v])              \n"
    "swl             $t9, 3(%[dst_v])              \n"
    "swr             $t0, 0(%[dst_u])              \n"
    "swl             $t0, 3(%[dst_u])              \n"
    "swr             $t1, 4(%[dst_v])              \n"
    "swl             $t1, 7(%[dst_v])              \n"
    "swr             $t2, 4(%[dst_u])              \n"
    "swl             $t2, 7(%[dst_u])              \n"
    "swr             $t3, 8(%[dst_v])              \n"
    "swl             $t3, 11(%[dst_v])             \n"
    "swr             $t5, 8(%[dst_u])              \n"
    "swl             $t5, 11(%[dst_u])             \n"
    "swr             $t6, 12(%[dst_v])             \n"
    "swl             $t6, 15(%[dst_v])             \n"
    "swr             $t7, 12(%[dst_u])             \n"
    "swl             $t7, 15(%[dst_u])             \n"
    "addiu           %[dst_u], %[dst_u], 16        \n"
    "bgtz            $t4, 1b                       \n"
    " addiu          %[dst_v], %[dst_v], 16        \n"

    "beqz            %[width], 3f                  \n"
    " nop                                          \n"

  "2:                                              \n"
    "lbu             $t0, 0(%[src_uv])             \n"
    "lbu             $t1, 1(%[src_uv])             \n"
    "addiu           %[src_uv], %[src_uv], 2       \n"
    "addiu           %[width], %[width], -1        \n"
    "sb              $t0, 0(%[dst_u])              \n"
    "sb              $t1, 0(%[dst_v])              \n"
    "addiu           %[dst_u], %[dst_u], 1         \n"
    "bgtz            %[width], 2b                  \n"
    " addiu          %[dst_v], %[dst_v], 1         \n"

  "3:                                              \n"
    ".set pop                                      \n"
     : [src_uv] "+r" (src_uv),
       [width] "+r" (width),
       [dst_u] "+r" (dst_u),
       [dst_v] "+r" (dst_v)
     :
     : "t0", "t1", "t2", "t3",
       "t4", "t5", "t6", "t7", "t8", "t9"
  );
}
#endif  // HAS_SPLITUV_MIPS_DSPR2

#endif  // __mips__

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

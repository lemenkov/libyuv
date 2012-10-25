/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/basic_types.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// This module is for GCC MIPS DSPR2
#if !defined(YUV_DISABLE_ASM) && defined(__mips_dsp) && (__mips_dsp_rev >= 2)

void ScaleRowDown2_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t /* src_stride */,
                              uint8* dst, int dst_width) {
  __asm__ __volatile__(
    ".set push                                     \n"
    ".set noreorder                                \n"

    "srl            $t9, %[dst_width], 4           \n"  // iterations -> by 16
    "beqz           $t9, 2f                        \n"
    " nop                                          \n"

  "1:                                              \n"
    "lw             $t0, 0(%[src_ptr])             \n"  // |3|2|1|0|
    "lw             $t1, 4(%[src_ptr])             \n"  // |7|6|5|4|
    "lw             $t2, 8(%[src_ptr])             \n"  // |11|10|9|8|
    "lw             $t3, 12(%[src_ptr])            \n"  // |15|14|13|12|
    "lw             $t4, 16(%[src_ptr])            \n"  // |19|18|17|16|
    "lw             $t5, 20(%[src_ptr])            \n"  // |23|22|21|20|
    "lw             $t6, 24(%[src_ptr])            \n"  // |27|26|25|24|
    "lw             $t7, 28(%[src_ptr])            \n"  // |31|30|29|28|
    "precr.qb.ph    $t8, $t1, $t0                  \n"  // |6|4|2|0|
    "precr.qb.ph    $t0, $t3, $t2                  \n"  // |14|12|10|8|
    "precr.qb.ph    $t1, $t5, $t4                  \n"  // |22|20|18|16|
    "precr.qb.ph    $t2, $t7, $t6                  \n"  // |30|28|26|24|
    "addiu          %[src_ptr], %[src_ptr], 32     \n"
    "addiu          $t9, $t9, -1                   \n"
    "sw             $t8, 0(%[dst])                 \n"
    "sw             $t0, 4(%[dst])                 \n"
    "sw             $t1, 8(%[dst])                 \n"
    "sw             $t2, 12(%[dst])                \n"
    "bgtz           $t9, 1b                        \n"
    " addiu         %[dst], %[dst], 16             \n"

  "2:                                              \n"
    "andi           $t9, %[dst_width], 0xf         \n"  // residue
    "beqz           $t9, 3f                        \n"
    " nop                                          \n"

  "21:                                             \n"
    "lbu            $t0, 0(%[src_ptr])             \n"
    "addiu          %[src_ptr], %[src_ptr], 2      \n"
    "addiu          $t9, $t9, -1                   \n"
    "sb             $t0, 0(%[dst])                 \n"
    "bgtz           $t9, 21b                       \n"
    " addiu         %[dst], %[dst], 1              \n"

  "3:                                              \n"
    ".set pop                                      \n"
  : [src_ptr] "+r" (src_ptr),
    [dst] "+r" (dst)
  : [dst_width] "r" (dst_width)
  : "t0", "t1", "t2", "t3", "t4", "t5",
    "t6", "t7", "t8", "t9"
  );
}

void ScaleRowDown2Int_MIPS_DSPR2(const uint8* src_ptr, ptrdiff_t src_stride,
                                 uint8* dst, int dst_width) {
  const uint8* t = src_ptr + src_stride;

  __asm__ __volatile__ (
    ".set push                                    \n"
    ".set noreorder                               \n"

    "srl            $t9, %[dst_width], 3          \n"  // iterations -> step 8
    "bltz           $t9, 2f                       \n"
    " nop                                         \n"

  "1:                                             \n"
    "lw             $t0, 0(%[src_ptr])            \n"  // |3|2|1|0|
    "lw             $t1, 4(%[src_ptr])            \n"  // |7|6|5|4|
    "lw             $t2, 8(%[src_ptr])            \n"  // |11|10|9|8|
    "lw             $t3, 12(%[src_ptr])           \n"  // |15|14|13|12|
    "lw             $t4, 0(%[t])                  \n"  // |19|18|17|16|
    "lw             $t5, 4(%[t])                  \n"  // |23|22|21|20|
    "lw             $t6, 8(%[t])                  \n"  // |27|26|25|24|
    "lw             $t7, 12(%[t])                 \n"  // |31|30|29|28|
    "addiu          $t9, $t9, -1                  \n"
    "srl            $t8, $t0, 16                  \n"  // |X|X|3|2|
    "ins            $t0, $t4, 16, 16              \n"  // |17|16|1|0|
    "ins            $t4, $t8, 0, 16               \n"  // |19|18|3|2|
    "raddu.w.qb     $t0, $t0                      \n"  // |17+16+1+0|
    "raddu.w.qb     $t4, $t4                      \n"  // |19+18+3+2|
    "shra_r.w       $t0, $t0, 2                   \n"  // |t0+2|>>2
    "shra_r.w       $t4, $t4, 2                   \n"  // |t4+2|>>2
    "srl            $t8, $t1, 16                  \n"  // |X|X|7|6|
    "ins            $t1, $t5, 16, 16              \n"  // |21|20|5|4|
    "ins            $t5, $t8, 0, 16               \n"  // |22|23|7|6|
    "raddu.w.qb     $t1, $t1                      \n"  // |21+20+5+4|
    "raddu.w.qb     $t5, $t5                      \n"  // |23+22+7+6|
    "shra_r.w       $t1, $t1, 2                   \n"  // |t1+2|>>2
    "shra_r.w       $t5, $t5, 2                   \n"  // |t5+2|>>2
    "srl            $t8, $t2, 16                  \n"  // |X|X|11|10|
    "ins            $t2, $t6, 16, 16              \n"  // |25|24|9|8|
    "ins            $t6, $t8, 0, 16               \n"  // |27|26|11|10|
    "raddu.w.qb     $t2, $t2                      \n"  // |25+24+9+8|
    "raddu.w.qb     $t6, $t6                      \n"  // |27+26+11+10|
    "shra_r.w       $t2, $t2, 2                   \n"  // |t2+2|>>2
    "shra_r.w       $t6, $t6, 2                   \n"  // |t5+2|>>2
    "srl            $t8, $t3, 16                  \n"  // |X|X|15|14|
    "ins            $t3, $t7, 16, 16              \n"  // |29|28|13|12|
    "ins            $t7, $t8, 0, 16               \n"  // |31|30|15|14|
    "raddu.w.qb     $t3, $t3                      \n"  // |29+28+13+12|
    "raddu.w.qb     $t7, $t7                      \n"  // |31+30+15+14|
    "shra_r.w       $t3, $t3, 2                   \n"  // |t3+2|>>2
    "shra_r.w       $t7, $t7, 2                   \n"  // |t7+2|>>2
    "addiu          %[src_ptr], %[src_ptr], 16    \n"
    "addiu          %[t], %[t], 16                \n"
    "sb             $t0, 0(%[dst])                \n"
    "sb             $t4, 1(%[dst])                \n"
    "sb             $t1, 2(%[dst])                \n"
    "sb             $t5, 3(%[dst])                \n"
    "sb             $t2, 4(%[dst])                \n"
    "sb             $t6, 5(%[dst])                \n"
    "sb             $t3, 6(%[dst])                \n"
    "sb             $t7, 7(%[dst])                \n"
    "bgtz           $t9, 1b                       \n"
    " addiu         %[dst], %[dst], 8             \n"

  "2:                                             \n"
    "andi           $t9, %[dst_width], 0x7        \n"  // x = residue
    "beqz           $t9, 3f                       \n"
    " nop                                         \n"

    "21:                                          \n"
    "lwr            $t1, 0(%[src_ptr])            \n"
    "lwl            $t1, 3(%[src_ptr])            \n"
    "lwr            $t2, 0(%[t])                  \n"
    "lwl            $t2, 3(%[t])                  \n"
    "srl            $t8, $t1, 16                  \n"
    "ins            $t1, $t2, 16, 16              \n"
    "ins            $t2, $t8, 0, 16               \n"
    "raddu.w.qb     $t1, $t1                      \n"
    "raddu.w.qb     $t2, $t2                      \n"
    "shra_r.w       $t1, $t1, 2                   \n"
    "shra_r.w       $t2, $t2, 2                   \n"
    "sb             $t1, 0(%[dst])                \n"
    "sb             $t2, 1(%[dst])                \n"
    "addiu          %[src_ptr], %[src_ptr], 4     \n"
    "addiu          $t9, $t9, -2                  \n"
    "addiu          %[t], %[t], 4                 \n"
    "bgtz           $t9, 21b                      \n"
    " addiu         %[dst], %[dst], 2             \n"

  "3:                                             \n"
    ".set pop                                     \n"

  : [src_ptr] "+r" (src_ptr),
    [dst] "+r" (dst), [t] "+r" (t)
  : [dst_width] "r" (dst_width)
  : "t0", "t1", "t2", "t3", "t4", "t5",
    "t6", "t7", "t8", "t9"
  );
}

void ScaleFilterRows_MIPS_DSPR2(unsigned char *dst_ptr,
                                const unsigned char* src_ptr,
                                ptrdiff_t src_stride,
                                int dst_width, int source_y_fraction) {
    int y0_fraction = 256 - source_y_fraction;
    const unsigned char* src_ptr1 = src_ptr + src_stride;

  __asm__ __volatile__ (
     ".set push                                           \n"
     ".set noreorder                                      \n"

     "replv.ph          $t0, %[y0_fraction]               \n"
     "replv.ph          $t1, %[source_y_fraction]         \n"
   "1:                                                    \n"
     "lw                $t2, 0(%[src_ptr])                \n"
     "lw                $t3, 0(%[src_ptr1])               \n"
     "lw                $t4, 4(%[src_ptr])                \n"
     "lw                $t5, 4(%[src_ptr1])               \n"
     "muleu_s.ph.qbl    $t6, $t2, $t0                     \n"
     "muleu_s.ph.qbr    $t7, $t2, $t0                     \n"
     "muleu_s.ph.qbl    $t8, $t3, $t1                     \n"
     "muleu_s.ph.qbr    $t9, $t3, $t1                     \n"
     "muleu_s.ph.qbl    $t2, $t4, $t0                     \n"
     "muleu_s.ph.qbr    $t3, $t4, $t0                     \n"
     "muleu_s.ph.qbl    $t4, $t5, $t1                     \n"
     "muleu_s.ph.qbr    $t5, $t5, $t1                     \n"
     "addq.ph           $t6, $t6, $t8                     \n"
     "addq.ph           $t7, $t7, $t9                     \n"
     "addq.ph           $t2, $t2, $t4                     \n"
     "addq.ph           $t3, $t3, $t5                     \n"
     "shra.ph           $t6, $t6, 8                       \n"
     "shra.ph           $t7, $t7, 8                       \n"
     "shra.ph           $t2, $t2, 8                       \n"
     "shra.ph           $t3, $t3, 8                       \n"
     "precr.qb.ph       $t6, $t6, $t7                     \n"
     "precr.qb.ph       $t2, $t2, $t3                     \n"
     "addiu             %[src_ptr], %[src_ptr], 8         \n"
     "addiu             %[src_ptr1], %[src_ptr1], 8       \n"
     "addiu             %[dst_width], %[dst_width], -8    \n"
     "sw                $t6, 0(%[dst_ptr])                \n"
     "sw                $t2, 4(%[dst_ptr])                \n"
     "bgtz              %[dst_width], 1b                  \n"
     " addiu            %[dst_ptr], %[dst_ptr], 8         \n"

     "lbu               $t0, -1(%[dst_ptr])               \n"
     "sb                $t0, 0(%[dst_ptr])                \n"
     ".set pop                                            \n"
  : [dst_ptr] "+r" (dst_ptr),
    [src_ptr1] "+r" (src_ptr1),
    [src_ptr] "+r" (src_ptr),
    [dst_width] "+r" (dst_width)
  : [source_y_fraction] "r" (source_y_fraction),
    [y0_fraction] "r" (y0_fraction),
    [src_stride] "r" (src_stride)
  : "t0", "t1", "t2", "t3", "t4", "t5",
    "t6", "t7", "t8", "t9"
  );
}
#endif  // defined(__mips_dsp) && (__mips_dsp_rev >= 2)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif


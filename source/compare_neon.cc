/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/basic_types.h"

#include "libyuv/compare_row.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(LIBYUV_DISABLE_NEON) && defined(__ARM_NEON__) && \
    !defined(__aarch64__)

// 256 bits at a time
uint32 HammingDistance_NEON(const uint8* src_a, const uint8* src_b, int count) {
  uint32 diff;
  uint32 total_diff = 0;

  for (int i = 0; i < count; i += 32, src_a += 32, src_b += 32) {

  __asm__ volatile(
      // Load constants.
      "vmov.u8             q12, #0x55          \n\t"  // m1.
      "vmov.u8             q13, #0x33          \n\t"  // m2.
      "vmov.u8             q14, #0x0f          \n\t"  // m4.
      "vmov.u8             q15, #0x01          \n\t"  // h01.

      // Load d1
      "vld1.32             {q0,q1}, [%1]       \n\t"  // load d1.

      // Load d2
      "vld1.32             {q2, q3}, [%2]      \n\t"  // load d2.

      // xor
      "veor.32             q0, q0, q2          \n\t"  // xor left side.
      "veor.32             q3, q1, q3          \n\t"  // xor right side.

      // x -= (x >> 1) & m1;
      "vshr.u32            q1, q0, #1           \n\t"
      "vshr.u32            q4, q3, #1           \n\t"
      "vand.32             q1, q1, q12         \n\t"
      "vand.32             q4, q4, q12         \n\t"
      "vsub.u32            q0, q0, q1          \n\t"
      "vsub.u32            q3, q3, q4          \n\t"

      // x = (x & m2) + ((x >> 2) & m2);
      "vand.32             q1, q0, q13         \n\t"
      "vand.32             q4, q3, q13         \n\t"
      "vshr.u32            q2, q0, #2           \n\t"
      "vshr.u32            q5, q3, #2           \n\t"
      "vand.32             q2, q2, q13         \n\t"
      "vand.32             q5, q5, q13         \n\t"
      "vadd.u32            q0, q1, q2          \n\t"
      "vadd.u32            q3, q4, q5          \n\t"

      // x = (x + (x >> 4)) & m4;
      "vshr.u32            q1, q0, #4           \n\t"
      "vshr.u32            q4, q3, #4           \n\t"
      "vadd.u32            q0, q0, q1          \n\t"
      "vadd.u32            q3, q3, q4          \n\t"
      "vand.32             q0, q0, q14         \n\t"
      "vand.32             q3, q3, q14         \n\t"

      // (x * h01) >> 24;
      "vmul.u32            q0, q0, q15         \n\t"
      "vmul.u32            q3, q3, q15         \n\t"
      "vshr.u32            q0, q0, #24          \n\t"
      "vshr.u32            q3, q3, #24          \n\t"

      // sum distances
      "vpadd.u32           d0, d0, d1          \n\t"
      "vpadd.u32           d6, d6, d7          \n\t"
      "vpadd.u32           d0, d0, d0          \n\t"
      "vpadd.u32           d6, d6, d6          \n\t"

      // add d0,d6.
      "vadd.u32            d0, d0, d6          \n\t"

      // Move distance to return register.
      "vmov.32             %0, d0[0]           \n\t"

      // Output.
      : "=r"(diff), "+r"(src_a), "+r"(src_b)
        // input
      :
      // Clobber list.
      : "q0", "q1", "q2", "q3", "q4", "q5", "q12", "q13", "q14", "q15");
    total_diff += diff;
  }
  return total_diff;
}

uint32 SumSquareError_NEON(const uint8* src_a, const uint8* src_b, int count) {
  uint32 sse;
  asm volatile (
    "vmov.u8    q8, #0                         \n"
    "vmov.u8    q10, #0                        \n"
    "vmov.u8    q9, #0                         \n"
    "vmov.u8    q11, #0                        \n"

  "1:                                          \n"
    MEMACCESS(0)
    "vld1.8     {q0}, [%0]!                    \n"
    MEMACCESS(1)
    "vld1.8     {q1}, [%1]!                    \n"
    "subs       %2, %2, #16                    \n"
    "vsubl.u8   q2, d0, d2                     \n"
    "vsubl.u8   q3, d1, d3                     \n"
    "vmlal.s16  q8, d4, d4                     \n"
    "vmlal.s16  q9, d6, d6                     \n"
    "vmlal.s16  q10, d5, d5                    \n"
    "vmlal.s16  q11, d7, d7                    \n"
    "bgt        1b                             \n"

    "vadd.u32   q8, q8, q9                     \n"
    "vadd.u32   q10, q10, q11                  \n"
    "vadd.u32   q11, q8, q10                   \n"
    "vpaddl.u32 q1, q11                        \n"
    "vadd.u64   d0, d2, d3                     \n"
    "vmov.32    %3, d0[0]                      \n"
    : "+r"(src_a),
      "+r"(src_b),
      "+r"(count),
      "=r"(sse)
    :
    : "memory", "cc", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11");
  return sse;
}

#endif  // defined(__ARM_NEON__) && !defined(__aarch64__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

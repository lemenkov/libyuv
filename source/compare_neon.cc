/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/compare_row.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Note: every register written by an inline asm block (vector, general
// purpose, mask/predicate, "cc" for flags and "memory" for stores) must be
// listed in its clobber list, even when the ABI treats it as caller-saved.
// With LTO/LTCG the compiler may keep values live in any register across the
// asm statement, and other OS ABIs (e.g. Windows) differ in which registers
// are callee-saved. List "memory" first, then "cc", then registers.

#if !defined(LIBYUV_DISABLE_NEON) && defined(__ARM_NEON__) && \
    !defined(__aarch64__)

// 256 bits at a time
// uses short accumulator which restricts count to 131 KB
uint32_t HammingDistance_NEON(const uint8_t* src_a,
                              const uint8_t* src_b,
                              int count) {
  uint32_t diff;

  asm volatile(
      "vmov.u16    q4, #0                        \n"  // accumulator

      "1:          \n"
      "vld1.8      {q0, q1}, [%0]!               \n"
      "vld1.8      {q2, q3}, [%1]!               \n"
      "veor.32     q0, q0, q2                    \n"
      "veor.32     q1, q1, q3                    \n"
      "vcnt.i8     q0, q0                        \n"
      "vcnt.i8     q1, q1                        \n"
      "subs        %2, %2, #32                   \n"
      "vadd.u8     q0, q0, q1                    \n"  // 16 byte counts
      "vpadal.u8   q4, q0                        \n"  // 8 shorts
      "bgt         1b                            \n"

      "vpaddl.u16  q0, q4                        \n"  // 4 ints
      "vpadd.u32   d0, d0, d1                    \n"
      "vpadd.u32   d0, d0, d0                    \n"
      "vmov.32     %3, d0[0]                     \n"

      : "+r"(src_a), "+r"(src_b), "+r"(count), "=r"(diff)
      :
      : "cc", "q0", "q1", "q2", "q3", "q4");
  return diff;
}

uint32_t SumSquareError_NEON(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count) {
  uint32_t sse;
  asm volatile(
      "vmov.u8     q8, #0                        \n"
      "vmov.u8     q10, #0                       \n"
      "vmov.u8     q9, #0                        \n"
      "vmov.u8     q11, #0                       \n"

      "1:          \n"
      "vld1.8      {q0}, [%0]!                   \n"
      "vld1.8      {q1}, [%1]!                   \n"
      "subs        %2, %2, #16                   \n"
      "vsubl.u8    q2, d0, d2                    \n"
      "vsubl.u8    q3, d1, d3                    \n"
      "vmlal.s16   q8, d4, d4                    \n"
      "vmlal.s16   q9, d6, d6                    \n"
      "vmlal.s16   q10, d5, d5                   \n"
      "vmlal.s16   q11, d7, d7                   \n"
      "bgt         1b                            \n"

      "vadd.u32    q8, q8, q9                    \n"
      "vadd.u32    q10, q10, q11                 \n"
      "vadd.u32    q11, q8, q10                  \n"
      "vpaddl.u32  q1, q11                       \n"
      "vadd.u64    d0, d2, d3                    \n"
      "vmov.32     %3, d0[0]                     \n"
      : "+r"(src_a), "+r"(src_b), "+r"(count), "=r"(sse)
      :
      : "memory", "cc", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11");
  return sse;
}

#define SSIM_ROW_NEON32(LOAD_A, LOAD_B, PADD_U8, PADD_U16)         \
  LOAD_A                                                           \
  LOAD_B                                                           \
  PADD_U8 "    q8, q0                         \n" PADD_U8          \
          "    q9, q1                         \n"                  \
          "vmull.u8    q2, d0, d0                     \n"          \
          "vmull.u8    q3, d1, d1                     \n" PADD_U16 \
          "   q10, q2                        \n" PADD_U16          \
          "   q11, q3                        \n"                   \
          "vmull.u8    q2, d2, d2                     \n"          \
          "vmull.u8    q3, d3, d3                     \n"          \
          "vpadal.u16  q10, q2                        \n"          \
          "vpadal.u16  q11, q3                        \n"          \
          "vmull.u8    q2, d0, d2                     \n"          \
          "vmull.u8    q3, d1, d3                     \n" PADD_U16 \
          "   q12, q2                        \n" PADD_U16          \
          "   q13, q3                        \n"

void Ssim4x4Row_NEON(const uint8_t* src_a,
                     int stride_a,
                     const uint8_t* src_b,
                     int stride_b,
                     uint32_t* dst,
                     int width) {
  asm volatile(
      "1:                                        \n"  //
      SSIM_ROW_NEON32("vld1.8 {d0, d1}, [%[src_a]], %[stride_a]\n",
                      "vld1.8 {d2, d3}, [%[src_b]], %[stride_b]\n", "vpaddl.u8",
                      "vpaddl.u16")  //
      SSIM_ROW_NEON32("vld1.8 {d0, d1}, [%[src_a]], %[stride_a]\n",
                      "vld1.8 {d2, d3}, [%[src_b]], %[stride_b]\n", "vpadal.u8",
                      "vpadal.u16")  //
      SSIM_ROW_NEON32("vld1.8 {d0, d1}, [%[src_a]], %[stride_a]\n",
                      "vld1.8 {d2, d3}, [%[src_b]], %[stride_b]\n", "vpadal.u8",
                      "vpadal.u16")  //
      SSIM_ROW_NEON32("vld1.8 {d0, d1}, [%[src_a]]\n",
                      "vld1.8 {d2, d3}, [%[src_b]]\n", "vpadal.u8",
                      "vpadal.u16")  //
      "sub         %[src_a], %[src_a], %[stride_a], lsl #1 \n"
      "sub         %[src_a], %[src_a], %[stride_a]         \n"
      "add         %[src_a], %[src_a], #16                 \n"
      "sub         %[src_b], %[src_b], %[stride_b], lsl #1 \n"
      "sub         %[src_b], %[src_b], %[stride_b]         \n"
      "add         %[src_b], %[src_b], #16                 \n"
      "vpaddl.u16  q8, q8                        \n"
      "vpaddl.u16  q9, q9                        \n"
      "vpadd.u32   d20, d20, d21                 \n"
      "vpadd.u32   d21, d22, d23                 \n"
      "vpadd.u32   d22, d24, d25                 \n"
      "vpadd.u32   d23, d26, d27                 \n"
      "subs        %[width], %[width], #16       \n"
      "vst4.32     {d16, d18, d20, d22}, [%[dst]]! \n"
      "vst4.32     {d17, d19, d21, d23}, [%[dst]]! \n"
      "bgt         1b                            \n"
      : [src_a] "+r"(src_a), [src_b] "+r"(src_b), [dst] "+r"(dst),
        [width] "+r"(width)
      : [stride_a] "r"(stride_a), [stride_b] "r"(stride_b)
      : "memory", "cc", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12",
        "q13");
}
#undef SSIM_ROW_NEON32

#endif  // defined(__ARM_NEON__) && !defined(__aarch64__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

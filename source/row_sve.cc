/*
 *  Copyright 2024 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(LIBYUV_DISABLE_SVE) && defined(__aarch64__)

#define READYUV444_SVE                           \
  "ld1b       {z0.h}, p1/z, [%[src_y]]       \n" \
  "ld1b       {z1.h}, p1/z, [%[src_u]]       \n" \
  "ld1b       {z2.h}, p1/z, [%[src_v]]       \n" \
  "add        %[src_y], %[src_y], %[vl]      \n" \
  "add        %[src_u], %[src_u], %[vl]      \n" \
  "add        %[src_v], %[src_v], %[vl]      \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "prfm       pldl1keep, [%[src_u], 448]     \n" \
  "trn1       z0.b, z0.b, z0.b               \n" \
  "prfm       pldl1keep, [%[src_v], 448]     \n"

#define YUVTORGB_SVE_SETUP                          \
  "ld1rb  {z28.h}, p0/z, [%[kUVCoeff], #0]      \n" \
  "ld1rb  {z29.h}, p0/z, [%[kUVCoeff], #1]      \n" \
  "ld1rb  {z30.h}, p0/z, [%[kUVCoeff], #2]      \n" \
  "ld1rb  {z31.h}, p0/z, [%[kUVCoeff], #3]      \n" \
  "ld1rh  {z24.h}, p0/z, [%[kRGBCoeffBias], #0] \n" \
  "ld1rh  {z25.h}, p0/z, [%[kRGBCoeffBias], #2] \n" \
  "ld1rh  {z26.h}, p0/z, [%[kRGBCoeffBias], #4] \n" \
  "ld1rh  {z27.h}, p0/z, [%[kRGBCoeffBias], #6] \n"

#define I4XXTORGB_SVE                                     \
  "umulh      z0.h, z24.h, z0.h              \n" /* Y */  \
  "mul        z6.h, z30.h, z1.h              \n"          \
  "mul        z4.h, z28.h, z1.h              \n" /* DB */ \
  "mul        z5.h, z29.h, z2.h              \n" /* DR */ \
  "mla        z6.h, p0/m, z31.h, z2.h        \n" /* DG */ \
  "add        z17.h, z0.h, z26.h             \n" /* G */  \
  "add        z16.h, z0.h, z4.h              \n" /* B */  \
  "add        z18.h, z0.h, z5.h              \n" /* R */  \
  "uqsub      z17.h, z17.h, z6.h             \n" /* G */  \
  "uqsub      z16.h, z16.h, z25.h            \n" /* B */  \
  "uqsub      z18.h, z18.h, z27.h            \n" /* R */

// Convert from 2.14 fixed point RGB to 8 bit RGBA, interleaving as BG and RA
// pairs to allow us to use ST2 for storing rather than ST4.
#define RGBTORGBA8_SVE                  \
  "uqshrnb     z16.b, z16.h, #6     \n" \
  "uqshrnb     z18.b, z18.h, #6     \n" \
  "uqshrnt     z16.b, z17.h, #6     \n" \
  "trn1        z17.b, z18.b, z19.b  \n"

#define YUVTORGB_SVE_REGS                                                     \
  "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z16", "z17", "z18", "z19", \
      "z24", "z25", "z26", "z27", "z28", "z29", "z30", "z31", "p0", "p1"

void I444ToARGBRow_SVE2(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n" YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n" /* A */
      "cmp      %w[width], %w[vl]                       \n"
      "b.le     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.h                                    \n"
      "1:                                               \n" READYUV444_SVE
          I4XXTORGB_SVE RGBTORGBA8_SVE
      "sub      %w[width], %w[width], %w[vl]            \n"
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "add      %[dst_argb], %[dst_argb], %[vl], lsl #2 \n"
      "cmp      %w[width], %w[vl]                       \n"
      "b.gt     1b                                      \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "2:                                               \n"
      "whilelt  p1.h, wzr, %w[width]                    \n" READYUV444_SVE
          I4XXTORGB_SVE RGBTORGBA8_SVE
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "cc", "memory", YUVTORGB_SVE_REGS);
}

#endif  // !defined(LIBYUV_DISABLE_SVE) && defined(__aarch64__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

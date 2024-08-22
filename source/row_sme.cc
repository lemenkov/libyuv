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

#if !defined(LIBYUV_DISABLE_SME) && defined(CLANG_HAS_SME) && \
    defined(__aarch64__)

#define YUVTORGB_SVE_SETUP                          \
  "ld1rb  {z28.b}, p0/z, [%[kUVCoeff], #0]      \n" \
  "ld1rb  {z29.b}, p0/z, [%[kUVCoeff], #1]      \n" \
  "ld1rb  {z30.b}, p0/z, [%[kUVCoeff], #2]      \n" \
  "ld1rb  {z31.b}, p0/z, [%[kUVCoeff], #3]      \n" \
  "ld1rh  {z24.h}, p0/z, [%[kRGBCoeffBias], #0] \n" \
  "ld1rh  {z25.h}, p0/z, [%[kRGBCoeffBias], #2] \n" \
  "ld1rh  {z26.h}, p0/z, [%[kRGBCoeffBias], #4] \n" \
  "ld1rh  {z27.h}, p0/z, [%[kRGBCoeffBias], #6] \n"

// Read twice as much data from YUV, putting the even elements from the Y data
// in z0.h and odd elements in z1.h. U/V data is not duplicated, stored in
// z2.h/z3.h.
#define READYUV422_SVE_2X                        \
  "ld1b       {z0.b}, p1/z, [%[src_y]]       \n" \
  "ld1b       {z2.h}, p1/z, [%[src_u]]       \n" \
  "ld1b       {z3.h}, p1/z, [%[src_v]]       \n" \
  "incb       %[src_y]                       \n" \
  "inch       %[src_u]                       \n" \
  "inch       %[src_v]                       \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "prfm       pldl1keep, [%[src_u], 128]     \n" \
  "prfm       pldl1keep, [%[src_v], 128]     \n" \
  "trn2       z1.b, z0.b, z0.b               \n" \
  "trn1       z0.b, z0.b, z0.b               \n"

// Read twice as much data from YUV, putting the even elements from the Y data
// in z0.h and odd elements in z1.h.
#define READYUV444_SVE_2X                        \
  "ld1b       {z0.b}, p1/z, [%[src_y]]       \n" \
  "ld1b       {z2.b}, p1/z, [%[src_u]]       \n" \
  "ld1b       {z3.b}, p1/z, [%[src_v]]       \n" \
  "incb       %[src_y]                       \n" \
  "incb       %[src_u]                       \n" \
  "incb       %[src_v]                       \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "prfm       pldl1keep, [%[src_u], 128]     \n" \
  "prfm       pldl1keep, [%[src_v], 128]     \n" \
  "trn2       z1.b, z0.b, z0.b               \n" \
  "trn1       z0.b, z0.b, z0.b               \n"

// The U/V component multiplies do not need to be duplicated in I422, we just
// need to combine them with Y0/Y1 correctly.
#define I422TORGB_SVE_2X                                  \
  "umulh      z0.h, z24.h, z0.h              \n" /* Y0 */ \
  "umulh      z1.h, z24.h, z1.h              \n" /* Y1 */ \
  "umullb     z6.h, z30.b, z2.b              \n"          \
  "umullb     z4.h, z28.b, z2.b              \n" /* DB */ \
  "umullb     z5.h, z29.b, z3.b              \n" /* DR */ \
  "umlalb     z6.h, z31.b, z3.b              \n" /* DG */ \
                                                          \
  "add        z17.h, z0.h, z26.h             \n" /* G0 */ \
  "add        z21.h, z1.h, z26.h             \n" /* G1 */ \
  "add        z16.h, z0.h, z4.h              \n" /* B0 */ \
  "add        z20.h, z1.h, z4.h              \n" /* B1 */ \
  "add        z18.h, z0.h, z5.h              \n" /* R0 */ \
  "add        z22.h, z1.h, z5.h              \n" /* R1 */ \
  "uqsub      z17.h, z17.h, z6.h             \n" /* G0 */ \
  "uqsub      z21.h, z21.h, z6.h             \n" /* G1 */ \
  "uqsub      z16.h, z16.h, z25.h            \n" /* B0 */ \
  "uqsub      z20.h, z20.h, z25.h            \n" /* B1 */ \
  "uqsub      z18.h, z18.h, z27.h            \n" /* R0 */ \
  "uqsub      z22.h, z22.h, z27.h            \n" /* R1 */

#define I444TORGB_SVE_2X                                  \
  "umulh      z0.h, z24.h, z0.h              \n" /* Y0 */ \
  "umulh      z1.h, z24.h, z1.h              \n" /* Y1 */ \
  "umullb     z6.h, z30.b, z2.b              \n"          \
  "umullt     z7.h, z30.b, z2.b              \n"          \
  "umullb     z4.h, z28.b, z2.b              \n" /* DB */ \
  "umullt     z2.h, z28.b, z2.b              \n" /* DB */ \
  "umlalb     z6.h, z31.b, z3.b              \n" /* DG */ \
  "umlalt     z7.h, z31.b, z3.b              \n" /* DG */ \
  "umullb     z5.h, z29.b, z3.b              \n" /* DR */ \
  "umullt     z3.h, z29.b, z3.b              \n" /* DR */ \
  "add        z17.h, z0.h, z26.h             \n" /* G */  \
  "add        z21.h, z1.h, z26.h             \n" /* G */  \
  "add        z16.h, z0.h, z4.h              \n" /* B */  \
  "add        z20.h, z1.h, z2.h              \n" /* B */  \
  "add        z18.h, z0.h, z5.h              \n" /* R */  \
  "add        z22.h, z1.h, z3.h              \n" /* R */  \
  "uqsub      z17.h, z17.h, z6.h             \n" /* G */  \
  "uqsub      z21.h, z21.h, z7.h             \n" /* G */  \
  "uqsub      z16.h, z16.h, z25.h            \n" /* B */  \
  "uqsub      z20.h, z20.h, z25.h            \n" /* B */  \
  "uqsub      z18.h, z18.h, z27.h            \n" /* R */  \
  "uqsub      z22.h, z22.h, z27.h            \n" /* R */

#define RGBTOARGB8_SVE_2X                                 \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h,  A: z19.b */ \
  "uqshrnb     z16.b, z16.h, #6     \n" /* B0 */          \
  "uqshrnb     z17.b, z17.h, #6     \n" /* G0 */          \
  "uqshrnb     z18.b, z18.h, #6     \n" /* R0 */          \
  "uqshrnt     z16.b, z20.h, #6     \n" /* B1 */          \
  "uqshrnt     z17.b, z21.h, #6     \n" /* G1 */          \
  "uqshrnt     z18.b, z22.h, #6     \n" /* R1 */

#define YUVTORGB_SVE_REGS                                                     \
  "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z16", "z17", "z18", "z19", \
      "z20", "z22", "z23", "z24", "z25", "z26", "z27", "z28", "z29", "z30",   \
      "z31", "p0", "p1", "p2", "p3"

__arm_locally_streaming void I444ToARGBRow_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  // Streaming-SVE only, no use of ZA tile.
  uint64_t vl;
  asm volatile(
      "cntb     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // A
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "1:                                               \n"  //
      READYUV444_SVE_2X I444TORGB_SVE_2X RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st4b     {z16.b, z17.b, z18.b, z19.b}, p1, [%[dst_argb]] \n"
      "incb     %[dst_argb], all, mul #4                \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p1.b, wzr, %w[width]                    \n"  //
      READYUV444_SVE_2X I444TORGB_SVE_2X RGBTOARGB8_SVE_2X
      "st4b     {z16.b, z17.b, z18.b, z19.b}, p1, [%[dst_argb]] \n"

      "99:                                              \n"
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

__arm_locally_streaming void I422ToARGBRow_SME(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) {
  // Streaming-SVE only, no use of ZA tile.
  uint64_t vl;
  asm volatile(
      "cntb     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // A0
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "1:                                               \n"  //
      READYUV422_SVE_2X I422TORGB_SVE_2X RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st4b     {z16.b, z17.b, z18.b, z19.b}, p1, [%[dst_argb]] \n"
      "incb     %[dst_argb], all, mul #4                \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p1.b, wzr, %w[width]                    \n"  //
      READYUV422_SVE_2X I422TORGB_SVE_2X RGBTOARGB8_SVE_2X
      "st4b     {z16.b, z17.b, z18.b, z19.b}, p1, [%[dst_argb]] \n"

      "99:                                              \n"
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

#endif  // !defined(LIBYUV_DISABLE_SME) && defined(CLANG_HAS_SME) &&
        // defined(__aarch64__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

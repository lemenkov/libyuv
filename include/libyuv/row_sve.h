/*
 *  Copyright 2024 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef INCLUDE_LIBYUV_ROW_SVE_H_
#define INCLUDE_LIBYUV_ROW_SVE_H_

#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if (!defined(LIBYUV_DISABLE_SVE) ||                        \
     (!defined(LIBYUV_DISABLE_SME) &&                       \
      (defined(CLANG_HAS_SME) || defined(GCC_HAS_SME)))) && \
    defined(__aarch64__)

#if !defined(LIBYUV_DISABLE_SME) && \
    (defined(CLANG_HAS_SME) || defined(GCC_HAS_SME)) && defined(__aarch64__)
#define STREAMING_COMPATIBLE __arm_streaming_compatible
#else
#define STREAMING_COMPATIBLE
#endif

#define YUVTORGB_SVE_SETUP                          \
  "ld1rb  {z28.b}, p0/z, [%[kUVCoeff], #0]      \n" \
  "ld1rb  {z29.b}, p0/z, [%[kUVCoeff], #1]      \n" \
  "ld1rb  {z30.b}, p0/z, [%[kUVCoeff], #2]      \n" \
  "ld1rb  {z31.b}, p0/z, [%[kUVCoeff], #3]      \n" \
  "ld1rh  {z24.h}, p0/z, [%[kRGBCoeffBias], #0] \n" \
  "ld1rh  {z25.h}, p0/z, [%[kRGBCoeffBias], #2] \n" \
  "ld1rh  {z26.h}, p0/z, [%[kRGBCoeffBias], #4] \n" \
  "ld1rh  {z27.h}, p0/z, [%[kRGBCoeffBias], #6] \n"

#define YUVTORGB_SVE_SETUP_AR30                     \
  YUVTORGB_SVE_SETUP                                \
  "add        z25.h, z25.h, #24                 \n" \
  "sub        z26.h, z26.h, #24                 \n" \
  "add        z27.h, z27.h, #24                 \n"

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

// Read twice as much data from YUV, putting the first half of the Y data in
// z0.h and the second half in z1.h.
#define READYUV444_SVE_2X                                    \
  "ld1b       {z0.h}, p1/z, [%[src_y]]                   \n" \
  "ld1b       {z1.h}, p2/z, [%[src_y], #1, mul vl]       \n" \
  "ld1b       {z2.h}, p1/z, [%[src_u]]                   \n" \
  "ld1b       {z3.h}, p2/z, [%[src_u], #1, mul vl]       \n" \
  "ld1b       {z4.h}, p1/z, [%[src_v]]                   \n" \
  "ld1b       {z5.h}, p2/z, [%[src_v], #1, mul vl]       \n" \
  "incb       %[src_y]                                   \n" \
  "incb       %[src_u]                                   \n" \
  "incb       %[src_v]                                   \n" \
  "prfm       pldl1keep, [%[src_y], 448]                 \n" \
  "prfm       pldl1keep, [%[src_u], 128]                 \n" \
  "prfm       pldl1keep, [%[src_v], 128]                 \n" \
  "trn1       z0.b, z0.b, z0.b                           \n" \
  "trn1       z1.b, z1.b, z1.b                           \n"

// Read twice as much data from YUV, putting the even elements from the Y data
// in z0.h and odd elements in z1.h.
#define READYUV444_RGB24_SVE_2X                  \
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

#define READYUV400_SVE                           \
  "ld1b       {z0.h}, p1/z, [%[src_y]]       \n" \
  "inch       %[src_y]                       \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "trn1       z0.b, z0.b, z0.b               \n"

#define READYUV400_SVE_2X                                    \
  "ld1b       {z0.h}, p1/z, [%[src_y]]                   \n" \
  "ld1b       {z1.h}, p2/z, [%[src_y], #1, mul vl]       \n" \
  "incb       %[src_y]                                   \n" \
  "prfm       pldl1keep, [%[src_y], 448]                 \n" \
  "trn1       z0.b, z0.b, z0.b                           \n" \
  "trn1       z1.b, z1.b, z1.b                           \n"

#define READYUV422_SVE                           \
  "ld1b       {z0.h}, p1/z, [%[src_y]]       \n" \
  "ld1b       {z1.s}, p1/z, [%[src_u]]       \n" \
  "ld1b       {z2.s}, p1/z, [%[src_v]]       \n" \
  "inch       %[src_y]                       \n" \
  "incw       %[src_u]                       \n" \
  "incw       %[src_v]                       \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "prfm       pldl1keep, [%[src_u], 128]     \n" \
  "prfm       pldl1keep, [%[src_v], 128]     \n" \
  "trn1       z0.b, z0.b, z0.b               \n" \
  "trn1       z1.h, z1.h, z1.h               \n" \
  "trn1       z2.h, z2.h, z2.h               \n"

// Read twice as much data from YUV, putting the first half of the Y data in
// z0.h and the second half in z1.h. U/V data is stored in z2.h/z3.h and
// z4.h/z5.h.
#define READYUV422_SVE_2X                                    \
  "ld1b       {z0.h}, p1/z, [%[src_y]]                   \n" \
  "ld1b       {z1.h}, p2/z, [%[src_y], #1, mul vl]       \n" \
  "ld1b       {z2.s}, p1/z, [%[src_u]]                   \n" \
  "ld1b       {z3.s}, p2/z, [%[src_u], #1, mul vl]       \n" \
  "ld1b       {z4.s}, p1/z, [%[src_v]]                   \n" \
  "ld1b       {z5.s}, p2/z, [%[src_v], #1, mul vl]       \n" \
  "incb       %[src_y]                                   \n" \
  "inch       %[src_u]                                   \n" \
  "inch       %[src_v]                                   \n" \
  "prfm       pldl1keep, [%[src_y], 448]                 \n" \
  "prfm       pldl1keep, [%[src_u], 128]                 \n" \
  "prfm       pldl1keep, [%[src_v], 128]                 \n" \
  "trn1       z0.b, z0.b, z0.b                           \n" \
  "trn1       z1.b, z1.b, z1.b                           \n" \
  "trn1       z2.h, z2.h, z2.h                           \n" \
  "trn1       z3.h, z3.h, z3.h                           \n" \
  "trn1       z4.h, z4.h, z4.h                           \n" \
  "trn1       z5.h, z5.h, z5.h                           \n"

// Read twice as much data from YUV, putting the even elements from the Y data
// in z0.h and odd elements in z1.h. U/V data is not duplicated, stored in
// z2.h/z3.h.
#define READYUV422_RGB24_SVE_2X                  \
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

#define READI210_SVE                             \
  "ld1h       {z3.h}, p1/z, [%[src_y]]       \n" \
  "ld1h       {z1.s}, p1/z, [%[src_u]]       \n" \
  "ld1h       {z2.s}, p1/z, [%[src_v]]       \n" \
  "incb       %[src_y]                       \n" \
  "inch       %[src_u]                       \n" \
  "inch       %[src_v]                       \n" \
  "lsl        z0.h, z3.h, #6                 \n" \
  "trn1       z1.h, z1.h, z1.h               \n" \
  "trn1       z2.h, z2.h, z2.h               \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "prfm       pldl1keep, [%[src_u], 128]     \n" \
  "prfm       pldl1keep, [%[src_v], 128]     \n" \
  "usra       z0.h, z3.h, #4                 \n" \
  "uqshrnb    z1.b, z1.h, #2                 \n" \
  "uqshrnb    z2.b, z2.h, #2                 \n"

#define READI210_SVE_2X                                      \
  "ld1h       {z6.h}, p1/z, [%[src_y]]                   \n" \
  "ld1h       {z7.h}, p2/z, [%[src_y], #1, mul vl]       \n" \
  "ld1h       {z2.s}, p1/z, [%[src_u]]                   \n" \
  "ld1h       {z3.s}, p2/z, [%[src_u], #1, mul vl]       \n" \
  "ld1h       {z4.s}, p1/z, [%[src_v]]                   \n" \
  "ld1h       {z5.s}, p2/z, [%[src_v], #1, mul vl]       \n" \
  "incb       %[src_y], all, mul #2                      \n" \
  "incb       %[src_u]                                   \n" \
  "incb       %[src_v]                                   \n" \
  "lsl        z0.h, z6.h, #6                             \n" \
  "lsl        z1.h, z7.h, #6                             \n" \
  "trn1       z2.h, z2.h, z2.h                           \n" \
  "trn1       z3.h, z3.h, z3.h                           \n" \
  "trn1       z4.h, z4.h, z4.h                           \n" \
  "trn1       z5.h, z5.h, z5.h                           \n" \
  "prfm       pldl1keep, [%[src_y], 448]                 \n" \
  "prfm       pldl1keep, [%[src_u], 128]                 \n" \
  "prfm       pldl1keep, [%[src_v], 128]                 \n" \
  "usra       z0.h, z6.h, #4                             \n" \
  "usra       z1.h, z7.h, #4                             \n" \
  "uqshrnb    z2.b, z2.h, #2                             \n" \
  "uqshrnb    z3.b, z3.h, #2                             \n" \
  "uqshrnb    z4.b, z4.h, #2                             \n" \
  "uqshrnb    z5.b, z5.h, #2                             \n"

#define READP210_SVE                             \
  "ld1h       {z0.h}, p1/z, [%[src_y]]       \n" \
  "ld1h       {z1.h}, p2/z, [%[src_uv]]      \n" \
  "incb       %[src_y]                       \n" \
  "incb       %[src_uv]                      \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "prfm       pldl1keep, [%[src_uv], 256]    \n" \
  "tbl        z1.b, {z1.b}, z22.b            \n"

#define READP210_SVE_2X                                      \
  "ld1h       {z0.h}, p1/z, [%[src_y]]                   \n" \
  "ld1h       {z1.h}, p2/z, [%[src_y], #1, mul vl]       \n" \
  "ld1w       {z2.s}, p1/z, [%[src_uv]]                  \n" \
  "ld1w       {z3.s}, p2/z, [%[src_uv], #1, mul vl]      \n" \
  "incb       %[src_y], all, mul #2                      \n" \
  "incb       %[src_uv], all, mul #2                     \n" \
  "prfm       pldl1keep, [%[src_y], 448]                 \n" \
  "prfm       pldl1keep, [%[src_uv], 256]                \n" \
  "trn2       z4.h, z2.h, z2.h                           \n" \
  "trn1       z2.h, z2.h, z2.h                           \n" \
  "trn2       z5.h, z3.h, z3.h                           \n" \
  "trn1       z3.h, z3.h, z3.h                           \n"

#define READI410_SVE                             \
  "ld1h       {z3.h}, p1/z, [%[src_y]]       \n" \
  "lsl        z0.h, z3.h, #6                 \n" \
  "usra       z0.h, z3.h, #4                 \n" \
  "ld1h       {z1.h}, p1/z, [%[src_u]]       \n" \
  "ld1h       {z2.h}, p1/z, [%[src_v]]       \n" \
  "incb       %[src_y]                       \n" \
  "incb       %[src_u]                       \n" \
  "incb       %[src_v]                       \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "prfm       pldl1keep, [%[src_u], 128]     \n" \
  "prfm       pldl1keep, [%[src_v], 128]     \n" \
  "uqshrnb    z1.b, z1.h, #2                 \n" \
  "uqshrnb    z2.b, z2.h, #2                 \n"

#define READI410_SVE_2X                                      \
  "ld1h       {z4.h}, p1/z, [%[src_y]]                   \n" \
  "ld1h       {z5.h}, p2/z, [%[src_y], #1, mul vl]       \n" \
  "incb       %[src_y], all, mul #2                      \n" \
  "lsl        z0.h, z4.h, #6                             \n" \
  "lsl        z1.h, z5.h, #6                             \n" \
  "usra       z0.h, z4.h, #4                             \n" \
  "usra       z1.h, z5.h, #4                             \n" \
  "ld1h       {z2.h}, p1/z, [%[src_u]]                   \n" \
  "ld1h       {z3.h}, p2/z, [%[src_u], #1, mul vl]       \n" \
  "incb       %[src_u], all, mul #2                      \n" \
  "uqshrnb    z2.b, z2.h, #2                             \n" \
  "uqshrnb    z3.b, z3.h, #2                             \n" \
  "ld1h       {z4.h}, p1/z, [%[src_v]]                   \n" \
  "ld1h       {z5.h}, p2/z, [%[src_v], #1, mul vl]       \n" \
  "incb       %[src_v], all, mul #2                      \n" \
  "prfm       pldl1keep, [%[src_y], 448]                 \n" \
  "prfm       pldl1keep, [%[src_u], 128]                 \n" \
  "prfm       pldl1keep, [%[src_v], 128]                 \n" \
  "uqshrnb    z4.b, z4.h, #2                             \n" \
  "uqshrnb    z5.b, z5.h, #2                             \n"

#define READA10_SVE_2X                                       \
  "ld1h       {z4.h}, p1/z, [%[src_a]]                   \n" \
  "ld1h       {z5.h}, p2/z, [%[src_a], #1, mul vl]       \n" \
  "incb       %[src_a], all, mul #2                      \n" \
  "uqshrnb    z19.b, z4.h, #2                            \n" \
  "uqshrnb    z23.b, z5.h, #2                            \n"

// We need different predicates for the UV components since we are reading
// 32-bit (pairs of UV) elements rather than 16-bit Y elements.
#define READP410_SVE                                    \
  "ld1h       {z0.h}, p1/z, [%[src_y]]              \n" \
  "ld1w       {z1.s}, p2/z, [%[src_uv]]             \n" \
  "ld1w       {z2.s}, p3/z, [%[src_uv], #1, mul vl] \n" \
  "incb       %[src_y]                              \n" \
  "incb       %[src_uv], all, mul #2                \n" \
  "prfm       pldl1keep, [%[src_y], 448]            \n" \
  "prfm       pldl1keep, [%[src_uv], 256]           \n" \
  "uzp2       z1.b, z1.b, z2.b                      \n"

#define READI212_SVE                             \
  "ld1h       {z3.h}, p1/z, [%[src_y]]       \n" \
  "ld1h       {z1.s}, p1/z, [%[src_u]]       \n" \
  "ld1h       {z2.s}, p1/z, [%[src_v]]       \n" \
  "incb       %[src_y]                       \n" \
  "inch       %[src_u]                       \n" \
  "inch       %[src_v]                       \n" \
  "lsl        z0.h, z3.h, #4                 \n" \
  "trn1       z1.h, z1.h, z1.h               \n" \
  "trn1       z2.h, z2.h, z2.h               \n" \
  "prfm       pldl1keep, [%[src_y], 448]     \n" \
  "prfm       pldl1keep, [%[src_u], 128]     \n" \
  "prfm       pldl1keep, [%[src_v], 128]     \n" \
  "usra       z0.h, z3.h, #8                 \n" \
  "uqshrnb    z1.b, z1.h, #4                 \n" \
  "uqshrnb    z2.b, z2.h, #4                 \n"

#define READI212_SVE_2X                                      \
  "ld1h       {z6.h}, p1/z, [%[src_y]]                   \n" \
  "ld1h       {z7.h}, p2/z, [%[src_y], #1, mul vl]       \n" \
  "ld1h       {z2.s}, p1/z, [%[src_u]]                   \n" \
  "ld1h       {z3.s}, p2/z, [%[src_u], #1, mul vl]       \n" \
  "ld1h       {z4.s}, p1/z, [%[src_v]]                   \n" \
  "ld1h       {z5.s}, p2/z, [%[src_v], #1, mul vl]       \n" \
  "incb       %[src_y], all, mul #2                      \n" \
  "incb       %[src_u]                                   \n" \
  "incb       %[src_v]                                   \n" \
  "lsl        z0.h, z6.h, #4                             \n" \
  "lsl        z1.h, z7.h, #4                             \n" \
  "trn1       z2.h, z2.h, z2.h                           \n" \
  "trn1       z3.h, z3.h, z3.h                           \n" \
  "trn1       z4.h, z4.h, z4.h                           \n" \
  "trn1       z5.h, z5.h, z5.h                           \n" \
  "prfm       pldl1keep, [%[src_y], 448]                 \n" \
  "prfm       pldl1keep, [%[src_u], 128]                 \n" \
  "prfm       pldl1keep, [%[src_v], 128]                 \n" \
  "usra       z0.h, z6.h, #8                             \n" \
  "usra       z1.h, z7.h, #8                             \n" \
  "uqshrnb    z2.b, z2.h, #4                             \n" \
  "uqshrnb    z3.b, z3.h, #4                             \n" \
  "uqshrnb    z4.b, z4.h, #4                             \n" \
  "uqshrnb    z5.b, z5.h, #4                             \n"

#define I400TORGB_SVE                                    \
  "umulh      z18.h, z24.h, z0.h             \n" /* Y */ \
  "movprfx    z16, z18                       \n"         \
  "usqadd     z16.h, p0/m, z16.h, z4.h       \n" /* B */ \
  "movprfx    z17, z18                       \n"         \
  "usqadd     z17.h, p0/m, z17.h, z6.h       \n" /* G */ \
  "usqadd     z18.h, p0/m, z18.h, z5.h       \n" /* R */

#define I400TORGB_SVE_2X                                  \
  "umulh      z18.h, z24.h, z0.h             \n" /* Y0 */ \
  "umulh      z22.h, z24.h, z1.h             \n" /* Y1 */ \
  "movprfx    z16, z18                       \n"          \
  "usqadd     z16.h, p0/m, z16.h, z4.h       \n" /* B0 */ \
  "movprfx    z20, z22                       \n"          \
  "usqadd     z20.h, p0/m, z20.h, z4.h       \n" /* B1 */ \
  "movprfx    z17, z18                       \n"          \
  "usqadd     z17.h, p0/m, z17.h, z6.h       \n" /* G0 */ \
  "movprfx    z21, z22                       \n"          \
  "usqadd     z21.h, p0/m, z21.h, z6.h       \n" /* G1 */ \
  "usqadd     z18.h, p0/m, z18.h, z5.h       \n" /* R0 */ \
  "usqadd     z22.h, p0/m, z22.h, z5.h       \n" /* R1 */

#define READNV_SVE_2X                                            \
  "ld1b       {z0.h}, p1/z, [%[src_y]]           \n"   /* Y0 */  \
  "ld1b       {z1.h}, p2/z, [%[src_y], #1, mul vl]\n"  /* Y1 */  \
  "ld1h       {z2.s}, p1/z, [%[src_uv]]          \n"   /* UV0 */ \
  "ld1h       {z3.s}, p2/z, [%[src_uv], #1, mul vl]\n" /* UV1 */ \
  "incb       %[src_y]                           \n"             \
  "incb       %[src_uv]                          \n"             \
  "prfm       pldl1keep, [%[src_y], 448]         \n"             \
  "prfm       pldl1keep, [%[src_uv], 256]        \n"             \
  "trn1       z0.b, z0.b, z0.b                   \n" /* Y0Y0 */  \
  "trn1       z1.b, z1.b, z1.b                   \n" /* Y1Y1 */  \
  "trn1       z2.h, z2.h, z2.h                   \n" /* UV0 */   \
  "trn1       z3.h, z3.h, z3.h                   \n" /* UV1 */

// We need a different predicate for the UV component to handle the tail.
// If there is a single element remaining then we want to load one Y element
// but two UV elements.
#define READNV_RGB24_SVE_2X                                         \
  "ld1b       {z0.b}, p1/z, [%[src_y]]       \n" /* Y0Y0 */         \
  "ld1b       {z2.b}, p2/z, [%[src_uv]]      \n" /* U0V0 or V0U0 */ \
  "incb       %[src_y]                       \n"                    \
  "incb       %[src_uv]                      \n"                    \
  "prfm       pldl1keep, [%[src_y], 448]     \n"                    \
  "prfm       pldl1keep, [%[src_uv], 256]    \n"                    \
  "trn2       z1.b, z0.b, z0.b               \n" /* YYYY */         \
  "trn1       z0.b, z0.b, z0.b               \n" /* YYYY */

// Like NVTORGB_SVE but U/V components are stored in widened .h elements of
// z1/z2 rather than even/odd .b lanes of z1.
#define I4XXTORGB_SVE                                     \
  "umulh      z0.h, z24.h, z0.h              \n" /* Y */  \
  "umullb     z6.h, z30.b, z1.b              \n"          \
  "umullb     z4.h, z28.b, z1.b              \n" /* DB */ \
  "umullb     z5.h, z29.b, z2.b              \n" /* DR */ \
  "umlalb     z6.h, z31.b, z2.b              \n" /* DG */ \
  "add        z17.h, z0.h, z26.h             \n" /* G */  \
  "add        z16.h, z0.h, z4.h              \n" /* B */  \
  "add        z18.h, z0.h, z5.h              \n" /* R */  \
  "uqsub      z17.h, z17.h, z6.h             \n" /* G */  \
  "uqsub      z16.h, z16.h, z25.h            \n" /* B */  \
  "uqsub      z18.h, z18.h, z27.h            \n" /* R */

// clang-format off
#define I4XXTORGB_SVE_2X(bt)                              \
  "umulh      z0.h, z24.h, z0.h              \n" /* Y0 */ \
  "umulh      z1.h, z24.h, z1.h              \n" /* Y1 */ \
  "umull" #bt " z6.h, z30.b, z2.b            \n"          \
  "umull" #bt " z7.h, z30.b, z3.b            \n"          \
  "umlal" #bt " z6.h, z31.b, z4.b            \n" /* DG0 */\
  "umlal" #bt " z7.h, z31.b, z5.b            \n" /* DG1 */\
  "umull" #bt " z2.h, z28.b, z2.b            \n" /* DB0 */\
  "umull" #bt " z3.h, z28.b, z3.b            \n" /* DB1 */\
  "umull" #bt " z4.h, z29.b, z4.b            \n" /* DR0 */\
  "umull" #bt " z5.h, z29.b, z5.b            \n" /* DR1 */\
  "add        z17.h, z0.h, z26.h             \n" /* G0 */ \
  "add        z21.h, z1.h, z26.h             \n" /* G1 */ \
  "add        z16.h, z0.h, z2.h              \n" /* B0 */ \
  "add        z20.h, z1.h, z3.h              \n" /* B1 */ \
  "add        z18.h, z0.h, z4.h              \n" /* R0 */ \
  "add        z22.h, z1.h, z5.h              \n" /* R1 */ \
  "uqsub      z17.h, z17.h, z6.h             \n" /* G0 */ \
  "uqsub      z21.h, z21.h, z7.h             \n" /* G1 */ \
  "uqsub      z16.h, z16.h, z25.h            \n" /* B0 */ \
  "uqsub      z20.h, z20.h, z25.h            \n" /* B1 */ \
  "uqsub      z18.h, z18.h, z27.h            \n" /* R0 */ \
  "uqsub      z22.h, z22.h, z27.h            \n" /* R1 */
// clang-format on

#define I444TORGB_RGB24_SVE_2X                            \
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

// Like I4XXTORGB_SVE but U/V components are stored in even/odd .b lanes of z1
// rather than widened .h elements of z1/z2.
#define NVTORGB_SVE                                       \
  "umulh      z0.h, z24.h, z0.h              \n" /* Y */  \
  "umullb     z6.h, z30.b, z1.b              \n"          \
  "umullb     z4.h, z28.b, z1.b              \n" /* DB */ \
  "umullt     z5.h, z29.b, z1.b              \n" /* DR */ \
  "umlalt     z6.h, z31.b, z1.b              \n" /* DG */ \
  "add        z17.h, z0.h, z26.h             \n" /* G */  \
  "add        z16.h, z0.h, z4.h              \n" /* B */  \
  "add        z18.h, z0.h, z5.h              \n" /* R */  \
  "uqsub      z17.h, z17.h, z6.h             \n" /* G */  \
  "uqsub      z16.h, z16.h, z25.h            \n" /* B */  \
  "uqsub      z18.h, z18.h, z27.h            \n" /* R */

// The U/V component multiplies do not need to be duplicated in I422, we just
// need to combine them with Y0/Y1 correctly.
#define I422TORGB_RGB24_SVE_2X                            \
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

// clang-format off
#define NVTORGB_SVE_2X(bt_u, bt_v)                         \
  "umulh      z0.h, z24.h, z0.h              \n" /* Y0 */  \
  "umulh      z1.h, z24.h, z1.h              \n" /* Y1 */  \
  "umull" #bt_u " z6.h, z30.b, z2.b          \n"           \
  "umull" #bt_u " z7.h, z30.b, z3.b          \n"           \
  "umull" #bt_u " z4.h, z28.b, z2.b          \n" /* DB0 */ \
  "umull" #bt_u " z5.h, z28.b, z3.b          \n" /* DB1 */ \
  "umlal" #bt_v " z6.h, z31.b, z2.b          \n" /* DG0 */ \
  "umlal" #bt_v " z7.h, z31.b, z3.b          \n" /* DG1 */ \
  "umull" #bt_v " z2.h, z29.b, z2.b          \n" /* DR0 */ \
  "umull" #bt_v " z3.h, z29.b, z3.b          \n" /* DR1 */ \
                                                           \
  "add        z17.h, z0.h, z26.h             \n" /* G0 */  \
  "add        z21.h, z1.h, z26.h             \n" /* G1 */  \
  "add        z16.h, z0.h, z4.h              \n" /* B0 */  \
  "add        z20.h, z1.h, z5.h              \n" /* B1 */  \
  "add        z18.h, z0.h, z2.h              \n" /* R0 */  \
  "add        z22.h, z1.h, z3.h              \n" /* R1 */  \
  "uqsub      z17.h, z17.h, z6.h             \n" /* G0 */  \
  "uqsub      z21.h, z21.h, z7.h             \n" /* G1 */  \
  "uqsub      z16.h, z16.h, z25.h            \n" /* B0 */  \
  "uqsub      z20.h, z20.h, z25.h            \n" /* B1 */  \
  "uqsub      z18.h, z18.h, z27.h            \n" /* R0 */  \
  "uqsub      z22.h, z22.h, z27.h            \n" /* R1 */

#define NVTORGB_RGB24_SVE_2X(bt_u, bt_v)                  \
  "umulh      z0.h, z24.h, z0.h              \n" /* Y0 */ \
  "umulh      z1.h, z24.h, z1.h              \n" /* Y1 */ \
  "umull" #bt_u " z6.h, z30.b, z2.b          \n"          \
  "umull" #bt_u " z4.h, z28.b, z2.b          \n" /* DB */ \
  "umull" #bt_v " z5.h, z29.b, z2.b          \n" /* DR */ \
  "umlal" #bt_v " z6.h, z31.b, z2.b          \n" /* DG */ \
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
// clang-format on

#define RGBTOARGB8_SVE_TOP_2X                        \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h */       \
  "uqshl     z16.h, p0/m, z16.h, #2     \n" /* B0 */ \
  "uqshl     z17.h, p0/m, z17.h, #2     \n" /* G0 */ \
  "uqshl     z18.h, p0/m, z18.h, #2     \n" /* R0 */ \
  "uqshl     z20.h, p0/m, z20.h, #2     \n" /* B1 */ \
  "uqshl     z21.h, p0/m, z21.h, #2     \n" /* G1 */ \
  "uqshl     z22.h, p0/m, z22.h, #2     \n" /* R1 */

// Convert from 2.14 fixed point RGB to 8 bit ARGB, interleaving as BG and RA
// pairs to allow us to use ST2 for storing rather than ST4.
#define RGBTOARGB8_SVE                                    \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h,  A: z19.b */ \
  "uqshrnb     z16.b, z16.h, #6     \n" /* B0 */          \
  "uqshrnb     z18.b, z18.h, #6     \n" /* R0 */          \
  "uqshrnt     z16.b, z17.h, #6     \n" /* BG */          \
  "trn1        z17.b, z18.b, z19.b  \n" /* RA */

// Convert from 2.14 fixed point RGBA to 8 bit ARGB, interleaving as BG and RA
// pairs to allow us to use ST2 for storing rather than ST4.
#define RGBATOARGB8_SVE                                   \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h,  A: z19.h */ \
  "uqshrnb     z16.b, z16.h, #6     \n" /* B0 */          \
  "uqshrnt     z16.b, z17.h, #6     \n" /* BG */          \
  "uqshrnb     z17.b, z18.h, #6     \n" /* R0 */          \
  "uqshrnt     z17.b, z19.h, #2     \n" /* RA */

// Convert from 2.14 fixed point RGB to 8 bit RGBA, interleaving as AB and GR
// pairs to allow us to use ST2 for storing rather than ST4.
#define RGBTORGBA8_SVE                                    \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h,  A: z19.b */ \
  "uqshrnt     z19.b, z16.h, #6     \n" /* AB */          \
  "uqshrnb     z20.b, z17.h, #6     \n" /* G0 */          \
  "uqshrnt     z20.b, z18.h, #6     \n" /* GR */

#define RGBTORGBA8_SVE_2X                                                \
  /* Inputs: B: z16.h/z20.h, G: z17.h/z21.h, R: z18.h/z22.h, A: z23.b */ \
  "mov         z19.d, z23.d         \n" /* A0 */                         \
  "uqshrnt     z19.b, z16.h, #6     \n" /* AB0 */                        \
  "mov         z16.d, z19.d         \n" /* AB0 */                        \
  "uqshrnb     z17.b, z17.h, #6     \n" /* G0 */                         \
  "uqshrnt     z17.b, z18.h, #6     \n" /* GR0 */                        \
  "mov         z19.d, z23.d         \n" /* A1 */                         \
  "uqshrnt     z19.b, z20.h, #6     \n" /* AB1 */                        \
  "mov         z20.d, z19.d         \n" /* AB1 */                        \
  "uqshrnb     z21.b, z21.h, #6     \n" /* G1 */                         \
  "uqshrnt     z21.b, z22.h, #6     \n" /* GR1 */

#define RGBTOARGB8_SVE_2X                                 \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h,  A: z19.b */ \
  /*         B: z20.h,  G: z21.h,  R: z22.h,  A: z19.b */ \
  "uqshrnb     z16.b, z16.h, #6     \n" /* B0 */          \
  "uqshrnb     z18.b, z18.h, #6     \n" /* R0 */          \
  "uqshrnt     z16.b, z17.h, #6     \n" /* BG0 */         \
  "trn1        z17.b, z18.b, z19.b  \n" /* RA0 */         \
  "uqshrnb     z20.b, z20.h, #6     \n" /* B1 */          \
  "uqshrnb     z22.b, z22.h, #6     \n" /* R1 */          \
  "uqshrnt     z20.b, z21.h, #6     \n" /* BG1 */         \
  "trn1        z21.b, z22.b, z19.b  \n" /* RA1 */

#define RGBATOARGB8_SVE_2X                                \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h,  A: z19.b */ \
  /*         B: z20.h,  G: z21.h,  R: z22.h,  A: z23.b */ \
  "uqshrnb     z16.b, z16.h, #6     \n" /* B0 */          \
  "uqshrnb     z18.b, z18.h, #6     \n" /* R0 */          \
  "uqshrnt     z16.b, z17.h, #6     \n" /* BG0 */         \
  "trn1        z17.b, z18.b, z19.b  \n" /* RA0 */         \
  "uqshrnb     z20.b, z20.h, #6     \n" /* B1 */          \
  "uqshrnb     z22.b, z22.h, #6     \n" /* R1 */          \
  "uqshrnt     z20.b, z21.h, #6     \n" /* BG1 */         \
  "trn1        z21.b, z22.b, z23.b  \n" /* RA1 */

#define RGBTORGB8_SVE_2X                         \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h */   \
  /*         B: z20.h,  G: z21.h,  R: z22.h */   \
  "uqshrnb     z16.b, z16.h, #6     \n" /* B0 */ \
  "uqshrnb     z17.b, z17.h, #6     \n" /* G0 */ \
  "uqshrnb     z18.b, z18.h, #6     \n" /* R0 */ \
  "uqshrnt     z16.b, z20.h, #6     \n" /* B1 */ \
  "uqshrnt     z17.b, z21.h, #6     \n" /* G1 */ \
  "uqshrnt     z18.b, z22.h, #6     \n" /* R1 */

// clang-format off
#define STOREARGB8_SVE_2X(dst)                                       \
  "st2h     {z16.h, z17.h}, p1, [%[" #dst "]]                    \n" \
  "st2h     {z20.h, z21.h}, p2, [%[" #dst "], #2, mul vl]        \n" \
  "incb     %[" #dst "], all, mul #4                             \n"
// clang-format on

// Store AR30 elements. Inputs are 2.14 fixed point RGB. We expect z23 to be
// populated with 0x3ff0 (0x3fff would also work) to saturate the R input
// rather than needing a pair of shifts to saturate and then insert into the
// correct position in the lane.
#define STOREAR30_SVE                                                    \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h */                           \
  "uqshl    z16.h, p0/m, z16.h, #2            \n" /* bbbbbbbbbbxxxxxx */ \
  "uqshl    z17.h, p0/m, z17.h, #2            \n" /* ggggggggggxxxxxx */ \
  "umin     z18.h, p0/m, z18.h, z23.h         \n" /* 00rrrrrrrrrrxxxx */ \
  "orr      z18.h, z18.h, #0xc000             \n" /* 11rrrrrrrrrrxxxx */ \
  "sri      z18.h, z17.h, #12                 \n" /* 11rrrrrrrrrrgggg */ \
  "lsl      z17.h, z17.h, #4                  \n" /* ggggggxxxxxx0000 */ \
  "sri      z17.h, z16.h, #6                  \n" /* ggggggbbbbbbbbbb */ \
  "st2h     {z17.h, z18.h}, p1, [%[dst_ar30]] \n"                        \
  "incb     %[dst_ar30], all, mul #2          \n"

#define STOREAR30_SVE_2X                                                 \
  /* Inputs: B: z16.h,  G: z17.h,  R: z18.h */                           \
  /*         B: z20.h,  G: z21.h,  R: z22.h */                           \
  "uqshl    z16.h, p0/m, z16.h, #2            \n" /* bbbbbbbbbbxxxxxx */ \
  "uqshl    z20.h, p0/m, z20.h, #2            \n" /* bbbbbbbbbbxxxxxx */ \
  "uqshl    z17.h, p0/m, z17.h, #2            \n" /* ggggggggggxxxxxx */ \
  "uqshl    z21.h, p0/m, z21.h, #2            \n" /* ggggggggggxxxxxx */ \
  "umin     z18.h, p0/m, z18.h, z23.h         \n" /* 00rrrrrrrrrrxxxx */ \
  "umin     z22.h, p0/m, z22.h, z23.h         \n" /* 00rrrrrrrrrrxxxx */ \
  "orr      z18.h, z18.h, #0xc000             \n" /* 11rrrrrrrrrrxxxx */ \
  "orr      z22.h, z22.h, #0xc000             \n" /* 11rrrrrrrrrrxxxx */ \
  "sri      z18.h, z17.h, #12                 \n" /* 11rrrrrrrrrrgggg */ \
  "sri      z22.h, z21.h, #12                 \n" /* 11rrrrrrrrrrgggg */ \
  "lsl      z17.h, z17.h, #4                  \n" /* ggggggxxxxxx0000 */ \
  "lsl      z21.h, z21.h, #4                  \n" /* ggggggxxxxxx0000 */ \
  "sri      z17.h, z16.h, #6                  \n" /* ggggggbbbbbbbbbb */ \
  "sri      z21.h, z20.h, #6                  \n" /* ggggggbbbbbbbbbb */ \
  "st2h     {z17.h, z18.h}, p1, [%[dst_ar30]] \n"                        \
  "st2h     {z21.h, z22.h}, p2, [%[dst_ar30], #2, mul vl] \n"            \
  "incb     %[dst_ar30], all, mul #4          \n"

#define YUVTORGB_SVE_REGS                                                     \
  "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z16", "z17", "z18", "z19", \
      "z20", "z21", "z22", "z23", "z24", "z25", "z26", "z27", "z28", "z29",   \
      "z30", "z31", "p0", "p1", "p2", "p3"

static inline void I444ToARGBRow_SVE_SC(const uint8_t* src_y,
                                        const uint8_t* src_u,
                                        const uint8_t* src_v,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // A
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV444_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV444_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I444ToRGB24Row_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_rgb24,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cntb     %[vl]                                     \n"
      "ptrue    p0.b                                      \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl]              \n"
      "b.lt     2f                                        \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                      \n"
      "1:                                                 \n"  //
      READYUV444_RGB24_SVE_2X I444TORGB_RGB24_SVE_2X RGBTORGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl]              \n"
      "st3b     {z16.b, z17.b, z18.b}, p1, [%[dst_rgb24]] \n"
      "incb     %[dst_rgb24], all, mul #3                 \n"
      "b.ge     1b                                        \n"

      "2:                                                 \n"
      "adds     %w[width], %w[width], %w[vl]              \n"
      "b.eq     99f                                       \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p1.b, wzr, %w[width]                      \n"  //
      READYUV444_RGB24_SVE_2X I444TORGB_RGB24_SVE_2X RGBTORGB8_SVE_2X
      "st3b     {z16.b, z17.b, z18.b}, p1, [%[dst_rgb24]] \n"

      "99:                                                \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_rgb24] "+r"(dst_rgb24),                       // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I400ToARGBRow_SVE_SC(const uint8_t* src_y,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"
      "dup      z19.b, #255                             \n"  // Alpha
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "mov      z1.h, #128                              \n"  // U/V
      "umullb   z6.h, z30.b, z1.b                       \n"
      "umullb   z4.h, z28.b, z1.b                       \n"  // DB
      "umullb   z5.h, z29.b, z1.b                       \n"  // DR
      "mla      z6.h, p0/m, z31.h, z1.h                 \n"  // DG
      "sub      z4.h, z4.h, z25.h                       \n"
      "sub      z5.h, z5.h, z27.h                       \n"
      "sub      z6.h, z26.h, z6.h                       \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV400_SVE_2X I400TORGB_SVE_2X RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV400_SVE_2X I400TORGB_SVE_2X RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I422ToARGBRow_SVE_SC(const uint8_t* src_y,
                                        const uint8_t* src_u,
                                        const uint8_t* src_v,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I422ToRGB24Row_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cntb     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "1:                                               \n"  //
      READYUV422_RGB24_SVE_2X I422TORGB_RGB24_SVE_2X RGBTORGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st3b     {z16.b, z17.b, z18.b}, p1, [%[dst_argb]] \n"
      "incb     %[dst_argb], all, mul #3                \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p1.b, wzr, %w[width]                    \n"  //
      READYUV422_RGB24_SVE_2X I422TORGB_RGB24_SVE_2X RGBTORGB8_SVE_2X
      "st3b     {z16.b, z17.b, z18.b}, p1, [%[dst_argb]] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

#define RGB8TORGB565_SVE_FROM_TOP_2X                        \
  "sri      z18.h, z17.h, #5     \n" /* rrrrrgggggg00000 */ \
  "sri      z22.h, z21.h, #5     \n" /* rrrrrgggggg00000 */ \
  "sri      z18.h, z16.h, #11    \n" /* rrrrrggggggbbbbb */ \
  "sri      z22.h, z20.h, #11    \n" /* rrrrrggggggbbbbb */

static inline void I422ToRGB565Row_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_rgb565,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_TOP_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      RGB8TORGB565_SVE_FROM_TOP_2X
      "st1h     {z18.h}, p1, [%[dst]]                   \n"
      "st1h     {z22.h}, p2, [%[dst], #1, mul vl]       \n"
      "incb     %[dst], all, mul #2                     \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b)
          RGBTOARGB8_SVE_TOP_2X RGB8TORGB565_SVE_FROM_TOP_2X
      "st1h     {z18.h}, p1, [%[dst]]                   \n"
      "st1h     {z22.h}, p2, [%[dst], #1, mul vl]       \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst] "+r"(dst_rgb565),                            // %[dst]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

#define RGB8TOARGB1555_SVE_FROM_TOP_2X                      \
  "dup      z0.h, #0x8000        \n" /* 1000000000000000 */ \
  "dup      z1.h, #0x8000        \n" /* 1000000000000000 */ \
  "sri      z0.h, z18.h, #1      \n" /* 1rrrrrxxxxxxxxxx */ \
  "sri      z1.h, z22.h, #1      \n" /* 1rrrrrxxxxxxxxxx */ \
  "sri      z0.h, z17.h, #6      \n" /* 1rrrrrgggggxxxxx */ \
  "sri      z1.h, z21.h, #6      \n" /* 1rrrrrgggggxxxxx */ \
  "sri      z0.h, z16.h, #11     \n" /* 1rrrrrgggggbbbbb */ \
  "sri      z1.h, z20.h, #11     \n" /* 1rrrrrgggggbbbbb */

static inline void I422ToARGB1555Row_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb1555,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_TOP_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      RGB8TOARGB1555_SVE_FROM_TOP_2X
      "st1h     {z0.h}, p1, [%[dst]]                    \n"
      "st1h     {z1.h}, p2, [%[dst], #1, mul vl]        \n"
      "incb     %[dst], all, mul #2                     \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b)
          RGBTOARGB8_SVE_TOP_2X RGB8TOARGB1555_SVE_FROM_TOP_2X
      "st1h     {z0.h}, p1, [%[dst]]                    \n"
      "st1h     {z1.h}, p2, [%[dst], #1, mul vl]        \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst] "+r"(dst_argb1555),                          // %[dst]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

#define RGB8TOARGB4444_SVE_FROM_TOP_2X                      \
  "dup      z0.h, #0xf000        \n" /* 1111000000000000 */ \
  "dup      z1.h, #0xf000        \n" /* 1111000000000000 */ \
  "sri      z0.h, z18.h, #4      \n" /* 1111rrrrxxxxxxxx */ \
  "sri      z1.h, z22.h, #4      \n" /* 1111rrrrxxxxxxxx */ \
  "sri      z0.h, z17.h, #8      \n" /* 1111rrrrggggxxxx */ \
  "sri      z1.h, z21.h, #8      \n" /* 1111rrrrggggxxxx */ \
  "sri      z0.h, z16.h, #12     \n" /* 1111rrrrggggbbbb */ \
  "sri      z1.h, z20.h, #12     \n" /* 1111rrrrggggbbbb */

static inline void I422ToARGB4444Row_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    uint8_t* dst_argb4444,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_TOP_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      RGB8TOARGB4444_SVE_FROM_TOP_2X
      "st1h     {z0.h}, p1, [%[dst]]                    \n"
      "st1h     {z1.h}, p2, [%[dst], #1, mul vl]        \n"
      "incb     %[dst], all, mul #2                     \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b)
          RGBTOARGB8_SVE_TOP_2X RGB8TOARGB4444_SVE_FROM_TOP_2X
      "st1h     {z0.h}, p1, [%[dst]]                    \n"
      "st1h     {z1.h}, p2, [%[dst], #1, mul vl]        \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst] "+r"(dst_argb4444),                          // %[dst]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I422ToRGBARow_SVE_SC(const uint8_t* src_y,
                                        const uint8_t* src_u,
                                        const uint8_t* src_v,
                                        uint8_t* dst_rgba,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z23.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) RGBTORGBA8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_rgba)
      "b.ge     1b                                      \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) RGBTORGBA8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_rgba]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_rgba], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_rgba] "+r"(dst_rgba),                         // %[dst_rgba]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I422ToAR30Row_SVE_SC(const uint8_t* src_y,
                                        const uint8_t* src_u,
                                        const uint8_t* src_v,
                                        uint8_t* dst_ar30,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  // The limit is used for saturating the 2.14 red channel in STOREAR30_SVE_2X.
  const uint16_t limit = 0x3ff0;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP_AR30
      "dup      z23.h, %w[limit]                        \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) STOREAR30_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.ge     1b                                      \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "2:                                               \n"
      "adds    %w[width], %w[width], %w[vl], lsl #1     \n"
      "b.eq    99f                                      \n"

      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV422_SVE_2X I4XXTORGB_SVE_2X(b) STOREAR30_SVE_2X

      "99:                                              \n"
      : [src_y] "+r"(src_y),                                // %[src_y]
        [src_u] "+r"(src_u),                                // %[src_u]
        [src_v] "+r"(src_v),                                // %[src_v]
        [dst_ar30] "+r"(dst_ar30),                          // %[dst_ar30]
        [width] "+r"(width),                                // %[width]
        [vl] "=&r"(vl)                                      // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),            // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias),  // %[kRGBCoeffBias]
        [limit] "r"(limit)                                  // %[limit]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I422AlphaToARGBRow_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    const uint8_t* src_a,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV422_SVE_2X
      "ld1b     {z19.h}, p1/z, [%[src_a]]               \n"
      "ld1b     {z23.h}, p2/z, [%[src_a], #1, mul vl]   \n"
      "incb     %[src_a]                                \n"  // Alpha
      I4XXTORGB_SVE_2X(b) RGBATOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV422_SVE_2X
      "ld1b     {z19.h}, p1/z, [%[src_a]]               \n"
      "ld1b     {z23.h}, p2/z, [%[src_a], #1, mul vl]   \n"  // Alpha
      I4XXTORGB_SVE_2X(b) RGBATOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [src_a] "+r"(src_a),                               // %[src_a]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I444AlphaToARGBRow_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_u,
    const uint8_t* src_v,
    const uint8_t* src_a,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth     %[vl]                                   \n"
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUV444_SVE_2X
      "ld1b     {z19.h}, p1/z, [%[src_a]]               \n"
      "ld1b     {z23.h}, p2/z, [%[src_a], #1, mul vl]   \n"
      "incb     %[src_a]                                \n"  // Alpha
      I4XXTORGB_SVE_2X(b) RGBATOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUV444_SVE_2X
      "ld1b     {z19.h}, p1/z, [%[src_a]]               \n"
      "ld1b     {z23.h}, p2/z, [%[src_a], #1, mul vl]   \n"  // Alpha
      I4XXTORGB_SVE_2X(b) RGBATOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [src_a] "+r"(src_a),                               // %[src_a]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "=&r"(vl)                                     // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void NV12ToARGBRow_SVE_SC(const uint8_t* src_y,
                                        const uint8_t* src_uv,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READNV_SVE_2X NVTORGB_SVE_2X(b, t) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READNV_SVE_2X NVTORGB_SVE_2X(b, t) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_uv] "+r"(src_uv),                             // %[src_uv]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void NV21ToARGBRow_SVE_SC(const uint8_t* src_y,
                                        const uint8_t* src_vu,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READNV_SVE_2X NVTORGB_SVE_2X(t, b) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READNV_SVE_2X NVTORGB_SVE_2X(t, b) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_uv] "+r"(src_vu),                             // %[src_vu]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void NV12ToRGB24Row_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_uv,
    uint8_t* dst_rgb24,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cntb %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READNV_RGB24_SVE_2X NVTORGB_RGB24_SVE_2X(b, t) RGBTORGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st3b     {z16.b, z17.b, z18.b}, p1, [%[dst_rgb24]]       \n"
      "incb     %[dst_rgb24], all, mul #3               \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p1.b, wzr, %w[width]                    \n"
      "add      %w[width], %w[width], #1                \n"  // Round up to even
      "and      %w[width], %w[width], #0xfffffffe       \n"
      "whilelt  p2.b, wzr, %w[width]                    \n"  //
      READNV_RGB24_SVE_2X NVTORGB_RGB24_SVE_2X(b, t) RGBTORGB8_SVE_2X
      "st3b     {z16.b, z17.b, z18.b}, p1, [%[dst_rgb24]]       \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_uv] "+r"(src_uv),                             // %[src_uv]
        [dst_rgb24] "+r"(dst_rgb24),                       // %[dst_rgb24]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void NV21ToRGB24Row_SVE_SC(
    const uint8_t* src_y,
    const uint8_t* src_vu,
    uint8_t* dst_rgb24,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cntb %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READNV_RGB24_SVE_2X NVTORGB_RGB24_SVE_2X(t, b) RGBTORGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st3b     {z16.b, z17.b, z18.b}, p1, [%[dst_rgb24]]       \n"
      "incb     %[dst_rgb24], all, mul #3               \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p1.b, wzr, %w[width]                    \n"
      "add      %w[width], %w[width], #1                \n"  // Round up to even
      "and      %w[width], %w[width], #0xfffffffe       \n"
      "whilelt  p2.b, wzr, %w[width]                    \n"  //
      READNV_RGB24_SVE_2X NVTORGB_RGB24_SVE_2X(t, b) RGBTORGB8_SVE_2X
      "st3b     {z16.b, z17.b, z18.b}, p1, [%[dst_rgb24]]       \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_uv] "+r"(src_vu),                             // %[src_vu]
        [dst_rgb24] "+r"(dst_rgb24),                       // %[dst_rgb24]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

#define READYUY2_SVE_2X                                                  \
  "ld1w       {z2.s}, p1/z, [%[src_yuy2]]            \n" /* YUYV low */  \
  "ld1w       {z3.s}, p2/z, [%[src_yuy2], #1, mul vl]\n" /* YUYV high */ \
  "incb       %[src_yuy2], all, mul #2               \n"                 \
  "prfm       pldl1keep, [%[src_yuy2], 448]          \n"                 \
  "trn1       z0.b, z2.b, z2.b                       \n" /* Y0Y0 */      \
  "trn1       z1.b, z3.b, z3.b                       \n" /* Y1Y1 */      \
  "trn2       z4.h, z2.h, z2.h                       \n" /* YVYV0 */     \
  "trn1       z2.h, z2.h, z2.h                       \n" /* YUYU0 */     \
  "trn2       z5.h, z3.h, z3.h                       \n" /* YVYV1 */     \
  "trn1       z3.h, z3.h, z3.h                       \n" /* YUYU1 */

static inline void YUY2ToARGBRow_SVE_SC(const uint8_t* src_yuy2,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"
      "dup      z19.b, #255                             \n"  // Alpha
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READYUY2_SVE_2X I4XXTORGB_SVE_2X(t) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READYUY2_SVE_2X I4XXTORGB_SVE_2X(t) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_yuy2] "+r"(src_yuy2),                         // %[src_yuy2]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

#define READUYVY_SVE_2X                                                  \
  "ld1w       {z2.s}, p1/z, [%[src_uyvy]]            \n" /* UYVY low */  \
  "ld1w       {z3.s}, p2/z, [%[src_uyvy], #1, mul vl]\n" /* UYVY high */ \
  "incb       %[src_uyvy], all, mul #2               \n"                 \
  "prfm       pldl1keep, [%[src_uyvy], 448]          \n"                 \
  "trn2       z0.b, z2.b, z2.b                       \n" /* Y0Y0 */      \
  "trn2       z1.b, z3.b, z3.b                       \n" /* Y1Y1 */      \
  "trn2       z4.h, z2.h, z2.h                       \n" /* VYVY0 */     \
  "trn1       z2.h, z2.h, z2.h                       \n" /* UYUY0 */     \
  "trn2       z5.h, z3.h, z3.h                       \n" /* VYVY1 */     \
  "trn1       z3.h, z3.h, z3.h                       \n" /* UYUY1 */

static inline void UYVYToARGBRow_SVE_SC(const uint8_t* src_uyvy,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"
      "dup      z19.b, #255                             \n"  // Alpha
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READUYVY_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READUYVY_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_uyvy] "+r"(src_uyvy),                         // %[src_uyvy]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I210ToARGBRow_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_u,
                                        const uint16_t* src_v,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READI210_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READI210_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I210AlphaToARGBRow_SVE_SC(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    const uint16_t* src_a,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                               \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1               \n"
      "b.lt     2f                                                 \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                               \n"
      "ptrue    p2.b                                               \n"
      "1:                                                          \n"  //
      READI210_SVE_2X I4XXTORGB_SVE_2X(b) READA10_SVE_2X RGBATOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1               \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                                 \n"

      "2:                                                          \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1               \n"
      "b.eq     99f                                                \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                               \n"
      "whilelt  p2.h, %w[vl], %w[width]                            \n"  //
      READI210_SVE_2X I4XXTORGB_SVE_2X(b) READA10_SVE_2X RGBATOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]                  \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl]      \n"

      "99:                                                         \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [src_a] "+r"(src_a),                               // %[src_a]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I210ToAR30Row_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_u,
                                        const uint16_t* src_v,
                                        uint8_t* dst_ar30,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  // The limit is used for saturating the 2.14 red channel in STOREAR30_SVE_2X.
  uint16_t limit = 0x3ff0;
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z23.h, %w[limit]                        \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READI210_SVE_2X I4XXTORGB_SVE_2X(b) STOREAR30_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READI210_SVE_2X I4XXTORGB_SVE_2X(b) STOREAR30_SVE_2X

      "99:                                              \n"
      : [src_y] "+r"(src_y),                                // %[src_y]
        [src_u] "+r"(src_u),                                // %[src_u]
        [src_v] "+r"(src_v),                                // %[src_v]
        [dst_ar30] "+r"(dst_ar30),                          // %[dst_ar30]
        [width] "+r"(width)                                 // %[width]
      : [vl] "r"(vl),                                       // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),            // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias),  // %[kRGBCoeffBias]
        [limit] "r"(limit)                                  // %[limit]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

// P210 has 10 bits in msb of 16 bit NV12 style layout.
static inline void P210ToARGBRow_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_uv,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READP210_SVE_2X I4XXTORGB_SVE_2X(t) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READP210_SVE_2X I4XXTORGB_SVE_2X(t) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_uv] "+r"(src_uv),                             // %[src_uv]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void P210ToAR30Row_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_uv,
                                        uint8_t* dst_ar30,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  // The limit is used for saturating the 2.14 red channel in STOREAR30_SVE_2X.
  uint16_t limit = 0x3ff0;
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z23.h, %w[limit]                        \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READP210_SVE_2X I4XXTORGB_SVE_2X(t) STOREAR30_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READP210_SVE_2X I4XXTORGB_SVE_2X(t) STOREAR30_SVE_2X

      "99:                                              \n"
      : [src_y] "+r"(src_y),                                // %[src_y]
        [src_uv] "+r"(src_uv),                              // %[src_uv]
        [dst_ar30] "+r"(dst_ar30),                          // %[dst_ar30]
        [width] "+r"(width)                                 // %[width]
      : [vl] "r"(vl),                                       // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),            // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias),  // %[kRGBCoeffBias]
        [limit] "r"(limit)                                  // %[limit]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I410ToARGBRow_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_u,
                                        const uint16_t* src_v,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READI410_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READI410_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I410AlphaToARGBRow_SVE_SC(
    const uint16_t* src_y,
    const uint16_t* src_u,
    const uint16_t* src_v,
    const uint16_t* src_a,
    uint8_t* dst_argb,
    const struct YuvConstants* yuvconstants,
    int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                             \n"  //
      YUVTORGB_SVE_SETUP
      "subs     %w[width], %w[width], %w[vl], lsl #1             \n"
      "b.lt     2f                                               \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                             \n"
      "ptrue    p2.b                                             \n"
      "1:                                                        \n"  //
      READI410_SVE_2X I4XXTORGB_SVE_2X(b) READA10_SVE_2X RGBATOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1             \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                               \n"

      "2:                                                        \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1             \n"
      "b.eq     99f                                              \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                             \n"
      "whilelt  p2.h, %w[vl], %w[width]                          \n"  //
      READI410_SVE_2X I4XXTORGB_SVE_2X(b) READA10_SVE_2X RGBATOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]                \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl]    \n"

      "99:                                                       \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [src_a] "+r"(src_a),                               // %[src_a]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I410ToAR30Row_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_u,
                                        const uint16_t* src_v,
                                        uint8_t* dst_ar30,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  // The limit is used for saturating the 2.14 red channel in STOREAR30_SVE_2X.
  uint16_t limit = 0x3ff0;
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z23.h, %w[limit]                        \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READI410_SVE_2X I4XXTORGB_SVE_2X(b) STOREAR30_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READI410_SVE_2X I4XXTORGB_SVE_2X(b) STOREAR30_SVE_2X

      "99:                                              \n"
      : [src_y] "+r"(src_y),                                // %[src_y]
        [src_u] "+r"(src_u),                                // %[src_u]
        [src_v] "+r"(src_v),                                // %[src_v]
        [dst_ar30] "+r"(dst_ar30),                          // %[dst_argb]
        [width] "+r"(width)                                 // %[width]
      : [vl] "r"(vl),                                       // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),            // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias),  // %[kRGBCoeffBias]
        [limit] "r"(limit)                                  // %[limit]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void P410ToARGBRow_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_uv,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.h                                    \n"
      "ptrue    p2.s                                    \n"
      "ptrue    p3.s                                    \n"
      "1:                                               \n"  //
      READP410_SVE NVTORGB_SVE RGBTOARGB8_SVE
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "add      %[dst_argb], %[dst_argb], %[vl], lsl #2 \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.s, wzr, %w[width]                    \n"
      "cntw     %x[vl]                                  \n"
      "whilelt  p3.s, %w[vl], %w[width]                 \n"  //
      READP410_SVE NVTORGB_SVE RGBTOARGB8_SVE
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_uv] "+r"(src_uv),                             // %[src_uv]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width),                               // %[width]
        [vl] "+r"(vl)                                      // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void P410ToAR30Row_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_uv,
                                        uint8_t* dst_ar30,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  // The limit is used for saturating the 2.14 red channel in STOREAR30_SVE.
  uint16_t limit = 0x3ff0;
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z23.h, %w[limit]                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.h                                    \n"
      "ptrue    p2.s                                    \n"
      "ptrue    p3.s                                    \n"
      "1:                                               \n"  //
      READP410_SVE NVTORGB_SVE
      "subs     %w[width], %w[width], %w[vl]            \n"  //
      STOREAR30_SVE
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.s, wzr, %w[width]                    \n"
      "cntw     %x[vl]                                  \n"
      "whilelt  p3.s, %w[vl], %w[width]                 \n"  //
      READP410_SVE NVTORGB_SVE STOREAR30_SVE

      "99:                                              \n"
      : [src_y] "+r"(src_y),                                // %[src_y]
        [src_uv] "+r"(src_uv),                              // %[src_uv]
        [dst_ar30] "+r"(dst_ar30),                          // %[dst_ar30]
        [width] "+r"(width),                                // %[width]
        [vl] "+r"(vl)                                       // %[vl]
      : [kUVCoeff] "r"(&yuvconstants->kUVCoeff),            // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias),  // %[kRGBCoeffBias]
        [limit] "r"(limit)                                  // %[limit]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I212ToAR30Row_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_u,
                                        const uint16_t* src_v,
                                        uint8_t* dst_ar30,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  // The limit is used for saturating the 2.14 red channel in STOREAR30_SVE_2X.
  uint16_t limit = 0x3ff0;
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z23.h, %w[limit]                        \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READI212_SVE_2X I4XXTORGB_SVE_2X(b) STOREAR30_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READI212_SVE_2X I4XXTORGB_SVE_2X(b) STOREAR30_SVE_2X

      "99:                                              \n"
      : [src_y] "+r"(src_y),                                // %[src_y]
        [src_u] "+r"(src_u),                                // %[src_u]
        [src_v] "+r"(src_v),                                // %[src_v]
        [dst_ar30] "+r"(dst_ar30),                          // %[dst_ar30]
        [width] "+r"(width)                                 // %[width]
      : [vl] "r"(vl),                                       // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),            // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias),  // %[kRGBCoeffBias]
        [limit] "r"(limit)                                  // %[limit]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

static inline void I212ToARGBRow_SVE_SC(const uint16_t* src_y,
                                        const uint16_t* src_u,
                                        const uint16_t* src_v,
                                        uint8_t* dst_argb,
                                        const struct YuvConstants* yuvconstants,
                                        int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cnth %0" : "=r"(vl));
  asm volatile(
      "ptrue    p0.b                                    \n"  //
      YUVTORGB_SVE_SETUP
      "dup      z19.b, #255                             \n"  // Alpha
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with an all-true predicate to avoid predicate
      // generation overhead.
      "ptrue    p1.b                                    \n"
      "ptrue    p2.b                                    \n"
      "1:                                               \n"  //
      READI212_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"  //
      STOREARGB8_SVE_2X(dst_argb)
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt  p1.h, wzr, %w[width]                    \n"
      "whilelt  p2.h, %w[vl], %w[width]                 \n"  //
      READI212_SVE_2X I4XXTORGB_SVE_2X(b) RGBTOARGB8_SVE_2X
      "st2h     {z16.h, z17.h}, p1, [%[dst_argb]]       \n"
      "st2h     {z20.h, z21.h}, p2, [%[dst_argb], #2, mul vl] \n"

      "99:                                              \n"
      : [src_y] "+r"(src_y),                               // %[src_y]
        [src_u] "+r"(src_u),                               // %[src_u]
        [src_v] "+r"(src_v),                               // %[src_v]
        [dst_argb] "+r"(dst_argb),                         // %[dst_argb]
        [width] "+r"(width)                                // %[width]
      : [vl] "r"(vl),                                      // %[vl]
        [kUVCoeff] "r"(&yuvconstants->kUVCoeff),           // %[kUVCoeff]
        [kRGBCoeffBias] "r"(&yuvconstants->kRGBCoeffBias)  // %[kRGBCoeffBias]
      : "memory", "cc", YUVTORGB_SVE_REGS);
}

#define CONVERT8TO8_SVE                                  \
  "ld1b        {z0.b}, p0/z, [%[src]]                \n" \
  "ld1b        {z1.b}, p1/z, [%[src], #1, mul vl]    \n" \
  "incb        %[src], all, mul #2                   \n" \
  "subs        %w[width], %w[width], %w[vl], lsl #1  \n" \
  "umulh       z0.b, z0.b, z2.b                      \n" \
  "umulh       z1.b, z1.b, z2.b                      \n" \
  "prfm        pldl1keep, [%[src], 448]              \n" \
  "add         z0.b, z0.b, z3.b                      \n" \
  "add         z1.b, z1.b, z3.b                      \n" \
  "st1b        {z0.b}, p0, [%[dst]]                  \n" \
  "st1b        {z1.b}, p1, [%[dst], #1, mul vl]      \n" \
  "incb        %[dst], all, mul #2                   \n"

static inline void Convert8To8Row_SVE_SC(const uint8_t* src_y,
                                         uint8_t* dst_y,
                                         int scale,
                                         int bias,
                                         int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "dup      z2.b, %w[scale]                         \n"
      "dup      z3.b, %w[bias]                          \n"
      "cntb     %[vl]                                   \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      // Run bulk of computation with all-true predicates to avoid predicate
      // generation overhead.
      "ptrue    p0.b                                    \n"
      "ptrue    p1.b                                    \n"
      "1:                                               \n"  //
      CONVERT8TO8_SVE
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt     p0.b, wzr, %w[width]                 \n"
      "whilelt     p1.b, %w[vl], %w[width]              \n"  //
      CONVERT8TO8_SVE

      "99:                                              \n"
      : [src] "+r"(src_y),    // %[src]
        [dst] "+r"(dst_y),    // %[dst]
        [width] "+r"(width),  // %[width]
        [vl] "=&r"(vl)        // %[vl]
      : [scale] "r"(scale),   // %[scale]
        [bias] "r"(bias)      // %[bias]
      : "memory", "cc", "z0", "z1", "z2", "z3", "p0", "p1");
}

// SVE constants are stored negated such that we can store 128 in int8_t.

// RGB to BT601 coefficients
// UB   0.875 coefficient = 112
// UG -0.5781 coefficient = -74
// UR -0.2969 coefficient = -38
// VB -0.1406 coefficient = -18
// VG -0.7344 coefficient = -94
// VR   0.875 coefficient = 112

static const int8_t kARGBToUVCoefficients[] = {
    // -UB, -UG, -UR, 0, -VB, -VG, -VR, 0
    -112, 74, 38, 0, 18, 94, -112, 0,
};

static const int8_t kABGRToUVCoefficients[] = {
    // -UR, -UG, -UB, 0, -VR, -VG, -VB, 0
    38, 74, -112, 0, -112, 94, 18, 0,
};

static const int8_t kBGRAToUVCoefficients[] = {
    // 0, -UR, -UG, -UB, 0, -VR, -VG, -VB
    0, 38, 74, -112, 0, -112, 94, 18,
};

static const int8_t kRGBAToUVCoefficients[] = {
    // 0, -UB, -UG, -UR, 0, -VB, -VG, -VR
    0, -112, 74, 38, 0, 18, 94, -112,
};

// RGB to JPEG coefficients
// UB  0.500    coefficient = 128
// UG -0.33126  coefficient = -85
// UR -0.16874  coefficient = -43
// VB -0.08131  coefficient = -21
// VG -0.41869  coefficient = -107
// VR 0.500     coefficient = 128

static const int8_t kARGBToUVJCoefficients[] = {
    // -UB, -UG, -UR, 0, -VB, -VG, -VR, 0
    -128, 85, 43, 0, 21, 107, -128, 0,
};

static const int8_t kABGRToUVJCoefficients[] = {
    // -UR, -UG, -UB, 0, -VR, -VG, -VB, 0
    43, 85, -128, 0, -128, 107, 21, 0,
};

#define ARGBTOUVMATRIX_SVE                                                  \
  "ld1d     {z0.d}, p1/z, [%[src0]]               \n" /* ABCD(bgra) */      \
  "ld1d     {z1.d}, p2/z, [%[src0], #1, mul vl]   \n" /* EFGH(bgra) */      \
  "ld1d     {z2.d}, p3/z, [%[src0], #2, mul vl]   \n" /* IJKL(bgra) */      \
  "ld1d     {z3.d}, p4/z, [%[src0], #3, mul vl]   \n" /* MNOP(bgra) */      \
  "ld1d     {z4.d}, p1/z, [%[src1]]               \n" /* ABCD(bgra) */      \
  "ld1d     {z5.d}, p2/z, [%[src1], #1, mul vl]   \n" /* EFGH(bgra) */      \
  "ld1d     {z6.d}, p3/z, [%[src1], #2, mul vl]   \n" /* IJKL(bgra) */      \
  "ld1d     {z7.d}, p4/z, [%[src1], #3, mul vl]   \n" /* MNOP(bgra) */      \
  "incb     %[src0], all, mul #4                  \n"                       \
  "incb     %[src1], all, mul #4                  \n"                       \
                                                                            \
  "uaddlb   z16.h, z0.b, z4.b                     \n" /* ABCD(br) */        \
  "uaddlb   z18.h, z1.b, z5.b                     \n" /* EFGH(br) */        \
  "uaddlb   z20.h, z2.b, z6.b                     \n" /* IJKL(br) */        \
  "uaddlb   z22.h, z3.b, z7.b                     \n" /* MNOP(br) */        \
  "uaddlt   z17.h, z0.b, z4.b                     \n" /* ABCD(ga) */        \
  "uaddlt   z19.h, z1.b, z5.b                     \n" /* EFGH(ga) */        \
  "uaddlt   z21.h, z2.b, z6.b                     \n" /* IJKL(ga) */        \
  "uaddlt   z23.h, z3.b, z7.b                     \n" /* MNOP(ga) */        \
                                                                            \
  /* Use ADDP on 32-bit elements to add adjacent pairs of 9-bit unsigned */ \
  "addp     z16.s, p0/m, z16.s, z18.s             \n" /* ABEFCDGH(br) */    \
  "addp     z17.s, p0/m, z17.s, z19.s             \n" /* ABEFCDGH(ga) */    \
  "addp     z20.s, p0/m, z20.s, z22.s             \n" /* IJMNKLOP(br) */    \
  "addp     z21.s, p0/m, z21.s, z23.s             \n" /* IJMNKLOP(ga) */    \
                                                                            \
  "rshrnb    z0.b, z16.h, #2                      \n" /* ABEFCDGH(b0r0) */  \
  "rshrnb    z1.b, z20.h, #2                      \n" /* IJMNKLOP(b0r0) */  \
  "rshrnt    z0.b, z17.h, #2                      \n" /* ABEFCDGH(bgra) */  \
  "rshrnt    z1.b, z21.h, #2                      \n" /* IJMNKLOP(bgra) */  \
                                                                            \
  "tbl       z0.s, {z0.s}, z27.s                  \n" /* ABCDEFGH */        \
  "tbl       z1.s, {z1.s}, z27.s                  \n" /* IJKLMNOP */        \
                                                                            \
  "subs     %w[width], %w[width], %w[vl], lsl #2  \n" /* VL per loop */     \
                                                                            \
  "mov      z16.b, #0                             \n"                       \
  "mov      z17.b, #0                             \n"                       \
  "mov      z20.b, #0                             \n"                       \
  "mov      z21.b, #0                             \n"                       \
                                                                            \
  "usdot    z16.s, z0.b, z24.b                    \n"                       \
  "usdot    z17.s, z1.b, z24.b                    \n"                       \
  "usdot    z20.s, z0.b, z25.b                    \n"                       \
  "usdot    z21.s, z1.b, z25.b                    \n"                       \
                                                                            \
  "subhnb   z16.b, z26.h, z16.h                   \n" /* U */               \
  "subhnb   z20.b, z26.h, z20.h                   \n" /* V */               \
  "subhnb   z17.b, z26.h, z17.h                   \n" /* U */               \
  "subhnb   z21.b, z26.h, z21.h                   \n" /* V */               \
                                                                            \
  "uzp1     z16.h, z16.h, z17.h                   \n"                       \
  "uzp1     z20.h, z20.h, z21.h                   \n"                       \
                                                                            \
  "st1b     {z16.h}, p5, [%[dst_u]]               \n" /* U */               \
  "st1b     {z20.h}, p5, [%[dst_v]]               \n" /* V */               \
  "inch     %[dst_u]                              \n"                       \
  "inch     %[dst_v]                              \n"

static inline void ARGBToUVMatrixRow_SVE_SC(const uint8_t* src_argb,
                                            int src_stride_argb,
                                            uint8_t* dst_u,
                                            uint8_t* dst_v,
                                            int width,
                                            const struct ArgbConstants* c)
    STREAMING_COMPATIBLE {
  const uint8_t* src_argb_1 = src_argb + src_stride_argb;
  const int odd = width & 1;
  const int even_width = width & ~1;
  const uint8_t* odd_src0 = src_argb + (ptrdiff_t)even_width * 4;
  const uint8_t* odd_src1 = src_argb_1 + (ptrdiff_t)even_width * 4;
  uint8_t* odd_dst_u = dst_u + (even_width >> 1);
  uint8_t* odd_dst_v = dst_v + (even_width >> 1);
  uint32_t u_coeff;
  uint32_t v_coeff;
  uint16_t add_uv;
  __builtin_memcpy(&u_coeff, &c->kRGBToU, sizeof(u_coeff));
  __builtin_memcpy(&v_coeff, &c->kRGBToV, sizeof(v_coeff));
  __builtin_memcpy(&add_uv, &c->kAddUV, sizeof(add_uv));
  uint64_t vl;
  asm("cntd %x0" : "=r"(vl));

  width >>= 1;

  asm volatile(
      "ptrue    p0.b                                 \n"
      "dup      z24.s, %w[u_coeff]                   \n"
      "dup      z25.s, %w[v_coeff]                   \n"
      "dup      z26.h, %w[add_uv]                    \n"

      // Generate some TBL indices to undo the interleaving from ADDP.
      "index    z0.s, #0, #1                         \n"
      "index    z1.s, #1, #1                         \n"
      "uzp1     z27.s, z0.s, z1.s                    \n"

      "subs     %w[width], %w[width], %w[vl], lsl #2 \n"
      "b.lt    2f                                    \n"

      "ptrue  p1.d                                   \n"
      "ptrue  p2.d                                   \n"
      "ptrue  p3.d                                   \n"
      "ptrue  p4.d                                   \n"
      "ptrue  p5.h                                   \n"
      "1:                                            \n"  //
      ARGBTOUVMATRIX_SVE
      "b.ge     1b                                   \n"

      "2:                                            \n"
      "adds    %w[width], %w[width], %w[vl], lsl #2  \n"
      "b.eq    99f                                   \n"

      "3:                                            \n"
      "whilelt  p1.d, wzr, %w[width]                 \n"
      "whilelt  p2.d, %w[vl], %w[width]              \n"
      "whilelt  p3.d, %w[vl2], %w[width]             \n"
      "whilelt  p4.d, %w[vl3], %w[width]             \n"
      "whilelt  p5.h, wzr, %w[width]                 \n"  //
      ARGBTOUVMATRIX_SVE
      "b.gt     3b                                   \n"

      "99:                                           \n"
      : [src0] "+r"(src_argb),            // %[src0]
        [src1] "+r"(src_argb_1),          // %[src1]
        [dst_u] "+r"(dst_u),              // %[dst_u]
        [dst_v] "+r"(dst_v),              // %[dst_v]
        [width] "+r"(width)               // %[width]
      : [u_coeff] "r"(u_coeff),           // %[u_coeff]
        [v_coeff] "r"(v_coeff),           // %[v_coeff]
        [add_uv] "r"((uint32_t)add_uv),   // %[add_uv]
        [vl] "r"(vl),                     // %[vl]
        [vl2] "r"(vl * 2),                // %[vl2]
        [vl3] "r"(vl * 3)                 // %[vl3]
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z16",
        "z17", "z18", "z19", "z20", "z21", "z22", "z23", "z24", "z25", "z26",
        "z27", "p0", "p1", "p2", "p3", "p4", "p5");

  if (odd) {
    asm volatile(
        "ptrue    p0.b                                 \n"
        "ptrue    p1.s, vl1                            \n"
        "dup      z24.s, %w[u_coeff]                   \n"
        "dup      z25.s, %w[v_coeff]                   \n"
        "dup      z26.h, %w[add_uv]                    \n"
        "ld1w     {z0.s}, p1/z, [%[src0]]              \n"
        "ld1w     {z4.s}, p1/z, [%[src1]]              \n"
        "urhadd   z0.b, p0/m, z0.b, z4.b               \n"
        "mov      z16.b, #0                            \n"
        "mov      z20.b, #0                            \n"
        "usdot    z16.s, z0.b, z24.b                   \n"
        "usdot    z20.s, z0.b, z25.b                   \n"
        "subhnb   z16.b, z26.h, z16.h                  \n"
        "subhnb   z20.b, z26.h, z20.h                  \n"
        "st1b     {z16.s}, p1, [%[dst_u]]              \n"
        "st1b     {z20.s}, p1, [%[dst_v]]              \n"
        :
        : [src0] "r"(odd_src0),
          [src1] "r"(odd_src1),
          [dst_u] "r"(odd_dst_u),
          [dst_v] "r"(odd_dst_v),
          [u_coeff] "r"(u_coeff),
          [v_coeff] "r"(v_coeff),
          [add_uv] "r"((uint32_t)add_uv)
        : "memory", "z0", "z4", "z16", "z20", "z24", "z25", "z26", "p0", "p1");
  }
}

#define ARGBTOYMATRIX_SVE                              \
  "ld1w     {z0.s}, p1/z, [%[src]]             \n"     \
  "ld1w     {z1.s}, p2/z, [%[src], #1, mul vl] \n"     \
  "ld1w     {z2.s}, p3/z, [%[src], #2, mul vl] \n"     \
  "ld1w     {z3.s}, p4/z, [%[src], #3, mul vl] \n"     \
  "incb     %[src], all, mul #4                    \n" \
  "mov      z16.b, #0                              \n" \
  "mov      z17.b, #0                              \n" \
  "mov      z18.b, #0                              \n" \
  "mov      z19.b, #0                              \n" \
  "udot     z16.s, z0.b, z24.b                     \n" \
  "udot     z17.s, z1.b, z24.b                     \n" \
  "udot     z18.s, z2.b, z24.b                     \n" \
  "udot     z19.s, z3.b, z24.b                     \n" \
  "uzp1     z16.h, z16.h, z17.h                    \n" \
  "uzp1     z18.h, z18.h, z19.h                    \n" \
  "addhnb   z16.b, z16.h, z25.h                    \n" \
  "addhnb   z18.b, z18.h, z25.h                    \n" \
  "uzp1     z20.b, z16.b, z18.b                    \n" \
  "st1b     {z20.b}, p5, [%[dst_y]]                \n" \
  "incb     %[dst_y]                               \n" \
  "subs     %w[width], %w[width], %w[vl], lsl #2   \n"

static inline void ARGBToYMatrixRow_SVE_SC(const uint8_t* src_argb,
                                           uint8_t* dst_y,
                                           int width,
                                           const struct ArgbConstants* c)
    STREAMING_COMPATIBLE {
  uint32_t y_coeff;
  uint16_t add_y;
  __builtin_memcpy(&y_coeff, &c->kRGBToY, sizeof(y_coeff));
  __builtin_memcpy(&add_y, &c->kAddY, sizeof(add_y));
  uint64_t vl;
  asm("cntw %x0" : "=r"(vl));

  asm volatile(
      "dup      z24.s, %w[y_coeff]                   \n"
      "dup      z25.h, %w[add_y]                     \n"

      "subs     %w[width], %w[width], %w[vl], lsl #2 \n"
      "b.lt     2f                                   \n"

      "ptrue    p1.s                                 \n"
      "ptrue    p2.s                                 \n"
      "ptrue    p3.s                                 \n"
      "ptrue    p4.s                                 \n"
      "ptrue    p5.b                                 \n"
      "1:                                            \n"  //
      ARGBTOYMATRIX_SVE
      "b.ge     1b                                   \n"

      "2:                                            \n"
      "adds     %w[width], %w[width], %w[vl], lsl #2 \n"
      "b.eq     99f                                  \n"

      "3:                                            \n"
      "whilelt  p1.s, wzr, %w[width]                 \n"
      "whilelt  p2.s, %w[vl], %w[width]              \n"
      "whilelt  p3.s, %w[vl2], %w[width]             \n"
      "whilelt  p4.s, %w[vl3], %w[width]             \n"
      "whilelt  p5.b, wzr, %w[width]                 \n"  //
      ARGBTOYMATRIX_SVE
      "b.gt     3b                                   \n"

      "99:                                           \n"
      : [src] "+r"(src_argb),           // %[src]
        [dst_y] "+r"(dst_y),            // %[dst_y]
        [width] "+r"(width)             // %[width]
      : [y_coeff] "r"(y_coeff),         // %[y_coeff]
        [add_y] "r"((uint32_t)add_y),   // %[add_y]
        [vl] "r"(vl),                   // %[vl]
        [vl2] "r"(vl * 2),              // %[vl2]
        [vl3] "r"(vl * 3)               // %[vl3]
      : "memory", "cc", "z0", "z1", "z2", "z3", "z16", "z17", "z18", "z19",
        "z20", "z24", "z25", "p1", "p2", "p3", "p4", "p5");
}

#define SSIM4X4_SVE                                               \
  "ld1b     {z0.b}, p1/z, [%[src_a]]                          \n" \
  "ld1b     {z1.b}, p1/z, [%[src_a], %[stride_a]]             \n" \
  "ld1b     {z2.b}, p1/z, [%[src_a], %[stride_a2]]            \n" \
  "ld1b     {z3.b}, p1/z, [%[src_a], %[stride_a3]]            \n" \
  "ld1b     {z4.b}, p1/z, [%[src_b]]                          \n" \
  "ld1b     {z5.b}, p1/z, [%[src_b], %[stride_b]]             \n" \
  "ld1b     {z6.b}, p1/z, [%[src_b], %[stride_b2]]            \n" \
  "ld1b     {z7.b}, p1/z, [%[src_b], %[stride_b3]]            \n" \
  "incb     %[src_a]                                          \n" \
  "incb     %[src_b]                                          \n" \
  "mov      z16.s, #0                                         \n" \
  "mov      z17.s, #0                                         \n" \
  "mov      z18.s, #0                                         \n" \
  "mov      z19.s, #0                                         \n" \
  "mov      z21.s, #0                                         \n" \
  "udot     z16.s, z0.b, z20.b                                \n" \
  "udot     z17.s, z4.b, z20.b                                \n" \
  "udot     z18.s, z0.b, z0.b                                 \n" \
  "udot     z21.s, z4.b, z4.b                                 \n" \
  "udot     z19.s, z0.b, z4.b                                 \n" \
  "udot     z16.s, z1.b, z20.b                                \n" \
  "udot     z17.s, z5.b, z20.b                                \n" \
  "udot     z18.s, z1.b, z1.b                                 \n" \
  "udot     z21.s, z5.b, z5.b                                 \n" \
  "udot     z19.s, z1.b, z5.b                                 \n" \
  "udot     z16.s, z2.b, z20.b                                \n" \
  "udot     z17.s, z6.b, z20.b                                \n" \
  "udot     z18.s, z2.b, z2.b                                 \n" \
  "udot     z21.s, z6.b, z6.b                                 \n" \
  "udot     z19.s, z2.b, z6.b                                 \n" \
  "udot     z16.s, z3.b, z20.b                                \n" \
  "udot     z17.s, z7.b, z20.b                                \n" \
  "udot     z18.s, z3.b, z3.b                                 \n" \
  "udot     z21.s, z7.b, z7.b                                 \n" \
  "udot     z19.s, z3.b, z7.b                                 \n" \
  "add      z18.s, z18.s, z21.s                               \n" \
  "subs     %w[width], %w[width], %w[vl]                      \n" \
  "st4w     {z16.s, z17.s, z18.s, z19.s}, p0, [%[dst]]        \n" \
  "incb     %[dst], all, mul #4                               \n"

static inline void Ssim4x4Row_SVE_SC(const uint8_t* src_a,
                                     int stride_a,
                                     const uint8_t* src_b,
                                     int stride_b,
                                     uint32_t* dst,
                                     int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  ptrdiff_t stride_a2 = (ptrdiff_t)stride_a * 2;
  ptrdiff_t stride_a3 = (ptrdiff_t)stride_a * 3;
  ptrdiff_t stride_b2 = (ptrdiff_t)stride_b * 2;
  ptrdiff_t stride_b3 = (ptrdiff_t)stride_b * 3;
  width >>= 2;
  asm volatile(
      "cntw     %x[vl]                                  \n"
      "mov      z20.b, #1                               \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.s                                    \n"
      "ptrue    p1.b                                    \n"
      "1:                                               \n"  //
      SSIM4X4_SVE
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.s, wzr, %w[width]                    \n"
      "lsl      %w[vl], %w[width], #2                   \n"
      "whilelt  p1.b, wzr, %w[vl]                       \n"  //
      SSIM4X4_SVE

      "99:                                              \n"
      : [src_a] "+r"(src_a),                  // %[src_a]
        [src_b] "+r"(src_b),                  // %[src_b]
        [dst] "+r"(dst),                      // %[dst]
        [width] "+r"(width),                  // %[width]
        [vl] "=&r"(vl)                        // %[vl]
      : [stride_a] "r"((ptrdiff_t)stride_a),  // %[stride_a]
        [stride_a2] "r"(stride_a2),           // %[stride_a2]
        [stride_a3] "r"(stride_a3),           // %[stride_a3]
        [stride_b] "r"((ptrdiff_t)stride_b),  // %[stride_b]
        [stride_b2] "r"(stride_b2),           // %[stride_b2]
        [stride_b3] "r"(stride_b3)            // %[stride_b3]
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z16",
        "z17", "z18", "z19", "z20", "z21", "p0", "p1");
}

#define SSIMWINDOW_SVE                                            \
  "ld1w     {z0.s}, p2/z, [%[row0]]                           \n" \
  "ld1w     {z1.s}, p3/z, [%[row0], #1, mul vl]               \n" \
  "ld1w     {z2.s}, p4/z, [%[row0], #2, mul vl]               \n" \
  "ld1w     {z3.s}, p5/z, [%[row0], #3, mul vl]               \n" \
  "incb     %[row0], all, mul #4                              \n" \
  "ld1w     {z4.s}, p2/z, [%[row0_next]]                      \n" \
  "ld1w     {z5.s}, p3/z, [%[row0_next], #1, mul vl]          \n" \
  "ld1w     {z6.s}, p4/z, [%[row0_next], #2, mul vl]          \n" \
  "ld1w     {z7.s}, p5/z, [%[row0_next], #3, mul vl]          \n" \
  "incb     %[row0_next], all, mul #4                         \n" \
  "add      z0.s, z0.s, z4.s                                  \n" \
  "add      z1.s, z1.s, z5.s                                  \n" \
  "add      z2.s, z2.s, z6.s                                  \n" \
  "add      z3.s, z3.s, z7.s                                  \n" \
  "ld1w     {z16.s}, p2/z, [%[row1]]                          \n" \
  "ld1w     {z17.s}, p3/z, [%[row1], #1, mul vl]              \n" \
  "ld1w     {z18.s}, p4/z, [%[row1], #2, mul vl]              \n" \
  "ld1w     {z19.s}, p5/z, [%[row1], #3, mul vl]              \n" \
  "incb     %[row1], all, mul #4                              \n" \
  "ld1w     {z4.s}, p2/z, [%[row1_next]]                      \n" \
  "ld1w     {z5.s}, p3/z, [%[row1_next], #1, mul vl]          \n" \
  "ld1w     {z6.s}, p4/z, [%[row1_next], #2, mul vl]          \n" \
  "ld1w     {z7.s}, p5/z, [%[row1_next], #3, mul vl]          \n" \
  "incb     %[row1_next], all, mul #4                         \n" \
  "add      z16.s, z16.s, z4.s                                \n" \
  "add      z17.s, z17.s, z5.s                                \n" \
  "add      z18.s, z18.s, z6.s                                \n" \
  "add      z19.s, z19.s, z7.s                                \n" \
  "add      z0.s, z0.s, z16.s                                 \n" \
  "add      z1.s, z1.s, z17.s                                 \n" \
  "add      z2.s, z2.s, z18.s                                 \n" \
  "add      z3.s, z3.s, z19.s                                 \n" \
  "uzp1     z4.s, z0.s, z1.s                                  \n" \
  "uzp2     z5.s, z0.s, z1.s                                  \n" \
  "uzp1     z6.s, z2.s, z3.s                                  \n" \
  "uzp2     z7.s, z2.s, z3.s                                  \n" \
  "uzp1     z0.s, z4.s, z6.s                                  \n" \
  "uzp2     z2.s, z4.s, z6.s                                  \n" \
  "uzp1     z1.s, z5.s, z7.s                                  \n" \
  "uzp2     z3.s, z5.s, z7.s                                  \n" \
  "mul      z4.s, z0.s, z1.s                                  \n" \
  "lsl      z4.s, z4.s, #1                                    \n" \
  "mul      z5.s, z0.s, z0.s                                  \n" \
  "mla      z5.s, p1/m, z1.s, z1.s                            \n" \
  "lsl      z3.s, z3.s, #7                                    \n" \
  "sub      z3.s, z3.s, z4.s                                  \n" \
  "add      z4.s, z4.s, z25.s                                 \n" \
  "add      z3.s, z3.s, z26.s                                 \n" \
  "lsl      z2.s, z2.s, #6                                    \n" \
  "sub      z2.s, z2.s, z5.s                                  \n" \
  "add      z5.s, z5.s, z25.s                                 \n" \
  "add      z2.s, z2.s, z26.s                                 \n" \
  "scvtf    z4.s, p1/m, z4.s                                  \n" \
  "scvtf    z3.s, p1/m, z3.s                                  \n" \
  "scvtf    z5.s, p1/m, z5.s                                  \n" \
  "scvtf    z2.s, p1/m, z2.s                                  \n" \
  "fmul     z4.s, z4.s, z3.s                                  \n" \
  "fmul     z5.s, z5.s, z2.s                                  \n" \
  "fdiv     z4.s, p1/m, z4.s, z5.s                            \n" \
  "fadd     z24.s, p0/m, z24.s, z4.s                          \n"

static inline float SsimWindowRow_SVE_SC(const uint32_t* row0,
                                         const uint32_t* row1,
                                         int num_windows) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm("cntw %x0" : "=r"(vl));
  const uint32_t* row0_next = row0 + 4;
  const uint32_t* row1_next = row1 + 4;
  float total;
  asm volatile(
      "ptrue    p1.s                                       \n"
      "mov      z24.s, #0                                  \n"
      "dup      z25.s, %w[c1]                              \n"
      "dup      z26.s, %w[c2]                              \n"
      "subs     %w[num_windows], %w[num_windows], %w[vl]   \n"
      "b.lt     2f                                         \n"

      "ptrue    p0.s                                       \n"
      "ptrue    p2.s                                       \n"
      "ptrue    p3.s                                       \n"
      "ptrue    p4.s                                       \n"
      "ptrue    p5.s                                       \n"
      "1:                                                  \n"  //
      SSIMWINDOW_SVE
      "subs     %w[num_windows], %w[num_windows], %w[vl]   \n"
      "b.ge     1b                                         \n"

      "2:                                                  \n"
      "adds     %w[num_windows], %w[num_windows], %w[vl]   \n"
      "b.eq     99f                                        \n"

      "whilelt  p0.s, wzr, %w[num_windows]                 \n"
      "lsl      %w[num_windows], %w[num_windows], #2       \n"
      "whilelt  p2.s, wzr, %w[num_windows]                 \n"
      "whilelt  p3.s, %w[vl], %w[num_windows]              \n"
      "whilelt  p4.s, %w[vl2], %w[num_windows]             \n"
      "whilelt  p5.s, %w[vl3], %w[num_windows]             \n"  //
      SSIMWINDOW_SVE

      "99:                                                 \n"
      "faddv    %s[total], p1, z24.s                       \n"
      : [row0] "+r"(row0),                // %[row0]
        [row0_next] "+r"(row0_next),      // %[row0_next]
        [row1] "+r"(row1),                // %[row1]
        [row1_next] "+r"(row1_next),      // %[row1_next]
        [num_windows] "+r"(num_windows),  // %[num_windows]
        [total] "=w"(total)               // %[total]
      : [vl] "r"(vl),                     // %[vl]
        [vl2] "r"(vl * 2),                // %[vl2]
        [vl3] "r"(vl * 3),                // %[vl3]
        [c1] "r"(26634),                  // %[c1]
        [c2] "r"(239708)                  // %[c2]
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z16",
        "z17", "z18", "z19", "z24", "z25", "z26", "p0", "p1", "p2", "p3", "p4",
        "p5");
  return total;
}

static inline void ARGBOrRow_SVE_SC(const uint8_t* src_argb,
                                    uint8_t* dst_argb,
                                    uint32_t value,
                                    int width) STREAMING_COMPATIBLE {
  uint32_t vl;
  asm volatile(
      "cntw     %x[vl]                                  \n"
      "dup      z2.s, %w[value]                         \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.s                                    \n"
      "1:                                               \n"
      "ld1w     {z0.s}, p0/z, [%[src]]                  \n"
      "ld1w     {z1.s}, p0/z, [%[src], #1, mul vl]      \n"
      "incb     %[src], all, mul #2                     \n"
      "orr      z0.d, z0.d, z2.d                        \n"
      "orr      z1.d, z1.d, z2.d                        \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "st1w     {z0.s}, p0, [%[dst]]                    \n"
      "st1w     {z1.s}, p0, [%[dst], #1, mul vl]        \n"
      "incb     %[dst], all, mul #2                     \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.s, wzr, %w[width]                    \n"
      "whilelt  p1.s, %w[vl], %w[width]                 \n"
      "ld1w     {z0.s}, p0/z, [%[src]]                  \n"
      "ld1w     {z1.s}, p1/z, [%[src], #1, mul vl]      \n"
      "orr      z0.d, z0.d, z2.d                        \n"
      "orr      z1.d, z1.d, z2.d                        \n"
      "st1w     {z0.s}, p0, [%[dst]]                    \n"
      "st1w     {z1.s}, p1, [%[dst], #1, mul vl]        \n"

      "99:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [width] "+r"(width),   // %[width]
        [vl] "=&r"(vl)         // %[vl]
      : [value] "r"(value)     // %[value]
      : "memory", "cc", "z0", "z1", "z2", "p0", "p1");
}

#define ARGBALPHABLEND_SVE_2X                                     \
  "ld1w     {z0.s}, p1/z, [%[src0]]                           \n" \
  "ld1w     {z1.s}, p2/z, [%[src0], #1, mul vl]               \n" \
  "ld1w     {z2.s}, p1/z, [%[src1]]                           \n" \
  "ld1w     {z3.s}, p2/z, [%[src1], #1, mul vl]               \n" \
  "incb     %[src0], all, mul #2                              \n" \
  "incb     %[src1], all, mul #2                              \n" \
  "lsr      z4.s, z0.s, #24                                   \n" \
  "lsr      z5.s, z1.s, #24                                   \n" \
  "lsr      z6.s, z2.s, #24                                   \n" \
  "lsr      z7.s, z3.s, #24                                   \n" \
  "mov      z16.d, z24.d                                      \n" \
  "mov      z17.d, z24.d                                      \n" \
  "umlalb   z16.h, z6.b, z4.b                                 \n" \
  "umlalb   z17.h, z7.b, z5.b                                 \n" \
  "usra     z16.h, z16.h, #8                                  \n" \
  "usra     z17.h, z17.h, #8                                  \n" \
  "lsr      z16.h, z16.h, #8                                  \n" \
  "lsr      z17.h, z17.h, #8                                  \n" \
  "sub      z16.s, z4.s, z16.s                                \n" \
  "sub      z17.s, z5.s, z17.s                                \n" \
  "add      z16.s, z6.s, z16.s                                \n" \
  "add      z17.s, z7.s, z17.s                                \n" \
  "lsl      z18.s, z4.s, #8                                   \n" \
  "lsl      z19.s, z5.s, #8                                   \n" \
  "sub      z18.s, z18.s, z4.s                                \n" \
  "sub      z19.s, z19.s, z5.s                                \n" \
  "sub      z6.s, z6.s, #1                                    \n" \
  "sub      z7.s, z7.s, #1                                    \n" \
  "asr      z6.s, z6.s, #31                                   \n" \
  "asr      z7.s, z7.s, #31                                   \n" \
  "ucvtf    z18.s, p0/m, z18.s                                \n" \
  "ucvtf    z19.s, p0/m, z19.s                                \n" \
  "ucvtf    z20.s, p0/m, z16.s                                \n" \
  "ucvtf    z21.s, p0/m, z17.s                                \n" \
  "fadd     z18.s, p0/m, z18.s, #0.5                          \n" \
  "fadd     z19.s, p0/m, z19.s, #0.5                          \n" \
  "frecpe   z22.s, z20.s                                      \n" \
  "frecpe   z23.s, z21.s                                      \n" \
  "frecps   z20.s, z20.s, z22.s                               \n" \
  "frecps   z21.s, z21.s, z23.s                               \n" \
  "fmul     z22.s, z22.s, z20.s                               \n" \
  "fmul     z23.s, z23.s, z21.s                               \n" \
  "fmul     z18.s, z18.s, z22.s                               \n" \
  "fmul     z19.s, z19.s, z23.s                               \n" \
  "fcvtzu   z18.s, p0/m, z18.s                                \n" \
  "fcvtzu   z19.s, p0/m, z19.s                                \n" \
  "mul      z18.s, z18.s, z25.s                               \n" \
  "mul      z19.s, z19.s, z25.s                               \n" \
  "orr      z18.d, z18.d, z6.d                                \n" \
  "orr      z19.d, z19.d, z7.d                                \n" \
  "not      z20.b, p0/m, z18.b                                \n" \
  "not      z21.b, p0/m, z19.b                                \n" \
  "mov      z4.d, z24.d                                       \n" \
  "mov      z5.d, z24.d                                       \n" \
  "mov      z6.d, z24.d                                       \n" \
  "mov      z7.d, z24.d                                       \n" \
  "umlalb   z4.h, z2.b, z20.b                                 \n" \
  "umlalt   z5.h, z2.b, z20.b                                 \n" \
  "umlalb   z6.h, z3.b, z21.b                                 \n" \
  "umlalt   z7.h, z3.b, z21.b                                 \n" \
  "umlalb   z4.h, z0.b, z18.b                                 \n" \
  "umlalt   z5.h, z0.b, z18.b                                 \n" \
  "umlalb   z6.h, z1.b, z19.b                                 \n" \
  "umlalt   z7.h, z1.b, z19.b                                 \n" \
  "usra     z4.h, z4.h, #8                                    \n" \
  "usra     z5.h, z5.h, #8                                    \n" \
  "usra     z6.h, z6.h, #8                                    \n" \
  "usra     z7.h, z7.h, #8                                    \n" \
  "shrnb    z0.b, z4.h, #8                                    \n" \
  "shrnt    z0.b, z5.h, #8                                    \n" \
  "shrnb    z1.b, z6.h, #8                                    \n" \
  "shrnt    z1.b, z7.h, #8                                    \n" \
  "and      z0.d, z0.d, z26.d                                 \n" \
  "and      z1.d, z1.d, z26.d                                 \n" \
  "lsl      z16.s, z16.s, #24                                 \n" \
  "lsl      z17.s, z17.s, #24                                 \n" \
  "orr      z0.d, z0.d, z16.d                                 \n" \
  "orr      z1.d, z1.d, z17.d                                 \n" \
  "st1w     {z0.s}, p1, [%[dst]]                              \n" \
  "st1w     {z1.s}, p2, [%[dst], #1, mul vl]                  \n" \
  "incb     %[dst], all, mul #2                               \n"

static inline void ARGBAlphaBlendRow_SVE_SC(const uint8_t* src_argb0,
                                            const uint8_t* src_argb1,
                                            uint8_t* dst_argb,
                                            int width) STREAMING_COMPATIBLE {
  uint32_t vl;
  asm volatile(
      "cntw     %x[vl]                                  \n"
      "ptrue    p0.b                                    \n"
      "mov      z24.h, #1                               \n"
      "mov      z25.b, #1                               \n"
      "mov      z26.s, #0x00ffffff                      \n"
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt     2f                                      \n"

      "ptrue    p1.s                                    \n"
      "ptrue    p2.s                                    \n"
      "1:                                               \n"  //
      ARGBALPHABLEND_SVE_2X
      "subs     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq     99f                                     \n"

      "whilelt  p1.s, wzr, %w[width]                    \n"
      "whilelt  p2.s, %w[vl], %w[width]                 \n"  //
      ARGBALPHABLEND_SVE_2X

      "99:                                              \n"
      : [src0] "+r"(src_argb0),  // %[src0]
        [src1] "+r"(src_argb1),  // %[src1]
        [dst] "+r"(dst_argb),    // %[dst]
        [width] "+r"(width),     // %[width]
        [vl] "=&r"(vl)           // %[vl]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z16",
        "z17", "z18", "z19", "z20", "z21", "z22", "z23", "z24", "z25", "z26",
        "p0", "p1", "p2");
}

#define ARGBTORGB565_SVE                    \
  /* Inputs:                                \
   * z0: rrrrrxxxbbbbbxxx                   \
   * z1: xxxxxxxxggggggxx                   \
   * z3: 0000000000000011 (3, 0, 3, 0, ...) \
   * z4: 0000011111100000                   \
   */                                       \
  "lsr     z0.b, p0/m, z0.b, z3.b       \n" \
  "lsl     z1.h, z1.h, #3               \n" \
  "bsl     z1.d, z1.d, z0.d, z4.d       \n"

static inline void ARGBToRGB565Row_SVE_SC(const uint8_t* src_argb,
                                          uint8_t* dst_rgb,
                                          int width) STREAMING_COMPATIBLE {
  unsigned bsl_mask = 0x7e0;
  uint64_t vl;
  width *= 2;
  asm volatile(
      "mov     z3.h, #3                     \n"
      "dup     z4.h, %w[bsl_mask]           \n"

      "cntb    %[vl]                        \n"
      "subs    %w[width], %w[width], %w[vl] \n"
      "b.lt    2f                           \n"

      "ptrue   p0.b                         \n"
      "1:                                   \n"
      "ld2b    {z0.b, z1.b}, p0/z, [%[src]] \n"  // BR, GA
      "incb    %[src], all, mul #2          \n"
      "subs    %w[width], %w[width], %w[vl] \n"  //
      ARGBTORGB565_SVE
      "st1b    {z1.b}, p0, [%[dst]]         \n"
      "incb    %[dst]                       \n"
      "b.ge    1b                           \n"

      "2:                                   \n"
      "adds    %w[width], %w[width], %w[vl] \n"
      "b.eq    99f                          \n"

      "whilelt p0.b, wzr, %w[width]         \n"
      "ld2b    {z0.b, z1.b}, p0/z, [%[src]] \n"  // BR, GA
      ARGBTORGB565_SVE
      "st1b    {z1.b}, p0, [%[dst]]         \n"

      "99:                                  \n"
      : [src] "+r"(src_argb),     // %[src]
        [dst] "+r"(dst_rgb),      // %[dst]
        [width] "+r"(width),      // %[width]
        [vl] "=&r"(vl)            // %[vl]
      : [bsl_mask] "r"(bsl_mask)  // %[bsl_mask]
      : "memory", "cc", "z0", "z1", "z3", "z4", "p0");
}

static inline void ARGBToRGB565DitherRow_SVE_SC(const uint8_t* src_argb,
                                                uint8_t* dst_rgb,
                                                uint32_t dither4,
                                                int width)
    STREAMING_COMPATIBLE {
  unsigned bsl_mask = 0x7e0;
  uint64_t vl;
  width *= 2;
  asm volatile(
      "mov     z3.h, #3                     \n"
      "dup     z4.h, %w[bsl_mask]           \n"
      "dup     z2.s, %w[dither4]            \n"
      "zip1    z2.b, z2.b, z2.b             \n"

      "cntb    %[vl]                        \n"
      "subs    %w[width], %w[width], %w[vl] \n"
      "b.lt    2f                           \n"

      "ptrue   p0.b                         \n"
      "1:                                   \n"
      "ld2b    {z0.b, z1.b}, p0/z, [%[src]] \n"  // BR, GA
      "incb    %[src], all, mul #2          \n"
      "uqadd   z0.b, z0.b, z2.b             \n"
      "uqadd   z1.b, z1.b, z2.b             \n"
      "subs    %w[width], %w[width], %w[vl] \n"  //
      ARGBTORGB565_SVE
      "st1b    {z1.b}, p0, [%[dst]]         \n"
      "incb    %[dst]                       \n"
      "b.ge    1b                           \n"

      "2:                                   \n"
      "adds    %w[width], %w[width], %w[vl] \n"
      "b.eq    99f                          \n"

      "whilelt p0.b, wzr, %w[width]         \n"
      "ld2b    {z0.b, z1.b}, p0/z, [%[src]] \n"  // BR, GA
      "uqadd   z0.b, z0.b, z2.b             \n"
      "uqadd   z1.b, z1.b, z2.b             \n"  //
      ARGBTORGB565_SVE
      "st1b    {z1.b}, p0, [%[dst]]         \n"

      "99:                                  \n"
      : [src] "+r"(src_argb),      // %[src]
        [dst] "+r"(dst_rgb),       // %[dst]
        [width] "+r"(width),       // %[width]
        [vl] "=&r"(vl)             // %[vl]
      : [bsl_mask] "r"(bsl_mask),  // %[bsl_mask]
        [dither4] "r"(dither4)     // %[dither4]
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "p0");
}

#define ARGB1555TOARGB_SVE                                    \
  /* Input: z1/z3.h = arrrrrgggggbbbbb */                     \
  "lsl     z0.h, z1.h, #3          \n" /* rrrgggggbbbbb000 */ \
  "lsl     z2.h, z3.h, #3          \n" /* rrrgggggbbbbb000 */ \
  "asr     z1.h, z1.h, #7          \n" /* aaaaaaaarrrrrggg */ \
  "asr     z3.h, z3.h, #7          \n" /* aaaaaaaarrrrrggg */ \
  "lsl     z0.b, p0/m, z0.b, z4.b  \n" /* ggggg000bbbbb000 */ \
  "lsl     z2.b, p0/m, z2.b, z4.b  \n" /* ggggg000bbbbb000 */ \
  "sri     z1.b, z1.b, #5          \n" /* aaaaaaaarrrrrrrr */ \
  "sri     z3.b, z3.b, #5          \n" /* aaaaaaaarrrrrrrr */ \
  "sri     z0.b, z0.b, #5          \n" /* ggggggggbbbbbbbb */ \
  "sri     z2.b, z2.b, #5          \n" /* ggggggggbbbbbbbb */

static inline void ARGB1555ToARGBRow_SVE_SC(const uint8_t* src_argb1555,
                                            uint8_t* dst_argb,
                                            int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "mov     z4.h, #0x0300                           \n"
      "ptrue   p0.b                                    \n"

      "cnth    %x[vl]                                  \n"
      "subs    %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.lt    2f                                      \n"

      "1:                                              \n"
      "ld1h    {z1.h}, p0/z, [%[src]]                  \n"
      "ld1h    {z3.h}, p0/z, [%[src], #1, mul vl]      \n"
      "incb    %[src], all, mul #2                     \n"  //
      ARGB1555TOARGB_SVE
      "subs    %w[width], %w[width], %w[vl], lsl #1    \n"
      "st2h    {z0.h, z1.h}, p0, [%[dst]]              \n"
      "st2h    {z2.h, z3.h}, p0, [%[dst], #2, mul vl]  \n"
      "incb    %[dst], all, mul #4                     \n"
      "b.ge    1b                                      \n"

      "2:                                              \n"
      "adds    %w[width], %w[width], %w[vl], lsl #1    \n"
      "b.eq    99f                                     \n"

      "whilelt p1.h, wzr, %w[width]                    \n"
      "whilelt p2.h, %w[vl], %w[width]                 \n"
      "ld1h    {z1.h}, p1/z, [%[src]]                  \n"
      "ld1h    {z3.h}, p2/z, [%[src], #1, mul vl]      \n"  //
      ARGB1555TOARGB_SVE
      "st2h    {z0.h, z1.h}, p1, [%[dst]]              \n"
      "st2h    {z2.h, z3.h}, p2, [%[dst], #2, mul vl]  \n"

      "99:                                             \n"
      : [src] "+r"(src_argb1555),  // %[src]
        [dst] "+r"(dst_argb),      // %[dst]
        [width] "+r"(width),       // %[width]
        [vl] "=&r"(vl)             // %[vl]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "p0", "p1", "p2");
}

// clang-format off
#define AYUVTOUV_SVE(zU0, zV0, zU1, zV1)                   /* e.g. */          \
  "ld2h     {z0.h, z1.h}, p0/z, [%[src0]]              \n" /* VUVU.. YAYA.. */ \
  "ld2h     {z1.h, z2.h}, p1/z, [%[src0], #2, mul vl]  \n" /* VUVU.. YAYA.. */ \
  "ld2h     {z2.h, z3.h}, p0/z, [%[src1]]              \n" /* VUVU.. YAYA.. */ \
  "ld2h     {z3.h, z4.h}, p1/z, [%[src1], #2, mul vl]  \n" /* VUVU.. YAYA.. */ \
  "incb     %[src0], all, mul #4                       \n"                     \
  "incb     %[src1], all, mul #4                       \n"                     \
  "uaddlb   z4.h, z0.b, z2.b                           \n" /* V */             \
  "uaddlt   z5.h, z0.b, z2.b                           \n" /* U */             \
  "uaddlb   z6.h, z1.b, z3.b                           \n" /* V */             \
  "uaddlt   z7.h, z1.b, z3.b                           \n" /* U */             \
  "addp   " #zU0 ".h, p0/m, " #zU0 ".h, " #zV0 ".h     \n" /* UV */            \
  "addp   " #zU1 ".h, p1/m, " #zU1 ".h, " #zV1 ".h     \n" /* UV */            \
  "subs     %w[width], %w[width], %w[vl]               \n"                     \
  "urshr  " #zU0 ".h, p0/m, " #zU0 ".h, #2             \n" /* U0V0 */          \
  "urshr  " #zU1 ".h, p1/m, " #zU1 ".h, #2             \n" /* U0V0 */          \
  "st1b     {" #zU0 ".h}, p0, [%[dst]]                 \n"                     \
  "st1b     {" #zU1 ".h}, p1, [%[dst], #1, mul vl]     \n"                     \
  "incb     %[dst]                                     \n"
// clang-format on

// Filter 2 rows of AYUV UV's (444) into UV (420).
// AYUV is VUYA in memory.  UV for NV12 is UV order in memory.
static inline void AYUVToUVRow_SVE_SC(const uint8_t* src_ayuv,
                                      int src_stride_ayuv,
                                      uint8_t* dst_uv,
                                      int width) STREAMING_COMPATIBLE {
  // Output a row of UV values, filtering 2x2 rows of AYUV.
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  if (width & 1) {
    width &= ~1;
    const uint8_t* last0 = src_ayuv + (ptrdiff_t)width * 4;
    const uint8_t* last1 = src_ayuv1 + (ptrdiff_t)width * 4;
    dst_uv[width] = (last0[1] + last1[1] + 1) >> 1;
    dst_uv[width + 1] = (last0[0] + last1[0] + 1) >> 1;
    if (width == 0) {
      return;
    }
  }
  int vl;
  asm volatile(
      "cntb    %x[vl]                            \n"
      "subs    %w[width], %w[width], %w[vl]      \n"
      "b.lt    2f                                \n"

      "ptrue   p0.h                              \n"
      "ptrue   p1.h                              \n"
      "1:                                        \n"
      AYUVTOUV_SVE(z5, z4, z7, z6)
      "b.ge    1b                                \n"

      "2:                                        \n"
      "adds    %w[width], %w[width], %w[vl]      \n"
      "b.eq    99f                               \n"

      "cnth    %x[vl]                            \n"
      "whilelt p0.h, wzr, %w[width]              \n"  // first row
      "whilelt p1.h, %w[vl], %w[width]           \n"  // second row
      AYUVTOUV_SVE(z5, z4, z7, z6)

      "99:                                       \n"
      : [src0] "+r"(src_ayuv),   // %[src0]
        [src1] "+r"(src_ayuv1),  // %[src1]
        [dst] "+r"(dst_uv),      // %[dst]
        [width] "+r"(width),     // %[width]
        [vl] "=&r"(vl)           // %[vl]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "p0",
        "p1");
}

// Filter 2 rows of AYUV UV's (444) into VU (420).
static inline void AYUVToVURow_SVE_SC(const uint8_t* src_ayuv,
                                      int src_stride_ayuv,
                                      uint8_t* dst_vu,
                                      int width) STREAMING_COMPATIBLE {
  // Output a row of VU values, filtering 2x2 rows of AYUV.
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  if (width & 1) {
    width &= ~1;
    const uint8_t* last0 = src_ayuv + (ptrdiff_t)width * 4;
    const uint8_t* last1 = src_ayuv1 + (ptrdiff_t)width * 4;
    dst_vu[width] = (last0[0] + last1[0] + 1) >> 1;
    dst_vu[width + 1] = (last0[1] + last1[1] + 1) >> 1;
    if (width == 0) {
      return;
    }
  }
  int vl;
  asm volatile(
      "cntb    %x[vl]                            \n"
      "subs    %w[width], %w[width], %w[vl]      \n"
      "b.lt    2f                                \n"

      "ptrue   p0.h                              \n"
      "ptrue   p1.h                              \n"
      "1:                                        \n"
      AYUVTOUV_SVE(z4, z5, z6, z7)
      "b.ge    1b                                \n"

      "2:                                        \n"
      "adds    %w[width], %w[width], %w[vl]      \n"
      "b.eq    99f                               \n"

      "cnth    %x[vl]                            \n"
      "whilelt p0.h, wzr, %w[width]              \n"  // first row
      "whilelt p1.h, %w[vl], %w[width]           \n"  // second row
      AYUVTOUV_SVE(z4, z5, z6, z7)

      "99:                                       \n"
      : [src0] "+r"(src_ayuv),   // %[src0]
        [src1] "+r"(src_ayuv1),  // %[src1]
        [dst] "+r"(dst_vu),      // %[dst]
        [width] "+r"(width),     // %[width]
        [vl] "=&r"(vl)           // %[vl]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "p0",
        "p1");
}

static inline void RAWToWXYZRow_SVE_SC(const uint8_t* src_raw,
                                       uint8_t* dst_wxyz,
                                       int width,
                                       uint32_t idx_start,
                                       uint32_t idx_step,
                                       uint32_t alpha) STREAMING_COMPATIBLE {
  uint64_t vl;
  uint64_t vl_mul3;
  uint32_t rem_mul3;
  asm volatile(
      "cntw    %x[vl]                                     \n"
      "add     %w[vl_mul3], %w[vl], %w[vl], lsl #1        \n"
      "index   z31.s, %w[idx_start], %w[idx_step]         \n"
      "dup     z30.s, %w[alpha]                           \n"
      "subs    %w[width], %w[width], %w[vl], lsl #1       \n"
      "b.lt    2f                                         \n"

      // Run bulk of computation with the same predicates to avoid predicate
      // generation overhead. We set up p1 to only load 3/4 of a vector.
      "ptrue   p0.s                                       \n"
      "whilelt p1.b, wzr, %w[vl_mul3]                     \n"
      "1:                                                 \n"
      "ld1b    {z0.b}, p1/z, [%[src]]                     \n"
      "add     %[src], %[src], %x[vl_mul3]                \n"
      "ld1b    {z1.b}, p1/z, [%[src]]                     \n"
      "add     %[src], %[src], %x[vl_mul3]                \n"
      "tbl     z0.b, {z0.b}, z31.b                        \n"
      "tbl     z1.b, {z1.b}, z31.b                        \n"
      "subs    %w[width], %w[width], %w[vl], lsl #1       \n"
      "orr     z0.d, z0.d, z30.d                          \n"
      "orr     z1.d, z1.d, z30.d                          \n"
      "st1w    {z0.s}, p0, [%[dst]]                       \n"
      "st1w    {z1.s}, p0, [%[dst], #1, mul vl]           \n"
      "incb    %[dst], all, mul #2                        \n"
      "b.ge    1b                                         \n"

      "2:                                                 \n"
      "adds    %w[width], %w[width], %w[vl], lsl #1       \n"
      "b.eq    99f                                        \n"

      // Calculate a pair of predicates for the final iteration to deal with
      // the tail.
      "3:                                                 \n"
      "add     %w[rem_mul3], %w[width], %w[width], lsl #1 \n"
      "whilelt p0.s, wzr, %w[width]                       \n"
      "whilelt p1.b, wzr, %w[rem_mul3]                    \n"
      "ld1b    {z0.b}, p1/z, [%[src]]                     \n"
      "add     %[src], %[src], %x[vl_mul3]                \n"
      "tbl     z0.b, {z0.b}, z31.b                        \n"
      "subs    %w[width], %w[width], %w[vl]               \n"
      "orr     z0.d, z0.d, z30.d                          \n"
      "st1w    {z0.s}, p0, [%[dst]]                       \n"
      "incb    %[dst]                                     \n"
      "b.gt    3b                                         \n"

      "99:                                                \n"
      : [src] "+r"(src_raw),         // %[src]
        [dst] "+r"(dst_wxyz),        // %[dst]
        [width] "+r"(width),         // %[width]
        [vl] "=&r"(vl),              // %[vl]
        [vl_mul3] "=&r"(vl_mul3),    // %[vl_mul3]
        [rem_mul3] "=&r"(rem_mul3)   // %[rem_mul3]
      : [idx_start] "r"(idx_start),  // %[idx_start]
        [idx_step] "r"(idx_step),    // %[idx_step]
        [alpha] "r"(alpha)           // %[alpha]
      : "memory", "cc", "z0", "z1", "z30", "z31", "p0", "p1");
}

static inline void RAWToARGBRow_SVE_SC(const uint8_t* src_raw,
                                       uint8_t* dst_argb,
                                       int width) STREAMING_COMPATIBLE {
  RAWToWXYZRow_SVE_SC(src_raw, dst_argb, width, 0xff000102U, 0x00030303U,
                      0xff000000U);
}

static inline void RAWToRGBARow_SVE_SC(const uint8_t* src_raw,
                                       uint8_t* dst_rgba,
                                       int width) STREAMING_COMPATIBLE {
  RAWToWXYZRow_SVE_SC(src_raw, dst_rgba, width, 0x000102ffU, 0x03030300U,
                      0x000000ffU);
}

static inline void RGB24ToARGBRow_SVE_SC(const uint8_t* src_rgb24,
                                         uint8_t* dst_argb,
                                         int width) STREAMING_COMPATIBLE {
  RAWToWXYZRow_SVE_SC(src_rgb24, dst_argb, width, 0xff020100U, 0x00030303U,
                      0xff000000U);
}

static const uint8_t kRAWToRGB24Indices[] = {
    2,   1,   0,   5,   4,   3,   8,   7,   6,   11,  10,  9,   14,  13,  12,
    17,  16,  15,  20,  19,  18,  23,  22,  21,  26,  25,  24,  29,  28,  27,
    32,  31,  30,  35,  34,  33,  38,  37,  36,  41,  40,  39,  44,  43,  42,
    47,  46,  45,  50,  49,  48,  53,  52,  51,  56,  55,  54,  59,  58,  57,
    62,  61,  60,  65,  64,  63,  68,  67,  66,  71,  70,  69,  74,  73,  72,
    77,  76,  75,  80,  79,  78,  83,  82,  81,  86,  85,  84,  89,  88,  87,
    92,  91,  90,  95,  94,  93,  98,  97,  96,  101, 100, 99,  104, 103, 102,
    107, 106, 105, 110, 109, 108, 113, 112, 111, 116, 115, 114, 119, 118, 117,
    122, 121, 120, 125, 124, 123, 128, 127, 126, 131, 130, 129, 134, 133, 132,
    137, 136, 135, 140, 139, 138, 143, 142, 141, 146, 145, 144, 149, 148, 147,
    152, 151, 150, 155, 154, 153, 158, 157, 156, 161, 160, 159, 164, 163, 162,
    167, 166, 165, 170, 169, 168, 173, 172, 171, 176, 175, 174, 179, 178, 177,
    182, 181, 180, 185, 184, 183, 188, 187, 186, 191, 190, 189, 194, 193, 192,
    197, 196, 195, 200, 199, 198, 203, 202, 201, 206, 205, 204, 209, 208, 207,
    212, 211, 210, 215, 214, 213, 218, 217, 216, 221, 220, 219, 224, 223, 222,
    227, 226, 225, 230, 229, 228, 233, 232, 231, 236, 235, 234, 239, 238, 237,
    242, 241, 240, 245, 244, 243, 248, 247, 246, 251, 250, 249, 254, 253, 252};

static inline void RAWToRGB24Row_SVE_SC(const uint8_t* src_raw,
                                        uint8_t* dst_rgb24,
                                        int width) STREAMING_COMPATIBLE {
  // width is in elements, convert to bytes.
  width *= 3;
  // we use the mul3 predicate pattern throughout to use the largest multiple
  // of three number of lanes, for instance with a vector length of 16 bytes
  // only the first 15 bytes will be used for load/store instructions.
  uint32_t vl;
  asm volatile(
      "cntb    %x[vl], mul3                              \n"
      "ptrue   p0.b, mul3                                \n"
      "ld1b    {z31.b}, p0/z, [%[kIndices]]              \n"
      "subs    %w[width], %w[width], %w[vl]              \n"
      "b.lt    2f                                        \n"

      // Run bulk of computation with the same predicate to avoid predicate
      // generation overhead.
      "1:                                                \n"
      "ld1b    {z0.b}, p0/z, [%[src]]                    \n"
      "add     %[src], %[src], %x[vl]                    \n"
      "tbl     z0.b, {z0.b}, z31.b                       \n"
      "subs    %w[width], %w[width], %w[vl]              \n"
      "st1b    {z0.b}, p0, [%[dst]]                      \n"
      "add     %[dst], %[dst], %x[vl]                    \n"
      "b.ge    1b                                        \n"

      "2:                                                \n"
      "adds    %w[width], %w[width], %w[vl]              \n"
      "b.eq    99f                                       \n"

      // Calculate a predicate for the final iteration to deal with the tail.
      "whilelt p0.b, wzr, %w[width]                      \n"
      "ld1b    {z0.b}, p0/z, [%[src]]                    \n"
      "tbl     z0.b, {z0.b}, z31.b                       \n"
      "st1b    {z0.b}, p0, [%[dst]]                      \n"

      "99:                                               \n"
      : [src] "+r"(src_raw),                // %[src]
        [dst] "+r"(dst_rgb24),              // %[dst]
        [width] "+r"(width),                // %[width]
        [vl] "=&r"(vl)                      // %[vl]
      : [kIndices] "r"(kRAWToRGB24Indices)  // %[kIndices]
      : "memory", "cc", "z0", "z31", "p0");
}

static inline void ARGBToXYZRow_SVE_SC(const uint8_t* src_argb,
                                       uint8_t* dst_xyz,
                                       int width,
                                       const uint8_t* indices)
    STREAMING_COMPATIBLE {
  uint64_t vl;
  uint64_t vl_mul3;
  uint32_t rem_mul3;
  asm volatile(
      "cntw    %x[vl]                                     \n"
      "add     %w[vl_mul3], %w[vl], %w[vl], lsl #1        \n"
      "whilelt p1.b, wzr, %w[vl_mul3]                     \n"
      "ld1b    {z31.b}, p1/z, [%[indices]]                \n"
      "subs    %w[width], %w[width], %w[vl_mul3]          \n"
      "b.lt    2f                                         \n"

      // Run bulk of computation with an all-true predicate to avoid partial
      // predicated store overhead. Ensure at least vl elements remain for the
      // tail so full-vector stores never write past dst_xyz + width * 3.
      "ptrue   p0.b                                       \n"
      "1:                                                 \n"
      "ld1w    {z0.s}, p0/z, [%[src]]                     \n"
      "ld1w    {z1.s}, p0/z, [%[src], #1, mul vl]         \n"
      "incb    %[src], all, mul #2                        \n"
      "tbl     z0.b, {z0.b}, z31.b                        \n"
      "tbl     z1.b, {z1.b}, z31.b                        \n"
      "subs    %w[width], %w[width], %w[vl], lsl #1       \n"
      "st1b    {z0.b}, p0, [%[dst]]                       \n"
      "add     %[dst], %[dst], %x[vl_mul3]                \n"
      "st1b    {z1.b}, p0, [%[dst]]                       \n"
      "add     %[dst], %[dst], %x[vl_mul3]                \n"
      "b.ge    1b                                         \n"

      "2:                                                 \n"
      "adds    %w[width], %w[width], %w[vl_mul3]          \n"
      "b.eq    99f                                        \n"

      // Calculate predicates for the remaining iterations to deal with the
      // tail.
      "3:                                                 \n"
      "add     %w[rem_mul3], %w[width], %w[width], lsl #1 \n"
      "whilelt p0.s, wzr, %w[width]                       \n"
      "whilelt p1.b, wzr, %w[rem_mul3]                    \n"
      "ld1w    {z0.s}, p0/z, [%[src]]                     \n"
      "incb    %[src]                                     \n"
      "tbl     z0.b, {z0.b}, z31.b                        \n"
      "subs    %w[width], %w[width], %w[vl]               \n"
      "st1b    {z0.b}, p1, [%[dst]]                       \n"
      "add     %[dst], %[dst], %x[vl_mul3]                \n"
      "b.gt    3b                                         \n"

      "99:                                                \n"
      : [src] "+r"(src_argb),       // %[src]
        [dst] "+r"(dst_xyz),        // %[dst]
        [width] "+r"(width),        // %[width]
        [vl] "=&r"(vl),             // %[vl]
        [vl_mul3] "=&r"(vl_mul3),   // %[vl_mul3]
        [rem_mul3] "=&r"(rem_mul3)  // %[rem_mul3]
      : [indices] "r"(indices)      // %[indices]
      : "memory", "cc", "z0", "z1", "z31", "p0", "p1");
}

static const uint8_t kARGBToRGB24RowIndices[] = {
    0,   1,   2,   4,   5,   6,   8,   9,   10,  12,  13,  14,  16,  17,  18,
    20,  21,  22,  24,  25,  26,  28,  29,  30,  32,  33,  34,  36,  37,  38,
    40,  41,  42,  44,  45,  46,  48,  49,  50,  52,  53,  54,  56,  57,  58,
    60,  61,  62,  64,  65,  66,  68,  69,  70,  72,  73,  74,  76,  77,  78,
    80,  81,  82,  84,  85,  86,  88,  89,  90,  92,  93,  94,  96,  97,  98,
    100, 101, 102, 104, 105, 106, 108, 109, 110, 112, 113, 114, 116, 117, 118,
    120, 121, 122, 124, 125, 126, 128, 129, 130, 132, 133, 134, 136, 137, 138,
    140, 141, 142, 144, 145, 146, 148, 149, 150, 152, 153, 154, 156, 157, 158,
    160, 161, 162, 164, 165, 166, 168, 169, 170, 172, 173, 174, 176, 177, 178,
    180, 181, 182, 184, 185, 186, 188, 189, 190, 192, 193, 194, 196, 197, 198,
    200, 201, 202, 204, 205, 206, 208, 209, 210, 212, 213, 214, 216, 217, 218,
    220, 221, 222, 224, 225, 226, 228, 229, 230, 232, 233, 234, 236, 237, 238,
    240, 241, 242, 244, 245, 246, 248, 249, 250, 252, 253, 254,
};

static const uint8_t kARGBToRAWRowIndices[] = {
    2,   1,   0,   6,   5,   4,   10,  9,   8,   14,  13,  12,  18,  17,  16,
    22,  21,  20,  26,  25,  24,  30,  29,  28,  34,  33,  32,  38,  37,  36,
    42,  41,  40,  46,  45,  44,  50,  49,  48,  54,  53,  52,  58,  57,  56,
    62,  61,  60,  66,  65,  64,  70,  69,  68,  74,  73,  72,  78,  77,  76,
    82,  81,  80,  86,  85,  84,  90,  89,  88,  94,  93,  92,  98,  97,  96,
    102, 101, 100, 106, 105, 104, 110, 109, 108, 114, 113, 112, 118, 117, 116,
    122, 121, 120, 126, 125, 124, 130, 129, 128, 134, 133, 132, 138, 137, 136,
    142, 141, 140, 146, 145, 144, 150, 149, 148, 154, 153, 152, 158, 157, 156,
    162, 161, 160, 166, 165, 164, 170, 169, 168, 174, 173, 172, 178, 177, 176,
    182, 181, 180, 186, 185, 184, 190, 189, 188, 194, 193, 192, 198, 197, 196,
    202, 201, 200, 206, 205, 204, 210, 209, 208, 214, 213, 212, 218, 217, 216,
    222, 221, 220, 226, 225, 224, 230, 229, 228, 234, 233, 232, 238, 237, 236,
    242, 241, 240, 246, 245, 244, 250, 249, 248, 254, 253, 252,
};

static inline void ARGBToRGB24Row_SVE_SC(const uint8_t* src_argb,
                                         uint8_t* dst_rgb,
                                         int width) STREAMING_COMPATIBLE {
  ARGBToXYZRow_SVE_SC(src_argb, dst_rgb, width, kARGBToRGB24RowIndices);
}

static inline void ARGBToRAWRow_SVE_SC(const uint8_t* src_argb,
                                       uint8_t* dst_rgb,
                                       int width) STREAMING_COMPATIBLE {
  ARGBToXYZRow_SVE_SC(src_argb, dst_rgb, width, kARGBToRAWRowIndices);
}

static inline void CopyRow_SVE_SC(const uint8_t* src,
                                  uint8_t* dst,
                                  int width) STREAMING_COMPATIBLE {
  int vl;
  int vl2;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "lsl      %w[vl2], %w[vl], #1                     \n"
      "subs     %w[width], %w[width], %w[vl2]           \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "1:                                               \n"
      "ld1b     {z0.b}, p0/z, [%[src], #0, mul vl]      \n"
      "ld1b     {z1.b}, p0/z, [%[src], #1, mul vl]      \n"
      "incb     %[src], all, mul #2                     \n"
      "subs     %w[width], %w[width], %w[vl2]           \n"
      "st1b     {z0.b}, p0, [%[dst], #0, mul vl]        \n"
      "st1b     {z1.b}, p0, [%[dst], #1, mul vl]        \n"
      "incb     %[dst], all, mul #2                     \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl2]           \n"
      "b.eq     99f                                     \n"

      "3:                                               \n"
      "whilelt  p0.b, wzr, %w[width]                    \n"
      "ld1b     {z0.b}, p0/z, [%[src]]                  \n"
      "incb     %[src]                                  \n"
      "st1b     {z0.b}, p0, [%[dst]]                    \n"
      "incb     %[dst]                                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.gt     3b                                      \n"

      "99:                                              \n"
      : [src] "+r"(src),      // %[src]
        [dst] "+r"(dst),      // %[dst]
        [width] "+r"(width),  // %[width]
        [vl] "=&r"(vl),       // %[vl]
        [vl2] "=&r"(vl2)      // %[vl2]
      :
      : "memory", "cc", "z0", "z1", "p0");
}

static inline void DivideRow_16_SVE_SC(const uint16_t* src_y,
                                       uint16_t* dst_y,
                                       int scale,
                                       int width) STREAMING_COMPATIBLE {
  if (scale == 65536) {  // Divide by 1, so do copy.
    if (src_y != dst_y) {
      CopyRow_SVE_SC((const uint8_t*)src_y, (uint8_t*)dst_y, width * 2);
    }
    return;
  }
  uint64_t vl;
  asm volatile(
      "cnth   %x[vl]                                     \n"
      "dup    z0.h, %w[scale]                            \n"
      "subs   %w[width], %w[width], %w[vl], lsl #1       \n"
      "b.lt    2f                                        \n"

      // Run bulk of computation with the same predicates to avoid predicate
      // generation overhead.
      "ptrue   p0.h                                      \n"
      "1:                                                \n"
      "ld1h   {z1.h}, p0/z, [%[src]]                     \n"
      "ld1h   {z2.h}, p0/z, [%[src], #1, mul vl]         \n"
      "incb   %[src], all, mul #2                        \n"
      "umulh  z1.h, z1.h, z0.h                           \n"
      "umulh  z2.h, z2.h, z0.h                           \n"
      "subs   %w[width], %w[width], %w[vl], lsl #1       \n"
      "st1h   {z1.h}, p0, [%[dst]]                       \n"
      "st1h   {z2.h}, p0, [%[dst], #1, mul vl]           \n"
      "incb   %[dst], all, mul #2                        \n"
      "b.ge    1b                                        \n"

      "2:                                                \n"
      "adds     %w[width], %w[width], %w[vl], lsl #1     \n"
      "b.eq     99f                                      \n"

      // Calculate a pair of predicates for the final iteration to deal with
      // the tail.
      "whilelt p0.h, wzr, %w[width]                      \n"
      "whilelt p1.h, %w[vl], %w[width]                   \n"
      "ld1h   {z1.h}, p0/z, [%[src]]                     \n"
      "ld1h   {z2.h}, p1/z, [%[src], #1, mul vl]         \n"
      "umulh  z1.h, z1.h, z0.h                           \n"
      "umulh  z2.h, z2.h, z0.h                           \n"
      "st1h   {z1.h}, p0, [%[dst]]                       \n"
      "st1h   {z2.h}, p1, [%[dst], #1, mul vl]           \n"

      "99:                                               \n"
      : [src] "+r"(src_y),    // %[src]
        [dst] "+r"(dst_y),    // %[dst]
        [width] "+r"(width),  // %[width]
        [vl] "=&r"(vl)        // %[vl]
      : [scale] "r"(scale)    // %[scale]
      : "memory", "cc", "z0", "z1", "z2", "p0", "p1");
}

#define HALFFLOAT_SVE                                    \
  "lsr         z1.s, z0.s, #16                       \n" \
  "and         z0.s, z0.s, #0xffff                   \n" \
  "lsr         z3.s, z2.s, #16                       \n" \
  "and         z2.s, z2.s, #0xffff                   \n" \
  "scvtf       z0.s, p2/m, z0.s                      \n" \
  "scvtf       z1.s, p2/m, z1.s                      \n" \
  "scvtf       z2.s, p2/m, z2.s                      \n" \
  "scvtf       z3.s, p2/m, z3.s                      \n" \
  "fmul        z0.s, z0.s, z4.s                      \n" \
  "fmul        z1.s, z1.s, z4.s                      \n" \
  "fmul        z2.s, z2.s, z4.s                      \n" \
  "fmul        z3.s, z3.s, z4.s                      \n" \
  "fmul        z0.s, z0.s, z5.s                      \n" \
  "fmul        z1.s, z1.s, z5.s                      \n" \
  "fmul        z2.s, z2.s, z5.s                      \n" \
  "fmul        z3.s, z3.s, z5.s                      \n" \
  "uqshrnb     z0.h, z0.s, #13                       \n" \
  "uqshrnb     z2.h, z2.s, #13                       \n" \
  "uqshrnt     z0.h, z1.s, #13                       \n" \
  "uqshrnt     z2.h, z3.s, #13                       \n"

static inline void HalfFloatRow_SVE_SC(const uint16_t* src,
                                       uint16_t* dst,
                                       float scale,
                                       int width) STREAMING_COMPATIBLE {
  uint64_t vl;
  asm volatile(
      "cnth        %x[vl]                                \n"
      "mov         z4.s, %s[scale]                       \n"
      "mov         z5.s, %s[bias]                        \n"
      "ptrue       p2.s                                  \n"
      "subs        %w[width], %w[width], %w[vl], lsl #1  \n"
      "b.lt        2f                                    \n"

      // Run bulk of computation with all-true predicates to avoid predicate
      // generation overhead.
      "ptrue       p0.h                                  \n"
      "1:                                                \n"
      "ld1h        {z0.h}, p0/z, [%[src]]                \n"
      "ld1h        {z2.h}, p0/z, [%[src], #1, mul vl]    \n"
      "incb        %[src], all, mul #2                   \n"  //
      HALFFLOAT_SVE
      "subs        %w[width], %w[width], %w[vl], lsl #1  \n"
      "st1h        {z0.h}, p0, [%[dst]]                  \n"
      "st1h        {z2.h}, p0, [%[dst], #1, mul vl]      \n"
      "incb        %[dst], all, mul #2                   \n"
      "b.ge        1b                                    \n"

      "2:                                                \n"
      "adds        %w[width], %w[width], %w[vl], lsl #1  \n"
      "b.eq        99f                                   \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt     p0.h, wzr, %w[width]                  \n"
      "whilelt     p1.h, %w[vl], %w[width]               \n"
      "ld1h        {z0.h}, p0/z, [%[src]]                \n"
      "ld1h        {z2.h}, p1/z, [%[src], #1, mul vl]    \n"  //
      HALFFLOAT_SVE
      "st1h        {z0.h}, p0, [%[dst]]                  \n"
      "st1h        {z2.h}, p1, [%[dst], #1, mul vl]      \n"

      "99:                                               \n"
      : [src] "+r"(src),               // %[src]
        [dst] "+r"(dst),               // %[dst]
        [width] "+r"(width),           // %[width]
        [vl] "=&r"(vl)                 // %[vl]
      : [scale] "w"(scale),            // %[scale]
        [bias] "w"(1.9259299444e-34f)  // %[bias] 2^-112
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "p0", "p1", "p2");
}

static inline void HalfFloat1Row_SVE_SC(const uint16_t* src,
                                        uint16_t* dst,
                                        float scale,
                                        int width) STREAMING_COMPATIBLE {
  (void)scale;
  uint64_t vl;
  asm volatile(
      "cnth        %x[vl]                                \n"
      "subs        %w[width], %w[width], %w[vl], lsl #1  \n"
      "b.lt        2f                                    \n"

      // Run bulk of computation with all-true predicates to avoid predicate
      // generation overhead.
      "ptrue       p0.h                                  \n"
      "1:                                                \n"
      "ld1h        {z0.h}, p0/z, [%[src]]                \n"
      "ld1h        {z1.h}, p0/z, [%[src], #1, mul vl]    \n"
      "incb        %[src], all, mul #2                   \n"
      "ucvtf       z0.h, p0/m, z0.h                      \n"
      "ucvtf       z1.h, p0/m, z1.h                      \n"
      "subs        %w[width], %w[width], %w[vl], lsl #1  \n"
      "st1h        {z0.h}, p0, [%[dst]]                  \n"
      "st1h        {z1.h}, p0, [%[dst], #1, mul vl]      \n"
      "incb        %[dst], all, mul #2                   \n"
      "b.ge        1b                                    \n"

      "2:                                                \n"
      "adds        %w[width], %w[width], %w[vl], lsl #1  \n"
      "b.eq        99f                                   \n"

      // Calculate predicates for the final iteration to deal with the tail.
      "whilelt     p0.h, wzr, %w[width]                  \n"
      "whilelt     p1.h, %w[vl], %w[width]               \n"
      "ld1h        {z0.h}, p0/z, [%[src]]                \n"
      "ld1h        {z1.h}, p1/z, [%[src], #1, mul vl]    \n"
      "ucvtf       z0.h, p0/m, z0.h                      \n"
      "ucvtf       z1.h, p0/m, z1.h                      \n"
      "st1h        {z0.h}, p0, [%[dst]]                  \n"
      "st1h        {z1.h}, p1, [%[dst], #1, mul vl]      \n"

      "99:                                               \n"
      : [src] "+r"(src),      // %[src]
        [dst] "+r"(dst),      // %[dst]
        [width] "+r"(width),  // %[width]
        [vl] "=&r"(vl)        // %[vl]
      :
      : "memory", "cc", "z0", "z1", "p0", "p1");
}

static inline void MergeUVRow_SVE_SC(const uint8_t* src_u,
                                     const uint8_t* src_v,
                                     uint8_t* dst_uv,
                                     int width) STREAMING_COMPATIBLE {
  int vl;
  asm volatile(
      "cntb    %x[vl]                                   \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "b.lt    2f                                       \n"

      "ptrue   p0.b                                     \n"
      "1:                                               \n"
      "ld1b    {z1.b}, p0/z, [%[src_u]]                 \n"
      "ld1b    {z2.b}, p0/z, [%[src_v]]                 \n"
      "incb    %[src_u]                                 \n"
      "incb    %[src_v]                                 \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "st2b    {z1.b, z2.b}, p0, [%[dst_uv]]            \n"
      "incb    %[dst_uv], all, mul #2                   \n"
      "b.ge    1b                                       \n"

      "2:                                               \n"
      "adds    %w[width], %w[width], %w[vl]             \n"
      "b.eq    99f                                      \n"

      "whilelt p0.b, wzr, %w[width]                     \n"
      "ld1b    {z1.b}, p0/z, [%[src_u]]                 \n"
      "ld1b    {z2.b}, p0/z, [%[src_v]]                 \n"
      "st2b    {z1.b, z2.b}, p0, [%[dst_uv]]            \n"

      "99:                                              \n"
      : [src_u] "+r"(src_u),    // %[src_u]
        [src_v] "+r"(src_v),    // %[src_v]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [width] "+r"(width),    // %[width]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", "cc", "z1", "z2", "p0");
}

static inline void SplitUVRow_SVE_SC(const uint8_t* src_uv,
                                     uint8_t* dst_u,
                                     uint8_t* dst_v,
                                     int width) STREAMING_COMPATIBLE {
  int vl;
  asm volatile(
      "cntb    %x[vl]                                   \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "b.lt    2f                                       \n"

      "ptrue   p0.b                                     \n"
      "1:                                               \n"
      "ld2b    {z1.b, z2.b}, p0/z, [%[src_uv]]          \n"
      "incb    %[src_uv], all, mul #2                   \n"
      "subs    %w[width], %w[width], %w[vl]             \n"
      "st1b    {z1.b}, p0, [%[dst_u]]                   \n"
      "st1b    {z2.b}, p0, [%[dst_v]]                   \n"
      "incb    %[dst_u]                                 \n"
      "incb    %[dst_v]                                 \n"
      "b.ge    1b                                       \n"

      "2:                                               \n"
      "adds    %w[width], %w[width], %w[vl]             \n"
      "b.eq    99f                                      \n"

      "whilelt p0.b, wzr, %w[width]                     \n"
      "ld2b    {z1.b, z2.b}, p0/z, [%[src_uv]]          \n"
      "st1b    {z1.b}, p0, [%[dst_u]]                   \n"
      "st1b    {z2.b}, p0, [%[dst_v]]                   \n"

      "99:                                              \n"
      : [src_uv] "+r"(src_uv),  // %[src_uv]
        [dst_u] "+r"(dst_u),    // %[dst_u]
        [dst_v] "+r"(dst_v),    // %[dst_v]
        [width] "+r"(width),    // %[width]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", "cc", "z1", "z2", "p0");
}

static inline void HalfMergeUVRow_SVE_SC(const uint8_t* src_u,
                                         int src_stride_u,
                                         const uint8_t* src_v,
                                         int src_stride_v,
                                         uint8_t* dst_uv,
                                         int width) STREAMING_COMPATIBLE {
  const uint8_t* src_u1 = src_u + src_stride_u;
  const uint8_t* src_v1 = src_v + src_stride_v;
  int vl;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "1:                                               \n"
      "ld1b     {z1.b}, p0/z, [%[src_u]]                \n"
      "ld1b     {z2.b}, p0/z, [%[src_u1]]               \n"
      "ld1b     {z3.b}, p0/z, [%[src_v]]                \n"
      "ld1b     {z4.b}, p0/z, [%[src_v1]]               \n"
      "incb     %[src_u]                                \n"
      "incb     %[src_u1]                               \n"
      "incb     %[src_v]                                \n"
      "incb     %[src_v1]                               \n"
      "uaddlb   z5.h, z1.b, z2.b                        \n"
      "uaddlb   z6.h, z3.b, z4.b                        \n"
      "uaddlt   z1.h, z1.b, z2.b                        \n"
      "uaddlt   z3.h, z3.b, z4.b                        \n"
      "add      z1.h, z1.h, z5.h                        \n"
      "add      z3.h, z3.h, z6.h                        \n"
      "rshrnb   z1.b, z1.h, #2                          \n"
      "rshrnt   z1.b, z3.h, #2                          \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1b     {z1.b}, p0, [%[dst_uv]]                 \n"
      "incb     %[dst_uv]                               \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.b, wzr, %w[width]                    \n"
      "ld1b     {z1.b}, p0/z, [%[src_u]]                \n"
      "ld1b     {z2.b}, p0/z, [%[src_u1]]               \n"
      "ld1b     {z3.b}, p0/z, [%[src_v]]                \n"
      "ld1b     {z4.b}, p0/z, [%[src_v1]]               \n"
      "uaddlb   z5.h, z1.b, z2.b                        \n"
      "uaddlb   z6.h, z3.b, z4.b                        \n"
      "uaddlt   z1.h, z1.b, z2.b                        \n"
      "uaddlt   z3.h, z3.b, z4.b                        \n"
      "add      z1.h, z1.h, z5.h                        \n"
      "add      z3.h, z3.h, z6.h                        \n"
      "rshrnb   z1.b, z1.h, #2                          \n"
      "rshrnt   z1.b, z3.h, #2                          \n"
      "st1b     {z1.b}, p0, [%[dst_uv]]                 \n"

      "99:                                              \n"
      : [src_u] "+r"(src_u),    // %[src_u]
        [src_u1] "+r"(src_u1),  // %[src_u1]
        [src_v] "+r"(src_v),    // %[src_v]
        [src_v1] "+r"(src_v1),  // %[src_v1]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [width] "+r"(width),    // %[width]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", "cc", "z1", "z2", "z3", "z4", "z5", "z6", "p0");
}

static inline void HalfUVRow_SVE_SC(const uint8_t* src_u,
                                    ptrdiff_t src_stride_u,
                                    const uint8_t* src_v,
                                    ptrdiff_t src_stride_v,
                                    uint8_t* dst_uv,
                                    int width) STREAMING_COMPATIBLE {
  const uint8_t* src_u1 = src_u + src_stride_u;
  const uint8_t* src_v1 = src_v + src_stride_v;
  int vl;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "1:                                               \n"
      "ld1b     {z0.b}, p0/z, [%[src_u]]                \n"
      "ld1b     {z2.b}, p0/z, [%[src_u1]]               \n"
      "ld1b     {z1.b}, p0/z, [%[src_v]]                \n"
      "ld1b     {z3.b}, p0/z, [%[src_v1]]               \n"
      "incb     %[src_u]                                \n"
      "incb     %[src_u1]                               \n"
      "incb     %[src_v]                                \n"
      "incb     %[src_v1]                               \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "urhadd   z0.b, p0/m, z0.b, z2.b                  \n"
      "urhadd   z1.b, p0/m, z1.b, z3.b                  \n"
      "st2b     {z0.b, z1.b}, p0, [%[dst_uv]]           \n"
      "incb     %[dst_uv], all, mul #2                  \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.b, wzr, %w[width]                    \n"
      "ld1b     {z0.b}, p0/z, [%[src_u]]                \n"
      "ld1b     {z2.b}, p0/z, [%[src_u1]]               \n"
      "ld1b     {z1.b}, p0/z, [%[src_v]]                \n"
      "ld1b     {z3.b}, p0/z, [%[src_v1]]               \n"
      "urhadd   z0.b, p0/m, z0.b, z2.b                  \n"
      "urhadd   z1.b, p0/m, z1.b, z3.b                  \n"
      "st2b     {z0.b, z1.b}, p0, [%[dst_uv]]           \n"

      "99:                                              \n"
      : [src_u] "+r"(src_u),    // %[src_u]
        [src_u1] "+r"(src_u1),  // %[src_u1]
        [src_v] "+r"(src_v),    // %[src_v]
        [src_v1] "+r"(src_v1),  // %[src_v1]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [width] "+r"(width),    // %[width]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "p0");
}

static inline void HalfRow_SVE_SC(const uint8_t* src_ptr,
                                  ptrdiff_t src_stride,
                                  uint8_t* dst_ptr,
                                  int width) STREAMING_COMPATIBLE {
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  int vl;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "1:                                               \n"
      "ld1b     {z2.b}, p0/z, [%[src_ptr]]              \n"
      "ld1b     {z3.b}, p0/z, [%[src_ptr1]]             \n"
      "incb     %[src_ptr]                              \n"
      "incb     %[src_ptr1]                             \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "urhadd   z2.b, p0/m, z2.b, z3.b                  \n"
      "st1b     {z2.b}, p0, [%[dst_ptr]]                \n"
      "incb     %[dst_ptr]                              \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.b, wzr, %w[width]                    \n"
      "ld1b     {z2.b}, p0/z, [%[src_ptr]]              \n"
      "ld1b     {z3.b}, p0/z, [%[src_ptr1]]             \n"
      "urhadd   z2.b, p0/m, z2.b, z3.b                  \n"
      "st1b     {z2.b}, p0, [%[dst_ptr]]                \n"

      "99:                                              \n"
      : [src_ptr] "+r"(src_ptr),    // %[src_ptr]
        [src_ptr1] "+r"(src_ptr1),  // %[src_ptr1]
        [dst_ptr] "+r"(dst_ptr),    // %[dst_ptr]
        [width] "+r"(width),        // %[width]
        [vl] "=&r"(vl)              // %[vl]
      :
      : "memory", "cc", "z2", "z3", "p0");
}

static inline void HalfRow_16_SVE_SC(const uint16_t* src_ptr,
                                     ptrdiff_t src_stride,
                                     uint16_t* dst_ptr,
                                     int width) STREAMING_COMPATIBLE {
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  int vl;
  asm volatile(
      "cnth     %x[vl]                                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.h                                    \n"
      "1:                                               \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z3.h}, p0/z, [%[src_ptr1]]             \n"
      "incb     %[src_ptr]                              \n"
      "incb     %[src_ptr1]                             \n"
      "urhadd   z2.h, p0/m, z2.h, z3.h                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1h     {z2.h}, p0, [%[dst_ptr]]                \n"
      "incb     %[dst_ptr]                              \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.h, wzr, %w[width]                    \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z3.h}, p0/z, [%[src_ptr1]]             \n"
      "urhadd   z2.h, p0/m, z2.h, z3.h                  \n"
      "st1h     {z2.h}, p0, [%[dst_ptr]]                \n"

      "99:                                              \n"
      : [src_ptr] "+r"(src_ptr),    // %[src_ptr]
        [src_ptr1] "+r"(src_ptr1),  // %[src_ptr1]
        [dst_ptr] "+r"(dst_ptr),    // %[dst_ptr]
        [width] "+r"(width),        // %[width]
        [vl] "=&r"(vl)              // %[vl]
      :
      : "memory", "cc", "z2", "z3", "p0");
}

static inline void InterpolateRow_SVE_SC(uint8_t* dst_ptr,
                                         const uint8_t* src_ptr,
                                         ptrdiff_t src_stride,
                                         int width,
                                         int source_y_fraction)
    STREAMING_COMPATIBLE {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint8_t* src_ptr1 = src_ptr + src_stride;

  if (y0_fraction == 0) {
    CopyRow_SVE_SC(src_ptr1, dst_ptr, width);
    return;
  }
  if (y0_fraction == 128) {
    HalfRow_SVE_SC(src_ptr, src_stride, dst_ptr, width);
    return;
  }
  if (y0_fraction == 256) {
    CopyRow_SVE_SC(src_ptr, dst_ptr, width);
    return;
  }

  int vl;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "dup      z0.b, %w[y0_fraction]                   \n"
      "dup      z1.b, %w[y1_fraction]                   \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "1:                                               \n"
      "ld1b     {z2.b}, p0/z, [%[src_ptr]]              \n"
      "ld1b     {z3.b}, p0/z, [%[src_ptr1]]             \n"
      "incb     %[src_ptr]                              \n"
      "incb     %[src_ptr1]                             \n"
      "umullb   z4.h, z2.b, z0.b                        \n"
      "umullt   z2.h, z2.b, z0.b                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "umlalb   z4.h, z3.b, z1.b                        \n"
      "umlalt   z2.h, z3.b, z1.b                        \n"
      "rshrnb   z3.b, z4.h, #8                          \n"
      "rshrnt   z3.b, z2.h, #8                          \n"
      "st1b     {z3.b}, p0, [%[dst_ptr]]                \n"
      "incb     %[dst_ptr]                              \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.b, wzr, %w[width]                    \n"
      "ld1b     {z2.b}, p0/z, [%[src_ptr]]              \n"
      "ld1b     {z3.b}, p0/z, [%[src_ptr1]]             \n"
      "umullb   z4.h, z2.b, z0.b                        \n"
      "umullt   z2.h, z2.b, z0.b                        \n"
      "umlalb   z4.h, z3.b, z1.b                        \n"
      "umlalt   z2.h, z3.b, z1.b                        \n"
      "rshrnb   z3.b, z4.h, #8                          \n"
      "rshrnt   z3.b, z2.h, #8                          \n"
      "st1b     {z3.b}, p0, [%[dst_ptr]]                \n"

      "99:                                              \n"
      : [src_ptr] "+r"(src_ptr),         // %[src_ptr]
        [src_ptr1] "+r"(src_ptr1),       // %[src_ptr1]
        [dst_ptr] "+r"(dst_ptr),         // %[dst_ptr]
        [width] "+r"(width),             // %[width]
        [vl] "=&r"(vl)                   // %[vl]
      : [y0_fraction] "r"(y0_fraction),  // %[y0_fraction]
        [y1_fraction] "r"(y1_fraction)   // %[y1_fraction]
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "p0");
}

static inline void InterpolateRow_16_SVE_SC(uint16_t* dst_ptr,
                                            const uint16_t* src_ptr,
                                            ptrdiff_t src_stride,
                                            int width,
                                            int source_y_fraction)
    STREAMING_COMPATIBLE {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint16_t* src_ptr1 = src_ptr + src_stride;

  if (y0_fraction == 0) {
    CopyRow_SVE_SC((const uint8_t*)src_ptr1, (uint8_t*)dst_ptr,
                   width * sizeof(uint16_t));
    return;
  }
  if (y0_fraction == 128) {
    HalfRow_16_SVE_SC(src_ptr, src_stride, dst_ptr, width);
    return;
  }
  if (y0_fraction == 256) {
    CopyRow_SVE_SC((const uint8_t*)src_ptr, (uint8_t*)dst_ptr,
                   width * sizeof(uint16_t));
    return;
  }

  int vl;
  asm volatile(
      "cnth     %x[vl]                                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "dup      z0.h, %w[y0_fraction]                   \n"
      "dup      z1.h, %w[y1_fraction]                   \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.h                                    \n"
      "1:                                               \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z3.h}, p0/z, [%[src_ptr1]]             \n"
      "incb     %[src_ptr]                              \n"
      "incb     %[src_ptr1]                             \n"
      "umullb   z4.s, z2.h, z0.h                        \n"
      "umullt   z2.s, z2.h, z0.h                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "umlalb   z4.s, z3.h, z1.h                        \n"
      "umlalt   z2.s, z3.h, z1.h                        \n"
      "rshrnb   z3.h, z4.s, #8                          \n"
      "rshrnt   z3.h, z2.s, #8                          \n"
      "st1h     {z3.h}, p0, [%[dst_ptr]]                \n"
      "incb     %[dst_ptr]                              \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.h, wzr, %w[width]                    \n"
      "ld1h     {z2.h}, p0/z, [%[src_ptr]]              \n"
      "ld1h     {z3.h}, p0/z, [%[src_ptr1]]             \n"
      "umullb   z4.s, z2.h, z0.h                        \n"
      "umullt   z2.s, z2.h, z0.h                        \n"
      "umlalb   z4.s, z3.h, z1.h                        \n"
      "umlalt   z2.s, z3.h, z1.h                        \n"
      "rshrnb   z3.h, z4.s, #8                          \n"
      "rshrnt   z3.h, z2.s, #8                          \n"
      "st1h     {z3.h}, p0, [%[dst_ptr]]                \n"

      "99:                                              \n"
      : [src_ptr] "+r"(src_ptr),         // %[src_ptr]
        [src_ptr1] "+r"(src_ptr1),       // %[src_ptr1]
        [dst_ptr] "+r"(dst_ptr),         // %[dst_ptr]
        [width] "+r"(width),             // %[width]
        [vl] "=&r"(vl)                   // %[vl]
      : [y0_fraction] "r"(y0_fraction),  // %[y0_fraction]
        [y1_fraction] "r"(y1_fraction)   // %[y1_fraction]
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "p0");
}

static inline void YUY2ToYRow_SVE_SC(const uint8_t* src_yuy2,
                                     uint8_t* dst_y,
                                     int width) STREAMING_COMPATIBLE {
  int vl;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "1:                                               \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_yuy2]]       \n"
      "incb     %[src_yuy2], all, mul #2                \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1b     {z0.b}, p0, [%[dst_y]]                  \n"
      "incb     %[dst_y]                                \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.b, wzr, %w[width]                    \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_yuy2]]       \n"
      "st1b     {z0.b}, p0, [%[dst_y]]                  \n"

      "99:                                              \n"
      : [src_yuy2] "+r"(src_yuy2),  // %[src_yuy2]
        [dst_y] "+r"(dst_y),        // %[dst_y]
        [width] "+r"(width),        // %[width]
        [vl] "=&r"(vl)              // %[vl]
      :
      : "memory", "cc", "z0", "z1", "p0");
}

static inline void UYVYToYRow_SVE_SC(const uint8_t* src_uyvy,
                                     uint8_t* dst_y,
                                     int width) STREAMING_COMPATIBLE {
  int vl;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "1:                                               \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_uyvy]]       \n"
      "incb     %[src_uyvy], all, mul #2                \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1b     {z1.b}, p0, [%[dst_y]]                  \n"
      "incb     %[dst_y]                                \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "whilelt  p0.b, wzr, %w[width]                    \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_uyvy]]       \n"
      "st1b     {z1.b}, p0, [%[dst_y]]                  \n"

      "99:                                              \n"
      : [src_uyvy] "+r"(src_uyvy),  // %[src_uyvy]
        [dst_y] "+r"(dst_y),        // %[dst_y]
        [width] "+r"(width),        // %[width]
        [vl] "=&r"(vl)              // %[vl]
      :
      : "memory", "cc", "z0", "z1", "p0");
}

static inline void YUY2ToUVRow_SVE_SC(const uint8_t* src_yuy2,
                                      int src_stride_yuy2,
                                      uint8_t* dst_u,
                                      uint8_t* dst_v,
                                      int width) STREAMING_COMPATIBLE {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  int vl;
  int vl_half;
  int width_half;
  if (src_stride_yuy2 == 0) {
    src_yuy2_1 = src_yuy2;
  }
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "lsr      %w[vl_half], %w[vl], #1                 \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "whilelt  p1.b, wzr, %w[vl_half]                  \n"
      "1:                                               \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_yuy2]]       \n"
      "ld2b     {z4.b, z5.b}, p0/z, [%[src_yuy2_1]]     \n"
      "incb     %[src_yuy2], all, mul #2                \n"
      "incb     %[src_yuy2_1], all, mul #2              \n"
      "urhadd   z1.b, p0/m, z1.b, z5.b                  \n"
      "uzp1     z0.b, z1.b, z1.b                        \n"
      "uzp2     z2.b, z1.b, z1.b                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1b     {z0.b}, p1, [%[dst_u]]                  \n"
      "st1b     {z2.b}, p1, [%[dst_v]]                  \n"
      "add      %[dst_u], %[dst_u], %x[vl_half]         \n"
      "add      %[dst_v], %[dst_v], %x[vl_half]         \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "add      %w[width_half], %w[width], #1           \n"
      "lsr      %w[width_half], %w[width_half], #1      \n"
      "whilelt  p1.b, wzr, %w[width_half]               \n"
      "zip1     p0.b, p1.b, p1.b                        \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_yuy2]]       \n"
      "ld2b     {z4.b, z5.b}, p0/z, [%[src_yuy2_1]]     \n"
      "urhadd   z1.b, p0/m, z1.b, z5.b                  \n"
      "uzp1     z0.b, z1.b, z1.b                        \n"
      "uzp2     z2.b, z1.b, z1.b                        \n"
      "st1b     {z0.b}, p1, [%[dst_u]]                  \n"
      "st1b     {z2.b}, p1, [%[dst_v]]                  \n"

      "99:                                              \n"
      : [src_yuy2] "+r"(src_yuy2),      // %[src_yuy2]
        [src_yuy2_1] "+r"(src_yuy2_1),  // %[src_yuy2_1]
        [dst_u] "+r"(dst_u),            // %[dst_u]
        [dst_v] "+r"(dst_v),            // %[dst_v]
        [width] "+r"(width),            // %[width]
        [vl] "=&r"(vl),                 // %[vl]
        [vl_half] "=&r"(vl_half),       // %[vl_half]
        [width_half] "=&r"(width_half)  // %[width_half]
      :
      : "memory", "cc", "z0", "z1", "z2", "z4", "z5", "p0", "p1");
}

static inline void UYVYToUVRow_SVE_SC(const uint8_t* src_uyvy,
                                      int src_stride_uyvy,
                                      uint8_t* dst_u,
                                      uint8_t* dst_v,
                                      int width) STREAMING_COMPATIBLE {
  const uint8_t* src_uyvy_1 = src_uyvy + src_stride_uyvy;
  int vl;
  int vl_half;
  int width_half;
  if (src_stride_uyvy == 0) {
    src_uyvy_1 = src_uyvy;
  }
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "lsr      %w[vl_half], %w[vl], #1                 \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "ptrue    p0.b                                    \n"
      "whilelt  p1.b, wzr, %w[vl_half]                  \n"
      "1:                                               \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_uyvy]]       \n"
      "ld2b     {z4.b, z5.b}, p0/z, [%[src_uyvy_1]]     \n"
      "incb     %[src_uyvy], all, mul #2                \n"
      "incb     %[src_uyvy_1], all, mul #2              \n"
      "urhadd   z0.b, p0/m, z0.b, z4.b                  \n"
      "uzp1     z1.b, z0.b, z0.b                        \n"
      "uzp2     z2.b, z0.b, z0.b                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1b     {z1.b}, p1, [%[dst_u]]                  \n"
      "st1b     {z2.b}, p1, [%[dst_v]]                  \n"
      "add      %[dst_u], %[dst_u], %x[vl_half]         \n"
      "add      %[dst_v], %[dst_v], %x[vl_half]         \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "add      %w[width_half], %w[width], #1           \n"
      "lsr      %w[width_half], %w[width_half], #1      \n"
      "whilelt  p1.b, wzr, %w[width_half]               \n"
      "zip1     p0.b, p1.b, p1.b                        \n"
      "ld2b     {z0.b, z1.b}, p0/z, [%[src_uyvy]]       \n"
      "ld2b     {z4.b, z5.b}, p0/z, [%[src_uyvy_1]]     \n"
      "urhadd   z0.b, p0/m, z0.b, z4.b                  \n"
      "uzp1     z1.b, z0.b, z0.b                        \n"
      "uzp2     z2.b, z0.b, z0.b                        \n"
      "st1b     {z1.b}, p1, [%[dst_u]]                  \n"
      "st1b     {z2.b}, p1, [%[dst_v]]                  \n"

      "99:                                              \n"
      : [src_uyvy] "+r"(src_uyvy),      // %[src_uyvy]
        [src_uyvy_1] "+r"(src_uyvy_1),  // %[src_uyvy_1]
        [dst_u] "+r"(dst_u),            // %[dst_u]
        [dst_v] "+r"(dst_v),            // %[dst_v]
        [width] "+r"(width),            // %[width]
        [vl] "=&r"(vl),                 // %[vl]
        [vl_half] "=&r"(vl_half),       // %[vl_half]
        [width_half] "=&r"(width_half)  // %[width_half]
      :
      : "memory", "cc", "z0", "z1", "z2", "z4", "z5", "p0", "p1");
}

static inline void HalfBoxRow_SVE_SC(const uint8_t* src_ptr,
                                     ptrdiff_t src_stride,
                                     uint8_t* dst_ptr,
                                     int width) STREAMING_COMPATIBLE {
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  int vl;
  int vl_in;
  asm volatile(
      "cntb     %x[vl]                                  \n"
      "ptrue    p0.b                                    \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "1:                                               \n"
      "ld1b     {z0.b}, p0/z, [%[src0]]                 \n"
      "ld1b     {z1.b}, p0/z, [%[src0], #1, mul vl]     \n"
      "ld1b     {z2.b}, p0/z, [%[src1]]                 \n"
      "ld1b     {z3.b}, p0/z, [%[src1], #1, mul vl]     \n"
      "incb     %[src0], all, mul #2                    \n"
      "incb     %[src1], all, mul #2                    \n"
      "mov      z4.h, #0                                \n"
      "mov      z5.h, #0                                \n"
      "uadalp   z4.h, p0/m, z0.b                        \n"
      "uadalp   z5.h, p0/m, z1.b                        \n"
      "uadalp   z4.h, p0/m, z2.b                        \n"
      "uadalp   z5.h, p0/m, z3.b                        \n"
      "rshrnb   z4.b, z4.h, #2                          \n"
      "rshrnb   z5.b, z5.h, #2                          \n"
      "uzp1     z0.b, z4.b, z5.b                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1b     {z0.b}, p0, [%[dst]]                    \n"
      "incb     %[dst]                                  \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "lsl      %w[vl_in], %w[width], #1                \n"
      "whilelt  p1.b, wzr, %w[vl_in]                    \n"
      "whilelt  p2.b, %w[vl], %w[vl_in]                 \n"
      "whilelt  p3.b, wzr, %w[width]                    \n"
      "ld1b     {z0.b}, p1/z, [%[src0]]                 \n"
      "ld1b     {z1.b}, p2/z, [%[src0], #1, mul vl]     \n"
      "ld1b     {z2.b}, p1/z, [%[src1]]                 \n"
      "ld1b     {z3.b}, p2/z, [%[src1], #1, mul vl]     \n"
      "mov      z4.h, #0                                \n"
      "mov      z5.h, #0                                \n"
      "uadalp   z4.h, p0/m, z0.b                        \n"
      "uadalp   z5.h, p0/m, z1.b                        \n"
      "uadalp   z4.h, p0/m, z2.b                        \n"
      "uadalp   z5.h, p0/m, z3.b                        \n"
      "rshrnb   z4.b, z4.h, #2                          \n"
      "rshrnb   z5.b, z5.h, #2                          \n"
      "uzp1     z0.b, z4.b, z5.b                        \n"
      "st1b     {z0.b}, p3, [%[dst]]                    \n"

      "99:                                              \n"
      : [src0] "+r"(src_ptr),   // %[src0]
        [src1] "+r"(src_ptr1),  // %[src1]
        [dst] "+r"(dst_ptr),    // %[dst]
        [width] "+r"(width),    // %[width]
        [vl] "=&r"(vl),         // %[vl]
        [vl_in] "=&r"(vl_in)    // %[vl_in]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "p0", "p1", "p2",
        "p3");
}

static inline void HalfBoxRow_16_SVE_SC(const uint16_t* src_ptr,
                                        ptrdiff_t src_stride,
                                        uint16_t* dst_ptr,
                                        int width) STREAMING_COMPATIBLE {
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  int vl;
  int vl_in;
  asm volatile(
      "cnth     %x[vl]                                  \n"
      "ptrue    p0.b                                    \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "1:                                               \n"
      "ld1h     {z0.h}, p0/z, [%[src0]]                 \n"
      "ld1h     {z1.h}, p0/z, [%[src0], #1, mul vl]     \n"
      "ld1h     {z2.h}, p0/z, [%[src1]]                 \n"
      "ld1h     {z3.h}, p0/z, [%[src1], #1, mul vl]     \n"
      "incb     %[src0], all, mul #2                    \n"
      "incb     %[src1], all, mul #2                    \n"
      "mov      z4.s, #0                                \n"
      "mov      z5.s, #0                                \n"
      "uadalp   z4.s, p0/m, z0.h                        \n"
      "uadalp   z5.s, p0/m, z1.h                        \n"
      "uadalp   z4.s, p0/m, z2.h                        \n"
      "uadalp   z5.s, p0/m, z3.h                        \n"
      "rshrnb   z4.h, z4.s, #2                          \n"
      "rshrnb   z5.h, z5.s, #2                          \n"
      "uzp1     z0.h, z4.h, z5.h                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1h     {z0.h}, p0, [%[dst]]                    \n"
      "incb     %[dst]                                  \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "lsl      %w[vl_in], %w[width], #1                \n"
      "whilelt  p1.h, wzr, %w[vl_in]                    \n"
      "whilelt  p2.h, %w[vl], %w[vl_in]                 \n"
      "whilelt  p3.h, wzr, %w[width]                    \n"
      "ld1h     {z0.h}, p1/z, [%[src0]]                 \n"
      "ld1h     {z1.h}, p2/z, [%[src0], #1, mul vl]     \n"
      "ld1h     {z2.h}, p1/z, [%[src1]]                 \n"
      "ld1h     {z3.h}, p2/z, [%[src1], #1, mul vl]     \n"
      "mov      z4.s, #0                                \n"
      "mov      z5.s, #0                                \n"
      "uadalp   z4.s, p0/m, z0.h                        \n"
      "uadalp   z5.s, p0/m, z1.h                        \n"
      "uadalp   z4.s, p0/m, z2.h                        \n"
      "uadalp   z5.s, p0/m, z3.h                        \n"
      "rshrnb   z4.h, z4.s, #2                          \n"
      "rshrnb   z5.h, z5.s, #2                          \n"
      "uzp1     z0.h, z4.h, z5.h                        \n"
      "st1h     {z0.h}, p3, [%[dst]]                    \n"

      "99:                                              \n"
      : [src0] "+r"(src_ptr),   // %[src0]
        [src1] "+r"(src_ptr1),  // %[src1]
        [dst] "+r"(dst_ptr),    // %[dst]
        [width] "+r"(width),    // %[width]
        [vl] "=&r"(vl),         // %[vl]
        [vl_in] "=&r"(vl_in)    // %[vl_in]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "p0", "p1", "p2",
        "p3");
}

static inline void HalfBoxUVRow_SVE_SC(const uint8_t* src_uv,
                                       ptrdiff_t src_stride_uv,
                                       uint8_t* dst_uv,
                                       int width) STREAMING_COMPATIBLE {
  const uint8_t* src_uv1 = src_uv + src_stride_uv;
  int vl;
  int vl_in;
  asm volatile(
      "cnth     %x[vl]                                  \n"
      "ptrue    p0.b                                    \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "1:                                               \n"
      "ld1h     {z0.h}, p0/z, [%[src0]]                 \n"
      "ld1h     {z1.h}, p0/z, [%[src0], #1, mul vl]     \n"
      "ld1h     {z2.h}, p0/z, [%[src1]]                 \n"
      "ld1h     {z3.h}, p0/z, [%[src1], #1, mul vl]     \n"
      "incb     %[src0], all, mul #2                    \n"
      "incb     %[src1], all, mul #2                    \n"
      "uaddlb   z4.h, z0.b, z2.b                        \n"
      "uaddlt   z5.h, z0.b, z2.b                        \n"
      "uaddlb   z6.h, z1.b, z3.b                        \n"
      "uaddlt   z7.h, z1.b, z3.b                        \n"
      "addp     z4.h, p0/m, z4.h, z5.h                  \n"
      "addp     z6.h, p0/m, z6.h, z7.h                  \n"
      "rshrnb   z4.b, z4.h, #2                          \n"
      "rshrnb   z6.b, z6.h, #2                          \n"
      "uzp1     z0.b, z4.b, z6.b                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1h     {z0.h}, p0, [%[dst]]                    \n"
      "incb     %[dst]                                  \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "lsl      %w[vl_in], %w[width], #1                \n"
      "whilelt  p1.h, wzr, %w[vl_in]                    \n"
      "whilelt  p2.h, %w[vl], %w[vl_in]                 \n"
      "whilelt  p3.h, wzr, %w[width]                    \n"
      "ld1h     {z0.h}, p1/z, [%[src0]]                 \n"
      "ld1h     {z1.h}, p2/z, [%[src0], #1, mul vl]     \n"
      "ld1h     {z2.h}, p1/z, [%[src1]]                 \n"
      "ld1h     {z3.h}, p2/z, [%[src1], #1, mul vl]     \n"
      "uaddlb   z4.h, z0.b, z2.b                        \n"
      "uaddlt   z5.h, z0.b, z2.b                        \n"
      "uaddlb   z6.h, z1.b, z3.b                        \n"
      "uaddlt   z7.h, z1.b, z3.b                        \n"
      "addp     z4.h, p0/m, z4.h, z5.h                  \n"
      "addp     z6.h, p0/m, z6.h, z7.h                  \n"
      "rshrnb   z4.b, z4.h, #2                          \n"
      "rshrnb   z6.b, z6.h, #2                          \n"
      "uzp1     z0.b, z4.b, z6.b                        \n"
      "st1h     {z0.h}, p3, [%[dst]]                    \n"

      "99:                                              \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [width] "+r"(width),   // %[width]
        [vl] "=&r"(vl),        // %[vl]
        [vl_in] "=&r"(vl_in)   // %[vl_in]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "p0",
        "p1", "p2", "p3");
}

static inline void HalfBoxUVRow_16_SVE_SC(const uint16_t* src_uv,
                                          ptrdiff_t src_stride_uv,
                                          uint16_t* dst_uv,
                                          int width) STREAMING_COMPATIBLE {
  const uint16_t* src_uv1 = src_uv + src_stride_uv;
  int vl;
  int vl_in;
  asm volatile(
      "cntw     %x[vl]                                  \n"
      "ptrue    p0.b                                    \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "b.lt     2f                                      \n"

      "1:                                               \n"
      "ld1w     {z0.s}, p0/z, [%[src0]]                 \n"
      "ld1w     {z1.s}, p0/z, [%[src0], #1, mul vl]     \n"
      "ld1w     {z2.s}, p0/z, [%[src1]]                 \n"
      "ld1w     {z3.s}, p0/z, [%[src1], #1, mul vl]     \n"
      "incb     %[src0], all, mul #2                    \n"
      "incb     %[src1], all, mul #2                    \n"
      "uaddlb   z4.s, z0.h, z2.h                        \n"
      "uaddlt   z5.s, z0.h, z2.h                        \n"
      "uaddlb   z6.s, z1.h, z3.h                        \n"
      "uaddlt   z7.s, z1.h, z3.h                        \n"
      "addp     z4.s, p0/m, z4.s, z5.s                  \n"
      "addp     z6.s, p0/m, z6.s, z7.s                  \n"
      "rshrnb   z4.h, z4.s, #2                          \n"
      "rshrnb   z6.h, z6.s, #2                          \n"
      "uzp1     z0.h, z4.h, z6.h                        \n"
      "subs     %w[width], %w[width], %w[vl]            \n"
      "st1w     {z0.s}, p0, [%[dst]]                    \n"
      "incb     %[dst]                                  \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl]            \n"
      "b.eq     99f                                     \n"

      "lsl      %w[vl_in], %w[width], #1                \n"
      "whilelt  p1.s, wzr, %w[vl_in]                    \n"
      "whilelt  p2.s, %w[vl], %w[vl_in]                 \n"
      "whilelt  p3.s, wzr, %w[width]                    \n"
      "ld1w     {z0.s}, p1/z, [%[src0]]                 \n"
      "ld1w     {z1.s}, p2/z, [%[src0], #1, mul vl]     \n"
      "ld1w     {z2.s}, p1/z, [%[src1]]                 \n"
      "ld1w     {z3.s}, p2/z, [%[src1], #1, mul vl]     \n"
      "uaddlb   z4.s, z0.h, z2.h                        \n"
      "uaddlt   z5.s, z0.h, z2.h                        \n"
      "uaddlb   z6.s, z1.h, z3.h                        \n"
      "uaddlt   z7.s, z1.h, z3.h                        \n"
      "addp     z4.s, p0/m, z4.s, z5.s                  \n"
      "addp     z6.s, p0/m, z6.s, z7.s                  \n"
      "rshrnb   z4.h, z4.s, #2                          \n"
      "rshrnb   z6.h, z6.s, #2                          \n"
      "uzp1     z0.h, z4.h, z6.h                        \n"
      "st1w     {z0.s}, p3, [%[dst]]                    \n"

      "99:                                              \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [width] "+r"(width),   // %[width]
        [vl] "=&r"(vl),        // %[vl]
        [vl_in] "=&r"(vl_in)   // %[vl_in]
      :
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "p0",
        "p1", "p2", "p3");
}

static inline void HalfWidthRow_16To8_SVE_SC(const uint16_t* src_uv,
                                             ptrdiff_t src_uv_stride,
                                             uint8_t* dst_uv,
                                             int scale,
                                             int width) STREAMING_COMPATIBLE {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  const int shift = 23 - __builtin_clz((int32_t)scale);
  int vl_b;
  int vl_h;
  int vl_in;
  asm volatile(
      "cntb     %x[vl_b]                                \n"
      "ptrue    p0.b                                    \n"
      "dup      z31.h, %w[shift]                        \n"
      "subs     %w[width], %w[width], %w[vl_b]          \n"
      "b.lt     2f                                      \n"

      "1:                                               \n"
      "ld1h     {z0.h}, p0/z, [%[src0]]                 \n"
      "ld1h     {z1.h}, p0/z, [%[src0], #1, mul vl]     \n"
      "ld1h     {z2.h}, p0/z, [%[src0], #2, mul vl]     \n"
      "ld1h     {z3.h}, p0/z, [%[src0], #3, mul vl]     \n"
      "ld1h     {z4.h}, p0/z, [%[src1]]                 \n"
      "ld1h     {z5.h}, p0/z, [%[src1], #1, mul vl]     \n"
      "ld1h     {z6.h}, p0/z, [%[src1], #2, mul vl]     \n"
      "ld1h     {z7.h}, p0/z, [%[src1], #3, mul vl]     \n"
      "incb     %[src0], all, mul #4                    \n"
      "incb     %[src1], all, mul #4                    \n"
      "mov      z16.s, #0                               \n"
      "mov      z17.s, #0                               \n"
      "mov      z18.s, #0                               \n"
      "mov      z19.s, #0                               \n"
      "uadalp   z16.s, p0/m, z0.h                       \n"
      "uadalp   z17.s, p0/m, z1.h                       \n"
      "uadalp   z18.s, p0/m, z2.h                       \n"
      "uadalp   z19.s, p0/m, z3.h                       \n"
      "uadalp   z16.s, p0/m, z4.h                       \n"
      "uadalp   z17.s, p0/m, z5.h                       \n"
      "uadalp   z18.s, p0/m, z6.h                       \n"
      "uadalp   z19.s, p0/m, z7.h                       \n"
      "rshrnb   z16.h, z16.s, #2                        \n"
      "rshrnb   z17.h, z17.s, #2                        \n"
      "rshrnb   z18.h, z18.s, #2                        \n"
      "rshrnb   z19.h, z19.s, #2                        \n"
      "uzp1     z16.h, z16.h, z17.h                     \n"
      "uzp1     z18.h, z18.h, z19.h                     \n"
      "uqshl    z16.h, p0/m, z16.h, z31.h               \n"
      "uqshl    z18.h, p0/m, z18.h, z31.h               \n"
      "uzp2     z0.b, z16.b, z18.b                      \n"
      "subs     %w[width], %w[width], %w[vl_b]          \n"
      "st1b     {z0.b}, p0, [%[dst]]                    \n"
      "incb     %[dst]                                  \n"
      "b.ge     1b                                      \n"

      "2:                                               \n"
      "adds     %w[width], %w[width], %w[vl_b]          \n"
      "b.eq     99f                                     \n"

      "cnth     %x[vl_h]                                \n"
      "3:                                               \n"
      "lsl      %w[vl_in], %w[width], #1                \n"
      "whilelt  p1.h, wzr, %w[vl_in]                    \n"
      "whilelt  p2.h, %w[vl_h], %w[vl_in]               \n"
      "whilelt  p3.h, wzr, %w[width]                    \n"
      "ld1h     {z0.h}, p1/z, [%[src0]]                 \n"
      "ld1h     {z1.h}, p2/z, [%[src0], #1, mul vl]     \n"
      "ld1h     {z2.h}, p1/z, [%[src1]]                 \n"
      "ld1h     {z3.h}, p2/z, [%[src1], #1, mul vl]     \n"
      "incb     %[src0], all, mul #2                    \n"
      "incb     %[src1], all, mul #2                    \n"
      "mov      z16.s, #0                               \n"
      "mov      z17.s, #0                               \n"
      "uadalp   z16.s, p0/m, z0.h                       \n"
      "uadalp   z17.s, p0/m, z1.h                       \n"
      "uadalp   z16.s, p0/m, z2.h                       \n"
      "uadalp   z17.s, p0/m, z3.h                       \n"
      "rshrnb   z16.h, z16.s, #2                        \n"
      "rshrnb   z17.h, z17.s, #2                        \n"
      "uzp1     z0.h, z16.h, z17.h                     \n"
      "uqshl    z0.h, p0/m, z0.h, z31.h                 \n"
      "shrnb    z0.b, z0.h, #8                          \n"
      "st1b     {z0.h}, p3, [%[dst]]                    \n"
      "inch     %[dst]                                  \n"
      "subs     %w[width], %w[width], %w[vl_h]          \n"
      "b.gt     3b                                      \n"

      "99:                                              \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [width] "+r"(width),   // %[width]
        [vl_b] "=&r"(vl_b),    // %[vl_b]
        [vl_h] "=&r"(vl_h),    // %[vl_h]
        [vl_in] "=&r"(vl_in)   // %[vl_in]
      : [shift] "r"(shift)     // %[shift]
      : "memory", "cc", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z16",
        "z17", "z18", "z19", "z31", "p0", "p1", "p2", "p3");
}

#endif  // (!defined(LIBYUV_DISABLE_SVE) || (!defined(LIBYUV_DISABLE_SME) &&
        // (defined(CLANG_HAS_SME) || defined(GCC_HAS_SME)))) &&
        // defined(__aarch64__)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // INCLUDE_LIBYUV_ROW_SVE_H_

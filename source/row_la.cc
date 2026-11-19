/*
 *  Copyright 2022 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <string.h>

#include "libyuv/basic_types.h"
#include "libyuv/row.h"

// Note: every register written by an inline asm block (vector, general
// purpose, mask/predicate, "cc" for flags and "memory" for stores) must be
// listed in its clobber list, even when the ABI treats it as caller-saved.
// With LTO/LTCG the compiler may keep values live in any register across the
// asm statement, and other OS ABIs (e.g. Windows) differ in which registers
// are callee-saved. List "memory" first, then "cc", then registers.

#if (!defined(LIBYUV_DISABLE_LSX) || !defined(LIBYUV_DISABLE_LASX)) && \
    defined(__loongarch_lp64)

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#ifdef HAS_COPYROW_LSX
void CopyRow_LSX(const uint8_t* src, uint8_t* dst, int count) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),   // %[src]
        [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(count)  // %[cnt]
      :
      : "memory", "$vr0");
}
#endif  // HAS_COPYROW_LSX

#ifdef HAS_SETROW_LSX
void SetRow_LSX(uint8_t* dst, uint8_t v8, int count) {
  asm volatile(
      "vreplgr2vr.b     $vr0, %[val]                   \n"

      "1:                                              \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(count)  // %[cnt]
      : [val] "r"(v8)      // %[val]
      : "memory", "$vr0");
}
#endif  // HAS_SETROW_LSX

#ifdef HAS_ARGBSETROW_LSX
void ARGBSetRow_LSX(uint8_t* dst_argb, uint32_t v32, int width) {
  asm volatile(
      "vreplgr2vr.w     $vr0, %[val]                   \n"

      "1:                                              \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [val] "r"(v32)         // %[val]
      : "memory", "$vr0");
}
#endif  // HAS_ARGBSETROW_LSX

#ifdef HAS_MIRRORROW_LSX
void MirrorRow_LSX(const uint8_t* src_y, uint8_t* dst_y, int width) {
  static const uint8_t kRev[16] = {15, 14, 13, 12, 11, 10, 9, 8,
                                   7,  6,  5,  4,  3,  2,  1, 0};
  src_y += (ptrdiff_t)width - 16;
  asm volatile(
      "vld              $vr1, %[rev], 0                \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vshuf.b          $vr0, $vr0, $vr0, $vr1         \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], -16            \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [rev] "r"(kRev)     // %[rev]
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_MIRRORROW_LSX

#ifdef HAS_MIRRORUVROW_LSX
void MirrorUVRow_LSX(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  static const uint8_t kRevUV[16] = {14, 15, 12, 13, 10, 11, 8, 9,
                                     6,  7,  4,  5,  2,  3,  0, 1};
  src_uv += (ptrdiff_t)width * 2 - 16;
  asm volatile(
      "vld              $vr1, %[rev], 0                \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vshuf.b          $vr0, $vr0, $vr0, $vr1         \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], -16            \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [dst] "+r"(dst_uv),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      : [rev] "r"(kRevUV)    // %[rev]
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_MIRRORUVROW_LSX

#ifdef HAS_ARGBMIRRORROW_LSX
void ARGBMirrorRow_LSX(const uint8_t* src_argb, uint8_t* dst_argb, int width) {
  src_argb += (ptrdiff_t)width * 4 - 16;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vshuf4i.w        $vr0, $vr0, 0x1b               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], -16            \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0");
}
#endif  // HAS_ARGBMIRRORROW_LSX

#ifdef HAS_MIRRORSPLITUVROW_LSX
void MirrorSplitUVRow_LSX(const uint8_t* src_uv,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width) {
  static const uint8_t kRevUV[16] = {14, 15, 12, 13, 10, 11, 8, 9,
                                     6,  7,  4,  5,  2,  3,  0, 1};
  src_uv += (ptrdiff_t)width * 2 - 32;
  asm volatile(
      "vld              $vr4, %[rev], 0                \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 16               \n"
      "vld              $vr1, %[src], 0                \n"
      "vshuf.b          $vr0, $vr0, $vr0, $vr4         \n"
      "vshuf.b          $vr1, $vr1, $vr1, $vr4         \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vpickod.b        $vr3, $vr1, $vr0               \n"
      "vst              $vr2, %[du], 0                 \n"
      "vst              $vr3, %[dv], 0                 \n"
      "addi.d           %[src], %[src], -32            \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [du] "+r"(dst_u),    // %[du]
        [dv] "+r"(dst_v),    // %[dv]
        [cnt] "+r"(width)    // %[cnt]
      : [rev] "r"(kRevUV)    // %[rev]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_MIRRORSPLITUVROW_LSX

#ifdef HAS_SPLITUVROW_LSX
void SplitUVRow_LSX(const uint8_t* src_uv,
                    uint8_t* dst_u,
                    uint8_t* dst_v,
                    int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vpickod.b        $vr3, $vr1, $vr0               \n"
      "vst              $vr2, %[du], 0                 \n"
      "vst              $vr3, %[dv], 0                 \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [du] "+r"(dst_u),    // %[du]
        [dv] "+r"(dst_v),    // %[dv]
        [cnt] "+r"(width)    // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_SPLITUVROW_LSX

#ifdef HAS_MERGEUVROW_LSX
void MergeUVRow_LSX(const uint8_t* src_u,
                    const uint8_t* src_v,
                    uint8_t* dst_uv,
                    int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[su], 0                 \n"
      "vld              $vr1, %[sv], 0                 \n"
      "vilvl.b          $vr2, $vr1, $vr0               \n"
      "vilvh.b          $vr3, $vr1, $vr0               \n"
      "vst              $vr2, %[dst], 0                \n"
      "vst              $vr3, %[dst], 16               \n"
      "addi.d           %[su], %[su], 16               \n"
      "addi.d           %[sv], %[sv], 16               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [su] "+r"(src_u),    // %[su]
        [sv] "+r"(src_v),    // %[sv]
        [dst] "+r"(dst_uv),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_MERGEUVROW_LSX

#ifdef HAS_ARGBEXTRACTALPHAROW_LSX
void ARGBExtractAlphaRow_LSX(const uint8_t* src_argb,
                             uint8_t* dst_a,
                             int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickod.b        $vr0, $vr1, $vr0               \n"
      "vpickod.b        $vr0, $vr0, $vr0               \n"
      "vstelm.d         $vr0, %[dst], 0, 0             \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 8              \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_a),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_ARGBEXTRACTALPHAROW_LSX

#ifdef HAS_I422TOYUY2ROW_LSX
void I422ToYUY2Row_LSX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_yuy2,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "vldrepl.d        $vr1, %[su], 0                 \n"
      "vldrepl.d        $vr2, %[sv], 0                 \n"
      "vilvl.b          $vr3, $vr2, $vr1               \n"
      "vilvl.b          $vr4, $vr3, $vr0               \n"
      "vilvh.b          $vr5, $vr3, $vr0               \n"
      "vst              $vr4, %[dst], 0                \n"
      "vst              $vr5, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_yuy2),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_I422TOYUY2ROW_LSX

#ifdef HAS_I422TOUYVYROW_LSX
void I422ToUYVYRow_LSX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_uyvy,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "vldrepl.d        $vr1, %[su], 0                 \n"
      "vldrepl.d        $vr2, %[sv], 0                 \n"
      "vilvl.b          $vr3, $vr2, $vr1               \n"
      "vilvl.b          $vr4, $vr0, $vr3               \n"
      "vilvh.b          $vr5, $vr0, $vr3               \n"
      "vst              $vr4, %[dst], 0                \n"
      "vst              $vr5, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_uyvy),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_I422TOUYVYROW_LSX

#ifdef HAS_YUY2TOYROW_LSX
void YUY2ToYRow_LSX(const uint8_t* src_yuy2, uint8_t* dst_y, int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_YUY2TOYROW_LSX

#ifdef HAS_UYVYTOYROW_LSX
void UYVYToYRow_LSX(const uint8_t* src_uyvy, uint8_t* dst_y, int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickod.b        $vr2, $vr1, $vr0               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_UYVYTOYROW_LSX

#ifdef HAS_YUY2TOUV422ROW_LSX
void YUY2ToUV422Row_LSX(const uint8_t* src_yuy2,
                        uint8_t* dst_u,
                        uint8_t* dst_v,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickod.b        $vr2, $vr1, $vr0               \n"
      "vpickev.b        $vr3, $vr2, $vr2               \n"
      "vpickod.b        $vr4, $vr2, $vr2               \n"
      "vstelm.d         $vr3, %[du], 0, 0              \n"
      "vstelm.d         $vr4, %[dv], 0, 0              \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_YUY2TOUV422ROW_LSX

#ifdef HAS_UYVYTOUV422ROW_LSX
void UYVYToUV422Row_LSX(const uint8_t* src_uyvy,
                        uint8_t* dst_u,
                        uint8_t* dst_v,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vpickev.b        $vr3, $vr2, $vr2               \n"
      "vpickod.b        $vr4, $vr2, $vr2               \n"
      "vstelm.d         $vr3, %[du], 0, 0              \n"
      "vstelm.d         $vr4, %[dv], 0, 0              \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_UYVYTOUV422ROW_LSX

#ifdef HAS_YUY2TOUVROW_LSX
void YUY2ToUVRow_LSX(const uint8_t* src_yuy2,
                     int src_stride_yuy2,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  const uint8_t* next_yuy2 = src_yuy2 + src_stride_yuy2;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s0], 16                \n"
      "vld              $vr2, %[s1], 0                 \n"
      "vld              $vr3, %[s1], 16                \n"
      "vpickod.b        $vr0, $vr1, $vr0               \n"
      "vpickod.b        $vr2, $vr3, $vr2               \n"
      "vavgr.bu         $vr0, $vr0, $vr2               \n"
      "vpickev.b        $vr1, $vr0, $vr0               \n"
      "vpickod.b        $vr2, $vr0, $vr0               \n"
      "vstelm.d         $vr1, %[du], 0, 0              \n"
      "vstelm.d         $vr2, %[dv], 0, 0              \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_yuy2),   // %[s0]
        [s1] "+r"(next_yuy2),  // %[s1]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_YUY2TOUVROW_LSX

#ifdef HAS_UYVYTOUVROW_LSX
void UYVYToUVRow_LSX(const uint8_t* src_uyvy,
                     int src_stride_uyvy,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  const uint8_t* next_uyvy = src_uyvy + src_stride_uyvy;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s0], 16                \n"
      "vld              $vr2, %[s1], 0                 \n"
      "vld              $vr3, %[s1], 16                \n"
      "vpickev.b        $vr0, $vr1, $vr0               \n"
      "vpickev.b        $vr2, $vr3, $vr2               \n"
      "vavgr.bu         $vr0, $vr0, $vr2               \n"
      "vpickev.b        $vr1, $vr0, $vr0               \n"
      "vpickod.b        $vr2, $vr0, $vr0               \n"
      "vstelm.d         $vr1, %[du], 0, 0              \n"
      "vstelm.d         $vr2, %[dv], 0, 0              \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_uyvy),   // %[s0]
        [s1] "+r"(next_uyvy),  // %[s1]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_UYVYTOUVROW_LSX

#ifdef HAS_I422TOARGBROW_LSX
void I422ToARGBRow_LSX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.w        $vr10, %[su], 0                \n"
      "vldrepl.w        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr10, $vr10, $vr10            \n"
      "vilvl.b          $vr11, $vr11, $vr11            \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr4               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[su], %[su], 4                \n"
      "addi.d           %[sv], %[sv], 4                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr20", "$vr21", "$vr22",
        "$vr23", "$vr24");
}
#endif  // HAS_I422TOARGBROW_LSX

#ifdef HAS_I422TORGBAROW_LSX
void I422ToRGBARow_LSX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.w        $vr10, %[su], 0                \n"
      "vldrepl.w        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr10, $vr10, $vr10            \n"
      "vilvl.b          $vr11, $vr11, $vr11            \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr2, $vr5               \n"
      "vilvl.b          $vr7, $vr4, $vr3               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[su], %[su], 4                \n"
      "addi.d           %[sv], %[sv], 4                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr20", "$vr21", "$vr22",
        "$vr23", "$vr24");
}
#endif  // HAS_I422TORGBAROW_LSX

#ifdef HAS_I422ALPHATOARGBROW_LSX
void I422AlphaToARGBRow_LSX(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            const uint8_t* src_a,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.w        $vr10, %[su], 0                \n"
      "vldrepl.w        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr10, $vr10, $vr10            \n"
      "vilvl.b          $vr11, $vr11, $vr11            \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "vldrepl.d        $vr5, %[sa], 0                 \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr4               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[su], %[su], 4                \n"
      "addi.d           %[sv], %[sv], 4                \n"
      "addi.d           %[sa], %[sa], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width),     // %[cnt]
        [sa] "+r"(src_a)       // %[sa]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr20", "$vr21", "$vr22",
        "$vr23", "$vr24");
}
#endif  // HAS_I422ALPHATOARGBROW_LSX

#ifdef HAS_I422TORGB24ROW_LSX
void I422ToRGB24Row_LSX(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  static const uint8_t kShuf0[16] = {0,  1,  2,  4,  5,  6,  8,  9,
                                     10, 12, 13, 14, 16, 17, 18, 20};
  static const uint8_t kShuf1[16] = {21, 22, 24, 25, 26, 28, 29, 30,
                                     0,  0,  0,  0,  0,  0,  0,  0};
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vld              $vr12, %[s0], 0                \n"
      "vld              $vr13, %[s1], 0                \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.w        $vr10, %[su], 0                \n"
      "vldrepl.w        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr10, $vr10, $vr10            \n"
      "vilvl.b          $vr11, $vr11, $vr11            \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvl.b          $vr7, $vr4, $vr4               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vshuf.b          $vr10, $vr9, $vr8, $vr12       \n"
      "vshuf.b          $vr11, $vr9, $vr8, $vr13       \n"
      "vst              $vr10, %[dst], 0               \n"
      "vstelm.d         $vr11, %[dst], 16, 0           \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[su], %[su], 4                \n"
      "addi.d           %[sv], %[sv], 4                \n"
      "addi.d           %[dst], %[dst], 24             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),       // %[sy]
        [su] "+r"(src_u),       // %[su]
        [sv] "+r"(src_v),       // %[sv]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [c] "r"(yuvconstants),  // %[c]
        [s0] "r"(kShuf0),       // %[s0]
        [s1] "r"(kShuf1)        // %[s1]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr6", "$vr7",
        "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr20", "$vr21",
        "$vr22", "$vr23", "$vr24");
}
#endif  // HAS_I422TORGB24ROW_LSX

#ifdef HAS_I422TORGB565ROW_LSX
void I422ToRGB565Row_LSX(const uint8_t* src_y,
                         const uint8_t* src_u,
                         const uint8_t* src_v,
                         uint8_t* dst_argb,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr14, 31                      \n"
      "vsrli.h          $vr14, $vr14, 8                \n"
      "vldi             $vr15, 63                      \n"
      "vsrli.h          $vr15, $vr15, 8                \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.w        $vr10, %[su], 0                \n"
      "vldrepl.w        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr10, $vr10, $vr10            \n"
      "vilvl.b          $vr11, $vr11, $vr11            \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vsrli.b          $vr2, $vr2, 3                  \n"
      "vsrli.b          $vr3, $vr3, 2                  \n"
      "vsrli.b          $vr4, $vr4, 3                  \n"
      "vilvl.b          $vr2, $vr2, $vr2               \n"
      "vilvl.b          $vr3, $vr3, $vr3               \n"
      "vilvl.b          $vr4, $vr4, $vr4               \n"
      "vand.v           $vr2, $vr2, $vr14              \n"
      "vand.v           $vr3, $vr3, $vr15              \n"
      "vand.v           $vr4, $vr4, $vr14              \n"
      "vslli.h          $vr3, $vr3, 5                  \n"
      "vslli.h          $vr4, $vr4, 11                 \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vor.v            $vr2, $vr2, $vr4               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[su], %[su], 4                \n"
      "addi.d           %[sv], %[sv], 4                \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr10", "$vr11",
        "$vr14", "$vr15", "$vr20", "$vr21", "$vr22", "$vr23", "$vr24");
}
#endif  // HAS_I422TORGB565ROW_LSX

#ifdef HAS_I422TOARGB4444ROW_LSX
void I422ToARGB4444Row_LSX(const uint8_t* src_y,
                           const uint8_t* src_u,
                           const uint8_t* src_v,
                           uint8_t* dst_argb,
                           const struct YuvConstants* yuvconstants,
                           int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr14, 15                      \n"
      "vsrli.h          $vr14, $vr14, 8                \n"
      "vldi             $vr15, 240                     \n"
      "vsrli.h          $vr15, $vr15, 8                \n"
      "vslli.h          $vr15, $vr15, 8                \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.w        $vr10, %[su], 0                \n"
      "vldrepl.w        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr10, $vr10, $vr10            \n"
      "vilvl.b          $vr11, $vr11, $vr11            \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vsrli.b          $vr2, $vr2, 4                  \n"
      "vsrli.b          $vr3, $vr3, 4                  \n"
      "vsrli.b          $vr4, $vr4, 4                  \n"
      "vilvl.b          $vr2, $vr2, $vr2               \n"
      "vilvl.b          $vr3, $vr3, $vr3               \n"
      "vilvl.b          $vr4, $vr4, $vr4               \n"
      "vand.v           $vr2, $vr2, $vr14              \n"
      "vand.v           $vr3, $vr3, $vr14              \n"
      "vand.v           $vr4, $vr4, $vr14              \n"
      "vslli.h          $vr3, $vr3, 4                  \n"
      "vslli.h          $vr4, $vr4, 8                  \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vor.v            $vr2, $vr2, $vr4               \n"
      "vor.v            $vr2, $vr2, $vr15              \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[su], %[su], 4                \n"
      "addi.d           %[sv], %[sv], 4                \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr10", "$vr11",
        "$vr14", "$vr15", "$vr20", "$vr21", "$vr22", "$vr23", "$vr24");
}
#endif  // HAS_I422TOARGB4444ROW_LSX

#ifdef HAS_I422TOARGB1555ROW_LSX
void I422ToARGB1555Row_LSX(const uint8_t* src_y,
                           const uint8_t* src_u,
                           const uint8_t* src_v,
                           uint8_t* dst_argb,
                           const struct YuvConstants* yuvconstants,
                           int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr14, 31                      \n"
      "vsrli.h          $vr14, $vr14, 8                \n"
      "vldi             $vr15, 128                     \n"
      "vsrli.h          $vr15, $vr15, 8                \n"
      "vslli.h          $vr15, $vr15, 8                \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.w        $vr10, %[su], 0                \n"
      "vldrepl.w        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr10, $vr10, $vr10            \n"
      "vilvl.b          $vr11, $vr11, $vr11            \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vsrli.b          $vr2, $vr2, 3                  \n"
      "vsrli.b          $vr3, $vr3, 3                  \n"
      "vsrli.b          $vr4, $vr4, 3                  \n"
      "vilvl.b          $vr2, $vr2, $vr2               \n"
      "vilvl.b          $vr3, $vr3, $vr3               \n"
      "vilvl.b          $vr4, $vr4, $vr4               \n"
      "vand.v           $vr2, $vr2, $vr14              \n"
      "vand.v           $vr3, $vr3, $vr14              \n"
      "vand.v           $vr4, $vr4, $vr14              \n"
      "vslli.h          $vr3, $vr3, 5                  \n"
      "vslli.h          $vr4, $vr4, 10                 \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vor.v            $vr2, $vr2, $vr4               \n"
      "vor.v            $vr2, $vr2, $vr15              \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[su], %[su], 4                \n"
      "addi.d           %[sv], %[sv], 4                \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr10", "$vr11",
        "$vr14", "$vr15", "$vr20", "$vr21", "$vr22", "$vr23", "$vr24");
}
#endif  // HAS_I422TOARGB1555ROW_LSX

#ifdef HAS_NV12TOARGBROW_LSX
void NV12ToARGBRow_LSX(const uint8_t* src_y,
                       const uint8_t* src_uv,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.d        $vr1, %[suv], 0                \n"
      "vilvl.h          $vr1, $vr1, $vr1               \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr4               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[suv], %[suv], 8              \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [suv] "+r"(src_uv),    // %[suv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr20", "$vr21", "$vr22", "$vr23", "$vr24");
}
#endif  // HAS_NV12TOARGBROW_LSX

#ifdef HAS_NV21TOARGBROW_LSX
void NV21ToARGBRow_LSX(const uint8_t* src_y,
                       const uint8_t* src_vu,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.d        $vr1, %[suv], 0                \n"
      "vshuf4i.b        $vr1, $vr1, 0xb1               \n"
      "vilvl.h          $vr1, $vr1, $vr1               \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr4               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[suv], %[suv], 8              \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [suv] "+r"(src_vu),    // %[suv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr20", "$vr21", "$vr22", "$vr23", "$vr24");
}
#endif  // HAS_NV21TOARGBROW_LSX

#ifdef HAS_NV12TORGB565ROW_LSX
void NV12ToRGB565Row_LSX(const uint8_t* src_y,
                         const uint8_t* src_uv,
                         uint8_t* dst_argb,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr14, 31                      \n"
      "vsrli.h          $vr14, $vr14, 8                \n"
      "vldi             $vr15, 63                      \n"
      "vsrli.h          $vr15, $vr15, 8                \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.d        $vr1, %[suv], 0                \n"
      "vilvl.h          $vr1, $vr1, $vr1               \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vsrli.b          $vr2, $vr2, 3                  \n"
      "vsrli.b          $vr3, $vr3, 2                  \n"
      "vsrli.b          $vr4, $vr4, 3                  \n"
      "vilvl.b          $vr2, $vr2, $vr2               \n"
      "vilvl.b          $vr3, $vr3, $vr3               \n"
      "vilvl.b          $vr4, $vr4, $vr4               \n"
      "vand.v           $vr2, $vr2, $vr14              \n"
      "vand.v           $vr3, $vr3, $vr15              \n"
      "vand.v           $vr4, $vr4, $vr14              \n"
      "vslli.h          $vr3, $vr3, 5                  \n"
      "vslli.h          $vr4, $vr4, 11                 \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vor.v            $vr2, $vr2, $vr4               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[suv], %[suv], 8              \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [suv] "+r"(src_uv),    // %[suv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr14", "$vr15",
        "$vr20", "$vr21", "$vr22", "$vr23", "$vr24");
}
#endif  // HAS_NV12TORGB565ROW_LSX

#ifdef HAS_I444TOARGBROW_LSX
void I444ToARGBRow_LSX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vldrepl.d        $vr10, %[su], 0                \n"
      "vldrepl.d        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr4               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr20", "$vr21", "$vr22",
        "$vr23", "$vr24");
}
#endif  // HAS_I444TOARGBROW_LSX

#ifdef HAS_I400TOARGBROW_LSX
void I400ToARGBRow_LSX(const uint8_t* src_y,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  asm volatile(
      "vld              $vr23, %[c], 96                \n"
      "vld              $vr24, %[c], 128               \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sy], 0                 \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vssrani.bu.h     $vr2, $vr0, 6                  \n"
      "vilvl.b          $vr6, $vr2, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr2               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr2", "$vr5", "$vr6", "$vr7", "$vr8", "$vr9",
        "$vr23", "$vr24");
}
#endif  // HAS_I400TOARGBROW_LSX

#ifdef HAS_J400TOARGBROW_LSX
void J400ToARGBRow_LSX(const uint8_t* src_y, uint8_t* dst_argb, int width) {
  asm volatile(
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr2, %[sy], 0                 \n"
      "vilvl.b          $vr6, $vr2, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr2               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr2", "$vr5", "$vr6", "$vr7", "$vr8", "$vr9");
}
#endif  // HAS_J400TOARGBROW_LSX

#ifdef HAS_YUY2TOARGBROW_LSX
void YUY2ToARGBRow_LSX(const uint8_t* src_yuy2,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr10, %[src], 0               \n"
      "vpickev.b        $vr0, $vr10, $vr10             \n"
      "vpickod.b        $vr1, $vr10, $vr10             \n"
      "vilvl.h          $vr1, $vr1, $vr1               \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr4               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr20", "$vr21", "$vr22", "$vr23",
        "$vr24");
}
#endif  // HAS_YUY2TOARGBROW_LSX

#ifdef HAS_UYVYTOARGBROW_LSX
void UYVYToARGBRow_LSX(const uint8_t* src_uyvy,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  asm volatile(
      "vldrepl.w        $vr20, %[c], 0                 \n"
      "vldrepl.w        $vr21, %[c], 32                \n"
      "vldrepl.w        $vr22, %[c], 64                \n"
      "vldrepl.h        $vr23, %[c], 96                \n"
      "vldrepl.h        $vr24, %[c], 128               \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr10, %[src], 0               \n"
      "vpickod.b        $vr0, $vr10, $vr10             \n"
      "vpickev.b        $vr1, $vr10, $vr10             \n"
      "vilvl.h          $vr1, $vr1, $vr1               \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vxori.b          $vr1, $vr1, 0x80               \n"
      "vmuh.hu          $vr0, $vr0, $vr23              \n"
      "vadd.h           $vr0, $vr0, $vr24              \n"
      "vmulwev.h.bu.b   $vr2, $vr20, $vr1              \n"
      "vmulwod.h.bu.b   $vr4, $vr22, $vr1              \n"
      "vmulwev.h.bu.b   $vr3, $vr21, $vr1              \n"
      "vmaddwod.h.bu.b  $vr3, $vr21, $vr1              \n"
      "vsadd.h          $vr2, $vr0, $vr2               \n"
      "vsadd.h          $vr4, $vr0, $vr4               \n"
      "vssub.h          $vr3, $vr0, $vr3               \n"
      "vssrani.bu.h     $vr2, $vr2, 6                  \n"
      "vssrani.bu.h     $vr3, $vr3, 6                  \n"
      "vssrani.bu.h     $vr4, $vr4, 6                  \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvl.b          $vr7, $vr5, $vr4               \n"
      "vilvl.h          $vr8, $vr7, $vr6               \n"
      "vilvh.h          $vr9, $vr7, $vr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "vst              $vr9, %[dst], 16               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr20", "$vr21", "$vr22", "$vr23",
        "$vr24");
}
#endif  // HAS_UYVYTOARGBROW_LSX

#if defined(HAS_ARGBTORGB24ROW_LSX) || defined(HAS_ARGBTORAWROW_LSX)
static LIBYUV_NOINLINE void ARGBTo24Helper_LSX(const uint8_t* src_argb,
                                               uint8_t* dst_24,
                                               const uint8_t* shuf,
                                               int width) {
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr2, %[src], 32               \n"
      "vld              $vr3, %[src], 48               \n"
      "vshuf.b          $vr0, $vr0, $vr0, $vr4         \n"
      "vshuf.b          $vr1, $vr1, $vr1, $vr4         \n"
      "vshuf.b          $vr2, $vr2, $vr2, $vr4         \n"
      "vshuf.b          $vr3, $vr3, $vr3, $vr4         \n"
      "vextrins.w       $vr0, $vr1, 0x30               \n"
      "vbsrl.v          $vr1, $vr1, 4                  \n"
      "vextrins.d       $vr1, $vr2, 0x10               \n"
      "vbsll.v          $vr3, $vr3, 4                  \n"
      "vextrins.w       $vr3, $vr2, 0x02               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr3, %[dst], 32               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 48             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_24),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(shuf)       // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // defined(HAS_ARGBTORGB24ROW_LSX) || defined(HAS_ARGBTORAWROW_LSX)

#ifdef HAS_ARGBTORGB24ROW_LSX
void ARGBToRGB24Row_LSX(const uint8_t* src_argb,
                        uint8_t* dst_rgb24,
                        int width) {
  static const uint8_t kShuf[16] = {0,  1,  2,  4,  5, 6, 8, 9,
                                    10, 12, 13, 14, 0, 0, 0, 0};
  ARGBTo24Helper_LSX(src_argb, dst_rgb24, kShuf, width);
}
#endif  // HAS_ARGBTORGB24ROW_LSX

#ifdef HAS_ARGBTORAWROW_LSX
void ARGBToRAWRow_LSX(const uint8_t* src_argb, uint8_t* dst_raw, int width) {
  static const uint8_t kShuf[16] = {2, 1,  0,  6,  5, 4, 10, 9,
                                    8, 14, 13, 12, 0, 0, 0,  0};
  ARGBTo24Helper_LSX(src_argb, dst_raw, kShuf, width);
}
#endif  // HAS_ARGBTORAWROW_LSX

#if defined(HAS_RGB24TOARGBROW_LSX) || defined(HAS_RAWTOARGBROW_LSX) || \
    defined(HAS_RAWTORGBAROW_LSX)
static LIBYUV_NOINLINE void RGB24ToARGBHelper_LSX(const uint8_t* src_24,
                                                  uint8_t* dst_argb,
                                                  const uint8_t* shuf,
                                                  int width) {
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "vld              $vr5, %[shuf], 16              \n"
      "vldi             $vr6, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 12               \n"
      "vld              $vr2, %[src], 24               \n"
      "vld              $vr3, %[src], 32               \n"
      "vshuf.b          $vr0, $vr6, $vr0, $vr4         \n"
      "vshuf.b          $vr1, $vr6, $vr1, $vr4         \n"
      "vshuf.b          $vr2, $vr6, $vr2, $vr4         \n"
      "vshuf.b          $vr3, $vr6, $vr3, $vr5         \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr2, %[dst], 32               \n"
      "vst              $vr3, %[dst], 48               \n"
      "addi.d           %[src], %[src], 48             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_24),    // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(shuf)       // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // defined(HAS_RGB24TOARGBROW_LSX) || defined(HAS_RAWTOARGBROW_LSX) ||
        // defined(HAS_RAWTORGBAROW_LSX)

#ifdef HAS_RGB24TOARGBROW_LSX
void RGB24ToARGBRow_LSX(const uint8_t* src_rgb24,
                        uint8_t* dst_argb,
                        int width) {
  static const uint8_t kShuf[32] = {0,  1,  2,  16, 3,  4,  5,  16, 6,  7, 8,
                                    16, 9,  10, 11, 16, 4,  5,  6,  16, 7, 8,
                                    9,  16, 10, 11, 12, 16, 13, 14, 15, 16};
  RGB24ToARGBHelper_LSX(src_rgb24, dst_argb, kShuf, width);
}
#endif  // HAS_RGB24TOARGBROW_LSX

#ifdef HAS_RAWTOARGBROW_LSX
void RAWToARGBRow_LSX(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  static const uint8_t kShuf[32] = {2,  1,  0,  16, 5,  4,  3,  16, 8,  7, 6,
                                    16, 11, 10, 9,  16, 6,  5,  4,  16, 9, 8,
                                    7,  16, 12, 11, 10, 16, 15, 14, 13, 16};
  RGB24ToARGBHelper_LSX(src_raw, dst_argb, kShuf, width);
}
#endif  // HAS_RAWTOARGBROW_LSX

#ifdef HAS_RAWTORGBAROW_LSX
void RAWToRGBARow_LSX(const uint8_t* src_raw, uint8_t* dst_rgba, int width) {
  static const uint8_t kShuf[32] = {16, 2,  1,  0,  16, 5,  4,  3,  16, 8,  7,
                                    6,  16, 11, 10, 9,  16, 6,  5,  4,  16, 9,
                                    8,  7,  16, 12, 11, 10, 16, 15, 14, 13};
  RGB24ToARGBHelper_LSX(src_raw, dst_rgba, kShuf, width);
}
#endif  // HAS_RAWTORGBAROW_LSX

#ifdef HAS_RAWTORGB24ROW_LSX
void RAWToRGB24Row_LSX(const uint8_t* src_raw, uint8_t* dst_rgb24, int width) {
  static const uint8_t kShuf[32] = {2,  1,  0,  5,  4,  3,  8, 7, 6, 11, 10,
                                    9,  0,  0,  0,  0,  6,  5, 4, 9, 8,  7,
                                    12, 11, 10, 15, 14, 13, 0, 0, 0, 0};
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "vld              $vr5, %[shuf], 16              \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 12               \n"
      "vld              $vr2, %[src], 24               \n"
      "vld              $vr3, %[src], 32               \n"
      "vshuf.b          $vr0, $vr0, $vr0, $vr4         \n"
      "vshuf.b          $vr1, $vr1, $vr1, $vr4         \n"
      "vshuf.b          $vr2, $vr2, $vr2, $vr4         \n"
      "vshuf.b          $vr3, $vr3, $vr3, $vr5         \n"
      "vextrins.w       $vr0, $vr1, 0x30               \n"
      "vbsrl.v          $vr1, $vr1, 4                  \n"
      "vextrins.d       $vr1, $vr2, 0x10               \n"
      "vbsll.v          $vr3, $vr3, 4                  \n"
      "vextrins.w       $vr3, $vr2, 0x02               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr3, %[dst], 32               \n"
      "addi.d           %[src], %[src], 48             \n"
      "addi.d           %[dst], %[dst], 48             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_raw),    // %[src]
        [dst] "+r"(dst_rgb24),  // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [shuf] "r"(kShuf)       // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_RAWTORGB24ROW_LSX

#ifdef HAS_ARGBTORGB565ROW_LSX
void ARGBToRGB565Row_LSX(const uint8_t* src_argb, uint8_t* dst_rgb, int width) {
  asm volatile(
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"
      "vldi             $vr7, 63                       \n"
      "vsrli.h          $vr7, $vr7, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vpickod.b        $vr3, $vr1, $vr0               \n"
      "vsrli.b          $vr2, $vr2, 3                  \n"
      "vsrli.b          $vr3, $vr3, 2                  \n"
      "vand.v           $vr4, $vr2, $vr6               \n"
      "vsrli.h          $vr2, $vr2, 8                  \n"
      "vand.v           $vr3, $vr3, $vr7               \n"
      "vslli.h          $vr3, $vr3, 5                  \n"
      "vslli.h          $vr2, $vr2, 11                 \n"
      "vor.v            $vr4, $vr4, $vr3               \n"
      "vor.v            $vr4, $vr4, $vr2               \n"
      "vst              $vr4, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_rgb),   // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr6", "$vr7");
}
#endif  // HAS_ARGBTORGB565ROW_LSX

#ifdef HAS_ARGBTORGB565DITHERROW_LSX
void ARGBToRGB565DitherRow_LSX(const uint8_t* src_argb,
                               uint8_t* dst_rgb,
                               uint32_t dither4,
                               int width) {
  asm volatile(
      "vreplgr2vr.w     $vr5, %[dith]                  \n"
      "vilvl.b          $vr5, $vr5, $vr5               \n"
      "vilvl.h          $vr5, $vr5, $vr5               \n"
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"
      "vldi             $vr7, 63                       \n"
      "vsrli.h          $vr7, $vr7, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vsadd.bu         $vr0, $vr0, $vr5               \n"
      "vpickev.b        $vr2, $vr0, $vr0               \n"
      "vpickod.b        $vr3, $vr0, $vr0               \n"
      "vsrli.b          $vr2, $vr2, 3                  \n"
      "vsrli.b          $vr3, $vr3, 2                  \n"
      "vand.v           $vr4, $vr2, $vr6               \n"
      "vsrli.h          $vr2, $vr2, 8                  \n"
      "vand.v           $vr3, $vr3, $vr7               \n"
      "vslli.h          $vr3, $vr3, 5                  \n"
      "vslli.h          $vr2, $vr2, 11                 \n"
      "vor.v            $vr4, $vr4, $vr3               \n"
      "vor.v            $vr4, $vr4, $vr2               \n"
      "vstelm.d         $vr4, %[dst], 0, 0             \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 8              \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_rgb),   // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [dith] "r"(dither4)    // %[dith]
      : "memory", "$vr0", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6", "$vr7");
}
#endif  // HAS_ARGBTORGB565DITHERROW_LSX

#ifdef HAS_ARGBTOARGB1555ROW_LSX
void ARGBToARGB1555Row_LSX(const uint8_t* src_argb,
                           uint8_t* dst_rgb,
                           int width) {
  asm volatile(
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vpickod.b        $vr3, $vr1, $vr0               \n"
      "vsrli.b          $vr2, $vr2, 3                  \n"
      "vand.v           $vr4, $vr2, $vr6               \n"
      "vsrli.h          $vr2, $vr2, 8                  \n"
      "vsrli.h          $vr5, $vr3, 15                 \n"
      "vsrli.b          $vr3, $vr3, 3                  \n"
      "vand.v           $vr3, $vr3, $vr6               \n"
      "vslli.h          $vr3, $vr3, 5                  \n"
      "vslli.h          $vr2, $vr2, 10                 \n"
      "vslli.h          $vr5, $vr5, 15                 \n"
      "vor.v            $vr4, $vr4, $vr3               \n"
      "vor.v            $vr4, $vr4, $vr2               \n"
      "vor.v            $vr4, $vr4, $vr5               \n"
      "vst              $vr4, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_rgb),   // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_ARGBTOARGB1555ROW_LSX

#ifdef HAS_ARGBTOARGB4444ROW_LSX
void ARGBToARGB4444Row_LSX(const uint8_t* src_argb,
                           uint8_t* dst_rgb,
                           int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vpickod.b        $vr3, $vr1, $vr0               \n"
      "vsrli.b          $vr2, $vr2, 4                  \n"
      "vsrli.b          $vr3, $vr3, 4                  \n"
      "vslli.b          $vr3, $vr3, 4                  \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_rgb),   // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_ARGBTOARGB4444ROW_LSX

#ifdef HAS_RGB565TOARGBROW_LSX
void RGB565ToARGBRow_LSX(const uint8_t* src_rgb565,
                         uint8_t* dst_argb,
                         int width) {
  asm volatile(
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"
      "vldi             $vr7, 63                       \n"
      "vsrli.h          $vr7, $vr7, 8                  \n"
      "vldi             $vr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr7               \n"
      "vsrli.h          $vr3, $vr0, 11                 \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 2                  \n"
      "vsrli.h          $vr2, $vr2, 4                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vpickev.b        $vr1, $vr1, $vr1               \n"
      "vpickev.b        $vr2, $vr2, $vr2               \n"
      "vpickev.b        $vr3, $vr3, $vr3               \n"
      "vilvl.b          $vr8, $vr2, $vr1               \n"
      "vilvl.b          $vr9, $vr5, $vr3               \n"
      "vilvl.h          $vr10, $vr9, $vr8              \n"
      "vilvh.h          $vr11, $vr9, $vr8              \n"
      "vst              $vr10, %[dst], 0               \n"
      "vst              $vr11, %[dst], 16              \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb565),  // %[src]
        [dst] "+r"(dst_argb),    // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_RGB565TOARGBROW_LSX

#ifdef HAS_ARGB1555TOARGBROW_LSX
void ARGB1555ToARGBRow_LSX(const uint8_t* src_argb1555,
                           uint8_t* dst_argb,
                           int width) {
  asm volatile(
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vsrli.h          $vr3, $vr0, 10                 \n"
      "vand.v           $vr3, $vr3, $vr6               \n"
      "vsrai.h          $vr5, $vr0, 15                 \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 3                  \n"
      "vsrli.h          $vr2, $vr2, 2                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vpickev.b        $vr1, $vr1, $vr1               \n"
      "vpickev.b        $vr2, $vr2, $vr2               \n"
      "vpickev.b        $vr3, $vr3, $vr3               \n"
      "vpickev.b        $vr5, $vr5, $vr5               \n"
      "vilvl.b          $vr8, $vr2, $vr1               \n"
      "vilvl.b          $vr9, $vr5, $vr3               \n"
      "vilvl.h          $vr10, $vr9, $vr8              \n"
      "vilvh.h          $vr11, $vr9, $vr8              \n"
      "vst              $vr10, %[dst], 0               \n"
      "vst              $vr11, %[dst], 16              \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb1555),  // %[src]
        [dst] "+r"(dst_argb),      // %[dst]
        [cnt] "+r"(width)          // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr8", "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_ARGB1555TOARGBROW_LSX

#ifdef HAS_ARGB4444TOARGBROW_LSX
void ARGB4444ToARGBRow_LSX(const uint8_t* src_argb4444,
                           uint8_t* dst_argb,
                           int width) {
  asm volatile(
      "vldi             $vr6, 0x0f                     \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.b          $vr2, $vr0, 4                  \n"
      "vslli.b          $vr3, $vr1, 4                  \n"
      "vor.v            $vr1, $vr1, $vr3               \n"
      "vslli.b          $vr3, $vr2, 4                  \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vilvl.b          $vr4, $vr2, $vr1               \n"
      "vilvh.b          $vr5, $vr2, $vr1               \n"
      "vst              $vr4, %[dst], 0                \n"
      "vst              $vr5, %[dst], 16               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb4444),  // %[src]
        [dst] "+r"(dst_argb),      // %[dst]
        [cnt] "+r"(width)          // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_ARGB4444TOARGBROW_LSX

#ifdef HAS_ARGBMULTIPLYROW_LSX
void ARGBMultiplyRow_LSX(const uint8_t* src_argb,
                         const uint8_t* src_argb1,
                         uint8_t* dst_argb,
                         int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s1], 0                 \n"
      "vmulwev.h.bu     $vr2, $vr0, $vr1               \n"
      "vmulwod.h.bu     $vr3, $vr0, $vr1               \n"
      "vsrari.h         $vr2, $vr2, 8                  \n"
      "vsrari.h         $vr3, $vr3, 8                  \n"
      "vpackev.b        $vr2, $vr3, $vr2               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 16               \n"
      "addi.d           %[s1], %[s1], 16               \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb),   // %[s0]
        [s1] "+r"(src_argb1),  // %[s1]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_ARGBMULTIPLYROW_LSX

#ifdef HAS_ARGBADDROW_LSX
void ARGBAddRow_LSX(const uint8_t* src_argb,
                    const uint8_t* src_argb1,
                    uint8_t* dst_argb,
                    int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s1], 0                 \n"
      "vsadd.bu         $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 16               \n"
      "addi.d           %[s1], %[s1], 16               \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb),   // %[s0]
        [s1] "+r"(src_argb1),  // %[s1]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_ARGBADDROW_LSX

#ifdef HAS_ARGBSUBTRACTROW_LSX
void ARGBSubtractRow_LSX(const uint8_t* src_argb,
                         const uint8_t* src_argb1,
                         uint8_t* dst_argb,
                         int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s1], 0                 \n"
      "vssub.bu         $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 16               \n"
      "addi.d           %[s1], %[s1], 16               \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb),   // %[s0]
        [s1] "+r"(src_argb1),  // %[s1]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_ARGBSUBTRACTROW_LSX

#ifdef HAS_ARGBATTENUATEROW_LSX
void ARGBAttenuateRow_LSX(const uint8_t* src_argb,
                          uint8_t* dst_argb,
                          int width) {
  static const uint8_t kAlpha[16] = {3,  3,  3,  16, 7,  7,  7,  16,
                                     11, 11, 11, 16, 15, 15, 15, 16};
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "vldi             $vr5, 0xff                     \n"
      "vldi             $vr6, 255                      \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vshuf.b          $vr1, $vr5, $vr0, $vr4         \n"
      "vmulwev.h.bu     $vr2, $vr0, $vr1               \n"
      "vmulwod.h.bu     $vr3, $vr0, $vr1               \n"
      "vadd.h           $vr2, $vr2, $vr6               \n"
      "vadd.h           $vr3, $vr3, $vr6               \n"
      "vpackod.b        $vr2, $vr3, $vr2               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(kAlpha)     // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_ARGBATTENUATEROW_LSX

#ifdef HAS_ARGBUNATTENUATEROW_LSX
void ARGBUnattenuateRow_LSX(const uint8_t* src_argb,
                            uint8_t* dst_argb,
                            int width) {
  uint64_t t0, t1;
  asm volatile(
      "addi.w           %[cnt], %[cnt], -4             \n"
      "bltz             %[cnt], 2f                     \n"
      "vldi             $vr5, 0xff                     \n"
      "vslli.w          $vr5, $vr5, 24                 \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "ld.bu            %[t0], %[src], 3               \n"
      "ld.bu            %[t1], %[src], 7               \n"
      "slli.d           %[t0], %[t0], 2                \n"
      "slli.d           %[t1], %[t1], 2                \n"
      "ldx.wu           %[t0], %[tbl], %[t0]           \n"
      "ldx.wu           %[t1], %[tbl], %[t1]           \n"
      "vinsgr2vr.w      $vr3, %[t0], 0                 \n"
      "vinsgr2vr.w      $vr3, %[t1], 2                 \n"
      "vshuf4i.h        $vr3, $vr3, 0x40               \n"
      "ld.bu            %[t0], %[src], 11              \n"
      "ld.bu            %[t1], %[src], 15              \n"
      "slli.d           %[t0], %[t0], 2                \n"
      "slli.d           %[t1], %[t1], 2                \n"
      "ldx.wu           %[t0], %[tbl], %[t0]           \n"
      "ldx.wu           %[t1], %[tbl], %[t1]           \n"
      "vinsgr2vr.w      $vr4, %[t0], 0                 \n"
      "vinsgr2vr.w      $vr4, %[t1], 2                 \n"
      "vshuf4i.h        $vr4, $vr4, 0x40               \n"
      "vilvl.b          $vr1, $vr0, $vr0               \n"
      "vilvh.b          $vr2, $vr0, $vr0               \n"
      "vmuh.hu          $vr1, $vr1, $vr3               \n"
      "vmuh.hu          $vr2, $vr2, $vr4               \n"
      "vsat.hu          $vr1, $vr1, 7                  \n"
      "vsat.hu          $vr2, $vr2, 7                  \n"
      "vpickev.b        $vr1, $vr2, $vr1               \n"
      "vand.v           $vr0, $vr0, $vr5               \n"
      "vandn.v          $vr1, $vr5, $vr1               \n"
      "vor.v            $vr1, $vr1, $vr0               \n"
      "vst              $vr1, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.w           %[cnt], %[cnt], -4             \n"
      "bgez             %[cnt], 1b                     \n"

      "2:                                              \n"
      "addi.w           %[cnt], %[cnt], 4              \n"
      : [src] "+r"(src_argb),     // %[src]
        [dst] "+r"(dst_argb),     // %[dst]
        [cnt] "+r"(width),        // %[cnt]
        [t0] "=&r"(t0),           // %[t0]
        [t1] "=&r"(t1)            // %[t1]
      : [tbl] "r"(fixed_invtbl8)  // %[tbl]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
  if (width > 0) {
    ARGBUnattenuateRow_C(src_argb, dst_argb, width);
  }
}
#endif  // HAS_ARGBUNATTENUATEROW_LSX

#ifdef HAS_ARGBBLENDROW_LSX
void ARGBBlendRow_LSX(const uint8_t* src_argb0,
                      const uint8_t* src_argb1,
                      uint8_t* dst_argb,
                      int width) {
  static const uint8_t kAlpha[16] = {3,  3,  3,  3,  7,  7,  7,  7,
                                     11, 11, 11, 11, 15, 15, 15, 15};
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "vldi             $vr5, 0xff                     \n"
      "vslli.w          $vr5, $vr5, 24                 \n"
      "vldi             $vr6, 1                        \n"
      "vslli.h          $vr6, $vr6, 8                  \n"
      "vldi             $vr8, 0                        \n"

      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s1], 0                 \n"
      "vshuf.b          $vr2, $vr0, $vr0, $vr4         \n"
      "vilvl.b          $vr3, $vr8, $vr2               \n"
      "vilvh.b          $vr2, $vr8, $vr2               \n"
      "vsub.h           $vr3, $vr6, $vr3               \n"
      "vsub.h           $vr2, $vr6, $vr2               \n"
      "vilvl.b          $vr7, $vr8, $vr1               \n"
      "vilvh.b          $vr1, $vr8, $vr1               \n"
      "vmul.h           $vr7, $vr7, $vr3               \n"
      "vmul.h           $vr1, $vr1, $vr2               \n"
      "vsrlni.b.h       $vr1, $vr7, 8                  \n"
      "vsadd.bu         $vr0, $vr0, $vr1               \n"
      "vor.v            $vr0, $vr0, $vr5               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 16               \n"
      "addi.d           %[s1], %[s1], 16               \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb0),  // %[s0]
        [s1] "+r"(src_argb1),  // %[s1]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(kAlpha)     // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8");
}
#endif  // HAS_ARGBBLENDROW_LSX

#ifdef HAS_ARGBORROW_LSX
void ARGBOrRow_LSX(const uint8_t* src_argb,
                   uint8_t* dst_argb,
                   uint32_t value,
                   int width) {
  asm volatile(
      "vreplgr2vr.w     $vr1, %[val]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vor.v            $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [val] "r"(value)       // %[val]
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_ARGBORROW_LSX

#ifdef HAS_ARGBALPHABLENDROW_LSX
void ARGBAlphaBlendRow_LSX(const uint8_t* src_argb0,
                           const uint8_t* src_argb1,
                           uint8_t* dst_argb,
                           int width) {
  static const uint8_t kShufRatio[16] = {0, 0, 0, 0, 4,  4,  4,  4,
                                         8, 8, 8, 8, 12, 12, 12, 12};
  asm volatile(
      "vld              $vr8, %[shuf], 0               \n"
      "vldi             $vr9, 1                        \n"
      "vsrli.h          $vr10, $vr9, 8                 \n"
      "vldi             $vr11, 0xff                    \n"
      "vsrli.w          $vr11, $vr11, 8                \n"
      "vldi             $vr12, 0                       \n"

      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s1], 0                 \n"
      "vsrli.w          $vr2, $vr0, 24                 \n"
      "vsrli.w          $vr3, $vr1, 24                 \n"
      "vmul.h           $vr4, $vr3, $vr2               \n"
      "vadd.h           $vr4, $vr4, $vr10              \n"
      "vmuh.hu          $vr4, $vr4, $vr9               \n"
      "vsub.w           $vr4, $vr2, $vr4               \n"
      "vadd.w           $vr4, $vr3, $vr4               \n"
      "vseq.w           $vr5, $vr3, $vr12              \n"
      "vsrli.w          $vr5, $vr5, 24                 \n"
      "vor.v            $vr6, $vr2, $vr5               \n"
      "vor.v            $vr7, $vr4, $vr5               \n"
      "vslli.w          $vr5, $vr6, 8                  \n"
      "vsub.w           $vr5, $vr5, $vr6               \n"
      "vffint.s.wu      $vr5, $vr5                     \n"
      "vffint.s.wu      $vr7, $vr7                     \n"
      "vfdiv.s          $vr5, $vr5, $vr7               \n"
      "vftintrz.wu.s    $vr5, $vr5                     \n"
      "vshuf.b          $vr5, $vr5, $vr5, $vr8         \n"
      "vnor.v           $vr6, $vr5, $vr5               \n"
      "vmulwev.h.bu     $vr2, $vr1, $vr6               \n"
      "vmulwod.h.bu     $vr3, $vr1, $vr6               \n"
      "vmulwev.h.bu     $vr7, $vr0, $vr5               \n"
      "vmulwod.h.bu     $vr0, $vr0, $vr5               \n"
      "vadd.h           $vr2, $vr2, $vr7               \n"
      "vadd.h           $vr3, $vr3, $vr0               \n"
      "vadd.h           $vr2, $vr2, $vr10              \n"
      "vadd.h           $vr3, $vr3, $vr10              \n"
      "vmuh.hu          $vr2, $vr2, $vr9               \n"
      "vmuh.hu          $vr3, $vr3, $vr9               \n"
      "vslli.h          $vr3, $vr3, 8                  \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vand.v           $vr2, $vr2, $vr11              \n"
      "vslli.w          $vr4, $vr4, 24                 \n"
      "vor.v            $vr2, $vr2, $vr4               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 16               \n"
      "addi.d           %[s1], %[s1], 16               \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb0),   // %[s0]
        [s1] "+r"(src_argb1),   // %[s1]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [shuf] "r"(kShufRatio)  // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12");
}
#endif  // HAS_ARGBALPHABLENDROW_LSX

#ifdef HAS_ARGBSHADEROW_LSX
void ARGBShadeRow_LSX(const uint8_t* src_argb,
                      uint8_t* dst_argb,
                      int width,
                      uint32_t value) {
  asm volatile(
      "vreplgr2vr.w     $vr4, %[val]                   \n"
      "vilvl.b          $vr5, $vr4, $vr4               \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vilvl.b          $vr1, $vr0, $vr0               \n"
      "vilvh.b          $vr2, $vr0, $vr0               \n"
      "vmuh.hu          $vr1, $vr1, $vr5               \n"
      "vmuh.hu          $vr2, $vr2, $vr5               \n"
      "vsrlni.b.h       $vr2, $vr1, 8                  \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [val] "r"(value)       // %[val]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr4", "$vr5");
}
#endif  // HAS_ARGBSHADEROW_LSX

#ifdef HAS_AR64SHUFFLEROW_LSX
void AR64ShuffleRow_LSX(const uint8_t* src_ar64,
                        uint8_t* dst_ar64,
                        const uint8_t* shuffler,
                        int width) {
  asm volatile(
      "bge              $zero, %[cnt], 2f              \n"
      "vld              $vr4, %[shuf], 0               \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vshuf.b          $vr0, $vr0, $vr0, $vr4         \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -2             \n"
      "blt              $zero, %[cnt], 1b              \n"

      "2:                                              \n"
      : [src] "+r"(src_ar64),  // %[src]
        [dst] "+r"(dst_ar64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(shuffler)   // %[shuf]
      : "memory", "$vr0", "$vr4");
}
#endif  // HAS_AR64SHUFFLEROW_LSX

#ifdef HAS_ARGBSHUFFLEROW_LSX
void ARGBShuffleRow_LSX(const uint8_t* src_argb,
                        uint8_t* dst_argb,
                        const uint8_t* shuffler,
                        int width) {
  asm volatile(
      "bge              $zero, %[cnt], 2f              \n"
      "vld              $vr4, %[shuf], 0               \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vshuf.b          $vr0, $vr0, $vr0, $vr4         \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "blt              $zero, %[cnt], 1b              \n"

      "2:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(shuffler)   // %[shuf]
      : "memory", "$vr0", "$vr4");
}
#endif  // HAS_ARGBSHUFFLEROW_LSX

#ifdef HAS_ARGBGRAYROW_LSX
void ARGBGrayRow_LSX(const uint8_t* src_argb, uint8_t* dst_argb, int width) {
  asm volatile(
      "vldi             $vr4, 29                       \n"
      "vldi             $vr5, 150                      \n"
      "vldi             $vr6, 77                       \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vpickod.b        $vr3, $vr1, $vr0               \n"
      "vmulwev.h.bu     $vr7, $vr2, $vr4               \n"
      "vmaddwod.h.bu    $vr7, $vr2, $vr6               \n"
      "vmaddwev.h.bu    $vr7, $vr3, $vr5               \n"
      "vsrlri.h         $vr7, $vr7, 8                  \n"
      "vsrli.h          $vr3, $vr3, 8                  \n"
      "vslli.h          $vr3, $vr3, 8                  \n"
      "vor.v            $vr3, $vr7, $vr3               \n"
      "vslli.h          $vr8, $vr7, 8                  \n"
      "vor.v            $vr7, $vr7, $vr8               \n"
      "vilvl.h          $vr0, $vr3, $vr7               \n"
      "vilvh.h          $vr1, $vr3, $vr7               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8");
}
#endif  // HAS_ARGBGRAYROW_LSX

#ifdef HAS_ARGBSEPIAROW_LSX
void ARGBSepiaRow_LSX(uint8_t* dst_argb, int width) {
  asm volatile(
      "vldi             $vr4, 17                       \n"
      "vldi             $vr5, 68                       \n"
      "vldi             $vr6, 35                       \n"
      "vldi             $vr7, 22                       \n"
      "vldi             $vr8, 88                       \n"
      "vldi             $vr9, 45                       \n"
      "vldi             $vr10, 24                      \n"
      "vldi             $vr11, 98                      \n"
      "vldi             $vr12, 50                      \n"

      "1:                                              \n"
      "vld              $vr0, %[dst], 0                \n"
      "vld              $vr1, %[dst], 16               \n"
      "vpickev.b        $vr2, $vr1, $vr0               \n"
      "vpickod.b        $vr3, $vr1, $vr0               \n"
      "vmulwev.h.bu     $vr13, $vr2, $vr4              \n"
      "vmaddwod.h.bu    $vr13, $vr2, $vr6              \n"
      "vmaddwev.h.bu    $vr13, $vr3, $vr5              \n"
      "vmulwev.h.bu     $vr14, $vr2, $vr7              \n"
      "vmaddwod.h.bu    $vr14, $vr2, $vr9              \n"
      "vmaddwev.h.bu    $vr14, $vr3, $vr8              \n"
      "vmulwev.h.bu     $vr15, $vr2, $vr10             \n"
      "vmaddwod.h.bu    $vr15, $vr2, $vr12             \n"
      "vmaddwev.h.bu    $vr15, $vr3, $vr11             \n"
      "vsrli.h          $vr13, $vr13, 7                \n"
      "vsrli.h          $vr14, $vr14, 7                \n"
      "vsrli.h          $vr15, $vr15, 7                \n"
      "vsat.hu          $vr13, $vr13, 7                \n"
      "vsat.hu          $vr14, $vr14, 7                \n"
      "vsat.hu          $vr15, $vr15, 7                \n"
      "vpickev.b        $vr13, $vr13, $vr13            \n"
      "vpickev.b        $vr14, $vr14, $vr14            \n"
      "vpickev.b        $vr15, $vr15, $vr15            \n"
      "vpickod.b        $vr3, $vr3, $vr3               \n"
      "vilvl.b          $vr2, $vr14, $vr13             \n"
      "vilvl.b          $vr3, $vr3, $vr15              \n"
      "vilvl.h          $vr0, $vr3, $vr2               \n"
      "vilvh.h          $vr1, $vr3, $vr2               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr14",
        "$vr15");
}
#endif  // HAS_ARGBSEPIAROW_LSX

#ifdef HAS_ARGBCOLORMATRIXROW_LSX
void ARGBColorMatrixRow_LSX(const uint8_t* src_argb,
                            uint8_t* dst_argb,
                            const int8_t* matrix_argb,
                            int width) {
  asm volatile(
      "vldrepl.w        $vr4, %[mat], 0                \n"
      "vldrepl.w        $vr5, %[mat], 4                \n"
      "vldrepl.w        $vr6, %[mat], 8                \n"
      "vldrepl.w        $vr7, %[mat], 12               \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vmulwev.h.bu.b   $vr8, $vr0, $vr4               \n"
      "vmaddwod.h.bu.b  $vr8, $vr0, $vr4               \n"
      "vmulwev.h.bu.b   $vr9, $vr0, $vr5               \n"
      "vmaddwod.h.bu.b  $vr9, $vr0, $vr5               \n"
      "vmulwev.h.bu.b   $vr10, $vr0, $vr6              \n"
      "vmaddwod.h.bu.b  $vr10, $vr0, $vr6              \n"
      "vmulwev.h.bu.b   $vr11, $vr0, $vr7              \n"
      "vmaddwod.h.bu.b  $vr11, $vr0, $vr7              \n"
      "vhaddw.w.h       $vr8, $vr8, $vr8               \n"
      "vhaddw.w.h       $vr9, $vr9, $vr9               \n"
      "vhaddw.w.h       $vr10, $vr10, $vr10            \n"
      "vhaddw.w.h       $vr11, $vr11, $vr11            \n"
      "vilvl.w          $vr12, $vr9, $vr8              \n"
      "vilvh.w          $vr13, $vr9, $vr8              \n"
      "vilvl.w          $vr14, $vr11, $vr10            \n"
      "vilvh.w          $vr15, $vr11, $vr10            \n"
      "vilvl.d          $vr8, $vr14, $vr12             \n"
      "vilvh.d          $vr9, $vr14, $vr12             \n"
      "vilvl.d          $vr10, $vr15, $vr13            \n"
      "vilvh.d          $vr11, $vr15, $vr13            \n"
      "vssrani.h.w      $vr9, $vr8, 6                  \n"
      "vssrani.h.w      $vr11, $vr10, 6                \n"
      "vssrani.bu.h     $vr11, $vr9, 0                 \n"
      "vst              $vr11, %[dst], 0               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),   // %[src]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [mat] "r"(matrix_argb)  // %[mat]
      : "memory", "$vr0", "$vr4", "$vr5", "$vr6", "$vr7", "$vr8", "$vr9",
        "$vr10", "$vr11", "$vr12", "$vr13", "$vr14", "$vr15");
}
#endif  // HAS_ARGBCOLORMATRIXROW_LSX

#ifdef HAS_ARGBQUANTIZEROW_LSX
void ARGBQuantizeRow_LSX(uint8_t* dst_argb,
                         int scale,
                         int interval_size,
                         int interval_offset,
                         int width) {
  asm volatile(
      "vreplgr2vr.h     $vr4, %[scl]                   \n"
      "vreplgr2vr.b     $vr5, %[isz]                   \n"
      "vreplgr2vr.b     $vr6, %[ioff]                  \n"
      "vldi             $vr7, 0xff                     \n"
      "vslli.w          $vr7, $vr7, 24                 \n"

      "1:                                              \n"
      "vld              $vr0, %[dst], 0                \n"
      "vilvl.b          $vr1, $vr0, $vr0               \n"
      "vilvh.b          $vr2, $vr0, $vr0               \n"
      "vsrli.h          $vr1, $vr1, 8                  \n"
      "vsrli.h          $vr2, $vr2, 8                  \n"
      "vmuh.hu          $vr1, $vr1, $vr4               \n"
      "vmuh.hu          $vr2, $vr2, $vr4               \n"
      "vsrlni.b.h       $vr2, $vr1, 0                  \n"
      "vmul.b           $vr2, $vr2, $vr5               \n"
      "vadd.b           $vr2, $vr2, $vr6               \n"
      "vand.v           $vr0, $vr0, $vr7               \n"
      "vandn.v          $vr2, $vr7, $vr2               \n"
      "vor.v            $vr2, $vr2, $vr0               \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [dst] "+r"(dst_argb),        // %[dst]
        [cnt] "+r"(width)            // %[cnt]
      : [scl] "r"(scale),            // %[scl]
        [isz] "r"(interval_size),    // %[isz]
        [ioff] "r"(interval_offset)  // %[ioff]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr4", "$vr5", "$vr6", "$vr7");
}
#endif  // HAS_ARGBQUANTIZEROW_LSX

#ifdef HAS_SOBELROW_LSX
void SobelRow_LSX(const uint8_t* src_sobelx,
                  const uint8_t* src_sobely,
                  uint8_t* dst_argb,
                  int width) {
  asm volatile(
      "vldi             $vr4, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sx], 0                 \n"
      "vldrepl.d        $vr1, %[sy], 0                 \n"
      "vsadd.bu         $vr0, $vr0, $vr1               \n"
      "vilvl.b          $vr2, $vr0, $vr0               \n"
      "vilvl.b          $vr3, $vr4, $vr0               \n"
      "vilvl.h          $vr0, $vr3, $vr2               \n"
      "vilvh.h          $vr1, $vr3, $vr2               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[sx], %[sx], 8                \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sx] "+r"(src_sobelx),  // %[sx]
        [sy] "+r"(src_sobely),  // %[sy]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_SOBELROW_LSX

#ifdef HAS_SOBELTOPLANEROW_LSX
void SobelToPlaneRow_LSX(const uint8_t* src_sobelx,
                         const uint8_t* src_sobely,
                         uint8_t* dst_y,
                         int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[sx], 0                 \n"
      "vld              $vr1, %[sy], 0                 \n"
      "vsadd.bu         $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[sx], %[sx], 16               \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sx] "+r"(src_sobelx),  // %[sx]
        [sy] "+r"(src_sobely),  // %[sy]
        [dst] "+r"(dst_y),      // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_SOBELTOPLANEROW_LSX

#ifdef HAS_SOBELXYROW_LSX
void SobelXYRow_LSX(const uint8_t* src_sobelx,
                    const uint8_t* src_sobely,
                    uint8_t* dst_argb,
                    int width) {
  asm volatile(
      "vldi             $vr4, 0xff                     \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[sx], 0                 \n"
      "vldrepl.d        $vr1, %[sy], 0                 \n"
      "vsadd.bu         $vr2, $vr0, $vr1               \n"
      "vilvl.b          $vr1, $vr2, $vr1               \n"
      "vilvl.b          $vr0, $vr4, $vr0               \n"
      "vilvl.h          $vr2, $vr0, $vr1               \n"
      "vilvh.h          $vr3, $vr0, $vr1               \n"
      "vst              $vr2, %[dst], 0                \n"
      "vst              $vr3, %[dst], 16               \n"
      "addi.d           %[sx], %[sx], 8                \n"
      "addi.d           %[sy], %[sy], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sx] "+r"(src_sobelx),  // %[sx]
        [sy] "+r"(src_sobely),  // %[sy]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_SOBELXYROW_LSX

#ifdef HAS_INTERPOLATEROW_LSX
void InterpolateRow_LSX(uint8_t* dst_ptr,
                        const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        int width,
                        int32_t source_y_fraction) {
  int y1_fraction = source_y_fraction;
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  if (y1_fraction == 0) {
    CopyRow_LSX(src_ptr, dst_ptr, width);
    return;
  }
  if (y1_fraction == 128) {
    asm volatile(
        "1:                                              \n"
        "vld              $vr0, %[s0], 0                 \n"
        "vld              $vr1, %[s1], 0                 \n"
        "vavgr.bu         $vr0, $vr0, $vr1               \n"
        "vst              $vr0, %[dst], 0                \n"
        "addi.d           %[s0], %[s0], 16               \n"
        "addi.d           %[s1], %[s1], 16               \n"
        "addi.d           %[dst], %[dst], 16             \n"
        "addi.d           %[cnt], %[cnt], -16            \n"
        "bnez             %[cnt], 1b                     \n"
        : [s0] "+r"(src_ptr),   // %[s0]
          [s1] "+r"(src_ptr1),  // %[s1]
          [dst] "+r"(dst_ptr),  // %[dst]
          [cnt] "+r"(width)     // %[cnt]
        :
        : "memory", "$vr0", "$vr1");
    return;
  }
  uint16_t y_frac = (uint16_t)((256 - y1_fraction) + (y1_fraction << 8));
  asm volatile(
      "vreplgr2vr.h     $vr3, %[yf]                    \n"

      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s1], 0                 \n"
      "vilvl.b          $vr2, $vr1, $vr0               \n"
      "vilvh.b          $vr4, $vr1, $vr0               \n"
      "vmulwev.h.bu     $vr0, $vr2, $vr3               \n"
      "vmaddwod.h.bu    $vr0, $vr2, $vr3               \n"
      "vmulwev.h.bu     $vr1, $vr4, $vr3               \n"
      "vmaddwod.h.bu    $vr1, $vr4, $vr3               \n"
      "vsrlrni.b.h      $vr1, $vr0, 8                  \n"
      "vst              $vr1, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 16               \n"
      "addi.d           %[s1], %[s1], 16               \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_ptr),   // %[s0]
        [s1] "+r"(src_ptr1),  // %[s1]
        [dst] "+r"(dst_ptr),  // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      : [yf] "r"(y_frac)      // %[yf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_INTERPOLATEROW_LSX

#ifdef HAS_HALFFLOATROW_LSX
void HalfFloatRow_LSX(const uint16_t* src,
                      uint16_t* dst,
                      float scale,
                      int width) {
  const float bias = 1.9259299444e-34f;  // 2^-112, see HalfFloatRow_C
  asm volatile(
      "vldrepl.w        $vr3, %[scale], 0              \n"
      "vldrepl.w        $vr5, %[bias], 0               \n"
      "vldi             $vr4, 0                        \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vilvl.h          $vr1, $vr4, $vr0               \n"
      "vilvh.h          $vr2, $vr4, $vr0               \n"
      "vffint.s.wu      $vr1, $vr1                     \n"
      "vffint.s.wu      $vr2, $vr2                     \n"
      "vfmul.s          $vr1, $vr1, $vr3               \n"
      "vfmul.s          $vr2, $vr2, $vr3               \n"
      "vfmul.s          $vr1, $vr1, $vr5               \n"
      "vfmul.s          $vr2, $vr2, $vr5               \n"
      "vsrlni.h.w       $vr2, $vr1, 13                 \n"
      "vst              $vr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),      // %[src]
        [dst] "+r"(dst),      // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      : [scale] "r"(&scale),  // %[scale]
        [bias] "r"(&bias)     // %[bias]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_HALFFLOATROW_LSX

#ifdef HAS_BYTETOFLOATROW_LSX
void ByteToFloatRow_LSX(const uint8_t* src,
                        float* dst,
                        float scale,
                        int width) {
  uint32_t scale_bits;
  memcpy(&scale_bits, &scale, sizeof(scale_bits));
  asm volatile(
      "vreplgr2vr.w     $vr3, %[scale]                 \n"
      "vldi             $vr4, 0                        \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[src], 0                \n"
      "vilvl.b          $vr0, $vr4, $vr0               \n"
      "vilvl.h          $vr1, $vr4, $vr0               \n"
      "vilvh.h          $vr2, $vr4, $vr0               \n"
      "vffint.s.wu      $vr1, $vr1                     \n"
      "vffint.s.wu      $vr2, $vr2                     \n"
      "vfmul.s          $vr1, $vr1, $vr3               \n"
      "vfmul.s          $vr2, $vr2, $vr3               \n"
      "vst              $vr1, %[dst], 0                \n"
      "vst              $vr2, %[dst], 16               \n"
      "addi.d           %[src], %[src], 8              \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),         // %[src]
        [dst] "+r"(dst),         // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      : [scale] "r"(scale_bits)  // %[scale]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_BYTETOFLOATROW_LSX

#ifdef HAS_ARGBTOYMATRIXROW_LSX
void ARGBToYMatrixRow_LSX(const uint8_t* src_argb,
                          uint8_t* dst_y,
                          int width,
                          const struct ArgbConstants* c) {
  uint16_t add_y;
  __builtin_memcpy(&add_y, &c->kAddY[0], sizeof(add_y));
  asm volatile(
      "vldrepl.b        $vr0, %[c], 0                  \n"
      "vldrepl.b        $vr1, %[c], 1                  \n"
      "vldrepl.b        $vr2, %[c], 2                  \n"
      "vldrepl.b        $vr3, %[c], 3                  \n"
      "vreplgr2vr.h     $vr4, %[add_y]                 \n"

      "1:                                              \n"
      "vld              $vr5, %[src], 0                \n"
      "vld              $vr6, %[src], 16               \n"
      "vor.v            $vr9, $vr4, $vr4               \n"
      "vpickev.b        $vr7, $vr6, $vr5               \n"
      "vpickod.b        $vr8, $vr6, $vr5               \n"
      "vmaddwev.h.bu    $vr9, $vr7, $vr0               \n"
      "vmaddwev.h.bu    $vr9, $vr8, $vr1               \n"
      "vmaddwod.h.bu    $vr9, $vr7, $vr2               \n"
      "vmaddwod.h.bu    $vr9, $vr8, $vr3               \n"
      "vpickod.b        $vr9, $vr9, $vr9               \n"
      "vstelm.d         $vr9, %[dst], 0, 0             \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 8              \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_y] "r"(add_y)     // %[add_y]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9");
}
#endif  // HAS_ARGBTOYMATRIXROW_LSX

#ifdef HAS_ARGBTOUV444MATRIXROW_LSX
void ARGBToUV444MatrixRow_LSX(const uint8_t* src_argb,
                              uint8_t* dst_u,
                              uint8_t* dst_v,
                              int width,
                              const struct ArgbConstants* c) {
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV[0], sizeof(add_uv));
  asm volatile(
      "vldrepl.b        $vr0, %[c], 32                 \n"
      "vldrepl.b        $vr1, %[c], 33                 \n"
      "vldrepl.b        $vr2, %[c], 34                 \n"
      "vldrepl.b        $vr3, %[c], 35                 \n"
      "vldrepl.b        $vr10, %[c], 64                \n"
      "vldrepl.b        $vr11, %[c], 65                \n"
      "vldrepl.b        $vr12, %[c], 66                \n"
      "vldrepl.b        $vr13, %[c], 67                \n"
      "vreplgr2vr.h     $vr4, %[add_uv]                \n"

      "1:                                              \n"
      "vld              $vr5, %[src], 0                \n"
      "vld              $vr6, %[src], 16               \n"
      "vpickev.b        $vr7, $vr6, $vr5               \n"
      "vpickod.b        $vr8, $vr6, $vr5               \n"
      "vmulwev.h.bu.b   $vr9, $vr7, $vr0               \n"
      "vmaddwev.h.bu.b  $vr9, $vr8, $vr1               \n"
      "vmaddwod.h.bu.b  $vr9, $vr7, $vr2               \n"
      "vmaddwod.h.bu.b  $vr9, $vr8, $vr3               \n"
      "vsub.h           $vr9, $vr4, $vr9               \n"
      "vmulwev.h.bu.b   $vr14, $vr7, $vr10             \n"
      "vmaddwev.h.bu.b  $vr14, $vr8, $vr11             \n"
      "vmaddwod.h.bu.b  $vr14, $vr7, $vr12             \n"
      "vmaddwod.h.bu.b  $vr14, $vr8, $vr13             \n"
      "vsub.h           $vr14, $vr4, $vr14             \n"
      "vpickod.b        $vr9, $vr14, $vr9              \n"
      "vstelm.d         $vr9, %[du], 0, 0              \n"
      "vstelm.d         $vr9, %[dv], 0, 1              \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_uv] "r"(add_uv)   // %[add_uv]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr14");
}
#endif  // HAS_ARGBTOUV444MATRIXROW_LSX

#ifdef HAS_ARGBTOUVMATRIXROW_LSX
void ARGBToUVMatrixRow_LSX(const uint8_t* src_argb,
                           int src_stride_argb,
                           uint8_t* dst_u,
                           uint8_t* dst_v,
                           int width,
                           const struct ArgbConstants* c) {
  const uint8_t* next_argb = src_argb + src_stride_argb;
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV[0], sizeof(add_uv));
  asm volatile(
      "vldrepl.b        $vr0, %[c], 32                 \n"
      "vldrepl.b        $vr1, %[c], 33                 \n"
      "vldrepl.b        $vr2, %[c], 34                 \n"
      "vldrepl.b        $vr3, %[c], 35                 \n"
      "vldrepl.b        $vr10, %[c], 64                \n"
      "vldrepl.b        $vr11, %[c], 65                \n"
      "vldrepl.b        $vr12, %[c], 66                \n"
      "vldrepl.b        $vr13, %[c], 67                \n"
      "vreplgr2vr.h     $vr4, %[add_uv]                \n"

      "1:                                              \n"
      "vld              $vr14, %[s0], 0                \n"
      "vld              $vr15, %[s0], 16               \n"
      "vld              $vr16, %[s1], 0                \n"
      "vld              $vr17, %[s1], 16               \n"
      "vpickev.w        $vr18, $vr15, $vr14            \n"
      "vpickod.w        $vr19, $vr15, $vr14            \n"
      "vpickev.w        $vr20, $vr17, $vr16            \n"
      "vpickod.w        $vr21, $vr17, $vr16            \n"
      "vaddwev.h.bu     $vr14, $vr18, $vr19            \n"
      "vaddwod.h.bu     $vr15, $vr18, $vr19            \n"
      "vaddwev.h.bu     $vr16, $vr20, $vr21            \n"
      "vaddwod.h.bu     $vr17, $vr20, $vr21            \n"
      "vadd.h           $vr14, $vr14, $vr16            \n"
      "vadd.h           $vr15, $vr15, $vr17            \n"
      "vsrari.h         $vr14, $vr14, 2                \n"
      "vsrari.h         $vr15, $vr15, 2                \n"
      "vpackev.b        $vr5, $vr15, $vr14             \n"
      "vld              $vr14, %[s0], 32               \n"
      "vld              $vr15, %[s0], 48               \n"
      "vld              $vr16, %[s1], 32               \n"
      "vld              $vr17, %[s1], 48               \n"
      "vpickev.w        $vr18, $vr15, $vr14            \n"
      "vpickod.w        $vr19, $vr15, $vr14            \n"
      "vpickev.w        $vr20, $vr17, $vr16            \n"
      "vpickod.w        $vr21, $vr17, $vr16            \n"
      "vaddwev.h.bu     $vr14, $vr18, $vr19            \n"
      "vaddwod.h.bu     $vr15, $vr18, $vr19            \n"
      "vaddwev.h.bu     $vr16, $vr20, $vr21            \n"
      "vaddwod.h.bu     $vr17, $vr20, $vr21            \n"
      "vadd.h           $vr14, $vr14, $vr16            \n"
      "vadd.h           $vr15, $vr15, $vr17            \n"
      "vsrari.h         $vr14, $vr14, 2                \n"
      "vsrari.h         $vr15, $vr15, 2                \n"
      "vpackev.b        $vr6, $vr15, $vr14             \n"
      "vpickev.b        $vr7, $vr6, $vr5               \n"
      "vpickod.b        $vr8, $vr6, $vr5               \n"
      "vmulwev.h.bu.b   $vr9, $vr7, $vr0               \n"
      "vmaddwev.h.bu.b  $vr9, $vr8, $vr1               \n"
      "vmaddwod.h.bu.b  $vr9, $vr7, $vr2               \n"
      "vmaddwod.h.bu.b  $vr9, $vr8, $vr3               \n"
      "vsub.h           $vr9, $vr4, $vr9               \n"
      "vmulwev.h.bu.b   $vr14, $vr7, $vr10             \n"
      "vmaddwev.h.bu.b  $vr14, $vr8, $vr11             \n"
      "vmaddwod.h.bu.b  $vr14, $vr7, $vr12             \n"
      "vmaddwod.h.bu.b  $vr14, $vr8, $vr13             \n"
      "vsub.h           $vr14, $vr4, $vr14             \n"
      "vpickod.b        $vr9, $vr14, $vr9              \n"
      "vstelm.d         $vr9, %[du], 0, 0              \n"
      "vstelm.d         $vr9, %[dv], 0, 1              \n"
      "addi.d           %[s0], %[s0], 64               \n"
      "addi.d           %[s1], %[s1], 64               \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb),   // %[s0]
        [s1] "+r"(next_argb),  // %[s1]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_uv] "r"(add_uv)   // %[add_uv]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr14",
        "$vr15", "$vr16", "$vr17", "$vr18", "$vr19", "$vr20", "$vr21");
}
#endif  // HAS_ARGBTOUVMATRIXROW_LSX

#ifdef HAS_RGBTOYMATRIXROW_LSX
void RGBToYMatrixRow_LSX(const uint8_t* src_rgba,
                         uint8_t* dst_y,
                         int width,
                         const struct ArgbConstants* c) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                    0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                    9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  uint16_t add_y;
  __builtin_memcpy(&add_y, &c->kAddY[0], sizeof(add_y));
  asm volatile(
      "vldrepl.b        $vr0, %[c], 0                  \n"
      "vldrepl.b        $vr1, %[c], 1                  \n"
      "vldrepl.b        $vr2, %[c], 2                  \n"
      "vreplgr2vr.h     $vr3, %[add_y]                 \n"
      "vld              $vr4, %[shuf], 0               \n"
      "vld              $vr5, %[shuf], 16              \n"

      "1:                                              \n"
      "vld              $vr6, %[src], 0                \n"
      "vld              $vr7, %[src], 12               \n"
      "vld              $vr8, %[src], 24               \n"
      "vld              $vr9, %[src], 32               \n"
      "vshuf.b          $vr6, $vr6, $vr6, $vr4         \n"
      "vshuf.b          $vr7, $vr7, $vr7, $vr4         \n"
      "vshuf.b          $vr8, $vr8, $vr8, $vr4         \n"
      "vshuf.b          $vr9, $vr9, $vr9, $vr5         \n"
      "vor.v            $vr12, $vr3, $vr3              \n"
      "vor.v            $vr13, $vr3, $vr3              \n"
      "vpickev.b        $vr10, $vr7, $vr6              \n"
      "vpickod.b        $vr11, $vr7, $vr6              \n"
      "vmaddwev.h.bu    $vr12, $vr10, $vr0             \n"
      "vmaddwev.h.bu    $vr12, $vr11, $vr1             \n"
      "vmaddwod.h.bu    $vr12, $vr10, $vr2             \n"
      "vpickev.b        $vr10, $vr9, $vr8              \n"
      "vpickod.b        $vr11, $vr9, $vr8              \n"
      "vmaddwev.h.bu    $vr13, $vr10, $vr0             \n"
      "vmaddwev.h.bu    $vr13, $vr11, $vr1             \n"
      "vmaddwod.h.bu    $vr13, $vr10, $vr2             \n"
      "vpickod.b        $vr12, $vr13, $vr12            \n"
      "vst              $vr12, %[dst], 0               \n"
      "addi.d           %[src], %[src], 48             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgba),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_y] "r"(add_y),    // %[add_y]
        [shuf] "r"(kShuf)      // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13");
}
#endif  // HAS_RGBTOYMATRIXROW_LSX

#ifdef HAS_RGBTOUV444MATRIXROW_LSX
void RGBToUV444MatrixRow_LSX(const uint8_t* src_rgb,
                             uint8_t* dst_u,
                             uint8_t* dst_v,
                             int width,
                             const struct ArgbConstants* c) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                    0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                    9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV[0], sizeof(add_uv));
  asm volatile(
      "vldrepl.b        $vr0, %[c], 32                 \n"
      "vldrepl.b        $vr1, %[c], 33                 \n"
      "vldrepl.b        $vr2, %[c], 34                 \n"
      "vldrepl.b        $vr14, %[c], 64                \n"
      "vldrepl.b        $vr15, %[c], 65                \n"
      "vldrepl.b        $vr16, %[c], 66                \n"
      "vreplgr2vr.h     $vr3, %[add_uv]                \n"
      "vld              $vr4, %[shuf], 0               \n"
      "vld              $vr5, %[shuf], 16              \n"

      "1:                                              \n"
      "vld              $vr6, %[src], 0                \n"
      "vld              $vr7, %[src], 12               \n"
      "vld              $vr8, %[src], 24               \n"
      "vld              $vr9, %[src], 32               \n"
      "vshuf.b          $vr6, $vr6, $vr6, $vr4         \n"
      "vshuf.b          $vr7, $vr7, $vr7, $vr4         \n"
      "vshuf.b          $vr8, $vr8, $vr8, $vr4         \n"
      "vshuf.b          $vr9, $vr9, $vr9, $vr5         \n"
      "vpickev.b        $vr10, $vr7, $vr6              \n"
      "vpickod.b        $vr11, $vr7, $vr6              \n"
      "vmulwev.h.bu.b   $vr12, $vr10, $vr0             \n"
      "vmaddwev.h.bu.b  $vr12, $vr11, $vr1             \n"
      "vmaddwod.h.bu.b  $vr12, $vr10, $vr2             \n"
      "vsub.h           $vr12, $vr3, $vr12             \n"
      "vmulwev.h.bu.b   $vr17, $vr10, $vr14            \n"
      "vmaddwev.h.bu.b  $vr17, $vr11, $vr15            \n"
      "vmaddwod.h.bu.b  $vr17, $vr10, $vr16            \n"
      "vsub.h           $vr17, $vr3, $vr17             \n"
      "vpickev.b        $vr10, $vr9, $vr8              \n"
      "vpickod.b        $vr11, $vr9, $vr8              \n"
      "vmulwev.h.bu.b   $vr13, $vr10, $vr0             \n"
      "vmaddwev.h.bu.b  $vr13, $vr11, $vr1             \n"
      "vmaddwod.h.bu.b  $vr13, $vr10, $vr2             \n"
      "vsub.h           $vr13, $vr3, $vr13             \n"
      "vmulwev.h.bu.b   $vr18, $vr10, $vr14            \n"
      "vmaddwev.h.bu.b  $vr18, $vr11, $vr15            \n"
      "vmaddwod.h.bu.b  $vr18, $vr10, $vr16            \n"
      "vsub.h           $vr18, $vr3, $vr18             \n"
      "vpickod.b        $vr12, $vr13, $vr12            \n"
      "vpickod.b        $vr17, $vr18, $vr17            \n"
      "vst              $vr12, %[du], 0                \n"
      "vst              $vr17, %[dv], 0                \n"
      "addi.d           %[src], %[src], 48             \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb),   // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_uv] "r"(add_uv),  // %[add_uv]
        [shuf] "r"(kShuf)      // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr14",
        "$vr15", "$vr16", "$vr17", "$vr18");
}
#endif  // HAS_RGBTOUV444MATRIXROW_LSX

#ifdef HAS_RGBTOUVMATRIXROW_LSX
void RGBToUVMatrixRow_LSX(const uint8_t* src_rgb,
                          int src_stride_rgb,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width,
                          const struct ArgbConstants* c) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                    0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                    9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  const uint8_t* next_rgb = src_rgb + src_stride_rgb;
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV[0], sizeof(add_uv));
  asm volatile(
      "vldrepl.b        $vr20, %[c], 32                \n"
      "vsrai.h          $vr20, $vr20, 8                \n"
      "vldrepl.b        $vr21, %[c], 33                \n"
      "vsrai.h          $vr21, $vr21, 8                \n"
      "vldrepl.b        $vr22, %[c], 34                \n"
      "vsrai.h          $vr22, $vr22, 8                \n"
      "vldrepl.b        $vr23, %[c], 64                \n"
      "vsrai.h          $vr23, $vr23, 8                \n"
      "vldrepl.b        $vr24, %[c], 65                \n"
      "vsrai.h          $vr24, $vr24, 8                \n"
      "vldrepl.b        $vr25, %[c], 66                \n"
      "vsrai.h          $vr25, $vr25, 8                \n"
      "vreplgr2vr.h     $vr26, %[add_uv]               \n"
      "vld              $vr16, %[shuf], 0              \n"
      "vld              $vr17, %[shuf], 16             \n"

      "1:                                              \n"
      "vld              $vr8, %[s0], 0                 \n"
      "vld              $vr9, %[s0], 12                \n"
      "vld              $vr10, %[s0], 24               \n"
      "vld              $vr11, %[s0], 32               \n"
      "vshuf.b          $vr8, $vr8, $vr8, $vr16        \n"
      "vshuf.b          $vr9, $vr9, $vr9, $vr16        \n"
      "vshuf.b          $vr10, $vr10, $vr10, $vr16     \n"
      "vshuf.b          $vr11, $vr11, $vr11, $vr17     \n"
      "vpickev.b        $vr12, $vr9, $vr8              \n"
      "vpickod.b        $vr13, $vr9, $vr8              \n"
      "vpickev.b        $vr14, $vr11, $vr10            \n"
      "vpickod.b        $vr15, $vr11, $vr10            \n"
      "vpickev.b        $vr0, $vr14, $vr12             \n"
      "vpickod.b        $vr2, $vr14, $vr12             \n"
      "vpickev.b        $vr1, $vr15, $vr13             \n"
      "vld              $vr8, %[s1], 0                 \n"
      "vld              $vr9, %[s1], 12                \n"
      "vld              $vr10, %[s1], 24               \n"
      "vld              $vr11, %[s1], 32               \n"
      "vshuf.b          $vr8, $vr8, $vr8, $vr16        \n"
      "vshuf.b          $vr9, $vr9, $vr9, $vr16        \n"
      "vshuf.b          $vr10, $vr10, $vr10, $vr16     \n"
      "vshuf.b          $vr11, $vr11, $vr11, $vr17     \n"
      "vpickev.b        $vr12, $vr9, $vr8              \n"
      "vpickod.b        $vr13, $vr9, $vr8              \n"
      "vpickev.b        $vr14, $vr11, $vr10            \n"
      "vpickod.b        $vr15, $vr11, $vr10            \n"
      "vpickev.b        $vr3, $vr14, $vr12             \n"
      "vpickod.b        $vr5, $vr14, $vr12             \n"
      "vpickev.b        $vr4, $vr15, $vr13             \n"
      "vhaddw.hu.bu     $vr0, $vr0, $vr0               \n"
      "vhaddw.hu.bu     $vr3, $vr3, $vr3               \n"
      "vadd.h           $vr0, $vr0, $vr3               \n"
      "vsrari.h         $vr0, $vr0, 2                  \n"
      "vhaddw.hu.bu     $vr1, $vr1, $vr1               \n"
      "vhaddw.hu.bu     $vr4, $vr4, $vr4               \n"
      "vadd.h           $vr1, $vr1, $vr4               \n"
      "vsrari.h         $vr1, $vr1, 2                  \n"
      "vhaddw.hu.bu     $vr2, $vr2, $vr2               \n"
      "vhaddw.hu.bu     $vr5, $vr5, $vr5               \n"
      "vadd.h           $vr2, $vr2, $vr5               \n"
      "vsrari.h         $vr2, $vr2, 2                  \n"
      "vor.v            $vr6, $vr26, $vr26             \n"
      "vor.v            $vr7, $vr26, $vr26             \n"
      "vmsub.h          $vr6, $vr0, $vr20              \n"
      "vmsub.h          $vr6, $vr1, $vr21              \n"
      "vmsub.h          $vr6, $vr2, $vr22              \n"
      "vmsub.h          $vr7, $vr0, $vr23              \n"
      "vmsub.h          $vr7, $vr1, $vr24              \n"
      "vmsub.h          $vr7, $vr2, $vr25              \n"
      "vpickod.b        $vr6, $vr7, $vr6               \n"
      "vstelm.d         $vr6, %[du], 0, 0              \n"
      "vstelm.d         $vr6, %[dv], 0, 1              \n"
      "addi.d           %[s0], %[s0], 48               \n"
      "addi.d           %[s1], %[s1], 48               \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_rgb),    // %[s0]
        [s1] "+r"(next_rgb),   // %[s1]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_uv] "r"(add_uv),  // %[add_uv]
        [shuf] "r"(kShuf)      // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr14",
        "$vr15", "$vr16", "$vr17", "$vr20", "$vr21", "$vr22", "$vr23", "$vr24",
        "$vr25", "$vr26");
}
#endif  // HAS_RGBTOUVMATRIXROW_LSX

#ifdef HAS_RGB565TOYROW_LSX
void RGB565ToYRow_LSX(const uint8_t* src_rgb565, uint8_t* dst_y, int width) {
  asm volatile(
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"
      "vldi             $vr7, 63                       \n"
      "vsrli.h          $vr7, $vr7, 8                  \n"
      "vldi             $vr8, 25                       \n"
      "vsrli.h          $vr8, $vr8, 8                  \n"
      "vldi             $vr9, 129                      \n"
      "vsrli.h          $vr9, $vr9, 8                  \n"
      "vldi             $vr10, 66                      \n"
      "vsrli.h          $vr10, $vr10, 8                \n"
      "vldi             $vr11, 33                      \n"
      "vsrli.h          $vr11, $vr11, 8                \n"
      "vslli.h          $vr11, $vr11, 7                \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr7               \n"
      "vsrli.h          $vr3, $vr0, 11                 \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 2                  \n"
      "vsrli.h          $vr2, $vr2, 4                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vmul.h           $vr1, $vr1, $vr8               \n"
      "vmadd.h          $vr1, $vr2, $vr9               \n"
      "vmadd.h          $vr1, $vr3, $vr10              \n"
      "vadd.h           $vr1, $vr1, $vr11              \n"
      "vpickod.b        $vr1, $vr1, $vr1               \n"
      "vstelm.d         $vr1, %[dst], 0, 0             \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 8              \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb565),  // %[src]
        [dst] "+r"(dst_y),       // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr6", "$vr7",
        "$vr8", "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_RGB565TOYROW_LSX

#ifdef HAS_ARGB1555TOYROW_LSX
void ARGB1555ToYRow_LSX(const uint8_t* src_argb1555,
                        uint8_t* dst_y,
                        int width) {
  asm volatile(
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"
      "vldi             $vr8, 25                       \n"
      "vsrli.h          $vr8, $vr8, 8                  \n"
      "vldi             $vr9, 129                      \n"
      "vsrli.h          $vr9, $vr9, 8                  \n"
      "vldi             $vr10, 66                      \n"
      "vsrli.h          $vr10, $vr10, 8                \n"
      "vldi             $vr11, 33                      \n"
      "vsrli.h          $vr11, $vr11, 8                \n"
      "vslli.h          $vr11, $vr11, 7                \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vsrli.h          $vr3, $vr0, 10                 \n"
      "vand.v           $vr3, $vr3, $vr6               \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 3                  \n"
      "vsrli.h          $vr2, $vr2, 2                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vmul.h           $vr1, $vr1, $vr8               \n"
      "vmadd.h          $vr1, $vr2, $vr9               \n"
      "vmadd.h          $vr1, $vr3, $vr10              \n"
      "vadd.h           $vr1, $vr1, $vr11              \n"
      "vpickod.b        $vr1, $vr1, $vr1               \n"
      "vstelm.d         $vr1, %[dst], 0, 0             \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 8              \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb1555),  // %[src]
        [dst] "+r"(dst_y),         // %[dst]
        [cnt] "+r"(width)          // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr6", "$vr8",
        "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_ARGB1555TOYROW_LSX

#ifdef HAS_RGB565TOUVROW_LSX
void RGB565ToUVRow_LSX(const uint8_t* src_rgb565,
                       int src_stride_rgb565,
                       uint8_t* dst_u,
                       uint8_t* dst_v,
                       int width) {
  const uint8_t* next_rgb565 = src_rgb565 + src_stride_rgb565;
  asm volatile(
      "vldi             $vr20, 112                     \n"
      "vsrli.h          $vr20, $vr20, 8                \n"
      "vldi             $vr21, 74                      \n"
      "vsrli.h          $vr21, $vr21, 8                \n"
      "vldi             $vr22, 38                      \n"
      "vsrli.h          $vr22, $vr22, 8                \n"
      "vldi             $vr23, 94                      \n"
      "vsrli.h          $vr23, $vr23, 8                \n"
      "vldi             $vr24, 18                      \n"
      "vsrli.h          $vr24, $vr24, 8                \n"
      "vldi             $vr25, 128                     \n"
      "vsrli.h          $vr25, $vr25, 8                \n"
      "vslli.h          $vr25, $vr25, 8                \n"
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"
      "vldi             $vr7, 63                       \n"
      "vsrli.h          $vr7, $vr7, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr7               \n"
      "vsrli.h          $vr3, $vr0, 11                 \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 2                  \n"
      "vsrli.h          $vr2, $vr2, 4                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vor.v            $vr10, $vr1, $vr1              \n"
      "vor.v            $vr11, $vr2, $vr2              \n"
      "vor.v            $vr12, $vr3, $vr3              \n"
      "vld              $vr0, %[s0], 16                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr7               \n"
      "vsrli.h          $vr3, $vr0, 11                 \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 2                  \n"
      "vsrli.h          $vr2, $vr2, 4                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vpickev.b        $vr10, $vr1, $vr10             \n"
      "vpickev.b        $vr11, $vr2, $vr11             \n"
      "vpickev.b        $vr12, $vr3, $vr12             \n"
      "vld              $vr0, %[s1], 0                 \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr7               \n"
      "vsrli.h          $vr3, $vr0, 11                 \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 2                  \n"
      "vsrli.h          $vr2, $vr2, 4                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vor.v            $vr13, $vr1, $vr1              \n"
      "vor.v            $vr14, $vr2, $vr2              \n"
      "vor.v            $vr15, $vr3, $vr3              \n"
      "vld              $vr0, %[s1], 16                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr7               \n"
      "vsrli.h          $vr3, $vr0, 11                 \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 2                  \n"
      "vsrli.h          $vr2, $vr2, 4                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vpickev.b        $vr5, $vr3, $vr15              \n"
      "vpickev.b        $vr4, $vr2, $vr14              \n"
      "vpickev.b        $vr3, $vr1, $vr13              \n"
      "vor.v            $vr0, $vr10, $vr10             \n"
      "vor.v            $vr1, $vr11, $vr11             \n"
      "vor.v            $vr2, $vr12, $vr12             \n"
      "vhaddw.hu.bu     $vr0, $vr0, $vr0               \n"
      "vhaddw.hu.bu     $vr3, $vr3, $vr3               \n"
      "vadd.h           $vr0, $vr0, $vr3               \n"
      "vsrari.h         $vr0, $vr0, 2                  \n"
      "vhaddw.hu.bu     $vr1, $vr1, $vr1               \n"
      "vhaddw.hu.bu     $vr4, $vr4, $vr4               \n"
      "vadd.h           $vr1, $vr1, $vr4               \n"
      "vsrari.h         $vr1, $vr1, 2                  \n"
      "vhaddw.hu.bu     $vr2, $vr2, $vr2               \n"
      "vhaddw.hu.bu     $vr5, $vr5, $vr5               \n"
      "vadd.h           $vr2, $vr2, $vr5               \n"
      "vsrari.h         $vr2, $vr2, 2                  \n"
      "vor.v            $vr8, $vr25, $vr25             \n"
      "vor.v            $vr9, $vr25, $vr25             \n"
      "vmadd.h          $vr8, $vr0, $vr20              \n"
      "vmsub.h          $vr8, $vr1, $vr21              \n"
      "vmsub.h          $vr8, $vr2, $vr22              \n"
      "vmadd.h          $vr9, $vr2, $vr20              \n"
      "vmsub.h          $vr9, $vr1, $vr23              \n"
      "vmsub.h          $vr9, $vr0, $vr24              \n"
      "vpickod.b        $vr8, $vr9, $vr8               \n"
      "vstelm.d         $vr8, %[du], 0, 0              \n"
      "vstelm.d         $vr8, %[dv], 0, 1              \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_rgb565),   // %[s0]
        [s1] "+r"(next_rgb565),  // %[s1]
        [du] "+r"(dst_u),        // %[du]
        [dv] "+r"(dst_v),        // %[dv]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr14",
        "$vr15", "$vr20", "$vr21", "$vr22", "$vr23", "$vr24", "$vr25");
}
#endif  // HAS_RGB565TOUVROW_LSX

#ifdef HAS_ARGB1555TOUVROW_LSX
void ARGB1555ToUVRow_LSX(const uint8_t* src_argb1555,
                         int src_stride_argb1555,
                         uint8_t* dst_u,
                         uint8_t* dst_v,
                         int width) {
  const uint8_t* next_argb1555 = src_argb1555 + src_stride_argb1555;
  asm volatile(
      "vldi             $vr20, 112                     \n"
      "vsrli.h          $vr20, $vr20, 8                \n"
      "vldi             $vr21, 74                      \n"
      "vsrli.h          $vr21, $vr21, 8                \n"
      "vldi             $vr22, 38                      \n"
      "vsrli.h          $vr22, $vr22, 8                \n"
      "vldi             $vr23, 94                      \n"
      "vsrli.h          $vr23, $vr23, 8                \n"
      "vldi             $vr24, 18                      \n"
      "vsrli.h          $vr24, $vr24, 8                \n"
      "vldi             $vr25, 128                     \n"
      "vsrli.h          $vr25, $vr25, 8                \n"
      "vslli.h          $vr25, $vr25, 8                \n"
      "vldi             $vr6, 31                       \n"
      "vsrli.h          $vr6, $vr6, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vsrli.h          $vr3, $vr0, 10                 \n"
      "vand.v           $vr3, $vr3, $vr6               \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 3                  \n"
      "vsrli.h          $vr2, $vr2, 2                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vor.v            $vr10, $vr1, $vr1              \n"
      "vor.v            $vr11, $vr2, $vr2              \n"
      "vor.v            $vr12, $vr3, $vr3              \n"
      "vld              $vr0, %[s0], 16                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vsrli.h          $vr3, $vr0, 10                 \n"
      "vand.v           $vr3, $vr3, $vr6               \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 3                  \n"
      "vsrli.h          $vr2, $vr2, 2                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vpickev.b        $vr10, $vr1, $vr10             \n"
      "vpickev.b        $vr11, $vr2, $vr11             \n"
      "vpickev.b        $vr12, $vr3, $vr12             \n"
      "vld              $vr0, %[s1], 0                 \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vsrli.h          $vr3, $vr0, 10                 \n"
      "vand.v           $vr3, $vr3, $vr6               \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 3                  \n"
      "vsrli.h          $vr2, $vr2, 2                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vor.v            $vr13, $vr1, $vr1              \n"
      "vor.v            $vr14, $vr2, $vr2              \n"
      "vor.v            $vr15, $vr3, $vr3              \n"
      "vld              $vr0, %[s1], 16                \n"
      "vand.v           $vr1, $vr0, $vr6               \n"
      "vsrli.h          $vr2, $vr0, 5                  \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vsrli.h          $vr3, $vr0, 10                 \n"
      "vand.v           $vr3, $vr3, $vr6               \n"
      "vslli.h          $vr4, $vr1, 3                  \n"
      "vsrli.h          $vr1, $vr1, 2                  \n"
      "vor.v            $vr1, $vr4, $vr1               \n"
      "vslli.h          $vr4, $vr2, 3                  \n"
      "vsrli.h          $vr2, $vr2, 2                  \n"
      "vor.v            $vr2, $vr4, $vr2               \n"
      "vslli.h          $vr4, $vr3, 3                  \n"
      "vsrli.h          $vr3, $vr3, 2                  \n"
      "vor.v            $vr3, $vr4, $vr3               \n"
      "vpickev.b        $vr5, $vr3, $vr15              \n"
      "vpickev.b        $vr4, $vr2, $vr14              \n"
      "vpickev.b        $vr3, $vr1, $vr13              \n"
      "vor.v            $vr0, $vr10, $vr10             \n"
      "vor.v            $vr1, $vr11, $vr11             \n"
      "vor.v            $vr2, $vr12, $vr12             \n"
      "vhaddw.hu.bu     $vr0, $vr0, $vr0               \n"
      "vhaddw.hu.bu     $vr3, $vr3, $vr3               \n"
      "vadd.h           $vr0, $vr0, $vr3               \n"
      "vsrari.h         $vr0, $vr0, 2                  \n"
      "vhaddw.hu.bu     $vr1, $vr1, $vr1               \n"
      "vhaddw.hu.bu     $vr4, $vr4, $vr4               \n"
      "vadd.h           $vr1, $vr1, $vr4               \n"
      "vsrari.h         $vr1, $vr1, 2                  \n"
      "vhaddw.hu.bu     $vr2, $vr2, $vr2               \n"
      "vhaddw.hu.bu     $vr5, $vr5, $vr5               \n"
      "vadd.h           $vr2, $vr2, $vr5               \n"
      "vsrari.h         $vr2, $vr2, 2                  \n"
      "vor.v            $vr8, $vr25, $vr25             \n"
      "vor.v            $vr9, $vr25, $vr25             \n"
      "vmadd.h          $vr8, $vr0, $vr20              \n"
      "vmsub.h          $vr8, $vr1, $vr21              \n"
      "vmsub.h          $vr8, $vr2, $vr22              \n"
      "vmadd.h          $vr9, $vr2, $vr20              \n"
      "vmsub.h          $vr9, $vr1, $vr23              \n"
      "vmsub.h          $vr9, $vr0, $vr24              \n"
      "vpickod.b        $vr8, $vr9, $vr8               \n"
      "vstelm.d         $vr8, %[du], 0, 0              \n"
      "vstelm.d         $vr8, %[dv], 0, 1              \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb1555),   // %[s0]
        [s1] "+r"(next_argb1555),  // %[s1]
        [du] "+r"(dst_u),          // %[du]
        [dv] "+r"(dst_v),          // %[dv]
        [cnt] "+r"(width)          // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr14", "$vr15",
        "$vr20", "$vr21", "$vr22", "$vr23", "$vr24", "$vr25");
}
#endif  // HAS_ARGB1555TOUVROW_LSX

#if defined(HAS_RGB24TOUVROW_LSX) || defined(HAS_RAWTOUVROW_LSX)
static LIBYUV_NOINLINE void RGB24ToUVHelper_LSX(const uint8_t* src,
                                                int stride,
                                                uint8_t* dst_u,
                                                uint8_t* dst_v,
                                                const uint8_t* shuf,
                                                int width) {
  const uint8_t* next = src + stride;
  asm volatile(
      "vldi             $vr20, 112                     \n"
      "vsrli.h          $vr20, $vr20, 8                \n"
      "vldi             $vr21, 74                      \n"
      "vsrli.h          $vr21, $vr21, 8                \n"
      "vldi             $vr22, 38                      \n"
      "vsrli.h          $vr22, $vr22, 8                \n"
      "vldi             $vr23, 94                      \n"
      "vsrli.h          $vr23, $vr23, 8                \n"
      "vldi             $vr24, 18                      \n"
      "vsrli.h          $vr24, $vr24, 8                \n"
      "vldi             $vr25, 128                     \n"
      "vsrli.h          $vr25, $vr25, 8                \n"
      "vslli.h          $vr25, $vr25, 8                \n"
      "vld              $vr16, %[shuf], 0              \n"
      "vld              $vr17, %[shuf], 16             \n"

      "1:                                              \n"
      "vld              $vr8, %[s0], 0                 \n"
      "vld              $vr9, %[s0], 12                \n"
      "vld              $vr10, %[s0], 24               \n"
      "vld              $vr11, %[s0], 32               \n"
      "vshuf.b          $vr8, $vr8, $vr8, $vr16        \n"
      "vshuf.b          $vr9, $vr9, $vr9, $vr16        \n"
      "vshuf.b          $vr10, $vr10, $vr10, $vr16     \n"
      "vshuf.b          $vr11, $vr11, $vr11, $vr17     \n"
      "vpickev.b        $vr12, $vr9, $vr8              \n"
      "vpickod.b        $vr13, $vr9, $vr8              \n"
      "vpickev.b        $vr14, $vr11, $vr10            \n"
      "vpickod.b        $vr15, $vr11, $vr10            \n"
      "vpickev.b        $vr0, $vr14, $vr12             \n"
      "vpickod.b        $vr2, $vr14, $vr12             \n"
      "vpickev.b        $vr1, $vr15, $vr13             \n"
      "vld              $vr8, %[s1], 0                 \n"
      "vld              $vr9, %[s1], 12                \n"
      "vld              $vr10, %[s1], 24               \n"
      "vld              $vr11, %[s1], 32               \n"
      "vshuf.b          $vr8, $vr8, $vr8, $vr16        \n"
      "vshuf.b          $vr9, $vr9, $vr9, $vr16        \n"
      "vshuf.b          $vr10, $vr10, $vr10, $vr16     \n"
      "vshuf.b          $vr11, $vr11, $vr11, $vr17     \n"
      "vpickev.b        $vr12, $vr9, $vr8              \n"
      "vpickod.b        $vr13, $vr9, $vr8              \n"
      "vpickev.b        $vr14, $vr11, $vr10            \n"
      "vpickod.b        $vr15, $vr11, $vr10            \n"
      "vpickev.b        $vr3, $vr14, $vr12             \n"
      "vpickod.b        $vr5, $vr14, $vr12             \n"
      "vpickev.b        $vr4, $vr15, $vr13             \n"
      "vhaddw.hu.bu     $vr0, $vr0, $vr0               \n"
      "vhaddw.hu.bu     $vr3, $vr3, $vr3               \n"
      "vadd.h           $vr0, $vr0, $vr3               \n"
      "vsrari.h         $vr0, $vr0, 2                  \n"
      "vhaddw.hu.bu     $vr1, $vr1, $vr1               \n"
      "vhaddw.hu.bu     $vr4, $vr4, $vr4               \n"
      "vadd.h           $vr1, $vr1, $vr4               \n"
      "vsrari.h         $vr1, $vr1, 2                  \n"
      "vhaddw.hu.bu     $vr2, $vr2, $vr2               \n"
      "vhaddw.hu.bu     $vr5, $vr5, $vr5               \n"
      "vadd.h           $vr2, $vr2, $vr5               \n"
      "vsrari.h         $vr2, $vr2, 2                  \n"
      "vor.v            $vr6, $vr25, $vr25             \n"
      "vor.v            $vr7, $vr25, $vr25             \n"
      "vmadd.h          $vr6, $vr0, $vr20              \n"
      "vmsub.h          $vr6, $vr1, $vr21              \n"
      "vmsub.h          $vr6, $vr2, $vr22              \n"
      "vmadd.h          $vr7, $vr2, $vr20              \n"
      "vmsub.h          $vr7, $vr1, $vr23              \n"
      "vmsub.h          $vr7, $vr0, $vr24              \n"
      "vpickod.b        $vr6, $vr7, $vr6               \n"
      "vstelm.d         $vr6, %[du], 0, 0              \n"
      "vstelm.d         $vr6, %[dv], 0, 1              \n"
      "addi.d           %[s0], %[s0], 48               \n"
      "addi.d           %[s1], %[s1], 48               \n"
      "addi.d           %[du], %[du], 8                \n"
      "addi.d           %[dv], %[dv], 8                \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src),    // %[s0]
        [s1] "+r"(next),   // %[s1]
        [du] "+r"(dst_u),  // %[du]
        [dv] "+r"(dst_v),  // %[dv]
        [cnt] "+r"(width)  // %[cnt]
      : [shuf] "r"(shuf)   // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13", "$vr14",
        "$vr15", "$vr16", "$vr17", "$vr20", "$vr21", "$vr22", "$vr23", "$vr24",
        "$vr25");
}
#endif  // defined(HAS_RGB24TOUVROW_LSX) || defined(HAS_RAWTOUVROW_LSX)

#ifdef HAS_RGB24TOUVROW_LSX
void RGB24ToUVRow_LSX(const uint8_t* src_rgb24,
                      int src_stride_rgb24,
                      uint8_t* dst_u,
                      uint8_t* dst_v,
                      int width) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                    0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                    9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  RGB24ToUVHelper_LSX(src_rgb24, src_stride_rgb24, dst_u, dst_v, kShuf, width);
}
#endif  // HAS_RGB24TOUVROW_LSX

#ifdef HAS_RAWTOUVROW_LSX
void RAWToUVRow_LSX(const uint8_t* src_raw,
                    int src_stride_raw,
                    uint8_t* dst_u,
                    uint8_t* dst_v,
                    int width) {
  static const uint8_t kShuf[32] = {2, 1,  0,  0,  5,  4, 3,  0,  8,  7, 6,
                                    0, 11, 10, 9,  0,  6, 5,  4,  0,  9, 8,
                                    7, 0,  12, 11, 10, 0, 15, 14, 13, 0};
  RGB24ToUVHelper_LSX(src_raw, src_stride_raw, dst_u, dst_v, kShuf, width);
}
#endif  // HAS_RAWTOUVROW_LSX

#ifdef HAS_SWAPUVROW_LSX
void SwapUVRow_LSX(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vshuf4i.b        $vr0, $vr0, 0xb1               \n"
      "vshuf4i.b        $vr1, $vr1, 0xb1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [dst] "+r"(dst_vu),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_SWAPUVROW_LSX

#ifdef HAS_ARGBCOPYYTOALPHAROW_LSX
void ARGBCopyYToAlphaRow_LSX(const uint8_t* src, uint8_t* dst, int width) {
  asm volatile(
      "vldi             $vr4, 0xff                     \n"
      "vslli.w          $vr4, $vr4, 24                 \n"

      "1:                                              \n"
      "vldrepl.d        $vr0, %[src], 0                \n"
      "vld              $vr1, %[dst], 0                \n"
      "vld              $vr2, %[dst], 16               \n"
      "vilvl.b          $vr0, $vr0, $vr0               \n"
      "vilvh.h          $vr3, $vr0, $vr0               \n"
      "vilvl.h          $vr0, $vr0, $vr0               \n"
      "vslli.w          $vr0, $vr0, 24                 \n"
      "vslli.w          $vr3, $vr3, 24                 \n"
      "vandn.v          $vr1, $vr4, $vr1               \n"
      "vandn.v          $vr2, $vr4, $vr2               \n"
      "vor.v            $vr1, $vr1, $vr0               \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vst              $vr1, %[dst], 0                \n"
      "vst              $vr2, %[dst], 16               \n"
      "addi.d           %[src], %[src], 8              \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),   // %[src]
        [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(width)  // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_ARGBCOPYYTOALPHAROW_LSX

#ifdef HAS_HALFMERGEUVROW_LSX
void HalfMergeUVRow_LSX(const uint8_t* src_u,
                        int src_stride_u,
                        const uint8_t* src_v,
                        int src_stride_v,
                        uint8_t* dst_uv,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[su], 0                 \n"
      "vld              $vr1, %[sv], 0                 \n"
      "vldx             $vr2, %[su], %[stu]            \n"
      "vldx             $vr3, %[sv], %[stv]            \n"
      "vhaddw.hu.bu     $vr0, $vr0, $vr0               \n"
      "vhaddw.hu.bu     $vr1, $vr1, $vr1               \n"
      "vhaddw.hu.bu     $vr2, $vr2, $vr2               \n"
      "vhaddw.hu.bu     $vr3, $vr3, $vr3               \n"
      "vadd.h           $vr0, $vr0, $vr2               \n"
      "vadd.h           $vr1, $vr1, $vr3               \n"
      "vsrari.h         $vr0, $vr0, 2                  \n"
      "vsrari.h         $vr1, $vr1, 2                  \n"
      "vpackev.b        $vr0, $vr1, $vr0               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[su], %[su], 16               \n"
      "addi.d           %[sv], %[sv], 16               \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [su] "+r"(src_u),                    // %[su]
        [sv] "+r"(src_v),                    // %[sv]
        [dst] "+r"(dst_uv),                  // %[dst]
        [cnt] "+r"(width)                    // %[cnt]
      : [stu] "r"((ptrdiff_t)src_stride_u),  // %[stu]
        [stv] "r"((ptrdiff_t)src_stride_v)   // %[stv]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_HALFMERGEUVROW_LSX

#ifdef HAS_HALFUVROW_LSX
void HalfUVRow_LSX(const uint8_t* src_u,
                   ptrdiff_t src_stride_u,
                   const uint8_t* src_v,
                   ptrdiff_t src_stride_v,
                   uint8_t* dst_uv,
                   int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[su], 0                 \n"
      "vldx             $vr2, %[su], %[stu]            \n"
      "vavgr.bu         $vr0, $vr0, $vr2               \n"
      "vld              $vr1, %[sv], 0                 \n"
      "vldx             $vr3, %[sv], %[stv]            \n"
      "vavgr.bu         $vr1, $vr1, $vr3               \n"
      "vilvl.b          $vr2, $vr1, $vr0               \n"
      "vilvh.b          $vr3, $vr1, $vr0               \n"
      "vst              $vr2, %[dst], 0                \n"
      "vst              $vr3, %[dst], 16               \n"
      "addi.d           %[su], %[su], 16               \n"
      "addi.d           %[sv], %[sv], 16               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [su] "+r"(src_u),         // %[su]
        [sv] "+r"(src_v),         // %[sv]
        [dst] "+r"(dst_uv),       // %[dst]
        [cnt] "+r"(width)         // %[cnt]
      : [stu] "r"(src_stride_u),  // %[stu]
        [stv] "r"(src_stride_v)   // %[stv]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_HALFUVROW_LSX

#ifdef HAS_CONVERT16TO8ROW_LSX
void Convert16To8Row_LSX(const uint16_t* src_y,
                         uint8_t* dst_y,
                         int scale,
                         int width) {
  asm volatile(
      "vreplgr2vr.h     $vr2, %[scl]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vmuh.hu          $vr0, $vr0, $vr2               \n"
      "vmuh.hu          $vr1, $vr1, $vr2               \n"
      "vssrlni.bu.h     $vr1, $vr0, 0                  \n"
      "vst              $vr1, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [scl] "r"(scale)    // %[scl]
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_CONVERT16TO8ROW_LSX

#ifdef HAS_CONVERT8TO16ROW_LSX
void Convert8To16Row_LSX(const uint8_t* src_y,
                         uint16_t* dst_y,
                         int bits,
                         int width) {
  const int shift = 16 - bits;
  asm volatile(
      "vreplgr2vr.h     $vr3, %[shf]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vilvl.b          $vr1, $vr0, $vr0               \n"
      "vilvh.b          $vr2, $vr0, $vr0               \n"
      "vsrl.h           $vr1, $vr1, $vr3               \n"
      "vsrl.h           $vr2, $vr2, $vr3               \n"
      "vst              $vr1, %[dst], 0                \n"
      "vst              $vr2, %[dst], 16               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [shf] "r"(shift)    // %[shf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_CONVERT8TO16ROW_LSX

#ifdef HAS_MULTIPLYROW_16_LSX
void MultiplyRow_16_LSX(const uint16_t* src_y,
                        uint16_t* dst_y,
                        int scale,
                        int width) {
  asm volatile(
      "vreplgr2vr.h     $vr2, %[scl]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vmul.h           $vr0, $vr0, $vr2               \n"
      "vmul.h           $vr1, $vr1, $vr2               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [scl] "r"(scale)    // %[scl]
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_MULTIPLYROW_16_LSX

#ifdef HAS_DIVIDEROW_16_LSX
void DivideRow_16_LSX(const uint16_t* src_y,
                      uint16_t* dst_y,
                      int scale,
                      int width) {
  if (scale == 65536) {  // Divide by 1, so do memcpy.
    if (src_y != dst_y) {
      memcpy(dst_y, src_y, (size_t)width * 2);
    }
    return;
  }
  asm volatile(
      "vreplgr2vr.h     $vr2, %[scl]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vmuh.hu          $vr0, $vr0, $vr2               \n"
      "vmuh.hu          $vr1, $vr1, $vr2               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [scl] "r"(scale)    // %[scl]
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_DIVIDEROW_16_LSX

#ifdef HAS_MERGEUVROW_16_LSX
void MergeUVRow_16_LSX(const uint16_t* src_u,
                       const uint16_t* src_v,
                       uint16_t* dst_uv,
                       int depth,
                       int width) {
  int shift = 16 - depth;
  asm volatile(
      "vreplgr2vr.h     $vr4, %[shf]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[su], 0                 \n"
      "vld              $vr1, %[sv], 0                 \n"
      "vsll.h           $vr0, $vr0, $vr4               \n"
      "vsll.h           $vr1, $vr1, $vr4               \n"
      "vilvl.h          $vr2, $vr1, $vr0               \n"
      "vilvh.h          $vr3, $vr1, $vr0               \n"
      "vst              $vr2, %[dst], 0                \n"
      "vst              $vr3, %[dst], 16               \n"
      "addi.d           %[su], %[su], 16               \n"
      "addi.d           %[sv], %[sv], 16               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [su] "+r"(src_u),    // %[su]
        [sv] "+r"(src_v),    // %[sv]
        [dst] "+r"(dst_uv),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      : [shf] "r"(shift)     // %[shf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_MERGEUVROW_16_LSX

#ifdef HAS_SPLITUVROW_16_LSX
void SplitUVRow_16_LSX(const uint16_t* src_uv,
                       uint16_t* dst_u,
                       uint16_t* dst_v,
                       int depth,
                       int width) {
  int shift = 16 - depth;
  asm volatile(
      "vreplgr2vr.h     $vr4, %[shf]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickev.h        $vr2, $vr1, $vr0               \n"
      "vpickod.h        $vr3, $vr1, $vr0               \n"
      "vsrl.h           $vr2, $vr2, $vr4               \n"
      "vsrl.h           $vr3, $vr3, $vr4               \n"
      "vst              $vr2, %[du], 0                 \n"
      "vst              $vr3, %[dv], 0                 \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [du] "+r"(dst_u),    // %[du]
        [dv] "+r"(dst_v),    // %[dv]
        [cnt] "+r"(width)    // %[cnt]
      : [shf] "r"(shift)     // %[shf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_SPLITUVROW_16_LSX

#ifdef HAS_COPYROW_16_LSX
void CopyRow_16_LSX(const uint16_t* src, uint16_t* dst, int count) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),   // %[src]
        [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(count)  // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_COPYROW_16_LSX

#ifdef HAS_HALFROW_LSX
void HalfRow_LSX(const uint8_t* src_uv,
                 ptrdiff_t src_uv_stride,
                 uint8_t* dst_uv,
                 int width) {
  const uint8_t* src_uv1 = src_uv + src_uv_stride;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src0], 0               \n"
      "vld              $vr1, %[src1], 0               \n"
      "vavgr.bu         $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 16           \n"
      "addi.d           %[src1], %[src1], 16           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_HALFROW_LSX

#ifdef HAS_HALFROW_16_LSX
void HalfRow_16_LSX(const uint16_t* src_uv,
                    ptrdiff_t src_uv_stride,
                    uint16_t* dst_uv,
                    int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src0], 0               \n"
      "vld              $vr1, %[src1], 0               \n"
      "vavgr.hu         $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 16           \n"
      "addi.d           %[src1], %[src1], 16           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_HALFROW_16_LSX

#ifdef HAS_ARGBCOPYALPHAROW_LSX
void ARGBCopyAlphaRow_LSX(const uint8_t* src, uint8_t* dst, int width) {
  asm volatile(
      "vldi             $vr4, 0xff                     \n"
      "vslli.w          $vr4, $vr4, 24                 \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr2, %[dst], 0                \n"
      "vld              $vr3, %[dst], 16               \n"
      "vbitsel.v        $vr2, $vr2, $vr0, $vr4         \n"
      "vbitsel.v        $vr3, $vr3, $vr1, $vr4         \n"
      "vst              $vr2, %[dst], 0                \n"
      "vst              $vr3, %[dst], 16               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),   // %[src]
        [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(width)  // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_ARGBCOPYALPHAROW_LSX

#ifdef HAS_YUY2TONVUVROW_LSX
void YUY2ToNVUVRow_LSX(const uint8_t* src_yuy2,
                       int src_stride_yuy2,
                       uint8_t* dst_uv,
                       int width) {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src0], 0               \n"
      "vld              $vr1, %[src0], 16              \n"
      "vld              $vr2, %[src1], 0               \n"
      "vld              $vr3, %[src1], 16              \n"
      "vpickod.b        $vr0, $vr1, $vr0               \n"
      "vpickod.b        $vr2, $vr3, $vr2               \n"
      "vavgr.bu         $vr0, $vr0, $vr2               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 32           \n"
      "addi.d           %[src1], %[src1], 32           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_yuy2),    // %[src0]
        [src1] "+r"(src_yuy2_1),  // %[src1]
        [dst] "+r"(dst_uv),       // %[dst]
        [cnt] "+r"(width)         // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_YUY2TONVUVROW_LSX

#ifdef HAS_AR64TOARGBROW_LSX
void AR64ToARGBRow_LSX(const uint16_t* src_ar64,
                       uint8_t* dst_argb,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickod.b        $vr0, $vr1, $vr0               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ar64),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_AR64TOARGBROW_LSX

#ifdef HAS_AB64TOARGBROW_LSX
void AB64ToARGBRow_LSX(const uint16_t* src_ab64,
                       uint8_t* dst_argb,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vpickod.b        $vr0, $vr1, $vr0               \n"
      "vshuf4i.b        $vr0, $vr0, 0xc6               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ab64),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_AB64TOARGBROW_LSX

#ifdef HAS_ARGBTOAR64ROW_LSX
void ARGBToAR64Row_LSX(const uint8_t* src_argb,
                       uint16_t* dst_ar64,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vilvl.b          $vr1, $vr0, $vr0               \n"
      "vilvh.b          $vr2, $vr0, $vr0               \n"
      "vst              $vr1, %[dst], 0                \n"
      "vst              $vr2, %[dst], 16               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_ar64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_ARGBTOAR64ROW_LSX

#ifdef HAS_ARGBTOAB64ROW_LSX
void ARGBToAB64Row_LSX(const uint8_t* src_argb,
                       uint16_t* dst_ab64,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vshuf4i.b        $vr0, $vr0, 0xc6               \n"
      "vilvl.b          $vr1, $vr0, $vr0               \n"
      "vilvh.b          $vr2, $vr0, $vr0               \n"
      "vst              $vr1, %[dst], 0                \n"
      "vst              $vr2, %[dst], 16               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_ab64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_ARGBTOAB64ROW_LSX

#ifdef HAS_CONVERT8TO8ROW_LSX
void Convert8To8Row_LSX(const uint8_t* src_y,
                        uint8_t* dst_y,
                        int scale,
                        int bias,
                        int width) {
  asm volatile(
      "vreplgr2vr.b     $vr2, %[scl]                   \n"
      "vreplgr2vr.b     $vr3, %[bia]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vmuh.bu          $vr0, $vr0, $vr2               \n"
      "vadd.b           $vr0, $vr0, $vr3               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [scl] "r"(scale),   // %[scl]
        [bia] "r"(bias)     // %[bia]
      : "memory", "$vr0", "$vr2", "$vr3");
}
#endif  // HAS_CONVERT8TO8ROW_LSX

#ifdef HAS_HALFROW_16TO8_LSX
void HalfRow_16To8_LSX(const uint16_t* src_uv,
                       ptrdiff_t src_uv_stride,
                       uint8_t* dst_uv,
                       int scale,
                       int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  asm volatile(
      "vreplgr2vr.h     $vr4, %[scl]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src0], 0               \n"
      "vld              $vr1, %[src0], 16              \n"
      "vld              $vr2, %[src1], 0               \n"
      "vld              $vr3, %[src1], 16              \n"
      "vavgr.hu         $vr0, $vr0, $vr2               \n"
      "vavgr.hu         $vr1, $vr1, $vr3               \n"
      "vmuh.hu          $vr0, $vr0, $vr4               \n"
      "vmuh.hu          $vr1, $vr1, $vr4               \n"
      "vssrlni.bu.h     $vr1, $vr0, 0                  \n"
      "vst              $vr1, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 32           \n"
      "addi.d           %[src1], %[src1], 32           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [scl] "r"(scale)       // %[scl]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_HALFROW_16TO8_LSX

#ifdef HAS_AR30TOAB30ROW_LSX
void AR30ToAB30Row_LSX(const uint8_t* src_ar30,
                       uint8_t* dst_ab30,
                       int width) {
  uint32_t m_ag = 0xc00ffc00u;
  uint32_t m_r = 0x3ff00000u;
  uint32_t m_b = 0x000003ffu;
  asm volatile(
      "vreplgr2vr.w     $vr3, %[m_ag]                  \n"
      "vreplgr2vr.w     $vr4, %[m_r]                   \n"
      "vreplgr2vr.w     $vr5, %[m_b]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vsrli.w          $vr1, $vr0, 20                 \n"
      "vslli.w          $vr2, $vr0, 20                 \n"
      "vand.v           $vr0, $vr0, $vr3               \n"
      "vand.v           $vr1, $vr1, $vr5               \n"
      "vand.v           $vr2, $vr2, $vr4               \n"
      "vor.v            $vr0, $vr0, $vr1               \n"
      "vor.v            $vr0, $vr0, $vr2               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ar30),  // %[src]
        [dst] "+r"(dst_ab30),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_ag] "r"(m_ag),      // %[m_ag]
        [m_r] "r"(m_r),        // %[m_r]
        [m_b] "r"(m_b)         // %[m_b]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_AR30TOAB30ROW_LSX

#ifdef HAS_ARGBTOAR30ROW_LSX
void ARGBToAR30Row_LSX(const uint8_t* src_argb,
                       uint8_t* dst_ar30,
                       int width) {
  uint32_t m_a = 0xc0000000u;
  uint32_t m_r = 0x3fc00000u;
  uint32_t m_g = 0x000ff000u;
  uint32_t m_b = 0x000003fcu;
  uint32_t m_l = 0x00300c03u;
  asm volatile(
      "vreplgr2vr.w     $vr4, %[m_a]                   \n"
      "vreplgr2vr.w     $vr5, %[m_r]                   \n"
      "vreplgr2vr.w     $vr6, %[m_g]                   \n"
      "vreplgr2vr.w     $vr7, %[m_b]                   \n"
      "vreplgr2vr.w     $vr8, %[m_l]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vslli.w          $vr1, $vr0, 6                  \n"
      "vslli.w          $vr2, $vr0, 4                  \n"
      "vslli.w          $vr3, $vr0, 2                  \n"
      "vand.v           $vr0, $vr0, $vr4               \n"
      "vand.v           $vr1, $vr1, $vr5               \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vand.v           $vr3, $vr3, $vr7               \n"
      "vor.v            $vr0, $vr0, $vr1               \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vor.v            $vr0, $vr0, $vr2               \n"
      "vsrli.w          $vr1, $vr0, 8                  \n"
      "vand.v           $vr1, $vr1, $vr8               \n"
      "vor.v            $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_ar30),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_a] "r"(m_a),        // %[m_a]
        [m_r] "r"(m_r),        // %[m_r]
        [m_g] "r"(m_g),        // %[m_g]
        [m_b] "r"(m_b),        // %[m_b]
        [m_l] "r"(m_l)         // %[m_l]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8");
}
#endif  // HAS_ARGBTOAR30ROW_LSX

#ifdef HAS_ABGRTOAR30ROW_LSX
void ABGRToAR30Row_LSX(const uint8_t* src_abgr,
                       uint8_t* dst_ar30,
                       int width) {
  uint32_t m_a = 0xc0000000u;
  uint32_t m_r = 0x3fc00000u;
  uint32_t m_g = 0x000ff000u;
  uint32_t m_b = 0x000003fcu;
  uint32_t m_l = 0x00300c03u;
  asm volatile(
      "vreplgr2vr.w     $vr4, %[m_a]                   \n"
      "vreplgr2vr.w     $vr5, %[m_r]                   \n"
      "vreplgr2vr.w     $vr6, %[m_g]                   \n"
      "vreplgr2vr.w     $vr7, %[m_b]                   \n"
      "vreplgr2vr.w     $vr8, %[m_l]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vshuf4i.b        $vr0, $vr0, 0xc6               \n"
      "vslli.w          $vr1, $vr0, 6                  \n"
      "vslli.w          $vr2, $vr0, 4                  \n"
      "vslli.w          $vr3, $vr0, 2                  \n"
      "vand.v           $vr0, $vr0, $vr4               \n"
      "vand.v           $vr1, $vr1, $vr5               \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vand.v           $vr3, $vr3, $vr7               \n"
      "vor.v            $vr0, $vr0, $vr1               \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vor.v            $vr0, $vr0, $vr2               \n"
      "vsrli.w          $vr1, $vr0, 8                  \n"
      "vand.v           $vr1, $vr1, $vr8               \n"
      "vor.v            $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_abgr),  // %[src]
        [dst] "+r"(dst_ar30),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_a] "r"(m_a),        // %[m_a]
        [m_r] "r"(m_r),        // %[m_r]
        [m_g] "r"(m_g),        // %[m_g]
        [m_b] "r"(m_b),        // %[m_b]
        [m_l] "r"(m_l)         // %[m_l]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8");
}
#endif  // HAS_ABGRTOAR30ROW_LSX

#ifdef HAS_AR30TOARGBROW_LSX
void AR30ToARGBRow_LSX(const uint8_t* src_ar30,
                       uint8_t* dst_argb,
                       int width) {
  uint32_t m_a = 0xc0000000u;
  uint32_t m_2 = 0x300000ffu;
  uint32_t m_4 = 0x0c00ff00u;
  uint32_t m_6 = 0x03ff0000u;
  asm volatile(
      "vreplgr2vr.w     $vr4, %[m_a]                   \n"
      "vreplgr2vr.w     $vr5, %[m_2]                   \n"
      "vreplgr2vr.w     $vr6, %[m_4]                   \n"
      "vreplgr2vr.w     $vr7, %[m_6]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vsrli.w          $vr1, $vr0, 2                  \n"
      "vsrli.w          $vr2, $vr0, 4                  \n"
      "vsrli.w          $vr3, $vr0, 6                  \n"
      "vand.v           $vr0, $vr0, $vr4               \n"
      "vand.v           $vr1, $vr1, $vr5               \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vand.v           $vr3, $vr3, $vr7               \n"
      "vor.v            $vr0, $vr0, $vr1               \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vor.v            $vr0, $vr0, $vr2               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ar30),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_a] "r"(m_a),        // %[m_a]
        [m_2] "r"(m_2),        // %[m_2]
        [m_4] "r"(m_4),        // %[m_4]
        [m_6] "r"(m_6)         // %[m_6]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_AR30TOARGBROW_LSX

#ifdef HAS_AR30TOABGRROW_LSX
void AR30ToABGRRow_LSX(const uint8_t* src_ar30,
                       uint8_t* dst_abgr,
                       int width) {
  uint32_t m_a = 0xc0000000u;
  uint32_t m_2 = 0x300000ffu;
  uint32_t m_4 = 0x0c00ff00u;
  uint32_t m_6 = 0x03ff0000u;
  asm volatile(
      "vreplgr2vr.w     $vr4, %[m_a]                   \n"
      "vreplgr2vr.w     $vr5, %[m_2]                   \n"
      "vreplgr2vr.w     $vr6, %[m_4]                   \n"
      "vreplgr2vr.w     $vr7, %[m_6]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vsrli.w          $vr1, $vr0, 2                  \n"
      "vsrli.w          $vr2, $vr0, 4                  \n"
      "vsrli.w          $vr3, $vr0, 6                  \n"
      "vand.v           $vr0, $vr0, $vr4               \n"
      "vand.v           $vr1, $vr1, $vr5               \n"
      "vand.v           $vr2, $vr2, $vr6               \n"
      "vand.v           $vr3, $vr3, $vr7               \n"
      "vor.v            $vr0, $vr0, $vr1               \n"
      "vor.v            $vr2, $vr2, $vr3               \n"
      "vor.v            $vr0, $vr0, $vr2               \n"
      "vshuf4i.b        $vr0, $vr0, 0xc6               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ar30),  // %[src]
        [dst] "+r"(dst_abgr),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_a] "r"(m_a),        // %[m_a]
        [m_2] "r"(m_2),        // %[m_2]
        [m_4] "r"(m_4),        // %[m_4]
        [m_6] "r"(m_6)         // %[m_6]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_AR30TOABGRROW_LSX

#ifdef HAS_AYUVTOYROW_LSX
void AYUVToYRow_LSX(const uint8_t* src_ayuv, uint8_t* dst_y, int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr2, %[src], 32               \n"
      "vld              $vr3, %[src], 48               \n"
      "vpickod.h        $vr0, $vr1, $vr0               \n"
      "vpickod.h        $vr2, $vr3, $vr2               \n"
      "vpickev.b        $vr0, $vr2, $vr0               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ayuv),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_AYUVTOYROW_LSX

#ifdef HAS_AYUVTOUVROW_LSX
void AYUVToUVRow_LSX(const uint8_t* src_ayuv,
                     int src_stride_ayuv,
                     uint8_t* dst_uv,
                     int width) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src0], 0               \n"
      "vld              $vr1, %[src0], 16              \n"
      "vld              $vr2, %[src0], 32              \n"
      "vld              $vr3, %[src0], 48              \n"
      "vpickev.h        $vr0, $vr1, $vr0               \n"
      "vpickev.h        $vr2, $vr3, $vr2               \n"
      "vpickev.h        $vr1, $vr2, $vr0               \n"
      "vpickod.h        $vr0, $vr2, $vr0               \n"
      "vld              $vr2, %[src1], 0               \n"
      "vld              $vr3, %[src1], 16              \n"
      "vld              $vr4, %[src1], 32              \n"
      "vld              $vr5, %[src1], 48              \n"
      "vpickev.h        $vr2, $vr3, $vr2               \n"
      "vpickev.h        $vr4, $vr5, $vr4               \n"
      "vpickev.h        $vr3, $vr4, $vr2               \n"
      "vpickod.h        $vr2, $vr4, $vr2               \n"
      "vaddwev.h.bu     $vr4, $vr1, $vr0               \n"
      "vaddwod.h.bu     $vr5, $vr1, $vr0               \n"
      "vaddwev.h.bu     $vr0, $vr3, $vr2               \n"
      "vaddwod.h.bu     $vr1, $vr3, $vr2               \n"
      "vadd.h           $vr0, $vr4, $vr0               \n"
      "vadd.h           $vr1, $vr5, $vr1               \n"
      "vsrari.h         $vr0, $vr0, 2                  \n"
      "vsrari.h         $vr1, $vr1, 2                  \n"
      "vpackev.b        $vr0, $vr0, $vr1               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 64           \n"
      "addi.d           %[src1], %[src1], 64           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_ayuv),   // %[src0]
        [src1] "+r"(src_ayuv1),  // %[src1]
        [dst] "+r"(dst_uv),      // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_AYUVTOUVROW_LSX

#ifdef HAS_AYUVTOVUROW_LSX
void AYUVToVURow_LSX(const uint8_t* src_ayuv,
                     int src_stride_ayuv,
                     uint8_t* dst_vu,
                     int width) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src0], 0               \n"
      "vld              $vr1, %[src0], 16              \n"
      "vld              $vr2, %[src0], 32              \n"
      "vld              $vr3, %[src0], 48              \n"
      "vpickev.h        $vr0, $vr1, $vr0               \n"
      "vpickev.h        $vr2, $vr3, $vr2               \n"
      "vpickev.h        $vr1, $vr2, $vr0               \n"
      "vpickod.h        $vr0, $vr2, $vr0               \n"
      "vld              $vr2, %[src1], 0               \n"
      "vld              $vr3, %[src1], 16              \n"
      "vld              $vr4, %[src1], 32              \n"
      "vld              $vr5, %[src1], 48              \n"
      "vpickev.h        $vr2, $vr3, $vr2               \n"
      "vpickev.h        $vr4, $vr5, $vr4               \n"
      "vpickev.h        $vr3, $vr4, $vr2               \n"
      "vpickod.h        $vr2, $vr4, $vr2               \n"
      "vaddwev.h.bu     $vr4, $vr1, $vr0               \n"
      "vaddwod.h.bu     $vr5, $vr1, $vr0               \n"
      "vaddwev.h.bu     $vr0, $vr3, $vr2               \n"
      "vaddwod.h.bu     $vr1, $vr3, $vr2               \n"
      "vadd.h           $vr0, $vr4, $vr0               \n"
      "vadd.h           $vr1, $vr5, $vr1               \n"
      "vsrari.h         $vr0, $vr0, 2                  \n"
      "vsrari.h         $vr1, $vr1, 2                  \n"
      "vpackev.b        $vr0, $vr1, $vr0               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 64           \n"
      "addi.d           %[src1], %[src1], 64           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_ayuv),   // %[src0]
        [src1] "+r"(src_ayuv1),  // %[src1]
        [dst] "+r"(dst_vu),      // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_AYUVTOVUROW_LSX

#ifdef HAS_DETILEROW_LSX
void DetileRow_LSX(const uint8_t* src,
                   ptrdiff_t src_tile_stride,
                   uint8_t* dst,
                   int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vst              $vr0, %[dst], 0                \n"
      "add.d            %[src], %[src], %[str]         \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),            // %[src]
        [dst] "+r"(dst),            // %[dst]
        [cnt] "+r"(width)           // %[cnt]
      : [str] "r"(src_tile_stride)  // %[str]
      : "memory", "$vr0");
}
#endif  // HAS_DETILEROW_LSX

#ifdef HAS_DETILEROW_16_LSX
void DetileRow_16_LSX(const uint16_t* src,
                      ptrdiff_t src_tile_stride,
                      uint16_t* dst,
                      int width) {
  ptrdiff_t byte_stride = src_tile_stride * 2;
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "add.d            %[src], %[src], %[str]         \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),        // %[src]
        [dst] "+r"(dst),        // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [str] "r"(byte_stride)  // %[str]
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_DETILEROW_16_LSX

#ifdef HAS_MERGEARGBROW_LSX
void MergeARGBRow_LSX(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      const uint8_t* src_a,
                      uint8_t* dst_argb,
                      int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "vld              $vr3, %[sa], 0                 \n"
      "vilvl.b          $vr4, $vr1, $vr0               \n"
      "vilvh.b          $vr5, $vr1, $vr0               \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvh.b          $vr7, $vr3, $vr2               \n"
      "vilvl.h          $vr0, $vr6, $vr4               \n"
      "vilvh.h          $vr1, $vr6, $vr4               \n"
      "vilvl.h          $vr2, $vr7, $vr5               \n"
      "vilvh.h          $vr3, $vr7, $vr5               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr2, %[dst], 32               \n"
      "vst              $vr3, %[dst], 48               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[sa], %[sa], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [sa] "+r"(src_a),      // %[sa]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_MERGEARGBROW_LSX

#ifdef HAS_MERGEXRGBROW_LSX
void MergeXRGBRow_LSX(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      uint8_t* dst_argb,
                      int width) {
  asm volatile(
      "vldi             $vr3, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "vilvl.b          $vr4, $vr1, $vr0               \n"
      "vilvh.b          $vr5, $vr1, $vr0               \n"
      "vilvl.b          $vr6, $vr3, $vr2               \n"
      "vilvh.b          $vr7, $vr3, $vr2               \n"
      "vilvl.h          $vr0, $vr6, $vr4               \n"
      "vilvh.h          $vr1, $vr6, $vr4               \n"
      "vilvl.h          $vr2, $vr7, $vr5               \n"
      "vilvh.h          $vr4, $vr7, $vr5               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr2, %[dst], 32               \n"
      "vst              $vr4, %[dst], 48               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_MERGEXRGBROW_LSX

#ifdef HAS_SPLITARGBROW_LSX
void SplitARGBRow_LSX(const uint8_t* src_argb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      uint8_t* dst_a,
                      int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr2, %[src], 32               \n"
      "vld              $vr3, %[src], 48               \n"
      "vpickev.b        $vr4, $vr1, $vr0               \n"
      "vpickod.b        $vr5, $vr1, $vr0               \n"
      "vpickev.b        $vr6, $vr3, $vr2               \n"
      "vpickod.b        $vr7, $vr3, $vr2               \n"
      "vpickev.b        $vr0, $vr6, $vr4               \n"
      "vpickod.b        $vr1, $vr6, $vr4               \n"
      "vpickev.b        $vr2, $vr7, $vr5               \n"
      "vpickod.b        $vr3, $vr7, $vr5               \n"
      "vst              $vr0, %[db], 0                 \n"
      "vst              $vr2, %[dg], 0                 \n"
      "vst              $vr1, %[dr], 0                 \n"
      "vst              $vr3, %[da], 0                 \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[db], %[db], 16               \n"
      "addi.d           %[dg], %[dg], 16               \n"
      "addi.d           %[dr], %[dr], 16               \n"
      "addi.d           %[da], %[da], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [db] "+r"(dst_b),      // %[db]
        [dg] "+r"(dst_g),      // %[dg]
        [dr] "+r"(dst_r),      // %[dr]
        [da] "+r"(dst_a),      // %[da]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_SPLITARGBROW_LSX

#ifdef HAS_SPLITXRGBROW_LSX
void SplitXRGBRow_LSX(const uint8_t* src_argb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr2, %[src], 32               \n"
      "vld              $vr3, %[src], 48               \n"
      "vpickev.b        $vr4, $vr1, $vr0               \n"
      "vpickod.b        $vr5, $vr1, $vr0               \n"
      "vpickev.b        $vr6, $vr3, $vr2               \n"
      "vpickod.b        $vr7, $vr3, $vr2               \n"
      "vpickev.b        $vr0, $vr6, $vr4               \n"
      "vpickod.b        $vr1, $vr6, $vr4               \n"
      "vpickev.b        $vr2, $vr7, $vr5               \n"
      "vst              $vr0, %[db], 0                 \n"
      "vst              $vr2, %[dg], 0                 \n"
      "vst              $vr1, %[dr], 0                 \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[db], %[db], 16               \n"
      "addi.d           %[dg], %[dg], 16               \n"
      "addi.d           %[dr], %[dr], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [db] "+r"(dst_b),      // %[db]
        [dg] "+r"(dst_g),      // %[dg]
        [dr] "+r"(dst_r),      // %[dr]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_SPLITXRGBROW_LSX

#if defined(HAS_SPLITRGBROW_LSX) || defined(HAS_SPLITRGBROW_LASX)
// Shuffle tables for RGB24 to planar. Each channel takes 2 shuffles: A
// gathers from the first 32 bytes, B appends from the last 16 bytes.
// Tables are repeated in both 128 bit lanes for LASX.
static const ulvec8 kSplitRGBShufR0 = {
    0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 0, 0, 0, 0, 0,
    0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 0, 0, 0, 0, 0};
static const ulvec8 kSplitRGBShufR1 = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 17, 20, 23, 26, 29,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 17, 20, 23, 26, 29};
static const ulvec8 kSplitRGBShufG0 = {
    1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 0, 0, 0, 0, 0,
    1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 0, 0, 0, 0, 0};
static const ulvec8 kSplitRGBShufG1 = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 18, 21, 24, 27, 30,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 18, 21, 24, 27, 30};
static const ulvec8 kSplitRGBShufB0 = {
    2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 0, 0, 0, 0, 0, 0,
    2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 0, 0, 0, 0, 0, 0};
static const ulvec8 kSplitRGBShufB1 = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 19, 22, 25, 28, 31,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 19, 22, 25, 28, 31};
#endif  // HAS_SPLITRGBROW_LSX || HAS_SPLITRGBROW_LASX

#ifdef HAS_SPLITRGBROW_LSX
void SplitRGBRow_LSX(const uint8_t* src_rgb,
                     uint8_t* dst_r,
                     uint8_t* dst_g,
                     uint8_t* dst_b,
                     int width) {
  asm volatile(
      "vld              $vr6, %[shr0], 0               \n"
      "vld              $vr7, %[shr1], 0               \n"
      "vld              $vr8, %[shg0], 0               \n"
      "vld              $vr9, %[shg1], 0               \n"
      "vld              $vr10, %[shb0], 0              \n"
      "vld              $vr11, %[shb1], 0              \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr2, %[src], 32               \n"
      "vshuf.b          $vr3, $vr1, $vr0, $vr6         \n"
      "vshuf.b          $vr4, $vr1, $vr0, $vr8         \n"
      "vshuf.b          $vr5, $vr1, $vr0, $vr10        \n"
      "vshuf.b          $vr3, $vr2, $vr3, $vr7         \n"
      "vshuf.b          $vr4, $vr2, $vr4, $vr9         \n"
      "vshuf.b          $vr5, $vr2, $vr5, $vr11        \n"
      "vst              $vr3, %[dr], 0                 \n"
      "vst              $vr4, %[dg], 0                 \n"
      "vst              $vr5, %[db], 0                 \n"
      "addi.d           %[src], %[src], 48             \n"
      "addi.d           %[dr], %[dr], 16               \n"
      "addi.d           %[dg], %[dg], 16               \n"
      "addi.d           %[db], %[db], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb),           // %[src]
        [dr] "+r"(dst_r),              // %[dr]
        [dg] "+r"(dst_g),              // %[dg]
        [db] "+r"(dst_b),              // %[db]
        [cnt] "+r"(width)              // %[cnt]
      : [shr0] "r"(&kSplitRGBShufR0),  // %[shr0]
        [shr1] "r"(&kSplitRGBShufR1),  // %[shr1]
        [shg0] "r"(&kSplitRGBShufG0),  // %[shg0]
        [shg1] "r"(&kSplitRGBShufG1),  // %[shg1]
        [shb0] "r"(&kSplitRGBShufB0),  // %[shb0]
        [shb1] "r"(&kSplitRGBShufB1)   // %[shb1]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_SPLITRGBROW_LSX

#if defined(HAS_MERGERGBROW_LSX) || defined(HAS_MERGERGBROW_LASX)
// Shuffle tables for planar to RGB24. Each 16 bytes of output takes 2
// shuffles: A interleaves R and G, B inserts B.
// Tables are repeated in both 128 bit lanes for LASX.
static const ulvec8 kMergeRGBShuf0A = {0, 16, 0,  1, 17, 0, 2,  18, 0, 3,  19,
                                       0, 4,  20, 0, 5,  0, 16, 0,  1, 17, 0,
                                       2, 18, 0,  3, 19, 0, 4,  20, 0, 5};
static const ulvec8 kMergeRGBShuf0B = {
    0, 1, 16, 3, 4, 17, 6, 7, 18, 9, 10, 19, 12, 13, 20, 15,
    0, 1, 16, 3, 4, 17, 6, 7, 18, 9, 10, 19, 12, 13, 20, 15};
static const ulvec8 kMergeRGBShuf1A = {21, 0,  6, 22, 0,  7,  23, 0, 8,  24, 0,
                                       9,  25, 0, 10, 26, 21, 0,  6, 22, 0,  7,
                                       23, 0,  8, 24, 0,  9,  25, 0, 10, 26};
static const ulvec8 kMergeRGBShuf1B = {
    0, 21, 2, 3, 22, 5, 6, 23, 8, 9, 24, 11, 12, 25, 14, 15,
    0, 21, 2, 3, 22, 5, 6, 23, 8, 9, 24, 11, 12, 25, 14, 15};
static const ulvec8 kMergeRGBShuf2A = {
    0, 11, 27, 0, 12, 28, 0, 13, 29, 0, 14, 30, 0, 15, 31, 0,
    0, 11, 27, 0, 12, 28, 0, 13, 29, 0, 14, 30, 0, 15, 31, 0};
static const ulvec8 kMergeRGBShuf2B = {
    26, 1, 2, 27, 4, 5, 28, 7, 8, 29, 10, 11, 30, 13, 14, 31,
    26, 1, 2, 27, 4, 5, 28, 7, 8, 29, 10, 11, 30, 13, 14, 31};
#endif  // HAS_MERGERGBROW_LSX || HAS_MERGERGBROW_LASX

#ifdef HAS_MERGERGBROW_LSX
void MergeRGBRow_LSX(const uint8_t* src_r,
                     const uint8_t* src_g,
                     const uint8_t* src_b,
                     uint8_t* dst_rgb,
                     int width) {
  asm volatile(
      "vld              $vr6, %[sh0a], 0               \n"
      "vld              $vr7, %[sh0b], 0               \n"
      "vld              $vr8, %[sh1a], 0               \n"
      "vld              $vr9, %[sh1b], 0               \n"
      "vld              $vr10, %[sh2a], 0              \n"
      "vld              $vr11, %[sh2b], 0              \n"

      "1:                                              \n"
      "vld              $vr0, %[sr], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sb], 0                 \n"
      "vshuf.b          $vr3, $vr1, $vr0, $vr6         \n"
      "vshuf.b          $vr4, $vr1, $vr0, $vr8         \n"
      "vshuf.b          $vr5, $vr1, $vr0, $vr10        \n"
      "vshuf.b          $vr3, $vr2, $vr3, $vr7         \n"
      "vshuf.b          $vr4, $vr2, $vr4, $vr9         \n"
      "vshuf.b          $vr5, $vr2, $vr5, $vr11        \n"
      "vst              $vr3, %[dst], 0                \n"
      "vst              $vr4, %[dst], 16               \n"
      "vst              $vr5, %[dst], 32               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[dst], %[dst], 48             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sr] "+r"(src_r),              // %[sr]
        [sg] "+r"(src_g),              // %[sg]
        [sb] "+r"(src_b),              // %[sb]
        [dst] "+r"(dst_rgb),           // %[dst]
        [cnt] "+r"(width)              // %[cnt]
      : [sh0a] "r"(&kMergeRGBShuf0A),  // %[sh0a]
        [sh0b] "r"(&kMergeRGBShuf0B),  // %[sh0b]
        [sh1a] "r"(&kMergeRGBShuf1A),  // %[sh1a]
        [sh1b] "r"(&kMergeRGBShuf1B),  // %[sh1b]
        [sh2a] "r"(&kMergeRGBShuf2A),  // %[sh2a]
        [sh2b] "r"(&kMergeRGBShuf2B)   // %[sh2b]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_MERGERGBROW_LSX

#ifdef HAS_MERGEAR64ROW_LSX
void MergeAR64Row_LSX(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      const uint16_t* src_a,
                      uint16_t* dst_ar64,
                      int depth,
                      int width) {
  int shift = 16 - depth;
  int max = (1 << depth) - 1;
  asm volatile(
      "vreplgr2vr.h     $vr8, %[max]                   \n"
      "vreplgr2vr.h     $vr9, %[shf]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "vld              $vr3, %[sa], 0                 \n"
      "vmin.hu          $vr0, $vr0, $vr8               \n"
      "vmin.hu          $vr1, $vr1, $vr8               \n"
      "vmin.hu          $vr2, $vr2, $vr8               \n"
      "vmin.hu          $vr3, $vr3, $vr8               \n"
      "vsll.h           $vr0, $vr0, $vr9               \n"
      "vsll.h           $vr1, $vr1, $vr9               \n"
      "vsll.h           $vr2, $vr2, $vr9               \n"
      "vsll.h           $vr3, $vr3, $vr9               \n"
      "vilvl.h          $vr4, $vr1, $vr0               \n"
      "vilvh.h          $vr5, $vr1, $vr0               \n"
      "vilvl.h          $vr6, $vr3, $vr2               \n"
      "vilvh.h          $vr7, $vr3, $vr2               \n"
      "vilvl.w          $vr0, $vr6, $vr4               \n"
      "vilvh.w          $vr1, $vr6, $vr4               \n"
      "vilvl.w          $vr2, $vr7, $vr5               \n"
      "vilvh.w          $vr3, $vr7, $vr5               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr2, %[dst], 32               \n"
      "vst              $vr3, %[dst], 48               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[sa], %[sa], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [sa] "+r"(src_a),      // %[sa]
        [dst] "+r"(dst_ar64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [max] "r"(max),        // %[max]
        [shf] "r"(shift)       // %[shf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9");
}
#endif  // HAS_MERGEAR64ROW_LSX

#ifdef HAS_MERGEXR64ROW_LSX
void MergeXR64Row_LSX(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      uint16_t* dst_ar64,
                      int depth,
                      int width) {
  int shift = 16 - depth;
  int max = (1 << depth) - 1;
  asm volatile(
      "vreplgr2vr.h     $vr8, %[max]                   \n"
      "vreplgr2vr.h     $vr9, %[shf]                   \n"
      "vldi             $vr3, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "vmin.hu          $vr0, $vr0, $vr8               \n"
      "vmin.hu          $vr1, $vr1, $vr8               \n"
      "vmin.hu          $vr2, $vr2, $vr8               \n"
      "vsll.h           $vr0, $vr0, $vr9               \n"
      "vsll.h           $vr1, $vr1, $vr9               \n"
      "vsll.h           $vr2, $vr2, $vr9               \n"
      "vilvl.h          $vr4, $vr1, $vr0               \n"
      "vilvh.h          $vr5, $vr1, $vr0               \n"
      "vilvl.h          $vr6, $vr3, $vr2               \n"
      "vilvh.h          $vr7, $vr3, $vr2               \n"
      "vilvl.w          $vr0, $vr6, $vr4               \n"
      "vilvh.w          $vr1, $vr6, $vr4               \n"
      "vilvl.w          $vr2, $vr7, $vr5               \n"
      "vilvh.w          $vr7, $vr7, $vr5               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr2, %[dst], 32               \n"
      "vst              $vr7, %[dst], 48               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [dst] "+r"(dst_ar64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [max] "r"(max),        // %[max]
        [shf] "r"(shift)       // %[shf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9");
}
#endif  // HAS_MERGEXR64ROW_LSX

#ifdef HAS_MERGEXR30ROW_LSX
void MergeXR30Row_LSX(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      uint8_t* dst_ar30,
                      int depth,
                      int width) {
  int shift = depth - 10;
  asm volatile(
      "vreplgr2vr.h     $vr4, %[shf]                   \n"
      "vldi             $vr6, 0xff                     \n"
      "vsrli.h          $vr5, $vr6, 6                  \n"  // 1023
      "vslli.h          $vr6, $vr6, 14                 \n"  // 0xc000

      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "vsrl.h           $vr0, $vr0, $vr4               \n"
      "vsrl.h           $vr1, $vr1, $vr4               \n"
      "vsrl.h           $vr2, $vr2, $vr4               \n"
      "vmin.hu          $vr0, $vr0, $vr5               \n"
      "vmin.hu          $vr1, $vr1, $vr5               \n"
      "vmin.hu          $vr2, $vr2, $vr5               \n"
      "vslli.h          $vr3, $vr1, 10                 \n"
      "vsrli.h          $vr1, $vr1, 6                  \n"
      "vslli.h          $vr2, $vr2, 4                  \n"
      "vor.v            $vr0, $vr0, $vr3               \n"  // B | G << 10
      "vor.v            $vr1, $vr1, $vr2               \n"  // G >> 6 | R << 4
      "vor.v            $vr1, $vr1, $vr6               \n"  // A
      "vilvl.h          $vr2, $vr1, $vr0               \n"
      "vilvh.h          $vr3, $vr1, $vr0               \n"
      "vst              $vr2, %[dst], 0                \n"
      "vst              $vr3, %[dst], 16               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [dst] "+r"(dst_ar30),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shf] "r"(shift)       // %[shf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_MERGEXR30ROW_LSX

#ifdef HAS_MERGEARGB16TO8ROW_LSX
void MergeARGB16To8Row_LSX(const uint16_t* src_r,
                           const uint16_t* src_g,
                           const uint16_t* src_b,
                           const uint16_t* src_a,
                           uint8_t* dst_argb,
                           int depth,
                           int width) {
  int shift = depth - 8;
  asm volatile(
      "vreplgr2vr.h     $vr6, %[shf]                   \n"

      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "vld              $vr3, %[sa], 0                 \n"
      "vssrln.bu.h      $vr0, $vr0, $vr6               \n"
      "vssrln.bu.h      $vr1, $vr1, $vr6               \n"
      "vssrln.bu.h      $vr2, $vr2, $vr6               \n"
      "vssrln.bu.h      $vr3, $vr3, $vr6               \n"
      "vilvl.b          $vr4, $vr1, $vr0               \n"
      "vilvl.b          $vr5, $vr3, $vr2               \n"
      "vilvl.h          $vr0, $vr5, $vr4               \n"
      "vilvh.h          $vr1, $vr5, $vr4               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[sa], %[sa], 16               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [sa] "+r"(src_a),      // %[sa]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shf] "r"(shift)       // %[shf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_MERGEARGB16TO8ROW_LSX

#ifdef HAS_MERGEXRGB16TO8ROW_LSX
void MergeXRGB16To8Row_LSX(const uint16_t* src_r,
                           const uint16_t* src_g,
                           const uint16_t* src_b,
                           uint8_t* dst_argb,
                           int depth,
                           int width) {
  int shift = depth - 8;
  asm volatile(
      "vreplgr2vr.h     $vr6, %[shf]                   \n"
      "vldi             $vr3, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "vssrln.bu.h      $vr0, $vr0, $vr6               \n"
      "vssrln.bu.h      $vr1, $vr1, $vr6               \n"
      "vssrln.bu.h      $vr2, $vr2, $vr6               \n"
      "vilvl.b          $vr4, $vr1, $vr0               \n"
      "vilvl.b          $vr5, $vr3, $vr2               \n"
      "vilvl.h          $vr0, $vr5, $vr4               \n"
      "vilvh.h          $vr1, $vr5, $vr4               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shf] "r"(shift)       // %[shf]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_MERGEXRGB16TO8ROW_LSX

#ifdef HAS_RGB24MIRRORROW_LSX
static const uvec8 kRGB24MirrorShuf0 = {13, 14, 15, 10, 11, 12, 7, 8,
                                        9,  4,  5,  6,  1,  2,  3, 30};
static const uvec8 kRGB24MirrorShuf1 = {15, 16, 11, 12, 13, 8, 9, 10,
                                        5,  6,  7,  2,  3,  4, 0, 0};
static const uvec8 kRGB24MirrorShuf2 = {17, 12, 13, 14, 9, 10, 11, 6,
                                        7,  8,  3,  4,  5, 0,  1,  2};
void RGB24MirrorRow_LSX(const uint8_t* src_rgb24,
                        uint8_t* dst_rgb24,
                        int width) {
  src_rgb24 += (ptrdiff_t)width * 3 - 48;
  asm volatile(
      "vld              $vr4, %[sh0], 0                \n"
      "vld              $vr5, %[sh1], 0                \n"
      "vld              $vr6, %[sh2], 0                \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr2, %[src], 32               \n"
      "vshuf.b          $vr3, $vr1, $vr2, $vr4         \n"
      "vshuf.b          $vr7, $vr2, $vr1, $vr5         \n"
      "vextrins.b       $vr7, $vr0, 0xef               \n"
      "vshuf.b          $vr0, $vr1, $vr0, $vr6         \n"
      "vst              $vr3, %[dst], 0                \n"
      "vst              $vr7, %[dst], 16               \n"
      "vst              $vr0, %[dst], 32               \n"
      "addi.d           %[src], %[src], -48            \n"
      "addi.d           %[dst], %[dst], 48             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb24),          // %[src]
        [dst] "+r"(dst_rgb24),          // %[dst]
        [cnt] "+r"(width)               // %[cnt]
      : [sh0] "r"(&kRGB24MirrorShuf0),  // %[sh0]
        [sh1] "r"(&kRGB24MirrorShuf1),  // %[sh1]
        [sh2] "r"(&kRGB24MirrorShuf2)   // %[sh2]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_RGB24MIRRORROW_LSX

#ifdef HAS_BLENDPLANEROW_LSX
void BlendPlaneRow_LSX(const uint8_t* src0,
                       const uint8_t* src1,
                       const uint8_t* alpha,
                       uint8_t* dst,
                       int width) {
  asm volatile(
      "vldi             $vr6, 0x4ff                    \n"

      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr1, %[s1], 0                 \n"
      "vld              $vr2, %[alp], 0                \n"
      "vxori.b          $vr3, $vr2, 0xff               \n"
      "vmulwev.h.bu     $vr4, $vr0, $vr2               \n"
      "vmaddwev.h.bu    $vr4, $vr1, $vr3               \n"
      "vmulwod.h.bu     $vr5, $vr0, $vr2               \n"
      "vmaddwod.h.bu    $vr5, $vr1, $vr3               \n"
      "vadd.h           $vr4, $vr4, $vr6               \n"
      "vadd.h           $vr5, $vr5, $vr6               \n"
      "vpackod.b        $vr0, $vr5, $vr4               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 16               \n"
      "addi.d           %[s1], %[s1], 16               \n"
      "addi.d           %[alp], %[alp], 16             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src0),    // %[s0]
        [s1] "+r"(src1),    // %[s1]
        [alp] "+r"(alpha),  // %[alp]
        [dst] "+r"(dst),    // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_BLENDPLANEROW_LSX

#ifdef HAS_INTERPOLATEROW_16_LSX
void InterpolateRow_16_LSX(uint16_t* dst_ptr,
                           const uint16_t* src_ptr,
                           ptrdiff_t src_stride,
                           int width,
                           int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  if (y1_fraction == 0) {
    asm volatile(
        "1:                                              \n"
        "vld              $vr0, %[src0], 0               \n"
        "vst              $vr0, %[dst], 0                \n"
        "addi.d           %[src0], %[src0], 16           \n"
        "addi.d           %[dst], %[dst], 16             \n"
        "addi.d           %[cnt], %[cnt], -8             \n"
        "bnez             %[cnt], 1b                     \n"
        : [src0] "+r"(src_ptr),  // %[src0]
          [dst] "+r"(dst_ptr),   // %[dst]
          [cnt] "+r"(width)      // %[cnt]
        :
        : "memory", "$vr0");
    return;
  }
  if (y1_fraction == 128) {
    HalfRow_16_LSX(src_ptr, src_stride, dst_ptr, width);
    return;
  }
  asm volatile(
      "vreplgr2vr.h     $vr4, %[f0]                    \n"
      "vreplgr2vr.h     $vr5, %[f1]                    \n"

      "1:                                              \n"
      "vld              $vr0, %[src0], 0               \n"
      "vld              $vr1, %[src1], 0               \n"
      "vmulwev.w.hu     $vr2, $vr0, $vr4               \n"
      "vmaddwev.w.hu    $vr2, $vr1, $vr5               \n"
      "vmulwod.w.hu     $vr3, $vr0, $vr4               \n"
      "vmaddwod.w.hu    $vr3, $vr1, $vr5               \n"
      "vsrari.w         $vr2, $vr2, 8                  \n"
      "vsrari.w         $vr3, $vr3, 8                  \n"
      "vpackev.h        $vr3, $vr3, $vr2               \n"
      "vst              $vr3, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 16           \n"
      "addi.d           %[src1], %[src1], 16           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_ptr),   // %[src0]
        [src1] "+r"(src_ptr1),  // %[src1]
        [dst] "+r"(dst_ptr),    // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [f0] "r"(y0_fraction),  // %[f0]
        [f1] "r"(y1_fraction)   // %[f1]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_INTERPOLATEROW_16_LSX

#ifdef HAS_COPYROW_LASX
void CopyRow_LASX(const uint8_t* src, uint8_t* dst, int count) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),   // %[src]
        [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(count)  // %[cnt]
      :
      : "memory", "$xr0");
}
#endif  // HAS_COPYROW_LASX

#ifdef HAS_SETROW_LASX
void SetRow_LASX(uint8_t* dst, uint8_t v8, int count) {
  asm volatile(
      "xvreplgr2vr.b    $xr0, %[val]                   \n"

      "1:                                              \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(count)  // %[cnt]
      : [val] "r"(v8)      // %[val]
      : "memory", "$xr0");
}
#endif  // HAS_SETROW_LASX

#ifdef HAS_ARGBSETROW_LASX
void ARGBSetRow_LASX(uint8_t* dst_argb, uint32_t v32, int width) {
  asm volatile(
      "xvreplgr2vr.w    $xr0, %[val]                   \n"

      "1:                                              \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [val] "r"(v32)         // %[val]
      : "memory", "$xr0");
}
#endif  // HAS_ARGBSETROW_LASX

#ifdef HAS_MIRRORROW_LASX
void MirrorRow_LASX(const uint8_t* src_y, uint8_t* dst_y, int width) {
  static const uint8_t kRev[16] = {15, 14, 13, 12, 11, 10, 9, 8,
                                   7,  6,  5,  4,  3,  2,  1, 0};
  src_y += (ptrdiff_t)width - 32;
  asm volatile(
      "vld              $vr1, %[rev], 0                \n"
      "xvpermi.q        $xr1, $xr1, 0x00               \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvpermi.q        $xr0, $xr0, 0x01               \n"
      "xvshuf.b         $xr0, $xr0, $xr0, $xr1         \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], -32            \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [rev] "r"(kRev)     // %[rev]
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_MIRRORROW_LASX

#ifdef HAS_MIRRORUVROW_LASX
void MirrorUVRow_LASX(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  static const uint8_t kRevUV[16] = {14, 15, 12, 13, 10, 11, 8, 9,
                                     6,  7,  4,  5,  2,  3,  0, 1};
  src_uv += (ptrdiff_t)width * 2 - 32;
  asm volatile(
      "vld              $vr1, %[rev], 0                \n"
      "xvpermi.q        $xr1, $xr1, 0x00               \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvpermi.q        $xr0, $xr0, 0x01               \n"
      "xvshuf.b         $xr0, $xr0, $xr0, $xr1         \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], -32            \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [dst] "+r"(dst_uv),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      : [rev] "r"(kRevUV)    // %[rev]
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_MIRRORUVROW_LASX

#ifdef HAS_ARGBMIRRORROW_LASX
void ARGBMirrorRow_LASX(const uint8_t* src_argb, uint8_t* dst_argb, int width) {
  src_argb += (ptrdiff_t)width * 4 - 32;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvpermi.q        $xr0, $xr0, 0x01               \n"
      "xvshuf4i.w       $xr0, $xr0, 0x1b               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], -32            \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0");
}
#endif  // HAS_ARGBMIRRORROW_LASX

#ifdef HAS_MIRRORSPLITUVROW_LASX
void MirrorSplitUVRow_LASX(const uint8_t* src_uv,
                           uint8_t* dst_u,
                           uint8_t* dst_v,
                           int width) {
  static const uint8_t kRevUV[16] = {14, 15, 12, 13, 10, 11, 8, 9,
                                     6,  7,  4,  5,  2,  3,  0, 1};
  src_uv += (ptrdiff_t)width * 2 - 64;
  asm volatile(
      "vld              $vr4, %[rev], 0                \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 32               \n"
      "xvld             $xr1, %[src], 0                \n"
      "xvshuf.b         $xr0, $xr0, $xr0, $xr4         \n"
      "xvshuf.b         $xr1, $xr1, $xr1, $xr4         \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0x72               \n"
      "xvpermi.d        $xr3, $xr3, 0x72               \n"
      "xvst             $xr2, %[du], 0                 \n"
      "xvst             $xr3, %[dv], 0                 \n"
      "addi.d           %[src], %[src], -64            \n"
      "addi.d           %[du], %[du], 32               \n"
      "addi.d           %[dv], %[dv], 32               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [du] "+r"(dst_u),    // %[du]
        [dv] "+r"(dst_v),    // %[dv]
        [cnt] "+r"(width)    // %[cnt]
      : [rev] "r"(kRevUV)    // %[rev]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_MIRRORSPLITUVROW_LASX

#ifdef HAS_SPLITUVROW_LASX
void SplitUVRow_LASX(const uint8_t* src_uv,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpermi.d        $xr3, $xr3, 0xd8               \n"
      "xvst             $xr2, %[du], 0                 \n"
      "xvst             $xr3, %[dv], 0                 \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[du], %[du], 32               \n"
      "addi.d           %[dv], %[dv], 32               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [du] "+r"(dst_u),    // %[du]
        [dv] "+r"(dst_v),    // %[dv]
        [cnt] "+r"(width)    // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_SPLITUVROW_LASX

#ifdef HAS_MERGEUVROW_LASX
void MergeUVRow_LASX(const uint8_t* src_u,
                     const uint8_t* src_v,
                     uint8_t* dst_uv,
                     int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[su], 0                 \n"
      "xvld             $xr1, %[sv], 0                 \n"
      "xvilvl.b         $xr2, $xr1, $xr0               \n"
      "xvilvh.b         $xr3, $xr1, $xr0               \n"
      "vst              $vr2, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr2, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr3, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr3, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[su], %[su], 32               \n"
      "addi.d           %[sv], %[sv], 32               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [su] "+r"(src_u),    // %[su]
        [sv] "+r"(src_v),    // %[sv]
        [dst] "+r"(dst_uv),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr31");
}
#endif  // HAS_MERGEUVROW_LASX

#ifdef HAS_ARGBEXTRACTALPHAROW_LASX
void ARGBExtractAlphaRow_LASX(const uint8_t* src_argb,
                              uint8_t* dst_a,
                              int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr31, %[src], 32              \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr1, $xr31, 0x02              \n"
      "xvpickod.b       $xr0, $xr1, $xr0               \n"
      "xvpickod.b       $xr0, $xr0, $xr0               \n"
      "xvpermi.d        $xr31, $xr0, 0xd8              \n"
      "vst              $vr31, %[dst], 0               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_a),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr31");
}
#endif  // HAS_ARGBEXTRACTALPHAROW_LASX

#ifdef HAS_I422TOYUY2ROW_LASX
void I422ToYUY2Row_LASX(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_yuy2,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[sy], 0                 \n"
      "vld              $vr1, %[su], 0                 \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "vld              $vr2, %[sv], 0                 \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvilvl.b         $xr3, $xr2, $xr1               \n"
      "xvilvl.b         $xr4, $xr3, $xr0               \n"
      "xvilvh.b         $xr5, $xr3, $xr0               \n"
      "xvor.v           $xr31, $xr4, $xr4              \n"
      "xvpermi.q        $xr31, $xr5, 0x02              \n"
      "xvpermi.q        $xr4, $xr5, 0x13               \n"
      "xvst             $xr31, %[dst], 0               \n"
      "xvst             $xr4, %[dst], 32               \n"
      "addi.d           %[sy], %[sy], 32               \n"
      "addi.d           %[su], %[su], 16               \n"
      "addi.d           %[sv], %[sv], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_yuy2),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr31");
}
#endif  // HAS_I422TOYUY2ROW_LASX

#ifdef HAS_I422TOUYVYROW_LASX
void I422ToUYVYRow_LASX(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_uyvy,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[sy], 0                 \n"
      "vld              $vr1, %[su], 0                 \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "vld              $vr2, %[sv], 0                 \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvilvl.b         $xr3, $xr2, $xr1               \n"
      "xvilvl.b         $xr4, $xr0, $xr3               \n"
      "xvilvh.b         $xr5, $xr0, $xr3               \n"
      "xvor.v           $xr31, $xr4, $xr4              \n"
      "xvpermi.q        $xr31, $xr5, 0x02              \n"
      "xvpermi.q        $xr4, $xr5, 0x13               \n"
      "xvst             $xr31, %[dst], 0               \n"
      "xvst             $xr4, %[dst], 32               \n"
      "addi.d           %[sy], %[sy], 32               \n"
      "addi.d           %[su], %[su], 16               \n"
      "addi.d           %[sv], %[sv], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_uyvy),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr31");
}
#endif  // HAS_I422TOUYVYROW_LASX

#ifdef HAS_YUY2TOYROW_LASX
void YUY2ToYRow_LASX(const uint8_t* src_yuy2, uint8_t* dst_y, int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2");
}
#endif  // HAS_YUY2TOYROW_LASX

#ifdef HAS_UYVYTOYROW_LASX
void UYVYToYRow_LASX(const uint8_t* src_uyvy, uint8_t* dst_y, int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickod.b       $xr2, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2");
}
#endif  // HAS_UYVYTOYROW_LASX

#ifdef HAS_YUY2TOUV422ROW_LASX
void YUY2ToUV422Row_LASX(const uint8_t* src_yuy2,
                         uint8_t* dst_u,
                         uint8_t* dst_v,
                         int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickod.b       $xr2, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpickev.b       $xr3, $xr2, $xr2               \n"
      "xvpickod.b       $xr4, $xr2, $xr2               \n"
      "xvpermi.d        $xr3, $xr3, 0xd8               \n"
      "vst              $vr3, %[du], 0                 \n"
      "xvpermi.d        $xr4, $xr4, 0xd8               \n"
      "vst              $vr4, %[dv], 0                 \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_YUY2TOUV422ROW_LASX

#ifdef HAS_UYVYTOUV422ROW_LASX
void UYVYToUV422Row_LASX(const uint8_t* src_uyvy,
                         uint8_t* dst_u,
                         uint8_t* dst_v,
                         int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpickev.b       $xr3, $xr2, $xr2               \n"
      "xvpickod.b       $xr4, $xr2, $xr2               \n"
      "xvpermi.d        $xr3, $xr3, 0xd8               \n"
      "vst              $vr3, %[du], 0                 \n"
      "xvpermi.d        $xr4, $xr4, 0xd8               \n"
      "vst              $vr4, %[dv], 0                 \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_UYVYTOUV422ROW_LASX

#ifdef HAS_YUY2TOUVROW_LASX
void YUY2ToUVRow_LASX(const uint8_t* src_yuy2,
                      int src_stride_yuy2,
                      uint8_t* dst_u,
                      uint8_t* dst_v,
                      int width) {
  const uint8_t* next_yuy2 = src_yuy2 + src_stride_yuy2;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s0], 32                \n"
      "xvld             $xr2, %[s1], 0                 \n"
      "xvld             $xr3, %[s1], 32                \n"
      "xvpickod.b       $xr0, $xr1, $xr0               \n"
      "xvpickod.b       $xr2, $xr3, $xr2               \n"
      "xvavgr.bu        $xr0, $xr0, $xr2               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpickev.b       $xr1, $xr0, $xr0               \n"
      "xvpickod.b       $xr2, $xr0, $xr0               \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "vst              $vr1, %[du], 0                 \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "vst              $vr2, %[dv], 0                 \n"
      "addi.d           %[s0], %[s0], 64               \n"
      "addi.d           %[s1], %[s1], 64               \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_yuy2),   // %[s0]
        [s1] "+r"(next_yuy2),  // %[s1]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_YUY2TOUVROW_LASX

#ifdef HAS_UYVYTOUVROW_LASX
void UYVYToUVRow_LASX(const uint8_t* src_uyvy,
                      int src_stride_uyvy,
                      uint8_t* dst_u,
                      uint8_t* dst_v,
                      int width) {
  const uint8_t* next_uyvy = src_uyvy + src_stride_uyvy;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s0], 32                \n"
      "xvld             $xr2, %[s1], 0                 \n"
      "xvld             $xr3, %[s1], 32                \n"
      "xvpickev.b       $xr0, $xr1, $xr0               \n"
      "xvpickev.b       $xr2, $xr3, $xr2               \n"
      "xvavgr.bu        $xr0, $xr0, $xr2               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpickev.b       $xr1, $xr0, $xr0               \n"
      "xvpickod.b       $xr2, $xr0, $xr0               \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "vst              $vr1, %[du], 0                 \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "vst              $vr2, %[dv], 0                 \n"
      "addi.d           %[s0], %[s0], 64               \n"
      "addi.d           %[s1], %[s1], 64               \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_uyvy),   // %[s0]
        [s1] "+r"(next_uyvy),  // %[s1]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_UYVYTOUVROW_LASX

#ifdef HAS_I422TOARGBROW_LASX
void I422ToARGBRow_LASX(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vldrepl.d        $vr10, %[su], 0                \n"
      "vldrepl.d        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr3, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr4               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr20", "$xr21", "$xr22",
        "$xr23", "$xr24", "$xr31");
}
#endif  // HAS_I422TOARGBROW_LASX

#ifdef HAS_I422TORGBAROW_LASX
void I422ToRGBARow_LASX(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vldrepl.d        $vr10, %[su], 0                \n"
      "vldrepl.d        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr2, $xr5               \n"
      "xvilvl.b         $xr7, $xr4, $xr3               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr20", "$xr21", "$xr22",
        "$xr23", "$xr24", "$xr31");
}
#endif  // HAS_I422TORGBAROW_LASX

#ifdef HAS_I422ALPHATOARGBROW_LASX
void I422AlphaToARGBRow_LASX(const uint8_t* src_y,
                             const uint8_t* src_u,
                             const uint8_t* src_v,
                             const uint8_t* src_a,
                             uint8_t* dst_argb,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vldrepl.d        $vr10, %[su], 0                \n"
      "vldrepl.d        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "vld              $vr5, %[sa], 0                 \n"
      "xvpermi.d        $xr5, $xr5, 0xd8               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr3, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr4               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[sa], %[sa], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width),     // %[cnt]
        [sa] "+r"(src_a)       // %[sa]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr20", "$xr21", "$xr22",
        "$xr23", "$xr24", "$xr31");
}
#endif  // HAS_I422ALPHATOARGBROW_LASX

#ifdef HAS_I422TORGB24ROW_LASX
void I422ToRGB24Row_LASX(const uint8_t* src_y,
                         const uint8_t* src_u,
                         const uint8_t* src_v,
                         uint8_t* dst_argb,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  static const uint8_t kShuf0[16] = {0,  1,  2,  4,  5,  6,  8,  9,
                                     10, 12, 13, 14, 16, 17, 18, 20};
  static const uint8_t kShuf1[16] = {21, 22, 24, 25, 26, 28, 29, 30,
                                     0,  0,  0,  0,  0,  0,  0,  0};
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "vld              $vr12, %[s0], 0                \n"
      "xvpermi.q        $xr12, $xr12, 0x00             \n"
      "vld              $vr13, %[s1], 0                \n"
      "xvpermi.q        $xr13, $xr13, 0x00             \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vldrepl.d        $vr10, %[su], 0                \n"
      "vldrepl.d        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr3, $xr2               \n"
      "xvilvl.b         $xr7, $xr4, $xr4               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "xvshuf.b         $xr10, $xr9, $xr8, $xr12       \n"
      "xvshuf.b         $xr11, $xr9, $xr8, $xr13       \n"
      "vst              $vr10, %[dst], 0               \n"
      "xvpermi.q        $xr31, $xr10, 0x01             \n"
      "vst              $vr31, %[dst], 24              \n"
      "vstelm.d         $vr11, %[dst], 16, 0           \n"
      "xvpermi.q        $xr31, $xr11, 0x01             \n"
      "vstelm.d         $vr31, %[dst], 40, 0           \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 48             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),       // %[sy]
        [su] "+r"(src_u),       // %[su]
        [sv] "+r"(src_v),       // %[sv]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [c] "r"(yuvconstants),  // %[c]
        [s0] "r"(kShuf0),       // %[s0]
        [s1] "r"(kShuf1)        // %[s1]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr6", "$xr7",
        "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr20", "$xr21",
        "$xr22", "$xr23", "$xr24", "$xr31");
}
#endif  // HAS_I422TORGB24ROW_LASX

#ifdef HAS_I422TORGB565ROW_LASX
void I422ToRGB565Row_LASX(const uint8_t* src_y,
                          const uint8_t* src_u,
                          const uint8_t* src_v,
                          uint8_t* dst_argb,
                          const struct YuvConstants* yuvconstants,
                          int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr14, 31                      \n"
      "xvsrli.h         $xr14, $xr14, 8                \n"
      "xvldi            $xr15, 63                      \n"
      "xvsrli.h         $xr15, $xr15, 8                \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vldrepl.d        $vr10, %[su], 0                \n"
      "vldrepl.d        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvsrli.b         $xr2, $xr2, 3                  \n"
      "xvsrli.b         $xr3, $xr3, 2                  \n"
      "xvsrli.b         $xr4, $xr4, 3                  \n"
      "xvilvl.b         $xr2, $xr2, $xr2               \n"
      "xvilvl.b         $xr3, $xr3, $xr3               \n"
      "xvilvl.b         $xr4, $xr4, $xr4               \n"
      "xvand.v          $xr2, $xr2, $xr14              \n"
      "xvand.v          $xr3, $xr3, $xr15              \n"
      "xvand.v          $xr4, $xr4, $xr14              \n"
      "xvslli.h         $xr3, $xr3, 5                  \n"
      "xvslli.h         $xr4, $xr4, 11                 \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvor.v           $xr2, $xr2, $xr4               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr10", "$xr11",
        "$xr14", "$xr15", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24");
}
#endif  // HAS_I422TORGB565ROW_LASX

#ifdef HAS_I422TOARGB4444ROW_LASX
void I422ToARGB4444Row_LASX(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr14, 15                      \n"
      "xvsrli.h         $xr14, $xr14, 8                \n"
      "xvldi            $xr15, 240                     \n"
      "xvsrli.h         $xr15, $xr15, 8                \n"
      "xvslli.h         $xr15, $xr15, 8                \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vldrepl.d        $vr10, %[su], 0                \n"
      "vldrepl.d        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvsrli.b         $xr2, $xr2, 4                  \n"
      "xvsrli.b         $xr3, $xr3, 4                  \n"
      "xvsrli.b         $xr4, $xr4, 4                  \n"
      "xvilvl.b         $xr2, $xr2, $xr2               \n"
      "xvilvl.b         $xr3, $xr3, $xr3               \n"
      "xvilvl.b         $xr4, $xr4, $xr4               \n"
      "xvand.v          $xr2, $xr2, $xr14              \n"
      "xvand.v          $xr3, $xr3, $xr14              \n"
      "xvand.v          $xr4, $xr4, $xr14              \n"
      "xvslli.h         $xr3, $xr3, 4                  \n"
      "xvslli.h         $xr4, $xr4, 8                  \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvor.v           $xr2, $xr2, $xr4               \n"
      "xvor.v           $xr2, $xr2, $xr15              \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr10", "$xr11",
        "$xr14", "$xr15", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24");
}
#endif  // HAS_I422TOARGB4444ROW_LASX

#ifdef HAS_I422TOARGB1555ROW_LASX
void I422ToARGB1555Row_LASX(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr14, 31                      \n"
      "xvsrli.h         $xr14, $xr14, 8                \n"
      "xvldi            $xr15, 128                     \n"
      "xvsrli.h         $xr15, $xr15, 8                \n"
      "xvslli.h         $xr15, $xr15, 8                \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vldrepl.d        $vr10, %[su], 0                \n"
      "vldrepl.d        $vr11, %[sv], 0                \n"
      "vilvl.b          $vr1, $vr11, $vr10             \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvsrli.b         $xr2, $xr2, 3                  \n"
      "xvsrli.b         $xr3, $xr3, 3                  \n"
      "xvsrli.b         $xr4, $xr4, 3                  \n"
      "xvilvl.b         $xr2, $xr2, $xr2               \n"
      "xvilvl.b         $xr3, $xr3, $xr3               \n"
      "xvilvl.b         $xr4, $xr4, $xr4               \n"
      "xvand.v          $xr2, $xr2, $xr14              \n"
      "xvand.v          $xr3, $xr3, $xr14              \n"
      "xvand.v          $xr4, $xr4, $xr14              \n"
      "xvslli.h         $xr3, $xr3, 5                  \n"
      "xvslli.h         $xr4, $xr4, 10                 \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvor.v           $xr2, $xr2, $xr4               \n"
      "xvor.v           $xr2, $xr2, $xr15              \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 8                \n"
      "addi.d           %[sv], %[sv], 8                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr10", "$xr11",
        "$xr14", "$xr15", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24");
}
#endif  // HAS_I422TOARGB1555ROW_LASX

#ifdef HAS_NV12TOARGBROW_LASX
void NV12ToARGBRow_LASX(const uint8_t* src_y,
                        const uint8_t* src_uv,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vld              $vr1, %[suv], 0                \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr3, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr4               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[suv], %[suv], 16             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [suv] "+r"(src_uv),    // %[suv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24",
        "$xr31");
}
#endif  // HAS_NV12TOARGBROW_LASX

#ifdef HAS_NV21TOARGBROW_LASX
void NV21ToARGBRow_LASX(const uint8_t* src_y,
                        const uint8_t* src_vu,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vld              $vr1, %[suv], 0                \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvshuf4i.b       $xr1, $xr1, 0xb1               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr3, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr4               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[suv], %[suv], 16             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [suv] "+r"(src_vu),    // %[suv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24",
        "$xr31");
}
#endif  // HAS_NV21TOARGBROW_LASX

#ifdef HAS_NV12TORGB565ROW_LASX
void NV12ToRGB565Row_LASX(const uint8_t* src_y,
                          const uint8_t* src_uv,
                          uint8_t* dst_argb,
                          const struct YuvConstants* yuvconstants,
                          int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr14, 31                      \n"
      "xvsrli.h         $xr14, $xr14, 8                \n"
      "xvldi            $xr15, 63                      \n"
      "xvsrli.h         $xr15, $xr15, 8                \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vld              $vr1, %[suv], 0                \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvsrli.b         $xr2, $xr2, 3                  \n"
      "xvsrli.b         $xr3, $xr3, 2                  \n"
      "xvsrli.b         $xr4, $xr4, 3                  \n"
      "xvilvl.b         $xr2, $xr2, $xr2               \n"
      "xvilvl.b         $xr3, $xr3, $xr3               \n"
      "xvilvl.b         $xr4, $xr4, $xr4               \n"
      "xvand.v          $xr2, $xr2, $xr14              \n"
      "xvand.v          $xr3, $xr3, $xr15              \n"
      "xvand.v          $xr4, $xr4, $xr14              \n"
      "xvslli.h         $xr3, $xr3, 5                  \n"
      "xvslli.h         $xr4, $xr4, 11                 \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvor.v           $xr2, $xr2, $xr4               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[suv], %[suv], 16             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [suv] "+r"(src_uv),    // %[suv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr14", "$xr15",
        "$xr20", "$xr21", "$xr22", "$xr23", "$xr24");
}
#endif  // HAS_NV12TORGB565ROW_LASX

#ifdef HAS_I444TOARGBROW_LASX
void I444ToARGBRow_LASX(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vld              $vr10, %[su], 0                \n"
      "xvpermi.d        $xr10, $xr10, 0xd8             \n"
      "vld              $vr11, %[sv], 0                \n"
      "xvpermi.d        $xr11, $xr11, 0xd8             \n"
      "xvilvl.b         $xr1, $xr11, $xr10             \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr3, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr4               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[su], %[su], 16               \n"
      "addi.d           %[sv], %[sv], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [su] "+r"(src_u),      // %[su]
        [sv] "+r"(src_v),      // %[sv]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr20", "$xr21", "$xr22",
        "$xr23", "$xr24", "$xr31");
}
#endif  // HAS_I444TOARGBROW_LASX

#ifdef HAS_I400TOARGBROW_LASX
void I400ToARGBRow_LASX(const uint8_t* src_y,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "vld              $vr23, %[c], 96                \n"
      "xvpermi.q        $xr23, $xr23, 0x00             \n"
      "vld              $vr24, %[c], 128               \n"
      "xvpermi.q        $xr24, $xr24, 0x00             \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sy], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvssrani.bu.h    $xr2, $xr0, 6                  \n"
      "xvilvl.b         $xr6, $xr2, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr2               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr2", "$xr5", "$xr6", "$xr7", "$xr8", "$xr9",
        "$xr23", "$xr24", "$xr31");
}
#endif  // HAS_I400TOARGBROW_LASX

#ifdef HAS_J400TOARGBROW_LASX
void J400ToARGBRow_LASX(const uint8_t* src_y, uint8_t* dst_argb, int width) {
  asm volatile(
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr2, %[sy], 0                 \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvilvl.b         $xr6, $xr2, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr2               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sy] "+r"(src_y),      // %[sy]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr2", "$xr5", "$xr6", "$xr7", "$xr8", "$xr9", "$xr31");
}
#endif  // HAS_J400TOARGBROW_LASX

#ifdef HAS_YUY2TOARGBROW_LASX
void YUY2ToARGBRow_LASX(const uint8_t* src_yuy2,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "xvld             $xr10, %[src], 0               \n"
      "xvpickev.b       $xr0, $xr10, $xr10             \n"
      "xvpickod.b       $xr1, $xr10, $xr10             \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr3, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr4               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr20", "$xr21", "$xr22", "$xr23",
        "$xr24", "$xr31");
}
#endif  // HAS_YUY2TOARGBROW_LASX

#ifdef HAS_UYVYTOARGBROW_LASX
void UYVYToARGBRow_LASX(const uint8_t* src_uyvy,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "xvldrepl.w       $xr20, %[c], 0                 \n"
      "xvldrepl.w       $xr21, %[c], 32                \n"
      "xvldrepl.w       $xr22, %[c], 64                \n"
      "xvldrepl.h       $xr23, %[c], 96                \n"
      "xvldrepl.h       $xr24, %[c], 128               \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "xvld             $xr10, %[src], 0               \n"
      "xvpickod.b       $xr0, $xr10, $xr10             \n"
      "xvpickev.b       $xr1, $xr10, $xr10             \n"
      "xvilvl.h         $xr1, $xr1, $xr1               \n"
      "xvilvl.b         $xr0, $xr0, $xr0               \n"
      "xvxori.b         $xr1, $xr1, 0x80               \n"
      "xvmuh.hu         $xr0, $xr0, $xr23              \n"
      "xvadd.h          $xr0, $xr0, $xr24              \n"
      "xvmulwev.h.bu.b  $xr2, $xr20, $xr1              \n"
      "xvmulwod.h.bu.b  $xr4, $xr22, $xr1              \n"
      "xvmulwev.h.bu.b  $xr3, $xr21, $xr1              \n"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1              \n"
      "xvsadd.h         $xr2, $xr0, $xr2               \n"
      "xvsadd.h         $xr4, $xr0, $xr4               \n"
      "xvssub.h         $xr3, $xr0, $xr3               \n"
      "xvssrani.bu.h    $xr2, $xr2, 6                  \n"
      "xvssrani.bu.h    $xr3, $xr3, 6                  \n"
      "xvssrani.bu.h    $xr4, $xr4, 6                  \n"
      "xvilvl.b         $xr6, $xr3, $xr2               \n"
      "xvilvl.b         $xr7, $xr5, $xr4               \n"
      "xvilvl.h         $xr8, $xr7, $xr6               \n"
      "xvilvh.h         $xr9, $xr7, $xr6               \n"
      "vst              $vr8, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr9, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(yuvconstants)  // %[c]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr20", "$xr21", "$xr22", "$xr23",
        "$xr24", "$xr31");
}
#endif  // HAS_UYVYTOARGBROW_LASX

#if defined(HAS_ARGBTORGB24ROW_LASX) || defined(HAS_ARGBTORAWROW_LASX)
static LIBYUV_NOINLINE void ARGBTo24Helper_LASX(const uint8_t* src_argb,
                                                uint8_t* dst_24,
                                                const uint8_t* shuf,
                                                int width) {
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr31, %[src], 64              \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr31, %[src], 80              \n"
      "xvpermi.q        $xr1, $xr31, 0x02              \n"
      "vld              $vr2, %[src], 32               \n"
      "vld              $vr31, %[src], 96              \n"
      "xvpermi.q        $xr2, $xr31, 0x02              \n"
      "vld              $vr3, %[src], 48               \n"
      "vld              $vr31, %[src], 112             \n"
      "xvpermi.q        $xr3, $xr31, 0x02              \n"
      "xvshuf.b         $xr0, $xr0, $xr0, $xr4         \n"
      "xvshuf.b         $xr1, $xr1, $xr1, $xr4         \n"
      "xvshuf.b         $xr2, $xr2, $xr2, $xr4         \n"
      "xvshuf.b         $xr3, $xr3, $xr3, $xr4         \n"
      "xvextrins.w      $xr0, $xr1, 0x30               \n"
      "xvbsrl.v         $xr1, $xr1, 4                  \n"
      "xvextrins.d      $xr1, $xr2, 0x10               \n"
      "xvbsll.v         $xr3, $xr3, 4                  \n"
      "xvextrins.w      $xr3, $xr2, 0x02               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr3, %[dst], 32               \n"
      "xvpermi.q        $xr0, $xr0, 0x01               \n"
      "xvpermi.q        $xr1, $xr1, 0x01               \n"
      "xvpermi.q        $xr3, $xr3, 0x01               \n"
      "vst              $vr0, %[dst], 48               \n"
      "vst              $vr1, %[dst], 64               \n"
      "vst              $vr3, %[dst], 80               \n"
      "addi.d           %[src], %[src], 128            \n"
      "addi.d           %[dst], %[dst], 96             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_24),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(shuf)       // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr31");
}
#endif  // defined(HAS_ARGBTORGB24ROW_LASX) || defined(HAS_ARGBTORAWROW_LASX)

#ifdef HAS_ARGBTORGB24ROW_LASX
void ARGBToRGB24Row_LASX(const uint8_t* src_argb,
                         uint8_t* dst_rgb24,
                         int width) {
  static const uint8_t kShuf[16] = {0,  1,  2,  4,  5, 6, 8, 9,
                                    10, 12, 13, 14, 0, 0, 0, 0};
  ARGBTo24Helper_LASX(src_argb, dst_rgb24, kShuf, width);
}
#endif  // HAS_ARGBTORGB24ROW_LASX

#ifdef HAS_ARGBTORAWROW_LASX
void ARGBToRAWRow_LASX(const uint8_t* src_argb, uint8_t* dst_raw, int width) {
  static const uint8_t kShuf[16] = {2, 1,  0,  6,  5, 4, 10, 9,
                                    8, 14, 13, 12, 0, 0, 0,  0};
  ARGBTo24Helper_LASX(src_argb, dst_raw, kShuf, width);
}
#endif  // HAS_ARGBTORAWROW_LASX

#if defined(HAS_RGB24TOARGBROW_LASX) || defined(HAS_RAWTOARGBROW_LASX) || \
    defined(HAS_RAWTORGBAROW_LASX)
static LIBYUV_NOINLINE void RGB24ToARGBHelper_LASX(const uint8_t* src_24,
                                                   uint8_t* dst_argb,
                                                   const uint8_t* shuf,
                                                   int width) {
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"
      "vld              $vr5, %[shuf], 16              \n"
      "xvpermi.q        $xr5, $xr5, 0x00               \n"
      "xvldi            $xr6, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "vld              $vr1, %[src], 12               \n"
      "vld              $vr31, %[src], 60              \n"
      "xvpermi.q        $xr1, $xr31, 0x02              \n"
      "vld              $vr2, %[src], 24               \n"
      "vld              $vr31, %[src], 72              \n"
      "xvpermi.q        $xr2, $xr31, 0x02              \n"
      "vld              $vr3, %[src], 32               \n"
      "vld              $vr31, %[src], 80              \n"
      "xvpermi.q        $xr3, $xr31, 0x02              \n"
      "xvshuf.b         $xr0, $xr6, $xr0, $xr4         \n"
      "xvshuf.b         $xr1, $xr6, $xr1, $xr4         \n"
      "xvshuf.b         $xr2, $xr6, $xr2, $xr4         \n"
      "xvshuf.b         $xr3, $xr6, $xr3, $xr5         \n"
      "vst              $vr0, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr0, 0x01              \n"
      "vst              $vr31, %[dst], 64              \n"
      "vst              $vr1, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr1, 0x01              \n"
      "vst              $vr31, %[dst], 80              \n"
      "vst              $vr2, %[dst], 32               \n"
      "xvpermi.q        $xr31, $xr2, 0x01              \n"
      "vst              $vr31, %[dst], 96              \n"
      "vst              $vr3, %[dst], 48               \n"
      "xvpermi.q        $xr31, $xr3, 0x01              \n"
      "vst              $vr31, %[dst], 112             \n"
      "addi.d           %[src], %[src], 96             \n"
      "addi.d           %[dst], %[dst], 128            \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_24),    // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(shuf)       // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr31");
}
#endif  // defined(HAS_RGB24TOARGBROW_LASX) || defined(HAS_RAWTOARGBROW_LASX) ||
        // defined(HAS_RAWTORGBAROW_LASX)

#ifdef HAS_RGB24TOARGBROW_LASX
void RGB24ToARGBRow_LASX(const uint8_t* src_rgb24,
                         uint8_t* dst_argb,
                         int width) {
  static const uint8_t kShuf[32] = {0,  1,  2,  16, 3,  4,  5,  16, 6,  7, 8,
                                    16, 9,  10, 11, 16, 4,  5,  6,  16, 7, 8,
                                    9,  16, 10, 11, 12, 16, 13, 14, 15, 16};
  RGB24ToARGBHelper_LASX(src_rgb24, dst_argb, kShuf, width);
}
#endif  // HAS_RGB24TOARGBROW_LASX

#ifdef HAS_RAWTOARGBROW_LASX
void RAWToARGBRow_LASX(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  static const uint8_t kShuf[32] = {2,  1,  0,  16, 5,  4,  3,  16, 8,  7, 6,
                                    16, 11, 10, 9,  16, 6,  5,  4,  16, 9, 8,
                                    7,  16, 12, 11, 10, 16, 15, 14, 13, 16};
  RGB24ToARGBHelper_LASX(src_raw, dst_argb, kShuf, width);
}
#endif  // HAS_RAWTOARGBROW_LASX

#ifdef HAS_RAWTORGBAROW_LASX
void RAWToRGBARow_LASX(const uint8_t* src_raw, uint8_t* dst_rgba, int width) {
  static const uint8_t kShuf[32] = {16, 2,  1,  0,  16, 5,  4,  3,  16, 8,  7,
                                    6,  16, 11, 10, 9,  16, 6,  5,  4,  16, 9,
                                    8,  7,  16, 12, 11, 10, 16, 15, 14, 13};
  RGB24ToARGBHelper_LASX(src_raw, dst_rgba, kShuf, width);
}
#endif  // HAS_RAWTORGBAROW_LASX

#ifdef HAS_RAWTORGB24ROW_LASX
void RAWToRGB24Row_LASX(const uint8_t* src_raw, uint8_t* dst_rgb24, int width) {
  static const uint8_t kShuf[32] = {2,  1,  0,  5,  4,  3,  8, 7, 6, 11, 10,
                                    9,  0,  0,  0,  0,  6,  5, 4, 9, 8,  7,
                                    12, 11, 10, 15, 14, 13, 0, 0, 0, 0};
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"
      "vld              $vr5, %[shuf], 16              \n"
      "xvpermi.q        $xr5, $xr5, 0x00               \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "vld              $vr1, %[src], 12               \n"
      "vld              $vr31, %[src], 60              \n"
      "xvpermi.q        $xr1, $xr31, 0x02              \n"
      "vld              $vr2, %[src], 24               \n"
      "vld              $vr31, %[src], 72              \n"
      "xvpermi.q        $xr2, $xr31, 0x02              \n"
      "vld              $vr3, %[src], 32               \n"
      "vld              $vr31, %[src], 80              \n"
      "xvpermi.q        $xr3, $xr31, 0x02              \n"
      "xvshuf.b         $xr0, $xr0, $xr0, $xr4         \n"
      "xvshuf.b         $xr1, $xr1, $xr1, $xr4         \n"
      "xvshuf.b         $xr2, $xr2, $xr2, $xr4         \n"
      "xvshuf.b         $xr3, $xr3, $xr3, $xr5         \n"
      "xvextrins.w      $xr0, $xr1, 0x30               \n"
      "xvbsrl.v         $xr1, $xr1, 4                  \n"
      "xvextrins.d      $xr1, $xr2, 0x10               \n"
      "xvbsll.v         $xr3, $xr3, 4                  \n"
      "xvextrins.w      $xr3, $xr2, 0x02               \n"
      "vst              $vr0, %[dst], 0                \n"
      "vst              $vr1, %[dst], 16               \n"
      "vst              $vr3, %[dst], 32               \n"
      "xvpermi.q        $xr0, $xr0, 0x01               \n"
      "xvpermi.q        $xr1, $xr1, 0x01               \n"
      "xvpermi.q        $xr3, $xr3, 0x01               \n"
      "vst              $vr0, %[dst], 48               \n"
      "vst              $vr1, %[dst], 64               \n"
      "vst              $vr3, %[dst], 80               \n"
      "addi.d           %[src], %[src], 96             \n"
      "addi.d           %[dst], %[dst], 96             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_raw),    // %[src]
        [dst] "+r"(dst_rgb24),  // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [shuf] "r"(kShuf)       // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr31");
}
#endif  // HAS_RAWTORGB24ROW_LASX

#ifdef HAS_ARGBTORGB565ROW_LASX
void ARGBToRGB565Row_LASX(const uint8_t* src_argb,
                          uint8_t* dst_rgb,
                          int width) {
  asm volatile(
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"
      "xvldi            $xr7, 63                       \n"
      "xvsrli.h         $xr7, $xr7, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr31, %[src], 32              \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr1, $xr31, 0x02              \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvsrli.b         $xr2, $xr2, 3                  \n"
      "xvsrli.b         $xr3, $xr3, 2                  \n"
      "xvand.v          $xr4, $xr2, $xr6               \n"
      "xvsrli.h         $xr2, $xr2, 8                  \n"
      "xvand.v          $xr3, $xr3, $xr7               \n"
      "xvslli.h         $xr3, $xr3, 5                  \n"
      "xvslli.h         $xr2, $xr2, 11                 \n"
      "xvor.v           $xr4, $xr4, $xr3               \n"
      "xvor.v           $xr4, $xr4, $xr2               \n"
      "xvst             $xr4, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_rgb),   // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr6", "$xr7",
        "$xr31");
}
#endif  // HAS_ARGBTORGB565ROW_LASX

#ifdef HAS_ARGBTORGB565DITHERROW_LASX
void ARGBToRGB565DitherRow_LASX(const uint8_t* src_argb,
                                uint8_t* dst_rgb,
                                uint32_t dither4,
                                int width) {
  asm volatile(
      "xvreplgr2vr.w    $xr5, %[dith]                  \n"
      "xvilvl.b         $xr5, $xr5, $xr5               \n"
      "xvilvl.h         $xr5, $xr5, $xr5               \n"
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"
      "xvldi            $xr7, 63                       \n"
      "xvsrli.h         $xr7, $xr7, 8                  \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvsadd.bu        $xr0, $xr0, $xr5               \n"
      "xvpickev.b       $xr2, $xr0, $xr0               \n"
      "xvpickod.b       $xr3, $xr0, $xr0               \n"
      "xvsrli.b         $xr2, $xr2, 3                  \n"
      "xvsrli.b         $xr3, $xr3, 2                  \n"
      "xvand.v          $xr4, $xr2, $xr6               \n"
      "xvsrli.h         $xr2, $xr2, 8                  \n"
      "xvand.v          $xr3, $xr3, $xr7               \n"
      "xvslli.h         $xr3, $xr3, 5                  \n"
      "xvslli.h         $xr2, $xr2, 11                 \n"
      "xvor.v           $xr4, $xr4, $xr3               \n"
      "xvor.v           $xr4, $xr4, $xr2               \n"
      "xvpermi.d        $xr31, $xr4, 0xd8              \n"
      "vst              $vr31, %[dst], 0               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_rgb),   // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [dith] "r"(dither4)    // %[dith]
      : "memory", "$xr0", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6", "$xr7",
        "$xr31");
}
#endif  // HAS_ARGBTORGB565DITHERROW_LASX

#ifdef HAS_ARGBTOARGB1555ROW_LASX
void ARGBToARGB1555Row_LASX(const uint8_t* src_argb,
                            uint8_t* dst_rgb,
                            int width) {
  asm volatile(
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr31, %[src], 32              \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr1, $xr31, 0x02              \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvsrli.b         $xr2, $xr2, 3                  \n"
      "xvand.v          $xr4, $xr2, $xr6               \n"
      "xvsrli.h         $xr2, $xr2, 8                  \n"
      "xvsrli.h         $xr5, $xr3, 15                 \n"
      "xvsrli.b         $xr3, $xr3, 3                  \n"
      "xvand.v          $xr3, $xr3, $xr6               \n"
      "xvslli.h         $xr3, $xr3, 5                  \n"
      "xvslli.h         $xr2, $xr2, 10                 \n"
      "xvslli.h         $xr5, $xr5, 15                 \n"
      "xvor.v           $xr4, $xr4, $xr3               \n"
      "xvor.v           $xr4, $xr4, $xr2               \n"
      "xvor.v           $xr4, $xr4, $xr5               \n"
      "xvst             $xr4, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_rgb),   // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr31");
}
#endif  // HAS_ARGBTOARGB1555ROW_LASX

#ifdef HAS_ARGBTOARGB4444ROW_LASX
void ARGBToARGB4444Row_LASX(const uint8_t* src_argb,
                            uint8_t* dst_rgb,
                            int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vld              $vr31, %[src], 32              \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "vld              $vr1, %[src], 16               \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr1, $xr31, 0x02              \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvsrli.b         $xr2, $xr2, 4                  \n"
      "xvsrli.b         $xr3, $xr3, 4                  \n"
      "xvslli.b         $xr3, $xr3, 4                  \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_rgb),   // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr31");
}
#endif  // HAS_ARGBTOARGB4444ROW_LASX

#ifdef HAS_RGB565TOARGBROW_LASX
void RGB565ToARGBRow_LASX(const uint8_t* src_rgb565,
                          uint8_t* dst_argb,
                          int width) {
  asm volatile(
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"
      "xvldi            $xr7, 63                       \n"
      "xvsrli.h         $xr7, $xr7, 8                  \n"
      "xvldi            $xr5, 0xff                     \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr7               \n"
      "xvsrli.h         $xr3, $xr0, 11                 \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 2                  \n"
      "xvsrli.h         $xr2, $xr2, 4                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvpickev.b       $xr1, $xr1, $xr1               \n"
      "xvpickev.b       $xr2, $xr2, $xr2               \n"
      "xvpickev.b       $xr3, $xr3, $xr3               \n"
      "xvilvl.b         $xr8, $xr2, $xr1               \n"
      "xvilvl.b         $xr9, $xr5, $xr3               \n"
      "xvilvl.h         $xr10, $xr9, $xr8              \n"
      "xvilvh.h         $xr11, $xr9, $xr8              \n"
      "vst              $vr10, %[dst], 0               \n"
      "xvpermi.q        $xr31, $xr10, 0x01             \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr11, %[dst], 16              \n"
      "xvpermi.q        $xr31, $xr11, 0x01             \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb565),  // %[src]
        [dst] "+r"(dst_argb),    // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr31");
}
#endif  // HAS_RGB565TOARGBROW_LASX

#ifdef HAS_ARGB1555TOARGBROW_LASX
void ARGB1555ToARGBRow_LASX(const uint8_t* src_argb1555,
                            uint8_t* dst_argb,
                            int width) {
  asm volatile(
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvsrli.h         $xr3, $xr0, 10                 \n"
      "xvand.v          $xr3, $xr3, $xr6               \n"
      "xvsrai.h         $xr5, $xr0, 15                 \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 3                  \n"
      "xvsrli.h         $xr2, $xr2, 2                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvpickev.b       $xr1, $xr1, $xr1               \n"
      "xvpickev.b       $xr2, $xr2, $xr2               \n"
      "xvpickev.b       $xr3, $xr3, $xr3               \n"
      "xvpickev.b       $xr5, $xr5, $xr5               \n"
      "xvilvl.b         $xr8, $xr2, $xr1               \n"
      "xvilvl.b         $xr9, $xr5, $xr3               \n"
      "xvilvl.h         $xr10, $xr9, $xr8              \n"
      "xvilvh.h         $xr11, $xr9, $xr8              \n"
      "vst              $vr10, %[dst], 0               \n"
      "xvpermi.q        $xr31, $xr10, 0x01             \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr11, %[dst], 16              \n"
      "xvpermi.q        $xr31, $xr11, 0x01             \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb1555),  // %[src]
        [dst] "+r"(dst_argb),      // %[dst]
        [cnt] "+r"(width)          // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr8", "$xr9", "$xr10", "$xr11", "$xr31");
}
#endif  // HAS_ARGB1555TOARGBROW_LASX

#ifdef HAS_ARGB4444TOARGBROW_LASX
void ARGB4444ToARGBRow_LASX(const uint8_t* src_argb4444,
                            uint8_t* dst_argb,
                            int width) {
  asm volatile(
      "xvldi            $xr6, 0x0f                     \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.b         $xr2, $xr0, 4                  \n"
      "xvslli.b         $xr3, $xr1, 4                  \n"
      "xvor.v           $xr1, $xr1, $xr3               \n"
      "xvslli.b         $xr3, $xr2, 4                  \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvilvl.b         $xr4, $xr2, $xr1               \n"
      "xvilvh.b         $xr5, $xr2, $xr1               \n"
      "vst              $vr4, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr4, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr5, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr5, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb4444),  // %[src]
        [dst] "+r"(dst_argb),      // %[dst]
        [cnt] "+r"(width)          // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr31");
}
#endif  // HAS_ARGB4444TOARGBROW_LASX

#ifdef HAS_ARGBMULTIPLYROW_LASX
void ARGBMultiplyRow_LASX(const uint8_t* src_argb,
                          const uint8_t* src_argb1,
                          uint8_t* dst_argb,
                          int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s1], 0                 \n"
      "xvmulwev.h.bu    $xr2, $xr0, $xr1               \n"
      "xvmulwod.h.bu    $xr3, $xr0, $xr1               \n"
      "xvsrari.h        $xr2, $xr2, 8                  \n"
      "xvsrari.h        $xr3, $xr3, 8                  \n"
      "xvpackev.b       $xr2, $xr3, $xr2               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb),   // %[s0]
        [s1] "+r"(src_argb1),  // %[s1]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_ARGBMULTIPLYROW_LASX

#ifdef HAS_ARGBADDROW_LASX
void ARGBAddRow_LASX(const uint8_t* src_argb,
                     const uint8_t* src_argb1,
                     uint8_t* dst_argb,
                     int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s1], 0                 \n"
      "xvsadd.bu        $xr0, $xr0, $xr1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb),   // %[s0]
        [s1] "+r"(src_argb1),  // %[s1]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_ARGBADDROW_LASX

#ifdef HAS_ARGBSUBTRACTROW_LASX
void ARGBSubtractRow_LASX(const uint8_t* src_argb,
                          const uint8_t* src_argb1,
                          uint8_t* dst_argb,
                          int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s1], 0                 \n"
      "xvssub.bu        $xr0, $xr0, $xr1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb),   // %[s0]
        [s1] "+r"(src_argb1),  // %[s1]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_ARGBSUBTRACTROW_LASX

#ifdef HAS_ARGBATTENUATEROW_LASX
void ARGBAttenuateRow_LASX(const uint8_t* src_argb,
                           uint8_t* dst_argb,
                           int width) {
  static const uint8_t kAlpha[16] = {3,  3,  3,  16, 7,  7,  7,  16,
                                     11, 11, 11, 16, 15, 15, 15, 16};
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"
      "xvldi            $xr5, 0xff                     \n"
      "xvldi            $xr6, 255                      \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvshuf.b         $xr1, $xr5, $xr0, $xr4         \n"
      "xvmulwev.h.bu    $xr2, $xr0, $xr1               \n"
      "xvmulwod.h.bu    $xr3, $xr0, $xr1               \n"
      "xvadd.h          $xr2, $xr2, $xr6               \n"
      "xvadd.h          $xr3, $xr3, $xr6               \n"
      "xvpackod.b       $xr2, $xr3, $xr2               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(kAlpha)     // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_ARGBATTENUATEROW_LASX

#ifdef HAS_ARGBUNATTENUATEROW_LASX
void ARGBUnattenuateRow_LASX(const uint8_t* src_argb,
                             uint8_t* dst_argb,
                             int width) {
  uint64_t t0, t1;
  asm volatile(
      "addi.w           %[cnt], %[cnt], -8             \n"
      "bltz             %[cnt], 2f                     \n"
      "xvldi            $xr5, 0xff                     \n"
      "xvslli.w         $xr5, $xr5, 24                 \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "ld.bu            %[t0], %[src], 3               \n"
      "ld.bu            %[t1], %[src], 7               \n"
      "slli.d           %[t0], %[t0], 2                \n"
      "slli.d           %[t1], %[t1], 2                \n"
      "ldx.wu           %[t0], %[tbl], %[t0]           \n"
      "ldx.wu           %[t1], %[tbl], %[t1]           \n"
      "xvinsgr2vr.w     $xr3, %[t0], 0                 \n"
      "xvinsgr2vr.w     $xr3, %[t1], 2                 \n"
      "ld.bu            %[t0], %[src], 19              \n"
      "ld.bu            %[t1], %[src], 23              \n"
      "slli.d           %[t0], %[t0], 2                \n"
      "slli.d           %[t1], %[t1], 2                \n"
      "ldx.wu           %[t0], %[tbl], %[t0]           \n"
      "ldx.wu           %[t1], %[tbl], %[t1]           \n"
      "xvinsgr2vr.w     $xr3, %[t0], 4                 \n"
      "xvinsgr2vr.w     $xr3, %[t1], 6                 \n"
      "xvshuf4i.h       $xr3, $xr3, 0x40               \n"
      "ld.bu            %[t0], %[src], 11              \n"
      "ld.bu            %[t1], %[src], 15              \n"
      "slli.d           %[t0], %[t0], 2                \n"
      "slli.d           %[t1], %[t1], 2                \n"
      "ldx.wu           %[t0], %[tbl], %[t0]           \n"
      "ldx.wu           %[t1], %[tbl], %[t1]           \n"
      "xvinsgr2vr.w     $xr4, %[t0], 0                 \n"
      "xvinsgr2vr.w     $xr4, %[t1], 2                 \n"
      "ld.bu            %[t0], %[src], 27              \n"
      "ld.bu            %[t1], %[src], 31              \n"
      "slli.d           %[t0], %[t0], 2                \n"
      "slli.d           %[t1], %[t1], 2                \n"
      "ldx.wu           %[t0], %[tbl], %[t0]           \n"
      "ldx.wu           %[t1], %[tbl], %[t1]           \n"
      "xvinsgr2vr.w     $xr4, %[t0], 4                 \n"
      "xvinsgr2vr.w     $xr4, %[t1], 6                 \n"
      "xvshuf4i.h       $xr4, $xr4, 0x40               \n"
      "xvilvl.b         $xr1, $xr0, $xr0               \n"
      "xvilvh.b         $xr2, $xr0, $xr0               \n"
      "xvmuh.hu         $xr1, $xr1, $xr3               \n"
      "xvmuh.hu         $xr2, $xr2, $xr4               \n"
      "xvsat.hu         $xr1, $xr1, 7                  \n"
      "xvsat.hu         $xr2, $xr2, 7                  \n"
      "xvpickev.b       $xr1, $xr2, $xr1               \n"
      "xvand.v          $xr0, $xr0, $xr5               \n"
      "xvandn.v         $xr1, $xr5, $xr1               \n"
      "xvor.v           $xr1, $xr1, $xr0               \n"
      "xvst             $xr1, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.w           %[cnt], %[cnt], -8             \n"
      "bgez             %[cnt], 1b                     \n"

      "2:                                              \n"
      "addi.w           %[cnt], %[cnt], 8              \n"
      : [src] "+r"(src_argb),     // %[src]
        [dst] "+r"(dst_argb),     // %[dst]
        [cnt] "+r"(width),        // %[cnt]
        [t0] "=&r"(t0),           // %[t0]
        [t1] "=&r"(t1)            // %[t1]
      : [tbl] "r"(fixed_invtbl8)  // %[tbl]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
  if (width > 0) {
    ARGBUnattenuateRow_C(src_argb, dst_argb, width);
  }
}
#endif  // HAS_ARGBUNATTENUATEROW_LASX

#ifdef HAS_ARGBBLENDROW_LASX
void ARGBBlendRow_LASX(const uint8_t* src_argb0,
                       const uint8_t* src_argb1,
                       uint8_t* dst_argb,
                       int width) {
  static const uint8_t kAlpha[16] = {3,  3,  3,  3,  7,  7,  7,  7,
                                     11, 11, 11, 11, 15, 15, 15, 15};
  asm volatile(
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"
      "xvldi            $xr5, 0xff                     \n"
      "xvslli.w         $xr5, $xr5, 24                 \n"
      "xvldi            $xr6, 1                        \n"
      "xvslli.h         $xr6, $xr6, 8                  \n"
      "xvldi            $xr8, 0                        \n"

      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s1], 0                 \n"
      "xvshuf.b         $xr2, $xr0, $xr0, $xr4         \n"
      "xvilvl.b         $xr3, $xr8, $xr2               \n"
      "xvilvh.b         $xr2, $xr8, $xr2               \n"
      "xvsub.h          $xr3, $xr6, $xr3               \n"
      "xvsub.h          $xr2, $xr6, $xr2               \n"
      "xvilvl.b         $xr7, $xr8, $xr1               \n"
      "xvilvh.b         $xr1, $xr8, $xr1               \n"
      "xvmul.h          $xr7, $xr7, $xr3               \n"
      "xvmul.h          $xr1, $xr1, $xr2               \n"
      "xvsrlni.b.h      $xr1, $xr7, 8                  \n"
      "xvsadd.bu        $xr0, $xr0, $xr1               \n"
      "xvor.v           $xr0, $xr0, $xr5               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb0),  // %[s0]
        [s1] "+r"(src_argb1),  // %[s1]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(kAlpha)     // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8");
}
#endif  // HAS_ARGBBLENDROW_LASX

#ifdef HAS_ARGBORROW_LASX
void ARGBOrRow_LASX(const uint8_t* src_argb,
                    uint8_t* dst_argb,
                    uint32_t value,
                    int width) {
  asm volatile(
      "xvreplgr2vr.w    $xr1, %[val]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvor.v           $xr0, $xr0, $xr1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [val] "r"(value)       // %[val]
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_ARGBORROW_LASX

#ifdef HAS_ARGBALPHABLENDROW_LASX
void ARGBAlphaBlendRow_LASX(const uint8_t* src_argb0,
                            const uint8_t* src_argb1,
                            uint8_t* dst_argb,
                            int width) {
  static const uint8_t kShufRatio[16] = {0, 0, 0, 0, 4,  4,  4,  4,
                                         8, 8, 8, 8, 12, 12, 12, 12};
  asm volatile(
      "vld              $vr8, %[shuf], 0               \n"
      "xvpermi.q        $xr8, $xr8, 0x00               \n"
      "xvldi            $xr9, 1                        \n"
      "xvsrli.h         $xr10, $xr9, 8                 \n"
      "xvldi            $xr11, 0xff                    \n"
      "xvsrli.w         $xr11, $xr11, 8                \n"
      "xvldi            $xr12, 0                       \n"

      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s1], 0                 \n"
      "xvsrli.w         $xr2, $xr0, 24                 \n"
      "xvsrli.w         $xr3, $xr1, 24                 \n"
      "xvmul.h          $xr4, $xr3, $xr2               \n"
      "xvadd.h          $xr4, $xr4, $xr10              \n"
      "xvmuh.hu         $xr4, $xr4, $xr9               \n"
      "xvsub.w          $xr4, $xr2, $xr4               \n"
      "xvadd.w          $xr4, $xr3, $xr4               \n"
      "xvseq.w          $xr5, $xr3, $xr12              \n"
      "xvsrli.w         $xr5, $xr5, 24                 \n"
      "xvor.v           $xr6, $xr2, $xr5               \n"
      "xvor.v           $xr7, $xr4, $xr5               \n"
      "xvslli.w         $xr5, $xr6, 8                  \n"
      "xvsub.w          $xr5, $xr5, $xr6               \n"
      "xvffint.s.wu     $xr5, $xr5                     \n"
      "xvffint.s.wu     $xr7, $xr7                     \n"
      "xvfdiv.s         $xr5, $xr5, $xr7               \n"
      "xvftintrz.wu.s   $xr5, $xr5                     \n"
      "xvshuf.b         $xr5, $xr5, $xr5, $xr8         \n"
      "xvnor.v          $xr6, $xr5, $xr5               \n"
      "xvmulwev.h.bu    $xr2, $xr1, $xr6               \n"
      "xvmulwod.h.bu    $xr3, $xr1, $xr6               \n"
      "xvmulwev.h.bu    $xr7, $xr0, $xr5               \n"
      "xvmulwod.h.bu    $xr0, $xr0, $xr5               \n"
      "xvadd.h          $xr2, $xr2, $xr7               \n"
      "xvadd.h          $xr3, $xr3, $xr0               \n"
      "xvadd.h          $xr2, $xr2, $xr10              \n"
      "xvadd.h          $xr3, $xr3, $xr10              \n"
      "xvmuh.hu         $xr2, $xr2, $xr9               \n"
      "xvmuh.hu         $xr3, $xr3, $xr9               \n"
      "xvslli.h         $xr3, $xr3, 8                  \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvand.v          $xr2, $xr2, $xr11              \n"
      "xvslli.w         $xr4, $xr4, 24                 \n"
      "xvor.v           $xr2, $xr2, $xr4               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb0),   // %[s0]
        [s1] "+r"(src_argb1),   // %[s1]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [shuf] "r"(kShufRatio)  // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12");
}
#endif  // HAS_ARGBALPHABLENDROW_LASX

#ifdef HAS_ARGBSHADEROW_LASX
void ARGBShadeRow_LASX(const uint8_t* src_argb,
                       uint8_t* dst_argb,
                       int width,
                       uint32_t value) {
  asm volatile(
      "xvreplgr2vr.w    $xr4, %[val]                   \n"
      "xvilvl.b         $xr5, $xr4, $xr4               \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvilvl.b         $xr1, $xr0, $xr0               \n"
      "xvilvh.b         $xr2, $xr0, $xr0               \n"
      "xvmuh.hu         $xr1, $xr1, $xr5               \n"
      "xvmuh.hu         $xr2, $xr2, $xr5               \n"
      "xvsrlni.b.h      $xr2, $xr1, 8                  \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [val] "r"(value)       // %[val]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr4", "$xr5");
}
#endif  // HAS_ARGBSHADEROW_LASX

#ifdef HAS_AR64SHUFFLEROW_LASX
void AR64ShuffleRow_LASX(const uint8_t* src_ar64,
                         uint8_t* dst_ar64,
                         const uint8_t* shuffler,
                         int width) {
  asm volatile(
      "bge              $zero, %[cnt], 2f              \n"
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvshuf.b         $xr0, $xr0, $xr0, $xr4         \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -4             \n"
      "blt              $zero, %[cnt], 1b              \n"

      "2:                                              \n"
      : [src] "+r"(src_ar64),  // %[src]
        [dst] "+r"(dst_ar64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(shuffler)   // %[shuf]
      : "memory", "$xr0", "$xr4");
}
#endif  // HAS_AR64SHUFFLEROW_LASX

#ifdef HAS_ARGBSHUFFLEROW_LASX
void ARGBShuffleRow_LASX(const uint8_t* src_argb,
                         uint8_t* dst_argb,
                         const uint8_t* shuffler,
                         int width) {
  asm volatile(
      "bge              $zero, %[cnt], 2f              \n"
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvshuf.b         $xr0, $xr0, $xr0, $xr4         \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "blt              $zero, %[cnt], 1b              \n"

      "2:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shuf] "r"(shuffler)   // %[shuf]
      : "memory", "$xr0", "$xr4");
}
#endif  // HAS_ARGBSHUFFLEROW_LASX

#ifdef HAS_ARGBGRAYROW_LASX
void ARGBGrayRow_LASX(const uint8_t* src_argb, uint8_t* dst_argb, int width) {
  asm volatile(
      "xvldi            $xr4, 29                       \n"
      "xvldi            $xr5, 150                      \n"
      "xvldi            $xr6, 77                       \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvmulwev.h.bu    $xr7, $xr2, $xr4               \n"
      "xvmaddwod.h.bu   $xr7, $xr2, $xr6               \n"
      "xvmaddwev.h.bu   $xr7, $xr3, $xr5               \n"
      "xvsrlri.h        $xr7, $xr7, 8                  \n"
      "xvsrli.h         $xr3, $xr3, 8                  \n"
      "xvslli.h         $xr3, $xr3, 8                  \n"
      "xvor.v           $xr3, $xr7, $xr3               \n"
      "xvslli.h         $xr8, $xr7, 8                  \n"
      "xvor.v           $xr7, $xr7, $xr8               \n"
      "xvilvl.h         $xr0, $xr3, $xr7               \n"
      "xvilvh.h         $xr1, $xr3, $xr7               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8");
}
#endif  // HAS_ARGBGRAYROW_LASX

#ifdef HAS_ARGBSEPIAROW_LASX
void ARGBSepiaRow_LASX(uint8_t* dst_argb, int width) {
  asm volatile(
      "xvldi            $xr4, 17                       \n"
      "xvldi            $xr5, 68                       \n"
      "xvldi            $xr6, 35                       \n"
      "xvldi            $xr7, 22                       \n"
      "xvldi            $xr8, 88                       \n"
      "xvldi            $xr9, 45                       \n"
      "xvldi            $xr10, 24                      \n"
      "xvldi            $xr11, 98                      \n"
      "xvldi            $xr12, 50                      \n"

      "1:                                              \n"
      "xvld             $xr0, %[dst], 0                \n"
      "xvld             $xr1, %[dst], 32               \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvmulwev.h.bu    $xr13, $xr2, $xr4              \n"
      "xvmaddwod.h.bu   $xr13, $xr2, $xr6              \n"
      "xvmaddwev.h.bu   $xr13, $xr3, $xr5              \n"
      "xvmulwev.h.bu    $xr14, $xr2, $xr7              \n"
      "xvmaddwod.h.bu   $xr14, $xr2, $xr9              \n"
      "xvmaddwev.h.bu   $xr14, $xr3, $xr8              \n"
      "xvmulwev.h.bu    $xr15, $xr2, $xr10             \n"
      "xvmaddwod.h.bu   $xr15, $xr2, $xr12             \n"
      "xvmaddwev.h.bu   $xr15, $xr3, $xr11             \n"
      "xvsrli.h         $xr13, $xr13, 7                \n"
      "xvsrli.h         $xr14, $xr14, 7                \n"
      "xvsrli.h         $xr15, $xr15, 7                \n"
      "xvsat.hu         $xr13, $xr13, 7                \n"
      "xvsat.hu         $xr14, $xr14, 7                \n"
      "xvsat.hu         $xr15, $xr15, 7                \n"
      "xvpickev.b       $xr13, $xr13, $xr13            \n"
      "xvpickev.b       $xr14, $xr14, $xr14            \n"
      "xvpickev.b       $xr15, $xr15, $xr15            \n"
      "xvpickod.b       $xr3, $xr3, $xr3               \n"
      "xvilvl.b         $xr2, $xr14, $xr13             \n"
      "xvilvl.b         $xr3, $xr3, $xr15              \n"
      "xvilvl.h         $xr0, $xr3, $xr2               \n"
      "xvilvh.h         $xr1, $xr3, $xr2               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14",
        "$xr15");
}
#endif  // HAS_ARGBSEPIAROW_LASX

#ifdef HAS_ARGBCOLORMATRIXROW_LASX
void ARGBColorMatrixRow_LASX(const uint8_t* src_argb,
                             uint8_t* dst_argb,
                             const int8_t* matrix_argb,
                             int width) {
  asm volatile(
      "vldrepl.w        $vr4, %[mat], 0                \n"
      "vldrepl.w        $vr31, %[mat], 0               \n"
      "xvpermi.q        $xr4, $xr31, 0x02              \n"
      "vldrepl.w        $vr5, %[mat], 4                \n"
      "vldrepl.w        $vr31, %[mat], 4               \n"
      "xvpermi.q        $xr5, $xr31, 0x02              \n"
      "vldrepl.w        $vr6, %[mat], 8                \n"
      "vldrepl.w        $vr31, %[mat], 8               \n"
      "xvpermi.q        $xr6, $xr31, 0x02              \n"
      "vldrepl.w        $vr7, %[mat], 12               \n"
      "vldrepl.w        $vr31, %[mat], 12              \n"
      "xvpermi.q        $xr7, $xr31, 0x02              \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvmulwev.h.bu.b  $xr8, $xr0, $xr4               \n"
      "xvmaddwod.h.bu.b $xr8, $xr0, $xr4               \n"
      "xvmulwev.h.bu.b  $xr9, $xr0, $xr5               \n"
      "xvmaddwod.h.bu.b $xr9, $xr0, $xr5               \n"
      "xvmulwev.h.bu.b  $xr10, $xr0, $xr6              \n"
      "xvmaddwod.h.bu.b $xr10, $xr0, $xr6              \n"
      "xvmulwev.h.bu.b  $xr11, $xr0, $xr7              \n"
      "xvmaddwod.h.bu.b $xr11, $xr0, $xr7              \n"
      "xvhaddw.w.h      $xr8, $xr8, $xr8               \n"
      "xvhaddw.w.h      $xr9, $xr9, $xr9               \n"
      "xvhaddw.w.h      $xr10, $xr10, $xr10            \n"
      "xvhaddw.w.h      $xr11, $xr11, $xr11            \n"
      "xvilvl.w         $xr12, $xr9, $xr8              \n"
      "xvilvh.w         $xr13, $xr9, $xr8              \n"
      "xvilvl.w         $xr14, $xr11, $xr10            \n"
      "xvilvh.w         $xr15, $xr11, $xr10            \n"
      "xvilvl.d         $xr8, $xr14, $xr12             \n"
      "xvilvh.d         $xr9, $xr14, $xr12             \n"
      "xvilvl.d         $xr10, $xr15, $xr13            \n"
      "xvilvh.d         $xr11, $xr15, $xr13            \n"
      "xvssrani.h.w     $xr9, $xr8, 6                  \n"
      "xvssrani.h.w     $xr11, $xr10, 6                \n"
      "xvssrani.bu.h    $xr11, $xr9, 0                 \n"
      "xvst             $xr11, %[dst], 0               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),   // %[src]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [mat] "r"(matrix_argb)  // %[mat]
      : "memory", "$xr0", "$xr4", "$xr5", "$xr6", "$xr7", "$xr8", "$xr9",
        "$xr10", "$xr11", "$xr12", "$xr13", "$xr14", "$xr15", "$xr31");
}
#endif  // HAS_ARGBCOLORMATRIXROW_LASX

#ifdef HAS_ARGBQUANTIZEROW_LASX
void ARGBQuantizeRow_LASX(uint8_t* dst_argb,
                          int scale,
                          int interval_size,
                          int interval_offset,
                          int width) {
  asm volatile(
      "xvreplgr2vr.h    $xr4, %[scl]                   \n"
      "xvreplgr2vr.b    $xr5, %[isz]                   \n"
      "xvreplgr2vr.b    $xr6, %[ioff]                  \n"
      "xvldi            $xr7, 0xff                     \n"
      "xvslli.w         $xr7, $xr7, 24                 \n"

      "1:                                              \n"
      "xvld             $xr0, %[dst], 0                \n"
      "xvilvl.b         $xr1, $xr0, $xr0               \n"
      "xvilvh.b         $xr2, $xr0, $xr0               \n"
      "xvsrli.h         $xr1, $xr1, 8                  \n"
      "xvsrli.h         $xr2, $xr2, 8                  \n"
      "xvmuh.hu         $xr1, $xr1, $xr4               \n"
      "xvmuh.hu         $xr2, $xr2, $xr4               \n"
      "xvsrlni.b.h      $xr2, $xr1, 0                  \n"
      "xvmul.b          $xr2, $xr2, $xr5               \n"
      "xvadd.b          $xr2, $xr2, $xr6               \n"
      "xvand.v          $xr0, $xr0, $xr7               \n"
      "xvandn.v         $xr2, $xr7, $xr2               \n"
      "xvor.v           $xr2, $xr2, $xr0               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [dst] "+r"(dst_argb),        // %[dst]
        [cnt] "+r"(width)            // %[cnt]
      : [scl] "r"(scale),            // %[scl]
        [isz] "r"(interval_size),    // %[isz]
        [ioff] "r"(interval_offset)  // %[ioff]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr4", "$xr5", "$xr6", "$xr7");
}
#endif  // HAS_ARGBQUANTIZEROW_LASX

#ifdef HAS_SOBELROW_LASX
void SobelRow_LASX(const uint8_t* src_sobelx,
                   const uint8_t* src_sobely,
                   uint8_t* dst_argb,
                   int width) {
  asm volatile(
      "xvldi            $xr4, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sx], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vld              $vr1, %[sy], 0                 \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvsadd.bu        $xr0, $xr0, $xr1               \n"
      "xvilvl.b         $xr2, $xr0, $xr0               \n"
      "xvilvl.b         $xr3, $xr4, $xr0               \n"
      "xvilvl.h         $xr0, $xr3, $xr2               \n"
      "xvilvh.h         $xr1, $xr3, $xr2               \n"
      "vst              $vr0, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr0, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr1, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr1, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sx], %[sx], 16               \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sx] "+r"(src_sobelx),  // %[sx]
        [sy] "+r"(src_sobely),  // %[sy]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr31");
}
#endif  // HAS_SOBELROW_LASX

#ifdef HAS_SOBELTOPLANEROW_LASX
void SobelToPlaneRow_LASX(const uint8_t* src_sobelx,
                          const uint8_t* src_sobely,
                          uint8_t* dst_y,
                          int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[sx], 0                 \n"
      "xvld             $xr1, %[sy], 0                 \n"
      "xvsadd.bu        $xr0, $xr0, $xr1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[sx], %[sx], 32               \n"
      "addi.d           %[sy], %[sy], 32               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sx] "+r"(src_sobelx),  // %[sx]
        [sy] "+r"(src_sobely),  // %[sy]
        [dst] "+r"(dst_y),      // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_SOBELTOPLANEROW_LASX

#ifdef HAS_SOBELXYROW_LASX
void SobelXYRow_LASX(const uint8_t* src_sobelx,
                     const uint8_t* src_sobely,
                     uint8_t* dst_argb,
                     int width) {
  asm volatile(
      "xvldi            $xr4, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sx], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vld              $vr1, %[sy], 0                 \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvsadd.bu        $xr2, $xr0, $xr1               \n"
      "xvilvl.b         $xr1, $xr2, $xr1               \n"
      "xvilvl.b         $xr0, $xr4, $xr0               \n"
      "xvilvl.h         $xr2, $xr0, $xr1               \n"
      "xvilvh.h         $xr3, $xr0, $xr1               \n"
      "vst              $vr2, %[dst], 0                \n"
      "xvpermi.q        $xr31, $xr2, 0x01              \n"
      "vst              $vr31, %[dst], 32              \n"
      "vst              $vr3, %[dst], 16               \n"
      "xvpermi.q        $xr31, $xr3, 0x01              \n"
      "vst              $vr31, %[dst], 48              \n"
      "addi.d           %[sx], %[sx], 16               \n"
      "addi.d           %[sy], %[sy], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sx] "+r"(src_sobelx),  // %[sx]
        [sy] "+r"(src_sobely),  // %[sy]
        [dst] "+r"(dst_argb),   // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr31");
}
#endif  // HAS_SOBELXYROW_LASX

#ifdef HAS_INTERPOLATEROW_LASX
void InterpolateRow_LASX(uint8_t* dst_ptr,
                         const uint8_t* src_ptr,
                         ptrdiff_t src_stride,
                         int width,
                         int32_t source_y_fraction) {
  int y1_fraction = source_y_fraction;
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  if (y1_fraction == 0) {
    CopyRow_LASX(src_ptr, dst_ptr, width);
    return;
  }
  if (y1_fraction == 128) {
    asm volatile(
        "1:                                              \n"
        "xvld             $xr0, %[s0], 0                 \n"
        "xvld             $xr1, %[s1], 0                 \n"
        "xvavgr.bu        $xr0, $xr0, $xr1               \n"
        "xvst             $xr0, %[dst], 0                \n"
        "addi.d           %[s0], %[s0], 32               \n"
        "addi.d           %[s1], %[s1], 32               \n"
        "addi.d           %[dst], %[dst], 32             \n"
        "addi.d           %[cnt], %[cnt], -32            \n"
        "bnez             %[cnt], 1b                     \n"
        : [s0] "+r"(src_ptr),   // %[s0]
          [s1] "+r"(src_ptr1),  // %[s1]
          [dst] "+r"(dst_ptr),  // %[dst]
          [cnt] "+r"(width)     // %[cnt]
        :
        : "memory", "$xr0", "$xr1");
    return;
  }
  uint16_t y_frac = (uint16_t)((256 - y1_fraction) + (y1_fraction << 8));
  asm volatile(
      "xvreplgr2vr.h    $xr3, %[yf]                    \n"

      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s1], 0                 \n"
      "xvilvl.b         $xr2, $xr1, $xr0               \n"
      "xvilvh.b         $xr4, $xr1, $xr0               \n"
      "xvmulwev.h.bu    $xr0, $xr2, $xr3               \n"
      "xvmaddwod.h.bu   $xr0, $xr2, $xr3               \n"
      "xvmulwev.h.bu    $xr1, $xr4, $xr3               \n"
      "xvmaddwod.h.bu   $xr1, $xr4, $xr3               \n"
      "xvsrlrni.b.h     $xr1, $xr0, 8                  \n"
      "xvst             $xr1, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_ptr),   // %[s0]
        [s1] "+r"(src_ptr1),  // %[s1]
        [dst] "+r"(dst_ptr),  // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      : [yf] "r"(y_frac)      // %[yf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_INTERPOLATEROW_LASX

#ifdef HAS_HALFFLOATROW_LASX
void HalfFloatRow_LASX(const uint16_t* src,
                       uint16_t* dst,
                       float scale,
                       int width) {
  const float bias = 1.9259299444e-34f;  // 2^-112, see HalfFloatRow_C
  asm volatile(
      "xvldrepl.w       $xr3, %[scale], 0              \n"
      "xvldrepl.w       $xr5, %[bias], 0               \n"
      "xvldi            $xr4, 0                        \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvilvl.h         $xr1, $xr4, $xr0               \n"
      "xvilvh.h         $xr2, $xr4, $xr0               \n"
      "xvffint.s.wu     $xr1, $xr1                     \n"
      "xvffint.s.wu     $xr2, $xr2                     \n"
      "xvfmul.s         $xr1, $xr1, $xr3               \n"
      "xvfmul.s         $xr2, $xr2, $xr3               \n"
      "xvfmul.s         $xr1, $xr1, $xr5               \n"
      "xvfmul.s         $xr2, $xr2, $xr5               \n"
      "xvsrlni.h.w      $xr2, $xr1, 13                 \n"
      "xvst             $xr2, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),      // %[src]
        [dst] "+r"(dst),      // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      : [scale] "r"(&scale),  // %[scale]
        [bias] "r"(&bias)     // %[bias]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_HALFFLOATROW_LASX

#ifdef HAS_BYTETOFLOATROW_LASX
void ByteToFloatRow_LASX(const uint8_t* src,
                         float* dst,
                         float scale,
                         int width) {
  uint32_t scale_bits;
  memcpy(&scale_bits, &scale, sizeof(scale_bits));
  asm volatile(
      "xvreplgr2vr.w    $xr3, %[scale]                 \n"
      "xvldi            $xr4, 0                        \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvilvl.b         $xr0, $xr4, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvilvl.h         $xr1, $xr4, $xr0               \n"
      "xvilvh.h         $xr2, $xr4, $xr0               \n"
      "xvffint.s.wu     $xr1, $xr1                     \n"
      "xvffint.s.wu     $xr2, $xr2                     \n"
      "xvfmul.s         $xr1, $xr1, $xr3               \n"
      "xvfmul.s         $xr2, $xr2, $xr3               \n"
      "xvst             $xr1, %[dst], 0                \n"
      "xvst             $xr2, %[dst], 32               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),         // %[src]
        [dst] "+r"(dst),         // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      : [scale] "r"(scale_bits)  // %[scale]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_BYTETOFLOATROW_LASX

#ifdef HAS_ARGBTOYMATRIXROW_LASX
void ARGBToYMatrixRow_LASX(const uint8_t* src_argb,
                           uint8_t* dst_y,
                           int width,
                           const struct ArgbConstants* c) {
  uint16_t add_y;
  __builtin_memcpy(&add_y, &c->kAddY[0], sizeof(add_y));
  asm volatile(
      "xvldrepl.b       $xr0, %[c], 0                  \n"
      "xvldrepl.b       $xr1, %[c], 1                  \n"
      "xvldrepl.b       $xr2, %[c], 2                  \n"
      "xvldrepl.b       $xr3, %[c], 3                  \n"
      "xvreplgr2vr.h    $xr4, %[add_y]                 \n"

      "1:                                              \n"
      "vld              $vr5, %[src], 0                \n"
      "vld              $vr31, %[src], 32              \n"
      "xvpermi.q        $xr5, $xr31, 0x02              \n"
      "vld              $vr6, %[src], 16               \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr6, $xr31, 0x02              \n"
      "xvor.v           $xr9, $xr4, $xr4               \n"
      "xvpickev.b       $xr7, $xr6, $xr5               \n"
      "xvpickod.b       $xr8, $xr6, $xr5               \n"
      "xvmaddwev.h.bu   $xr9, $xr7, $xr0               \n"
      "xvmaddwev.h.bu   $xr9, $xr8, $xr1               \n"
      "xvmaddwod.h.bu   $xr9, $xr7, $xr2               \n"
      "xvmaddwod.h.bu   $xr9, $xr8, $xr3               \n"
      "xvpickod.b       $xr9, $xr9, $xr9               \n"
      "xvpermi.d        $xr31, $xr9, 0xd8              \n"
      "vst              $vr31, %[dst], 0               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_y] "r"(add_y)     // %[add_y]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr31");
}
#endif  // HAS_ARGBTOYMATRIXROW_LASX

#ifdef HAS_ARGBTOUV444MATRIXROW_LASX
void ARGBToUV444MatrixRow_LASX(const uint8_t* src_argb,
                               uint8_t* dst_u,
                               uint8_t* dst_v,
                               int width,
                               const struct ArgbConstants* c) {
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV[0], sizeof(add_uv));
  asm volatile(
      "xvldrepl.b       $xr0, %[c], 32                 \n"
      "xvldrepl.b       $xr1, %[c], 33                 \n"
      "xvldrepl.b       $xr2, %[c], 34                 \n"
      "xvldrepl.b       $xr3, %[c], 35                 \n"
      "xvldrepl.b       $xr10, %[c], 64                \n"
      "xvldrepl.b       $xr11, %[c], 65                \n"
      "xvldrepl.b       $xr12, %[c], 66                \n"
      "xvldrepl.b       $xr13, %[c], 67                \n"
      "xvreplgr2vr.h    $xr4, %[add_uv]                \n"

      "1:                                              \n"
      "vld              $vr5, %[src], 0                \n"
      "vld              $vr31, %[src], 32              \n"
      "xvpermi.q        $xr5, $xr31, 0x02              \n"
      "vld              $vr6, %[src], 16               \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr6, $xr31, 0x02              \n"
      "xvpickev.b       $xr7, $xr6, $xr5               \n"
      "xvpickod.b       $xr8, $xr6, $xr5               \n"
      "xvmulwev.h.bu.b  $xr9, $xr7, $xr0               \n"
      "xvmaddwev.h.bu.b $xr9, $xr8, $xr1               \n"
      "xvmaddwod.h.bu.b $xr9, $xr7, $xr2               \n"
      "xvmaddwod.h.bu.b $xr9, $xr8, $xr3               \n"
      "xvsub.h          $xr9, $xr4, $xr9               \n"
      "xvmulwev.h.bu.b  $xr14, $xr7, $xr10             \n"
      "xvmaddwev.h.bu.b $xr14, $xr8, $xr11             \n"
      "xvmaddwod.h.bu.b $xr14, $xr7, $xr12             \n"
      "xvmaddwod.h.bu.b $xr14, $xr8, $xr13             \n"
      "xvsub.h          $xr14, $xr4, $xr14             \n"
      "xvpickod.b       $xr9, $xr14, $xr9              \n"
      "xvpermi.d        $xr31, $xr9, 0xd8              \n"
      "vst              $vr31, %[du], 0                \n"
      "vstelm.d         $vr9, %[dv], 0, 1              \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vstelm.d         $vr31, %[dv], 8, 1             \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_uv] "r"(add_uv)   // %[add_uv]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14",
        "$xr31");
}
#endif  // HAS_ARGBTOUV444MATRIXROW_LASX

#ifdef HAS_ARGBTOUVMATRIXROW_LASX
void ARGBToUVMatrixRow_LASX(const uint8_t* src_argb,
                            int src_stride_argb,
                            uint8_t* dst_u,
                            uint8_t* dst_v,
                            int width,
                            const struct ArgbConstants* c) {
  const uint8_t* next_argb = src_argb + src_stride_argb;
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV[0], sizeof(add_uv));
  asm volatile(
      "xvldrepl.b       $xr0, %[c], 32                 \n"
      "xvldrepl.b       $xr1, %[c], 33                 \n"
      "xvldrepl.b       $xr2, %[c], 34                 \n"
      "xvldrepl.b       $xr3, %[c], 35                 \n"
      "xvldrepl.b       $xr10, %[c], 64                \n"
      "xvldrepl.b       $xr11, %[c], 65                \n"
      "xvldrepl.b       $xr12, %[c], 66                \n"
      "xvldrepl.b       $xr13, %[c], 67                \n"
      "xvreplgr2vr.h    $xr4, %[add_uv]                \n"

      "1:                                              \n"
      "xvld             $xr14, %[s0], 0                \n"
      "xvld             $xr15, %[s0], 32               \n"
      "xvld             $xr16, %[s1], 0                \n"
      "xvld             $xr17, %[s1], 32               \n"
      "xvpickev.w       $xr18, $xr15, $xr14            \n"
      "xvpickod.w       $xr19, $xr15, $xr14            \n"
      "xvpickev.w       $xr20, $xr17, $xr16            \n"
      "xvpickod.w       $xr21, $xr17, $xr16            \n"
      "xvaddwev.h.bu    $xr14, $xr18, $xr19            \n"
      "xvaddwod.h.bu    $xr15, $xr18, $xr19            \n"
      "xvaddwev.h.bu    $xr16, $xr20, $xr21            \n"
      "xvaddwod.h.bu    $xr17, $xr20, $xr21            \n"
      "xvadd.h          $xr14, $xr14, $xr16            \n"
      "xvadd.h          $xr15, $xr15, $xr17            \n"
      "xvsrari.h        $xr14, $xr14, 2                \n"
      "xvsrari.h        $xr15, $xr15, 2                \n"
      "xvpackev.b       $xr5, $xr15, $xr14             \n"
      "xvpermi.d        $xr5, $xr5, 0xd8               \n"
      "xvld             $xr14, %[s0], 64               \n"
      "xvld             $xr15, %[s0], 96               \n"
      "xvld             $xr16, %[s1], 64               \n"
      "xvld             $xr17, %[s1], 96               \n"
      "xvpickev.w       $xr18, $xr15, $xr14            \n"
      "xvpickod.w       $xr19, $xr15, $xr14            \n"
      "xvpickev.w       $xr20, $xr17, $xr16            \n"
      "xvpickod.w       $xr21, $xr17, $xr16            \n"
      "xvaddwev.h.bu    $xr14, $xr18, $xr19            \n"
      "xvaddwod.h.bu    $xr15, $xr18, $xr19            \n"
      "xvaddwev.h.bu    $xr16, $xr20, $xr21            \n"
      "xvaddwod.h.bu    $xr17, $xr20, $xr21            \n"
      "xvadd.h          $xr14, $xr14, $xr16            \n"
      "xvadd.h          $xr15, $xr15, $xr17            \n"
      "xvsrari.h        $xr14, $xr14, 2                \n"
      "xvsrari.h        $xr15, $xr15, 2                \n"
      "xvpackev.b       $xr6, $xr15, $xr14             \n"
      "xvpermi.d        $xr6, $xr6, 0xd8               \n"
      "xvor.v           $xr7, $xr5, $xr5               \n"
      "xvpermi.q        $xr5, $xr6, 0x02               \n"
      "xvpermi.q        $xr6, $xr7, 0x31               \n"
      "xvpickev.b       $xr7, $xr6, $xr5               \n"
      "xvpickod.b       $xr8, $xr6, $xr5               \n"
      "xvmulwev.h.bu.b  $xr9, $xr7, $xr0               \n"
      "xvmaddwev.h.bu.b $xr9, $xr8, $xr1               \n"
      "xvmaddwod.h.bu.b $xr9, $xr7, $xr2               \n"
      "xvmaddwod.h.bu.b $xr9, $xr8, $xr3               \n"
      "xvsub.h          $xr9, $xr4, $xr9               \n"
      "xvmulwev.h.bu.b  $xr14, $xr7, $xr10             \n"
      "xvmaddwev.h.bu.b $xr14, $xr8, $xr11             \n"
      "xvmaddwod.h.bu.b $xr14, $xr7, $xr12             \n"
      "xvmaddwod.h.bu.b $xr14, $xr8, $xr13             \n"
      "xvsub.h          $xr14, $xr4, $xr14             \n"
      "xvpickod.b       $xr9, $xr14, $xr9              \n"
      "xvpermi.d        $xr31, $xr9, 0xd8              \n"
      "vst              $vr31, %[du], 0                \n"
      "vstelm.d         $vr9, %[dv], 0, 1              \n"
      "xvpermi.q        $xr31, $xr9, 0x01              \n"
      "vstelm.d         $vr31, %[dv], 8, 1             \n"
      "addi.d           %[s0], %[s0], 128              \n"
      "addi.d           %[s1], %[s1], 128              \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb),   // %[s0]
        [s1] "+r"(next_argb),  // %[s1]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_uv] "r"(add_uv)   // %[add_uv]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14",
        "$xr15", "$xr16", "$xr17", "$xr18", "$xr19", "$xr20", "$xr21", "$xr31");
}
#endif  // HAS_ARGBTOUVMATRIXROW_LASX

#ifdef HAS_RGBTOYMATRIXROW_LASX
void RGBToYMatrixRow_LASX(const uint8_t* src_rgba,
                          uint8_t* dst_y,
                          int width,
                          const struct ArgbConstants* c) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                     0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                     9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  uint16_t add_y;
  __builtin_memcpy(&add_y, &c->kAddY[0], sizeof(add_y));
  asm volatile(
      "xvldrepl.b       $xr0, %[c], 0                  \n"
      "xvldrepl.b       $xr1, %[c], 1                  \n"
      "xvldrepl.b       $xr2, %[c], 2                  \n"
      "xvreplgr2vr.h    $xr3, %[add_y]                 \n"
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"
      "vld              $vr5, %[shuf], 16              \n"
      "xvpermi.q        $xr5, $xr5, 0x00               \n"

      "1:                                              \n"
      "vld              $vr6, %[src], 0                \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr6, $xr31, 0x02              \n"
      "vld              $vr7, %[src], 12               \n"
      "vld              $vr31, %[src], 60              \n"
      "xvpermi.q        $xr7, $xr31, 0x02              \n"
      "vld              $vr8, %[src], 24               \n"
      "vld              $vr31, %[src], 72              \n"
      "xvpermi.q        $xr8, $xr31, 0x02              \n"
      "vld              $vr9, %[src], 32               \n"
      "vld              $vr31, %[src], 80              \n"
      "xvpermi.q        $xr9, $xr31, 0x02              \n"
      "xvshuf.b         $xr6, $xr6, $xr6, $xr4         \n"
      "xvshuf.b         $xr7, $xr7, $xr7, $xr4         \n"
      "xvshuf.b         $xr8, $xr8, $xr8, $xr4         \n"
      "xvshuf.b         $xr9, $xr9, $xr9, $xr5         \n"
      "xvor.v           $xr12, $xr3, $xr3              \n"
      "xvor.v           $xr13, $xr3, $xr3              \n"
      "xvpickev.b       $xr10, $xr7, $xr6              \n"
      "xvpickod.b       $xr11, $xr7, $xr6              \n"
      "xvmaddwev.h.bu   $xr12, $xr10, $xr0             \n"
      "xvmaddwev.h.bu   $xr12, $xr11, $xr1             \n"
      "xvmaddwod.h.bu   $xr12, $xr10, $xr2             \n"
      "xvpickev.b       $xr10, $xr9, $xr8              \n"
      "xvpickod.b       $xr11, $xr9, $xr8              \n"
      "xvmaddwev.h.bu   $xr13, $xr10, $xr0             \n"
      "xvmaddwev.h.bu   $xr13, $xr11, $xr1             \n"
      "xvmaddwod.h.bu   $xr13, $xr10, $xr2             \n"
      "xvpickod.b       $xr12, $xr13, $xr12            \n"
      "xvst             $xr12, %[dst], 0               \n"
      "addi.d           %[src], %[src], 96             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgba),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_y] "r"(add_y),    // %[add_y]
        [shuf] "r"(kShuf)      // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr31");
}
#endif  // HAS_RGBTOYMATRIXROW_LASX

#ifdef HAS_RGBTOUV444MATRIXROW_LASX
void RGBToUV444MatrixRow_LASX(const uint8_t* src_rgb,
                              uint8_t* dst_u,
                              uint8_t* dst_v,
                              int width,
                              const struct ArgbConstants* c) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                     0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                     9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV[0], sizeof(add_uv));
  asm volatile(
      "xvldrepl.b       $xr0, %[c], 32                 \n"
      "xvldrepl.b       $xr1, %[c], 33                 \n"
      "xvldrepl.b       $xr2, %[c], 34                 \n"
      "xvldrepl.b       $xr14, %[c], 64                \n"
      "xvldrepl.b       $xr15, %[c], 65                \n"
      "xvldrepl.b       $xr16, %[c], 66                \n"
      "xvreplgr2vr.h    $xr3, %[add_uv]                \n"
      "vld              $vr4, %[shuf], 0               \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"
      "vld              $vr5, %[shuf], 16              \n"
      "xvpermi.q        $xr5, $xr5, 0x00               \n"

      "1:                                              \n"
      "vld              $vr6, %[src], 0                \n"
      "vld              $vr31, %[src], 48              \n"
      "xvpermi.q        $xr6, $xr31, 0x02              \n"
      "vld              $vr7, %[src], 12               \n"
      "vld              $vr31, %[src], 60              \n"
      "xvpermi.q        $xr7, $xr31, 0x02              \n"
      "vld              $vr8, %[src], 24               \n"
      "vld              $vr31, %[src], 72              \n"
      "xvpermi.q        $xr8, $xr31, 0x02              \n"
      "vld              $vr9, %[src], 32               \n"
      "vld              $vr31, %[src], 80              \n"
      "xvpermi.q        $xr9, $xr31, 0x02              \n"
      "xvshuf.b         $xr6, $xr6, $xr6, $xr4         \n"
      "xvshuf.b         $xr7, $xr7, $xr7, $xr4         \n"
      "xvshuf.b         $xr8, $xr8, $xr8, $xr4         \n"
      "xvshuf.b         $xr9, $xr9, $xr9, $xr5         \n"
      "xvpickev.b       $xr10, $xr7, $xr6              \n"
      "xvpickod.b       $xr11, $xr7, $xr6              \n"
      "xvmulwev.h.bu.b  $xr12, $xr10, $xr0             \n"
      "xvmaddwev.h.bu.b $xr12, $xr11, $xr1             \n"
      "xvmaddwod.h.bu.b $xr12, $xr10, $xr2             \n"
      "xvsub.h          $xr12, $xr3, $xr12             \n"
      "xvmulwev.h.bu.b  $xr17, $xr10, $xr14            \n"
      "xvmaddwev.h.bu.b $xr17, $xr11, $xr15            \n"
      "xvmaddwod.h.bu.b $xr17, $xr10, $xr16            \n"
      "xvsub.h          $xr17, $xr3, $xr17             \n"
      "xvpickev.b       $xr10, $xr9, $xr8              \n"
      "xvpickod.b       $xr11, $xr9, $xr8              \n"
      "xvmulwev.h.bu.b  $xr13, $xr10, $xr0             \n"
      "xvmaddwev.h.bu.b $xr13, $xr11, $xr1             \n"
      "xvmaddwod.h.bu.b $xr13, $xr10, $xr2             \n"
      "xvsub.h          $xr13, $xr3, $xr13             \n"
      "xvmulwev.h.bu.b  $xr18, $xr10, $xr14            \n"
      "xvmaddwev.h.bu.b $xr18, $xr11, $xr15            \n"
      "xvmaddwod.h.bu.b $xr18, $xr10, $xr16            \n"
      "xvsub.h          $xr18, $xr3, $xr18             \n"
      "xvpickod.b       $xr12, $xr13, $xr12            \n"
      "xvpickod.b       $xr17, $xr18, $xr17            \n"
      "xvst             $xr12, %[du], 0                \n"
      "xvst             $xr17, %[dv], 0                \n"
      "addi.d           %[src], %[src], 96             \n"
      "addi.d           %[du], %[du], 32               \n"
      "addi.d           %[dv], %[dv], 32               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb),   // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_uv] "r"(add_uv),  // %[add_uv]
        [shuf] "r"(kShuf)      // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14",
        "$xr15", "$xr16", "$xr17", "$xr18", "$xr31");
}
#endif  // HAS_RGBTOUV444MATRIXROW_LASX

#ifdef HAS_RGBTOUVMATRIXROW_LASX
void RGBToUVMatrixRow_LASX(const uint8_t* src_rgb,
                           int src_stride_rgb,
                           uint8_t* dst_u,
                           uint8_t* dst_v,
                           int width,
                           const struct ArgbConstants* c) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                     0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                     9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  const uint8_t* next_rgb = src_rgb + src_stride_rgb;
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV[0], sizeof(add_uv));
  asm volatile(
      "xvldrepl.b       $xr20, %[c], 32                \n"
      "xvsrai.h         $xr20, $xr20, 8                \n"
      "xvldrepl.b       $xr21, %[c], 33                \n"
      "xvsrai.h         $xr21, $xr21, 8                \n"
      "xvldrepl.b       $xr22, %[c], 34                \n"
      "xvsrai.h         $xr22, $xr22, 8                \n"
      "xvldrepl.b       $xr23, %[c], 64                \n"
      "xvsrai.h         $xr23, $xr23, 8                \n"
      "xvldrepl.b       $xr24, %[c], 65                \n"
      "xvsrai.h         $xr24, $xr24, 8                \n"
      "xvldrepl.b       $xr25, %[c], 66                \n"
      "xvsrai.h         $xr25, $xr25, 8                \n"
      "xvreplgr2vr.h    $xr26, %[add_uv]               \n"
      "vld              $vr16, %[shuf], 0              \n"
      "xvpermi.q        $xr16, $xr16, 0x00             \n"
      "vld              $vr17, %[shuf], 16             \n"
      "xvpermi.q        $xr17, $xr17, 0x00             \n"

      "1:                                              \n"
      "vld              $vr8, %[s0], 0                 \n"
      "vld              $vr31, %[s0], 48               \n"
      "xvpermi.q        $xr8, $xr31, 0x02              \n"
      "vld              $vr9, %[s0], 12                \n"
      "vld              $vr31, %[s0], 60               \n"
      "xvpermi.q        $xr9, $xr31, 0x02              \n"
      "vld              $vr10, %[s0], 24               \n"
      "vld              $vr31, %[s0], 72               \n"
      "xvpermi.q        $xr10, $xr31, 0x02             \n"
      "vld              $vr11, %[s0], 32               \n"
      "vld              $vr31, %[s0], 80               \n"
      "xvpermi.q        $xr11, $xr31, 0x02             \n"
      "xvshuf.b         $xr8, $xr8, $xr8, $xr16        \n"
      "xvshuf.b         $xr9, $xr9, $xr9, $xr16        \n"
      "xvshuf.b         $xr10, $xr10, $xr10, $xr16     \n"
      "xvshuf.b         $xr11, $xr11, $xr11, $xr17     \n"
      "xvpickev.b       $xr12, $xr9, $xr8              \n"
      "xvpickod.b       $xr13, $xr9, $xr8              \n"
      "xvpickev.b       $xr14, $xr11, $xr10            \n"
      "xvpickod.b       $xr15, $xr11, $xr10            \n"
      "xvpickev.b       $xr0, $xr14, $xr12             \n"
      "xvpickod.b       $xr2, $xr14, $xr12             \n"
      "xvpickev.b       $xr1, $xr15, $xr13             \n"
      "vld              $vr8, %[s1], 0                 \n"
      "vld              $vr31, %[s1], 48               \n"
      "xvpermi.q        $xr8, $xr31, 0x02              \n"
      "vld              $vr9, %[s1], 12                \n"
      "vld              $vr31, %[s1], 60               \n"
      "xvpermi.q        $xr9, $xr31, 0x02              \n"
      "vld              $vr10, %[s1], 24               \n"
      "vld              $vr31, %[s1], 72               \n"
      "xvpermi.q        $xr10, $xr31, 0x02             \n"
      "vld              $vr11, %[s1], 32               \n"
      "vld              $vr31, %[s1], 80               \n"
      "xvpermi.q        $xr11, $xr31, 0x02             \n"
      "xvshuf.b         $xr8, $xr8, $xr8, $xr16        \n"
      "xvshuf.b         $xr9, $xr9, $xr9, $xr16        \n"
      "xvshuf.b         $xr10, $xr10, $xr10, $xr16     \n"
      "xvshuf.b         $xr11, $xr11, $xr11, $xr17     \n"
      "xvpickev.b       $xr12, $xr9, $xr8              \n"
      "xvpickod.b       $xr13, $xr9, $xr8              \n"
      "xvpickev.b       $xr14, $xr11, $xr10            \n"
      "xvpickod.b       $xr15, $xr11, $xr10            \n"
      "xvpickev.b       $xr3, $xr14, $xr12             \n"
      "xvpickod.b       $xr5, $xr14, $xr12             \n"
      "xvpickev.b       $xr4, $xr15, $xr13             \n"
      "xvhaddw.hu.bu    $xr0, $xr0, $xr0               \n"
      "xvhaddw.hu.bu    $xr3, $xr3, $xr3               \n"
      "xvadd.h          $xr0, $xr0, $xr3               \n"
      "xvsrari.h        $xr0, $xr0, 2                  \n"
      "xvhaddw.hu.bu    $xr1, $xr1, $xr1               \n"
      "xvhaddw.hu.bu    $xr4, $xr4, $xr4               \n"
      "xvadd.h          $xr1, $xr1, $xr4               \n"
      "xvsrari.h        $xr1, $xr1, 2                  \n"
      "xvhaddw.hu.bu    $xr2, $xr2, $xr2               \n"
      "xvhaddw.hu.bu    $xr5, $xr5, $xr5               \n"
      "xvadd.h          $xr2, $xr2, $xr5               \n"
      "xvsrari.h        $xr2, $xr2, 2                  \n"
      "xvor.v           $xr6, $xr26, $xr26             \n"
      "xvor.v           $xr7, $xr26, $xr26             \n"
      "xvmsub.h         $xr6, $xr0, $xr20              \n"
      "xvmsub.h         $xr6, $xr1, $xr21              \n"
      "xvmsub.h         $xr6, $xr2, $xr22              \n"
      "xvmsub.h         $xr7, $xr0, $xr23              \n"
      "xvmsub.h         $xr7, $xr1, $xr24              \n"
      "xvmsub.h         $xr7, $xr2, $xr25              \n"
      "xvpickod.b       $xr6, $xr7, $xr6               \n"
      "xvpermi.d        $xr31, $xr6, 0xd8              \n"
      "vst              $vr31, %[du], 0                \n"
      "vstelm.d         $vr6, %[dv], 0, 1              \n"
      "xvpermi.q        $xr31, $xr6, 0x01              \n"
      "vstelm.d         $vr31, %[dv], 8, 1             \n"
      "addi.d           %[s0], %[s0], 96               \n"
      "addi.d           %[s1], %[s1], 96               \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_rgb),    // %[s0]
        [s1] "+r"(next_rgb),   // %[s1]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [cnt] "+r"(width)      // %[cnt]
      : [c] "r"(c),            // %[c]
        [add_uv] "r"(add_uv),  // %[add_uv]
        [shuf] "r"(kShuf)      // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14",
        "$xr15", "$xr16", "$xr17", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24",
        "$xr25", "$xr26", "$xr31");
}
#endif  // HAS_RGBTOUVMATRIXROW_LASX

#ifdef HAS_RGB565TOYROW_LASX
void RGB565ToYRow_LASX(const uint8_t* src_rgb565, uint8_t* dst_y, int width) {
  asm volatile(
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"
      "xvldi            $xr7, 63                       \n"
      "xvsrli.h         $xr7, $xr7, 8                  \n"
      "xvldi            $xr8, 25                       \n"
      "xvsrli.h         $xr8, $xr8, 8                  \n"
      "xvldi            $xr9, 129                      \n"
      "xvsrli.h         $xr9, $xr9, 8                  \n"
      "xvldi            $xr10, 66                      \n"
      "xvsrli.h         $xr10, $xr10, 8                \n"
      "xvldi            $xr11, 33                      \n"
      "xvsrli.h         $xr11, $xr11, 8                \n"
      "xvslli.h         $xr11, $xr11, 7                \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr7               \n"
      "xvsrli.h         $xr3, $xr0, 11                 \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 2                  \n"
      "xvsrli.h         $xr2, $xr2, 4                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvmul.h          $xr1, $xr1, $xr8               \n"
      "xvmadd.h         $xr1, $xr2, $xr9               \n"
      "xvmadd.h         $xr1, $xr3, $xr10              \n"
      "xvadd.h          $xr1, $xr1, $xr11              \n"
      "xvpickod.b       $xr1, $xr1, $xr1               \n"
      "xvpermi.d        $xr31, $xr1, 0xd8              \n"
      "vst              $vr31, %[dst], 0               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb565),  // %[src]
        [dst] "+r"(dst_y),       // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr6", "$xr7",
        "$xr8", "$xr9", "$xr10", "$xr11", "$xr31");
}
#endif  // HAS_RGB565TOYROW_LASX

#ifdef HAS_ARGB1555TOYROW_LASX
void ARGB1555ToYRow_LASX(const uint8_t* src_argb1555,
                         uint8_t* dst_y,
                         int width) {
  asm volatile(
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"
      "xvldi            $xr8, 25                       \n"
      "xvsrli.h         $xr8, $xr8, 8                  \n"
      "xvldi            $xr9, 129                      \n"
      "xvsrli.h         $xr9, $xr9, 8                  \n"
      "xvldi            $xr10, 66                      \n"
      "xvsrli.h         $xr10, $xr10, 8                \n"
      "xvldi            $xr11, 33                      \n"
      "xvsrli.h         $xr11, $xr11, 8                \n"
      "xvslli.h         $xr11, $xr11, 7                \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvsrli.h         $xr3, $xr0, 10                 \n"
      "xvand.v          $xr3, $xr3, $xr6               \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 3                  \n"
      "xvsrli.h         $xr2, $xr2, 2                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvmul.h          $xr1, $xr1, $xr8               \n"
      "xvmadd.h         $xr1, $xr2, $xr9               \n"
      "xvmadd.h         $xr1, $xr3, $xr10              \n"
      "xvadd.h          $xr1, $xr1, $xr11              \n"
      "xvpickod.b       $xr1, $xr1, $xr1               \n"
      "xvpermi.d        $xr31, $xr1, 0xd8              \n"
      "vst              $vr31, %[dst], 0               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb1555),  // %[src]
        [dst] "+r"(dst_y),         // %[dst]
        [cnt] "+r"(width)          // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr6", "$xr8",
        "$xr9", "$xr10", "$xr11", "$xr31");
}
#endif  // HAS_ARGB1555TOYROW_LASX

#ifdef HAS_RGB565TOUVROW_LASX
void RGB565ToUVRow_LASX(const uint8_t* src_rgb565,
                        int src_stride_rgb565,
                        uint8_t* dst_u,
                        uint8_t* dst_v,
                        int width) {
  const uint8_t* next_rgb565 = src_rgb565 + src_stride_rgb565;
  asm volatile(
      "xvldi            $xr20, 112                     \n"
      "xvsrli.h         $xr20, $xr20, 8                \n"
      "xvldi            $xr21, 74                      \n"
      "xvsrli.h         $xr21, $xr21, 8                \n"
      "xvldi            $xr22, 38                      \n"
      "xvsrli.h         $xr22, $xr22, 8                \n"
      "xvldi            $xr23, 94                      \n"
      "xvsrli.h         $xr23, $xr23, 8                \n"
      "xvldi            $xr24, 18                      \n"
      "xvsrli.h         $xr24, $xr24, 8                \n"
      "xvldi            $xr25, 128                     \n"
      "xvsrli.h         $xr25, $xr25, 8                \n"
      "xvslli.h         $xr25, $xr25, 8                \n"
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"
      "xvldi            $xr7, 63                       \n"
      "xvsrli.h         $xr7, $xr7, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr31, %[s0], 32               \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr7               \n"
      "xvsrli.h         $xr3, $xr0, 11                 \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 2                  \n"
      "xvsrli.h         $xr2, $xr2, 4                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvor.v           $xr10, $xr1, $xr1              \n"
      "xvor.v           $xr11, $xr2, $xr2              \n"
      "xvor.v           $xr12, $xr3, $xr3              \n"
      "vld              $vr0, %[s0], 16                \n"
      "vld              $vr31, %[s0], 48               \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr7               \n"
      "xvsrli.h         $xr3, $xr0, 11                 \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 2                  \n"
      "xvsrli.h         $xr2, $xr2, 4                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvpickev.b       $xr10, $xr1, $xr10             \n"
      "xvpickev.b       $xr11, $xr2, $xr11             \n"
      "xvpickev.b       $xr12, $xr3, $xr12             \n"
      "vld              $vr0, %[s1], 0                 \n"
      "vld              $vr31, %[s1], 32               \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr7               \n"
      "xvsrli.h         $xr3, $xr0, 11                 \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 2                  \n"
      "xvsrli.h         $xr2, $xr2, 4                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvor.v           $xr13, $xr1, $xr1              \n"
      "xvor.v           $xr14, $xr2, $xr2              \n"
      "xvor.v           $xr15, $xr3, $xr3              \n"
      "vld              $vr0, %[s1], 16                \n"
      "vld              $vr31, %[s1], 48               \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr7               \n"
      "xvsrli.h         $xr3, $xr0, 11                 \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 2                  \n"
      "xvsrli.h         $xr2, $xr2, 4                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvpickev.b       $xr5, $xr3, $xr15              \n"
      "xvpickev.b       $xr4, $xr2, $xr14              \n"
      "xvpickev.b       $xr3, $xr1, $xr13              \n"
      "xvor.v           $xr0, $xr10, $xr10             \n"
      "xvor.v           $xr1, $xr11, $xr11             \n"
      "xvor.v           $xr2, $xr12, $xr12             \n"
      "xvhaddw.hu.bu    $xr0, $xr0, $xr0               \n"
      "xvhaddw.hu.bu    $xr3, $xr3, $xr3               \n"
      "xvadd.h          $xr0, $xr0, $xr3               \n"
      "xvsrari.h        $xr0, $xr0, 2                  \n"
      "xvhaddw.hu.bu    $xr1, $xr1, $xr1               \n"
      "xvhaddw.hu.bu    $xr4, $xr4, $xr4               \n"
      "xvadd.h          $xr1, $xr1, $xr4               \n"
      "xvsrari.h        $xr1, $xr1, 2                  \n"
      "xvhaddw.hu.bu    $xr2, $xr2, $xr2               \n"
      "xvhaddw.hu.bu    $xr5, $xr5, $xr5               \n"
      "xvadd.h          $xr2, $xr2, $xr5               \n"
      "xvsrari.h        $xr2, $xr2, 2                  \n"
      "xvor.v           $xr8, $xr25, $xr25             \n"
      "xvor.v           $xr9, $xr25, $xr25             \n"
      "xvmadd.h         $xr8, $xr0, $xr20              \n"
      "xvmsub.h         $xr8, $xr1, $xr21              \n"
      "xvmsub.h         $xr8, $xr2, $xr22              \n"
      "xvmadd.h         $xr9, $xr2, $xr20              \n"
      "xvmsub.h         $xr9, $xr1, $xr23              \n"
      "xvmsub.h         $xr9, $xr0, $xr24              \n"
      "xvpickod.b       $xr8, $xr9, $xr8               \n"
      "xvpermi.d        $xr31, $xr8, 0xd8              \n"
      "vst              $vr31, %[du], 0                \n"
      "vstelm.d         $vr8, %[dv], 0, 1              \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vstelm.d         $vr31, %[dv], 8, 1             \n"
      "addi.d           %[s0], %[s0], 64               \n"
      "addi.d           %[s1], %[s1], 64               \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_rgb565),   // %[s0]
        [s1] "+r"(next_rgb565),  // %[s1]
        [du] "+r"(dst_u),        // %[du]
        [dv] "+r"(dst_v),        // %[dv]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14",
        "$xr15", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24", "$xr25", "$xr31");
}
#endif  // HAS_RGB565TOUVROW_LASX

#ifdef HAS_ARGB1555TOUVROW_LASX
void ARGB1555ToUVRow_LASX(const uint8_t* src_argb1555,
                          int src_stride_argb1555,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width) {
  const uint8_t* next_argb1555 = src_argb1555 + src_stride_argb1555;
  asm volatile(
      "xvldi            $xr20, 112                     \n"
      "xvsrli.h         $xr20, $xr20, 8                \n"
      "xvldi            $xr21, 74                      \n"
      "xvsrli.h         $xr21, $xr21, 8                \n"
      "xvldi            $xr22, 38                      \n"
      "xvsrli.h         $xr22, $xr22, 8                \n"
      "xvldi            $xr23, 94                      \n"
      "xvsrli.h         $xr23, $xr23, 8                \n"
      "xvldi            $xr24, 18                      \n"
      "xvsrli.h         $xr24, $xr24, 8                \n"
      "xvldi            $xr25, 128                     \n"
      "xvsrli.h         $xr25, $xr25, 8                \n"
      "xvslli.h         $xr25, $xr25, 8                \n"
      "xvldi            $xr6, 31                       \n"
      "xvsrli.h         $xr6, $xr6, 8                  \n"

      "1:                                              \n"
      "vld              $vr0, %[s0], 0                 \n"
      "vld              $vr31, %[s0], 32               \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvsrli.h         $xr3, $xr0, 10                 \n"
      "xvand.v          $xr3, $xr3, $xr6               \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 3                  \n"
      "xvsrli.h         $xr2, $xr2, 2                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvor.v           $xr10, $xr1, $xr1              \n"
      "xvor.v           $xr11, $xr2, $xr2              \n"
      "xvor.v           $xr12, $xr3, $xr3              \n"
      "vld              $vr0, %[s0], 16                \n"
      "vld              $vr31, %[s0], 48               \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvsrli.h         $xr3, $xr0, 10                 \n"
      "xvand.v          $xr3, $xr3, $xr6               \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 3                  \n"
      "xvsrli.h         $xr2, $xr2, 2                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvpickev.b       $xr10, $xr1, $xr10             \n"
      "xvpickev.b       $xr11, $xr2, $xr11             \n"
      "xvpickev.b       $xr12, $xr3, $xr12             \n"
      "vld              $vr0, %[s1], 0                 \n"
      "vld              $vr31, %[s1], 32               \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvsrli.h         $xr3, $xr0, 10                 \n"
      "xvand.v          $xr3, $xr3, $xr6               \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 3                  \n"
      "xvsrli.h         $xr2, $xr2, 2                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvor.v           $xr13, $xr1, $xr1              \n"
      "xvor.v           $xr14, $xr2, $xr2              \n"
      "xvor.v           $xr15, $xr3, $xr3              \n"
      "vld              $vr0, %[s1], 16                \n"
      "vld              $vr31, %[s1], 48               \n"
      "xvpermi.q        $xr0, $xr31, 0x02              \n"
      "xvand.v          $xr1, $xr0, $xr6               \n"
      "xvsrli.h         $xr2, $xr0, 5                  \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvsrli.h         $xr3, $xr0, 10                 \n"
      "xvand.v          $xr3, $xr3, $xr6               \n"
      "xvslli.h         $xr4, $xr1, 3                  \n"
      "xvsrli.h         $xr1, $xr1, 2                  \n"
      "xvor.v           $xr1, $xr4, $xr1               \n"
      "xvslli.h         $xr4, $xr2, 3                  \n"
      "xvsrli.h         $xr2, $xr2, 2                  \n"
      "xvor.v           $xr2, $xr4, $xr2               \n"
      "xvslli.h         $xr4, $xr3, 3                  \n"
      "xvsrli.h         $xr3, $xr3, 2                  \n"
      "xvor.v           $xr3, $xr4, $xr3               \n"
      "xvpickev.b       $xr5, $xr3, $xr15              \n"
      "xvpickev.b       $xr4, $xr2, $xr14              \n"
      "xvpickev.b       $xr3, $xr1, $xr13              \n"
      "xvor.v           $xr0, $xr10, $xr10             \n"
      "xvor.v           $xr1, $xr11, $xr11             \n"
      "xvor.v           $xr2, $xr12, $xr12             \n"
      "xvhaddw.hu.bu    $xr0, $xr0, $xr0               \n"
      "xvhaddw.hu.bu    $xr3, $xr3, $xr3               \n"
      "xvadd.h          $xr0, $xr0, $xr3               \n"
      "xvsrari.h        $xr0, $xr0, 2                  \n"
      "xvhaddw.hu.bu    $xr1, $xr1, $xr1               \n"
      "xvhaddw.hu.bu    $xr4, $xr4, $xr4               \n"
      "xvadd.h          $xr1, $xr1, $xr4               \n"
      "xvsrari.h        $xr1, $xr1, 2                  \n"
      "xvhaddw.hu.bu    $xr2, $xr2, $xr2               \n"
      "xvhaddw.hu.bu    $xr5, $xr5, $xr5               \n"
      "xvadd.h          $xr2, $xr2, $xr5               \n"
      "xvsrari.h        $xr2, $xr2, 2                  \n"
      "xvor.v           $xr8, $xr25, $xr25             \n"
      "xvor.v           $xr9, $xr25, $xr25             \n"
      "xvmadd.h         $xr8, $xr0, $xr20              \n"
      "xvmsub.h         $xr8, $xr1, $xr21              \n"
      "xvmsub.h         $xr8, $xr2, $xr22              \n"
      "xvmadd.h         $xr9, $xr2, $xr20              \n"
      "xvmsub.h         $xr9, $xr1, $xr23              \n"
      "xvmsub.h         $xr9, $xr0, $xr24              \n"
      "xvpickod.b       $xr8, $xr9, $xr8               \n"
      "xvpermi.d        $xr31, $xr8, 0xd8              \n"
      "vst              $vr31, %[du], 0                \n"
      "vstelm.d         $vr8, %[dv], 0, 1              \n"
      "xvpermi.q        $xr31, $xr8, 0x01              \n"
      "vstelm.d         $vr31, %[dv], 8, 1             \n"
      "addi.d           %[s0], %[s0], 64               \n"
      "addi.d           %[s1], %[s1], 64               \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src_argb1555),   // %[s0]
        [s1] "+r"(next_argb1555),  // %[s1]
        [du] "+r"(dst_u),          // %[du]
        [dv] "+r"(dst_v),          // %[dv]
        [cnt] "+r"(width)          // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14", "$xr15",
        "$xr20", "$xr21", "$xr22", "$xr23", "$xr24", "$xr25", "$xr31");
}
#endif  // HAS_ARGB1555TOUVROW_LASX

#if defined(HAS_RGB24TOUVROW_LASX) || defined(HAS_RAWTOUVROW_LASX)
static LIBYUV_NOINLINE void RGB24ToUVHelper_LASX(const uint8_t* src,
                                                 int stride,
                                                 uint8_t* dst_u,
                                                 uint8_t* dst_v,
                                                 const uint8_t* shuf,
                                                 int width) {
  const uint8_t* next = src + stride;
  asm volatile(
      "xvldi            $xr20, 112                     \n"
      "xvsrli.h         $xr20, $xr20, 8                \n"
      "xvldi            $xr21, 74                      \n"
      "xvsrli.h         $xr21, $xr21, 8                \n"
      "xvldi            $xr22, 38                      \n"
      "xvsrli.h         $xr22, $xr22, 8                \n"
      "xvldi            $xr23, 94                      \n"
      "xvsrli.h         $xr23, $xr23, 8                \n"
      "xvldi            $xr24, 18                      \n"
      "xvsrli.h         $xr24, $xr24, 8                \n"
      "xvldi            $xr25, 128                     \n"
      "xvsrli.h         $xr25, $xr25, 8                \n"
      "xvslli.h         $xr25, $xr25, 8                \n"
      "vld              $vr16, %[shuf], 0              \n"
      "xvpermi.q        $xr16, $xr16, 0x00             \n"
      "vld              $vr17, %[shuf], 16             \n"
      "xvpermi.q        $xr17, $xr17, 0x00             \n"

      "1:                                              \n"
      "vld              $vr8, %[s0], 0                 \n"
      "vld              $vr31, %[s0], 48               \n"
      "xvpermi.q        $xr8, $xr31, 0x02              \n"
      "vld              $vr9, %[s0], 12                \n"
      "vld              $vr31, %[s0], 60               \n"
      "xvpermi.q        $xr9, $xr31, 0x02              \n"
      "vld              $vr10, %[s0], 24               \n"
      "vld              $vr31, %[s0], 72               \n"
      "xvpermi.q        $xr10, $xr31, 0x02             \n"
      "vld              $vr11, %[s0], 32               \n"
      "vld              $vr31, %[s0], 80               \n"
      "xvpermi.q        $xr11, $xr31, 0x02             \n"
      "xvshuf.b         $xr8, $xr8, $xr8, $xr16        \n"
      "xvshuf.b         $xr9, $xr9, $xr9, $xr16        \n"
      "xvshuf.b         $xr10, $xr10, $xr10, $xr16     \n"
      "xvshuf.b         $xr11, $xr11, $xr11, $xr17     \n"
      "xvpickev.b       $xr12, $xr9, $xr8              \n"
      "xvpickod.b       $xr13, $xr9, $xr8              \n"
      "xvpickev.b       $xr14, $xr11, $xr10            \n"
      "xvpickod.b       $xr15, $xr11, $xr10            \n"
      "xvpickev.b       $xr0, $xr14, $xr12             \n"
      "xvpickod.b       $xr2, $xr14, $xr12             \n"
      "xvpickev.b       $xr1, $xr15, $xr13             \n"
      "vld              $vr8, %[s1], 0                 \n"
      "vld              $vr31, %[s1], 48               \n"
      "xvpermi.q        $xr8, $xr31, 0x02              \n"
      "vld              $vr9, %[s1], 12                \n"
      "vld              $vr31, %[s1], 60               \n"
      "xvpermi.q        $xr9, $xr31, 0x02              \n"
      "vld              $vr10, %[s1], 24               \n"
      "vld              $vr31, %[s1], 72               \n"
      "xvpermi.q        $xr10, $xr31, 0x02             \n"
      "vld              $vr11, %[s1], 32               \n"
      "vld              $vr31, %[s1], 80               \n"
      "xvpermi.q        $xr11, $xr31, 0x02             \n"
      "xvshuf.b         $xr8, $xr8, $xr8, $xr16        \n"
      "xvshuf.b         $xr9, $xr9, $xr9, $xr16        \n"
      "xvshuf.b         $xr10, $xr10, $xr10, $xr16     \n"
      "xvshuf.b         $xr11, $xr11, $xr11, $xr17     \n"
      "xvpickev.b       $xr12, $xr9, $xr8              \n"
      "xvpickod.b       $xr13, $xr9, $xr8              \n"
      "xvpickev.b       $xr14, $xr11, $xr10            \n"
      "xvpickod.b       $xr15, $xr11, $xr10            \n"
      "xvpickev.b       $xr3, $xr14, $xr12             \n"
      "xvpickod.b       $xr5, $xr14, $xr12             \n"
      "xvpickev.b       $xr4, $xr15, $xr13             \n"
      "xvhaddw.hu.bu    $xr0, $xr0, $xr0               \n"
      "xvhaddw.hu.bu    $xr3, $xr3, $xr3               \n"
      "xvadd.h          $xr0, $xr0, $xr3               \n"
      "xvsrari.h        $xr0, $xr0, 2                  \n"
      "xvhaddw.hu.bu    $xr1, $xr1, $xr1               \n"
      "xvhaddw.hu.bu    $xr4, $xr4, $xr4               \n"
      "xvadd.h          $xr1, $xr1, $xr4               \n"
      "xvsrari.h        $xr1, $xr1, 2                  \n"
      "xvhaddw.hu.bu    $xr2, $xr2, $xr2               \n"
      "xvhaddw.hu.bu    $xr5, $xr5, $xr5               \n"
      "xvadd.h          $xr2, $xr2, $xr5               \n"
      "xvsrari.h        $xr2, $xr2, 2                  \n"
      "xvor.v           $xr6, $xr25, $xr25             \n"
      "xvor.v           $xr7, $xr25, $xr25             \n"
      "xvmadd.h         $xr6, $xr0, $xr20              \n"
      "xvmsub.h         $xr6, $xr1, $xr21              \n"
      "xvmsub.h         $xr6, $xr2, $xr22              \n"
      "xvmadd.h         $xr7, $xr2, $xr20              \n"
      "xvmsub.h         $xr7, $xr1, $xr23              \n"
      "xvmsub.h         $xr7, $xr0, $xr24              \n"
      "xvpickod.b       $xr6, $xr7, $xr6               \n"
      "xvpermi.d        $xr31, $xr6, 0xd8              \n"
      "vst              $vr31, %[du], 0                \n"
      "vstelm.d         $vr6, %[dv], 0, 1              \n"
      "xvpermi.q        $xr31, $xr6, 0x01              \n"
      "vstelm.d         $vr31, %[dv], 8, 1             \n"
      "addi.d           %[s0], %[s0], 96               \n"
      "addi.d           %[s1], %[s1], 96               \n"
      "addi.d           %[du], %[du], 16               \n"
      "addi.d           %[dv], %[dv], 16               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src),    // %[s0]
        [s1] "+r"(next),   // %[s1]
        [du] "+r"(dst_u),  // %[du]
        [dv] "+r"(dst_v),  // %[dv]
        [cnt] "+r"(width)  // %[cnt]
      : [shuf] "r"(shuf)   // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14",
        "$xr15", "$xr16", "$xr17", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24",
        "$xr25", "$xr31");
}
#endif  // defined(HAS_RGB24TOUVROW_LASX) || defined(HAS_RAWTOUVROW_LASX)

#ifdef HAS_RGB24TOUVROW_LASX
void RGB24ToUVRow_LASX(const uint8_t* src_rgb24,
                       int src_stride_rgb24,
                       uint8_t* dst_u,
                       uint8_t* dst_v,
                       int width) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                    0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                    9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  RGB24ToUVHelper_LASX(src_rgb24, src_stride_rgb24, dst_u, dst_v, kShuf, width);
}
#endif  // HAS_RGB24TOUVROW_LASX

#ifdef HAS_RAWTOUVROW_LASX
void RAWToUVRow_LASX(const uint8_t* src_raw,
                     int src_stride_raw,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  static const uint8_t kShuf[32] = {2, 1,  0,  0,  5,  4, 3,  0,  8,  7, 6,
                                    0, 11, 10, 9,  0,  6, 5,  4,  0,  9, 8,
                                    7, 0,  12, 11, 10, 0, 15, 14, 13, 0};
  RGB24ToUVHelper_LASX(src_raw, src_stride_raw, dst_u, dst_v, kShuf, width);
}
#endif  // HAS_RAWTOUVROW_LASX

#ifdef HAS_SWAPUVROW_LASX
void SwapUVRow_LASX(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvshuf4i.b       $xr0, $xr0, 0xb1               \n"
      "xvshuf4i.b       $xr1, $xr1, 0xb1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [dst] "+r"(dst_vu),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_SWAPUVROW_LASX

#ifdef HAS_ARGBCOPYYTOALPHAROW_LASX
void ARGBCopyYToAlphaRow_LASX(const uint8_t* src, uint8_t* dst, int width) {
  asm volatile(
      "xvldi            $xr4, 0xff                     \n"
      "xvslli.w         $xr4, $xr4, 24                 \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "xvld             $xr1, %[dst], 0                \n"
      "xvld             $xr2, %[dst], 32               \n"
      "vilvl.b          $vr3, $vr0, $vr0               \n"
      "vilvh.b          $vr0, $vr0, $vr0               \n"
      "xvpermi.d        $xr3, $xr3, 0xd8               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvilvl.h         $xr3, $xr3, $xr3               \n"
      "xvilvl.h         $xr0, $xr0, $xr0               \n"
      "xvslli.w         $xr3, $xr3, 24                 \n"
      "xvslli.w         $xr0, $xr0, 24                 \n"
      "xvandn.v         $xr1, $xr4, $xr1               \n"
      "xvandn.v         $xr2, $xr4, $xr2               \n"
      "xvor.v           $xr1, $xr1, $xr3               \n"
      "xvor.v           $xr2, $xr2, $xr0               \n"
      "xvst             $xr1, %[dst], 0                \n"
      "xvst             $xr2, %[dst], 32               \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),   // %[src]
        [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(width)  // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_ARGBCOPYYTOALPHAROW_LASX

#ifdef HAS_HALFMERGEUVROW_LASX
void HalfMergeUVRow_LASX(const uint8_t* src_u,
                         int src_stride_u,
                         const uint8_t* src_v,
                         int src_stride_v,
                         uint8_t* dst_uv,
                         int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[su], 0                 \n"
      "xvld             $xr1, %[sv], 0                 \n"
      "xvldx            $xr2, %[su], %[stu]            \n"
      "xvldx            $xr3, %[sv], %[stv]            \n"
      "xvhaddw.hu.bu    $xr0, $xr0, $xr0               \n"
      "xvhaddw.hu.bu    $xr1, $xr1, $xr1               \n"
      "xvhaddw.hu.bu    $xr2, $xr2, $xr2               \n"
      "xvhaddw.hu.bu    $xr3, $xr3, $xr3               \n"
      "xvadd.h          $xr0, $xr0, $xr2               \n"
      "xvadd.h          $xr1, $xr1, $xr3               \n"
      "xvsrari.h        $xr0, $xr0, 2                  \n"
      "xvsrari.h        $xr1, $xr1, 2                  \n"
      "xvpackev.b       $xr0, $xr1, $xr0               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[su], %[su], 32               \n"
      "addi.d           %[sv], %[sv], 32               \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [su] "+r"(src_u),                    // %[su]
        [sv] "+r"(src_v),                    // %[sv]
        [dst] "+r"(dst_uv),                  // %[dst]
        [cnt] "+r"(width)                    // %[cnt]
      : [stu] "r"((ptrdiff_t)src_stride_u),  // %[stu]
        [stv] "r"((ptrdiff_t)src_stride_v)   // %[stv]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_HALFMERGEUVROW_LASX

#ifdef HAS_HALFUVROW_LASX
void HalfUVRow_LASX(const uint8_t* src_u,
                    ptrdiff_t src_stride_u,
                    const uint8_t* src_v,
                    ptrdiff_t src_stride_v,
                    uint8_t* dst_uv,
                    int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[su], 0                 \n"
      "xvldx            $xr2, %[su], %[stu]            \n"
      "xvavgr.bu        $xr0, $xr0, $xr2               \n"
      "xvld             $xr1, %[sv], 0                 \n"
      "xvldx            $xr3, %[sv], %[stv]            \n"
      "xvavgr.bu        $xr1, $xr1, $xr3               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvilvl.b         $xr2, $xr1, $xr0               \n"
      "xvilvh.b         $xr3, $xr1, $xr0               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "xvst             $xr3, %[dst], 32               \n"
      "addi.d           %[su], %[su], 32               \n"
      "addi.d           %[sv], %[sv], 32               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [su] "+r"(src_u),         // %[su]
        [sv] "+r"(src_v),         // %[sv]
        [dst] "+r"(dst_uv),       // %[dst]
        [cnt] "+r"(width)         // %[cnt]
      : [stu] "r"(src_stride_u),  // %[stu]
        [stv] "r"(src_stride_v)   // %[stv]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_HALFUVROW_LASX

#ifdef HAS_CONVERT16TO8ROW_LASX
void Convert16To8Row_LASX(const uint16_t* src_y,
                          uint8_t* dst_y,
                          int scale,
                          int width) {
  asm volatile(
      "xvreplgr2vr.h    $xr2, %[scl]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvmuh.hu         $xr0, $xr0, $xr2               \n"
      "xvmuh.hu         $xr1, $xr1, $xr2               \n"
      "xvssrlni.bu.h    $xr1, $xr0, 0                  \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvst             $xr1, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [scl] "r"(scale)    // %[scl]
      : "memory", "$xr0", "$xr1", "$xr2");
}
#endif  // HAS_CONVERT16TO8ROW_LASX

#ifdef HAS_CONVERT8TO16ROW_LASX
void Convert8To16Row_LASX(const uint8_t* src_y,
                          uint16_t* dst_y,
                          int bits,
                          int width) {
  const int shift = 16 - bits;
  asm volatile(
      "xvreplgr2vr.h    $xr3, %[shf]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvilvl.b         $xr1, $xr0, $xr0               \n"
      "xvilvh.b         $xr2, $xr0, $xr0               \n"
      "xvsrl.h          $xr1, $xr1, $xr3               \n"
      "xvsrl.h          $xr2, $xr2, $xr3               \n"
      "xvst             $xr1, %[dst], 0                \n"
      "xvst             $xr2, %[dst], 32               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [shf] "r"(shift)    // %[shf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_CONVERT8TO16ROW_LASX

#ifdef HAS_MULTIPLYROW_16_LASX
void MultiplyRow_16_LASX(const uint16_t* src_y,
                         uint16_t* dst_y,
                         int scale,
                         int width) {
  asm volatile(
      "xvreplgr2vr.h    $xr2, %[scl]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvmul.h          $xr0, $xr0, $xr2               \n"
      "xvmul.h          $xr1, $xr1, $xr2               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [scl] "r"(scale)    // %[scl]
      : "memory", "$xr0", "$xr1", "$xr2");
}
#endif  // HAS_MULTIPLYROW_16_LASX

#ifdef HAS_DIVIDEROW_16_LASX
void DivideRow_16_LASX(const uint16_t* src_y,
                       uint16_t* dst_y,
                       int scale,
                       int width) {
  if (scale == 65536) {  // Divide by 1, so do memcpy.
    if (src_y != dst_y) {
      memcpy(dst_y, src_y, (size_t)width * 2);
    }
    return;
  }
  asm volatile(
      "xvreplgr2vr.h    $xr2, %[scl]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvmuh.hu         $xr0, $xr0, $xr2               \n"
      "xvmuh.hu         $xr1, $xr1, $xr2               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [scl] "r"(scale)    // %[scl]
      : "memory", "$xr0", "$xr1", "$xr2");
}
#endif  // HAS_DIVIDEROW_16_LASX

#ifdef HAS_MERGEUVROW_16_LASX
void MergeUVRow_16_LASX(const uint16_t* src_u,
                        const uint16_t* src_v,
                        uint16_t* dst_uv,
                        int depth,
                        int width) {
  int shift = 16 - depth;
  asm volatile(
      "xvreplgr2vr.h    $xr4, %[shf]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[su], 0                 \n"
      "xvld             $xr1, %[sv], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvsll.h          $xr0, $xr0, $xr4               \n"
      "xvsll.h          $xr1, $xr1, $xr4               \n"
      "xvilvl.h         $xr2, $xr1, $xr0               \n"
      "xvilvh.h         $xr3, $xr1, $xr0               \n"
      "xvst             $xr2, %[dst], 0                \n"
      "xvst             $xr3, %[dst], 32               \n"
      "addi.d           %[su], %[su], 32               \n"
      "addi.d           %[sv], %[sv], 32               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [su] "+r"(src_u),    // %[su]
        [sv] "+r"(src_v),    // %[sv]
        [dst] "+r"(dst_uv),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      : [shf] "r"(shift)     // %[shf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_MERGEUVROW_16_LASX

#ifdef HAS_SPLITUVROW_16_LASX
void SplitUVRow_16_LASX(const uint16_t* src_uv,
                        uint16_t* dst_u,
                        uint16_t* dst_v,
                        int depth,
                        int width) {
  int shift = 16 - depth;
  asm volatile(
      "xvreplgr2vr.h    $xr4, %[shf]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.h       $xr2, $xr1, $xr0               \n"
      "xvpickod.h       $xr3, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpermi.d        $xr3, $xr3, 0xd8               \n"
      "xvsrl.h          $xr2, $xr2, $xr4               \n"
      "xvsrl.h          $xr3, $xr3, $xr4               \n"
      "xvst             $xr2, %[du], 0                 \n"
      "xvst             $xr3, %[dv], 0                 \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[du], %[du], 32               \n"
      "addi.d           %[dv], %[dv], 32               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [du] "+r"(dst_u),    // %[du]
        [dv] "+r"(dst_v),    // %[dv]
        [cnt] "+r"(width)    // %[cnt]
      : [shf] "r"(shift)     // %[shf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_SPLITUVROW_16_LASX

#ifdef HAS_COPYROW_16_LASX
void CopyRow_16_LASX(const uint16_t* src, uint16_t* dst, int count) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),   // %[src]
        [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(count)  // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_COPYROW_16_LASX

#ifdef HAS_HALFROW_LASX
void HalfRow_LASX(const uint8_t* src_uv,
                  ptrdiff_t src_uv_stride,
                  uint8_t* dst_uv,
                  int width) {
  const uint8_t* src_uv1 = src_uv + src_uv_stride;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src0], 0               \n"
      "xvld             $xr1, %[src1], 0               \n"
      "xvavgr.bu        $xr0, $xr0, $xr1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 32           \n"
      "addi.d           %[src1], %[src1], 32           \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_HALFROW_LASX

#ifdef HAS_HALFROW_16_LASX
void HalfRow_16_LASX(const uint16_t* src_uv,
                     ptrdiff_t src_uv_stride,
                     uint16_t* dst_uv,
                     int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src0], 0               \n"
      "xvld             $xr1, %[src1], 0               \n"
      "xvavgr.hu        $xr0, $xr0, $xr1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 32           \n"
      "addi.d           %[src1], %[src1], 32           \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_HALFROW_16_LASX

#ifdef HAS_HALFWIDTHROW_LASX
void HalfWidthRow_LASX(const uint8_t* src_ptr, uint8_t* dst_ptr, int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvavgr.bu        $xr0, $xr2, $xr3               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst_ptr),  // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_HALFWIDTHROW_LASX

#ifdef HAS_HALFWIDTHROW_16_LASX
void HalfWidthRow_16_LASX(const uint16_t* src_ptr,
                          uint16_t* dst_ptr,
                          int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.h       $xr2, $xr1, $xr0               \n"
      "xvpickod.h       $xr3, $xr1, $xr0               \n"
      "xvavgr.hu        $xr0, $xr2, $xr3               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst_ptr),  // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_HALFWIDTHROW_16_LASX

#ifdef HAS_HALFWIDTHUVROW_LASX
void HalfWidthUVRow_LASX(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.h       $xr2, $xr1, $xr0               \n"
      "xvpickod.h       $xr3, $xr1, $xr0               \n"
      "xvavgr.bu        $xr0, $xr2, $xr3               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [dst] "+r"(dst_uv),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_HALFWIDTHUVROW_LASX

#ifdef HAS_HALFWIDTHUVROW_16_LASX
void HalfWidthUVRow_16_LASX(const uint16_t* src_uv,
                            uint16_t* dst_uv,
                            int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.w       $xr2, $xr1, $xr0               \n"
      "xvpickod.w       $xr3, $xr1, $xr0               \n"
      "xvavgr.hu        $xr0, $xr2, $xr3               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),  // %[src]
        [dst] "+r"(dst_uv),  // %[dst]
        [cnt] "+r"(width)    // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_HALFWIDTHUVROW_16_LASX

#ifdef HAS_HALFBOXROW_LASX
void HalfBoxRow_LASX(const uint8_t* src_ptr,
                     ptrdiff_t src_stride,
                     uint8_t* dst_ptr,
                     int width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvld             $xr2, %[nex], 0                \n"
      "xvld             $xr3, %[nex], 32               \n"
      "xvaddwev.h.bu    $xr4, $xr0, $xr2               \n"
      "xvaddwod.h.bu    $xr5, $xr0, $xr2               \n"
      "xvaddwev.h.bu    $xr6, $xr1, $xr3               \n"
      "xvaddwod.h.bu    $xr7, $xr1, $xr3               \n"
      "xvadd.h          $xr4, $xr4, $xr5               \n"
      "xvadd.h          $xr6, $xr6, $xr7               \n"
      "xvsrarni.b.h     $xr6, $xr4, 2                  \n"
      "xvpermi.d        $xr6, $xr6, 0xd8               \n"
      "xvst             $xr6, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[nex], %[nex], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ptr),  // %[src]
        [nex] "+r"(src_nex),  // %[nex]
        [dst] "+r"(dst_ptr),  // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_HALFBOXROW_LASX

#ifdef HAS_HALFBOXROW_16_LASX
void HalfBoxRow_16_LASX(const uint16_t* src_ptr,
                        ptrdiff_t src_stride,
                        uint16_t* dst_ptr,
                        int width) {
  const uint16_t* src_nex = src_ptr + src_stride;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvld             $xr2, %[nex], 0                \n"
      "xvld             $xr3, %[nex], 32               \n"
      "xvaddwev.w.hu    $xr4, $xr0, $xr2               \n"
      "xvaddwod.w.hu    $xr5, $xr0, $xr2               \n"
      "xvaddwev.w.hu    $xr6, $xr1, $xr3               \n"
      "xvaddwod.w.hu    $xr7, $xr1, $xr3               \n"
      "xvadd.w          $xr4, $xr4, $xr5               \n"
      "xvadd.w          $xr6, $xr6, $xr7               \n"
      "xvsrlrni.h.w     $xr6, $xr4, 2                  \n"
      "xvpermi.d        $xr6, $xr6, 0xd8               \n"
      "xvst             $xr6, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[nex], %[nex], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ptr),  // %[src]
        [nex] "+r"(src_nex),  // %[nex]
        [dst] "+r"(dst_ptr),  // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_HALFBOXROW_16_LASX

#ifdef HAS_HALFBOXUVROW_LASX
void HalfBoxUVRow_LASX(const uint8_t* src_uv,
                       ptrdiff_t src_stride_uv,
                       uint8_t* dst_uv,
                       int width) {
  const uint8_t* src_nex = src_uv + src_stride_uv;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvld             $xr2, %[nex], 0                \n"
      "xvld             $xr3, %[nex], 32               \n"
      "xvpickev.h       $xr4, $xr1, $xr0               \n"
      "xvpickod.h       $xr5, $xr1, $xr0               \n"
      "xvpickev.h       $xr6, $xr3, $xr2               \n"
      "xvpickod.h       $xr7, $xr3, $xr2               \n"
      "xvaddwev.h.bu    $xr0, $xr4, $xr5               \n"
      "xvaddwod.h.bu    $xr1, $xr4, $xr5               \n"
      "xvaddwev.h.bu    $xr2, $xr6, $xr7               \n"
      "xvaddwod.h.bu    $xr3, $xr6, $xr7               \n"
      "xvadd.h          $xr0, $xr0, $xr2               \n"
      "xvadd.h          $xr1, $xr1, $xr3               \n"
      "xvsrari.h        $xr0, $xr0, 2                  \n"
      "xvsrari.h        $xr1, $xr1, 2                  \n"
      "xvpackev.b       $xr0, $xr1, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[nex], %[nex], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),   // %[src]
        [nex] "+r"(src_nex),  // %[nex]
        [dst] "+r"(dst_uv),   // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_HALFBOXUVROW_LASX

#ifdef HAS_HALFBOXUVROW_16_LASX
void HalfBoxUVRow_16_LASX(const uint16_t* src_uv,
                          ptrdiff_t src_stride_uv,
                          uint16_t* dst_uv,
                          int width) {
  const uint16_t* src_nex = src_uv + src_stride_uv;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvld             $xr2, %[nex], 0                \n"
      "xvld             $xr3, %[nex], 32               \n"
      "xvpickev.w       $xr4, $xr1, $xr0               \n"
      "xvpickod.w       $xr5, $xr1, $xr0               \n"
      "xvpickev.w       $xr6, $xr3, $xr2               \n"
      "xvpickod.w       $xr7, $xr3, $xr2               \n"
      "xvaddwev.w.hu    $xr0, $xr4, $xr5               \n"
      "xvaddwod.w.hu    $xr1, $xr4, $xr5               \n"
      "xvaddwev.w.hu    $xr2, $xr6, $xr7               \n"
      "xvaddwod.w.hu    $xr3, $xr6, $xr7               \n"
      "xvadd.w          $xr0, $xr0, $xr2               \n"
      "xvadd.w          $xr1, $xr1, $xr3               \n"
      "xvsrari.w        $xr0, $xr0, 2                  \n"
      "xvsrari.w        $xr1, $xr1, 2                  \n"
      "xvpackev.h       $xr0, $xr1, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[nex], %[nex], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_uv),   // %[src]
        [nex] "+r"(src_nex),  // %[nex]
        [dst] "+r"(dst_uv),   // %[dst]
        [cnt] "+r"(width)     // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_HALFBOXUVROW_16_LASX

#ifdef HAS_ARGBCOPYALPHAROW_LASX
void ARGBCopyAlphaRow_LASX(const uint8_t* src, uint8_t* dst, int width) {
  asm volatile(
      "xvldi            $xr4, 0xff                     \n"
      "xvslli.w         $xr4, $xr4, 24                 \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvld             $xr2, %[dst], 0                \n"
      "xvld             $xr3, %[dst], 32               \n"
      "xvbitsel.v       $xr2, $xr2, $xr0, $xr4         \n"
      "xvbitsel.v       $xr3, $xr3, $xr1, $xr4         \n"
      "xvst             $xr2, %[dst], 0                \n"
      "xvst             $xr3, %[dst], 32               \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),   // %[src]
        [dst] "+r"(dst),   // %[dst]
        [cnt] "+r"(width)  // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_ARGBCOPYALPHAROW_LASX

#ifdef HAS_YUY2TONVUVROW_LASX
void YUY2ToNVUVRow_LASX(const uint8_t* src_yuy2,
                        int src_stride_yuy2,
                        uint8_t* dst_uv,
                        int width) {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src0], 0               \n"
      "xvld             $xr1, %[src0], 32              \n"
      "xvld             $xr2, %[src1], 0               \n"
      "xvld             $xr3, %[src1], 32              \n"
      "xvpickod.b       $xr0, $xr1, $xr0               \n"
      "xvpickod.b       $xr2, $xr3, $xr2               \n"
      "xvavgr.bu        $xr0, $xr0, $xr2               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 64           \n"
      "addi.d           %[src1], %[src1], 64           \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_yuy2),    // %[src0]
        [src1] "+r"(src_yuy2_1),  // %[src1]
        [dst] "+r"(dst_uv),       // %[dst]
        [cnt] "+r"(width)         // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_YUY2TONVUVROW_LASX

#ifdef HAS_AR64TOARGBROW_LASX
void AR64ToARGBRow_LASX(const uint16_t* src_ar64,
                        uint8_t* dst_argb,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickod.b       $xr0, $xr1, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ar64),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_AR64TOARGBROW_LASX

#ifdef HAS_AB64TOARGBROW_LASX
void AB64ToARGBRow_LASX(const uint16_t* src_ab64,
                        uint8_t* dst_argb,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickod.b       $xr0, $xr1, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvshuf4i.b       $xr0, $xr0, 0xc6               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ab64),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_AB64TOARGBROW_LASX

#ifdef HAS_ARGBTOAR64ROW_LASX
void ARGBToAR64Row_LASX(const uint8_t* src_argb,
                        uint16_t* dst_ar64,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvilvl.b         $xr1, $xr0, $xr0               \n"
      "xvilvh.b         $xr2, $xr0, $xr0               \n"
      "xvst             $xr1, %[dst], 0                \n"
      "xvst             $xr2, %[dst], 32               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_ar64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2");
}
#endif  // HAS_ARGBTOAR64ROW_LASX

#ifdef HAS_ARGBTOAB64ROW_LASX
void ARGBToAB64Row_LASX(const uint8_t* src_argb,
                        uint16_t* dst_ab64,
                        int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvshuf4i.b       $xr0, $xr0, 0xc6               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvilvl.b         $xr1, $xr0, $xr0               \n"
      "xvilvh.b         $xr2, $xr0, $xr0               \n"
      "xvst             $xr1, %[dst], 0                \n"
      "xvst             $xr2, %[dst], 32               \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_ab64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2");
}
#endif  // HAS_ARGBTOAB64ROW_LASX

#ifdef HAS_CONVERT8TO8ROW_LASX
void Convert8To8Row_LASX(const uint8_t* src_y,
                         uint8_t* dst_y,
                         int scale,
                         int bias,
                         int width) {
  asm volatile(
      "xvreplgr2vr.b    $xr2, %[scl]                   \n"
      "xvreplgr2vr.b    $xr3, %[bia]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvmuh.bu         $xr0, $xr0, $xr2               \n"
      "xvadd.b          $xr0, $xr0, $xr3               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      : [scl] "r"(scale),   // %[scl]
        [bia] "r"(bias)     // %[bia]
      : "memory", "$xr0", "$xr2", "$xr3");
}
#endif  // HAS_CONVERT8TO8ROW_LASX

#ifdef HAS_HALFROW_16TO8_LASX
void HalfRow_16To8_LASX(const uint16_t* src_uv,
                        ptrdiff_t src_uv_stride,
                        uint8_t* dst_uv,
                        int scale,
                        int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  asm volatile(
      "xvreplgr2vr.h    $xr4, %[scl]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src0], 0               \n"
      "xvld             $xr1, %[src0], 32              \n"
      "xvld             $xr2, %[src1], 0               \n"
      "xvld             $xr3, %[src1], 32              \n"
      "xvavgr.hu        $xr0, $xr0, $xr2               \n"
      "xvavgr.hu        $xr1, $xr1, $xr3               \n"
      "xvmuh.hu         $xr0, $xr0, $xr4               \n"
      "xvmuh.hu         $xr1, $xr1, $xr4               \n"
      "xvssrlni.bu.h    $xr1, $xr0, 0                  \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvst             $xr1, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 64           \n"
      "addi.d           %[src1], %[src1], 64           \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [scl] "r"(scale)       // %[scl]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_HALFROW_16TO8_LASX

#ifdef HAS_HALFWIDTHROW_16TO8_LASX
// 2x2 box filter of 16 bit samples, scaled to 8 bit.  16 pixels per loop.
// scale is 1 << (24 - depth) and must not exceed 32768 (depth 9 or more).
void HalfWidthRow_16To8_LASX(const uint16_t* src_uv,
                             ptrdiff_t src_uv_stride,
                             uint8_t* dst_uv,
                             int scale,
                             int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  asm volatile(
      "xvreplgr2vr.h    $xr6, %[scl]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src0], 0               \n"
      "xvld             $xr1, %[src0], 32              \n"
      "xvld             $xr2, %[src1], 0               \n"
      "xvld             $xr3, %[src1], 32              \n"
      "xvaddwev.w.hu    $xr4, $xr0, $xr2               \n"
      "xvaddwod.w.hu    $xr0, $xr0, $xr2               \n"
      "xvaddwev.w.hu    $xr5, $xr1, $xr3               \n"
      "xvaddwod.w.hu    $xr1, $xr1, $xr3               \n"
      "xvadd.w          $xr0, $xr0, $xr4               \n"
      "xvadd.w          $xr1, $xr1, $xr5               \n"
      "xvsrlrni.h.w     $xr1, $xr0, 2                  \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvmuh.hu         $xr1, $xr1, $xr6               \n"
      "xvssrlni.bu.h    $xr1, $xr1, 0                  \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "vst              $vr1, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 64           \n"
      "addi.d           %[src1], %[src1], 64           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_uv),   // %[src0]
        [src1] "+r"(src_uv1),  // %[src1]
        [dst] "+r"(dst_uv),    // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [scl] "r"(scale)       // %[scl]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_HALFWIDTHROW_16TO8_LASX

#ifdef HAS_AR30TOAB30ROW_LASX
void AR30ToAB30Row_LASX(const uint8_t* src_ar30,
                        uint8_t* dst_ab30,
                        int width) {
  uint32_t m_ag = 0xc00ffc00u;
  uint32_t m_r = 0x3ff00000u;
  uint32_t m_b = 0x000003ffu;
  asm volatile(
      "xvreplgr2vr.w    $xr3, %[m_ag]                  \n"
      "xvreplgr2vr.w    $xr4, %[m_r]                   \n"
      "xvreplgr2vr.w    $xr5, %[m_b]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvsrli.w         $xr1, $xr0, 20                 \n"
      "xvslli.w         $xr2, $xr0, 20                 \n"
      "xvand.v          $xr0, $xr0, $xr3               \n"
      "xvand.v          $xr1, $xr1, $xr5               \n"
      "xvand.v          $xr2, $xr2, $xr4               \n"
      "xvor.v           $xr0, $xr0, $xr1               \n"
      "xvor.v           $xr0, $xr0, $xr2               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ar30),  // %[src]
        [dst] "+r"(dst_ab30),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_ag] "r"(m_ag),      // %[m_ag]
        [m_r] "r"(m_r),        // %[m_r]
        [m_b] "r"(m_b)         // %[m_b]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_AR30TOAB30ROW_LASX

#ifdef HAS_ARGBTOAR30ROW_LASX
void ARGBToAR30Row_LASX(const uint8_t* src_argb,
                        uint8_t* dst_ar30,
                        int width) {
  uint32_t m_a = 0xc0000000u;
  uint32_t m_r = 0x3fc00000u;
  uint32_t m_g = 0x000ff000u;
  uint32_t m_b = 0x000003fcu;
  uint32_t m_l = 0x00300c03u;
  asm volatile(
      "xvreplgr2vr.w    $xr4, %[m_a]                   \n"
      "xvreplgr2vr.w    $xr5, %[m_r]                   \n"
      "xvreplgr2vr.w    $xr6, %[m_g]                   \n"
      "xvreplgr2vr.w    $xr7, %[m_b]                   \n"
      "xvreplgr2vr.w    $xr8, %[m_l]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvslli.w         $xr1, $xr0, 6                  \n"
      "xvslli.w         $xr2, $xr0, 4                  \n"
      "xvslli.w         $xr3, $xr0, 2                  \n"
      "xvand.v          $xr0, $xr0, $xr4               \n"
      "xvand.v          $xr1, $xr1, $xr5               \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvand.v          $xr3, $xr3, $xr7               \n"
      "xvor.v           $xr0, $xr0, $xr1               \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvor.v           $xr0, $xr0, $xr2               \n"
      "xvsrli.w         $xr1, $xr0, 8                  \n"
      "xvand.v          $xr1, $xr1, $xr8               \n"
      "xvor.v           $xr0, $xr0, $xr1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_ar30),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_a] "r"(m_a),        // %[m_a]
        [m_r] "r"(m_r),        // %[m_r]
        [m_g] "r"(m_g),        // %[m_g]
        [m_b] "r"(m_b),        // %[m_b]
        [m_l] "r"(m_l)         // %[m_l]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8");
}
#endif  // HAS_ARGBTOAR30ROW_LASX

#ifdef HAS_ABGRTOAR30ROW_LASX
void ABGRToAR30Row_LASX(const uint8_t* src_abgr,
                        uint8_t* dst_ar30,
                        int width) {
  uint32_t m_a = 0xc0000000u;
  uint32_t m_r = 0x3fc00000u;
  uint32_t m_g = 0x000ff000u;
  uint32_t m_b = 0x000003fcu;
  uint32_t m_l = 0x00300c03u;
  asm volatile(
      "xvreplgr2vr.w    $xr4, %[m_a]                   \n"
      "xvreplgr2vr.w    $xr5, %[m_r]                   \n"
      "xvreplgr2vr.w    $xr6, %[m_g]                   \n"
      "xvreplgr2vr.w    $xr7, %[m_b]                   \n"
      "xvreplgr2vr.w    $xr8, %[m_l]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvshuf4i.b       $xr0, $xr0, 0xc6               \n"
      "xvslli.w         $xr1, $xr0, 6                  \n"
      "xvslli.w         $xr2, $xr0, 4                  \n"
      "xvslli.w         $xr3, $xr0, 2                  \n"
      "xvand.v          $xr0, $xr0, $xr4               \n"
      "xvand.v          $xr1, $xr1, $xr5               \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvand.v          $xr3, $xr3, $xr7               \n"
      "xvor.v           $xr0, $xr0, $xr1               \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvor.v           $xr0, $xr0, $xr2               \n"
      "xvsrli.w         $xr1, $xr0, 8                  \n"
      "xvand.v          $xr1, $xr1, $xr8               \n"
      "xvor.v           $xr0, $xr0, $xr1               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_abgr),  // %[src]
        [dst] "+r"(dst_ar30),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_a] "r"(m_a),        // %[m_a]
        [m_r] "r"(m_r),        // %[m_r]
        [m_g] "r"(m_g),        // %[m_g]
        [m_b] "r"(m_b),        // %[m_b]
        [m_l] "r"(m_l)         // %[m_l]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8");
}
#endif  // HAS_ABGRTOAR30ROW_LASX

#ifdef HAS_AR30TOARGBROW_LASX
void AR30ToARGBRow_LASX(const uint8_t* src_ar30,
                        uint8_t* dst_argb,
                        int width) {
  uint32_t m_a = 0xc0000000u;
  uint32_t m_2 = 0x300000ffu;
  uint32_t m_4 = 0x0c00ff00u;
  uint32_t m_6 = 0x03ff0000u;
  asm volatile(
      "xvreplgr2vr.w    $xr4, %[m_a]                   \n"
      "xvreplgr2vr.w    $xr5, %[m_2]                   \n"
      "xvreplgr2vr.w    $xr6, %[m_4]                   \n"
      "xvreplgr2vr.w    $xr7, %[m_6]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvsrli.w         $xr1, $xr0, 2                  \n"
      "xvsrli.w         $xr2, $xr0, 4                  \n"
      "xvsrli.w         $xr3, $xr0, 6                  \n"
      "xvand.v          $xr0, $xr0, $xr4               \n"
      "xvand.v          $xr1, $xr1, $xr5               \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvand.v          $xr3, $xr3, $xr7               \n"
      "xvor.v           $xr0, $xr0, $xr1               \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvor.v           $xr0, $xr0, $xr2               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ar30),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_a] "r"(m_a),        // %[m_a]
        [m_2] "r"(m_2),        // %[m_2]
        [m_4] "r"(m_4),        // %[m_4]
        [m_6] "r"(m_6)         // %[m_6]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_AR30TOARGBROW_LASX

#ifdef HAS_AR30TOABGRROW_LASX
void AR30ToABGRRow_LASX(const uint8_t* src_ar30,
                        uint8_t* dst_abgr,
                        int width) {
  uint32_t m_a = 0xc0000000u;
  uint32_t m_2 = 0x300000ffu;
  uint32_t m_4 = 0x0c00ff00u;
  uint32_t m_6 = 0x03ff0000u;
  asm volatile(
      "xvreplgr2vr.w    $xr4, %[m_a]                   \n"
      "xvreplgr2vr.w    $xr5, %[m_2]                   \n"
      "xvreplgr2vr.w    $xr6, %[m_4]                   \n"
      "xvreplgr2vr.w    $xr7, %[m_6]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvsrli.w         $xr1, $xr0, 2                  \n"
      "xvsrli.w         $xr2, $xr0, 4                  \n"
      "xvsrli.w         $xr3, $xr0, 6                  \n"
      "xvand.v          $xr0, $xr0, $xr4               \n"
      "xvand.v          $xr1, $xr1, $xr5               \n"
      "xvand.v          $xr2, $xr2, $xr6               \n"
      "xvand.v          $xr3, $xr3, $xr7               \n"
      "xvor.v           $xr0, $xr0, $xr1               \n"
      "xvor.v           $xr2, $xr2, $xr3               \n"
      "xvor.v           $xr0, $xr0, $xr2               \n"
      "xvshuf4i.b       $xr0, $xr0, 0xc6               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -8             \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ar30),  // %[src]
        [dst] "+r"(dst_abgr),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [m_a] "r"(m_a),        // %[m_a]
        [m_2] "r"(m_2),        // %[m_2]
        [m_4] "r"(m_4),        // %[m_4]
        [m_6] "r"(m_6)         // %[m_6]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_AR30TOABGRROW_LASX

#ifdef HAS_AYUVTOYROW_LASX
void AYUVToYRow_LASX(const uint8_t* src_ayuv, uint8_t* dst_y, int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickod.h       $xr0, $xr1, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpickev.b       $xr0, $xr0, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_ayuv),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_AYUVTOYROW_LASX

#ifdef HAS_AYUVTOUVROW_LASX
void AYUVToUVRow_LASX(const uint8_t* src_ayuv,
                      int src_stride_ayuv,
                      uint8_t* dst_uv,
                      int width) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src0], 0               \n"
      "xvld             $xr1, %[src0], 32              \n"
      "xvld             $xr2, %[src1], 0               \n"
      "xvld             $xr3, %[src1], 32              \n"
      "xvpickev.h       $xr0, $xr1, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpickev.h       $xr2, $xr3, $xr2               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpickev.h       $xr1, $xr0, $xr0               \n"
      "xvpickod.h       $xr0, $xr0, $xr0               \n"
      "xvpickev.h       $xr3, $xr2, $xr2               \n"
      "xvpickod.h       $xr2, $xr2, $xr2               \n"
      "xvaddwev.h.bu    $xr4, $xr1, $xr0               \n"
      "xvaddwod.h.bu    $xr5, $xr1, $xr0               \n"
      "xvaddwev.h.bu    $xr0, $xr3, $xr2               \n"
      "xvaddwod.h.bu    $xr1, $xr3, $xr2               \n"
      "xvadd.h          $xr0, $xr4, $xr0               \n"
      "xvadd.h          $xr1, $xr5, $xr1               \n"
      "xvsrari.h        $xr0, $xr0, 2                  \n"
      "xvsrari.h        $xr1, $xr1, 2                  \n"
      "xvpackev.b       $xr0, $xr0, $xr1               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 64           \n"
      "addi.d           %[src1], %[src1], 64           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_ayuv),   // %[src0]
        [src1] "+r"(src_ayuv1),  // %[src1]
        [dst] "+r"(dst_uv),      // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_AYUVTOUVROW_LASX

#ifdef HAS_AYUVTOVUROW_LASX
void AYUVToVURow_LASX(const uint8_t* src_ayuv,
                      int src_stride_ayuv,
                      uint8_t* dst_vu,
                      int width) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src0], 0               \n"
      "xvld             $xr1, %[src0], 32              \n"
      "xvld             $xr2, %[src1], 0               \n"
      "xvld             $xr3, %[src1], 32              \n"
      "xvpickev.h       $xr0, $xr1, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpickev.h       $xr2, $xr3, $xr2               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpickev.h       $xr1, $xr0, $xr0               \n"
      "xvpickod.h       $xr0, $xr0, $xr0               \n"
      "xvpickev.h       $xr3, $xr2, $xr2               \n"
      "xvpickod.h       $xr2, $xr2, $xr2               \n"
      "xvaddwev.h.bu    $xr4, $xr1, $xr0               \n"
      "xvaddwod.h.bu    $xr5, $xr1, $xr0               \n"
      "xvaddwev.h.bu    $xr0, $xr3, $xr2               \n"
      "xvaddwod.h.bu    $xr1, $xr3, $xr2               \n"
      "xvadd.h          $xr0, $xr4, $xr0               \n"
      "xvadd.h          $xr1, $xr5, $xr1               \n"
      "xvsrari.h        $xr0, $xr0, 2                  \n"
      "xvsrari.h        $xr1, $xr1, 2                  \n"
      "xvpackev.b       $xr0, $xr1, $xr0               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "vst              $vr0, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 64           \n"
      "addi.d           %[src1], %[src1], 64           \n"
      "addi.d           %[dst], %[dst], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_ayuv),   // %[src0]
        [src1] "+r"(src_ayuv1),  // %[src1]
        [dst] "+r"(dst_vu),      // %[dst]
        [cnt] "+r"(width)        // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_AYUVTOVUROW_LASX

#ifdef HAS_DETILEROW_LASX
void DetileRow_LASX(const uint8_t* src,
                    ptrdiff_t src_tile_stride,
                    uint8_t* dst,
                    int width) {
  ptrdiff_t src_tile_stride2 = src_tile_stride * 2;
  const uint8_t* src1;
  asm volatile(
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bltz             %[cnt], 2f                     \n"
      "add.d            %[src1], %[src0], %[str]       \n"

      "1:                                              \n"
      "vld              $vr0, %[src0], 0               \n"
      "vld              $vr1, %[src1], 0               \n"
      "xvpermi.q        $xr0, $xr1, 0x02               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "add.d            %[src0], %[src0], %[str2]      \n"
      "add.d            %[src1], %[src1], %[str2]      \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bgez             %[cnt], 1b                     \n"

      "2:                                              \n"
      "addi.d           %[cnt], %[cnt], 32             \n"
      "blez             %[cnt], 3f                     \n"
      "vld              $vr0, %[src0], 0               \n"
      "vst              $vr0, %[dst], 0                \n"

      "3:                                              \n"
      : [src0] "+r"(src),             // %[src0]
        [src1] "=&r"(src1),           // %[src1]
        [dst] "+r"(dst),              // %[dst]
        [cnt] "+r"(width)             // %[cnt]
      : [str] "r"(src_tile_stride),   // %[str]
        [str2] "r"(src_tile_stride2)  // %[str2]
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_DETILEROW_LASX

#ifdef HAS_DETILEROW_16_LASX
void DetileRow_16_LASX(const uint16_t* src,
                       ptrdiff_t src_tile_stride,
                       uint16_t* dst,
                       int width) {
  ptrdiff_t byte_stride = src_tile_stride * 2;
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvst             $xr0, %[dst], 0                \n"
      "add.d            %[src], %[src], %[str]         \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src),        // %[src]
        [dst] "+r"(dst),        // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [str] "r"(byte_stride)  // %[str]
      : "memory", "$xr0");
}
#endif  // HAS_DETILEROW_16_LASX

#ifdef HAS_MERGEARGBROW_LASX
void MergeARGBRow_LASX(const uint8_t* src_r,
                       const uint8_t* src_g,
                       const uint8_t* src_b,
                       const uint8_t* src_a,
                       uint8_t* dst_argb,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "vld              $vr3, %[sa], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpermi.d        $xr3, $xr3, 0xd8               \n"
      "xvilvl.b         $xr4, $xr1, $xr0               \n"
      "xvilvl.b         $xr5, $xr3, $xr2               \n"
      "xvpermi.d        $xr4, $xr4, 0xd8               \n"
      "xvpermi.d        $xr5, $xr5, 0xd8               \n"
      "xvilvl.h         $xr0, $xr5, $xr4               \n"
      "xvilvh.h         $xr1, $xr5, $xr4               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[sa], %[sa], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [sa] "+r"(src_a),      // %[sa]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_MERGEARGBROW_LASX

#ifdef HAS_MERGEXRGBROW_LASX
void MergeXRGBRow_LASX(const uint8_t* src_r,
                       const uint8_t* src_g,
                       const uint8_t* src_b,
                       uint8_t* dst_argb,
                       int width) {
  asm volatile(
      "xvldi            $xr3, 0xff                     \n"

      "1:                                              \n"
      "vld              $vr0, %[sb], 0                 \n"
      "vld              $vr1, %[sg], 0                 \n"
      "vld              $vr2, %[sr], 0                 \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvilvl.b         $xr4, $xr1, $xr0               \n"
      "xvilvl.b         $xr5, $xr3, $xr2               \n"
      "xvpermi.d        $xr4, $xr4, 0xd8               \n"
      "xvpermi.d        $xr5, $xr5, 0xd8               \n"
      "xvilvl.h         $xr0, $xr5, $xr4               \n"
      "xvilvh.h         $xr1, $xr5, $xr4               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[sg], %[sg], 16               \n"
      "addi.d           %[sr], %[sr], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_MERGEXRGBROW_LASX

#ifdef HAS_SPLITARGBROW_LASX
void SplitARGBRow_LASX(const uint8_t* src_argb,
                       uint8_t* dst_r,
                       uint8_t* dst_g,
                       uint8_t* dst_b,
                       uint8_t* dst_a,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpermi.d        $xr3, $xr3, 0xd8               \n"
      "xvpickev.b       $xr0, $xr2, $xr2               \n"
      "xvpickod.b       $xr1, $xr2, $xr2               \n"
      "xvpickev.b       $xr4, $xr3, $xr3               \n"
      "xvpickod.b       $xr5, $xr3, $xr3               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvpermi.d        $xr4, $xr4, 0xd8               \n"
      "xvpermi.d        $xr5, $xr5, 0xd8               \n"
      "vst              $vr0, %[db], 0                 \n"
      "vst              $vr4, %[dg], 0                 \n"
      "vst              $vr1, %[dr], 0                 \n"
      "vst              $vr5, %[da], 0                 \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[db], %[db], 16               \n"
      "addi.d           %[dg], %[dg], 16               \n"
      "addi.d           %[dr], %[dr], 16               \n"
      "addi.d           %[da], %[da], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [db] "+r"(dst_b),      // %[db]
        [dg] "+r"(dst_g),      // %[dg]
        [dr] "+r"(dst_r),      // %[dr]
        [da] "+r"(dst_a),      // %[da]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_SPLITARGBROW_LASX

#ifdef HAS_SPLITXRGBROW_LASX
void SplitXRGBRow_LASX(const uint8_t* src_argb,
                       uint8_t* dst_r,
                       uint8_t* dst_g,
                       uint8_t* dst_b,
                       int width) {
  asm volatile(
      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvpickev.b       $xr2, $xr1, $xr0               \n"
      "xvpickod.b       $xr3, $xr1, $xr0               \n"
      "xvpermi.d        $xr2, $xr2, 0xd8               \n"
      "xvpermi.d        $xr3, $xr3, 0xd8               \n"
      "xvpickev.b       $xr0, $xr2, $xr2               \n"
      "xvpickod.b       $xr1, $xr2, $xr2               \n"
      "xvpickev.b       $xr4, $xr3, $xr3               \n"
      "xvpermi.d        $xr0, $xr0, 0xd8               \n"
      "xvpermi.d        $xr1, $xr1, 0xd8               \n"
      "xvpermi.d        $xr4, $xr4, 0xd8               \n"
      "vst              $vr0, %[db], 0                 \n"
      "vst              $vr4, %[dg], 0                 \n"
      "vst              $vr1, %[dr], 0                 \n"
      "addi.d           %[src], %[src], 64             \n"
      "addi.d           %[db], %[db], 16               \n"
      "addi.d           %[dg], %[dg], 16               \n"
      "addi.d           %[dr], %[dr], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_argb),  // %[src]
        [db] "+r"(dst_b),      // %[db]
        [dg] "+r"(dst_g),      // %[dg]
        [dr] "+r"(dst_r),      // %[dr]
        [cnt] "+r"(width)      // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_SPLITXRGBROW_LASX

#ifdef HAS_SPLITRGBROW_LASX
void SplitRGBRow_LASX(const uint8_t* src_rgb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      int width) {
  asm volatile(
      "xvld             $xr6, %[shr0], 0               \n"
      "xvld             $xr7, %[shr1], 0               \n"
      "xvld             $xr8, %[shg0], 0               \n"
      "xvld             $xr9, %[shg1], 0               \n"
      "xvld             $xr10, %[shb0], 0              \n"
      "xvld             $xr11, %[shb1], 0              \n"

      "1:                                              \n"
      "xvld             $xr0, %[src], 0                \n"
      "xvld             $xr1, %[src], 32               \n"
      "xvld             $xr2, %[src], 64               \n"
      // Arrange so each 128 bit lane holds 16 pixels across 3 registers.
      "xvori.b          $xr3, $xr0, 0                  \n"
      "xvpermi.q        $xr3, $xr1, 0x12               \n"  // 0..15, 48..63
      "xvpermi.q        $xr0, $xr2, 0x03               \n"  // 16..31, 64..79
      "xvpermi.q        $xr1, $xr2, 0x12               \n"  // 32..47, 80..95
      "xvshuf.b         $xr2, $xr0, $xr3, $xr6         \n"
      "xvshuf.b         $xr4, $xr0, $xr3, $xr8         \n"
      "xvshuf.b         $xr5, $xr0, $xr3, $xr10        \n"
      "xvshuf.b         $xr2, $xr1, $xr2, $xr7         \n"
      "xvshuf.b         $xr4, $xr1, $xr4, $xr9         \n"
      "xvshuf.b         $xr5, $xr1, $xr5, $xr11        \n"
      "xvst             $xr2, %[dr], 0                 \n"
      "xvst             $xr4, %[dg], 0                 \n"
      "xvst             $xr5, %[db], 0                 \n"
      "addi.d           %[src], %[src], 96             \n"
      "addi.d           %[dr], %[dr], 32               \n"
      "addi.d           %[dg], %[dg], 32               \n"
      "addi.d           %[db], %[db], 32               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb),           // %[src]
        [dr] "+r"(dst_r),              // %[dr]
        [dg] "+r"(dst_g),              // %[dg]
        [db] "+r"(dst_b),              // %[db]
        [cnt] "+r"(width)              // %[cnt]
      : [shr0] "r"(&kSplitRGBShufR0),  // %[shr0]
        [shr1] "r"(&kSplitRGBShufR1),  // %[shr1]
        [shg0] "r"(&kSplitRGBShufG0),  // %[shg0]
        [shg1] "r"(&kSplitRGBShufG1),  // %[shg1]
        [shb0] "r"(&kSplitRGBShufB0),  // %[shb0]
        [shb1] "r"(&kSplitRGBShufB1)   // %[shb1]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11");
}
#endif  // HAS_SPLITRGBROW_LASX

#ifdef HAS_MERGERGBROW_LASX
void MergeRGBRow_LASX(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      uint8_t* dst_rgb,
                      int width) {
  asm volatile(
      "xvld             $xr6, %[sh0a], 0               \n"
      "xvld             $xr7, %[sh0b], 0               \n"
      "xvld             $xr8, %[sh1a], 0               \n"
      "xvld             $xr9, %[sh1b], 0               \n"
      "xvld             $xr10, %[sh2a], 0              \n"
      "xvld             $xr11, %[sh2b], 0              \n"

      "1:                                              \n"
      "xvld             $xr0, %[sr], 0                 \n"
      "xvld             $xr1, %[sg], 0                 \n"
      "xvld             $xr2, %[sb], 0                 \n"
      "xvshuf.b         $xr3, $xr1, $xr0, $xr6         \n"
      "xvshuf.b         $xr4, $xr1, $xr0, $xr8         \n"
      "xvshuf.b         $xr5, $xr1, $xr0, $xr10        \n"
      "xvshuf.b         $xr3, $xr2, $xr3, $xr7         \n"  // 0..15, 48..63
      "xvshuf.b         $xr4, $xr2, $xr4, $xr9         \n"  // 16..31, 64..79
      "xvshuf.b         $xr5, $xr2, $xr5, $xr11        \n"  // 32..47, 80..95
      "xvori.b          $xr0, $xr3, 0                  \n"
      "xvpermi.q        $xr0, $xr4, 0x02               \n"  // 0..31
      "xvpermi.q        $xr3, $xr5, 0x30               \n"  // 32..63
      "xvpermi.q        $xr4, $xr5, 0x13               \n"  // 64..95
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr3, %[dst], 32               \n"
      "xvst             $xr4, %[dst], 64               \n"
      "addi.d           %[sr], %[sr], 32               \n"
      "addi.d           %[sg], %[sg], 32               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[dst], %[dst], 96             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sr] "+r"(src_r),              // %[sr]
        [sg] "+r"(src_g),              // %[sg]
        [sb] "+r"(src_b),              // %[sb]
        [dst] "+r"(dst_rgb),           // %[dst]
        [cnt] "+r"(width)              // %[cnt]
      : [sh0a] "r"(&kMergeRGBShuf0A),  // %[sh0a]
        [sh0b] "r"(&kMergeRGBShuf0B),  // %[sh0b]
        [sh1a] "r"(&kMergeRGBShuf1A),  // %[sh1a]
        [sh1b] "r"(&kMergeRGBShuf1B),  // %[sh1b]
        [sh2a] "r"(&kMergeRGBShuf2A),  // %[sh2a]
        [sh2b] "r"(&kMergeRGBShuf2B)   // %[sh2b]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11");
}
#endif  // HAS_MERGERGBROW_LASX

#ifdef HAS_MERGEAR64ROW_LASX
void MergeAR64Row_LASX(const uint16_t* src_r,
                       const uint16_t* src_g,
                       const uint16_t* src_b,
                       const uint16_t* src_a,
                       uint16_t* dst_ar64,
                       int depth,
                       int width) {
  int shift = 16 - depth;
  int max = (1 << depth) - 1;
  asm volatile(
      "xvreplgr2vr.h    $xr8, %[max]                   \n"
      "xvreplgr2vr.h    $xr9, %[shf]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[sb], 0                 \n"
      "xvld             $xr1, %[sg], 0                 \n"
      "xvld             $xr2, %[sr], 0                 \n"
      "xvld             $xr3, %[sa], 0                 \n"
      "xvmin.hu         $xr0, $xr0, $xr8               \n"
      "xvmin.hu         $xr1, $xr1, $xr8               \n"
      "xvmin.hu         $xr2, $xr2, $xr8               \n"
      "xvmin.hu         $xr3, $xr3, $xr8               \n"
      "xvsll.h          $xr0, $xr0, $xr9               \n"
      "xvsll.h          $xr1, $xr1, $xr9               \n"
      "xvsll.h          $xr2, $xr2, $xr9               \n"
      "xvsll.h          $xr3, $xr3, $xr9               \n"
      "xvilvl.h         $xr4, $xr1, $xr0               \n"
      "xvilvh.h         $xr5, $xr1, $xr0               \n"
      "xvilvl.h         $xr6, $xr3, $xr2               \n"
      "xvilvh.h         $xr7, $xr3, $xr2               \n"
      "xvilvl.w         $xr0, $xr6, $xr4               \n"  // 0,1, 8,9
      "xvilvh.w         $xr1, $xr6, $xr4               \n"  // 2,3, 10,11
      "xvilvl.w         $xr2, $xr7, $xr5               \n"  // 4,5, 12,13
      "xvilvh.w         $xr3, $xr7, $xr5               \n"  // 6,7, 14,15
      "xvori.b          $xr4, $xr0, 0                  \n"
      "xvori.b          $xr5, $xr2, 0                  \n"
      "xvpermi.q        $xr4, $xr1, 0x02               \n"  // 0..3
      "xvpermi.q        $xr5, $xr3, 0x02               \n"  // 4..7
      "xvpermi.q        $xr1, $xr0, 0x31               \n"  // 8..11
      "xvpermi.q        $xr3, $xr2, 0x31               \n"  // 12..15
      "xvst             $xr4, %[dst], 0                \n"
      "xvst             $xr5, %[dst], 32               \n"
      "xvst             $xr1, %[dst], 64               \n"
      "xvst             $xr3, %[dst], 96               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[sg], %[sg], 32               \n"
      "addi.d           %[sr], %[sr], 32               \n"
      "addi.d           %[sa], %[sa], 32               \n"
      "addi.d           %[dst], %[dst], 128            \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [sa] "+r"(src_a),      // %[sa]
        [dst] "+r"(dst_ar64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [max] "r"(max),        // %[max]
        [shf] "r"(shift)       // %[shf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9");
}
#endif  // HAS_MERGEAR64ROW_LASX

#ifdef HAS_MERGEXR64ROW_LASX
void MergeXR64Row_LASX(const uint16_t* src_r,
                       const uint16_t* src_g,
                       const uint16_t* src_b,
                       uint16_t* dst_ar64,
                       int depth,
                       int width) {
  int shift = 16 - depth;
  int max = (1 << depth) - 1;
  asm volatile(
      "xvreplgr2vr.h    $xr8, %[max]                   \n"
      "xvreplgr2vr.h    $xr9, %[shf]                   \n"
      "xvldi            $xr10, 0xff                    \n"

      "1:                                              \n"
      "xvld             $xr0, %[sb], 0                 \n"
      "xvld             $xr1, %[sg], 0                 \n"
      "xvld             $xr2, %[sr], 0                 \n"
      "xvmin.hu         $xr0, $xr0, $xr8               \n"
      "xvmin.hu         $xr1, $xr1, $xr8               \n"
      "xvmin.hu         $xr2, $xr2, $xr8               \n"
      "xvsll.h          $xr0, $xr0, $xr9               \n"
      "xvsll.h          $xr1, $xr1, $xr9               \n"
      "xvsll.h          $xr2, $xr2, $xr9               \n"
      "xvilvl.h         $xr4, $xr1, $xr0               \n"
      "xvilvh.h         $xr5, $xr1, $xr0               \n"
      "xvilvl.h         $xr6, $xr10, $xr2              \n"
      "xvilvh.h         $xr7, $xr10, $xr2              \n"
      "xvilvl.w         $xr0, $xr6, $xr4               \n"  // 0,1, 8,9
      "xvilvh.w         $xr1, $xr6, $xr4               \n"  // 2,3, 10,11
      "xvilvl.w         $xr2, $xr7, $xr5               \n"  // 4,5, 12,13
      "xvilvh.w         $xr3, $xr7, $xr5               \n"  // 6,7, 14,15
      "xvori.b          $xr4, $xr0, 0                  \n"
      "xvori.b          $xr5, $xr2, 0                  \n"
      "xvpermi.q        $xr4, $xr1, 0x02               \n"  // 0..3
      "xvpermi.q        $xr5, $xr3, 0x02               \n"  // 4..7
      "xvpermi.q        $xr1, $xr0, 0x31               \n"  // 8..11
      "xvpermi.q        $xr3, $xr2, 0x31               \n"  // 12..15
      "xvst             $xr4, %[dst], 0                \n"
      "xvst             $xr5, %[dst], 32               \n"
      "xvst             $xr1, %[dst], 64               \n"
      "xvst             $xr3, %[dst], 96               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[sg], %[sg], 32               \n"
      "addi.d           %[sr], %[sr], 32               \n"
      "addi.d           %[dst], %[dst], 128            \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [dst] "+r"(dst_ar64),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [max] "r"(max),        // %[max]
        [shf] "r"(shift)       // %[shf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10");
}
#endif  // HAS_MERGEXR64ROW_LASX

#ifdef HAS_MERGEXR30ROW_LASX
void MergeXR30Row_LASX(const uint16_t* src_r,
                       const uint16_t* src_g,
                       const uint16_t* src_b,
                       uint8_t* dst_ar30,
                       int depth,
                       int width) {
  int shift = depth - 10;
  asm volatile(
      "xvreplgr2vr.h    $xr4, %[shf]                   \n"
      "xvldi            $xr6, 0xff                     \n"
      "xvsrli.h         $xr5, $xr6, 6                  \n"  // 1023
      "xvslli.h         $xr6, $xr6, 14                 \n"  // 0xc000

      "1:                                              \n"
      "xvld             $xr0, %[sb], 0                 \n"
      "xvld             $xr1, %[sg], 0                 \n"
      "xvld             $xr2, %[sr], 0                 \n"
      "xvsrl.h          $xr0, $xr0, $xr4               \n"
      "xvsrl.h          $xr1, $xr1, $xr4               \n"
      "xvsrl.h          $xr2, $xr2, $xr4               \n"
      "xvmin.hu         $xr0, $xr0, $xr5               \n"
      "xvmin.hu         $xr1, $xr1, $xr5               \n"
      "xvmin.hu         $xr2, $xr2, $xr5               \n"
      "xvslli.h         $xr3, $xr1, 10                 \n"
      "xvsrli.h         $xr1, $xr1, 6                  \n"
      "xvslli.h         $xr2, $xr2, 4                  \n"
      "xvor.v           $xr0, $xr0, $xr3               \n"  // B | G << 10
      "xvor.v           $xr1, $xr1, $xr2               \n"  // G >> 6 | R << 4
      "xvor.v           $xr1, $xr1, $xr6               \n"  // A
      "xvilvl.h         $xr2, $xr1, $xr0               \n"  // 0..3, 8..11
      "xvilvh.h         $xr3, $xr1, $xr0               \n"  // 4..7, 12..15
      "xvori.b          $xr0, $xr2, 0                  \n"
      "xvpermi.q        $xr0, $xr3, 0x02               \n"  // 0..7
      "xvpermi.q        $xr3, $xr2, 0x31               \n"  // 8..15
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr3, %[dst], 32               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[sg], %[sg], 32               \n"
      "addi.d           %[sr], %[sr], 32               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [dst] "+r"(dst_ar30),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shf] "r"(shift)       // %[shf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_MERGEXR30ROW_LASX

#ifdef HAS_MERGEARGB16TO8ROW_LASX
void MergeARGB16To8Row_LASX(const uint16_t* src_r,
                            const uint16_t* src_g,
                            const uint16_t* src_b,
                            const uint16_t* src_a,
                            uint8_t* dst_argb,
                            int depth,
                            int width) {
  int shift = depth - 8;
  asm volatile(
      "xvreplgr2vr.h    $xr6, %[shf]                   \n"

      "1:                                              \n"
      "xvld             $xr0, %[sb], 0                 \n"
      "xvld             $xr1, %[sg], 0                 \n"
      "xvld             $xr2, %[sr], 0                 \n"
      "xvld             $xr3, %[sa], 0                 \n"
      "xvssrln.bu.h     $xr0, $xr0, $xr6               \n"
      "xvssrln.bu.h     $xr1, $xr1, $xr6               \n"
      "xvssrln.bu.h     $xr2, $xr2, $xr6               \n"
      "xvssrln.bu.h     $xr3, $xr3, $xr6               \n"
      "xvilvl.b         $xr4, $xr1, $xr0               \n"
      "xvilvl.b         $xr5, $xr3, $xr2               \n"
      "xvilvl.h         $xr0, $xr5, $xr4               \n"  // 0..3, 8..11
      "xvilvh.h         $xr1, $xr5, $xr4               \n"  // 4..7, 12..15
      "xvori.b          $xr2, $xr0, 0                  \n"
      "xvpermi.q        $xr2, $xr1, 0x02               \n"  // 0..7
      "xvpermi.q        $xr1, $xr0, 0x31               \n"  // 8..15
      "xvst             $xr2, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[sg], %[sg], 32               \n"
      "addi.d           %[sr], %[sr], 32               \n"
      "addi.d           %[sa], %[sa], 32               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [sa] "+r"(src_a),      // %[sa]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shf] "r"(shift)       // %[shf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_MERGEARGB16TO8ROW_LASX

#ifdef HAS_MERGEXRGB16TO8ROW_LASX
void MergeXRGB16To8Row_LASX(const uint16_t* src_r,
                            const uint16_t* src_g,
                            const uint16_t* src_b,
                            uint8_t* dst_argb,
                            int depth,
                            int width) {
  int shift = depth - 8;
  asm volatile(
      "xvreplgr2vr.h    $xr6, %[shf]                   \n"
      "xvldi            $xr3, 0xff                     \n"

      "1:                                              \n"
      "xvld             $xr0, %[sb], 0                 \n"
      "xvld             $xr1, %[sg], 0                 \n"
      "xvld             $xr2, %[sr], 0                 \n"
      "xvssrln.bu.h     $xr0, $xr0, $xr6               \n"
      "xvssrln.bu.h     $xr1, $xr1, $xr6               \n"
      "xvssrln.bu.h     $xr2, $xr2, $xr6               \n"
      "xvilvl.b         $xr4, $xr1, $xr0               \n"
      "xvilvl.b         $xr5, $xr3, $xr2               \n"
      "xvilvl.h         $xr0, $xr5, $xr4               \n"  // 0..3, 8..11
      "xvilvh.h         $xr1, $xr5, $xr4               \n"  // 4..7, 12..15
      "xvori.b          $xr2, $xr0, 0                  \n"
      "xvpermi.q        $xr2, $xr1, 0x02               \n"  // 0..7
      "xvpermi.q        $xr1, $xr0, 0x31               \n"  // 8..15
      "xvst             $xr2, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[sg], %[sg], 32               \n"
      "addi.d           %[sr], %[sr], 32               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [sb] "+r"(src_b),      // %[sb]
        [sg] "+r"(src_g),      // %[sg]
        [sr] "+r"(src_r),      // %[sr]
        [dst] "+r"(dst_argb),  // %[dst]
        [cnt] "+r"(width)      // %[cnt]
      : [shf] "r"(shift)       // %[shf]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_MERGEXRGB16TO8ROW_LASX

#ifdef HAS_RGB24MIRRORROW_LASX
static const uvec8 kRGB24MirrorShuf0_LASX = {13, 14, 15, 10, 11, 12, 7, 8,
                                             9,  4,  5,  6,  1,  2,  3, 30};
static const uvec8 kRGB24MirrorShuf1_LASX = {15, 16, 11, 12, 13, 8, 9, 10,
                                             5,  6,  7,  2,  3,  4, 0, 0};
static const uvec8 kRGB24MirrorShuf2_LASX = {17, 12, 13, 14, 9, 10, 11, 6,
                                             7,  8,  3,  4,  5, 0,  1,  2};
void RGB24MirrorRow_LASX(const uint8_t* src_rgb24,
                         uint8_t* dst_rgb24,
                         int width) {
  src_rgb24 += (ptrdiff_t)width * 3 - 96;
  asm volatile(
      "vld              $vr4, %[sh0], 0                \n"
      "vld              $vr5, %[sh1], 0                \n"
      "vld              $vr6, %[sh2], 0                \n"
      "xvpermi.q        $xr4, $xr4, 0x00               \n"
      "xvpermi.q        $xr5, $xr5, 0x00               \n"
      "xvpermi.q        $xr6, $xr6, 0x00               \n"

      "1:                                              \n"
      "vld              $vr2, %[src], 80               \n"
      "vld              $vr1, %[src], 64               \n"
      "vld              $vr0, %[src], 48               \n"
      "vld              $vr7, %[src], 32               \n"
      "vld              $vr8, %[src], 16               \n"
      "vld              $vr9, %[src], 0                \n"
      "xvpermi.q        $xr2, $xr7, 0x02               \n"
      "xvpermi.q        $xr1, $xr8, 0x02               \n"
      "xvpermi.q        $xr0, $xr9, 0x02               \n"
      "xvshuf.b         $xr3, $xr1, $xr2, $xr4         \n"
      "xvshuf.b         $xr7, $xr2, $xr1, $xr5         \n"
      "xvextrins.b      $xr7, $xr0, 0xef               \n"
      "xvshuf.b         $xr0, $xr1, $xr0, $xr6         \n"
      "vst              $vr3, %[dst], 0                \n"
      "vst              $vr7, %[dst], 16               \n"
      "vst              $vr0, %[dst], 32               \n"
      "xvpermi.q        $xr3, $xr3, 0x01               \n"
      "xvpermi.q        $xr7, $xr7, 0x01               \n"
      "xvpermi.q        $xr0, $xr0, 0x01               \n"
      "vst              $vr3, %[dst], 48               \n"
      "vst              $vr7, %[dst], 64               \n"
      "vst              $vr0, %[dst], 80               \n"
      "addi.d           %[src], %[src], -96            \n"
      "addi.d           %[dst], %[dst], 96             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src] "+r"(src_rgb24),               // %[src]
        [dst] "+r"(dst_rgb24),               // %[dst]
        [cnt] "+r"(width)                    // %[cnt]
      : [sh0] "r"(&kRGB24MirrorShuf0_LASX),  // %[sh0]
        [sh1] "r"(&kRGB24MirrorShuf1_LASX),  // %[sh1]
        [sh2] "r"(&kRGB24MirrorShuf2_LASX)   // %[sh2]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9");
}
#endif  // HAS_RGB24MIRRORROW_LASX

#ifdef HAS_BLENDPLANEROW_LASX
void BlendPlaneRow_LASX(const uint8_t* src0,
                        const uint8_t* src1,
                        const uint8_t* alpha,
                        uint8_t* dst,
                        int width) {
  asm volatile(
      "xvldi            $xr6, 0x4ff                    \n"

      "1:                                              \n"
      "xvld             $xr0, %[s0], 0                 \n"
      "xvld             $xr1, %[s1], 0                 \n"
      "xvld             $xr2, %[alp], 0                \n"
      "xvxori.b         $xr3, $xr2, 0xff               \n"
      "xvmulwev.h.bu    $xr4, $xr0, $xr2               \n"
      "xvmaddwev.h.bu   $xr4, $xr1, $xr3               \n"
      "xvmulwod.h.bu    $xr5, $xr0, $xr2               \n"
      "xvmaddwod.h.bu   $xr5, $xr1, $xr3               \n"
      "xvadd.h          $xr4, $xr4, $xr6               \n"
      "xvadd.h          $xr5, $xr5, $xr6               \n"
      "xvpackod.b       $xr0, $xr5, $xr4               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "addi.d           %[s0], %[s0], 32               \n"
      "addi.d           %[s1], %[s1], 32               \n"
      "addi.d           %[alp], %[alp], 32             \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bnez             %[cnt], 1b                     \n"
      : [s0] "+r"(src0),    // %[s0]
        [s1] "+r"(src1),    // %[s1]
        [alp] "+r"(alpha),  // %[alp]
        [dst] "+r"(dst),    // %[dst]
        [cnt] "+r"(width)   // %[cnt]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_BLENDPLANEROW_LASX

#ifdef HAS_INTERPOLATEROW_16_LASX
void InterpolateRow_16_LASX(uint16_t* dst_ptr,
                            const uint16_t* src_ptr,
                            ptrdiff_t src_stride,
                            int width,
                            int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  if (y1_fraction == 0) {
    asm volatile(
        "1:                                              \n"
        "xvld             $xr0, %[src0], 0               \n"
        "xvst             $xr0, %[dst], 0                \n"
        "addi.d           %[src0], %[src0], 32           \n"
        "addi.d           %[dst], %[dst], 32             \n"
        "addi.d           %[cnt], %[cnt], -16            \n"
        "bnez             %[cnt], 1b                     \n"
        : [src0] "+r"(src_ptr),  // %[src0]
          [dst] "+r"(dst_ptr),   // %[dst]
          [cnt] "+r"(width)      // %[cnt]
        :
        : "memory", "$xr0");
    return;
  }
  if (y1_fraction == 128) {
    HalfRow_16_LASX(src_ptr, src_stride, dst_ptr, width);
    return;
  }
  asm volatile(
      "xvreplgr2vr.h    $xr4, %[f0]                    \n"
      "xvreplgr2vr.h    $xr5, %[f1]                    \n"

      "1:                                              \n"
      "xvld             $xr0, %[src0], 0               \n"
      "xvld             $xr1, %[src1], 0               \n"
      "xvmulwev.w.hu    $xr2, $xr0, $xr4               \n"
      "xvmaddwev.w.hu   $xr2, $xr1, $xr5               \n"
      "xvmulwod.w.hu    $xr3, $xr0, $xr4               \n"
      "xvmaddwod.w.hu   $xr3, $xr1, $xr5               \n"
      "xvsrari.w        $xr2, $xr2, 8                  \n"
      "xvsrari.w        $xr3, $xr3, 8                  \n"
      "xvpackev.h       $xr3, $xr3, $xr2               \n"
      "xvst             $xr3, %[dst], 0                \n"
      "addi.d           %[src0], %[src0], 32           \n"
      "addi.d           %[src1], %[src1], 32           \n"
      "addi.d           %[dst], %[dst], 32             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bnez             %[cnt], 1b                     \n"
      : [src0] "+r"(src_ptr),   // %[src0]
        [src1] "+r"(src_ptr1),  // %[src1]
        [dst] "+r"(dst_ptr),    // %[dst]
        [cnt] "+r"(width)       // %[cnt]
      : [f0] "r"(y0_fraction),  // %[f0]
        [f1] "r"(y1_fraction)   // %[f1]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_INTERPOLATEROW_16_LASX

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // (!defined(LIBYUV_DISABLE_LSX) || !defined(LIBYUV_DISABLE_LASX)) &&
        // defined(__loongarch_lp64)

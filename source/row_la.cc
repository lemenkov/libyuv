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
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(count)
      :
      : "memory", "$vr0");
}
#endif  // HAS_COPYROW_LSX

#ifdef HAS_SETROW_LSX
void SetRow_LSX(uint8_t* dst, uint8_t v8, int count) {
  asm volatile(
      "vreplgr2vr.b $vr0, %[val]\n\t"
      "1:\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [dst] "+r"(dst), [cnt] "+r"(count)
      : [val] "r"(v8)
      : "memory", "$vr0");
}
#endif  // HAS_SETROW_LSX

#ifdef HAS_ARGBSETROW_LSX
void ARGBSetRow_LSX(uint8_t* dst_argb, uint32_t v32, int width) {
  asm volatile(
      "vreplgr2vr.w $vr0, %[val]\n\t"
      "1:\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [val] "r"(v32)
      : "memory", "$vr0");
}
#endif  // HAS_ARGBSETROW_LSX

#ifdef HAS_MIRRORROW_LSX
void MirrorRow_LSX(const uint8_t* src_y, uint8_t* dst_y, int width) {
  static const uint8_t kRev[16] = {15, 14, 13, 12, 11, 10, 9, 8,
                                   7,  6,  5,  4,  3,  2,  1, 0};
  src_y += width - 16;
  asm volatile(
      "vld $vr1, %[rev], 0\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vshuf.b $vr0, $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], -16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [rev] "r"(kRev)
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_MIRRORROW_LSX

#ifdef HAS_MIRRORUVROW_LSX
void MirrorUVRow_LSX(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  static const uint8_t kRevUV[16] = {14, 15, 12, 13, 10, 11, 8, 9,
                                     6,  7,  4,  5,  2,  3,  0, 1};
  src_uv += width * 2 - 16;
  asm volatile(
      "vld $vr1, %[rev], 0\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vshuf.b $vr0, $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], -16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uv), [dst] "+r"(dst_uv), [cnt] "+r"(width)
      : [rev] "r"(kRevUV)
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_MIRRORUVROW_LSX

#ifdef HAS_ARGBMIRRORROW_LSX
void ARGBMirrorRow_LSX(const uint8_t* src_argb, uint8_t* dst_argb, int width) {
  src_argb += width * 4 - 16;
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vshuf4i.w $vr0, $vr0, 0x1b\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], -16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
  src_uv += width * 2 - 32;
  asm volatile(
      "vld $vr4, %[rev], 0\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 16\n\t"
      "vld $vr1, %[src], 0\n\t"
      "vshuf.b $vr0, $vr0, $vr0, $vr4\n\t"
      "vshuf.b $vr1, $vr1, $vr1, $vr4\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vpickod.b $vr3, $vr1, $vr0\n\t"
      "vst $vr2, %[du], 0\n\t"
      "vst $vr3, %[dv], 0\n\t"
      "addi.d %[src], %[src], -32\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      :
      [src] "+r"(src_uv), [du] "+r"(dst_u), [dv] "+r"(dst_v), [cnt] "+r"(width)
      : [rev] "r"(kRevUV)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_MIRRORSPLITUVROW_LSX

#ifdef HAS_SPLITUVROW_LSX
void SplitUVRow_LSX(const uint8_t* src_uv,
                    uint8_t* dst_u,
                    uint8_t* dst_v,
                    int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vpickod.b $vr3, $vr1, $vr0\n\t"
      "vst $vr2, %[du], 0\n\t"
      "vst $vr3, %[dv], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      :
      [src] "+r"(src_uv), [du] "+r"(dst_u), [dv] "+r"(dst_v), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[su], 0\n\t"
      "vld $vr1, %[sv], 0\n\t"
      "vilvl.b $vr2, $vr1, $vr0\n\t"
      "vilvh.b $vr3, $vr1, $vr0\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "vst $vr3, %[dst], 16\n\t"
      "addi.d %[su], %[su], 16\n\t"
      "addi.d %[sv], %[sv], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      :
      [su] "+r"(src_u), [sv] "+r"(src_v), [dst] "+r"(dst_uv), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_MERGEUVROW_LSX

#ifdef HAS_ARGBEXTRACTALPHAROW_LSX
void ARGBExtractAlphaRow_LSX(const uint8_t* src_argb,
                             uint8_t* dst_a,
                             int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickod.b $vr0, $vr1, $vr0\n\t"
      "vpickod.b $vr0, $vr0, $vr0\n\t"
      "vstelm.d $vr0, %[dst], 0, 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 8\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_a), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "vldrepl.d $vr1, %[su], 0\n\t"
      "vldrepl.d $vr2, %[sv], 0\n\t"
      "vilvl.b $vr3, $vr2, $vr1\n\t"
      "vilvl.b $vr4, $vr3, $vr0\n\t"
      "vilvh.b $vr5, $vr3, $vr0\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "vst $vr5, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_yuy2), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "vldrepl.d $vr1, %[su], 0\n\t"
      "vldrepl.d $vr2, %[sv], 0\n\t"
      "vilvl.b $vr3, $vr2, $vr1\n\t"
      "vilvl.b $vr4, $vr0, $vr3\n\t"
      "vilvh.b $vr5, $vr0, $vr3\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "vst $vr5, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_uyvy), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_I422TOUYVYROW_LSX

#ifdef HAS_YUY2TOYROW_LSX
void YUY2ToYRow_LSX(const uint8_t* src_yuy2, uint8_t* dst_y, int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_yuy2), [dst] "+r"(dst_y), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_YUY2TOYROW_LSX

#ifdef HAS_UYVYTOYROW_LSX
void UYVYToYRow_LSX(const uint8_t* src_uyvy, uint8_t* dst_y, int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickod.b $vr2, $vr1, $vr0\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uyvy), [dst] "+r"(dst_y), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickod.b $vr2, $vr1, $vr0\n\t"
      "vpickev.b $vr3, $vr2, $vr2\n\t"
      "vpickod.b $vr4, $vr2, $vr2\n\t"
      "vstelm.d $vr3, %[du], 0, 0\n\t"
      "vstelm.d $vr4, %[dv], 0, 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[du], %[du], 8\n\t"
      "addi.d %[dv], %[dv], 8\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_yuy2), [du] "+r"(dst_u), [dv] "+r"(dst_v),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vpickev.b $vr3, $vr2, $vr2\n\t"
      "vpickod.b $vr4, $vr2, $vr2\n\t"
      "vstelm.d $vr3, %[du], 0, 0\n\t"
      "vstelm.d $vr4, %[dv], 0, 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[du], %[du], 8\n\t"
      "addi.d %[dv], %[dv], 8\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uyvy), [du] "+r"(dst_u), [dv] "+r"(dst_v),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s0], 16\n\t"
      "vld $vr2, %[s1], 0\n\t"
      "vld $vr3, %[s1], 16\n\t"
      "vpickod.b $vr0, $vr1, $vr0\n\t"
      "vpickod.b $vr2, $vr3, $vr2\n\t"
      "vavgr.bu $vr0, $vr0, $vr2\n\t"
      "vpickev.b $vr1, $vr0, $vr0\n\t"
      "vpickod.b $vr2, $vr0, $vr0\n\t"
      "vstelm.d $vr1, %[du], 0, 0\n\t"
      "vstelm.d $vr2, %[dv], 0, 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[du], %[du], 8\n\t"
      "addi.d %[dv], %[dv], 8\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_yuy2), [s1] "+r"(next_yuy2), [du] "+r"(dst_u),
        [dv] "+r"(dst_v), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s0], 16\n\t"
      "vld $vr2, %[s1], 0\n\t"
      "vld $vr3, %[s1], 16\n\t"
      "vpickev.b $vr0, $vr1, $vr0\n\t"
      "vpickev.b $vr2, $vr3, $vr2\n\t"
      "vavgr.bu $vr0, $vr0, $vr2\n\t"
      "vpickev.b $vr1, $vr0, $vr0\n\t"
      "vpickod.b $vr2, $vr0, $vr0\n\t"
      "vstelm.d $vr1, %[du], 0, 0\n\t"
      "vstelm.d $vr2, %[dv], 0, 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[du], %[du], 8\n\t"
      "addi.d %[dv], %[dv], 8\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_uyvy), [s1] "+r"(next_uyvy), [du] "+r"(dst_u),
        [dv] "+r"(dst_v), [cnt] "+r"(width)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.w $vr10, %[su], 0\n\t"
      "vldrepl.w $vr11, %[sv], 0\n\t"
      "vilvl.b $vr10, $vr10, $vr10\n\t"
      "vilvl.b $vr11, $vr11, $vr11\n\t"
      "vilvl.b $vr1, $vr11, $vr10\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr4\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[su], %[su], 4\n\t"
      "addi.d %[sv], %[sv], 4\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.w $vr10, %[su], 0\n\t"
      "vldrepl.w $vr11, %[sv], 0\n\t"
      "vilvl.b $vr10, $vr10, $vr10\n\t"
      "vilvl.b $vr11, $vr11, $vr11\n\t"
      "vilvl.b $vr1, $vr11, $vr10\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr2, $vr5\n\t"
      "vilvl.b $vr7, $vr4, $vr3\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[su], %[su], 4\n\t"
      "addi.d %[sv], %[sv], 4\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.w $vr10, %[su], 0\n\t"
      "vldrepl.w $vr11, %[sv], 0\n\t"
      "vilvl.b $vr10, $vr10, $vr10\n\t"
      "vilvl.b $vr11, $vr11, $vr11\n\t"
      "vilvl.b $vr1, $vr11, $vr10\n\t"
      "vldrepl.d $vr5, %[sa], 0\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr4\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[su], %[su], 4\n\t"
      "addi.d %[sv], %[sv], 4\n\t"
      "addi.d %[sa], %[sa], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width), [sa] "+r"(src_a)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vld $vr12, %[s0], 0\n\t"
      "vld $vr13, %[s1], 0\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.w $vr10, %[su], 0\n\t"
      "vldrepl.w $vr11, %[sv], 0\n\t"
      "vilvl.b $vr10, $vr10, $vr10\n\t"
      "vilvl.b $vr11, $vr11, $vr11\n\t"
      "vilvl.b $vr1, $vr11, $vr10\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvl.b $vr7, $vr4, $vr4\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vshuf.b $vr10, $vr9, $vr8, $vr12\n\t"
      "vshuf.b $vr11, $vr9, $vr8, $vr13\n\t"
      "vst $vr10, %[dst], 0\n\t"
      "vstelm.d $vr11, %[dst], 16, 0\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[su], %[su], 4\n\t"
      "addi.d %[sv], %[sv], 4\n\t"
      "addi.d %[dst], %[dst], 24\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants), [s0] "r"(kShuf0), [s1] "r"(kShuf1)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr14, 31\n\t"
      "vsrli.h $vr14, $vr14, 8\n\t"
      "vldi $vr15, 63\n\t"
      "vsrli.h $vr15, $vr15, 8\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.w $vr10, %[su], 0\n\t"
      "vldrepl.w $vr11, %[sv], 0\n\t"
      "vilvl.b $vr10, $vr10, $vr10\n\t"
      "vilvl.b $vr11, $vr11, $vr11\n\t"
      "vilvl.b $vr1, $vr11, $vr10\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vsrli.b $vr2, $vr2, 3\n\t"
      "vsrli.b $vr3, $vr3, 2\n\t"
      "vsrli.b $vr4, $vr4, 3\n\t"
      "vilvl.b $vr2, $vr2, $vr2\n\t"
      "vilvl.b $vr3, $vr3, $vr3\n\t"
      "vilvl.b $vr4, $vr4, $vr4\n\t"
      "vand.v $vr2, $vr2, $vr14\n\t"
      "vand.v $vr3, $vr3, $vr15\n\t"
      "vand.v $vr4, $vr4, $vr14\n\t"
      "vslli.h $vr3, $vr3, 5\n\t"
      "vslli.h $vr4, $vr4, 11\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vor.v $vr2, $vr2, $vr4\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[su], %[su], 4\n\t"
      "addi.d %[sv], %[sv], 4\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr14, 15\n\t"
      "vsrli.h $vr14, $vr14, 8\n\t"
      "vldi $vr15, 240\n\t"
      "vsrli.h $vr15, $vr15, 8\n\t"
      "vslli.h $vr15, $vr15, 8\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.w $vr10, %[su], 0\n\t"
      "vldrepl.w $vr11, %[sv], 0\n\t"
      "vilvl.b $vr10, $vr10, $vr10\n\t"
      "vilvl.b $vr11, $vr11, $vr11\n\t"
      "vilvl.b $vr1, $vr11, $vr10\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vsrli.b $vr2, $vr2, 4\n\t"
      "vsrli.b $vr3, $vr3, 4\n\t"
      "vsrli.b $vr4, $vr4, 4\n\t"
      "vilvl.b $vr2, $vr2, $vr2\n\t"
      "vilvl.b $vr3, $vr3, $vr3\n\t"
      "vilvl.b $vr4, $vr4, $vr4\n\t"
      "vand.v $vr2, $vr2, $vr14\n\t"
      "vand.v $vr3, $vr3, $vr14\n\t"
      "vand.v $vr4, $vr4, $vr14\n\t"
      "vslli.h $vr3, $vr3, 4\n\t"
      "vslli.h $vr4, $vr4, 8\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vor.v $vr2, $vr2, $vr4\n\t"
      "vor.v $vr2, $vr2, $vr15\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[su], %[su], 4\n\t"
      "addi.d %[sv], %[sv], 4\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr14, 31\n\t"
      "vsrli.h $vr14, $vr14, 8\n\t"
      "vldi $vr15, 128\n\t"
      "vsrli.h $vr15, $vr15, 8\n\t"
      "vslli.h $vr15, $vr15, 8\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.w $vr10, %[su], 0\n\t"
      "vldrepl.w $vr11, %[sv], 0\n\t"
      "vilvl.b $vr10, $vr10, $vr10\n\t"
      "vilvl.b $vr11, $vr11, $vr11\n\t"
      "vilvl.b $vr1, $vr11, $vr10\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vsrli.b $vr2, $vr2, 3\n\t"
      "vsrli.b $vr3, $vr3, 3\n\t"
      "vsrli.b $vr4, $vr4, 3\n\t"
      "vilvl.b $vr2, $vr2, $vr2\n\t"
      "vilvl.b $vr3, $vr3, $vr3\n\t"
      "vilvl.b $vr4, $vr4, $vr4\n\t"
      "vand.v $vr2, $vr2, $vr14\n\t"
      "vand.v $vr3, $vr3, $vr14\n\t"
      "vand.v $vr4, $vr4, $vr14\n\t"
      "vslli.h $vr3, $vr3, 5\n\t"
      "vslli.h $vr4, $vr4, 10\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vor.v $vr2, $vr2, $vr4\n\t"
      "vor.v $vr2, $vr2, $vr15\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[su], %[su], 4\n\t"
      "addi.d %[sv], %[sv], 4\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.d $vr1, %[suv], 0\n\t"
      "vilvl.h $vr1, $vr1, $vr1\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr4\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[suv], %[suv], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [suv] "+r"(src_uv), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.d $vr1, %[suv], 0\n\t"
      "vshuf4i.b $vr1, $vr1, 0xb1\n\t"
      "vilvl.h $vr1, $vr1, $vr1\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr4\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[suv], %[suv], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [suv] "+r"(src_vu), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr14, 31\n\t"
      "vsrli.h $vr14, $vr14, 8\n\t"
      "vldi $vr15, 63\n\t"
      "vsrli.h $vr15, $vr15, 8\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.d $vr1, %[suv], 0\n\t"
      "vilvl.h $vr1, $vr1, $vr1\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vsrli.b $vr2, $vr2, 3\n\t"
      "vsrli.b $vr3, $vr3, 2\n\t"
      "vsrli.b $vr4, $vr4, 3\n\t"
      "vilvl.b $vr2, $vr2, $vr2\n\t"
      "vilvl.b $vr3, $vr3, $vr3\n\t"
      "vilvl.b $vr4, $vr4, $vr4\n\t"
      "vand.v $vr2, $vr2, $vr14\n\t"
      "vand.v $vr3, $vr3, $vr15\n\t"
      "vand.v $vr4, $vr4, $vr14\n\t"
      "vslli.h $vr3, $vr3, 5\n\t"
      "vslli.h $vr4, $vr4, 11\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vor.v $vr2, $vr2, $vr4\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[suv], %[suv], 8\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [suv] "+r"(src_uv), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vldrepl.d $vr10, %[su], 0\n\t"
      "vldrepl.d $vr11, %[sv], 0\n\t"
      "vilvl.b $vr1, $vr11, $vr10\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr4\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vld $vr23, %[c], 96\n\t"
      "vld $vr24, %[c], 128\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sy], 0\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vssrani.bu.h $vr2, $vr0, 6\n\t"
      "vilvl.b $vr6, $vr2, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr2\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
      : "memory", "$vr0", "$vr2", "$vr5", "$vr6", "$vr7", "$vr8", "$vr9",
        "$vr23", "$vr24");
}
#endif  // HAS_I400TOARGBROW_LSX

#ifdef HAS_J400TOARGBROW_LSX
void J400ToARGBRow_LSX(const uint8_t* src_y, uint8_t* dst_argb, int width) {
  asm volatile(
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr2, %[sy], 0\n\t"
      "vilvl.b $vr6, $vr2, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr2\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr10, %[src], 0\n\t"
      "vpickev.b $vr0, $vr10, $vr10\n\t"
      "vpickod.b $vr1, $vr10, $vr10\n\t"
      "vilvl.h $vr1, $vr1, $vr1\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr4\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_yuy2), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vldrepl.w $vr20, %[c], 0\n\t"
      "vldrepl.w $vr21, %[c], 32\n\t"
      "vldrepl.w $vr22, %[c], 64\n\t"
      "vldrepl.h $vr23, %[c], 96\n\t"
      "vldrepl.h $vr24, %[c], 128\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr10, %[src], 0\n\t"
      "vpickod.b $vr0, $vr10, $vr10\n\t"
      "vpickev.b $vr1, $vr10, $vr10\n\t"
      "vilvl.h $vr1, $vr1, $vr1\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vxori.b $vr1, $vr1, 0x80\n\t"
      "vmuh.hu $vr0, $vr0, $vr23\n\t"
      "vadd.h $vr0, $vr0, $vr24\n\t"
      "vmulwev.h.bu.b $vr2, $vr20, $vr1\n\t"
      "vmulwod.h.bu.b $vr4, $vr22, $vr1\n\t"
      "vmulwev.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vmaddwod.h.bu.b $vr3, $vr21, $vr1\n\t"
      "vsadd.h $vr2, $vr0, $vr2\n\t"
      "vsadd.h $vr4, $vr0, $vr4\n\t"
      "vssub.h $vr3, $vr0, $vr3\n\t"
      "vssrani.bu.h $vr2, $vr2, 6\n\t"
      "vssrani.bu.h $vr3, $vr3, 6\n\t"
      "vssrani.bu.h $vr4, $vr4, 6\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvl.b $vr7, $vr5, $vr4\n\t"
      "vilvl.h $vr8, $vr7, $vr6\n\t"
      "vilvh.h $vr9, $vr7, $vr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uyvy), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vld $vr4, %[shuf], 0\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr2, %[src], 32\n\t"
      "vld $vr3, %[src], 48\n\t"
      "vshuf.b $vr0, $vr0, $vr0, $vr4\n\t"
      "vshuf.b $vr1, $vr1, $vr1, $vr4\n\t"
      "vshuf.b $vr2, $vr2, $vr2, $vr4\n\t"
      "vshuf.b $vr3, $vr3, $vr3, $vr4\n\t"
      "vstelm.d $vr0, %[dst], 0, 0\n\t"
      "vstelm.w $vr0, %[dst], 8, 2\n\t"
      "vstelm.w $vr1, %[dst], 12, 0\n\t"
      "vstelm.w $vr1, %[dst], 16, 1\n\t"
      "vstelm.w $vr1, %[dst], 20, 2\n\t"
      "vstelm.d $vr2, %[dst], 24, 0\n\t"
      "vstelm.w $vr2, %[dst], 32, 2\n\t"
      "vstelm.w $vr3, %[dst], 36, 0\n\t"
      "vstelm.w $vr3, %[dst], 40, 1\n\t"
      "vstelm.w $vr3, %[dst], 44, 2\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 48\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_24), [cnt] "+r"(width)
      : [shuf] "r"(shuf)
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
      "vld $vr4, %[shuf], 0\n\t"
      "vld $vr5, %[shuf], 16\n\t"
      "vldi $vr6, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 12\n\t"
      "vld $vr2, %[src], 24\n\t"
      "vld $vr3, %[src], 32\n\t"
      "vshuf.b $vr0, $vr6, $vr0, $vr4\n\t"
      "vshuf.b $vr1, $vr6, $vr1, $vr4\n\t"
      "vshuf.b $vr2, $vr6, $vr2, $vr4\n\t"
      "vshuf.b $vr3, $vr6, $vr3, $vr5\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "vst $vr2, %[dst], 32\n\t"
      "vst $vr3, %[dst], 48\n\t"
      "addi.d %[src], %[src], 48\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_24), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [shuf] "r"(shuf)
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
      "vld $vr4, %[shuf], 0\n\t"
      "vld $vr5, %[shuf], 16\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 12\n\t"
      "vld $vr2, %[src], 24\n\t"
      "vld $vr3, %[src], 32\n\t"
      "vshuf.b $vr0, $vr0, $vr0, $vr4\n\t"
      "vshuf.b $vr1, $vr1, $vr1, $vr4\n\t"
      "vshuf.b $vr2, $vr2, $vr2, $vr4\n\t"
      "vshuf.b $vr3, $vr3, $vr3, $vr5\n\t"
      "vstelm.d $vr0, %[dst], 0, 0\n\t"
      "vstelm.w $vr0, %[dst], 8, 2\n\t"
      "vstelm.w $vr1, %[dst], 12, 0\n\t"
      "vstelm.w $vr1, %[dst], 16, 1\n\t"
      "vstelm.w $vr1, %[dst], 20, 2\n\t"
      "vstelm.d $vr2, %[dst], 24, 0\n\t"
      "vstelm.w $vr2, %[dst], 32, 2\n\t"
      "vstelm.w $vr3, %[dst], 36, 0\n\t"
      "vstelm.w $vr3, %[dst], 40, 1\n\t"
      "vstelm.w $vr3, %[dst], 44, 2\n\t"
      "addi.d %[src], %[src], 48\n\t"
      "addi.d %[dst], %[dst], 48\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_raw), [dst] "+r"(dst_rgb24), [cnt] "+r"(width)
      : [shuf] "r"(kShuf)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_RAWTORGB24ROW_LSX

#ifdef HAS_ARGBTORGB565ROW_LSX
void ARGBToRGB565Row_LSX(const uint8_t* src_argb, uint8_t* dst_rgb, int width) {
  asm volatile(
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "vldi $vr7, 63\n\t"
      "vsrli.h $vr7, $vr7, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vpickod.b $vr3, $vr1, $vr0\n\t"
      "vsrli.b $vr2, $vr2, 3\n\t"
      "vsrli.b $vr3, $vr3, 2\n\t"
      "vand.v $vr4, $vr2, $vr6\n\t"
      "vsrli.h $vr2, $vr2, 8\n\t"
      "vand.v $vr3, $vr3, $vr7\n\t"
      "vslli.h $vr3, $vr3, 5\n\t"
      "vslli.h $vr2, $vr2, 11\n\t"
      "vor.v $vr4, $vr4, $vr3\n\t"
      "vor.v $vr4, $vr4, $vr2\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_rgb), [cnt] "+r"(width)
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
      "vreplgr2vr.w $vr5, %[dith]\n\t"
      "vilvl.b $vr5, $vr5, $vr5\n\t"
      "vilvl.h $vr5, $vr5, $vr5\n\t"
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "vldi $vr7, 63\n\t"
      "vsrli.h $vr7, $vr7, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vsadd.bu $vr0, $vr0, $vr5\n\t"
      "vpickev.b $vr2, $vr0, $vr0\n\t"
      "vpickod.b $vr3, $vr0, $vr0\n\t"
      "vsrli.b $vr2, $vr2, 3\n\t"
      "vsrli.b $vr3, $vr3, 2\n\t"
      "vand.v $vr4, $vr2, $vr6\n\t"
      "vsrli.h $vr2, $vr2, 8\n\t"
      "vand.v $vr3, $vr3, $vr7\n\t"
      "vslli.h $vr3, $vr3, 5\n\t"
      "vslli.h $vr2, $vr2, 11\n\t"
      "vor.v $vr4, $vr4, $vr3\n\t"
      "vor.v $vr4, $vr4, $vr2\n\t"
      "vstelm.d $vr4, %[dst], 0, 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 8\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_rgb), [cnt] "+r"(width)
      : [dith] "r"(dither4)
      : "memory", "$vr0", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6", "$vr7");
}
#endif  // HAS_ARGBTORGB565DITHERROW_LSX

#ifdef HAS_ARGBTOARGB1555ROW_LSX
void ARGBToARGB1555Row_LSX(const uint8_t* src_argb,
                           uint8_t* dst_rgb,
                           int width) {
  asm volatile(
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vpickod.b $vr3, $vr1, $vr0\n\t"
      "vsrli.b $vr2, $vr2, 3\n\t"
      "vand.v $vr4, $vr2, $vr6\n\t"
      "vsrli.h $vr2, $vr2, 8\n\t"
      "vsrli.h $vr5, $vr3, 15\n\t"
      "vsrli.b $vr3, $vr3, 3\n\t"
      "vand.v $vr3, $vr3, $vr6\n\t"
      "vslli.h $vr3, $vr3, 5\n\t"
      "vslli.h $vr2, $vr2, 10\n\t"
      "vslli.h $vr5, $vr5, 15\n\t"
      "vor.v $vr4, $vr4, $vr3\n\t"
      "vor.v $vr4, $vr4, $vr2\n\t"
      "vor.v $vr4, $vr4, $vr5\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_rgb), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_ARGBTOARGB1555ROW_LSX

#ifdef HAS_ARGBTOARGB4444ROW_LSX
void ARGBToARGB4444Row_LSX(const uint8_t* src_argb,
                           uint8_t* dst_rgb,
                           int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vpickod.b $vr3, $vr1, $vr0\n\t"
      "vsrli.b $vr2, $vr2, 4\n\t"
      "vsrli.b $vr3, $vr3, 4\n\t"
      "vslli.b $vr3, $vr3, 4\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_rgb), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_ARGBTOARGB4444ROW_LSX

#ifdef HAS_RGB565TOARGBROW_LSX
void RGB565ToARGBRow_LSX(const uint8_t* src_rgb565,
                         uint8_t* dst_argb,
                         int width) {
  asm volatile(
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "vldi $vr7, 63\n\t"
      "vsrli.h $vr7, $vr7, 8\n\t"
      "vldi $vr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr7\n\t"
      "vsrli.h $vr3, $vr0, 11\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 2\n\t"
      "vsrli.h $vr2, $vr2, 4\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vpickev.b $vr1, $vr1, $vr1\n\t"
      "vpickev.b $vr2, $vr2, $vr2\n\t"
      "vpickev.b $vr3, $vr3, $vr3\n\t"
      "vilvl.b $vr8, $vr2, $vr1\n\t"
      "vilvl.b $vr9, $vr5, $vr3\n\t"
      "vilvl.h $vr10, $vr9, $vr8\n\t"
      "vilvh.h $vr11, $vr9, $vr8\n\t"
      "vst $vr10, %[dst], 0\n\t"
      "vst $vr11, %[dst], 16\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_rgb565), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vsrli.h $vr3, $vr0, 10\n\t"
      "vand.v $vr3, $vr3, $vr6\n\t"
      "vsrai.h $vr5, $vr0, 15\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 3\n\t"
      "vsrli.h $vr2, $vr2, 2\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vpickev.b $vr1, $vr1, $vr1\n\t"
      "vpickev.b $vr2, $vr2, $vr2\n\t"
      "vpickev.b $vr3, $vr3, $vr3\n\t"
      "vpickev.b $vr5, $vr5, $vr5\n\t"
      "vilvl.b $vr8, $vr2, $vr1\n\t"
      "vilvl.b $vr9, $vr5, $vr3\n\t"
      "vilvl.h $vr10, $vr9, $vr8\n\t"
      "vilvh.h $vr11, $vr9, $vr8\n\t"
      "vst $vr10, %[dst], 0\n\t"
      "vst $vr11, %[dst], 16\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb1555), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "vldi $vr6, 0x0f\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.b $vr2, $vr0, 4\n\t"
      "vslli.b $vr3, $vr1, 4\n\t"
      "vor.v $vr1, $vr1, $vr3\n\t"
      "vslli.b $vr3, $vr2, 4\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vilvl.b $vr4, $vr2, $vr1\n\t"
      "vilvh.b $vr5, $vr2, $vr1\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "vst $vr5, %[dst], 16\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb4444), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s1], 0\n\t"
      "vmulwev.h.bu $vr2, $vr0, $vr1\n\t"
      "vmulwod.h.bu $vr3, $vr0, $vr1\n\t"
      "vsrari.h $vr2, $vr2, 8\n\t"
      "vsrari.h $vr3, $vr3, 8\n\t"
      "vpackev.b $vr2, $vr3, $vr2\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 16\n\t"
      "addi.d %[s1], %[s1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s1], 0\n\t"
      "vsadd.bu $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 16\n\t"
      "addi.d %[s1], %[s1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s1], 0\n\t"
      "vssub.bu $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 16\n\t"
      "addi.d %[s1], %[s1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
      "vld $vr4, %[shuf], 0\n\t"
      "vldi $vr5, 0xff\n\t"
      "vldi $vr6, 255\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vshuf.b $vr1, $vr5, $vr0, $vr4\n\t"
      "vmulwev.h.bu $vr2, $vr0, $vr1\n\t"
      "vmulwod.h.bu $vr3, $vr0, $vr1\n\t"
      "vadd.h $vr2, $vr2, $vr6\n\t"
      "vadd.h $vr3, $vr3, $vr6\n\t"
      "vpackod.b $vr2, $vr3, $vr2\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [shuf] "r"(kAlpha)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_ARGBATTENUATEROW_LSX

#ifdef HAS_ARGBBLENDROW_LSX
void ARGBBlendRow_LSX(const uint8_t* src_argb0,
                      const uint8_t* src_argb1,
                      uint8_t* dst_argb,
                      int width) {
  static const uint8_t kAlpha[16] = {3,  3,  3,  3,  7,  7,  7,  7,
                                     11, 11, 11, 11, 15, 15, 15, 15};
  asm volatile(
      "vld $vr4, %[shuf], 0\n\t"
      "vldi $vr5, 0xff\n\t"
      "vslli.w $vr5, $vr5, 24\n\t"
      "vldi $vr6, 1\n\t"
      "vslli.h $vr6, $vr6, 8\n\t"
      "vldi $vr8, 0\n\t"
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s1], 0\n\t"
      "vshuf.b $vr2, $vr0, $vr0, $vr4\n\t"
      "vilvl.b $vr3, $vr8, $vr2\n\t"
      "vilvh.b $vr2, $vr8, $vr2\n\t"
      "vsub.h $vr3, $vr6, $vr3\n\t"
      "vsub.h $vr2, $vr6, $vr2\n\t"
      "vilvl.b $vr7, $vr8, $vr1\n\t"
      "vilvh.b $vr1, $vr8, $vr1\n\t"
      "vmul.h $vr7, $vr7, $vr3\n\t"
      "vmul.h $vr1, $vr1, $vr2\n\t"
      "vsrlni.b.h $vr1, $vr7, 8\n\t"
      "vsadd.bu $vr0, $vr0, $vr1\n\t"
      "vor.v $vr0, $vr0, $vr5\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 16\n\t"
      "addi.d %[s1], %[s1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb0), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [shuf] "r"(kAlpha)
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
      "vreplgr2vr.w $vr1, %[val]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vor.v $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [val] "r"(value)
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
      "vld $vr8, %[shuf], 0\n\t"
      "vldi $vr9, 1\n\t"
      "vsrli.h $vr10, $vr9, 8\n\t"
      "vldi $vr11, 0xff\n\t"
      "vsrli.w $vr11, $vr11, 8\n\t"
      "vldi $vr12, 0\n\t"
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s1], 0\n\t"
      "vsrli.w $vr2, $vr0, 24\n\t"
      "vsrli.w $vr3, $vr1, 24\n\t"
      "vmul.h $vr4, $vr3, $vr2\n\t"
      "vadd.h $vr4, $vr4, $vr10\n\t"
      "vmuh.hu $vr4, $vr4, $vr9\n\t"
      "vsub.w $vr4, $vr2, $vr4\n\t"
      "vadd.w $vr4, $vr3, $vr4\n\t"
      "vseq.w $vr5, $vr3, $vr12\n\t"
      "vsrli.w $vr5, $vr5, 24\n\t"
      "vor.v $vr6, $vr2, $vr5\n\t"
      "vor.v $vr7, $vr4, $vr5\n\t"
      "vslli.w $vr5, $vr6, 8\n\t"
      "vsub.w $vr5, $vr5, $vr6\n\t"
      "vffint.s.wu $vr5, $vr5\n\t"
      "vffint.s.wu $vr7, $vr7\n\t"
      "vfdiv.s $vr5, $vr5, $vr7\n\t"
      "vftintrz.wu.s $vr5, $vr5\n\t"
      "vshuf.b $vr5, $vr5, $vr5, $vr8\n\t"
      "vnor.v $vr6, $vr5, $vr5\n\t"
      "vmulwev.h.bu $vr2, $vr1, $vr6\n\t"
      "vmulwod.h.bu $vr3, $vr1, $vr6\n\t"
      "vmulwev.h.bu $vr7, $vr0, $vr5\n\t"
      "vmulwod.h.bu $vr0, $vr0, $vr5\n\t"
      "vadd.h $vr2, $vr2, $vr7\n\t"
      "vadd.h $vr3, $vr3, $vr0\n\t"
      "vadd.h $vr2, $vr2, $vr10\n\t"
      "vadd.h $vr3, $vr3, $vr10\n\t"
      "vmuh.hu $vr2, $vr2, $vr9\n\t"
      "vmuh.hu $vr3, $vr3, $vr9\n\t"
      "vslli.h $vr3, $vr3, 8\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vand.v $vr2, $vr2, $vr11\n\t"
      "vslli.w $vr4, $vr4, 24\n\t"
      "vor.v $vr2, $vr2, $vr4\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 16\n\t"
      "addi.d %[s1], %[s1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb0), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [shuf] "r"(kShufRatio)
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
      "vreplgr2vr.w $vr4, %[val]\n\t"
      "vilvl.b $vr5, $vr4, $vr4\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vilvl.b $vr1, $vr0, $vr0\n\t"
      "vilvh.b $vr2, $vr0, $vr0\n\t"
      "vmuh.hu $vr1, $vr1, $vr5\n\t"
      "vmuh.hu $vr2, $vr2, $vr5\n\t"
      "vsrlni.b.h $vr2, $vr1, 8\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [val] "r"(value)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr4", "$vr5");
}
#endif  // HAS_ARGBSHADEROW_LSX

#ifdef HAS_ARGBSHUFFLEROW_LSX
void ARGBShuffleRow_LSX(const uint8_t* src_argb,
                        uint8_t* dst_argb,
                        const uint8_t* shuffler,
                        int width) {
  asm volatile(
      "vld $vr4, %[shuf], 0\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vshuf.b $vr0, $vr0, $vr0, $vr4\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [shuf] "r"(shuffler)
      : "memory", "$vr0", "$vr4");
}
#endif  // HAS_ARGBSHUFFLEROW_LSX

#ifdef HAS_ARGBGRAYROW_LSX
void ARGBGrayRow_LSX(const uint8_t* src_argb, uint8_t* dst_argb, int width) {
  asm volatile(
      "vldi $vr4, 29\n\t"
      "vldi $vr5, 150\n\t"
      "vldi $vr6, 77\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vpickod.b $vr3, $vr1, $vr0\n\t"
      "vmulwev.h.bu $vr7, $vr2, $vr4\n\t"
      "vmaddwod.h.bu $vr7, $vr2, $vr6\n\t"
      "vmaddwev.h.bu $vr7, $vr3, $vr5\n\t"
      "vsrlri.h $vr7, $vr7, 8\n\t"
      "vsrli.h $vr3, $vr3, 8\n\t"
      "vslli.h $vr3, $vr3, 8\n\t"
      "vor.v $vr3, $vr7, $vr3\n\t"
      "vslli.h $vr8, $vr7, 8\n\t"
      "vor.v $vr7, $vr7, $vr8\n\t"
      "vilvl.h $vr0, $vr3, $vr7\n\t"
      "vilvh.h $vr1, $vr3, $vr7\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8");
}
#endif  // HAS_ARGBGRAYROW_LSX

#ifdef HAS_ARGBSEPIAROW_LSX
void ARGBSepiaRow_LSX(uint8_t* dst_argb, int width) {
  asm volatile(
      "vldi $vr4, 17\n\t"
      "vldi $vr5, 68\n\t"
      "vldi $vr6, 35\n\t"
      "vldi $vr7, 22\n\t"
      "vldi $vr8, 88\n\t"
      "vldi $vr9, 45\n\t"
      "vldi $vr10, 24\n\t"
      "vldi $vr11, 98\n\t"
      "vldi $vr12, 50\n\t"
      "1:\n\t"
      "vld $vr0, %[dst], 0\n\t"
      "vld $vr1, %[dst], 16\n\t"
      "vpickev.b $vr2, $vr1, $vr0\n\t"
      "vpickod.b $vr3, $vr1, $vr0\n\t"
      "vmulwev.h.bu $vr13, $vr2, $vr4\n\t"
      "vmaddwod.h.bu $vr13, $vr2, $vr6\n\t"
      "vmaddwev.h.bu $vr13, $vr3, $vr5\n\t"
      "vmulwev.h.bu $vr14, $vr2, $vr7\n\t"
      "vmaddwod.h.bu $vr14, $vr2, $vr9\n\t"
      "vmaddwev.h.bu $vr14, $vr3, $vr8\n\t"
      "vmulwev.h.bu $vr15, $vr2, $vr10\n\t"
      "vmaddwod.h.bu $vr15, $vr2, $vr12\n\t"
      "vmaddwev.h.bu $vr15, $vr3, $vr11\n\t"
      "vsrli.h $vr13, $vr13, 7\n\t"
      "vsrli.h $vr14, $vr14, 7\n\t"
      "vsrli.h $vr15, $vr15, 7\n\t"
      "vsat.hu $vr13, $vr13, 7\n\t"
      "vsat.hu $vr14, $vr14, 7\n\t"
      "vsat.hu $vr15, $vr15, 7\n\t"
      "vpickev.b $vr13, $vr13, $vr13\n\t"
      "vpickev.b $vr14, $vr14, $vr14\n\t"
      "vpickev.b $vr15, $vr15, $vr15\n\t"
      "vpickod.b $vr3, $vr3, $vr3\n\t"
      "vilvl.b $vr2, $vr14, $vr13\n\t"
      "vilvl.b $vr3, $vr3, $vr15\n\t"
      "vilvl.h $vr0, $vr3, $vr2\n\t"
      "vilvh.h $vr1, $vr3, $vr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "vldrepl.w $vr4, %[mat], 0\n\t"
      "vldrepl.w $vr5, %[mat], 4\n\t"
      "vldrepl.w $vr6, %[mat], 8\n\t"
      "vldrepl.w $vr7, %[mat], 12\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vmulwev.h.bu.b $vr8, $vr0, $vr4\n\t"
      "vmaddwod.h.bu.b $vr8, $vr0, $vr4\n\t"
      "vmulwev.h.bu.b $vr9, $vr0, $vr5\n\t"
      "vmaddwod.h.bu.b $vr9, $vr0, $vr5\n\t"
      "vmulwev.h.bu.b $vr10, $vr0, $vr6\n\t"
      "vmaddwod.h.bu.b $vr10, $vr0, $vr6\n\t"
      "vmulwev.h.bu.b $vr11, $vr0, $vr7\n\t"
      "vmaddwod.h.bu.b $vr11, $vr0, $vr7\n\t"
      "vhaddw.w.h $vr8, $vr8, $vr8\n\t"
      "vhaddw.w.h $vr9, $vr9, $vr9\n\t"
      "vhaddw.w.h $vr10, $vr10, $vr10\n\t"
      "vhaddw.w.h $vr11, $vr11, $vr11\n\t"
      "vilvl.w $vr12, $vr9, $vr8\n\t"
      "vilvh.w $vr13, $vr9, $vr8\n\t"
      "vilvl.w $vr14, $vr11, $vr10\n\t"
      "vilvh.w $vr15, $vr11, $vr10\n\t"
      "vilvl.d $vr8, $vr14, $vr12\n\t"
      "vilvh.d $vr9, $vr14, $vr12\n\t"
      "vilvl.d $vr10, $vr15, $vr13\n\t"
      "vilvh.d $vr11, $vr15, $vr13\n\t"
      "vssrani.h.w $vr9, $vr8, 6\n\t"
      "vssrani.h.w $vr11, $vr10, 6\n\t"
      "vssrani.bu.h $vr11, $vr9, 0\n\t"
      "vst $vr11, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [mat] "r"(matrix_argb)
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
      "vreplgr2vr.h $vr4, %[scl]\n\t"
      "vreplgr2vr.b $vr5, %[isz]\n\t"
      "vreplgr2vr.b $vr6, %[ioff]\n\t"
      "vldi $vr7, 0xff\n\t"
      "vslli.w $vr7, $vr7, 24\n\t"
      "1:\n\t"
      "vld $vr0, %[dst], 0\n\t"
      "vilvl.b $vr1, $vr0, $vr0\n\t"
      "vilvh.b $vr2, $vr0, $vr0\n\t"
      "vsrli.h $vr1, $vr1, 8\n\t"
      "vsrli.h $vr2, $vr2, 8\n\t"
      "vmuh.hu $vr1, $vr1, $vr4\n\t"
      "vmuh.hu $vr2, $vr2, $vr4\n\t"
      "vsrlni.b.h $vr2, $vr1, 0\n\t"
      "vmul.b $vr2, $vr2, $vr5\n\t"
      "vadd.b $vr2, $vr2, $vr6\n\t"
      "vand.v $vr0, $vr0, $vr7\n\t"
      "vandn.v $vr2, $vr7, $vr2\n\t"
      "vor.v $vr2, $vr2, $vr0\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [scl] "r"(scale), [isz] "r"(interval_size), [ioff] "r"(interval_offset)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr4", "$vr5", "$vr6", "$vr7");
}
#endif  // HAS_ARGBQUANTIZEROW_LSX

#ifdef HAS_SOBELROW_LSX
void SobelRow_LSX(const uint8_t* src_sobelx,
                  const uint8_t* src_sobely,
                  uint8_t* dst_argb,
                  int width) {
  asm volatile(
      "vldi $vr4, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sx], 0\n\t"
      "vldrepl.d $vr1, %[sy], 0\n\t"
      "vsadd.bu $vr0, $vr0, $vr1\n\t"
      "vilvl.b $vr2, $vr0, $vr0\n\t"
      "vilvl.b $vr3, $vr4, $vr0\n\t"
      "vilvl.h $vr0, $vr3, $vr2\n\t"
      "vilvh.h $vr1, $vr3, $vr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "addi.d %[sx], %[sx], 8\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sx] "+r"(src_sobelx), [sy] "+r"(src_sobely), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[sx], 0\n\t"
      "vld $vr1, %[sy], 0\n\t"
      "vsadd.bu $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[sx], %[sx], 16\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sx] "+r"(src_sobelx), [sy] "+r"(src_sobely), [dst] "+r"(dst_y),
        [cnt] "+r"(width)
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
      "vldi $vr4, 0xff\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[sx], 0\n\t"
      "vldrepl.d $vr1, %[sy], 0\n\t"
      "vsadd.bu $vr2, $vr0, $vr1\n\t"
      "vilvl.b $vr1, $vr2, $vr1\n\t"
      "vilvl.b $vr0, $vr4, $vr0\n\t"
      "vilvl.h $vr2, $vr0, $vr1\n\t"
      "vilvh.h $vr3, $vr0, $vr1\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "vst $vr3, %[dst], 16\n\t"
      "addi.d %[sx], %[sx], 8\n\t"
      "addi.d %[sy], %[sy], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sx] "+r"(src_sobelx), [sy] "+r"(src_sobely), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
        "1:\n\t"
        "vld $vr0, %[s0], 0\n\t"
        "vld $vr1, %[s1], 0\n\t"
        "vavgr.bu $vr0, $vr0, $vr1\n\t"
        "vst $vr0, %[dst], 0\n\t"
        "addi.d %[s0], %[s0], 16\n\t"
        "addi.d %[s1], %[s1], 16\n\t"
        "addi.d %[dst], %[dst], 16\n\t"
        "addi.d %[cnt], %[cnt], -16\n\t"
        "bnez %[cnt], 1b\n\t"
        : [s0] "+r"(src_ptr), [s1] "+r"(src_ptr1), [dst] "+r"(dst_ptr),
          [cnt] "+r"(width)
        :
        : "memory", "$vr0", "$vr1");
    return;
  }
  uint16_t y_frac = (uint16_t)((256 - y1_fraction) + (y1_fraction << 8));
  asm volatile(
      "vreplgr2vr.h $vr3, %[yf]\n\t"
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s1], 0\n\t"
      "vilvl.b $vr2, $vr1, $vr0\n\t"
      "vilvh.b $vr4, $vr1, $vr0\n\t"
      "vmulwev.h.bu $vr0, $vr2, $vr3\n\t"
      "vmaddwod.h.bu $vr0, $vr2, $vr3\n\t"
      "vmulwev.h.bu $vr1, $vr4, $vr3\n\t"
      "vmaddwod.h.bu $vr1, $vr4, $vr3\n\t"
      "vsrarni.b.h $vr1, $vr0, 8\n\t"
      "vst $vr1, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 16\n\t"
      "addi.d %[s1], %[s1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_ptr), [s1] "+r"(src_ptr1), [dst] "+r"(dst_ptr),
        [cnt] "+r"(width)
      : [yf] "r"(y_frac)
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
      "vldrepl.w $vr3, %[scale], 0\n\t"
      "vldrepl.w $vr5, %[bias], 0\n\t"
      "vldi $vr4, 0\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vilvl.h $vr1, $vr4, $vr0\n\t"
      "vilvh.h $vr2, $vr4, $vr0\n\t"
      "vffint.s.wu $vr1, $vr1\n\t"
      "vffint.s.wu $vr2, $vr2\n\t"
      "vfmul.s $vr1, $vr1, $vr3\n\t"
      "vfmul.s $vr2, $vr2, $vr3\n\t"
      "vfmul.s $vr1, $vr1, $vr5\n\t"
      "vfmul.s $vr2, $vr2, $vr5\n\t"
      "vsrlni.h.w $vr2, $vr1, 13\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
      : [scale] "r"(&scale), [bias] "r"(&bias)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_HALFFLOATROW_LSX

#ifdef HAS_ARGBTOYMATRIXROW_LSX
void ARGBToYMatrixRow_LSX(const uint8_t* src_argb,
                          uint8_t* dst_y,
                          int width,
                          const struct ArgbConstants* c) {
  asm volatile(
      "vldrepl.b $vr0, %[c], 0\n\t"
      "vldrepl.b $vr1, %[c], 1\n\t"
      "vldrepl.b $vr2, %[c], 2\n\t"
      "vldrepl.b $vr3, %[c], 3\n\t"
      "vldrepl.h $vr4, %[c], 96\n\t"
      "1:\n\t"
      "vld $vr5, %[src], 0\n\t"
      "vld $vr6, %[src], 16\n\t"
      "vor.v $vr9, $vr4, $vr4\n\t"
      "vpickev.b $vr7, $vr6, $vr5\n\t"
      "vpickod.b $vr8, $vr6, $vr5\n\t"
      "vmaddwev.h.bu $vr9, $vr7, $vr0\n\t"
      "vmaddwev.h.bu $vr9, $vr8, $vr1\n\t"
      "vmaddwod.h.bu $vr9, $vr7, $vr2\n\t"
      "vmaddwod.h.bu $vr9, $vr8, $vr3\n\t"
      "vpickod.b $vr9, $vr9, $vr9\n\t"
      "vstelm.d $vr9, %[dst], 0, 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 8\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [c] "r"(c)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9");
}
#endif  // HAS_ARGBTOYMATRIXROW_LSX

#ifdef HAS_RGBTOYMATRIXROW_LSX
void RGBToYMatrixRow_LSX(const uint8_t* src_rgba,
                         uint8_t* dst_y,
                         int width,
                         const struct ArgbConstants* c) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                    0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                    9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  asm volatile(
      "vldrepl.b $vr0, %[c], 0\n\t"
      "vldrepl.b $vr1, %[c], 1\n\t"
      "vldrepl.b $vr2, %[c], 2\n\t"
      "vldrepl.h $vr3, %[c], 96\n\t"
      "vld $vr4, %[shuf], 0\n\t"
      "vld $vr5, %[shuf], 16\n\t"
      "1:\n\t"
      "vld $vr6, %[src], 0\n\t"
      "vld $vr7, %[src], 12\n\t"
      "vld $vr8, %[src], 24\n\t"
      "vld $vr9, %[src], 32\n\t"
      "vshuf.b $vr6, $vr6, $vr6, $vr4\n\t"
      "vshuf.b $vr7, $vr7, $vr7, $vr4\n\t"
      "vshuf.b $vr8, $vr8, $vr8, $vr4\n\t"
      "vshuf.b $vr9, $vr9, $vr9, $vr5\n\t"
      "vor.v $vr12, $vr3, $vr3\n\t"
      "vor.v $vr13, $vr3, $vr3\n\t"
      "vpickev.b $vr10, $vr7, $vr6\n\t"
      "vpickod.b $vr11, $vr7, $vr6\n\t"
      "vmaddwev.h.bu $vr12, $vr10, $vr0\n\t"
      "vmaddwev.h.bu $vr12, $vr11, $vr1\n\t"
      "vmaddwod.h.bu $vr12, $vr10, $vr2\n\t"
      "vpickev.b $vr10, $vr9, $vr8\n\t"
      "vpickod.b $vr11, $vr9, $vr8\n\t"
      "vmaddwev.h.bu $vr13, $vr10, $vr0\n\t"
      "vmaddwev.h.bu $vr13, $vr11, $vr1\n\t"
      "vmaddwod.h.bu $vr13, $vr10, $vr2\n\t"
      "vpickod.b $vr12, $vr13, $vr12\n\t"
      "vst $vr12, %[dst], 0\n\t"
      "addi.d %[src], %[src], 48\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_rgba), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [c] "r"(c), [shuf] "r"(kShuf)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13");
}
#endif  // HAS_RGBTOYMATRIXROW_LSX

#ifdef HAS_RGB565TOYROW_LSX
void RGB565ToYRow_LSX(const uint8_t* src_rgb565, uint8_t* dst_y, int width) {
  asm volatile(
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "vldi $vr7, 63\n\t"
      "vsrli.h $vr7, $vr7, 8\n\t"
      "vldi $vr8, 25\n\t"
      "vsrli.h $vr8, $vr8, 8\n\t"
      "vldi $vr9, 129\n\t"
      "vsrli.h $vr9, $vr9, 8\n\t"
      "vldi $vr10, 66\n\t"
      "vsrli.h $vr10, $vr10, 8\n\t"
      "vldi $vr11, 33\n\t"
      "vsrli.h $vr11, $vr11, 8\n\t"
      "vslli.h $vr11, $vr11, 7\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr7\n\t"
      "vsrli.h $vr3, $vr0, 11\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 2\n\t"
      "vsrli.h $vr2, $vr2, 4\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vmul.h $vr1, $vr1, $vr8\n\t"
      "vmadd.h $vr1, $vr2, $vr9\n\t"
      "vmadd.h $vr1, $vr3, $vr10\n\t"
      "vadd.h $vr1, $vr1, $vr11\n\t"
      "vpickod.b $vr1, $vr1, $vr1\n\t"
      "vstelm.d $vr1, %[dst], 0, 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 8\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_rgb565), [dst] "+r"(dst_y), [cnt] "+r"(width)
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
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "vldi $vr8, 25\n\t"
      "vsrli.h $vr8, $vr8, 8\n\t"
      "vldi $vr9, 129\n\t"
      "vsrli.h $vr9, $vr9, 8\n\t"
      "vldi $vr10, 66\n\t"
      "vsrli.h $vr10, $vr10, 8\n\t"
      "vldi $vr11, 33\n\t"
      "vsrli.h $vr11, $vr11, 8\n\t"
      "vslli.h $vr11, $vr11, 7\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vsrli.h $vr3, $vr0, 10\n\t"
      "vand.v $vr3, $vr3, $vr6\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 3\n\t"
      "vsrli.h $vr2, $vr2, 2\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vmul.h $vr1, $vr1, $vr8\n\t"
      "vmadd.h $vr1, $vr2, $vr9\n\t"
      "vmadd.h $vr1, $vr3, $vr10\n\t"
      "vadd.h $vr1, $vr1, $vr11\n\t"
      "vpickod.b $vr1, $vr1, $vr1\n\t"
      "vstelm.d $vr1, %[dst], 0, 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 8\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb1555), [dst] "+r"(dst_y), [cnt] "+r"(width)
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
      "vldi $vr20, 112\n\t"
      "vsrli.h $vr20, $vr20, 8\n\t"
      "vldi $vr21, 74\n\t"
      "vsrli.h $vr21, $vr21, 8\n\t"
      "vldi $vr22, 38\n\t"
      "vsrli.h $vr22, $vr22, 8\n\t"
      "vldi $vr23, 94\n\t"
      "vsrli.h $vr23, $vr23, 8\n\t"
      "vldi $vr24, 18\n\t"
      "vsrli.h $vr24, $vr24, 8\n\t"
      "vldi $vr25, 128\n\t"
      "vsrli.h $vr25, $vr25, 8\n\t"
      "vslli.h $vr25, $vr25, 8\n\t"
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "vldi $vr7, 63\n\t"
      "vsrli.h $vr7, $vr7, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr7\n\t"
      "vsrli.h $vr3, $vr0, 11\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 2\n\t"
      "vsrli.h $vr2, $vr2, 4\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vor.v $vr10, $vr1, $vr1\n\t"
      "vor.v $vr11, $vr2, $vr2\n\t"
      "vor.v $vr12, $vr3, $vr3\n\t"
      "vld $vr0, %[s0], 16\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr7\n\t"
      "vsrli.h $vr3, $vr0, 11\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 2\n\t"
      "vsrli.h $vr2, $vr2, 4\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vpickev.b $vr10, $vr1, $vr10\n\t"
      "vpickev.b $vr11, $vr2, $vr11\n\t"
      "vpickev.b $vr12, $vr3, $vr12\n\t"
      "vld $vr0, %[s1], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr7\n\t"
      "vsrli.h $vr3, $vr0, 11\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 2\n\t"
      "vsrli.h $vr2, $vr2, 4\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vor.v $vr13, $vr1, $vr1\n\t"
      "vor.v $vr14, $vr2, $vr2\n\t"
      "vor.v $vr15, $vr3, $vr3\n\t"
      "vld $vr0, %[s1], 16\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr7\n\t"
      "vsrli.h $vr3, $vr0, 11\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 2\n\t"
      "vsrli.h $vr2, $vr2, 4\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vpickev.b $vr5, $vr3, $vr15\n\t"
      "vpickev.b $vr4, $vr2, $vr14\n\t"
      "vpickev.b $vr3, $vr1, $vr13\n\t"
      "vor.v $vr0, $vr10, $vr10\n\t"
      "vor.v $vr1, $vr11, $vr11\n\t"
      "vor.v $vr2, $vr12, $vr12\n\t"
      "vhaddw.hu.bu $vr0, $vr0, $vr0\n\t"
      "vhaddw.hu.bu $vr3, $vr3, $vr3\n\t"
      "vadd.h $vr0, $vr0, $vr3\n\t"
      "vsrari.h $vr0, $vr0, 2\n\t"
      "vhaddw.hu.bu $vr1, $vr1, $vr1\n\t"
      "vhaddw.hu.bu $vr4, $vr4, $vr4\n\t"
      "vadd.h $vr1, $vr1, $vr4\n\t"
      "vsrari.h $vr1, $vr1, 2\n\t"
      "vhaddw.hu.bu $vr2, $vr2, $vr2\n\t"
      "vhaddw.hu.bu $vr5, $vr5, $vr5\n\t"
      "vadd.h $vr2, $vr2, $vr5\n\t"
      "vsrari.h $vr2, $vr2, 2\n\t"
      "vor.v $vr8, $vr25, $vr25\n\t"
      "vor.v $vr9, $vr25, $vr25\n\t"
      "vmadd.h $vr8, $vr0, $vr20\n\t"
      "vmsub.h $vr8, $vr1, $vr21\n\t"
      "vmsub.h $vr8, $vr2, $vr22\n\t"
      "vmadd.h $vr9, $vr2, $vr20\n\t"
      "vmsub.h $vr9, $vr1, $vr23\n\t"
      "vmsub.h $vr9, $vr0, $vr24\n\t"
      "vpickod.b $vr8, $vr9, $vr8\n\t"
      "vstelm.d $vr8, %[du], 0, 0\n\t"
      "vstelm.d $vr8, %[dv], 0, 1\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[du], %[du], 8\n\t"
      "addi.d %[dv], %[dv], 8\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_rgb565), [s1] "+r"(next_rgb565), [du] "+r"(dst_u),
        [dv] "+r"(dst_v), [cnt] "+r"(width)
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
      "vldi $vr20, 112\n\t"
      "vsrli.h $vr20, $vr20, 8\n\t"
      "vldi $vr21, 74\n\t"
      "vsrli.h $vr21, $vr21, 8\n\t"
      "vldi $vr22, 38\n\t"
      "vsrli.h $vr22, $vr22, 8\n\t"
      "vldi $vr23, 94\n\t"
      "vsrli.h $vr23, $vr23, 8\n\t"
      "vldi $vr24, 18\n\t"
      "vsrli.h $vr24, $vr24, 8\n\t"
      "vldi $vr25, 128\n\t"
      "vsrli.h $vr25, $vr25, 8\n\t"
      "vslli.h $vr25, $vr25, 8\n\t"
      "vldi $vr6, 31\n\t"
      "vsrli.h $vr6, $vr6, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vsrli.h $vr3, $vr0, 10\n\t"
      "vand.v $vr3, $vr3, $vr6\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 3\n\t"
      "vsrli.h $vr2, $vr2, 2\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vor.v $vr10, $vr1, $vr1\n\t"
      "vor.v $vr11, $vr2, $vr2\n\t"
      "vor.v $vr12, $vr3, $vr3\n\t"
      "vld $vr0, %[s0], 16\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vsrli.h $vr3, $vr0, 10\n\t"
      "vand.v $vr3, $vr3, $vr6\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 3\n\t"
      "vsrli.h $vr2, $vr2, 2\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vpickev.b $vr10, $vr1, $vr10\n\t"
      "vpickev.b $vr11, $vr2, $vr11\n\t"
      "vpickev.b $vr12, $vr3, $vr12\n\t"
      "vld $vr0, %[s1], 0\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vsrli.h $vr3, $vr0, 10\n\t"
      "vand.v $vr3, $vr3, $vr6\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 3\n\t"
      "vsrli.h $vr2, $vr2, 2\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vor.v $vr13, $vr1, $vr1\n\t"
      "vor.v $vr14, $vr2, $vr2\n\t"
      "vor.v $vr15, $vr3, $vr3\n\t"
      "vld $vr0, %[s1], 16\n\t"
      "vand.v $vr1, $vr0, $vr6\n\t"
      "vsrli.h $vr2, $vr0, 5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vsrli.h $vr3, $vr0, 10\n\t"
      "vand.v $vr3, $vr3, $vr6\n\t"
      "vslli.h $vr4, $vr1, 3\n\t"
      "vsrli.h $vr1, $vr1, 2\n\t"
      "vor.v $vr1, $vr4, $vr1\n\t"
      "vslli.h $vr4, $vr2, 3\n\t"
      "vsrli.h $vr2, $vr2, 2\n\t"
      "vor.v $vr2, $vr4, $vr2\n\t"
      "vslli.h $vr4, $vr3, 3\n\t"
      "vsrli.h $vr3, $vr3, 2\n\t"
      "vor.v $vr3, $vr4, $vr3\n\t"
      "vpickev.b $vr5, $vr3, $vr15\n\t"
      "vpickev.b $vr4, $vr2, $vr14\n\t"
      "vpickev.b $vr3, $vr1, $vr13\n\t"
      "vor.v $vr0, $vr10, $vr10\n\t"
      "vor.v $vr1, $vr11, $vr11\n\t"
      "vor.v $vr2, $vr12, $vr12\n\t"
      "vhaddw.hu.bu $vr0, $vr0, $vr0\n\t"
      "vhaddw.hu.bu $vr3, $vr3, $vr3\n\t"
      "vadd.h $vr0, $vr0, $vr3\n\t"
      "vsrari.h $vr0, $vr0, 2\n\t"
      "vhaddw.hu.bu $vr1, $vr1, $vr1\n\t"
      "vhaddw.hu.bu $vr4, $vr4, $vr4\n\t"
      "vadd.h $vr1, $vr1, $vr4\n\t"
      "vsrari.h $vr1, $vr1, 2\n\t"
      "vhaddw.hu.bu $vr2, $vr2, $vr2\n\t"
      "vhaddw.hu.bu $vr5, $vr5, $vr5\n\t"
      "vadd.h $vr2, $vr2, $vr5\n\t"
      "vsrari.h $vr2, $vr2, 2\n\t"
      "vor.v $vr8, $vr25, $vr25\n\t"
      "vor.v $vr9, $vr25, $vr25\n\t"
      "vmadd.h $vr8, $vr0, $vr20\n\t"
      "vmsub.h $vr8, $vr1, $vr21\n\t"
      "vmsub.h $vr8, $vr2, $vr22\n\t"
      "vmadd.h $vr9, $vr2, $vr20\n\t"
      "vmsub.h $vr9, $vr1, $vr23\n\t"
      "vmsub.h $vr9, $vr0, $vr24\n\t"
      "vpickod.b $vr8, $vr9, $vr8\n\t"
      "vstelm.d $vr8, %[du], 0, 0\n\t"
      "vstelm.d $vr8, %[dv], 0, 1\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[du], %[du], 8\n\t"
      "addi.d %[dv], %[dv], 8\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb1555), [s1] "+r"(next_argb1555), [du] "+r"(dst_u),
        [dv] "+r"(dst_v), [cnt] "+r"(width)
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
      "vldi $vr20, 112\n\t"
      "vsrli.h $vr20, $vr20, 8\n\t"
      "vldi $vr21, 74\n\t"
      "vsrli.h $vr21, $vr21, 8\n\t"
      "vldi $vr22, 38\n\t"
      "vsrli.h $vr22, $vr22, 8\n\t"
      "vldi $vr23, 94\n\t"
      "vsrli.h $vr23, $vr23, 8\n\t"
      "vldi $vr24, 18\n\t"
      "vsrli.h $vr24, $vr24, 8\n\t"
      "vldi $vr25, 128\n\t"
      "vsrli.h $vr25, $vr25, 8\n\t"
      "vslli.h $vr25, $vr25, 8\n\t"
      "vld $vr16, %[shuf], 0\n\t"
      "vld $vr17, %[shuf], 16\n\t"
      "1:\n\t"
      "vld $vr8, %[s0], 0\n\t"
      "vld $vr9, %[s0], 12\n\t"
      "vld $vr10, %[s0], 24\n\t"
      "vld $vr11, %[s0], 32\n\t"
      "vshuf.b $vr8, $vr8, $vr8, $vr16\n\t"
      "vshuf.b $vr9, $vr9, $vr9, $vr16\n\t"
      "vshuf.b $vr10, $vr10, $vr10, $vr16\n\t"
      "vshuf.b $vr11, $vr11, $vr11, $vr17\n\t"
      "vpickev.b $vr12, $vr9, $vr8\n\t"
      "vpickod.b $vr13, $vr9, $vr8\n\t"
      "vpickev.b $vr14, $vr11, $vr10\n\t"
      "vpickod.b $vr15, $vr11, $vr10\n\t"
      "vpickev.b $vr0, $vr14, $vr12\n\t"
      "vpickod.b $vr2, $vr14, $vr12\n\t"
      "vpickev.b $vr1, $vr15, $vr13\n\t"
      "vld $vr8, %[s1], 0\n\t"
      "vld $vr9, %[s1], 12\n\t"
      "vld $vr10, %[s1], 24\n\t"
      "vld $vr11, %[s1], 32\n\t"
      "vshuf.b $vr8, $vr8, $vr8, $vr16\n\t"
      "vshuf.b $vr9, $vr9, $vr9, $vr16\n\t"
      "vshuf.b $vr10, $vr10, $vr10, $vr16\n\t"
      "vshuf.b $vr11, $vr11, $vr11, $vr17\n\t"
      "vpickev.b $vr12, $vr9, $vr8\n\t"
      "vpickod.b $vr13, $vr9, $vr8\n\t"
      "vpickev.b $vr14, $vr11, $vr10\n\t"
      "vpickod.b $vr15, $vr11, $vr10\n\t"
      "vpickev.b $vr3, $vr14, $vr12\n\t"
      "vpickod.b $vr5, $vr14, $vr12\n\t"
      "vpickev.b $vr4, $vr15, $vr13\n\t"
      "vhaddw.hu.bu $vr0, $vr0, $vr0\n\t"
      "vhaddw.hu.bu $vr3, $vr3, $vr3\n\t"
      "vadd.h $vr0, $vr0, $vr3\n\t"
      "vsrari.h $vr0, $vr0, 2\n\t"
      "vhaddw.hu.bu $vr1, $vr1, $vr1\n\t"
      "vhaddw.hu.bu $vr4, $vr4, $vr4\n\t"
      "vadd.h $vr1, $vr1, $vr4\n\t"
      "vsrari.h $vr1, $vr1, 2\n\t"
      "vhaddw.hu.bu $vr2, $vr2, $vr2\n\t"
      "vhaddw.hu.bu $vr5, $vr5, $vr5\n\t"
      "vadd.h $vr2, $vr2, $vr5\n\t"
      "vsrari.h $vr2, $vr2, 2\n\t"
      "vor.v $vr6, $vr25, $vr25\n\t"
      "vor.v $vr7, $vr25, $vr25\n\t"
      "vmadd.h $vr6, $vr0, $vr20\n\t"
      "vmsub.h $vr6, $vr1, $vr21\n\t"
      "vmsub.h $vr6, $vr2, $vr22\n\t"
      "vmadd.h $vr7, $vr2, $vr20\n\t"
      "vmsub.h $vr7, $vr1, $vr23\n\t"
      "vmsub.h $vr7, $vr0, $vr24\n\t"
      "vpickod.b $vr6, $vr7, $vr6\n\t"
      "vstelm.d $vr6, %[du], 0, 0\n\t"
      "vstelm.d $vr6, %[dv], 0, 1\n\t"
      "addi.d %[s0], %[s0], 48\n\t"
      "addi.d %[s1], %[s1], 48\n\t"
      "addi.d %[du], %[du], 8\n\t"
      "addi.d %[dv], %[dv], 8\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src), [s1] "+r"(next), [du] "+r"(dst_u), [dv] "+r"(dst_v),
        [cnt] "+r"(width)
      : [shuf] "r"(shuf)
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
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vshuf4i.b $vr0, $vr0, 0xb1\n\t"
      "vshuf4i.b $vr1, $vr1, 0xb1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uv), [dst] "+r"(dst_vu), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_SWAPUVROW_LSX

#ifdef HAS_ARGBCOPYYTOALPHAROW_LSX
void ARGBCopyYToAlphaRow_LSX(const uint8_t* src, uint8_t* dst, int width) {
  asm volatile(
      "vldi $vr4, 0xff\n\t"
      "vslli.w $vr4, $vr4, 24\n\t"
      "1:\n\t"
      "vldrepl.d $vr0, %[src], 0\n\t"
      "vld $vr1, %[dst], 0\n\t"
      "vld $vr2, %[dst], 16\n\t"
      "vilvl.b $vr0, $vr0, $vr0\n\t"
      "vilvh.h $vr3, $vr0, $vr0\n\t"
      "vilvl.h $vr0, $vr0, $vr0\n\t"
      "vslli.w $vr0, $vr0, 24\n\t"
      "vslli.w $vr3, $vr3, 24\n\t"
      "vandn.v $vr1, $vr4, $vr1\n\t"
      "vandn.v $vr2, $vr4, $vr2\n\t"
      "vor.v $vr1, $vr1, $vr0\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vst $vr1, %[dst], 0\n\t"
      "vst $vr2, %[dst], 16\n\t"
      "addi.d %[src], %[src], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[su], 0\n\t"
      "vld $vr1, %[sv], 0\n\t"
      "vldx $vr2, %[su], %[stu]\n\t"
      "vldx $vr3, %[sv], %[stv]\n\t"
      "vhaddw.hu.bu $vr0, $vr0, $vr0\n\t"
      "vhaddw.hu.bu $vr1, $vr1, $vr1\n\t"
      "vhaddw.hu.bu $vr2, $vr2, $vr2\n\t"
      "vhaddw.hu.bu $vr3, $vr3, $vr3\n\t"
      "vadd.h $vr0, $vr0, $vr2\n\t"
      "vadd.h $vr1, $vr1, $vr3\n\t"
      "vsrari.h $vr0, $vr0, 2\n\t"
      "vsrari.h $vr1, $vr1, 2\n\t"
      "vpackev.b $vr0, $vr1, $vr0\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[su], %[su], 16\n\t"
      "addi.d %[sv], %[sv], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [su] "+r"(src_u), [sv] "+r"(src_v), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      : [stu] "r"((ptrdiff_t)src_stride_u), [stv] "r"((ptrdiff_t)src_stride_v)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_HALFMERGEUVROW_LSX

#ifdef HAS_CONVERT16TO8ROW_LSX
void Convert16To8Row_LSX(const uint16_t* src_y,
                         uint8_t* dst_y,
                         int scale,
                         int width) {
  asm volatile(
      "vreplgr2vr.h $vr2, %[scl]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vmuh.hu $vr0, $vr0, $vr2\n\t"
      "vmuh.hu $vr1, $vr1, $vr2\n\t"
      "vssrani.bu.h $vr1, $vr0, 0\n\t"
      "vst $vr1, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [scl] "r"(scale)
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
      "vreplgr2vr.h $vr3, %[shf]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vilvl.b $vr1, $vr0, $vr0\n\t"
      "vilvh.b $vr2, $vr0, $vr0\n\t"
      "vsrl.h $vr1, $vr1, $vr3\n\t"
      "vsrl.h $vr2, $vr2, $vr3\n\t"
      "vst $vr1, %[dst], 0\n\t"
      "vst $vr2, %[dst], 16\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [shf] "r"(shift)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_CONVERT8TO16ROW_LSX

#ifdef HAS_MULTIPLYROW_16_LSX
void MultiplyRow_16_LSX(const uint16_t* src_y,
                        uint16_t* dst_y,
                        int scale,
                        int width) {
  asm volatile(
      "vreplgr2vr.h $vr2, %[scl]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vmul.h $vr0, $vr0, $vr2\n\t"
      "vmul.h $vr1, $vr1, $vr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [scl] "r"(scale)
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
      "vreplgr2vr.h $vr2, %[scl]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vmuh.hu $vr0, $vr0, $vr2\n\t"
      "vmuh.hu $vr1, $vr1, $vr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [scl] "r"(scale)
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
      "vreplgr2vr.h $vr4, %[shf]\n\t"
      "1:\n\t"
      "vld $vr0, %[su], 0\n\t"
      "vld $vr1, %[sv], 0\n\t"
      "vsll.h $vr0, $vr0, $vr4\n\t"
      "vsll.h $vr1, $vr1, $vr4\n\t"
      "vilvl.h $vr2, $vr1, $vr0\n\t"
      "vilvh.h $vr3, $vr1, $vr0\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "vst $vr3, %[dst], 16\n\t"
      "addi.d %[su], %[su], 16\n\t"
      "addi.d %[sv], %[sv], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [su] "+r"(src_u), [sv] "+r"(src_v), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      : [shf] "r"(shift)
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
      "vreplgr2vr.h $vr4, %[shf]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickev.h $vr2, $vr1, $vr0\n\t"
      "vpickod.h $vr3, $vr1, $vr0\n\t"
      "vsrl.h $vr2, $vr2, $vr4\n\t"
      "vsrl.h $vr3, $vr3, $vr4\n\t"
      "vst $vr2, %[du], 0\n\t"
      "vst $vr3, %[dv], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uv), [du] "+r"(dst_u), [dv] "+r"(dst_v),
        [cnt] "+r"(width)
      : [shf] "r"(shift)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
}
#endif  // HAS_SPLITUVROW_16_LSX

#ifdef HAS_COPYROW_16_LSX
void CopyRow_16_LSX(const uint16_t* src, uint16_t* dst, int count) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(count)
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
      "1:\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vld $vr1, %[src1], 0\n\t"
      "vavgr.bu $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 16\n\t"
      "addi.d %[src1], %[src1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_uv), [src1] "+r"(src_uv1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vld $vr1, %[src1], 0\n\t"
      "vavgr.hu $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 16\n\t"
      "addi.d %[src1], %[src1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_uv), [src1] "+r"(src_uv1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_HALFROW_16_LSX

#ifdef HAS_ARGBCOPYALPHAROW_LSX
void ARGBCopyAlphaRow_LSX(const uint8_t* src, uint8_t* dst, int width) {
  asm volatile(
      "vldi $vr4, 0xff\n\t"
      "vslli.w $vr4, $vr4, 24\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr2, %[dst], 0\n\t"
      "vld $vr3, %[dst], 16\n\t"
      "vbitsel.v $vr2, $vr2, $vr0, $vr4\n\t"
      "vbitsel.v $vr3, $vr3, $vr1, $vr4\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "vst $vr3, %[dst], 16\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vld $vr1, %[src0], 16\n\t"
      "vld $vr2, %[src1], 0\n\t"
      "vld $vr3, %[src1], 16\n\t"
      "vpickod.b $vr0, $vr1, $vr0\n\t"
      "vpickod.b $vr2, $vr3, $vr2\n\t"
      "vavgr.bu $vr0, $vr0, $vr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 32\n\t"
      "addi.d %[src1], %[src1], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_yuy2), [src1] "+r"(src_yuy2_1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_YUY2TONVUVROW_LSX

#ifdef HAS_AR64TOARGBROW_LSX
void AR64ToARGBRow_LSX(const uint16_t* src_ar64,
                       uint8_t* dst_argb,
                       int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickod.b $vr0, $vr1, $vr0\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ar64), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_AR64TOARGBROW_LSX

#ifdef HAS_AB64TOARGBROW_LSX
void AB64ToARGBRow_LSX(const uint16_t* src_ab64,
                       uint8_t* dst_argb,
                       int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vpickod.b $vr0, $vr1, $vr0\n\t"
      "vshuf4i.b $vr0, $vr0, 0xc6\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ab64), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1");
}
#endif  // HAS_AB64TOARGBROW_LSX

#ifdef HAS_ARGBTOAR64ROW_LSX
void ARGBToAR64Row_LSX(const uint8_t* src_argb,
                       uint16_t* dst_ar64,
                       int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vilvl.b $vr1, $vr0, $vr0\n\t"
      "vilvh.b $vr2, $vr0, $vr0\n\t"
      "vst $vr1, %[dst], 0\n\t"
      "vst $vr2, %[dst], 16\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_ar64), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2");
}
#endif  // HAS_ARGBTOAR64ROW_LSX

#ifdef HAS_ARGBTOAB64ROW_LSX
void ARGBToAB64Row_LSX(const uint8_t* src_argb,
                       uint16_t* dst_ab64,
                       int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vshuf4i.b $vr0, $vr0, 0xc6\n\t"
      "vilvl.b $vr1, $vr0, $vr0\n\t"
      "vilvh.b $vr2, $vr0, $vr0\n\t"
      "vst $vr1, %[dst], 0\n\t"
      "vst $vr2, %[dst], 16\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_ab64), [cnt] "+r"(width)
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
      "vreplgr2vr.b $vr2, %[scl]\n\t"
      "vreplgr2vr.b $vr3, %[bia]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vmuh.bu $vr0, $vr0, $vr2\n\t"
      "vadd.b $vr0, $vr0, $vr3\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [scl] "r"(scale), [bia] "r"(bias)
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
      "vreplgr2vr.h $vr4, %[scl]\n\t"
      "1:\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vld $vr1, %[src0], 16\n\t"
      "vld $vr2, %[src1], 0\n\t"
      "vld $vr3, %[src1], 16\n\t"
      "vavgr.hu $vr0, $vr0, $vr2\n\t"
      "vavgr.hu $vr1, $vr1, $vr3\n\t"
      "vmuh.hu $vr0, $vr0, $vr4\n\t"
      "vmuh.hu $vr1, $vr1, $vr4\n\t"
      "vssrani.bu.h $vr1, $vr0, 0\n\t"
      "vst $vr1, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 32\n\t"
      "addi.d %[src1], %[src1], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_uv), [src1] "+r"(src_uv1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      : [scl] "r"(scale)
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
      "vreplgr2vr.w $vr3, %[m_ag]\n\t"
      "vreplgr2vr.w $vr4, %[m_r]\n\t"
      "vreplgr2vr.w $vr5, %[m_b]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vsrli.w $vr1, $vr0, 20\n\t"
      "vslli.w $vr2, $vr0, 20\n\t"
      "vand.v $vr0, $vr0, $vr3\n\t"
      "vand.v $vr1, $vr1, $vr5\n\t"
      "vand.v $vr2, $vr2, $vr4\n\t"
      "vor.v $vr0, $vr0, $vr1\n\t"
      "vor.v $vr0, $vr0, $vr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ar30), [dst] "+r"(dst_ab30), [cnt] "+r"(width)
      : [m_ag] "r"(m_ag), [m_r] "r"(m_r), [m_b] "r"(m_b)
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
      "vreplgr2vr.w $vr4, %[m_a]\n\t"
      "vreplgr2vr.w $vr5, %[m_r]\n\t"
      "vreplgr2vr.w $vr6, %[m_g]\n\t"
      "vreplgr2vr.w $vr7, %[m_b]\n\t"
      "vreplgr2vr.w $vr8, %[m_l]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vslli.w $vr1, $vr0, 6\n\t"
      "vslli.w $vr2, $vr0, 4\n\t"
      "vslli.w $vr3, $vr0, 2\n\t"
      "vand.v $vr0, $vr0, $vr4\n\t"
      "vand.v $vr1, $vr1, $vr5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vand.v $vr3, $vr3, $vr7\n\t"
      "vor.v $vr0, $vr0, $vr1\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vor.v $vr0, $vr0, $vr2\n\t"
      "vsrli.w $vr1, $vr0, 8\n\t"
      "vand.v $vr1, $vr1, $vr8\n\t"
      "vor.v $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_ar30), [cnt] "+r"(width)
      : [m_a] "r"(m_a), [m_r] "r"(m_r), [m_g] "r"(m_g), [m_b] "r"(m_b),
        [m_l] "r"(m_l)
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
      "vreplgr2vr.w $vr4, %[m_a]\n\t"
      "vreplgr2vr.w $vr5, %[m_r]\n\t"
      "vreplgr2vr.w $vr6, %[m_g]\n\t"
      "vreplgr2vr.w $vr7, %[m_b]\n\t"
      "vreplgr2vr.w $vr8, %[m_l]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vshuf4i.b $vr0, $vr0, 0xc6\n\t"
      "vslli.w $vr1, $vr0, 6\n\t"
      "vslli.w $vr2, $vr0, 4\n\t"
      "vslli.w $vr3, $vr0, 2\n\t"
      "vand.v $vr0, $vr0, $vr4\n\t"
      "vand.v $vr1, $vr1, $vr5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vand.v $vr3, $vr3, $vr7\n\t"
      "vor.v $vr0, $vr0, $vr1\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vor.v $vr0, $vr0, $vr2\n\t"
      "vsrli.w $vr1, $vr0, 8\n\t"
      "vand.v $vr1, $vr1, $vr8\n\t"
      "vor.v $vr0, $vr0, $vr1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_abgr), [dst] "+r"(dst_ar30), [cnt] "+r"(width)
      : [m_a] "r"(m_a), [m_r] "r"(m_r), [m_g] "r"(m_g), [m_b] "r"(m_b),
        [m_l] "r"(m_l)
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
      "vreplgr2vr.w $vr4, %[m_a]\n\t"
      "vreplgr2vr.w $vr5, %[m_2]\n\t"
      "vreplgr2vr.w $vr6, %[m_4]\n\t"
      "vreplgr2vr.w $vr7, %[m_6]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vsrli.w $vr1, $vr0, 2\n\t"
      "vsrli.w $vr2, $vr0, 4\n\t"
      "vsrli.w $vr3, $vr0, 6\n\t"
      "vand.v $vr0, $vr0, $vr4\n\t"
      "vand.v $vr1, $vr1, $vr5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vand.v $vr3, $vr3, $vr7\n\t"
      "vor.v $vr0, $vr0, $vr1\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vor.v $vr0, $vr0, $vr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ar30), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [m_a] "r"(m_a), [m_2] "r"(m_2), [m_4] "r"(m_4), [m_6] "r"(m_6)
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
      "vreplgr2vr.w $vr4, %[m_a]\n\t"
      "vreplgr2vr.w $vr5, %[m_2]\n\t"
      "vreplgr2vr.w $vr6, %[m_4]\n\t"
      "vreplgr2vr.w $vr7, %[m_6]\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vsrli.w $vr1, $vr0, 2\n\t"
      "vsrli.w $vr2, $vr0, 4\n\t"
      "vsrli.w $vr3, $vr0, 6\n\t"
      "vand.v $vr0, $vr0, $vr4\n\t"
      "vand.v $vr1, $vr1, $vr5\n\t"
      "vand.v $vr2, $vr2, $vr6\n\t"
      "vand.v $vr3, $vr3, $vr7\n\t"
      "vor.v $vr0, $vr0, $vr1\n\t"
      "vor.v $vr2, $vr2, $vr3\n\t"
      "vor.v $vr0, $vr0, $vr2\n\t"
      "vshuf4i.b $vr0, $vr0, 0xc6\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -4\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ar30), [dst] "+r"(dst_abgr), [cnt] "+r"(width)
      : [m_a] "r"(m_a), [m_2] "r"(m_2), [m_4] "r"(m_4), [m_6] "r"(m_6)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_AR30TOABGRROW_LSX

#ifdef HAS_AYUVTOYROW_LSX
void AYUVToYRow_LSX(const uint8_t* src_ayuv, uint8_t* dst_y, int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr2, %[src], 32\n\t"
      "vld $vr3, %[src], 48\n\t"
      "vpickev.h $vr0, $vr1, $vr0\n\t"
      "vpickev.h $vr2, $vr3, $vr2\n\t"
      "vpickev.b $vr0, $vr2, $vr0\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ayuv), [dst] "+r"(dst_y), [cnt] "+r"(width)
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
      "vldi $vr6, 0x02\n\t"
      "1:\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vld $vr1, %[src0], 16\n\t"
      "vld $vr2, %[src0], 32\n\t"
      "vld $vr3, %[src0], 48\n\t"
      "vld $vr4, %[src1], 0\n\t"
      "vld $vr5, %[src1], 16\n\t"
      "vaddwev.h.bu $vr0, $vr0, $vr4\n\t"
      "vaddwod.h.bu $vr4, $vr1, $vr5\n\t"
      "vld $vr1, %[src1], 32\n\t"
      "vld $vr5, %[src1], 48\n\t"
      "vaddwev.h.bu $vr2, $vr2, $vr1\n\t"
      "vaddwod.h.bu $vr3, $vr3, $vr5\n\t"
      "vadd.h $vr0, $vr0, $vr4\n\t"
      "vadd.h $vr2, $vr2, $vr3\n\t"
      "vadd.h $vr0, $vr0, $vr6\n\t"
      "vadd.h $vr2, $vr2, $vr6\n\t"
      "vsrli.h $vr0, $vr0, 2\n\t"
      "vsrli.h $vr2, $vr2, 2\n\t"
      "vpickev.b $vr0, $vr2, $vr0\n\t"
      "vshuf4i.b $vr0, $vr0, 0xb1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 64\n\t"
      "addi.d %[src1], %[src1], 64\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_ayuv), [src1] "+r"(src_ayuv1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_AYUVTOUVROW_LSX

#ifdef HAS_AYUVTOVUROW_LSX
void AYUVToVURow_LSX(const uint8_t* src_ayuv,
                     int src_stride_ayuv,
                     uint8_t* dst_vu,
                     int width) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  asm volatile(
      "vldi $vr6, 0x02\n\t"
      "1:\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vld $vr1, %[src0], 16\n\t"
      "vld $vr2, %[src0], 32\n\t"
      "vld $vr3, %[src0], 48\n\t"
      "vld $vr4, %[src1], 0\n\t"
      "vld $vr5, %[src1], 16\n\t"
      "vaddwev.h.bu $vr0, $vr0, $vr4\n\t"
      "vaddwod.h.bu $vr4, $vr1, $vr5\n\t"
      "vld $vr1, %[src1], 32\n\t"
      "vld $vr5, %[src1], 48\n\t"
      "vaddwev.h.bu $vr2, $vr2, $vr1\n\t"
      "vaddwod.h.bu $vr3, $vr3, $vr5\n\t"
      "vadd.h $vr0, $vr0, $vr4\n\t"
      "vadd.h $vr2, $vr2, $vr3\n\t"
      "vadd.h $vr0, $vr0, $vr6\n\t"
      "vadd.h $vr2, $vr2, $vr6\n\t"
      "vsrli.h $vr0, $vr0, 2\n\t"
      "vsrli.h $vr2, $vr2, 2\n\t"
      "vpickev.b $vr0, $vr2, $vr0\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 64\n\t"
      "addi.d %[src1], %[src1], 64\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_ayuv), [src1] "+r"(src_ayuv1), [dst] "+r"(dst_vu),
        [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_AYUVTOVUROW_LSX

#ifdef HAS_DETILEROW_LSX
void DetileRow_LSX(const uint8_t* src,
                   ptrdiff_t src_tile_stride,
                   uint8_t* dst,
                   int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "add.d %[src], %[src], %[str]\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
      : [str] "r"(src_tile_stride)
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
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "add.d %[src], %[src], %[str]\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
      : [str] "r"(byte_stride)
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
      "1:\n\t"
      "vld $vr0, %[sb], 0\n\t"
      "vld $vr1, %[sg], 0\n\t"
      "vld $vr2, %[sr], 0\n\t"
      "vld $vr3, %[sa], 0\n\t"
      "vilvl.b $vr4, $vr1, $vr0\n\t"
      "vilvh.b $vr5, $vr1, $vr0\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvh.b $vr7, $vr3, $vr2\n\t"
      "vilvl.h $vr0, $vr6, $vr4\n\t"
      "vilvh.h $vr1, $vr6, $vr4\n\t"
      "vilvl.h $vr2, $vr7, $vr5\n\t"
      "vilvh.h $vr3, $vr7, $vr5\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "vst $vr2, %[dst], 32\n\t"
      "vst $vr3, %[dst], 48\n\t"
      "addi.d %[sb], %[sb], 16\n\t"
      "addi.d %[sg], %[sg], 16\n\t"
      "addi.d %[sr], %[sr], 16\n\t"
      "addi.d %[sa], %[sa], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sb] "+r"(src_b), [sg] "+r"(src_g), [sr] "+r"(src_r),
        [sa] "+r"(src_a), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "vldi $vr3, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sb], 0\n\t"
      "vld $vr1, %[sg], 0\n\t"
      "vld $vr2, %[sr], 0\n\t"
      "vilvl.b $vr4, $vr1, $vr0\n\t"
      "vilvh.b $vr5, $vr1, $vr0\n\t"
      "vilvl.b $vr6, $vr3, $vr2\n\t"
      "vilvh.b $vr7, $vr3, $vr2\n\t"
      "vilvl.h $vr0, $vr6, $vr4\n\t"
      "vilvh.h $vr1, $vr6, $vr4\n\t"
      "vilvl.h $vr2, $vr7, $vr5\n\t"
      "vilvh.h $vr4, $vr7, $vr5\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "vst $vr2, %[dst], 32\n\t"
      "vst $vr4, %[dst], 48\n\t"
      "addi.d %[sb], %[sb], 16\n\t"
      "addi.d %[sg], %[sg], 16\n\t"
      "addi.d %[sr], %[sr], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sb] "+r"(src_b), [sg] "+r"(src_g), [sr] "+r"(src_r),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr2, %[src], 32\n\t"
      "vld $vr3, %[src], 48\n\t"
      "vpickev.b $vr4, $vr1, $vr0\n\t"
      "vpickod.b $vr5, $vr1, $vr0\n\t"
      "vpickev.b $vr6, $vr3, $vr2\n\t"
      "vpickod.b $vr7, $vr3, $vr2\n\t"
      "vpickev.b $vr0, $vr6, $vr4\n\t"
      "vpickod.b $vr1, $vr6, $vr4\n\t"
      "vpickev.b $vr2, $vr7, $vr5\n\t"
      "vpickod.b $vr3, $vr7, $vr5\n\t"
      "vst $vr0, %[db], 0\n\t"
      "vst $vr2, %[dg], 0\n\t"
      "vst $vr1, %[dr], 0\n\t"
      "vst $vr3, %[da], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[db], %[db], 16\n\t"
      "addi.d %[dg], %[dg], 16\n\t"
      "addi.d %[dr], %[dr], 16\n\t"
      "addi.d %[da], %[da], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [db] "+r"(dst_b), [dg] "+r"(dst_g),
        [dr] "+r"(dst_r), [da] "+r"(dst_a), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr2, %[src], 32\n\t"
      "vld $vr3, %[src], 48\n\t"
      "vpickev.b $vr4, $vr1, $vr0\n\t"
      "vpickod.b $vr5, $vr1, $vr0\n\t"
      "vpickev.b $vr6, $vr3, $vr2\n\t"
      "vpickod.b $vr7, $vr3, $vr2\n\t"
      "vpickev.b $vr0, $vr6, $vr4\n\t"
      "vpickod.b $vr1, $vr6, $vr4\n\t"
      "vpickev.b $vr2, $vr7, $vr5\n\t"
      "vst $vr0, %[db], 0\n\t"
      "vst $vr2, %[dg], 0\n\t"
      "vst $vr1, %[dr], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[db], %[db], 16\n\t"
      "addi.d %[dg], %[dg], 16\n\t"
      "addi.d %[dr], %[dr], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [db] "+r"(dst_b), [dg] "+r"(dst_g),
        [dr] "+r"(dst_r), [cnt] "+r"(width)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_SPLITXRGBROW_LSX

#ifdef HAS_RGB24MIRRORROW_LSX
static const uvec8 kRGB24MirrorShuf0 = {13, 14, 15, 10, 11, 12, 7, 8,
                                        9,  4,  5,  6,  1,  2,  3, 0};
static const uvec8 kRGB24MirrorShuf1 = {14, 15, 16, 11, 12, 13, 8, 9,
                                        10, 5,  6,  7,  2,  3,  4, 1};
static const uvec8 kRGB24MirrorShuf2 = {15, 0,  1, 12, 13, 14, 9, 10,
                                        11, 6,  7, 8,  3,  4,  5, 2};
void RGB24MirrorRow_LSX(const uint8_t* src_rgb24,
                        uint8_t* dst_rgb24,
                        int width) {
  src_rgb24 += width * 3 - 48;
  asm volatile(
      "vld $vr4, %[sh0], 0\n\t"
      "vld $vr5, %[sh1], 0\n\t"
      "vld $vr6, %[sh2], 0\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr2, %[src], 32\n\t"
      "vshuf.b $vr3, $vr2, $vr1, $vr4\n\t"
      "vshuf.b $vr1, $vr1, $vr0, $vr5\n\t"
      "vshuf.b $vr0, $vr0, $vr0, $vr6\n\t"
      "vst $vr3, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "vst $vr0, %[dst], 32\n\t"
      "addi.d %[src], %[src], -48\n\t"
      "addi.d %[dst], %[dst], 48\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_rgb24), [dst] "+r"(dst_rgb24), [cnt] "+r"(width)
      : [sh0] "r"(&kRGB24MirrorShuf0), [sh1] "r"(&kRGB24MirrorShuf1),
        [sh2] "r"(&kRGB24MirrorShuf2)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6");
}
#endif  // HAS_RGB24MIRRORROW_LSX

#ifdef HAS_BLENDPLANEROW_LSX
void BlendPlaneRow_LSX(const uint8_t* src0,
                       const uint8_t* src1,
                       const uint8_t* alpha,
                       uint8_t* dst,
                       int width) {
  asm volatile(
      "vldi $vr6, 0x7f\n\t"
      "vslli.h $vr6, $vr6, 1\n\t"
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr1, %[s1], 0\n\t"
      "vld $vr2, %[alp], 0\n\t"
      "vxori.b $vr3, $vr2, 0xff\n\t"
      "vmulwev.h.bu $vr4, $vr0, $vr2\n\t"
      "vmaddwev.h.bu $vr4, $vr1, $vr3\n\t"
      "vmulwod.h.bu $vr5, $vr0, $vr2\n\t"
      "vmaddwod.h.bu $vr5, $vr1, $vr3\n\t"
      "vadd.h $vr4, $vr4, $vr6\n\t"
      "vadd.h $vr5, $vr5, $vr6\n\t"
      "vsrli.h $vr4, $vr4, 8\n\t"
      "vsrli.h $vr5, $vr5, 8\n\t"
      "vpackev.b $vr0, $vr5, $vr4\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 16\n\t"
      "addi.d %[s1], %[s1], 16\n\t"
      "addi.d %[alp], %[alp], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src0), [s1] "+r"(src1), [alp] "+r"(alpha), [dst] "+r"(dst),
        [cnt] "+r"(width)
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
        "1:\n\t"
        "vld $vr0, %[src0], 0\n\t"
        "vst $vr0, %[dst], 0\n\t"
        "addi.d %[src0], %[src0], 16\n\t"
        "addi.d %[dst], %[dst], 16\n\t"
        "addi.d %[cnt], %[cnt], -8\n\t"
        "bnez %[cnt], 1b\n\t"
        : [src0] "+r"(src_ptr), [dst] "+r"(dst_ptr), [cnt] "+r"(width)
        :
        : "memory", "$vr0");
    return;
  }
  if (y1_fraction == 128) {
    HalfRow_16_LSX(src_ptr, src_stride, dst_ptr, width);
    return;
  }
  asm volatile(
      "vreplgr2vr.h $vr4, %[f0]\n\t"
      "vreplgr2vr.h $vr5, %[f1]\n\t"
      "1:\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vld $vr1, %[src1], 0\n\t"
      "vmulwev.w.hu $vr2, $vr0, $vr4\n\t"
      "vmaddwev.w.hu $vr2, $vr1, $vr5\n\t"
      "vmulwod.w.hu $vr3, $vr0, $vr4\n\t"
      "vmaddwod.w.hu $vr3, $vr1, $vr5\n\t"
      "vsrari.w $vr2, $vr2, 8\n\t"
      "vsrari.w $vr3, $vr3, 8\n\t"
      "vpackev.h $vr3, $vr3, $vr2\n\t"
      "vst $vr3, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 16\n\t"
      "addi.d %[src1], %[src1], 16\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_ptr), [src1] "+r"(src_ptr1), [dst] "+r"(dst_ptr),
        [cnt] "+r"(width)
      : [f0] "r"(y0_fraction), [f1] "r"(y1_fraction)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_INTERPOLATEROW_16_LSX

#ifdef HAS_COPYROW_LASX
void CopyRow_LASX(const uint8_t* src, uint8_t* dst, int count) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(count)
      :
      : "memory", "$xr0");
}
#endif  // HAS_COPYROW_LASX

#ifdef HAS_SETROW_LASX
void SetRow_LASX(uint8_t* dst, uint8_t v8, int count) {
  asm volatile(
      "xvreplgr2vr.b $xr0, %[val]\n\t"
      "1:\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [dst] "+r"(dst), [cnt] "+r"(count)
      : [val] "r"(v8)
      : "memory", "$xr0");
}
#endif  // HAS_SETROW_LASX

#ifdef HAS_ARGBSETROW_LASX
void ARGBSetRow_LASX(uint8_t* dst_argb, uint32_t v32, int width) {
  asm volatile(
      "xvreplgr2vr.w $xr0, %[val]\n\t"
      "1:\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [val] "r"(v32)
      : "memory", "$xr0");
}
#endif  // HAS_ARGBSETROW_LASX

#ifdef HAS_MIRRORROW_LASX
void MirrorRow_LASX(const uint8_t* src_y, uint8_t* dst_y, int width) {
  static const uint8_t kRev[16] = {15, 14, 13, 12, 11, 10, 9, 8,
                                   7,  6,  5,  4,  3,  2,  1, 0};
  src_y += width - 32;
  asm volatile(
      "vld $vr1, %[rev], 0\n\t"
      "xvpermi.q $xr1, $xr1, 0x00\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 16\n\t"
      "vld $vr31, %[src], 0\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvshuf.b $xr0, $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], -32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [rev] "r"(kRev)
      : "memory", "$xr0", "$xr1", "$xr31");
}
#endif  // HAS_MIRRORROW_LASX

#ifdef HAS_MIRRORUVROW_LASX
void MirrorUVRow_LASX(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  static const uint8_t kRevUV[16] = {14, 15, 12, 13, 10, 11, 8, 9,
                                     6,  7,  4,  5,  2,  3,  0, 1};
  src_uv += width * 2 - 32;
  asm volatile(
      "vld $vr1, %[rev], 0\n\t"
      "xvpermi.q $xr1, $xr1, 0x00\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 16\n\t"
      "vld $vr31, %[src], 0\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvshuf.b $xr0, $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], -32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uv), [dst] "+r"(dst_uv), [cnt] "+r"(width)
      : [rev] "r"(kRevUV)
      : "memory", "$xr0", "$xr1", "$xr31");
}
#endif  // HAS_MIRRORUVROW_LASX

#ifdef HAS_ARGBMIRRORROW_LASX
void ARGBMirrorRow_LASX(const uint8_t* src_argb, uint8_t* dst_argb, int width) {
  src_argb += width * 4 - 32;
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 16\n\t"
      "vld $vr31, %[src], 0\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvshuf4i.w $xr0, $xr0, 0x1b\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], -32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr31");
}
#endif  // HAS_ARGBMIRRORROW_LASX

#ifdef HAS_MIRRORSPLITUVROW_LASX
void MirrorSplitUVRow_LASX(const uint8_t* src_uv,
                           uint8_t* dst_u,
                           uint8_t* dst_v,
                           int width) {
  static const uint8_t kRevUV[16] = {14, 15, 12, 13, 10, 11, 8, 9,
                                     6,  7,  4,  5,  2,  3,  0, 1};
  src_uv += width * 2 - 64;
  asm volatile(
      "vld $vr4, %[rev], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 48\n\t"
      "vld $vr31, %[src], 16\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 32\n\t"
      "vld $vr31, %[src], 0\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvshuf.b $xr0, $xr0, $xr0, $xr4\n\t"
      "xvshuf.b $xr1, $xr1, $xr1, $xr4\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvst $xr2, %[du], 0\n\t"
      "xvst $xr3, %[dv], 0\n\t"
      "addi.d %[src], %[src], -64\n\t"
      "addi.d %[du], %[du], 32\n\t"
      "addi.d %[dv], %[dv], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      :
      [src] "+r"(src_uv), [du] "+r"(dst_u), [dv] "+r"(dst_v), [cnt] "+r"(width)
      : [rev] "r"(kRevUV)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr31");
}
#endif  // HAS_MIRRORSPLITUVROW_LASX

#ifdef HAS_SPLITUVROW_LASX
void SplitUVRow_LASX(const uint8_t* src_uv,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvst $xr2, %[du], 0\n\t"
      "xvst $xr3, %[dv], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[du], %[du], 32\n\t"
      "addi.d %[dv], %[dv], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      :
      [src] "+r"(src_uv), [du] "+r"(dst_u), [dv] "+r"(dst_v), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr31");
}
#endif  // HAS_SPLITUVROW_LASX

#ifdef HAS_MERGEUVROW_LASX
void MergeUVRow_LASX(const uint8_t* src_u,
                     const uint8_t* src_v,
                     uint8_t* dst_uv,
                     int width) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[su], 0\n\t"
      "xvld $xr1, %[sv], 0\n\t"
      "xvilvl.b $xr2, $xr1, $xr0\n\t"
      "xvilvh.b $xr3, $xr1, $xr0\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr2, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr3, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr3, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[su], %[su], 32\n\t"
      "addi.d %[sv], %[sv], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      :
      [su] "+r"(src_u), [sv] "+r"(src_v), [dst] "+r"(dst_uv), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr31");
}
#endif  // HAS_MERGEUVROW_LASX

#ifdef HAS_ARGBEXTRACTALPHAROW_LASX
void ARGBExtractAlphaRow_LASX(const uint8_t* src_argb,
                              uint8_t* dst_a,
                              int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickod.b $xr0, $xr1, $xr0\n\t"
      "xvpickod.b $xr0, $xr0, $xr0\n\t"
      "xvpermi.d $xr31, $xr0, 0xd8\n\t"
      "vst $vr31, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_a), [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[sy], 0\n\t"
      "vld $vr1, %[su], 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "vld $vr2, %[sv], 0\n\t"
      "xvpermi.d $xr2, $xr2, 0xd8\n\t"
      "xvilvl.b $xr3, $xr2, $xr1\n\t"
      "xvilvl.b $xr4, $xr3, $xr0\n\t"
      "xvilvh.b $xr5, $xr3, $xr0\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr4, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr5, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr5, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 32\n\t"
      "addi.d %[su], %[su], 16\n\t"
      "addi.d %[sv], %[sv], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_yuy2), [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[sy], 0\n\t"
      "vld $vr1, %[su], 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "vld $vr2, %[sv], 0\n\t"
      "xvpermi.d $xr2, $xr2, 0xd8\n\t"
      "xvilvl.b $xr3, $xr2, $xr1\n\t"
      "xvilvl.b $xr4, $xr0, $xr3\n\t"
      "xvilvh.b $xr5, $xr0, $xr3\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr4, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr5, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr5, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 32\n\t"
      "addi.d %[su], %[su], 16\n\t"
      "addi.d %[sv], %[sv], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_uyvy), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr31");
}
#endif  // HAS_I422TOUYVYROW_LASX

#ifdef HAS_YUY2TOYROW_LASX
void YUY2ToYRow_LASX(const uint8_t* src_yuy2, uint8_t* dst_y, int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_yuy2), [dst] "+r"(dst_y), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr31");
}
#endif  // HAS_YUY2TOYROW_LASX

#ifdef HAS_UYVYTOYROW_LASX
void UYVYToYRow_LASX(const uint8_t* src_uyvy, uint8_t* dst_y, int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickod.b $xr2, $xr1, $xr0\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uyvy), [dst] "+r"(dst_y), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr31");
}
#endif  // HAS_UYVYTOYROW_LASX

#ifdef HAS_YUY2TOUV422ROW_LASX
void YUY2ToUV422Row_LASX(const uint8_t* src_yuy2,
                         uint8_t* dst_u,
                         uint8_t* dst_v,
                         int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickod.b $xr2, $xr1, $xr0\n\t"
      "xvpickev.b $xr3, $xr2, $xr2\n\t"
      "xvpickod.b $xr4, $xr2, $xr2\n\t"
      "xvpermi.d $xr31, $xr3, 0xd8\n\t"
      "vst $vr31, %[du], 0\n\t"
      "xvpermi.d $xr31, $xr4, 0xd8\n\t"
      "vst $vr31, %[dv], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_yuy2), [du] "+r"(dst_u), [dv] "+r"(dst_v),
        [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr31");
}
#endif  // HAS_YUY2TOUV422ROW_LASX

#ifdef HAS_UYVYTOUV422ROW_LASX
void UYVYToUV422Row_LASX(const uint8_t* src_uyvy,
                         uint8_t* dst_u,
                         uint8_t* dst_v,
                         int width) {
  asm volatile(
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickev.b $xr3, $xr2, $xr2\n\t"
      "xvpickod.b $xr4, $xr2, $xr2\n\t"
      "xvpermi.d $xr31, $xr3, 0xd8\n\t"
      "vst $vr31, %[du], 0\n\t"
      "xvpermi.d $xr31, $xr4, 0xd8\n\t"
      "vst $vr31, %[dv], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uyvy), [du] "+r"(dst_u), [dv] "+r"(dst_v),
        [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr31");
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
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr31, %[s0], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[s0], 16\n\t"
      "vld $vr31, %[s0], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "vld $vr2, %[s1], 0\n\t"
      "vld $vr31, %[s1], 32\n\t"
      "xvpermi.q $xr2, $xr31, 0x02\n\t"
      "vld $vr3, %[s1], 16\n\t"
      "vld $vr31, %[s1], 48\n\t"
      "xvpermi.q $xr3, $xr31, 0x02\n\t"
      "xvpickod.b $xr0, $xr1, $xr0\n\t"
      "xvpickod.b $xr2, $xr3, $xr2\n\t"
      "xvavgr.bu $xr0, $xr0, $xr2\n\t"
      "xvpickev.b $xr1, $xr0, $xr0\n\t"
      "xvpickod.b $xr2, $xr0, $xr0\n\t"
      "xvpermi.d $xr31, $xr1, 0xd8\n\t"
      "vst $vr31, %[du], 0\n\t"
      "xvpermi.d $xr31, $xr2, 0xd8\n\t"
      "vst $vr31, %[dv], 0\n\t"
      "addi.d %[s0], %[s0], 64\n\t"
      "addi.d %[s1], %[s1], 64\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_yuy2), [s1] "+r"(next_yuy2), [du] "+r"(dst_u),
        [dv] "+r"(dst_v), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr31");
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
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr31, %[s0], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[s0], 16\n\t"
      "vld $vr31, %[s0], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "vld $vr2, %[s1], 0\n\t"
      "vld $vr31, %[s1], 32\n\t"
      "xvpermi.q $xr2, $xr31, 0x02\n\t"
      "vld $vr3, %[s1], 16\n\t"
      "vld $vr31, %[s1], 48\n\t"
      "xvpermi.q $xr3, $xr31, 0x02\n\t"
      "xvpickev.b $xr0, $xr1, $xr0\n\t"
      "xvpickev.b $xr2, $xr3, $xr2\n\t"
      "xvavgr.bu $xr0, $xr0, $xr2\n\t"
      "xvpickev.b $xr1, $xr0, $xr0\n\t"
      "xvpickod.b $xr2, $xr0, $xr0\n\t"
      "xvpermi.d $xr31, $xr1, 0xd8\n\t"
      "vst $vr31, %[du], 0\n\t"
      "xvpermi.d $xr31, $xr2, 0xd8\n\t"
      "vst $vr31, %[dv], 0\n\t"
      "addi.d %[s0], %[s0], 64\n\t"
      "addi.d %[s1], %[s1], 64\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_uyvy), [s1] "+r"(next_uyvy), [du] "+r"(dst_u),
        [dv] "+r"(dst_v), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr31");
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvldrepl.d $xr10, %[su], 0\n\t"
      "xvshuf4i.w $xr31, $xr10, 0x01\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "xvldrepl.d $xr11, %[sv], 0\n\t"
      "xvshuf4i.w $xr31, $xr11, 0x01\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvilvl.b $xr10, $xr10, $xr10\n\t"
      "xvilvl.b $xr11, $xr11, $xr11\n\t"
      "xvilvl.b $xr1, $xr11, $xr10\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr3, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr4\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvldrepl.d $xr10, %[su], 0\n\t"
      "xvshuf4i.w $xr31, $xr10, 0x01\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "xvldrepl.d $xr11, %[sv], 0\n\t"
      "xvshuf4i.w $xr31, $xr11, 0x01\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvilvl.b $xr10, $xr10, $xr10\n\t"
      "xvilvl.b $xr11, $xr11, $xr11\n\t"
      "xvilvl.b $xr1, $xr11, $xr10\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr2, $xr5\n\t"
      "xvilvl.b $xr7, $xr4, $xr3\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvldrepl.d $xr10, %[su], 0\n\t"
      "xvshuf4i.w $xr31, $xr10, 0x01\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "xvldrepl.d $xr11, %[sv], 0\n\t"
      "xvshuf4i.w $xr31, $xr11, 0x01\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvilvl.b $xr10, $xr10, $xr10\n\t"
      "xvilvl.b $xr11, $xr11, $xr11\n\t"
      "xvilvl.b $xr1, $xr11, $xr10\n\t"
      "vld $vr5, %[sa], 0\n\t"
      "xvpermi.d $xr5, $xr5, 0xd8\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr3, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr4\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[sa], %[sa], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width), [sa] "+r"(src_a)
      : [c] "r"(yuvconstants)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "vld $vr12, %[s0], 0\n\t"
      "xvpermi.q $xr12, $xr12, 0x00\n\t"
      "vld $vr13, %[s1], 0\n\t"
      "xvpermi.q $xr13, $xr13, 0x00\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvldrepl.d $xr10, %[su], 0\n\t"
      "xvshuf4i.w $xr31, $xr10, 0x01\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "xvldrepl.d $xr11, %[sv], 0\n\t"
      "xvshuf4i.w $xr31, $xr11, 0x01\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvilvl.b $xr10, $xr10, $xr10\n\t"
      "xvilvl.b $xr11, $xr11, $xr11\n\t"
      "xvilvl.b $xr1, $xr11, $xr10\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr3, $xr2\n\t"
      "xvilvl.b $xr7, $xr4, $xr4\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "xvshuf.b $xr10, $xr9, $xr8, $xr12\n\t"
      "xvshuf.b $xr11, $xr9, $xr8, $xr13\n\t"
      "vst $vr10, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr10, 0x01\n\t"
      "vst $vr31, %[dst], 24\n\t"
      "vstelm.d $vr11, %[dst], 16, 0\n\t"
      "xvpermi.q $xr31, $xr11, 0x01\n\t"
      "vstelm.d $vr31, %[dst], 40, 0\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 48\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants), [s0] "r"(kShuf0), [s1] "r"(kShuf1)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr14, 31\n\t"
      "xvsrli.h $xr14, $xr14, 8\n\t"
      "xvldi $xr15, 63\n\t"
      "xvsrli.h $xr15, $xr15, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvldrepl.d $xr10, %[su], 0\n\t"
      "xvshuf4i.w $xr31, $xr10, 0x01\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "xvldrepl.d $xr11, %[sv], 0\n\t"
      "xvshuf4i.w $xr31, $xr11, 0x01\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvilvl.b $xr10, $xr10, $xr10\n\t"
      "xvilvl.b $xr11, $xr11, $xr11\n\t"
      "xvilvl.b $xr1, $xr11, $xr10\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvsrli.b $xr2, $xr2, 3\n\t"
      "xvsrli.b $xr3, $xr3, 2\n\t"
      "xvsrli.b $xr4, $xr4, 3\n\t"
      "xvilvl.b $xr2, $xr2, $xr2\n\t"
      "xvilvl.b $xr3, $xr3, $xr3\n\t"
      "xvilvl.b $xr4, $xr4, $xr4\n\t"
      "xvand.v $xr2, $xr2, $xr14\n\t"
      "xvand.v $xr3, $xr3, $xr15\n\t"
      "xvand.v $xr4, $xr4, $xr14\n\t"
      "xvslli.h $xr3, $xr3, 5\n\t"
      "xvslli.h $xr4, $xr4, 11\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvor.v $xr2, $xr2, $xr4\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr10", "$xr11",
        "$xr14", "$xr15", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24", "$xr31");
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr14, 15\n\t"
      "xvsrli.h $xr14, $xr14, 8\n\t"
      "xvldi $xr15, 240\n\t"
      "xvsrli.h $xr15, $xr15, 8\n\t"
      "xvslli.h $xr15, $xr15, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvldrepl.d $xr10, %[su], 0\n\t"
      "xvshuf4i.w $xr31, $xr10, 0x01\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "xvldrepl.d $xr11, %[sv], 0\n\t"
      "xvshuf4i.w $xr31, $xr11, 0x01\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvilvl.b $xr10, $xr10, $xr10\n\t"
      "xvilvl.b $xr11, $xr11, $xr11\n\t"
      "xvilvl.b $xr1, $xr11, $xr10\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvsrli.b $xr2, $xr2, 4\n\t"
      "xvsrli.b $xr3, $xr3, 4\n\t"
      "xvsrli.b $xr4, $xr4, 4\n\t"
      "xvilvl.b $xr2, $xr2, $xr2\n\t"
      "xvilvl.b $xr3, $xr3, $xr3\n\t"
      "xvilvl.b $xr4, $xr4, $xr4\n\t"
      "xvand.v $xr2, $xr2, $xr14\n\t"
      "xvand.v $xr3, $xr3, $xr14\n\t"
      "xvand.v $xr4, $xr4, $xr14\n\t"
      "xvslli.h $xr3, $xr3, 4\n\t"
      "xvslli.h $xr4, $xr4, 8\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvor.v $xr2, $xr2, $xr4\n\t"
      "xvor.v $xr2, $xr2, $xr15\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr10", "$xr11",
        "$xr14", "$xr15", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24", "$xr31");
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr14, 31\n\t"
      "xvsrli.h $xr14, $xr14, 8\n\t"
      "xvldi $xr15, 128\n\t"
      "xvsrli.h $xr15, $xr15, 8\n\t"
      "xvslli.h $xr15, $xr15, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvldrepl.d $xr10, %[su], 0\n\t"
      "xvshuf4i.w $xr31, $xr10, 0x01\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "xvldrepl.d $xr11, %[sv], 0\n\t"
      "xvshuf4i.w $xr31, $xr11, 0x01\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvilvl.b $xr10, $xr10, $xr10\n\t"
      "xvilvl.b $xr11, $xr11, $xr11\n\t"
      "xvilvl.b $xr1, $xr11, $xr10\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvsrli.b $xr2, $xr2, 3\n\t"
      "xvsrli.b $xr3, $xr3, 3\n\t"
      "xvsrli.b $xr4, $xr4, 3\n\t"
      "xvilvl.b $xr2, $xr2, $xr2\n\t"
      "xvilvl.b $xr3, $xr3, $xr3\n\t"
      "xvilvl.b $xr4, $xr4, $xr4\n\t"
      "xvand.v $xr2, $xr2, $xr14\n\t"
      "xvand.v $xr3, $xr3, $xr14\n\t"
      "xvand.v $xr4, $xr4, $xr14\n\t"
      "xvslli.h $xr3, $xr3, 5\n\t"
      "xvslli.h $xr4, $xr4, 10\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvor.v $xr2, $xr2, $xr4\n\t"
      "xvor.v $xr2, $xr2, $xr15\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 8\n\t"
      "addi.d %[sv], %[sv], 8\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr10", "$xr11",
        "$xr14", "$xr15", "$xr20", "$xr21", "$xr22", "$xr23", "$xr24", "$xr31");
}
#endif  // HAS_I422TOARGB1555ROW_LASX

#ifdef HAS_NV12TOARGBROW_LASX
void NV12ToARGBRow_LASX(const uint8_t* src_y,
                        const uint8_t* src_uv,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  asm volatile(
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "vld $vr1, %[suv], 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvilvl.h $xr1, $xr1, $xr1\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr3, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr4\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[suv], %[suv], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [suv] "+r"(src_uv), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "vld $vr1, %[suv], 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvshuf4i.b $xr1, $xr1, 0xb1\n\t"
      "xvilvl.h $xr1, $xr1, $xr1\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr3, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr4\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[suv], %[suv], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [suv] "+r"(src_vu), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr14, 31\n\t"
      "xvsrli.h $xr14, $xr14, 8\n\t"
      "xvldi $xr15, 63\n\t"
      "xvsrli.h $xr15, $xr15, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "vld $vr1, %[suv], 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvilvl.h $xr1, $xr1, $xr1\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvsrli.b $xr2, $xr2, 3\n\t"
      "xvsrli.b $xr3, $xr3, 2\n\t"
      "xvsrli.b $xr4, $xr4, 3\n\t"
      "xvilvl.b $xr2, $xr2, $xr2\n\t"
      "xvilvl.b $xr3, $xr3, $xr3\n\t"
      "xvilvl.b $xr4, $xr4, $xr4\n\t"
      "xvand.v $xr2, $xr2, $xr14\n\t"
      "xvand.v $xr3, $xr3, $xr15\n\t"
      "xvand.v $xr4, $xr4, $xr14\n\t"
      "xvslli.h $xr3, $xr3, 5\n\t"
      "xvslli.h $xr4, $xr4, 11\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvor.v $xr2, $xr2, $xr4\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[suv], %[suv], 16\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [suv] "+r"(src_uv), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "vld $vr10, %[su], 0\n\t"
      "xvpermi.d $xr10, $xr10, 0xd8\n\t"
      "vld $vr11, %[sv], 0\n\t"
      "xvpermi.d $xr11, $xr11, 0xd8\n\t"
      "xvilvl.b $xr1, $xr11, $xr10\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr3, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr4\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[su], %[su], 16\n\t"
      "addi.d %[sv], %[sv], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [su] "+r"(src_u), [sv] "+r"(src_v),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vld $vr23, %[c], 96\n\t"
      "xvpermi.q $xr23, $xr23, 0x00\n\t"
      "vld $vr24, %[c], 128\n\t"
      "xvpermi.q $xr24, $xr24, 0x00\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sy], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvssrani.bu.h $xr2, $xr0, 6\n\t"
      "xvilvl.b $xr6, $xr2, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr2\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
      : "memory", "$xr0", "$xr2", "$xr5", "$xr6", "$xr7", "$xr8", "$xr9",
        "$xr23", "$xr24", "$xr31");
}
#endif  // HAS_I400TOARGBROW_LASX

#ifdef HAS_J400TOARGBROW_LASX
void J400ToARGBRow_LASX(const uint8_t* src_y, uint8_t* dst_argb, int width) {
  asm volatile(
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "vld $vr2, %[sy], 0\n\t"
      "xvpermi.d $xr2, $xr2, 0xd8\n\t"
      "xvilvl.b $xr6, $xr2, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr2\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sy] "+r"(src_y), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "xvld $xr10, %[src], 0\n\t"
      "xvpickev.b $xr0, $xr10, $xr10\n\t"
      "xvpickod.b $xr1, $xr10, $xr10\n\t"
      "xvilvl.h $xr1, $xr1, $xr1\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr3, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr4\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_yuy2), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "xvldrepl.w $xr20, %[c], 0\n\t"
      "xvldrepl.w $xr21, %[c], 32\n\t"
      "xvldrepl.w $xr22, %[c], 64\n\t"
      "xvldrepl.h $xr23, %[c], 96\n\t"
      "xvldrepl.h $xr24, %[c], 128\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "xvld $xr10, %[src], 0\n\t"
      "xvpickod.b $xr0, $xr10, $xr10\n\t"
      "xvpickev.b $xr1, $xr10, $xr10\n\t"
      "xvilvl.h $xr1, $xr1, $xr1\n\t"
      "xvilvl.b $xr0, $xr0, $xr0\n\t"
      "xvxori.b $xr1, $xr1, 0x80\n\t"
      "xvmuh.hu $xr0, $xr0, $xr23\n\t"
      "xvadd.h $xr0, $xr0, $xr24\n\t"
      "xvmulwev.h.bu.b $xr2, $xr20, $xr1\n\t"
      "xvmulwod.h.bu.b $xr4, $xr22, $xr1\n\t"
      "xvmulwev.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvmaddwod.h.bu.b $xr3, $xr21, $xr1\n\t"
      "xvsadd.h $xr2, $xr0, $xr2\n\t"
      "xvsadd.h $xr4, $xr0, $xr4\n\t"
      "xvssub.h $xr3, $xr0, $xr3\n\t"
      "xvssrani.bu.h $xr2, $xr2, 6\n\t"
      "xvssrani.bu.h $xr3, $xr3, 6\n\t"
      "xvssrani.bu.h $xr4, $xr4, 6\n\t"
      "xvilvl.b $xr6, $xr3, $xr2\n\t"
      "xvilvl.b $xr7, $xr5, $xr4\n\t"
      "xvilvl.h $xr8, $xr7, $xr6\n\t"
      "xvilvh.h $xr9, $xr7, $xr6\n\t"
      "vst $vr8, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr9, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr9, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uyvy), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [c] "r"(yuvconstants)
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
      "vld $vr4, %[shuf], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 64\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 80\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "vld $vr2, %[src], 32\n\t"
      "vld $vr31, %[src], 96\n\t"
      "xvpermi.q $xr2, $xr31, 0x02\n\t"
      "vld $vr3, %[src], 48\n\t"
      "vld $vr31, %[src], 112\n\t"
      "xvpermi.q $xr3, $xr31, 0x02\n\t"
      "xvshuf.b $xr0, $xr0, $xr0, $xr4\n\t"
      "xvshuf.b $xr1, $xr1, $xr1, $xr4\n\t"
      "xvshuf.b $xr2, $xr2, $xr2, $xr4\n\t"
      "xvshuf.b $xr3, $xr3, $xr3, $xr4\n\t"
      "vstelm.d $vr0, %[dst], 0, 0\n\t"
      "xvpermi.q $xr31, $xr0, 0x01\n\t"
      "vstelm.d $vr31, %[dst], 48, 0\n\t"
      "vstelm.w $vr0, %[dst], 8, 2\n\t"
      "xvpermi.q $xr31, $xr0, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 56, 2\n\t"
      "vstelm.w $vr1, %[dst], 12, 0\n\t"
      "vstelm.w $vr1, %[dst], 16, 1\n\t"
      "xvpermi.q $xr31, $xr1, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 60, 0\n\t"
      "vstelm.w $vr31, %[dst], 64, 1\n\t"
      "vstelm.w $vr1, %[dst], 20, 2\n\t"
      "xvpermi.q $xr31, $xr1, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 68, 2\n\t"
      "vstelm.d $vr2, %[dst], 24, 0\n\t"
      "xvpermi.q $xr31, $xr2, 0x01\n\t"
      "vstelm.d $vr31, %[dst], 72, 0\n\t"
      "vstelm.w $vr2, %[dst], 32, 2\n\t"
      "xvpermi.q $xr31, $xr2, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 80, 2\n\t"
      "vstelm.w $vr3, %[dst], 36, 0\n\t"
      "vstelm.w $vr3, %[dst], 40, 1\n\t"
      "xvpermi.q $xr31, $xr3, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 84, 0\n\t"
      "vstelm.w $vr31, %[dst], 88, 1\n\t"
      "vstelm.w $vr3, %[dst], 44, 2\n\t"
      "xvpermi.q $xr31, $xr3, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 92, 2\n\t"
      "addi.d %[src], %[src], 128\n\t"
      "addi.d %[dst], %[dst], 96\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_24), [cnt] "+r"(width)
      : [shuf] "r"(shuf)
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
      "vld $vr4, %[shuf], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "vld $vr5, %[shuf], 16\n\t"
      "xvpermi.q $xr5, $xr5, 0x00\n\t"
      "xvldi $xr6, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 12\n\t"
      "vld $vr31, %[src], 60\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "vld $vr2, %[src], 24\n\t"
      "vld $vr31, %[src], 72\n\t"
      "xvpermi.q $xr2, $xr31, 0x02\n\t"
      "vld $vr3, %[src], 32\n\t"
      "vld $vr31, %[src], 80\n\t"
      "xvpermi.q $xr3, $xr31, 0x02\n\t"
      "xvshuf.b $xr0, $xr6, $xr0, $xr4\n\t"
      "xvshuf.b $xr1, $xr6, $xr1, $xr4\n\t"
      "xvshuf.b $xr2, $xr6, $xr2, $xr4\n\t"
      "xvshuf.b $xr3, $xr6, $xr3, $xr5\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr0, 0x01\n\t"
      "vst $vr31, %[dst], 64\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr1, 0x01\n\t"
      "vst $vr31, %[dst], 80\n\t"
      "vst $vr2, %[dst], 32\n\t"
      "xvpermi.q $xr31, $xr2, 0x01\n\t"
      "vst $vr31, %[dst], 96\n\t"
      "vst $vr3, %[dst], 48\n\t"
      "xvpermi.q $xr31, $xr3, 0x01\n\t"
      "vst $vr31, %[dst], 112\n\t"
      "addi.d %[src], %[src], 96\n\t"
      "addi.d %[dst], %[dst], 128\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_24), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [shuf] "r"(shuf)
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
      "vld $vr4, %[shuf], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "vld $vr5, %[shuf], 16\n\t"
      "xvpermi.q $xr5, $xr5, 0x00\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 12\n\t"
      "vld $vr31, %[src], 60\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "vld $vr2, %[src], 24\n\t"
      "vld $vr31, %[src], 72\n\t"
      "xvpermi.q $xr2, $xr31, 0x02\n\t"
      "vld $vr3, %[src], 32\n\t"
      "vld $vr31, %[src], 80\n\t"
      "xvpermi.q $xr3, $xr31, 0x02\n\t"
      "xvshuf.b $xr0, $xr0, $xr0, $xr4\n\t"
      "xvshuf.b $xr1, $xr1, $xr1, $xr4\n\t"
      "xvshuf.b $xr2, $xr2, $xr2, $xr4\n\t"
      "xvshuf.b $xr3, $xr3, $xr3, $xr5\n\t"
      "vstelm.d $vr0, %[dst], 0, 0\n\t"
      "xvpermi.q $xr31, $xr0, 0x01\n\t"
      "vstelm.d $vr31, %[dst], 48, 0\n\t"
      "vstelm.w $vr0, %[dst], 8, 2\n\t"
      "xvpermi.q $xr31, $xr0, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 56, 2\n\t"
      "vstelm.w $vr1, %[dst], 12, 0\n\t"
      "vstelm.w $vr1, %[dst], 16, 1\n\t"
      "xvpermi.q $xr31, $xr1, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 60, 0\n\t"
      "vstelm.w $vr31, %[dst], 64, 1\n\t"
      "vstelm.w $vr1, %[dst], 20, 2\n\t"
      "xvpermi.q $xr31, $xr1, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 68, 2\n\t"
      "vstelm.d $vr2, %[dst], 24, 0\n\t"
      "xvpermi.q $xr31, $xr2, 0x01\n\t"
      "vstelm.d $vr31, %[dst], 72, 0\n\t"
      "vstelm.w $vr2, %[dst], 32, 2\n\t"
      "xvpermi.q $xr31, $xr2, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 80, 2\n\t"
      "vstelm.w $vr3, %[dst], 36, 0\n\t"
      "vstelm.w $vr3, %[dst], 40, 1\n\t"
      "xvpermi.q $xr31, $xr3, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 84, 0\n\t"
      "vstelm.w $vr31, %[dst], 88, 1\n\t"
      "vstelm.w $vr3, %[dst], 44, 2\n\t"
      "xvpermi.q $xr31, $xr3, 0x01\n\t"
      "vstelm.w $vr31, %[dst], 92, 2\n\t"
      "addi.d %[src], %[src], 96\n\t"
      "addi.d %[dst], %[dst], 96\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_raw), [dst] "+r"(dst_rgb24), [cnt] "+r"(width)
      : [shuf] "r"(kShuf)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr31");
}
#endif  // HAS_RAWTORGB24ROW_LASX

#ifdef HAS_ARGBTORGB565ROW_LASX
void ARGBToRGB565Row_LASX(const uint8_t* src_argb,
                          uint8_t* dst_rgb,
                          int width) {
  asm volatile(
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "xvldi $xr7, 63\n\t"
      "xvsrli.h $xr7, $xr7, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvsrli.b $xr2, $xr2, 3\n\t"
      "xvsrli.b $xr3, $xr3, 2\n\t"
      "xvand.v $xr4, $xr2, $xr6\n\t"
      "xvsrli.h $xr2, $xr2, 8\n\t"
      "xvand.v $xr3, $xr3, $xr7\n\t"
      "xvslli.h $xr3, $xr3, 5\n\t"
      "xvslli.h $xr2, $xr2, 11\n\t"
      "xvor.v $xr4, $xr4, $xr3\n\t"
      "xvor.v $xr4, $xr4, $xr2\n\t"
      "xvst $xr4, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_rgb), [cnt] "+r"(width)
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
      "xvreplgr2vr.w $xr5, %[dith]\n\t"
      "xvilvl.b $xr5, $xr5, $xr5\n\t"
      "xvilvl.h $xr5, $xr5, $xr5\n\t"
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "xvldi $xr7, 63\n\t"
      "xvsrli.h $xr7, $xr7, 8\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvsadd.bu $xr0, $xr0, $xr5\n\t"
      "xvpickev.b $xr2, $xr0, $xr0\n\t"
      "xvpickod.b $xr3, $xr0, $xr0\n\t"
      "xvsrli.b $xr2, $xr2, 3\n\t"
      "xvsrli.b $xr3, $xr3, 2\n\t"
      "xvand.v $xr4, $xr2, $xr6\n\t"
      "xvsrli.h $xr2, $xr2, 8\n\t"
      "xvand.v $xr3, $xr3, $xr7\n\t"
      "xvslli.h $xr3, $xr3, 5\n\t"
      "xvslli.h $xr2, $xr2, 11\n\t"
      "xvor.v $xr4, $xr4, $xr3\n\t"
      "xvor.v $xr4, $xr4, $xr2\n\t"
      "xvpermi.d $xr31, $xr4, 0xd8\n\t"
      "vst $vr31, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_rgb), [cnt] "+r"(width)
      : [dith] "r"(dither4)
      : "memory", "$xr0", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6", "$xr7",
        "$xr31");
}
#endif  // HAS_ARGBTORGB565DITHERROW_LASX

#ifdef HAS_ARGBTOARGB1555ROW_LASX
void ARGBToARGB1555Row_LASX(const uint8_t* src_argb,
                            uint8_t* dst_rgb,
                            int width) {
  asm volatile(
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvsrli.b $xr2, $xr2, 3\n\t"
      "xvand.v $xr4, $xr2, $xr6\n\t"
      "xvsrli.h $xr2, $xr2, 8\n\t"
      "xvsrli.h $xr5, $xr3, 15\n\t"
      "xvsrli.b $xr3, $xr3, 3\n\t"
      "xvand.v $xr3, $xr3, $xr6\n\t"
      "xvslli.h $xr3, $xr3, 5\n\t"
      "xvslli.h $xr2, $xr2, 10\n\t"
      "xvslli.h $xr5, $xr5, 15\n\t"
      "xvor.v $xr4, $xr4, $xr3\n\t"
      "xvor.v $xr4, $xr4, $xr2\n\t"
      "xvor.v $xr4, $xr4, $xr5\n\t"
      "xvst $xr4, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_rgb), [cnt] "+r"(width)
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
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvsrli.b $xr2, $xr2, 4\n\t"
      "xvsrli.b $xr3, $xr3, 4\n\t"
      "xvslli.b $xr3, $xr3, 4\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_rgb), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr31");
}
#endif  // HAS_ARGBTOARGB4444ROW_LASX

#ifdef HAS_RGB565TOARGBROW_LASX
void RGB565ToARGBRow_LASX(const uint8_t* src_rgb565,
                          uint8_t* dst_argb,
                          int width) {
  asm volatile(
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "xvldi $xr7, 63\n\t"
      "xvsrli.h $xr7, $xr7, 8\n\t"
      "xvldi $xr5, 0xff\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr7\n\t"
      "xvsrli.h $xr3, $xr0, 11\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 2\n\t"
      "xvsrli.h $xr2, $xr2, 4\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvpickev.b $xr1, $xr1, $xr1\n\t"
      "xvpickev.b $xr2, $xr2, $xr2\n\t"
      "xvpickev.b $xr3, $xr3, $xr3\n\t"
      "xvilvl.b $xr8, $xr2, $xr1\n\t"
      "xvilvl.b $xr9, $xr5, $xr3\n\t"
      "xvilvl.h $xr10, $xr9, $xr8\n\t"
      "xvilvh.h $xr11, $xr9, $xr8\n\t"
      "vst $vr10, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr10, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr11, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr11, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_rgb565), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvsrli.h $xr3, $xr0, 10\n\t"
      "xvand.v $xr3, $xr3, $xr6\n\t"
      "xvsrai.h $xr5, $xr0, 15\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 3\n\t"
      "xvsrli.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvpickev.b $xr1, $xr1, $xr1\n\t"
      "xvpickev.b $xr2, $xr2, $xr2\n\t"
      "xvpickev.b $xr3, $xr3, $xr3\n\t"
      "xvpickev.b $xr5, $xr5, $xr5\n\t"
      "xvilvl.b $xr8, $xr2, $xr1\n\t"
      "xvilvl.b $xr9, $xr5, $xr3\n\t"
      "xvilvl.h $xr10, $xr9, $xr8\n\t"
      "xvilvh.h $xr11, $xr9, $xr8\n\t"
      "vst $vr10, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr10, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr11, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr11, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb1555), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "xvldi $xr6, 0x0f\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.b $xr2, $xr0, 4\n\t"
      "xvslli.b $xr3, $xr1, 4\n\t"
      "xvor.v $xr1, $xr1, $xr3\n\t"
      "xvslli.b $xr3, $xr2, 4\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvilvl.b $xr4, $xr2, $xr1\n\t"
      "xvilvh.b $xr5, $xr2, $xr1\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr4, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr5, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr5, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb4444), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[s0], 0\n\t"
      "xvld $xr1, %[s1], 0\n\t"
      "xvmulwev.h.bu $xr2, $xr0, $xr1\n\t"
      "xvmulwod.h.bu $xr3, $xr0, $xr1\n\t"
      "xvsrari.h $xr2, $xr2, 8\n\t"
      "xvsrari.h $xr3, $xr3, 8\n\t"
      "xvpackev.b $xr2, $xr3, $xr2\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[s0], 0\n\t"
      "xvld $xr1, %[s1], 0\n\t"
      "xvsadd.bu $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[s0], 0\n\t"
      "xvld $xr1, %[s1], 0\n\t"
      "xvssub.bu $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
      "vld $vr4, %[shuf], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "xvldi $xr5, 0xff\n\t"
      "xvldi $xr6, 255\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvshuf.b $xr1, $xr5, $xr0, $xr4\n\t"
      "xvmulwev.h.bu $xr2, $xr0, $xr1\n\t"
      "xvmulwod.h.bu $xr3, $xr0, $xr1\n\t"
      "xvadd.h $xr2, $xr2, $xr6\n\t"
      "xvadd.h $xr3, $xr3, $xr6\n\t"
      "xvpackod.b $xr2, $xr3, $xr2\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [shuf] "r"(kAlpha)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_ARGBATTENUATEROW_LASX

#ifdef HAS_ARGBBLENDROW_LASX
void ARGBBlendRow_LASX(const uint8_t* src_argb0,
                       const uint8_t* src_argb1,
                       uint8_t* dst_argb,
                       int width) {
  static const uint8_t kAlpha[16] = {3,  3,  3,  3,  7,  7,  7,  7,
                                     11, 11, 11, 11, 15, 15, 15, 15};
  asm volatile(
      "vld $vr4, %[shuf], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "xvldi $xr5, 0xff\n\t"
      "xvslli.w $xr5, $xr5, 24\n\t"
      "xvldi $xr6, 1\n\t"
      "xvslli.h $xr6, $xr6, 8\n\t"
      "xvldi $xr8, 0\n\t"
      "1:\n\t"
      "xvld $xr0, %[s0], 0\n\t"
      "xvld $xr1, %[s1], 0\n\t"
      "xvshuf.b $xr2, $xr0, $xr0, $xr4\n\t"
      "xvilvl.b $xr3, $xr8, $xr2\n\t"
      "xvilvh.b $xr2, $xr8, $xr2\n\t"
      "xvsub.h $xr3, $xr6, $xr3\n\t"
      "xvsub.h $xr2, $xr6, $xr2\n\t"
      "xvilvl.b $xr7, $xr8, $xr1\n\t"
      "xvilvh.b $xr1, $xr8, $xr1\n\t"
      "xvmul.h $xr7, $xr7, $xr3\n\t"
      "xvmul.h $xr1, $xr1, $xr2\n\t"
      "xvsrlni.b.h $xr1, $xr7, 8\n\t"
      "xvsadd.bu $xr0, $xr0, $xr1\n\t"
      "xvor.v $xr0, $xr0, $xr5\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb0), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [shuf] "r"(kAlpha)
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
      "xvreplgr2vr.w $xr1, %[val]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvor.v $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [val] "r"(value)
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
      "vld $vr8, %[shuf], 0\n\t"
      "xvpermi.q $xr8, $xr8, 0x00\n\t"
      "xvldi $xr9, 1\n\t"
      "xvsrli.h $xr10, $xr9, 8\n\t"
      "xvldi $xr11, 0xff\n\t"
      "xvsrli.w $xr11, $xr11, 8\n\t"
      "xvldi $xr12, 0\n\t"
      "1:\n\t"
      "xvld $xr0, %[s0], 0\n\t"
      "xvld $xr1, %[s1], 0\n\t"
      "xvsrli.w $xr2, $xr0, 24\n\t"
      "xvsrli.w $xr3, $xr1, 24\n\t"
      "xvmul.h $xr4, $xr3, $xr2\n\t"
      "xvadd.h $xr4, $xr4, $xr10\n\t"
      "xvmuh.hu $xr4, $xr4, $xr9\n\t"
      "xvsub.w $xr4, $xr2, $xr4\n\t"
      "xvadd.w $xr4, $xr3, $xr4\n\t"
      "xvseq.w $xr5, $xr3, $xr12\n\t"
      "xvsrli.w $xr5, $xr5, 24\n\t"
      "xvor.v $xr6, $xr2, $xr5\n\t"
      "xvor.v $xr7, $xr4, $xr5\n\t"
      "xvslli.w $xr5, $xr6, 8\n\t"
      "xvsub.w $xr5, $xr5, $xr6\n\t"
      "xvffint.s.wu $xr5, $xr5\n\t"
      "xvffint.s.wu $xr7, $xr7\n\t"
      "xvfdiv.s $xr5, $xr5, $xr7\n\t"
      "xvftintrz.wu.s $xr5, $xr5\n\t"
      "xvshuf.b $xr5, $xr5, $xr5, $xr8\n\t"
      "xvnor.v $xr6, $xr5, $xr5\n\t"
      "xvmulwev.h.bu $xr2, $xr1, $xr6\n\t"
      "xvmulwod.h.bu $xr3, $xr1, $xr6\n\t"
      "xvmulwev.h.bu $xr7, $xr0, $xr5\n\t"
      "xvmulwod.h.bu $xr0, $xr0, $xr5\n\t"
      "xvadd.h $xr2, $xr2, $xr7\n\t"
      "xvadd.h $xr3, $xr3, $xr0\n\t"
      "xvadd.h $xr2, $xr2, $xr10\n\t"
      "xvadd.h $xr3, $xr3, $xr10\n\t"
      "xvmuh.hu $xr2, $xr2, $xr9\n\t"
      "xvmuh.hu $xr3, $xr3, $xr9\n\t"
      "xvslli.h $xr3, $xr3, 8\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvand.v $xr2, $xr2, $xr11\n\t"
      "xvslli.w $xr4, $xr4, 24\n\t"
      "xvor.v $xr2, $xr2, $xr4\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb0), [s1] "+r"(src_argb1), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
      : [shuf] "r"(kShufRatio)
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
      "xvreplgr2vr.w $xr4, %[val]\n\t"
      "xvilvl.b $xr5, $xr4, $xr4\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvilvl.b $xr1, $xr0, $xr0\n\t"
      "xvilvh.b $xr2, $xr0, $xr0\n\t"
      "xvmuh.hu $xr1, $xr1, $xr5\n\t"
      "xvmuh.hu $xr2, $xr2, $xr5\n\t"
      "xvsrlni.b.h $xr2, $xr1, 8\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [val] "r"(value)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr4", "$xr5");
}
#endif  // HAS_ARGBSHADEROW_LASX

#ifdef HAS_ARGBSHUFFLEROW_LASX
void ARGBShuffleRow_LASX(const uint8_t* src_argb,
                         uint8_t* dst_argb,
                         const uint8_t* shuffler,
                         int width) {
  asm volatile(
      "vld $vr4, %[shuf], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvshuf.b $xr0, $xr0, $xr0, $xr4\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [shuf] "r"(shuffler)
      : "memory", "$xr0", "$xr4");
}
#endif  // HAS_ARGBSHUFFLEROW_LASX

#ifdef HAS_ARGBGRAYROW_LASX
void ARGBGrayRow_LASX(const uint8_t* src_argb, uint8_t* dst_argb, int width) {
  asm volatile(
      "xvldi $xr4, 29\n\t"
      "xvldi $xr5, 150\n\t"
      "xvldi $xr6, 77\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvmulwev.h.bu $xr7, $xr2, $xr4\n\t"
      "xvmaddwod.h.bu $xr7, $xr2, $xr6\n\t"
      "xvmaddwev.h.bu $xr7, $xr3, $xr5\n\t"
      "xvsrlri.h $xr7, $xr7, 8\n\t"
      "xvsrli.h $xr3, $xr3, 8\n\t"
      "xvslli.h $xr3, $xr3, 8\n\t"
      "xvor.v $xr3, $xr7, $xr3\n\t"
      "xvslli.h $xr8, $xr7, 8\n\t"
      "xvor.v $xr7, $xr7, $xr8\n\t"
      "xvilvl.h $xr0, $xr3, $xr7\n\t"
      "xvilvh.h $xr1, $xr3, $xr7\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr0, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr1, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr31");
}
#endif  // HAS_ARGBGRAYROW_LASX

#ifdef HAS_ARGBSEPIAROW_LASX
void ARGBSepiaRow_LASX(uint8_t* dst_argb, int width) {
  asm volatile(
      "xvldi $xr4, 17\n\t"
      "xvldi $xr5, 68\n\t"
      "xvldi $xr6, 35\n\t"
      "xvldi $xr7, 22\n\t"
      "xvldi $xr8, 88\n\t"
      "xvldi $xr9, 45\n\t"
      "xvldi $xr10, 24\n\t"
      "xvldi $xr11, 98\n\t"
      "xvldi $xr12, 50\n\t"
      "1:\n\t"
      "vld $vr0, %[dst], 0\n\t"
      "vld $vr31, %[dst], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "vld $vr1, %[dst], 16\n\t"
      "vld $vr31, %[dst], 48\n\t"
      "xvpermi.q $xr1, $xr31, 0x02\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvmulwev.h.bu $xr13, $xr2, $xr4\n\t"
      "xvmaddwod.h.bu $xr13, $xr2, $xr6\n\t"
      "xvmaddwev.h.bu $xr13, $xr3, $xr5\n\t"
      "xvmulwev.h.bu $xr14, $xr2, $xr7\n\t"
      "xvmaddwod.h.bu $xr14, $xr2, $xr9\n\t"
      "xvmaddwev.h.bu $xr14, $xr3, $xr8\n\t"
      "xvmulwev.h.bu $xr15, $xr2, $xr10\n\t"
      "xvmaddwod.h.bu $xr15, $xr2, $xr12\n\t"
      "xvmaddwev.h.bu $xr15, $xr3, $xr11\n\t"
      "xvsrli.h $xr13, $xr13, 7\n\t"
      "xvsrli.h $xr14, $xr14, 7\n\t"
      "xvsrli.h $xr15, $xr15, 7\n\t"
      "xvsat.hu $xr13, $xr13, 7\n\t"
      "xvsat.hu $xr14, $xr14, 7\n\t"
      "xvsat.hu $xr15, $xr15, 7\n\t"
      "xvpickev.b $xr13, $xr13, $xr13\n\t"
      "xvpickev.b $xr14, $xr14, $xr14\n\t"
      "xvpickev.b $xr15, $xr15, $xr15\n\t"
      "xvpickod.b $xr3, $xr3, $xr3\n\t"
      "xvilvl.b $xr2, $xr14, $xr13\n\t"
      "xvilvl.b $xr3, $xr3, $xr15\n\t"
      "xvilvl.h $xr0, $xr3, $xr2\n\t"
      "xvilvh.h $xr1, $xr3, $xr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr0, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr1, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [dst] "+r"(dst_argb), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr14",
        "$xr15", "$xr31");
}
#endif  // HAS_ARGBSEPIAROW_LASX

#ifdef HAS_ARGBCOLORMATRIXROW_LASX
void ARGBColorMatrixRow_LASX(const uint8_t* src_argb,
                             uint8_t* dst_argb,
                             const int8_t* matrix_argb,
                             int width) {
  asm volatile(
      "vldrepl.w $vr4, %[mat], 0\n\t"
      "vldrepl.w $vr31, %[mat], 0\n\t"
      "xvpermi.q $xr4, $xr31, 0x02\n\t"
      "vldrepl.w $vr5, %[mat], 4\n\t"
      "vldrepl.w $vr31, %[mat], 4\n\t"
      "xvpermi.q $xr5, $xr31, 0x02\n\t"
      "vldrepl.w $vr6, %[mat], 8\n\t"
      "vldrepl.w $vr31, %[mat], 8\n\t"
      "xvpermi.q $xr6, $xr31, 0x02\n\t"
      "vldrepl.w $vr7, %[mat], 12\n\t"
      "vldrepl.w $vr31, %[mat], 12\n\t"
      "xvpermi.q $xr7, $xr31, 0x02\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvmulwev.h.bu.b $xr8, $xr0, $xr4\n\t"
      "xvmaddwod.h.bu.b $xr8, $xr0, $xr4\n\t"
      "xvmulwev.h.bu.b $xr9, $xr0, $xr5\n\t"
      "xvmaddwod.h.bu.b $xr9, $xr0, $xr5\n\t"
      "xvmulwev.h.bu.b $xr10, $xr0, $xr6\n\t"
      "xvmaddwod.h.bu.b $xr10, $xr0, $xr6\n\t"
      "xvmulwev.h.bu.b $xr11, $xr0, $xr7\n\t"
      "xvmaddwod.h.bu.b $xr11, $xr0, $xr7\n\t"
      "xvhaddw.w.h $xr8, $xr8, $xr8\n\t"
      "xvhaddw.w.h $xr9, $xr9, $xr9\n\t"
      "xvhaddw.w.h $xr10, $xr10, $xr10\n\t"
      "xvhaddw.w.h $xr11, $xr11, $xr11\n\t"
      "xvilvl.w $xr12, $xr9, $xr8\n\t"
      "xvilvh.w $xr13, $xr9, $xr8\n\t"
      "xvilvl.w $xr14, $xr11, $xr10\n\t"
      "xvilvh.w $xr15, $xr11, $xr10\n\t"
      "xvilvl.d $xr8, $xr14, $xr12\n\t"
      "xvilvh.d $xr9, $xr14, $xr12\n\t"
      "xvilvl.d $xr10, $xr15, $xr13\n\t"
      "xvilvh.d $xr11, $xr15, $xr13\n\t"
      "xvssrani.h.w $xr9, $xr8, 6\n\t"
      "xvssrani.h.w $xr11, $xr10, 6\n\t"
      "xvssrani.bu.h $xr11, $xr9, 0\n\t"
      "xvst $xr11, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [mat] "r"(matrix_argb)
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
      "xvreplgr2vr.h $xr4, %[scl]\n\t"
      "xvreplgr2vr.b $xr5, %[isz]\n\t"
      "xvreplgr2vr.b $xr6, %[ioff]\n\t"
      "xvldi $xr7, 0xff\n\t"
      "xvslli.w $xr7, $xr7, 24\n\t"
      "1:\n\t"
      "xvld $xr0, %[dst], 0\n\t"
      "xvilvl.b $xr1, $xr0, $xr0\n\t"
      "xvilvh.b $xr2, $xr0, $xr0\n\t"
      "xvsrli.h $xr1, $xr1, 8\n\t"
      "xvsrli.h $xr2, $xr2, 8\n\t"
      "xvmuh.hu $xr1, $xr1, $xr4\n\t"
      "xvmuh.hu $xr2, $xr2, $xr4\n\t"
      "xvsrlni.b.h $xr2, $xr1, 0\n\t"
      "xvmul.b $xr2, $xr2, $xr5\n\t"
      "xvadd.b $xr2, $xr2, $xr6\n\t"
      "xvand.v $xr0, $xr0, $xr7\n\t"
      "xvandn.v $xr2, $xr7, $xr2\n\t"
      "xvor.v $xr2, $xr2, $xr0\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [scl] "r"(scale), [isz] "r"(interval_size), [ioff] "r"(interval_offset)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr4", "$xr5", "$xr6", "$xr7");
}
#endif  // HAS_ARGBQUANTIZEROW_LASX

#ifdef HAS_SOBELROW_LASX
void SobelRow_LASX(const uint8_t* src_sobelx,
                   const uint8_t* src_sobely,
                   uint8_t* dst_argb,
                   int width) {
  asm volatile(
      "xvldi $xr4, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sx], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "vld $vr1, %[sy], 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvsadd.bu $xr0, $xr0, $xr1\n\t"
      "xvilvl.b $xr2, $xr0, $xr0\n\t"
      "xvilvl.b $xr3, $xr4, $xr0\n\t"
      "xvilvl.h $xr0, $xr3, $xr2\n\t"
      "xvilvh.h $xr1, $xr3, $xr2\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr0, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr1, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sx], %[sx], 16\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sx] "+r"(src_sobelx), [sy] "+r"(src_sobely), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[sx], 0\n\t"
      "xvld $xr1, %[sy], 0\n\t"
      "xvsadd.bu $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[sx], %[sx], 32\n\t"
      "addi.d %[sy], %[sy], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sx] "+r"(src_sobelx), [sy] "+r"(src_sobely), [dst] "+r"(dst_y),
        [cnt] "+r"(width)
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
      "xvldi $xr4, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sx], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "vld $vr1, %[sy], 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvsadd.bu $xr2, $xr0, $xr1\n\t"
      "xvilvl.b $xr1, $xr2, $xr1\n\t"
      "xvilvl.b $xr0, $xr4, $xr0\n\t"
      "xvilvl.h $xr2, $xr0, $xr1\n\t"
      "xvilvh.h $xr3, $xr0, $xr1\n\t"
      "vst $vr2, %[dst], 0\n\t"
      "xvpermi.q $xr31, $xr2, 0x01\n\t"
      "vst $vr31, %[dst], 32\n\t"
      "vst $vr3, %[dst], 16\n\t"
      "xvpermi.q $xr31, $xr3, 0x01\n\t"
      "vst $vr31, %[dst], 48\n\t"
      "addi.d %[sx], %[sx], 16\n\t"
      "addi.d %[sy], %[sy], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sx] "+r"(src_sobelx), [sy] "+r"(src_sobely), [dst] "+r"(dst_argb),
        [cnt] "+r"(width)
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
        "1:\n\t"
        "xvld $xr0, %[s0], 0\n\t"
        "xvld $xr1, %[s1], 0\n\t"
        "xvavgr.bu $xr0, $xr0, $xr1\n\t"
        "xvst $xr0, %[dst], 0\n\t"
        "addi.d %[s0], %[s0], 32\n\t"
        "addi.d %[s1], %[s1], 32\n\t"
        "addi.d %[dst], %[dst], 32\n\t"
        "addi.d %[cnt], %[cnt], -32\n\t"
        "bnez %[cnt], 1b\n\t"
        : [s0] "+r"(src_ptr), [s1] "+r"(src_ptr1), [dst] "+r"(dst_ptr),
          [cnt] "+r"(width)
        :
        : "memory", "$xr0", "$xr1");
    return;
  }
  uint16_t y_frac = (uint16_t)((256 - y1_fraction) + (y1_fraction << 8));
  asm volatile(
      "xvreplgr2vr.h $xr3, %[yf]\n\t"
      "1:\n\t"
      "xvld $xr0, %[s0], 0\n\t"
      "xvld $xr1, %[s1], 0\n\t"
      "xvilvl.b $xr2, $xr1, $xr0\n\t"
      "xvilvh.b $xr4, $xr1, $xr0\n\t"
      "xvmulwev.h.bu $xr0, $xr2, $xr3\n\t"
      "xvmaddwod.h.bu $xr0, $xr2, $xr3\n\t"
      "xvmulwev.h.bu $xr1, $xr4, $xr3\n\t"
      "xvmaddwod.h.bu $xr1, $xr4, $xr3\n\t"
      "xvsrarni.b.h $xr1, $xr0, 8\n\t"
      "xvst $xr1, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_ptr), [s1] "+r"(src_ptr1), [dst] "+r"(dst_ptr),
        [cnt] "+r"(width)
      : [yf] "r"(y_frac)
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
      "xvldrepl.w $xr3, %[scale], 0\n\t"
      "xvldrepl.w $xr5, %[bias], 0\n\t"
      "xvldi $xr4, 0\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvilvl.h $xr1, $xr4, $xr0\n\t"
      "xvilvh.h $xr2, $xr4, $xr0\n\t"
      "xvffint.s.wu $xr1, $xr1\n\t"
      "xvffint.s.wu $xr2, $xr2\n\t"
      "xvfmul.s $xr1, $xr1, $xr3\n\t"
      "xvfmul.s $xr2, $xr2, $xr3\n\t"
      "xvfmul.s $xr1, $xr1, $xr5\n\t"
      "xvfmul.s $xr2, $xr2, $xr5\n\t"
      "xvsrlni.h.w $xr2, $xr1, 13\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
      : [scale] "r"(&scale), [bias] "r"(&bias)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_HALFFLOATROW_LASX

#ifdef HAS_ARGBTOYMATRIXROW_LASX
void ARGBToYMatrixRow_LASX(const uint8_t* src_argb,
                           uint8_t* dst_y,
                           int width,
                           const struct ArgbConstants* c) {
  asm volatile(
      "xvldrepl.b $xr0, %[c], 0\n\t"
      "xvldrepl.b $xr1, %[c], 1\n\t"
      "xvldrepl.b $xr2, %[c], 2\n\t"
      "xvldrepl.b $xr3, %[c], 3\n\t"
      "xvldrepl.h $xr4, %[c], 96\n\t"
      "1:\n\t"
      "vld $vr5, %[src], 0\n\t"
      "vld $vr31, %[src], 32\n\t"
      "xvpermi.q $xr5, $xr31, 0x02\n\t"
      "vld $vr6, %[src], 16\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr6, $xr31, 0x02\n\t"
      "xvor.v $xr9, $xr4, $xr4\n\t"
      "xvpickev.b $xr7, $xr6, $xr5\n\t"
      "xvpickod.b $xr8, $xr6, $xr5\n\t"
      "xvmaddwev.h.bu $xr9, $xr7, $xr0\n\t"
      "xvmaddwev.h.bu $xr9, $xr8, $xr1\n\t"
      "xvmaddwod.h.bu $xr9, $xr7, $xr2\n\t"
      "xvmaddwod.h.bu $xr9, $xr8, $xr3\n\t"
      "xvpickod.b $xr9, $xr9, $xr9\n\t"
      "xvpermi.d $xr31, $xr9, 0xd8\n\t"
      "vst $vr31, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [c] "r"(c)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr31");
}
#endif  // HAS_ARGBTOYMATRIXROW_LASX

#ifdef HAS_RGBTOYMATRIXROW_LASX
void RGBToYMatrixRow_LASX(const uint8_t* src_rgba,
                          uint8_t* dst_y,
                          int width,
                          const struct ArgbConstants* c) {
  static const uint8_t kShuf[32] = {0, 1, 2,  0,  3,  4, 5,  0,  6,  7, 8,
                                    0, 9, 10, 11, 0,  4, 5,  6,  0,  7, 8,
                                    9, 0, 10, 11, 12, 0, 13, 14, 15, 0};
  asm volatile(
      "xvldrepl.b $xr0, %[c], 0\n\t"
      "xvldrepl.b $xr1, %[c], 1\n\t"
      "xvldrepl.b $xr2, %[c], 2\n\t"
      "xvldrepl.h $xr3, %[c], 96\n\t"
      "vld $vr4, %[shuf], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "vld $vr5, %[shuf], 16\n\t"
      "xvpermi.q $xr5, $xr5, 0x00\n\t"
      "1:\n\t"
      "vld $vr6, %[src], 0\n\t"
      "vld $vr31, %[src], 48\n\t"
      "xvpermi.q $xr6, $xr31, 0x02\n\t"
      "vld $vr7, %[src], 12\n\t"
      "vld $vr31, %[src], 60\n\t"
      "xvpermi.q $xr7, $xr31, 0x02\n\t"
      "vld $vr8, %[src], 24\n\t"
      "vld $vr31, %[src], 72\n\t"
      "xvpermi.q $xr8, $xr31, 0x02\n\t"
      "vld $vr9, %[src], 32\n\t"
      "vld $vr31, %[src], 80\n\t"
      "xvpermi.q $xr9, $xr31, 0x02\n\t"
      "xvshuf.b $xr6, $xr6, $xr6, $xr4\n\t"
      "xvshuf.b $xr7, $xr7, $xr7, $xr4\n\t"
      "xvshuf.b $xr8, $xr8, $xr8, $xr4\n\t"
      "xvshuf.b $xr9, $xr9, $xr9, $xr5\n\t"
      "xvor.v $xr12, $xr3, $xr3\n\t"
      "xvor.v $xr13, $xr3, $xr3\n\t"
      "xvpickev.b $xr10, $xr7, $xr6\n\t"
      "xvpickod.b $xr11, $xr7, $xr6\n\t"
      "xvmaddwev.h.bu $xr12, $xr10, $xr0\n\t"
      "xvmaddwev.h.bu $xr12, $xr11, $xr1\n\t"
      "xvmaddwod.h.bu $xr12, $xr10, $xr2\n\t"
      "xvpickev.b $xr10, $xr9, $xr8\n\t"
      "xvpickod.b $xr11, $xr9, $xr8\n\t"
      "xvmaddwev.h.bu $xr13, $xr10, $xr0\n\t"
      "xvmaddwev.h.bu $xr13, $xr11, $xr1\n\t"
      "xvmaddwod.h.bu $xr13, $xr10, $xr2\n\t"
      "xvpickod.b $xr12, $xr13, $xr12\n\t"
      "xvst $xr12, %[dst], 0\n\t"
      "addi.d %[src], %[src], 96\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_rgba), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [c] "r"(c), [shuf] "r"(kShuf)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12", "$xr13", "$xr31");
}
#endif  // HAS_RGBTOYMATRIXROW_LASX

#ifdef HAS_RGB565TOYROW_LASX
void RGB565ToYRow_LASX(const uint8_t* src_rgb565, uint8_t* dst_y, int width) {
  asm volatile(
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "xvldi $xr7, 63\n\t"
      "xvsrli.h $xr7, $xr7, 8\n\t"
      "xvldi $xr8, 25\n\t"
      "xvsrli.h $xr8, $xr8, 8\n\t"
      "xvldi $xr9, 129\n\t"
      "xvsrli.h $xr9, $xr9, 8\n\t"
      "xvldi $xr10, 66\n\t"
      "xvsrli.h $xr10, $xr10, 8\n\t"
      "xvldi $xr11, 33\n\t"
      "xvsrli.h $xr11, $xr11, 8\n\t"
      "xvslli.h $xr11, $xr11, 7\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr7\n\t"
      "xvsrli.h $xr3, $xr0, 11\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 2\n\t"
      "xvsrli.h $xr2, $xr2, 4\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvmul.h $xr1, $xr1, $xr8\n\t"
      "xvmadd.h $xr1, $xr2, $xr9\n\t"
      "xvmadd.h $xr1, $xr3, $xr10\n\t"
      "xvadd.h $xr1, $xr1, $xr11\n\t"
      "xvpickod.b $xr1, $xr1, $xr1\n\t"
      "xvpermi.d $xr31, $xr1, 0xd8\n\t"
      "vst $vr31, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_rgb565), [dst] "+r"(dst_y), [cnt] "+r"(width)
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
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "xvldi $xr8, 25\n\t"
      "xvsrli.h $xr8, $xr8, 8\n\t"
      "xvldi $xr9, 129\n\t"
      "xvsrli.h $xr9, $xr9, 8\n\t"
      "xvldi $xr10, 66\n\t"
      "xvsrli.h $xr10, $xr10, 8\n\t"
      "xvldi $xr11, 33\n\t"
      "xvsrli.h $xr11, $xr11, 8\n\t"
      "xvslli.h $xr11, $xr11, 7\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvsrli.h $xr3, $xr0, 10\n\t"
      "xvand.v $xr3, $xr3, $xr6\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 3\n\t"
      "xvsrli.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvmul.h $xr1, $xr1, $xr8\n\t"
      "xvmadd.h $xr1, $xr2, $xr9\n\t"
      "xvmadd.h $xr1, $xr3, $xr10\n\t"
      "xvadd.h $xr1, $xr1, $xr11\n\t"
      "xvpickod.b $xr1, $xr1, $xr1\n\t"
      "xvpermi.d $xr31, $xr1, 0xd8\n\t"
      "vst $vr31, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb1555), [dst] "+r"(dst_y), [cnt] "+r"(width)
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
      "xvldi $xr20, 112\n\t"
      "xvsrli.h $xr20, $xr20, 8\n\t"
      "xvldi $xr21, 74\n\t"
      "xvsrli.h $xr21, $xr21, 8\n\t"
      "xvldi $xr22, 38\n\t"
      "xvsrli.h $xr22, $xr22, 8\n\t"
      "xvldi $xr23, 94\n\t"
      "xvsrli.h $xr23, $xr23, 8\n\t"
      "xvldi $xr24, 18\n\t"
      "xvsrli.h $xr24, $xr24, 8\n\t"
      "xvldi $xr25, 128\n\t"
      "xvsrli.h $xr25, $xr25, 8\n\t"
      "xvslli.h $xr25, $xr25, 8\n\t"
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "xvldi $xr7, 63\n\t"
      "xvsrli.h $xr7, $xr7, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr31, %[s0], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr7\n\t"
      "xvsrli.h $xr3, $xr0, 11\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 2\n\t"
      "xvsrli.h $xr2, $xr2, 4\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvor.v $xr10, $xr1, $xr1\n\t"
      "xvor.v $xr11, $xr2, $xr2\n\t"
      "xvor.v $xr12, $xr3, $xr3\n\t"
      "vld $vr0, %[s0], 16\n\t"
      "vld $vr31, %[s0], 48\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr7\n\t"
      "xvsrli.h $xr3, $xr0, 11\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 2\n\t"
      "xvsrli.h $xr2, $xr2, 4\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvpickev.b $xr10, $xr1, $xr10\n\t"
      "xvpickev.b $xr11, $xr2, $xr11\n\t"
      "xvpickev.b $xr12, $xr3, $xr12\n\t"
      "vld $vr0, %[s1], 0\n\t"
      "vld $vr31, %[s1], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr7\n\t"
      "xvsrli.h $xr3, $xr0, 11\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 2\n\t"
      "xvsrli.h $xr2, $xr2, 4\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvor.v $xr13, $xr1, $xr1\n\t"
      "xvor.v $xr14, $xr2, $xr2\n\t"
      "xvor.v $xr15, $xr3, $xr3\n\t"
      "vld $vr0, %[s1], 16\n\t"
      "vld $vr31, %[s1], 48\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr7\n\t"
      "xvsrli.h $xr3, $xr0, 11\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 2\n\t"
      "xvsrli.h $xr2, $xr2, 4\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvpickev.b $xr5, $xr3, $xr15\n\t"
      "xvpickev.b $xr4, $xr2, $xr14\n\t"
      "xvpickev.b $xr3, $xr1, $xr13\n\t"
      "xvor.v $xr0, $xr10, $xr10\n\t"
      "xvor.v $xr1, $xr11, $xr11\n\t"
      "xvor.v $xr2, $xr12, $xr12\n\t"
      "xvhaddw.hu.bu $xr0, $xr0, $xr0\n\t"
      "xvhaddw.hu.bu $xr3, $xr3, $xr3\n\t"
      "xvadd.h $xr0, $xr0, $xr3\n\t"
      "xvsrari.h $xr0, $xr0, 2\n\t"
      "xvhaddw.hu.bu $xr1, $xr1, $xr1\n\t"
      "xvhaddw.hu.bu $xr4, $xr4, $xr4\n\t"
      "xvadd.h $xr1, $xr1, $xr4\n\t"
      "xvsrari.h $xr1, $xr1, 2\n\t"
      "xvhaddw.hu.bu $xr2, $xr2, $xr2\n\t"
      "xvhaddw.hu.bu $xr5, $xr5, $xr5\n\t"
      "xvadd.h $xr2, $xr2, $xr5\n\t"
      "xvsrari.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr8, $xr25, $xr25\n\t"
      "xvor.v $xr9, $xr25, $xr25\n\t"
      "xvmadd.h $xr8, $xr0, $xr20\n\t"
      "xvmsub.h $xr8, $xr1, $xr21\n\t"
      "xvmsub.h $xr8, $xr2, $xr22\n\t"
      "xvmadd.h $xr9, $xr2, $xr20\n\t"
      "xvmsub.h $xr9, $xr1, $xr23\n\t"
      "xvmsub.h $xr9, $xr0, $xr24\n\t"
      "xvpickod.b $xr8, $xr9, $xr8\n\t"
      "xvpermi.d $xr31, $xr8, 0xd8\n\t"
      "vst $vr31, %[du], 0\n\t"
      "vstelm.d $vr8, %[dv], 0, 1\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vstelm.d $vr31, %[dv], 8, 1\n\t"
      "addi.d %[s0], %[s0], 64\n\t"
      "addi.d %[s1], %[s1], 64\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_rgb565), [s1] "+r"(next_rgb565), [du] "+r"(dst_u),
        [dv] "+r"(dst_v), [cnt] "+r"(width)
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
      "xvldi $xr20, 112\n\t"
      "xvsrli.h $xr20, $xr20, 8\n\t"
      "xvldi $xr21, 74\n\t"
      "xvsrli.h $xr21, $xr21, 8\n\t"
      "xvldi $xr22, 38\n\t"
      "xvsrli.h $xr22, $xr22, 8\n\t"
      "xvldi $xr23, 94\n\t"
      "xvsrli.h $xr23, $xr23, 8\n\t"
      "xvldi $xr24, 18\n\t"
      "xvsrli.h $xr24, $xr24, 8\n\t"
      "xvldi $xr25, 128\n\t"
      "xvsrli.h $xr25, $xr25, 8\n\t"
      "xvslli.h $xr25, $xr25, 8\n\t"
      "xvldi $xr6, 31\n\t"
      "xvsrli.h $xr6, $xr6, 8\n\t"
      "1:\n\t"
      "vld $vr0, %[s0], 0\n\t"
      "vld $vr31, %[s0], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvsrli.h $xr3, $xr0, 10\n\t"
      "xvand.v $xr3, $xr3, $xr6\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 3\n\t"
      "xvsrli.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvor.v $xr10, $xr1, $xr1\n\t"
      "xvor.v $xr11, $xr2, $xr2\n\t"
      "xvor.v $xr12, $xr3, $xr3\n\t"
      "vld $vr0, %[s0], 16\n\t"
      "vld $vr31, %[s0], 48\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvsrli.h $xr3, $xr0, 10\n\t"
      "xvand.v $xr3, $xr3, $xr6\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 3\n\t"
      "xvsrli.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvpickev.b $xr10, $xr1, $xr10\n\t"
      "xvpickev.b $xr11, $xr2, $xr11\n\t"
      "xvpickev.b $xr12, $xr3, $xr12\n\t"
      "vld $vr0, %[s1], 0\n\t"
      "vld $vr31, %[s1], 32\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvsrli.h $xr3, $xr0, 10\n\t"
      "xvand.v $xr3, $xr3, $xr6\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 3\n\t"
      "xvsrli.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvor.v $xr13, $xr1, $xr1\n\t"
      "xvor.v $xr14, $xr2, $xr2\n\t"
      "xvor.v $xr15, $xr3, $xr3\n\t"
      "vld $vr0, %[s1], 16\n\t"
      "vld $vr31, %[s1], 48\n\t"
      "xvpermi.q $xr0, $xr31, 0x02\n\t"
      "xvand.v $xr1, $xr0, $xr6\n\t"
      "xvsrli.h $xr2, $xr0, 5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvsrli.h $xr3, $xr0, 10\n\t"
      "xvand.v $xr3, $xr3, $xr6\n\t"
      "xvslli.h $xr4, $xr1, 3\n\t"
      "xvsrli.h $xr1, $xr1, 2\n\t"
      "xvor.v $xr1, $xr4, $xr1\n\t"
      "xvslli.h $xr4, $xr2, 3\n\t"
      "xvsrli.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr2, $xr4, $xr2\n\t"
      "xvslli.h $xr4, $xr3, 3\n\t"
      "xvsrli.h $xr3, $xr3, 2\n\t"
      "xvor.v $xr3, $xr4, $xr3\n\t"
      "xvpickev.b $xr5, $xr3, $xr15\n\t"
      "xvpickev.b $xr4, $xr2, $xr14\n\t"
      "xvpickev.b $xr3, $xr1, $xr13\n\t"
      "xvor.v $xr0, $xr10, $xr10\n\t"
      "xvor.v $xr1, $xr11, $xr11\n\t"
      "xvor.v $xr2, $xr12, $xr12\n\t"
      "xvhaddw.hu.bu $xr0, $xr0, $xr0\n\t"
      "xvhaddw.hu.bu $xr3, $xr3, $xr3\n\t"
      "xvadd.h $xr0, $xr0, $xr3\n\t"
      "xvsrari.h $xr0, $xr0, 2\n\t"
      "xvhaddw.hu.bu $xr1, $xr1, $xr1\n\t"
      "xvhaddw.hu.bu $xr4, $xr4, $xr4\n\t"
      "xvadd.h $xr1, $xr1, $xr4\n\t"
      "xvsrari.h $xr1, $xr1, 2\n\t"
      "xvhaddw.hu.bu $xr2, $xr2, $xr2\n\t"
      "xvhaddw.hu.bu $xr5, $xr5, $xr5\n\t"
      "xvadd.h $xr2, $xr2, $xr5\n\t"
      "xvsrari.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr8, $xr25, $xr25\n\t"
      "xvor.v $xr9, $xr25, $xr25\n\t"
      "xvmadd.h $xr8, $xr0, $xr20\n\t"
      "xvmsub.h $xr8, $xr1, $xr21\n\t"
      "xvmsub.h $xr8, $xr2, $xr22\n\t"
      "xvmadd.h $xr9, $xr2, $xr20\n\t"
      "xvmsub.h $xr9, $xr1, $xr23\n\t"
      "xvmsub.h $xr9, $xr0, $xr24\n\t"
      "xvpickod.b $xr8, $xr9, $xr8\n\t"
      "xvpermi.d $xr31, $xr8, 0xd8\n\t"
      "vst $vr31, %[du], 0\n\t"
      "vstelm.d $vr8, %[dv], 0, 1\n\t"
      "xvpermi.q $xr31, $xr8, 0x01\n\t"
      "vstelm.d $vr31, %[dv], 8, 1\n\t"
      "addi.d %[s0], %[s0], 64\n\t"
      "addi.d %[s1], %[s1], 64\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src_argb1555), [s1] "+r"(next_argb1555), [du] "+r"(dst_u),
        [dv] "+r"(dst_v), [cnt] "+r"(width)
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
      "xvldi $xr20, 112\n\t"
      "xvsrli.h $xr20, $xr20, 8\n\t"
      "xvldi $xr21, 74\n\t"
      "xvsrli.h $xr21, $xr21, 8\n\t"
      "xvldi $xr22, 38\n\t"
      "xvsrli.h $xr22, $xr22, 8\n\t"
      "xvldi $xr23, 94\n\t"
      "xvsrli.h $xr23, $xr23, 8\n\t"
      "xvldi $xr24, 18\n\t"
      "xvsrli.h $xr24, $xr24, 8\n\t"
      "xvldi $xr25, 128\n\t"
      "xvsrli.h $xr25, $xr25, 8\n\t"
      "xvslli.h $xr25, $xr25, 8\n\t"
      "vld $vr16, %[shuf], 0\n\t"
      "xvpermi.q $xr16, $xr16, 0x00\n\t"
      "vld $vr17, %[shuf], 16\n\t"
      "xvpermi.q $xr17, $xr17, 0x00\n\t"
      "1:\n\t"
      "vld $vr8, %[s0], 0\n\t"
      "vld $vr31, %[s0], 48\n\t"
      "xvpermi.q $xr8, $xr31, 0x02\n\t"
      "vld $vr9, %[s0], 12\n\t"
      "vld $vr31, %[s0], 60\n\t"
      "xvpermi.q $xr9, $xr31, 0x02\n\t"
      "vld $vr10, %[s0], 24\n\t"
      "vld $vr31, %[s0], 72\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "vld $vr11, %[s0], 32\n\t"
      "vld $vr31, %[s0], 80\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvshuf.b $xr8, $xr8, $xr8, $xr16\n\t"
      "xvshuf.b $xr9, $xr9, $xr9, $xr16\n\t"
      "xvshuf.b $xr10, $xr10, $xr10, $xr16\n\t"
      "xvshuf.b $xr11, $xr11, $xr11, $xr17\n\t"
      "xvpickev.b $xr12, $xr9, $xr8\n\t"
      "xvpickod.b $xr13, $xr9, $xr8\n\t"
      "xvpickev.b $xr14, $xr11, $xr10\n\t"
      "xvpickod.b $xr15, $xr11, $xr10\n\t"
      "xvpickev.b $xr0, $xr14, $xr12\n\t"
      "xvpickod.b $xr2, $xr14, $xr12\n\t"
      "xvpickev.b $xr1, $xr15, $xr13\n\t"
      "vld $vr8, %[s1], 0\n\t"
      "vld $vr31, %[s1], 48\n\t"
      "xvpermi.q $xr8, $xr31, 0x02\n\t"
      "vld $vr9, %[s1], 12\n\t"
      "vld $vr31, %[s1], 60\n\t"
      "xvpermi.q $xr9, $xr31, 0x02\n\t"
      "vld $vr10, %[s1], 24\n\t"
      "vld $vr31, %[s1], 72\n\t"
      "xvpermi.q $xr10, $xr31, 0x02\n\t"
      "vld $vr11, %[s1], 32\n\t"
      "vld $vr31, %[s1], 80\n\t"
      "xvpermi.q $xr11, $xr31, 0x02\n\t"
      "xvshuf.b $xr8, $xr8, $xr8, $xr16\n\t"
      "xvshuf.b $xr9, $xr9, $xr9, $xr16\n\t"
      "xvshuf.b $xr10, $xr10, $xr10, $xr16\n\t"
      "xvshuf.b $xr11, $xr11, $xr11, $xr17\n\t"
      "xvpickev.b $xr12, $xr9, $xr8\n\t"
      "xvpickod.b $xr13, $xr9, $xr8\n\t"
      "xvpickev.b $xr14, $xr11, $xr10\n\t"
      "xvpickod.b $xr15, $xr11, $xr10\n\t"
      "xvpickev.b $xr3, $xr14, $xr12\n\t"
      "xvpickod.b $xr5, $xr14, $xr12\n\t"
      "xvpickev.b $xr4, $xr15, $xr13\n\t"
      "xvhaddw.hu.bu $xr0, $xr0, $xr0\n\t"
      "xvhaddw.hu.bu $xr3, $xr3, $xr3\n\t"
      "xvadd.h $xr0, $xr0, $xr3\n\t"
      "xvsrari.h $xr0, $xr0, 2\n\t"
      "xvhaddw.hu.bu $xr1, $xr1, $xr1\n\t"
      "xvhaddw.hu.bu $xr4, $xr4, $xr4\n\t"
      "xvadd.h $xr1, $xr1, $xr4\n\t"
      "xvsrari.h $xr1, $xr1, 2\n\t"
      "xvhaddw.hu.bu $xr2, $xr2, $xr2\n\t"
      "xvhaddw.hu.bu $xr5, $xr5, $xr5\n\t"
      "xvadd.h $xr2, $xr2, $xr5\n\t"
      "xvsrari.h $xr2, $xr2, 2\n\t"
      "xvor.v $xr6, $xr25, $xr25\n\t"
      "xvor.v $xr7, $xr25, $xr25\n\t"
      "xvmadd.h $xr6, $xr0, $xr20\n\t"
      "xvmsub.h $xr6, $xr1, $xr21\n\t"
      "xvmsub.h $xr6, $xr2, $xr22\n\t"
      "xvmadd.h $xr7, $xr2, $xr20\n\t"
      "xvmsub.h $xr7, $xr1, $xr23\n\t"
      "xvmsub.h $xr7, $xr0, $xr24\n\t"
      "xvpickod.b $xr6, $xr7, $xr6\n\t"
      "xvpermi.d $xr31, $xr6, 0xd8\n\t"
      "vst $vr31, %[du], 0\n\t"
      "vstelm.d $vr6, %[dv], 0, 1\n\t"
      "xvpermi.q $xr31, $xr6, 0x01\n\t"
      "vstelm.d $vr31, %[dv], 8, 1\n\t"
      "addi.d %[s0], %[s0], 96\n\t"
      "addi.d %[s1], %[s1], 96\n\t"
      "addi.d %[du], %[du], 16\n\t"
      "addi.d %[dv], %[dv], 16\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src), [s1] "+r"(next), [du] "+r"(dst_u), [dv] "+r"(dst_v),
        [cnt] "+r"(width)
      : [shuf] "r"(shuf)
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
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvshuf4i.b $xr0, $xr0, 0xb1\n\t"
      "xvshuf4i.b $xr1, $xr1, 0xb1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "xvst $xr1, %[dst], 32\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uv), [dst] "+r"(dst_vu), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_SWAPUVROW_LASX

#ifdef HAS_ARGBCOPYYTOALPHAROW_LASX
void ARGBCopyYToAlphaRow_LASX(const uint8_t* src, uint8_t* dst, int width) {
  asm volatile(
      "xvldi $xr4, 0xff\n\t"
      "xvslli.w $xr4, $xr4, 24\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "xvld $xr1, %[dst], 0\n\t"
      "xvld $xr2, %[dst], 32\n\t"
      "vilvl.b $vr3, $vr0, $vr0\n\t"
      "vilvh.b $vr0, $vr0, $vr0\n\t"
      "xvpermi.d $xr3, $xr3, 0xd8\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvilvl.h $xr3, $xr3, $xr3\n\t"
      "xvilvl.h $xr0, $xr0, $xr0\n\t"
      "xvslli.w $xr3, $xr3, 24\n\t"
      "xvslli.w $xr0, $xr0, 24\n\t"
      "xvandn.v $xr1, $xr4, $xr1\n\t"
      "xvandn.v $xr2, $xr4, $xr2\n\t"
      "xvor.v $xr1, $xr1, $xr3\n\t"
      "xvor.v $xr2, $xr2, $xr0\n\t"
      "xvst $xr1, %[dst], 0\n\t"
      "xvst $xr2, %[dst], 32\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[su], 0\n\t"
      "xvld $xr1, %[sv], 0\n\t"
      "xvldx $xr2, %[su], %[stu]\n\t"
      "xvldx $xr3, %[sv], %[stv]\n\t"
      "xvhaddw.hu.bu $xr0, $xr0, $xr0\n\t"
      "xvhaddw.hu.bu $xr1, $xr1, $xr1\n\t"
      "xvhaddw.hu.bu $xr2, $xr2, $xr2\n\t"
      "xvhaddw.hu.bu $xr3, $xr3, $xr3\n\t"
      "xvadd.h $xr0, $xr0, $xr2\n\t"
      "xvadd.h $xr1, $xr1, $xr3\n\t"
      "xvsrari.h $xr0, $xr0, 2\n\t"
      "xvsrari.h $xr1, $xr1, 2\n\t"
      "xvpackev.b $xr0, $xr1, $xr0\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[su], %[su], 32\n\t"
      "addi.d %[sv], %[sv], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [su] "+r"(src_u), [sv] "+r"(src_v), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      : [stu] "r"((ptrdiff_t)src_stride_u), [stv] "r"((ptrdiff_t)src_stride_v)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_HALFMERGEUVROW_LASX

#ifdef HAS_HALFUVROW_LASX
void HalfUVRow_LASX(const uint8_t* src_u,
                    int src_stride_u,
                    const uint8_t* src_v,
                    int src_stride_v,
                    uint8_t* dst_uv,
                    int width) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[su], 0\n\t"
      "xvldx $xr2, %[su], %[stu]\n\t"
      "xvavgr.bu $xr0, $xr0, $xr2\n\t"
      "xvld $xr1, %[sv], 0\n\t"
      "xvldx $xr3, %[sv], %[stv]\n\t"
      "xvavgr.bu $xr1, $xr1, $xr3\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvilvl.b $xr2, $xr1, $xr0\n\t"
      "xvilvh.b $xr3, $xr1, $xr0\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "xvst $xr3, %[dst], 32\n\t"
      "addi.d %[su], %[su], 32\n\t"
      "addi.d %[sv], %[sv], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      :
      [su] "+r"(src_u), [sv] "+r"(src_v), [dst] "+r"(dst_uv), [cnt] "+r"(width)
      : [stu] "r"((ptrdiff_t)src_stride_u), [stv] "r"((ptrdiff_t)src_stride_v)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_HALFUVROW_LASX

#ifdef HAS_CONVERT16TO8ROW_LASX
void Convert16To8Row_LASX(const uint16_t* src_y,
                          uint8_t* dst_y,
                          int scale,
                          int width) {
  asm volatile(
      "xvreplgr2vr.h $xr2, %[scl]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvmuh.hu $xr0, $xr0, $xr2\n\t"
      "xvmuh.hu $xr1, $xr1, $xr2\n\t"
      "xvssrani.bu.h $xr1, $xr0, 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvst $xr1, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [scl] "r"(scale)
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
      "xvreplgr2vr.h $xr3, %[shf]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvilvl.b $xr1, $xr0, $xr0\n\t"
      "xvilvh.b $xr2, $xr0, $xr0\n\t"
      "xvsrl.h $xr1, $xr1, $xr3\n\t"
      "xvsrl.h $xr2, $xr2, $xr3\n\t"
      "xvst $xr1, %[dst], 0\n\t"
      "xvst $xr2, %[dst], 32\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [shf] "r"(shift)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_CONVERT8TO16ROW_LASX

#ifdef HAS_MULTIPLYROW_16_LASX
void MultiplyRow_16_LASX(const uint16_t* src_y,
                         uint16_t* dst_y,
                         int scale,
                         int width) {
  asm volatile(
      "xvreplgr2vr.h $xr2, %[scl]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvmul.h $xr0, $xr0, $xr2\n\t"
      "xvmul.h $xr1, $xr1, $xr2\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "xvst $xr1, %[dst], 32\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [scl] "r"(scale)
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
      "xvreplgr2vr.h $xr2, %[scl]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvmuh.hu $xr0, $xr0, $xr2\n\t"
      "xvmuh.hu $xr1, $xr1, $xr2\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "xvst $xr1, %[dst], 32\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [scl] "r"(scale)
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
      "xvreplgr2vr.h $xr4, %[shf]\n\t"
      "1:\n\t"
      "xvld $xr0, %[su], 0\n\t"
      "xvld $xr1, %[sv], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvsll.h $xr0, $xr0, $xr4\n\t"
      "xvsll.h $xr1, $xr1, $xr4\n\t"
      "xvilvl.h $xr2, $xr1, $xr0\n\t"
      "xvilvh.h $xr3, $xr1, $xr0\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "xvst $xr3, %[dst], 32\n\t"
      "addi.d %[su], %[su], 32\n\t"
      "addi.d %[sv], %[sv], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [su] "+r"(src_u), [sv] "+r"(src_v), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      : [shf] "r"(shift)
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
      "xvreplgr2vr.h $xr4, %[shf]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvpickev.h $xr2, $xr1, $xr0\n\t"
      "xvpickod.h $xr3, $xr1, $xr0\n\t"
      "xvpermi.d $xr2, $xr2, 0xd8\n\t"
      "xvpermi.d $xr3, $xr3, 0xd8\n\t"
      "xvsrl.h $xr2, $xr2, $xr4\n\t"
      "xvsrl.h $xr3, $xr3, $xr4\n\t"
      "xvst $xr2, %[du], 0\n\t"
      "xvst $xr3, %[dv], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[du], %[du], 32\n\t"
      "addi.d %[dv], %[dv], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_uv), [du] "+r"(dst_u), [dv] "+r"(dst_v),
        [cnt] "+r"(width)
      : [shf] "r"(shift)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_SPLITUVROW_16_LASX

#ifdef HAS_COPYROW_16_LASX
void CopyRow_16_LASX(const uint16_t* src, uint16_t* dst, int count) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "xvst $xr1, %[dst], 32\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(count)
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
      "1:\n\t"
      "xvld $xr0, %[src0], 0\n\t"
      "xvld $xr1, %[src1], 0\n\t"
      "xvavgr.bu $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 32\n\t"
      "addi.d %[src1], %[src1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_uv), [src1] "+r"(src_uv1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[src0], 0\n\t"
      "xvld $xr1, %[src1], 0\n\t"
      "xvavgr.hu $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 32\n\t"
      "addi.d %[src1], %[src1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_uv), [src1] "+r"(src_uv1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_HALFROW_16_LASX

#ifdef HAS_ARGBCOPYALPHAROW_LASX
void ARGBCopyAlphaRow_LASX(const uint8_t* src, uint8_t* dst, int width) {
  asm volatile(
      "xvldi $xr4, 0xff\n\t"
      "xvslli.w $xr4, $xr4, 24\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvld $xr2, %[dst], 0\n\t"
      "xvld $xr3, %[dst], 32\n\t"
      "xvbitsel.v $xr2, $xr2, $xr0, $xr4\n\t"
      "xvbitsel.v $xr3, $xr3, $xr1, $xr4\n\t"
      "xvst $xr2, %[dst], 0\n\t"
      "xvst $xr3, %[dst], 32\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[src0], 0\n\t"
      "xvld $xr1, %[src0], 32\n\t"
      "xvld $xr2, %[src1], 0\n\t"
      "xvld $xr3, %[src1], 32\n\t"
      "xvpickod.b $xr0, $xr1, $xr0\n\t"
      "xvpickod.b $xr2, $xr3, $xr2\n\t"
      "xvavgr.bu $xr0, $xr0, $xr2\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 64\n\t"
      "addi.d %[src1], %[src1], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_yuy2), [src1] "+r"(src_yuy2_1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_YUY2TONVUVROW_LASX

#ifdef HAS_AR64TOARGBROW_LASX
void AR64ToARGBRow_LASX(const uint16_t* src_ar64,
                        uint8_t* dst_argb,
                        int width) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvpickod.b $xr0, $xr1, $xr0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ar64), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_AR64TOARGBROW_LASX

#ifdef HAS_AB64TOARGBROW_LASX
void AB64ToARGBRow_LASX(const uint16_t* src_ab64,
                        uint8_t* dst_argb,
                        int width) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvpickod.b $xr0, $xr1, $xr0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvshuf4i.b $xr0, $xr0, 0xc6\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ab64), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1");
}
#endif  // HAS_AB64TOARGBROW_LASX

#ifdef HAS_ARGBTOAR64ROW_LASX
void ARGBToAR64Row_LASX(const uint8_t* src_argb,
                        uint16_t* dst_ar64,
                        int width) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvilvl.b $xr1, $xr0, $xr0\n\t"
      "xvilvh.b $xr2, $xr0, $xr0\n\t"
      "xvst $xr1, %[dst], 0\n\t"
      "xvst $xr2, %[dst], 32\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_ar64), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2");
}
#endif  // HAS_ARGBTOAR64ROW_LASX

#ifdef HAS_ARGBTOAB64ROW_LASX
void ARGBToAB64Row_LASX(const uint8_t* src_argb,
                        uint16_t* dst_ab64,
                        int width) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvshuf4i.b $xr0, $xr0, 0xc6\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvilvl.b $xr1, $xr0, $xr0\n\t"
      "xvilvh.b $xr2, $xr0, $xr0\n\t"
      "xvst $xr1, %[dst], 0\n\t"
      "xvst $xr2, %[dst], 32\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_ab64), [cnt] "+r"(width)
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
      "xvreplgr2vr.b $xr2, %[scl]\n\t"
      "xvreplgr2vr.b $xr3, %[bia]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvmuh.bu $xr0, $xr0, $xr2\n\t"
      "xvadd.b $xr0, $xr0, $xr3\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_y), [dst] "+r"(dst_y), [cnt] "+r"(width)
      : [scl] "r"(scale), [bia] "r"(bias)
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
      "xvreplgr2vr.h $xr4, %[scl]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src0], 0\n\t"
      "xvld $xr1, %[src0], 32\n\t"
      "xvld $xr2, %[src1], 0\n\t"
      "xvld $xr3, %[src1], 32\n\t"
      "xvavgr.hu $xr0, $xr0, $xr2\n\t"
      "xvavgr.hu $xr1, $xr1, $xr3\n\t"
      "xvmuh.hu $xr0, $xr0, $xr4\n\t"
      "xvmuh.hu $xr1, $xr1, $xr4\n\t"
      "xvssrani.bu.h $xr1, $xr0, 0\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvst $xr1, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 64\n\t"
      "addi.d %[src1], %[src1], 64\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_uv), [src1] "+r"(src_uv1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      : [scl] "r"(scale)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_HALFROW_16TO8_LASX

#ifdef HAS_AR30TOAB30ROW_LASX
void AR30ToAB30Row_LASX(const uint8_t* src_ar30,
                        uint8_t* dst_ab30,
                        int width) {
  uint32_t m_ag = 0xc00ffc00u;
  uint32_t m_r = 0x3ff00000u;
  uint32_t m_b = 0x000003ffu;
  asm volatile(
      "xvreplgr2vr.w $xr3, %[m_ag]\n\t"
      "xvreplgr2vr.w $xr4, %[m_r]\n\t"
      "xvreplgr2vr.w $xr5, %[m_b]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvsrli.w $xr1, $xr0, 20\n\t"
      "xvslli.w $xr2, $xr0, 20\n\t"
      "xvand.v $xr0, $xr0, $xr3\n\t"
      "xvand.v $xr1, $xr1, $xr5\n\t"
      "xvand.v $xr2, $xr2, $xr4\n\t"
      "xvor.v $xr0, $xr0, $xr1\n\t"
      "xvor.v $xr0, $xr0, $xr2\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ar30), [dst] "+r"(dst_ab30), [cnt] "+r"(width)
      : [m_ag] "r"(m_ag), [m_r] "r"(m_r), [m_b] "r"(m_b)
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
      "xvreplgr2vr.w $xr4, %[m_a]\n\t"
      "xvreplgr2vr.w $xr5, %[m_r]\n\t"
      "xvreplgr2vr.w $xr6, %[m_g]\n\t"
      "xvreplgr2vr.w $xr7, %[m_b]\n\t"
      "xvreplgr2vr.w $xr8, %[m_l]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvslli.w $xr1, $xr0, 6\n\t"
      "xvslli.w $xr2, $xr0, 4\n\t"
      "xvslli.w $xr3, $xr0, 2\n\t"
      "xvand.v $xr0, $xr0, $xr4\n\t"
      "xvand.v $xr1, $xr1, $xr5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvand.v $xr3, $xr3, $xr7\n\t"
      "xvor.v $xr0, $xr0, $xr1\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvor.v $xr0, $xr0, $xr2\n\t"
      "xvsrli.w $xr1, $xr0, 8\n\t"
      "xvand.v $xr1, $xr1, $xr8\n\t"
      "xvor.v $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [dst] "+r"(dst_ar30), [cnt] "+r"(width)
      : [m_a] "r"(m_a), [m_r] "r"(m_r), [m_g] "r"(m_g), [m_b] "r"(m_b),
        [m_l] "r"(m_l)
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
      "xvreplgr2vr.w $xr4, %[m_a]\n\t"
      "xvreplgr2vr.w $xr5, %[m_r]\n\t"
      "xvreplgr2vr.w $xr6, %[m_g]\n\t"
      "xvreplgr2vr.w $xr7, %[m_b]\n\t"
      "xvreplgr2vr.w $xr8, %[m_l]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvshuf4i.b $xr0, $xr0, 0xc6\n\t"
      "xvslli.w $xr1, $xr0, 6\n\t"
      "xvslli.w $xr2, $xr0, 4\n\t"
      "xvslli.w $xr3, $xr0, 2\n\t"
      "xvand.v $xr0, $xr0, $xr4\n\t"
      "xvand.v $xr1, $xr1, $xr5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvand.v $xr3, $xr3, $xr7\n\t"
      "xvor.v $xr0, $xr0, $xr1\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvor.v $xr0, $xr0, $xr2\n\t"
      "xvsrli.w $xr1, $xr0, 8\n\t"
      "xvand.v $xr1, $xr1, $xr8\n\t"
      "xvor.v $xr0, $xr0, $xr1\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_abgr), [dst] "+r"(dst_ar30), [cnt] "+r"(width)
      : [m_a] "r"(m_a), [m_r] "r"(m_r), [m_g] "r"(m_g), [m_b] "r"(m_b),
        [m_l] "r"(m_l)
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
      "xvreplgr2vr.w $xr4, %[m_a]\n\t"
      "xvreplgr2vr.w $xr5, %[m_2]\n\t"
      "xvreplgr2vr.w $xr6, %[m_4]\n\t"
      "xvreplgr2vr.w $xr7, %[m_6]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvsrli.w $xr1, $xr0, 2\n\t"
      "xvsrli.w $xr2, $xr0, 4\n\t"
      "xvsrli.w $xr3, $xr0, 6\n\t"
      "xvand.v $xr0, $xr0, $xr4\n\t"
      "xvand.v $xr1, $xr1, $xr5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvand.v $xr3, $xr3, $xr7\n\t"
      "xvor.v $xr0, $xr0, $xr1\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvor.v $xr0, $xr0, $xr2\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ar30), [dst] "+r"(dst_argb), [cnt] "+r"(width)
      : [m_a] "r"(m_a), [m_2] "r"(m_2), [m_4] "r"(m_4), [m_6] "r"(m_6)
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
      "xvreplgr2vr.w $xr4, %[m_a]\n\t"
      "xvreplgr2vr.w $xr5, %[m_2]\n\t"
      "xvreplgr2vr.w $xr6, %[m_4]\n\t"
      "xvreplgr2vr.w $xr7, %[m_6]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvsrli.w $xr1, $xr0, 2\n\t"
      "xvsrli.w $xr2, $xr0, 4\n\t"
      "xvsrli.w $xr3, $xr0, 6\n\t"
      "xvand.v $xr0, $xr0, $xr4\n\t"
      "xvand.v $xr1, $xr1, $xr5\n\t"
      "xvand.v $xr2, $xr2, $xr6\n\t"
      "xvand.v $xr3, $xr3, $xr7\n\t"
      "xvor.v $xr0, $xr0, $xr1\n\t"
      "xvor.v $xr2, $xr2, $xr3\n\t"
      "xvor.v $xr0, $xr0, $xr2\n\t"
      "xvshuf4i.b $xr0, $xr0, 0xc6\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -8\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ar30), [dst] "+r"(dst_abgr), [cnt] "+r"(width)
      : [m_a] "r"(m_a), [m_2] "r"(m_2), [m_4] "r"(m_4), [m_6] "r"(m_6)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_AR30TOABGRROW_LASX

#ifdef HAS_AYUVTOYROW_LASX
void AYUVToYRow_LASX(const uint8_t* src_ayuv, uint8_t* dst_y, int width) {
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvpickev.h $xr0, $xr1, $xr0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvpickev.b $xr0, $xr0, $xr0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_ayuv), [dst] "+r"(dst_y), [cnt] "+r"(width)
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
      "xvldi $xr6, 0x02\n\t"
      "1:\n\t"
      "xvld $xr0, %[src0], 0\n\t"
      "xvld $xr1, %[src0], 32\n\t"
      "xvld $xr2, %[src1], 0\n\t"
      "xvld $xr3, %[src1], 32\n\t"
      "xvaddwev.h.bu $xr4, $xr0, $xr2\n\t"
      "xvaddwod.h.bu $xr0, $xr0, $xr2\n\t"
      "xvaddwev.h.bu $xr5, $xr1, $xr3\n\t"
      "xvaddwod.h.bu $xr1, $xr1, $xr3\n\t"
      "xvpickev.d $xr2, $xr5, $xr4\n\t"
      "xvpickod.d $xr3, $xr5, $xr4\n\t"
      "xvpickev.d $xr4, $xr1, $xr0\n\t"
      "xvpickod.d $xr5, $xr1, $xr0\n\t"
      "xvadd.h $xr2, $xr2, $xr3\n\t"
      "xvadd.h $xr4, $xr4, $xr5\n\t"
      "xvadd.h $xr2, $xr2, $xr6\n\t"
      "xvadd.h $xr4, $xr4, $xr6\n\t"
      "xvsrli.h $xr2, $xr2, 2\n\t"
      "xvsrli.h $xr4, $xr4, 2\n\t"
      "xvpickev.b $xr0, $xr4, $xr2\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvshuf4i.b $xr0, $xr0, 0xb1\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 64\n\t"
      "addi.d %[src1], %[src1], 64\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_ayuv), [src1] "+r"(src_ayuv1), [dst] "+r"(dst_uv),
        [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_AYUVTOUVROW_LASX

#ifdef HAS_AYUVTOVUROW_LASX
void AYUVToVURow_LASX(const uint8_t* src_ayuv,
                      int src_stride_ayuv,
                      uint8_t* dst_vu,
                      int width) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  asm volatile(
      "xvldi $xr6, 0x02\n\t"
      "1:\n\t"
      "xvld $xr0, %[src0], 0\n\t"
      "xvld $xr1, %[src0], 32\n\t"
      "xvld $xr2, %[src1], 0\n\t"
      "xvld $xr3, %[src1], 32\n\t"
      "xvaddwev.h.bu $xr4, $xr0, $xr2\n\t"
      "xvaddwod.h.bu $xr0, $xr0, $xr2\n\t"
      "xvaddwev.h.bu $xr5, $xr1, $xr3\n\t"
      "xvaddwod.h.bu $xr1, $xr1, $xr3\n\t"
      "xvpickev.d $xr2, $xr5, $xr4\n\t"
      "xvpickod.d $xr3, $xr5, $xr4\n\t"
      "xvpickev.d $xr4, $xr1, $xr0\n\t"
      "xvpickod.d $xr5, $xr1, $xr0\n\t"
      "xvadd.h $xr2, $xr2, $xr3\n\t"
      "xvadd.h $xr4, $xr4, $xr5\n\t"
      "xvadd.h $xr2, $xr2, $xr6\n\t"
      "xvadd.h $xr4, $xr4, $xr6\n\t"
      "xvsrli.h $xr2, $xr2, 2\n\t"
      "xvsrli.h $xr4, $xr4, 2\n\t"
      "xvpickev.b $xr0, $xr4, $xr2\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 64\n\t"
      "addi.d %[src1], %[src1], 64\n\t"
      "addi.d %[dst], %[dst], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_ayuv), [src1] "+r"(src_ayuv1), [dst] "+r"(dst_vu),
        [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6");
}
#endif  // HAS_AYUVTOVUROW_LASX

#ifdef HAS_DETILEROW_LASX
void DetileRow_LASX(const uint8_t* src,
                    ptrdiff_t src_tile_stride,
                    uint8_t* dst,
                    int width) {
  const uint8_t* src1 = src + src_tile_stride;
  ptrdiff_t src_tile_stride2 = src_tile_stride * 2;
  asm volatile(
      "li.d $t0, 32\n\t"
      "blt %[cnt], $t0, 2f\n\t"
      "1:\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vld $vr1, %[src1], 0\n\t"
      "xvpermi.q $xr0, $xr1, 0x20\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "add.d %[src0], %[src0], %[str2]\n\t"
      "add.d %[src1], %[src1], %[str2]\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bge %[cnt], $t0, 1b\n\t"
      "2:\n\t"
      "beqz %[cnt], 3f\n\t"
      "vld $vr0, %[src0], 0\n\t"
      "vst $vr0, %[dst], 0\n\t"
      "3:\n\t"
      : [src0] "+r"(src), [src1] "+r"(src1), [dst] "+r"(dst), [cnt] "+r"(width)
      : [str2] "r"(src_tile_stride2)
      : "memory", "$t0", "$xr0", "$xr1");
}
#endif  // HAS_DETILEROW_LASX

#ifdef HAS_DETILEROW_16_LASX
void DetileRow_16_LASX(const uint16_t* src,
                       ptrdiff_t src_tile_stride,
                       uint16_t* dst,
                       int width) {
  ptrdiff_t byte_stride = src_tile_stride * 2;
  asm volatile(
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "add.d %[src], %[src], %[str]\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [dst] "+r"(dst), [cnt] "+r"(width)
      : [str] "r"(byte_stride)
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
      "1:\n\t"
      "vld $vr0, %[sb], 0\n\t"
      "vld $vr1, %[sg], 0\n\t"
      "vld $vr2, %[sr], 0\n\t"
      "vld $vr3, %[sa], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvpermi.d $xr2, $xr2, 0xd8\n\t"
      "xvpermi.d $xr3, $xr3, 0xd8\n\t"
      "xvilvl.b $xr4, $xr1, $xr0\n\t"
      "xvilvl.b $xr5, $xr3, $xr2\n\t"
      "xvpermi.d $xr4, $xr4, 0xd8\n\t"
      "xvpermi.d $xr5, $xr5, 0xd8\n\t"
      "xvilvl.h $xr0, $xr5, $xr4\n\t"
      "xvilvh.h $xr1, $xr5, $xr4\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "xvst $xr1, %[dst], 32\n\t"
      "addi.d %[sb], %[sb], 16\n\t"
      "addi.d %[sg], %[sg], 16\n\t"
      "addi.d %[sr], %[sr], 16\n\t"
      "addi.d %[sa], %[sa], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sb] "+r"(src_b), [sg] "+r"(src_g), [sr] "+r"(src_r),
        [sa] "+r"(src_a), [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "xvldi $xr3, 0xff\n\t"
      "1:\n\t"
      "vld $vr0, %[sb], 0\n\t"
      "vld $vr1, %[sg], 0\n\t"
      "vld $vr2, %[sr], 0\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvpermi.d $xr2, $xr2, 0xd8\n\t"
      "xvilvl.b $xr4, $xr1, $xr0\n\t"
      "xvilvl.b $xr5, $xr3, $xr2\n\t"
      "xvpermi.d $xr4, $xr4, 0xd8\n\t"
      "xvpermi.d $xr5, $xr5, 0xd8\n\t"
      "xvilvl.h $xr0, $xr5, $xr4\n\t"
      "xvilvh.h $xr1, $xr5, $xr4\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "xvst $xr1, %[dst], 32\n\t"
      "addi.d %[sb], %[sb], 16\n\t"
      "addi.d %[sg], %[sg], 16\n\t"
      "addi.d %[sr], %[sr], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sb] "+r"(src_b), [sg] "+r"(src_g), [sr] "+r"(src_r),
        [dst] "+r"(dst_argb), [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvpermi.d $xr2, $xr2, 0xd8\n\t"
      "xvpermi.d $xr3, $xr3, 0xd8\n\t"
      "xvpickev.b $xr0, $xr2, $xr2\n\t"
      "xvpickod.b $xr1, $xr2, $xr2\n\t"
      "xvpickev.b $xr4, $xr3, $xr3\n\t"
      "xvpickod.b $xr5, $xr3, $xr3\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvpermi.d $xr4, $xr4, 0xd8\n\t"
      "xvpermi.d $xr5, $xr5, 0xd8\n\t"
      "vst $vr0, %[db], 0\n\t"
      "vst $vr4, %[dg], 0\n\t"
      "vst $vr1, %[dr], 0\n\t"
      "vst $vr5, %[da], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[db], %[db], 16\n\t"
      "addi.d %[dg], %[dg], 16\n\t"
      "addi.d %[dr], %[dr], 16\n\t"
      "addi.d %[da], %[da], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [db] "+r"(dst_b), [dg] "+r"(dst_g),
        [dr] "+r"(dst_r), [da] "+r"(dst_a), [cnt] "+r"(width)
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
      "1:\n\t"
      "xvld $xr0, %[src], 0\n\t"
      "xvld $xr1, %[src], 32\n\t"
      "xvpickev.b $xr2, $xr1, $xr0\n\t"
      "xvpickod.b $xr3, $xr1, $xr0\n\t"
      "xvpermi.d $xr2, $xr2, 0xd8\n\t"
      "xvpermi.d $xr3, $xr3, 0xd8\n\t"
      "xvpickev.b $xr0, $xr2, $xr2\n\t"
      "xvpickod.b $xr1, $xr2, $xr2\n\t"
      "xvpickev.b $xr4, $xr3, $xr3\n\t"
      "xvpermi.d $xr0, $xr0, 0xd8\n\t"
      "xvpermi.d $xr1, $xr1, 0xd8\n\t"
      "xvpermi.d $xr4, $xr4, 0xd8\n\t"
      "vst $vr0, %[db], 0\n\t"
      "vst $vr4, %[dg], 0\n\t"
      "vst $vr1, %[dr], 0\n\t"
      "addi.d %[src], %[src], 64\n\t"
      "addi.d %[db], %[db], 16\n\t"
      "addi.d %[dg], %[dg], 16\n\t"
      "addi.d %[dr], %[dr], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_argb), [db] "+r"(dst_b), [dg] "+r"(dst_g),
        [dr] "+r"(dst_r), [cnt] "+r"(width)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
}
#endif  // HAS_SPLITXRGBROW_LASX

#ifdef HAS_RGB24MIRRORROW_LASX
static const uvec8 kRGB24MirrorShuf0_LASX = {13, 14, 15, 10, 11, 12, 7, 8,
                                             9,  4,  5,  6,  1,  2,  3, 0};
static const uvec8 kRGB24MirrorShuf1_LASX = {14, 15, 16, 11, 12, 13, 8, 9,
                                             10, 5,  6,  7,  2,  3,  4, 1};
static const uvec8 kRGB24MirrorShuf2_LASX = {15, 0,  1, 12, 13, 14, 9, 10,
                                             11, 6,  7, 8,  3,  4,  5, 2};
void RGB24MirrorRow_LASX(const uint8_t* src_rgb24,
                         uint8_t* dst_rgb24,
                         int width) {
  src_rgb24 += width * 3 - 96;
  asm volatile(
      "vld $vr4, %[sh0], 0\n\t"
      "vld $vr5, %[sh1], 0\n\t"
      "vld $vr6, %[sh2], 0\n\t"
      "xvpermi.q $xr4, $xr4, 0x00\n\t"
      "xvpermi.q $xr5, $xr5, 0x00\n\t"
      "xvpermi.q $xr6, $xr6, 0x00\n\t"
      "1:\n\t"
      "vld $vr2, %[src], 80\n\t"
      "vld $vr1, %[src], 64\n\t"
      "vld $vr0, %[src], 48\n\t"
      "vld $vr7, %[src], 32\n\t"
      "vld $vr8, %[src], 16\n\t"
      "vld $vr9, %[src], 0\n\t"
      "xvpermi.q $xr2, $xr7, 0x20\n\t"
      "xvpermi.q $xr1, $xr8, 0x20\n\t"
      "xvpermi.q $xr0, $xr9, 0x20\n\t"
      "xvshuf.b $xr3, $xr2, $xr1, $xr4\n\t"
      "xvshuf.b $xr1, $xr1, $xr0, $xr5\n\t"
      "xvshuf.b $xr0, $xr0, $xr0, $xr6\n\t"
      "vst $vr3, %[dst], 0\n\t"
      "vst $vr1, %[dst], 16\n\t"
      "vst $vr0, %[dst], 32\n\t"
      "xvpermi.q $xr3, $xr3, 0x01\n\t"
      "xvpermi.q $xr1, $xr1, 0x01\n\t"
      "xvpermi.q $xr0, $xr0, 0x01\n\t"
      "vst $vr3, %[dst], 48\n\t"
      "vst $vr1, %[dst], 64\n\t"
      "vst $vr0, %[dst], 80\n\t"
      "addi.d %[src], %[src], -96\n\t"
      "addi.d %[dst], %[dst], 96\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src_rgb24), [dst] "+r"(dst_rgb24), [cnt] "+r"(width)
      : [sh0] "r"(&kRGB24MirrorShuf0_LASX), [sh1] "r"(&kRGB24MirrorShuf1_LASX),
        [sh2] "r"(&kRGB24MirrorShuf2_LASX)
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
      "xvldi $xr6, 0x7f\n\t"
      "xvslli.h $xr6, $xr6, 1\n\t"
      "1:\n\t"
      "xvld $xr0, %[s0], 0\n\t"
      "xvld $xr1, %[s1], 0\n\t"
      "xvld $xr2, %[alp], 0\n\t"
      "xvxori.b $xr3, $xr2, 0xff\n\t"
      "xvmulwev.h.bu $xr4, $xr0, $xr2\n\t"
      "xvmaddwev.h.bu $xr4, $xr1, $xr3\n\t"
      "xvmulwod.h.bu $xr5, $xr0, $xr2\n\t"
      "xvmaddwod.h.bu $xr5, $xr1, $xr3\n\t"
      "xvadd.h $xr4, $xr4, $xr6\n\t"
      "xvadd.h $xr5, $xr5, $xr6\n\t"
      "xvsrli.h $xr4, $xr4, 8\n\t"
      "xvsrli.h $xr5, $xr5, 8\n\t"
      "xvpackev.b $xr0, $xr5, $xr4\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "addi.d %[s0], %[s0], 32\n\t"
      "addi.d %[s1], %[s1], 32\n\t"
      "addi.d %[alp], %[alp], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [s0] "+r"(src0), [s1] "+r"(src1), [alp] "+r"(alpha), [dst] "+r"(dst),
        [cnt] "+r"(width)
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
        "1:\n\t"
        "xvld $xr0, %[src0], 0\n\t"
        "xvst $xr0, %[dst], 0\n\t"
        "addi.d %[src0], %[src0], 32\n\t"
        "addi.d %[dst], %[dst], 32\n\t"
        "addi.d %[cnt], %[cnt], -16\n\t"
        "bnez %[cnt], 1b\n\t"
        : [src0] "+r"(src_ptr), [dst] "+r"(dst_ptr), [cnt] "+r"(width)
        :
        : "memory", "$xr0");
    return;
  }
  if (y1_fraction == 128) {
    HalfRow_16_LASX(src_ptr, src_stride, dst_ptr, width);
    return;
  }
  asm volatile(
      "xvreplgr2vr.h $xr4, %[f0]\n\t"
      "xvreplgr2vr.h $xr5, %[f1]\n\t"
      "1:\n\t"
      "xvld $xr0, %[src0], 0\n\t"
      "xvld $xr1, %[src1], 0\n\t"
      "xvmulwev.w.hu $xr2, $xr0, $xr4\n\t"
      "xvmaddwev.w.hu $xr2, $xr1, $xr5\n\t"
      "xvmulwod.w.hu $xr3, $xr0, $xr4\n\t"
      "xvmaddwod.w.hu $xr3, $xr1, $xr5\n\t"
      "xvsrari.w $xr2, $xr2, 8\n\t"
      "xvsrari.w $xr3, $xr3, 8\n\t"
      "xvpackev.h $xr3, $xr3, $xr2\n\t"
      "xvst $xr3, %[dst], 0\n\t"
      "addi.d %[src0], %[src0], 32\n\t"
      "addi.d %[src1], %[src1], 32\n\t"
      "addi.d %[dst], %[dst], 32\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src0] "+r"(src_ptr), [src1] "+r"(src_ptr1), [dst] "+r"(dst_ptr),
        [cnt] "+r"(width)
      : [f0] "r"(y0_fraction), [f1] "r"(y1_fraction)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_INTERPOLATEROW_16_LASX

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // (!defined(LIBYUV_DISABLE_LSX) || !defined(LIBYUV_DISABLE_LASX)) &&
        // defined(__loongarch_lp64)

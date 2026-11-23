/*
 *  Copyright 2023 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * Copyright (c) 2023 SiFive, Inc. All rights reserved.
 *
 * Contributed by Darren Hsieh <darren.hsieh@sifive.com>
 * Contributed by Bruce Lai <bruce.lai@sifive.com>
 */

#include "libyuv/compare_row.h"
#include "libyuv/row.h"
#include "libyuv/video_common.h"

// Note: every register written by an inline asm block (vector, general
// purpose, mask/predicate, "cc" for flags and "memory" for stores) must be
// listed in its clobber list, even when the ABI treats it as caller-saved.
// With LTO/LTCG the compiler may keep values live in any register across the
// asm statement, and other OS ABIs (e.g. Windows) differ in which registers
// are callee-saved. List "memory" first, then "cc", then registers.

// Enable LIBYUV_USE_VLSEG for CPUs that prefer vlseg/vsseg over unit-stride
// loads/stores.
// SiFive X280 is slow with vlseg and vsseg instructions.

// This module is for RVV (RISC-V Vector extension)
#if !defined(LIBYUV_DISABLE_RVV) && defined(__riscv_vector)
#include <assert.h>

// vsetvli/vsetivli write the vl and vtype CSRs. Only "vl" is declared, and
// only for GCC; clang historically rejected it. The "vtype" register is
// intentionally not declared, and whether it should be is unresolved.
// TODO(fbarchard): consider adding "vtype" (and "vl" for clang) to the
// clobber list once confirmed with compiler maintainers.
#ifndef RVV_VL_CLOBBER
#ifdef __clang__
#define RVV_VL_CLOBBER
#else
#define RVV_VL_CLOBBER "vl",
#endif
#endif

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Fill YUV -> RGB conversion constants into registers
// NOTE: To match behavior on other platforms, vxrm (fixed-point rounding mode
// register) is set to round-to-nearest-up mode(0).
#define YUVTORGB_SETUP                   \
  "csrwi       vxrm, 0               \n" \
  "lbu         %[ub], 0(%[yuvconst]) \n" \
  "lbu         %[vr], 1(%[yuvconst]) \n" \
  "lbu         %[ug], 2(%[yuvconst]) \n" \
  "lbu         %[vg], 3(%[yuvconst]) \n" \
  "lhu         %[yg], 16(%[yuvconst])\n" \
  "lh          %[bb], 18(%[yuvconst])\n" \
  "lh          %[bg], 20(%[yuvconst])\n" \
  "lh          %[br], 22(%[yuvconst])\n" \
  "addi        %[bb], %[bb], 32      \n" \
  "addi        %[bg], %[bg], -32     \n" \
  "addi        %[br], %[br], 32      \n" \
  "li          %[k0101], 0x0101      \n"

// Fill YUV -> AR30 conversion constants into registers
#define YUVTORGB_SETUP_AR30              \
  "csrwi       vxrm, 0               \n" \
  "lbu         %[ub], 0(%[yuvconst]) \n" \
  "lbu         %[vr], 1(%[yuvconst]) \n" \
  "lbu         %[ug], 2(%[yuvconst]) \n" \
  "lbu         %[vg], 3(%[yuvconst]) \n" \
  "lhu         %[yg], 16(%[yuvconst])\n" \
  "lh          %[bb], 18(%[yuvconst])\n" \
  "lh          %[bg], 20(%[yuvconst])\n" \
  "lh          %[br], 22(%[yuvconst])\n" \
  "addi        %[bb], %[bb], 24      \n" \
  "addi        %[bg], %[bg], -24     \n" \
  "addi        %[br], %[br], 24      \n" \
  "li          %[k0101], 0x0101      \n"

// Read [2*VLEN/8] Y, [VLEN/8] U and [VLEN/8] V from 422
#define READYUV422                               \
  "addi        %[vl], %[w], 1                \n" \
  "srli        %[vl], %[vl], 1               \n" \
  "vsetvli     %[vl], %[vl], e8, m1, ta, ma  \n" \
  "vle8.v      v0, (%[src_u])                \n" \
  "vle8.v      v1, (%[src_v])                \n" \
  "vwcvtu.x.x.v v2, v0                       \n" \
  "vwcvtu.x.x.v v4, v1                       \n" \
  "vsetvli     zero, zero, e16, m2, ta, ma   \n" \
  "vmul.vx     v2, v2, %[k0101]              \n" \
  "vmul.vx     v4, v4, %[k0101]              \n" \
  "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n" \
  "vle8.v      v0, (%[src_y])                \n" \
  "vwcvtu.x.x.v v8, v0                       \n"

// Read [2*VLEN/8] Y, [2*VLEN/8] U, and [2*VLEN/8] V from 444
#define READYUV444                               \
  "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n" \
  "vle8.v      v0, (%[src_y])                \n" \
  "vle8.v      v2, (%[src_u])                \n" \
  "vle8.v      v4, (%[src_v])                \n" \
  "vwcvtu.x.x.v v8, v0                       \n"

// Read [2*VLEN/8] Y from src_y; Read [VLEN/8] U and [VLEN/8] V from src_uv
#define READNV12                                 \
  "addi        %[vl], %[w], 1                \n" \
  "srli        %[vl], %[vl], 1               \n" \
  "vsetvli     %[vl], %[vl], e8, m1, ta, ma  \n" \
  "vlseg2e8.v  v0, (%[src_uv])               \n" \
  "vwcvtu.x.x.v v2, v0                       \n" \
  "vwcvtu.x.x.v v4, v1                       \n" \
  "vsetvli     zero, zero, e16, m2, ta, ma   \n" \
  "vmul.vx     v2, v2, %[k0101]              \n" \
  "vmul.vx     v4, v4, %[k0101]              \n" \
  "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n" \
  "vle8.v      v0, (%[src_y])                \n" \
  "vwcvtu.x.x.v v8, v0                       \n"

// Read 2*[VLEN/8] Y from src_y; Read [VLEN/8] U and [VLEN/8] V from src_vu
#define READNV21                                 \
  "addi        %[vl], %[w], 1                \n" \
  "srli        %[vl], %[vl], 1               \n" \
  "vsetvli     %[vl], %[vl], e8, m1, ta, ma  \n" \
  "vlseg2e8.v  v0, (%[src_vu])               \n" \
  "vwcvtu.x.x.v v4, v0                       \n" \
  "vwcvtu.x.x.v v2, v1                       \n" \
  "vsetvli     zero, zero, e16, m2, ta, ma   \n" \
  "vmul.vx     v2, v2, %[k0101]              \n" \
  "vmul.vx     v4, v4, %[k0101]              \n" \
  "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n" \
  "vle8.v      v0, (%[src_y])                \n" \
  "vwcvtu.x.x.v v8, v0                       \n"

// Convert from YUV to fixed point RGB
#define YUVTORGB                                 \
  "vwmulu.vx   v12, v2, %[ug]                \n" \
  "vwmaccu.vx  v12, %[vg], v4                \n" \
  "vwmulu.vx   v0, v2, %[ub]                 \n" \
  "vsetvli     zero, zero, e16, m4, ta, ma   \n" \
  "vmul.vx     v8, v8, %[k0101]              \n" \
  "vmulhu.vx   v8, v8, %[yg]                 \n" \
  "vadd.vx     v16, v8, %[bg]                \n" \
  "vadd.vv     v0, v8, v0                    \n" \
  "vsetvli     zero, zero, e8, m2, ta, ma    \n" \
  "vwmaccu.vx  v8, %[vr], v4                 \n" \
  "vsetvli     zero, zero, e16, m4, ta, ma   \n" \
  "vssubu.vv   v12, v16, v12                 \n" \
  "vssubu.vx   v0, v0, %[bb]                 \n" \
  "vssubu.vx   v8, v8, %[br]                 \n"

// Convert from fixed point RGB To 8 bit RGB
#define RGBTORGB8                                \
  "vsetvli     zero, zero, e8, m2, ta, ma    \n" \
  "vnclipu.wi  v26, v12, 6                   \n" \
  "vnclipu.wi  v24, v0, 6                    \n" \
  "vnclipu.wi  v28, v8, 6                    \n"

#define YUVTORGB_REGS                                                         \
  "v0", "v1", "v2", "v3", "v4", "v5", "v8", "v9", "v10", "v11", "v12", "v13", \
      "v14", "v15", "v16", "v17", "v18", "v19", "v24", "v25", "v26", "v27",   \
      "v28", "v29"

#ifdef HAS_ARGBTOAR64ROW_RVV
void ARGBToAR64Row_RVV(const uint8_t* src_argb, uint16_t* dst_ar64, int width) {
  int vl;
  asm volatile(
      "slli        %[w], %[w], 2                 \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vle8.v      v16, (%[src_argb])            \n"
      "vwcvtu.x.x.v v8, v16                      \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vmul.vx     v8, v8, %[k0101]              \n"
      "vse16.v     v8, (%[dst_ar64])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_argb], %[src_argb], %[vl]\n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_ar64], %[dst_ar64], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_ar64] "+r"(dst_ar64),  // %[dst_ar64]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      : [k0101] "r"(0x0101)
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v16", "v17");
}
#endif

#ifdef HAS_ARGBTOAB64ROW_RVV
void ARGBToAB64Row_RVV(const uint8_t* src_argb, uint16_t* dst_ab64, int width) {
  int vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m1, ta, ma   \n"
      "vlseg4e8.v  v4, (%[src_argb])             \n"
      "vwcvtu.x.x.v v12, v4                      \n"
      "vwcvtu.x.x.v v10, v5                      \n"
      "vwcvtu.x.x.v v8, v6                       \n"
      "vwcvtu.x.x.v v14, v7                      \n"
      "vsetvli     zero, zero, e16, m2, ta, ma   \n"
      "vmul.vx     v8, v8, %[k0101]              \n"
      "vmul.vx     v10, v10, %[k0101]            \n"
      "vmul.vx     v12, v12, %[k0101]            \n"
      "vmul.vx     v14, v14, %[k0101]            \n"
      "vsseg4e16.v v8, (%[dst_ab64])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src_argb], %[src_argb], %[vl]\n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_ab64], %[dst_ab64], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_ab64] "+r"(dst_ab64),  // %[dst_ab64]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      : [k0101] "r"(0x0101)
      : RVV_VL_CLOBBER "memory", "v4", "v5", "v6", "v7", "v8", "v9", "v10",
        "v11", "v12", "v13", "v14", "v15");
}
#endif

#ifdef HAS_AR64TOARGBROW_RVV
void AR64ToARGBRow_RVV(const uint16_t* src_ar64, uint8_t* dst_argb, int width) {
  int vl;
  asm volatile(
      "slli        %[w], %[w], 2                 \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vle16.v     v8, (%[src_ar64])             \n"
      "vnsrl.wi    v16, v8, 8                    \n"
      "vse8.v      v16, (%[dst_argb])            \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_ar64], %[src_ar64], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_ar64] "+r"(src_ar64),  // %[src_ar64]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v16", "v17");
}
#endif

#ifdef HAS_AB64TOARGBROW_RVV
void AB64ToARGBRow_RVV(const uint16_t* src_ab64, uint8_t* dst_argb, int width) {
  size_t vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m1, ta, ma   \n"
      "vlseg4e16.v v8, (%[src_ab64])             \n"
      "vnsrl.wi    v4, v12, 8                    \n"
      "vnsrl.wi    v5, v10, 8                    \n"
      "vnsrl.wi    v6, v8, 8                     \n"
      "vnsrl.wi    v7, v14, 8                    \n"
      "vsseg4e8.v  v4, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_ab64], %[src_ab64], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_ab64] "+r"(src_ab64),  // %[src_ab64]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v4", "v5", "v6", "v7", "v8", "v9", "v10",
        "v11", "v12", "v13", "v14", "v15");
}
#endif

#ifdef HAS_RAWTOARGBROW_RVV
void RAWToARGBRow_RVV(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  size_t vl, tmp;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vmv.v.i     v14, -1                       \n"

      "1:          \n"
      "vlseg3e8.v  v16, (%[src_raw])             \n"
      "vmv2r.v     v8, v20                       \n"
      "vmv2r.v     v10, v18                      \n"
      "vmv2r.v     v12, v16                      \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[src_raw], %[src_raw], %[vl] \n"
      "add         %[src_raw], %[src_raw], %[tmp]\n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_raw] "+r"(src_raw),    // %[src_raw]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [tmp] "=&r"(tmp)            // %[tmp]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21");
}
#endif

#ifdef HAS_RAWTORGBAROW_RVV
void RAWToRGBARow_RVV(const uint8_t* src_raw, uint8_t* dst_rgba, int width) {
  size_t vl, tmp;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vmv.v.i     v8, -1                        \n"

      "1:          \n"
      "vlseg3e8.v  v16, (%[src_raw])             \n"
      "vmv2r.v     v10, v20                      \n"
      "vmv2r.v     v12, v18                      \n"
      "vmv2r.v     v14, v16                      \n"
      "vsseg4e8.v  v8, (%[dst_rgba])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[src_raw], %[src_raw], %[vl] \n"
      "add         %[src_raw], %[src_raw], %[tmp]\n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_rgba], %[dst_rgba], %[vl]\n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_raw] "+r"(src_raw),    // %[src_raw]
        [dst_rgba] "+r"(dst_rgba),  // %[dst_rgba]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [tmp] "=&r"(tmp)            // %[tmp]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21");
}
#endif

#ifdef HAS_RAWTORGB24ROW_RVV
void RAWToRGB24Row_RVV(const uint8_t* src_raw, uint8_t* dst_rgb24, int width) {
  size_t vl, tmp;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg3e8.v  v16, (%[src_raw])             \n"
      "vmv2r.v     v8, v20                       \n"
      "vmv2r.v     v10, v18                      \n"
      "vmv2r.v     v12, v16                      \n"
      "vsseg3e8.v  v8, (%[dst_rgb24])            \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[tmp], %[tmp], %[vl]         \n"
      "add         %[src_raw], %[src_raw], %[tmp]\n"
      "add         %[dst_rgb24], %[dst_rgb24], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_raw] "+r"(src_raw),      // %[src_raw]
        [dst_rgb24] "+r"(dst_rgb24),  // %[dst_rgb24]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [tmp] "=&r"(tmp)              // %[tmp]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v16",
        "v17", "v18", "v19", "v20", "v21");
}
#endif

#ifdef HAS_ARGBTORAWROW_RVV
void ARGBToRAWRow_RVV(const uint8_t* src_argb, uint8_t* dst_raw, int width) {
  size_t vl, tmp;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src_argb])             \n"
      "vmv2r.v     v16, v12                      \n"
      "vmv2r.v     v18, v10                      \n"
      "vmv2r.v     v20, v8                       \n"
      "vsseg3e8.v  v16, (%[dst_raw])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[tmp], %[vl], 2              \n"
      "add         %[src_argb], %[src_argb], %[tmp]\n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[dst_raw], %[dst_raw], %[vl] \n"
      "add         %[dst_raw], %[dst_raw], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_raw] "+r"(dst_raw),    // %[dst_raw]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [tmp] "=&r"(tmp)            // %[tmp]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21");
}
#endif

#ifdef HAS_ARGBTORGB24ROW_RVV
void ARGBToRGB24Row_RVV(const uint8_t* src_argb,
                        uint8_t* dst_rgb24,
                        int width) {
  size_t vl, tmp;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src_argb])             \n"
      "vsseg3e8.v  v8, (%[dst_rgb24])            \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[tmp], %[vl], 2              \n"
      "add         %[src_argb], %[src_argb], %[tmp]\n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "add         %[dst_rgb24], %[dst_rgb24], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),    // %[src_argb]
        [dst_rgb24] "+r"(dst_rgb24),  // %[dst_rgb24]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [tmp] "=&r"(tmp)              // %[tmp]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_RGB24TOARGBROW_RVV
void RGB24ToARGBRow_RVV(const uint8_t* src_rgb24,
                        uint8_t* dst_argb,
                        int width) {
  size_t vl, tmp;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vmv.v.i     v14, -1                       \n"

      "1:          \n"
      "vlseg3e8.v  v8, (%[src_rgb24])            \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[src_rgb24], %[src_rgb24], %[vl]\n"
      "add         %[src_rgb24], %[src_rgb24], %[tmp]\n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_rgb24] "+r"(src_rgb24),  // %[src_rgb24]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [tmp] "=&r"(tmp)              // %[tmp]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_I444TOARGBROW_RVV
void I444ToARGBRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //
      "vsetvli     zero, %[w], e8, m2, ta, ma    \n"
      "vmv.v.i     v30, -1                       \n"

      "1:          \n"                                //
      READYUV444                                      //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg4e8.v  v24, (%[dst_argb])            \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[src_u], %[src_u], %[vl]     \n"
      "add         %[src_v], %[src_v], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_u] "+r"(src_u),          // %[src_u]
        [src_v] "+r"(src_v),          // %[src_v]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS, "v30", "v31");
}
#endif

#ifdef HAS_I444ALPHATOARGBROW_RVV
void I444AlphaToARGBRow_RVV(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            const uint8_t* src_a,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //

      "1:          \n"                                //
      READYUV444                                      //
      "vle8.v      v30, (%[src_a])               \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg4e8.v  v24, (%[dst_argb])            \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[src_u], %[src_u], %[vl]     \n"
      "add         %[src_v], %[src_v], %[vl]     \n"
      "add         %[src_a], %[src_a], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_u] "+r"(src_u),          // %[src_u]
        [src_v] "+r"(src_v),          // %[src_v]
        [src_a] "+r"(src_a),          // %[src_a]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS, "v30", "v31");
}
#endif

#ifdef HAS_I444TORGB24ROW_RVV
void I444ToRGB24Row_RVV(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_rgb24,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //

      "1:          \n"                                //
      READYUV444                                      //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg3e8.v  v24, (%[dst_rgb24])           \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[src_u], %[src_u], %[vl]     \n"
      "add         %[src_v], %[src_v], %[vl]     \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_u] "+r"(src_u),          // %[src_u]
        [src_v] "+r"(src_v),          // %[src_v]
        [dst_rgb24] "+r"(dst_rgb24),  // %[dst_rgb24]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS);
}
#endif

#ifdef HAS_I422TOARGBROW_RVV
void I422ToARGBRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //
      "vsetvli     zero, %[w], e8, m2, ta, ma    \n"
      "vmv.v.i     v30, -1                       \n"

      "1:          \n"                                //
      READYUV422                                      //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg4e8.v  v24, (%[dst_argb])            \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "srli        %[k0101], %[vl], 1            \n"
      "add         %[src_u], %[src_u], %[k0101]  \n"
      "add         %[src_v], %[src_v], %[k0101]  \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "li          %[k0101], 0x0101              \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_u] "+r"(src_u),          // %[src_u]
        [src_v] "+r"(src_v),          // %[src_v]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS, "v30", "v31");
}
#endif

#ifdef HAS_I422ALPHATOARGBROW_RVV
void I422AlphaToARGBRow_RVV(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            const uint8_t* src_a,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //

      "1:          \n"                                //
      READYUV422                                      //
      "vle8.v      v30, (%[src_a])               \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg4e8.v  v24, (%[dst_argb])            \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[src_a], %[src_a], %[vl]     \n"
      "srli        %[k0101], %[vl], 1            \n"
      "add         %[src_u], %[src_u], %[k0101]  \n"
      "add         %[src_v], %[src_v], %[k0101]  \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "li          %[k0101], 0x0101              \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_u] "+r"(src_u),          // %[src_u]
        [src_v] "+r"(src_v),          // %[src_v]
        [src_a] "+r"(src_a),          // %[src_a]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS, "v30", "v31");
}
#endif

#ifdef HAS_I422TORGBAROW_RVV
void I422ToRGBARow_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_rgba,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //

      "1:          \n"                                //
      READYUV422                                      //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vmv.v.i     v22, -1                       \n"
      "vsseg4e8.v  v22, (%[dst_rgba])            \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "srli        %[k0101], %[vl], 1            \n"
      "add         %[src_u], %[src_u], %[k0101]  \n"
      "add         %[src_v], %[src_v], %[k0101]  \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_rgba], %[dst_rgba], %[vl]\n"
      "li          %[k0101], 0x0101              \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_u] "+r"(src_u),          // %[src_u]
        [src_v] "+r"(src_v),          // %[src_v]
        [dst_rgba] "+r"(dst_rgba),    // %[dst_rgba]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS, "v22", "v23");
}
#endif

#ifdef HAS_I422TORGB24ROW_RVV
void I422ToRGB24Row_RVV(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_rgb24,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //

      "1:          \n"                                //
      READYUV422                                      //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg3e8.v  v24, (%[dst_rgb24])           \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "srli        %[k0101], %[vl], 1            \n"
      "add         %[src_u], %[src_u], %[k0101]  \n"
      "add         %[src_v], %[src_v], %[k0101]  \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "li          %[k0101], 0x0101              \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_u] "+r"(src_u),          // %[src_u]
        [src_v] "+r"(src_v),          // %[src_v]
        [dst_rgb24] "+r"(dst_rgb24),  // %[dst_rgb24]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS);
}
#endif

#ifdef HAS_I422TOAR30ROW_RVV
void I422ToAR30Row_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_ar30,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(            //
      YUVTORGB_SETUP_AR30  //

      "1:          \n"
      "addi        %[vl], %[w], 1                \n"
      "srli        %[vl], %[vl], 1               \n"
      "vsetvli     %[vl], %[vl], e8, mf2, ta, ma \n"
      "vle8.v      v0, (%[src_u])                \n"
      "vle8.v      v1, (%[src_v])                \n"
      "vwcvtu.x.x.v v2, v0                       \n"
      "vwcvtu.x.x.v v4, v1                       \n"
      "vsetvli     zero, zero, e16, m1, ta, ma   \n"
      "vmul.vx     v2, v2, %[k0101]              \n"
      "vmul.vx     v4, v4, %[k0101]              \n"
      "vsetvli     %[vl], %[w], e8, m1, ta, ma   \n"
      "vle8.v      v0, (%[src_y])                \n"
      "vwcvtu.x.x.v v8, v0                       \n"
      "vwmulu.vx   v12, v2, %[ug]                \n"
      "vwmaccu.vx  v12, %[vg], v4                \n"
      "vwmulu.vx   v0, v2, %[ub]                 \n"
      "vsetvli     zero, zero, e16, m2, ta, ma   \n"
      "vmul.vx     v8, v8, %[k0101]              \n"
      "vmulhu.vx   v8, v8, %[yg]                 \n"
      "vadd.vx     v16, v8, %[bg]                \n"
      "vadd.vv     v0, v8, v0                    \n"
      "vsetvli     zero, zero, e8, m1, ta, ma    \n"
      "vwmaccu.vx  v8, %[vr], v4                 \n"
      "vsetvli     zero, zero, e16, m2, ta, ma   \n"
      "vssubu.vv   v12, v16, v12                 \n"
      "vssubu.vx   v0, v0, %[bb]                 \n"
      "vssubu.vx   v8, v8, %[br]                 \n"
      "vsrl.vi     v0, v0, 4                     \n"
      "vsrl.vi     v12, v12, 4                   \n"
      "vsrl.vi     v8, v8, 4                     \n"
      "li          %[k0101], 1023                \n"
      "vminu.vx    v0, v0, %[k0101]              \n"
      "vminu.vx    v12, v12, %[k0101]            \n"
      "vminu.vx    v8, v8, %[k0101]              \n"
      "vwcvtu.x.x.v v16, v0                      \n"
      "li          %[k0101], 1024                \n"
      "vwmaccu.vx  v16, %[k0101], v12            \n"
      "li          %[k0101], 0x0c00              \n"
      "vor.vx      v8, v8, %[k0101]              \n"
      "vwcvtu.x.x.v v20, v8                      \n"
      "vsetvli     zero, zero, e32, m4, ta, ma   \n"
      "vsll.vi     v20, v20, 20                  \n"
      "vor.vv      v16, v16, v20                 \n"
      "vse32.v     v16, (%[dst_ar30])            \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "srli        %[k0101], %[vl], 1            \n"
      "add         %[src_u], %[src_u], %[k0101]  \n"
      "add         %[src_v], %[src_v], %[k0101]  \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_ar30], %[dst_ar30], %[vl]\n"
      "li          %[k0101], 0x0101              \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_u] "+r"(src_u),          // %[src_u]
        [src_v] "+r"(src_v),          // %[src_v]
        [dst_ar30] "+r"(dst_ar30),    // %[dst_ar30]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", "v0", "v1", "v2", "v4", "v8", "v9", "v12",
        "v13", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
}
#endif

#ifdef HAS_I400TOARGBROW_RVV
void I400ToARGBRow_RVV(const uint8_t* src_y,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t vl;
  const bool is_yb_positive = (yuvconstants->kRGBCoeffBias[4] >= 0);
  uint16_t yg = yuvconstants->kRGBCoeffBias[0];
  uint16_t yb = is_yb_positive
                    ? (uint16_t)(yuvconstants->kRGBCoeffBias[4] - 32)
                    : (uint16_t)(-yuvconstants->kRGBCoeffBias[4] + 32);
  size_t k0101;
  if (is_yb_positive) {
    asm volatile(
        "csrwi       vxrm, 0                       \n"
        "li          %[k0101], 0x0101              \n"
        "vsetvli     zero, %[w], e16, m4, ta, ma   \n"
        "vmv.v.x     v20, %[yg]                    \n"
        "vmv.v.x     v24, %[yb]                    \n"
        "vsetvli     zero, %[w], e8, m2, ta, ma    \n"
        "vmv.v.i     v14, -1                       \n"

        "1:          \n"
        "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
        "vle8.v      v0, (%[src_y])                \n"
        "vwcvtu.x.x.v v16, v0                      \n"
        "vsetvli     zero, zero, e16, m4, ta, ma   \n"
        "vmul.vx     v16, v16, %[k0101]            \n"
        "vmulhu.vv   v16, v16, v20                 \n"
        "vsaddu.vv   v16, v16, v24                 \n"
        "vsetvli     zero, zero, e8, m2, ta, ma    \n"
        "vnclipu.wi  v8, v16, 6                    \n"
        "vmv2r.v     v10, v8                       \n"
        "vmv2r.v     v12, v8                       \n"
        "vsseg4e8.v  v8, (%[dst_argb])             \n"
        "sub         %[w], %[w], %[vl]             \n"
        "add         %[src_y], %[src_y], %[vl]     \n"
        "slli        %[vl], %[vl], 2               \n"
        "add         %[dst_argb], %[dst_argb], %[vl]\n"
        "bgtz        %[w], 1b                      \n"
        : [src_y] "+r"(src_y),        // %[src_y]
          [dst_argb] "+r"(dst_argb),  // %[dst_argb]
          [w] "+r"(width),            // %[w]
          [vl] "=&r"(vl),             // %[vl]
          [k0101] "=&r"(k0101)        // %[k0101]
        : [yg] "r"(yg),               // %[yg]
          [yb] "r"(yb)                // %[yb]
        : RVV_VL_CLOBBER "memory", "v0", "v1", "v8", "v9", "v10", "v11", "v12",
          "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
          "v23", "v24", "v25", "v26", "v27");
  } else {
    asm volatile(
        "csrwi       vxrm, 0                       \n"
        "li          %[k0101], 0x0101              \n"
        "vsetvli     zero, %[w], e16, m4, ta, ma   \n"
        "vmv.v.x     v20, %[yg]                    \n"
        "vmv.v.x     v24, %[yb]                    \n"
        "vsetvli     zero, %[w], e8, m2, ta, ma    \n"
        "vmv.v.i     v14, -1                       \n"

        "1:          \n"
        "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
        "vle8.v      v0, (%[src_y])                \n"
        "vwcvtu.x.x.v v16, v0                      \n"
        "vsetvli     zero, zero, e16, m4, ta, ma   \n"
        "vmul.vx     v16, v16, %[k0101]            \n"
        "vmulhu.vv   v16, v16, v20                 \n"
        "vssubu.vv   v16, v16, v24                 \n"
        "vsetvli     zero, zero, e8, m2, ta, ma    \n"
        "vnclipu.wi  v8, v16, 6                    \n"
        "vmv2r.v     v10, v8                       \n"
        "vmv2r.v     v12, v8                       \n"
        "vsseg4e8.v  v8, (%[dst_argb])             \n"
        "sub         %[w], %[w], %[vl]             \n"
        "add         %[src_y], %[src_y], %[vl]     \n"
        "slli        %[vl], %[vl], 2               \n"
        "add         %[dst_argb], %[dst_argb], %[vl]\n"
        "bgtz        %[w], 1b                      \n"
        : [src_y] "+r"(src_y),        // %[src_y]
          [dst_argb] "+r"(dst_argb),  // %[dst_argb]
          [w] "+r"(width),            // %[w]
          [vl] "=&r"(vl),             // %[vl]
          [k0101] "=&r"(k0101)        // %[k0101]
        : [yg] "r"(yg),               // %[yg]
          [yb] "r"(yb)                // %[yb]
        : RVV_VL_CLOBBER "memory", "v0", "v1", "v8", "v9", "v10", "v11", "v12",
          "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
          "v23", "v24", "v25", "v26", "v27");
  }
}
#endif

#ifdef HAS_J400TOARGBROW_RVV
void J400ToARGBRow_RVV(const uint8_t* src_y, uint8_t* dst_argb, int width) {
  size_t vl;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vmv.v.i     v14, -1                       \n"

      "1:          \n"
      "vle8.v      v8, (%[src_y])                \n"
      "vmv2r.v     v10, v8                       \n"
      "vmv2r.v     v12, v8                       \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),        // %[src_y]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_COPYROW_RVV
void CopyRow_RVV(const uint8_t* src, uint8_t* dst, int width) {
  size_t vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vle8.v      v8, (%[src])                  \n"
      "vse8.v      v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl]         \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src),  // %[src]
        [dst] "+r"(dst),  // %[dst]
        [w] "+r"(width),  // %[w]
        [vl] "=&r"(vl)    // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11");
}
#endif

#ifdef HAS_NV12TOARGBROW_RVV
void NV12ToARGBRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_uv,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //
      "vsetvli     zero, %[w], e8, m2, ta, ma    \n"
      "vmv.v.i     v30, -1                       \n"

      "1:          \n"                                //
      READNV12                                        //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg4e8.v  v24, (%[dst_argb])            \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[src_uv], %[src_uv], %[vl]   \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_uv] "+r"(src_uv),        // %[src_uv]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS, "v30", "v31");
}
#endif

#ifdef HAS_NV12TORGB24ROW_RVV
void NV12ToRGB24Row_RVV(const uint8_t* src_y,
                        const uint8_t* src_uv,
                        uint8_t* dst_rgb24,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //

      "1:          \n"                                //
      READNV12                                        //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg3e8.v  v24, (%[dst_rgb24])           \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[src_uv], %[src_uv], %[vl]   \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_uv] "+r"(src_uv),        // %[src_uv]
        [dst_rgb24] "+r"(dst_rgb24),  // %[dst_rgb24]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS);
}
#endif

#ifdef HAS_NV21TOARGBROW_RVV
void NV21ToARGBRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_vu,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //
      "vsetvli     zero, %[w], e8, m2, ta, ma    \n"
      "vmv.v.i     v30, -1                       \n"

      "1:          \n"                                //
      READNV21                                        //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg4e8.v  v24, (%[dst_argb])            \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[src_vu], %[src_vu], %[vl]   \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_vu] "+r"(src_vu),        // %[src_vu]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS, "v30", "v31");
}
#endif

#ifdef HAS_NV21TORGB24ROW_RVV
void NV21ToRGB24Row_RVV(const uint8_t* src_y,
                        const uint8_t* src_vu,
                        uint8_t* dst_rgb24,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  size_t vl;
  size_t ub, vr, ug, vg;
  size_t yg, bb, bg, br;
  size_t k0101;
  asm volatile(       //
      YUVTORGB_SETUP  //

      "1:          \n"                                //
      READNV21                                        //
      "sub         %[w], %[w], %[vl]             \n"  //
      YUVTORGB                                        //
      RGBTORGB8                                       //
      "vsseg3e8.v  v24, (%[dst_rgb24])           \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[src_vu], %[src_vu], %[vl]   \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_rgb24], %[dst_rgb24], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),          // %[src_y]
        [src_vu] "+r"(src_vu),        // %[src_vu]
        [dst_rgb24] "+r"(dst_rgb24),  // %[dst_rgb24]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [ub] "=&r"(ub),               // %[ub]
        [vr] "=&r"(vr),               // %[vr]
        [ug] "=&r"(ug),               // %[ug]
        [vg] "=&r"(vg),               // %[vg]
        [yg] "=&r"(yg),               // %[yg]
        [bb] "=&r"(bb),               // %[bb]
        [bg] "=&r"(bg),               // %[bg]
        [br] "=&r"(br),               // %[br]
        [k0101] "=&r"(k0101)          // %[k0101]
      : [yuvconst] "r"(yuvconstants)  // %[yuvconst]
      : RVV_VL_CLOBBER "memory", YUVTORGB_REGS);
}
#endif

// Bilinear filter [VLEN/8]x2 -> [VLEN/8]x1
#ifdef HAS_INTERPOLATEROW_RVV
void InterpolateRow_RVV(uint8_t* dst_ptr,
                        const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        int dst_width,
                        int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  size_t dst_w = (size_t)dst_width;
  size_t vl;
  assert(source_y_fraction >= 0);
  assert(source_y_fraction < 256);
  // Blend 100 / 0 - Copy row unchanged.
  if (y1_fraction == 0) {
    asm volatile(
        "1:          \n"
        "vsetvli     %[vl], %[dst_w], e8, m4, ta, ma\n"
        "vle8.v      v8, (%[src_ptr])              \n"
        "vse8.v      v8, (%[dst_ptr])              \n"
        "sub         %[dst_w], %[dst_w], %[vl]     \n"
        "add         %[src_ptr], %[src_ptr], %[vl] \n"
        "add         %[dst_ptr], %[dst_ptr], %[vl] \n"
        "bgtz        %[dst_w], 1b                  \n"
        : [dst_ptr] "+r"(dst_ptr),  // %[dst_ptr]
          [src_ptr] "+r"(src_ptr),  // %[src_ptr]
          [dst_w] "+r"(dst_w),      // %[dst_w]
          [vl] "=&r"(vl)            // %[vl]
        :
        : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11");
    return;
  }
  // Blend 50 / 50.
  if (y1_fraction == 128) {
    asm volatile(
        "csrwi       vxrm, 0                       \n"

        "1:          \n"
        "vsetvli     %[vl], %[dst_w], e8, m4, ta, ma\n"
        "vle8.v      v8, (%[src_ptr])              \n"
        "vle8.v      v12, (%[src_ptr1])            \n"
        "vaaddu.vv   v8, v8, v12                   \n"
        "vse8.v      v8, (%[dst_ptr])              \n"
        "sub         %[dst_w], %[dst_w], %[vl]     \n"
        "add         %[src_ptr], %[src_ptr], %[vl] \n"
        "add         %[src_ptr1], %[src_ptr1], %[vl]\n"
        "add         %[dst_ptr], %[dst_ptr], %[vl] \n"
        "bgtz        %[dst_w], 1b                  \n"
        : [dst_ptr] "+r"(dst_ptr),    // %[dst_ptr]
          [src_ptr] "+r"(src_ptr),    // %[src_ptr]
          [src_ptr1] "+r"(src_ptr1),  // %[src_ptr1]
          [dst_w] "+r"(dst_w),        // %[dst_w]
          [vl] "=&r"(vl)              // %[vl]
        :
        : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13",
          "v14", "v15");
    return;
  }
  // General purpose row blend.
  asm volatile(
      "vsetvli     zero, %[dst_w], e16, m4, ta, ma\n"
      "vmv.v.x     v12, %[c128]                  \n"

      "1:          \n"
      "vsetvli     %[vl], %[dst_w], e8, m2, ta, ma\n"
      "vle8.v      v16, (%[src_ptr])             \n"
      "vmv4r.v     v8, v12                       \n"
      "vwmaccu.vx  v8, %[y0_fraction], v16       \n"
      "vle8.v      v16, (%[src_ptr1])            \n"
      "vwmaccu.vx  v8, %[y1_fraction], v16       \n"
      "vnsrl.wi    v16, v8, 8                    \n"
      "vse8.v      v16, (%[dst_ptr])             \n"
      "sub         %[dst_w], %[dst_w], %[vl]     \n"
      "add         %[src_ptr], %[src_ptr], %[vl] \n"
      "add         %[src_ptr1], %[src_ptr1], %[vl]\n"
      "add         %[dst_ptr], %[dst_ptr], %[vl] \n"
      "bgtz        %[dst_w], 1b                  \n"
      : [dst_ptr] "+r"(dst_ptr),         // %[dst_ptr]
        [src_ptr] "+r"(src_ptr),         // %[src_ptr]
        [src_ptr1] "+r"(src_ptr1),       // %[src_ptr1]
        [dst_w] "+r"(dst_w),             // %[dst_w]
        [vl] "=&r"(vl)                   // %[vl]
      : [y0_fraction] "r"(y0_fraction),  // %[y0_fraction]
        [y1_fraction] "r"(y1_fraction),  // %[y1_fraction]
        [c128] "r"(128)                  // %[c128]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17");
}
#endif

#ifdef HAS_SPLITRGBROW_RVV
void SplitRGBRow_RVV(const uint8_t* src_rgb,
                     uint8_t* dst_r,
                     uint8_t* dst_g,
                     uint8_t* dst_b,
                     int width) {
  size_t vl, tmp;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg3e8.v  v8, (%[src_rgb])              \n"
      "vse8.v      v8, (%[dst_r])                \n"
      "vse8.v      v10, (%[dst_g])               \n"
      "vse8.v      v12, (%[dst_b])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_r], %[dst_r], %[vl]     \n"
      "add         %[dst_g], %[dst_g], %[vl]     \n"
      "add         %[dst_b], %[dst_b], %[vl]     \n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[src_rgb], %[src_rgb], %[vl] \n"
      "add         %[src_rgb], %[src_rgb], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_rgb] "+r"(src_rgb),  // %[src_rgb]
        [dst_r] "+r"(dst_r),      // %[dst_r]
        [dst_g] "+r"(dst_g),      // %[dst_g]
        [dst_b] "+r"(dst_b),      // %[dst_b]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [tmp] "=&r"(tmp)          // %[tmp]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13");
}
#endif

#ifdef HAS_MERGERGBROW_RVV
void MergeRGBRow_RVV(const uint8_t* src_r,
                     const uint8_t* src_g,
                     const uint8_t* src_b,
                     uint8_t* dst_rgb,
                     int width) {
  size_t vl, tmp;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vle8.v      v8, (%[src_r])                \n"
      "vle8.v      v10, (%[src_g])               \n"
      "vle8.v      v12, (%[src_b])               \n"
      "vsseg3e8.v  v8, (%[dst_rgb])              \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_r], %[src_r], %[vl]     \n"
      "add         %[src_g], %[src_g], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[dst_rgb], %[dst_rgb], %[vl] \n"
      "add         %[dst_rgb], %[dst_rgb], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_r] "+r"(src_r),      // %[src_r]
        [src_g] "+r"(src_g),      // %[src_g]
        [src_b] "+r"(src_b),      // %[src_b]
        [dst_rgb] "+r"(dst_rgb),  // %[dst_rgb]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [tmp] "=&r"(tmp)          // %[tmp]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13");
}
#endif

#ifdef HAS_SPLITARGBROW_RVV
void SplitARGBRow_RVV(const uint8_t* src_argb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      uint8_t* dst_a,
                      int width) {
  size_t vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src_argb])             \n"
      "vse8.v      v14, (%[dst_a])               \n"
      "vse8.v      v12, (%[dst_r])               \n"
      "vse8.v      v10, (%[dst_g])               \n"
      "vse8.v      v8, (%[dst_b])                \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_a], %[dst_a], %[vl]     \n"
      "add         %[dst_r], %[dst_r], %[vl]     \n"
      "add         %[dst_g], %[dst_g], %[vl]     \n"
      "add         %[dst_b], %[dst_b], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src_argb], %[src_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_r] "+r"(dst_r),        // %[dst_r]
        [dst_g] "+r"(dst_g),        // %[dst_g]
        [dst_b] "+r"(dst_b),        // %[dst_b]
        [dst_a] "+r"(dst_a),        // %[dst_a]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_MERGEARGBROW_RVV
void MergeARGBRow_RVV(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      const uint8_t* src_a,
                      uint8_t* dst_argb,
                      int width) {
  size_t vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vle8.v      v12, (%[src_r])               \n"
      "vle8.v      v10, (%[src_g])               \n"
      "vle8.v      v8, (%[src_b])                \n"
      "vle8.v      v14, (%[src_a])               \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_r], %[src_r], %[vl]     \n"
      "add         %[src_g], %[src_g], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "add         %[src_a], %[src_a], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_r] "+r"(src_r),        // %[src_r]
        [src_g] "+r"(src_g),        // %[src_g]
        [src_b] "+r"(src_b),        // %[src_b]
        [src_a] "+r"(src_a),        // %[src_a]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_SPLITXRGBROW_RVV
void SplitXRGBRow_RVV(const uint8_t* src_argb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      int width) {
  size_t vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src_argb])             \n"
      "vse8.v      v12, (%[dst_r])               \n"
      "vse8.v      v10, (%[dst_g])               \n"
      "vse8.v      v8, (%[dst_b])                \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_r], %[dst_r], %[vl]     \n"
      "add         %[dst_g], %[dst_g], %[vl]     \n"
      "add         %[dst_b], %[dst_b], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src_argb], %[src_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_r] "+r"(dst_r),        // %[dst_r]
        [dst_g] "+r"(dst_g),        // %[dst_g]
        [dst_b] "+r"(dst_b),        // %[dst_b]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_MERGEXRGBROW_RVV
void MergeXRGBRow_RVV(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      uint8_t* dst_argb,
                      int width) {
  size_t vl;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vmv.v.i     v14, -1                       \n"

      "1:          \n"
      "vle8.v      v12, (%[src_r])               \n"
      "vle8.v      v10, (%[src_g])               \n"
      "vle8.v      v8, (%[src_b])                \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_r], %[src_r], %[vl]     \n"
      "add         %[src_g], %[src_g], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_r] "+r"(src_r),        // %[src_r]
        [src_g] "+r"(src_g),        // %[src_g]
        [src_b] "+r"(src_b),        // %[src_b]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_SPLITUVROW_RVV
void SplitUVRow_RVV(const uint8_t* src_uv,
                    uint8_t* dst_u,
                    uint8_t* dst_v,
                    int width) {
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vlseg2e8.v  v8, (%[src_uv])               \n"
      "vse8.v      v8, (%[dst_u])                \n"
      "vse8.v      v12, (%[dst_v])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_u], %[dst_u], %[vl]     \n"
      "add         %[dst_v], %[dst_v], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_uv], %[src_uv], %[vl]   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_uv] "+r"(src_uv),  // %[src_uv]
        [dst_u] "+r"(dst_u),    // %[dst_u]
        [dst_v] "+r"(dst_v),    // %[dst_v]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
#else
  size_t vl2;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src_uv])               \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
      "vnsrl.wi    v0, v8, 0                     \n"
      "vnsrl.wi    v4, v8, 8                     \n"
      "vse8.v      v0, (%[dst_u])                \n"
      "vse8.v      v4, (%[dst_v])                \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_u], %[dst_u], %[vl]     \n"
      "add         %[dst_v], %[dst_v], %[vl]     \n"
      "add         %[src_uv], %[src_uv], %[vl2]  \n"
      "bgtz        %[w], 1b                      \n"
      : [src_uv] "+r"(src_uv),  // %[src_uv]
        [dst_u] "+r"(dst_u),    // %[dst_u]
        [dst_v] "+r"(dst_v),    // %[dst_v]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl),         // %[vl]
        [vl2] "=&r"(vl2)        // %[vl2]
      :
      : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
        "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15");
#endif
}
#endif

#ifdef HAS_SPLITUVROW_16_RVV
void SplitUVRow_16_RVV(const uint16_t* src_uv,
                       uint16_t* dst_u,
                       uint16_t* dst_v,
                       int depth,
                       int width) {
  size_t w = (size_t)width;
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vlseg2e16.v v8, (%[src_uv])               \n"
      "vsrl.vx     v8, v8, %[shift]              \n"
      "vsrl.vx     v12, v12, %[shift]            \n"
      "vse16.v     v8, (%[dst_u])                \n"
      "vse16.v     v12, (%[dst_v])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_u], %[dst_u], %[vl]     \n"
      "add         %[dst_v], %[dst_v], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_uv], %[src_uv], %[vl]   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_uv] "+r"(src_uv),   // %[src_uv]
        [dst_u] "+r"(dst_u),     // %[dst_u]
        [dst_v] "+r"(dst_v),     // %[dst_v]
        [w] "+r"(w),             // %[w]
        [vl] "=&r"(vl)           // %[vl]
      : [shift] "r"(16 - depth)  // %[shift]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_MERGEUVROW_RVV
void MergeUVRow_RVV(const uint8_t* src_u,
                    const uint8_t* src_v,
                    uint8_t* dst_uv,
                    int width) {
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vle8.v      v8, (%[src_u])                \n"
      "vle8.v      v12, (%[src_v])               \n"
      "vsseg2e8.v  v8, (%[dst_uv])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_u], %[src_u], %[vl]     \n"
      "add         %[src_v], %[src_v], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_uv], %[dst_uv], %[vl]   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_u] "+r"(src_u),    // %[src_u]
        [src_v] "+r"(src_v),    // %[src_v]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
#else
  size_t vl2;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vle8.v      v0, (%[src_u])                \n"
      "vle8.v      v4, (%[src_v])                \n"
      "vwaddu.vv   v8, v0, v4                    \n"
      "vwmaccu.vx  v8, %[c255], v4               \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
      "vse8.v      v8, (%[dst_uv])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_u], %[src_u], %[vl]     \n"
      "add         %[src_v], %[src_v], %[vl]     \n"
      "add         %[dst_uv], %[dst_uv], %[vl2]  \n"
      "bgtz        %[w], 1b                      \n"
      : [src_u] "+r"(src_u),    // %[src_u]
        [src_v] "+r"(src_v),    // %[src_v]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl),         // %[vl]
        [vl2] "=&r"(vl2)        // %[vl2]
      : [c255] "r"(255)         // %[c255]
      : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
        "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15");
#endif
}
#endif

#ifdef HAS_MERGEUVROW_16_RVV
void MergeUVRow_16_RVV(const uint16_t* src_u,
                       const uint16_t* src_v,
                       uint16_t* dst_uv,
                       int depth,
                       int width) {
  size_t w = (size_t)width;
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v8, (%[src_u])                \n"
      "vle16.v     v12, (%[src_v])               \n"
      "vsll.vx     v8, v8, %[shift]              \n"
      "vsll.vx     v12, v12, %[shift]            \n"
      "vsseg2e16.v v8, (%[dst_uv])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_u], %[src_u], %[vl]     \n"
      "add         %[src_v], %[src_v], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_uv], %[dst_uv], %[vl]   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_u] "+r"(src_u),     // %[src_u]
        [src_v] "+r"(src_v),     // %[src_v]
        [dst_uv] "+r"(dst_uv),   // %[dst_uv]
        [w] "+r"(w),             // %[w]
        [vl] "=&r"(vl)           // %[vl]
      : [shift] "r"(16 - depth)  // %[shift]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
#else
  size_t vl2;
  // Interleave by widening: u + v * 65536 = (u + v) + v * 0xffff.
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v0, (%[src_u])                \n"
      "vle16.v     v4, (%[src_v])                \n"
      "vsll.vx     v0, v0, %[shift]              \n"
      "vsll.vx     v4, v4, %[shift]              \n"
      "vwaddu.vv   v8, v0, v4                    \n"
      "vwmaccu.vx  v8, %[cffff], v4              \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e16, m8, ta, ma \n"
      "vse16.v     v8, (%[dst_uv])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_u], %[src_u], %[vl2]    \n"
      "add         %[src_v], %[src_v], %[vl2]    \n"
      "slli        %[vl2], %[vl2], 1             \n"
      "add         %[dst_uv], %[dst_uv], %[vl2]  \n"
      "bgtz        %[w], 1b                      \n"
      : [src_u] "+r"(src_u),      // %[src_u]
        [src_v] "+r"(src_v),      // %[src_v]
        [dst_uv] "+r"(dst_uv),    // %[dst_uv]
        [w] "+r"(w),              // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [vl2] "=&r"(vl2)          // %[vl2]
      : [shift] "r"(16 - depth),  // %[shift]
        [cffff] "r"(0xffff)       // %[cffff]
      : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
        "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15");
#endif
}
#endif

#ifdef HAS_MERGEXR30ROW_RVV
void MergeXR30Row_RVV(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      uint8_t* dst_ar30,
                      int depth,
                      int width) {
  size_t w = (size_t)width;
  size_t vl;
  // AR30 is built as two 16 bit halves:
  //   low  = b | g << 10
  //   high = g >> 6 | r << 4 | 0xc000
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v8, (%[src_b])                \n"
      "vle16.v     v12, (%[src_g])               \n"
      "vle16.v     v16, (%[src_r])               \n"
      "vsrl.vx     v8, v8, %[shift]              \n"
      "vsrl.vx     v12, v12, %[shift]            \n"
      "vsrl.vx     v16, v16, %[shift]            \n"
      "vminu.vx    v8, v8, %[c1023]              \n"
      "vminu.vx    v12, v12, %[c1023]            \n"
      "vminu.vx    v16, v16, %[c1023]            \n"
      "vsll.vi     v20, v12, 10                  \n"
      "vsrl.vi     v12, v12, 6                   \n"
      "vsll.vi     v16, v16, 4                   \n"
      "vor.vv      v8, v8, v20                   \n"
      "vor.vv      v12, v12, v16                 \n"
      "vor.vx      v12, v12, %[cc000]            \n"
      "vsseg2e16.v v8, (%[dst_ar30])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_r], %[src_r], %[vl]     \n"
      "add         %[src_g], %[src_g], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_ar30], %[dst_ar30], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_r] "+r"(src_r),        // %[src_r]
        [src_g] "+r"(src_g),        // %[src_g]
        [src_b] "+r"(src_b),        // %[src_b]
        [dst_ar30] "+r"(dst_ar30),  // %[dst_ar30]
        [w] "+r"(w),                // %[w]
        [vl] "=&r"(vl)              // %[vl]
      : [shift] "r"(depth - 10),    // %[shift]
        [c1023] "r"(1023),          // %[c1023]
        [cc000] "r"(0xc000)         // %[cc000]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
#else
  size_t vl2;
  // Interleave by widening: low + high * 65536 = (low + high) + high * 0xffff.
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v8, (%[src_b])                \n"
      "vle16.v     v12, (%[src_g])               \n"
      "vle16.v     v16, (%[src_r])               \n"
      "vsrl.vx     v8, v8, %[shift]              \n"
      "vsrl.vx     v12, v12, %[shift]            \n"
      "vsrl.vx     v16, v16, %[shift]            \n"
      "vminu.vx    v8, v8, %[c1023]              \n"
      "vminu.vx    v12, v12, %[c1023]            \n"
      "vminu.vx    v16, v16, %[c1023]            \n"
      "vsll.vi     v20, v12, 10                  \n"
      "vsrl.vi     v12, v12, 6                   \n"
      "vsll.vi     v16, v16, 4                   \n"
      "vor.vv      v8, v8, v20                   \n"
      "vor.vv      v12, v12, v16                 \n"
      "vor.vx      v12, v12, %[cc000]            \n"
      "vwaddu.vv   v24, v8, v12                  \n"
      "vwmaccu.vx  v24, %[cffff], v12            \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e16, m8, ta, ma \n"
      "vse16.v     v24, (%[dst_ar30])            \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_r], %[src_r], %[vl2]    \n"
      "add         %[src_g], %[src_g], %[vl2]    \n"
      "add         %[src_b], %[src_b], %[vl2]    \n"
      "slli        %[vl2], %[vl2], 1             \n"
      "add         %[dst_ar30], %[dst_ar30], %[vl2]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_r] "+r"(src_r),        // %[src_r]
        [src_g] "+r"(src_g),        // %[src_g]
        [src_b] "+r"(src_b),        // %[src_b]
        [dst_ar30] "+r"(dst_ar30),  // %[dst_ar30]
        [w] "+r"(w),                // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [vl2] "=&r"(vl2)            // %[vl2]
      : [shift] "r"(depth - 10),    // %[shift]
        [c1023] "r"(1023),          // %[c1023]
        [cc000] "r"(0xc000),        // %[cc000]
        [cffff] "r"(0xffff)         // %[cffff]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
        "v25", "v26", "v27", "v28", "v29", "v30", "v31");
#endif
}
#endif

#ifdef HAS_MERGEAR64ROW_RVV
void MergeAR64Row_RVV(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      const uint16_t* src_a,
                      uint16_t* dst_ar64,
                      int depth,
                      int width) {
  size_t w = (size_t)width;
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m2, ta, ma  \n"
      "vle16.v     v8, (%[src_b])                \n"
      "vle16.v     v10, (%[src_g])               \n"
      "vle16.v     v12, (%[src_r])               \n"
      "vle16.v     v14, (%[src_a])               \n"
      "vminu.vx    v8, v8, %[max]                \n"
      "vminu.vx    v10, v10, %[max]              \n"
      "vminu.vx    v12, v12, %[max]              \n"
      "vminu.vx    v14, v14, %[max]              \n"
      "vsll.vx     v8, v8, %[shift]              \n"
      "vsll.vx     v10, v10, %[shift]            \n"
      "vsll.vx     v12, v12, %[shift]            \n"
      "vsll.vx     v14, v14, %[shift]            \n"
      "vsseg4e16.v v8, (%[dst_ar64])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_r], %[src_r], %[vl]     \n"
      "add         %[src_g], %[src_g], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "add         %[src_a], %[src_a], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_ar64], %[dst_ar64], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_r] "+r"(src_r),         // %[src_r]
        [src_g] "+r"(src_g),         // %[src_g]
        [src_b] "+r"(src_b),         // %[src_b]
        [src_a] "+r"(src_a),         // %[src_a]
        [dst_ar64] "+r"(dst_ar64),   // %[dst_ar64]
        [w] "+r"(w),                 // %[w]
        [vl] "=&r"(vl)               // %[vl]
      : [shift] "r"(16 - depth),     // %[shift]
        [max] "r"((1 << depth) - 1)  // %[max]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_MERGEXR64ROW_RVV
void MergeXR64Row_RVV(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      uint16_t* dst_ar64,
                      int depth,
                      int width) {
  size_t w = (size_t)width;
  size_t vl;
  asm volatile(
      "vsetvli     %[vl], %[w], e16, m2, ta, ma  \n"
      "vmv.v.i     v14, -1                       \n"

      "1:                                        \n"
      "vle16.v     v8, (%[src_b])                \n"
      "vle16.v     v10, (%[src_g])               \n"
      "vle16.v     v12, (%[src_r])               \n"
      "vminu.vx    v8, v8, %[max]                \n"
      "vminu.vx    v10, v10, %[max]              \n"
      "vminu.vx    v12, v12, %[max]              \n"
      "vsll.vx     v8, v8, %[shift]              \n"
      "vsll.vx     v10, v10, %[shift]            \n"
      "vsll.vx     v12, v12, %[shift]            \n"
      "vsseg4e16.v v8, (%[dst_ar64])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_r], %[src_r], %[vl]     \n"
      "add         %[src_g], %[src_g], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst_ar64], %[dst_ar64], %[vl]\n"
      "vsetvli     %[vl], %[w], e16, m2, ta, ma  \n"
      "bgtz        %[w], 1b                      \n"
      : [src_r] "+r"(src_r),         // %[src_r]
        [src_g] "+r"(src_g),         // %[src_g]
        [src_b] "+r"(src_b),         // %[src_b]
        [dst_ar64] "+r"(dst_ar64),   // %[dst_ar64]
        [w] "+r"(w),                 // %[w]
        [vl] "=&r"(vl)               // %[vl]
      : [shift] "r"(16 - depth),     // %[shift]
        [max] "r"((1 << depth) - 1)  // %[max]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_MERGEARGB16TO8ROW_RVV
// vnclipu shifts right and saturates to 8 bits. vxrm is set to round-down
// (truncate) to match the C version and restored on exit.
void MergeARGB16To8Row_RVV(const uint16_t* src_r,
                           const uint16_t* src_g,
                           const uint16_t* src_b,
                           const uint16_t* src_a,
                           uint8_t* dst_argb,
                           int depth,
                           int width) {
  size_t w = (size_t)width;
  size_t vl, vxrm;
  asm volatile(
      "csrr        %[vxrm], vxrm                 \n"
      "csrwi       vxrm, 2                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vle16.v     v16, (%[src_b])               \n"
      "vle16.v     v20, (%[src_g])               \n"
      "vle16.v     v24, (%[src_r])               \n"
      "vle16.v     v28, (%[src_a])               \n"
      "vnclipu.wx  v8, v16, %[shift]             \n"
      "vnclipu.wx  v10, v20, %[shift]            \n"
      "vnclipu.wx  v12, v24, %[shift]            \n"
      "vnclipu.wx  v14, v28, %[shift]            \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_r], %[src_r], %[vl]     \n"
      "add         %[src_g], %[src_g], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "add         %[src_a], %[src_a], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      "csrw        vxrm, %[vxrm]                 \n"
      : [src_r] "+r"(src_r),        // %[src_r]
        [src_g] "+r"(src_g),        // %[src_g]
        [src_b] "+r"(src_b),        // %[src_b]
        [src_a] "+r"(src_a),        // %[src_a]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(w),                // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [vxrm] "=&r"(vxrm)          // %[vxrm]
      : [shift] "r"(depth - 8)      // %[shift]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
        "v25", "v26", "v27", "v28", "v29", "v30", "v31");
}
#endif

#ifdef HAS_MERGEXRGB16TO8ROW_RVV
void MergeXRGB16To8Row_RVV(const uint16_t* src_r,
                           const uint16_t* src_g,
                           const uint16_t* src_b,
                           uint8_t* dst_argb,
                           int depth,
                           int width) {
  size_t w = (size_t)width;
  size_t vl, vxrm;
  asm volatile(
      "csrr        %[vxrm], vxrm                 \n"
      "csrwi       vxrm, 2                       \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vmv.v.i     v14, -1                       \n"

      "1:                                        \n"
      "vle16.v     v16, (%[src_b])               \n"
      "vle16.v     v20, (%[src_g])               \n"
      "vle16.v     v24, (%[src_r])               \n"
      "vnclipu.wx  v8, v16, %[shift]             \n"
      "vnclipu.wx  v10, v20, %[shift]            \n"
      "vnclipu.wx  v12, v24, %[shift]            \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_r], %[src_r], %[vl]     \n"
      "add         %[src_g], %[src_g], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "bgtz        %[w], 1b                      \n"
      "csrw        vxrm, %[vxrm]                 \n"
      : [src_r] "+r"(src_r),        // %[src_r]
        [src_g] "+r"(src_g),        // %[src_g]
        [src_b] "+r"(src_b),        // %[src_b]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(w),                // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [vxrm] "=&r"(vxrm)          // %[vxrm]
      : [shift] "r"(depth - 8)      // %[shift]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
        "v25", "v26", "v27");
}
#endif

#ifdef HAS_HALFROW_RVV
void HalfRow_RVV(const uint8_t* src_uv,
                 ptrdiff_t src_uv_stride,
                 uint8_t* dst_uv,
                 int width) {
  const uint8_t* src_uv1 = src_uv + src_uv_stride;
  size_t w = (size_t)width;
  size_t vl;
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vle8.v      v8, (%[src_uv])               \n"
      "vle8.v      v12, (%[src_uv1])             \n"
      "vaaddu.vv   v8, v8, v12                   \n"
      "vse8.v      v8, (%[dst_uv])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_uv], %[src_uv], %[vl]   \n"
      "add         %[src_uv1], %[src_uv1], %[vl] \n"
      "add         %[dst_uv], %[dst_uv], %[vl]   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_uv] "+r"(src_uv),    // %[src_uv]
        [src_uv1] "+r"(src_uv1),  // %[src_uv1]
        [dst_uv] "+r"(dst_uv),    // %[dst_uv]
        [w] "+r"(w),              // %[w]
        [vl] "=&r"(vl)            // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_HALFROW_16_RVV
void HalfRow_16_RVV(const uint16_t* src_uv,
                    ptrdiff_t src_uv_stride,
                    uint16_t* dst_uv,
                    int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  int vl;
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v8, (%[src_uv])               \n"
      "vle16.v     v12, (%[src_uv1])             \n"
      "vaaddu.vv   v8, v8, v12                   \n"
      "vse16.v     v8, (%[dst_uv])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_uv], %[src_uv], %[vl]   \n"
      "add         %[src_uv1], %[src_uv1], %[vl] \n"
      "add         %[dst_uv], %[dst_uv], %[vl]   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_uv] "+r"(src_uv),    // %[src_uv]
        [src_uv1] "+r"(src_uv1),  // %[src_uv1]
        [dst_uv] "+r"(dst_uv),    // %[dst_uv]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl)            // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
}
#endif

#ifdef HAS_HALFWIDTHROW_RVV
void HalfWidthRow_RVV(const uint8_t* src_ptr, uint8_t* dst_ptr, int width) {
  int vl;
  // Load bytes, then view them as 16 bit pairs.  A 16 bit load would need an
  // aligned src_ptr.
  asm volatile(
      "csrwi       vxrm, 0                        \n"
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma    \n"
      "sub         %[w], %[w], %[vl]              \n"
      "slli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma    \n"
      "vle8.v      v8, (%[src_ptr])               \n"
      "add         %[src_ptr], %[src_ptr], %[vl]  \n"
      "srli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma    \n"
      "vnsrl.wi    v16, v8, 0                     \n"
      "vnsrl.wi    v18, v8, 8                     \n"
      "vaaddu.vv   v16, v16, v18                  \n"
      "vse8.v      v16, (%[dst_ptr])              \n"
      "add         %[dst_ptr], %[dst_ptr], %[vl]  \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),          // %[w]
        [src_ptr] "+r"(src_ptr),  // %[src_ptr]
        [dst_ptr] "+r"(dst_ptr),  // %[dst_ptr]
        [vl] "=&r"(vl)            // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v16",
        "v17", "v18", "v19");
}

void HalfWidthRow_Odd_RVV(const uint8_t* src_ptr, uint8_t* dst_ptr, int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthRow_RVV(src_ptr, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  dst_ptr[0] = src_ptr[0];
}
#endif

#ifdef HAS_HALFWIDTHROW_16_RVV
void HalfWidthRow_16_RVV(const uint16_t* src_ptr,
                         uint16_t* dst_ptr,
                         int width) {
  int vl;
  asm volatile(
      "csrwi       vxrm, 0                        \n"
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e16, m2, ta, ma   \n"
      "sub         %[w], %[w], %[vl]              \n"
      "slli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e16, m4, ta, ma   \n"
      "vle16.v     v8, (%[src_ptr])               \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[src_ptr], %[src_ptr], %[vl]  \n"
      "srli        %[vl], %[vl], 2                \n"
      "vsetvli     zero, %[vl], e16, m2, ta, ma   \n"
      "vnsrl.wi    v16, v8, 0                     \n"
      "vnsrl.wi    v18, v8, 16                    \n"
      "vaaddu.vv   v16, v16, v18                  \n"
      "vse16.v     v16, (%[dst_ptr])              \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[dst_ptr], %[dst_ptr], %[vl]  \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),          // %[w]
        [src_ptr] "+r"(src_ptr),  // %[src_ptr]
        [dst_ptr] "+r"(dst_ptr),  // %[dst_ptr]
        [vl] "=&r"(vl)            // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v16",
        "v17", "v18", "v19");
}

void HalfWidthRow_16_Odd_RVV(const uint16_t* src_ptr,
                             uint16_t* dst_ptr,
                             int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthRow_16_RVV(src_ptr, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  dst_ptr[0] = src_ptr[0];
}
#endif

#ifdef HAS_HALFWIDTHUVROW_RVV
void HalfWidthUVRow_RVV(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  int vl;
  asm volatile(
      "csrwi       vxrm, 0                        \n"
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e16, m2, ta, ma   \n"
      "sub         %[w], %[w], %[vl]              \n"
      "slli        %[vl], %[vl], 2                \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma    \n"
      "vle8.v      v8, (%[src_uv])                \n"
      "add         %[src_uv], %[src_uv], %[vl]    \n"
      "srli        %[vl], %[vl], 2                \n"
      "vsetvli     zero, %[vl], e16, m2, ta, ma   \n"
      "vnsrl.wi    v16, v8, 0                     \n"
      "vnsrl.wi    v18, v8, 16                    \n"
      "slli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma    \n"
      "vaaddu.vv   v16, v16, v18                  \n"
      "vse8.v      v16, (%[dst_uv])               \n"
      "add         %[dst_uv], %[dst_uv], %[vl]    \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),        // %[w]
        [src_uv] "+r"(src_uv),  // %[src_uv]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v16",
        "v17", "v18", "v19");
}

void HalfWidthUVRow_Odd_RVV(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthUVRow_RVV(src_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  dst_uv[0] = src_uv[0];
  dst_uv[1] = src_uv[1];
}
#endif

#ifdef HAS_HALFWIDTHUVROW_16_RVV
void HalfWidthUVRow_16_RVV(const uint16_t* src_uv,
                           uint16_t* dst_uv,
                           int width) {
  int vl;
#ifdef __riscv_zve64x
  asm volatile(
      "csrwi       vxrm, 0                        \n"
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e32, m4, ta, ma   \n"
      "sub         %[w], %[w], %[vl]              \n"
      "slli        %[vl], %[vl], 2                \n"
      "vsetvli     zero, %[vl], e16, m8, ta, ma   \n"
      "vle16.v     v8, (%[src_uv])                \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[src_uv], %[src_uv], %[vl]    \n"
      "srli        %[vl], %[vl], 3                \n"
      "vsetvli     zero, %[vl], e32, m4, ta, ma   \n"
      "vnsrl.wi    v16, v8, 0                     \n"
      "vnsrl.wx    v20, v8, %[c32]                \n"
      "slli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e16, m4, ta, ma   \n"
      "vaaddu.vv   v16, v16, v20                  \n"
      "vse16.v     v16, (%[dst_uv])               \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[dst_uv], %[dst_uv], %[vl]    \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),        // %[w]
        [src_uv] "+r"(src_uv),  // %[src_uv]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [vl] "=&r"(vl)          // %[vl]
      : [c32] "r"(32)           // %[c32]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
#else
  asm volatile(
      "csrwi       vxrm, 0                        \n"
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e16, m2, ta, ma   \n"
      "vlseg4e16.v v8, (%[src_uv])                \n"
      "sub         %[w], %[w], %[vl]              \n"
      "vaaddu.vv   v8, v8, v12                    \n"
      "vaaddu.vv   v10, v10, v14                  \n"
      "vsseg2e16.v v8, (%[dst_uv])                \n"
      "slli        %[vl], %[vl], 2                \n"
      "add         %[dst_uv], %[dst_uv], %[vl]    \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[src_uv], %[src_uv], %[vl]    \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),        // %[w]
        [src_uv] "+r"(src_uv),  // %[src_uv]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
#endif
}

void HalfWidthUVRow_16_Odd_RVV(const uint16_t* src_uv,
                               uint16_t* dst_uv,
                               int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthUVRow_16_RVV(src_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  dst_uv[0] = src_uv[0];
  dst_uv[1] = src_uv[1];
}
#endif

#ifdef HAS_HALFBOXROW_RVV
void HalfBoxRow_RVV(const uint8_t* src_ptr,
                    ptrdiff_t src_stride,
                    uint8_t* dst_ptr,
                    int width) {
  const uint8_t* t = src_ptr + src_stride;
  int vl;
  asm volatile(
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma    \n"
      "sub         %[w], %[w], %[vl]              \n"
      "slli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma    \n"
      "vle8.v      v8, (%[s])                     \n"
      "vle8.v      v12, (%[t])                    \n"
      "add         %[s], %[s], %[vl]              \n"
      "add         %[t], %[t], %[vl]              \n"
      "srli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma    \n"
      "vnsrl.wi    v16, v8, 0                     \n"
      "vnsrl.wi    v18, v8, 8                     \n"
      "vnsrl.wi    v20, v12, 0                    \n"
      "vnsrl.wi    v22, v12, 8                    \n"
      "vwaddu.vx   v8, v16, %[c2]                 \n"
      "vwaddu.wv   v8, v8, v18                    \n"
      "vwaddu.wv   v8, v8, v20                    \n"
      "vwaddu.wv   v8, v8, v22                    \n"
      "vnsrl.wi    v16, v8, 2                     \n"
      "vse8.v      v16, (%[dst])                  \n"
      "add         %[dst], %[dst], %[vl]          \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),      // %[w]
        [s] "+r"(src_ptr),    // %[s]
        [t] "+r"(t),          // %[t]
        [dst] "+r"(dst_ptr),  // %[dst]
        [vl] "=&r"(vl)        // %[vl]
      : [c2] "r"(2)           // %[c2]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
}

void HalfBoxRow_Odd_RVV(const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        uint8_t* dst_ptr,
                        int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxRow_RVV(src_ptr, src_stride, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  const uint8_t* t = src_ptr + src_stride;
  dst_ptr[0] = (uint8_t)((src_ptr[0] + t[0] + 1) >> 1);
}
#endif

#ifdef HAS_HALFBOXROW_16_RVV
void HalfBoxRow_16_RVV(const uint16_t* src_ptr,
                       ptrdiff_t src_stride,
                       uint16_t* dst_ptr,
                       int width) {
  const uint16_t* t = src_ptr + src_stride;
  int vl;
  asm volatile(
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e16, m2, ta, ma   \n"
      "sub         %[w], %[w], %[vl]              \n"
      "slli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e16, m4, ta, ma   \n"
      "vle16.v     v8, (%[s])                     \n"
      "vle16.v     v12, (%[t])                    \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[s], %[s], %[vl]              \n"
      "add         %[t], %[t], %[vl]              \n"
      "srli        %[vl], %[vl], 2                \n"
      "vsetvli     zero, %[vl], e16, m2, ta, ma   \n"
      "vnsrl.wi    v16, v8, 0                     \n"
      "vnsrl.wi    v18, v8, 16                    \n"
      "vnsrl.wi    v20, v12, 0                    \n"
      "vnsrl.wi    v22, v12, 16                   \n"
      "vwaddu.vx   v8, v16, %[c2]                 \n"
      "vwaddu.wv   v8, v8, v18                    \n"
      "vwaddu.wv   v8, v8, v20                    \n"
      "vwaddu.wv   v8, v8, v22                    \n"
      "vnsrl.wi    v16, v8, 2                     \n"
      "vse16.v     v16, (%[dst])                  \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[dst], %[dst], %[vl]          \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),      // %[w]
        [s] "+r"(src_ptr),    // %[s]
        [t] "+r"(t),          // %[t]
        [dst] "+r"(dst_ptr),  // %[dst]
        [vl] "=&r"(vl)        // %[vl]
      : [c2] "r"(2)           // %[c2]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
}

void HalfBoxRow_16_Odd_RVV(const uint16_t* src_ptr,
                           ptrdiff_t src_stride,
                           uint16_t* dst_ptr,
                           int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxRow_16_RVV(src_ptr, src_stride, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  const uint16_t* t = src_ptr + src_stride;
  dst_ptr[0] = (uint16_t)(((uint32_t)src_ptr[0] + (uint32_t)t[0] + 1u) >> 1);
}
#endif

#ifdef HAS_HALFBOXUVROW_RVV
void HalfBoxUVRow_RVV(const uint8_t* src_uv,
                      ptrdiff_t src_stride_uv,
                      uint8_t* dst_uv,
                      int width) {
  const uint8_t* t_uv = src_uv + src_stride_uv;
  int vl;
  asm volatile(
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e16, m2, ta, ma   \n"
      "sub         %[w], %[w], %[vl]              \n"
      "slli        %[vl], %[vl], 2                \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma    \n"
      "vle8.v      v8, (%[s])                     \n"
      "vle8.v      v12, (%[t])                    \n"
      "add         %[s], %[s], %[vl]              \n"
      "add         %[t], %[t], %[vl]              \n"
      "srli        %[vl], %[vl], 2                \n"
      "vsetvli     zero, %[vl], e16, m2, ta, ma   \n"
      "vnsrl.wi    v16, v8, 0                     \n"
      "vnsrl.wi    v18, v8, 16                    \n"
      "vnsrl.wi    v20, v12, 0                    \n"
      "vnsrl.wi    v22, v12, 16                   \n"
      "slli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma    \n"
      "vwaddu.vx   v8, v16, %[c2]                 \n"
      "vwaddu.wv   v8, v8, v18                    \n"
      "vwaddu.wv   v8, v8, v20                    \n"
      "vwaddu.wv   v8, v8, v22                    \n"
      "vnsrl.wi    v16, v8, 2                     \n"
      "vse8.v      v16, (%[dst_uv])               \n"
      "add         %[dst_uv], %[dst_uv], %[vl]    \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),        // %[w]
        [s] "+r"(src_uv),       // %[s]
        [t] "+r"(t_uv),         // %[t]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [vl] "=&r"(vl)          // %[vl]
      : [c2] "r"(2)             // %[c2]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
}

void HalfBoxUVRow_Odd_RVV(const uint8_t* src_uv,
                          ptrdiff_t src_stride_uv,
                          uint8_t* dst_uv,
                          int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxUVRow_RVV(src_uv, src_stride_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  const uint8_t* t_uv = src_uv + src_stride_uv;
  dst_uv[0] = (uint8_t)((src_uv[0] + t_uv[0] + 1) >> 1);
  dst_uv[1] = (uint8_t)((src_uv[1] + t_uv[1] + 1) >> 1);
}
#endif

#ifdef HAS_HALFBOXUVROW_16_RVV
void HalfBoxUVRow_16_RVV(const uint16_t* src_uv,
                         ptrdiff_t src_stride_uv,
                         uint16_t* dst_uv,
                         int width) {
  const uint16_t* t_uv = src_uv + src_stride_uv;
  int vl;
#ifdef __riscv_zve64x
  asm volatile(
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e32, m2, ta, ma   \n"
      "sub         %[w], %[w], %[vl]              \n"
      "slli        %[vl], %[vl], 2                \n"
      "vsetvli     zero, %[vl], e16, m4, ta, ma   \n"
      "vle16.v     v8, (%[s])                     \n"
      "vle16.v     v12, (%[t])                    \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[s], %[s], %[vl]              \n"
      "add         %[t], %[t], %[vl]              \n"
      "srli        %[vl], %[vl], 3                \n"
      "vsetvli     zero, %[vl], e32, m2, ta, ma   \n"
      "vnsrl.wi    v16, v8, 0                     \n"
      "vnsrl.wx    v18, v8, %[c32]                \n"
      "vnsrl.wi    v20, v12, 0                    \n"
      "vnsrl.wx    v22, v12, %[c32]               \n"
      "slli        %[vl], %[vl], 1                \n"
      "vsetvli     zero, %[vl], e16, m2, ta, ma   \n"
      "vwaddu.vx   v8, v16, %[c2]                 \n"
      "vwaddu.wv   v8, v8, v18                    \n"
      "vwaddu.wv   v8, v8, v20                    \n"
      "vwaddu.wv   v8, v8, v22                    \n"
      "vnsrl.wi    v16, v8, 2                     \n"
      "vse16.v     v16, (%[dst_uv])               \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[dst_uv], %[dst_uv], %[vl]    \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),        // %[w]
        [s] "+r"(src_uv),       // %[s]
        [t] "+r"(t_uv),         // %[t]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [vl] "=&r"(vl)          // %[vl]
      : [c2] "r"(2),            // %[c2]
        [c32] "r"(32)           // %[c32]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
#else
  asm volatile(
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e16, m1, ta, ma   \n"
      "vlseg4e16.v v16, (%[s])                    \n"
      "vlseg4e16.v v20, (%[t])                    \n"
      "sub         %[w], %[w], %[vl]              \n"
      "vwaddu.vx   v8, v16, %[c2]                 \n"
      "vwaddu.vx   v10, v17, %[c2]                \n"
      "vwaddu.wv   v8, v8, v18                    \n"
      "vwaddu.wv   v10, v10, v19                  \n"
      "vwaddu.wv   v8, v8, v20                    \n"
      "vwaddu.wv   v10, v10, v21                  \n"
      "vwaddu.wv   v8, v8, v22                    \n"
      "vwaddu.wv   v10, v10, v23                  \n"
      "vnsrl.wi    v16, v8, 2                     \n"
      "vnsrl.wi    v17, v10, 2                    \n"
      "vsseg2e16.v v16, (%[dst_uv])               \n"
      "slli        %[vl], %[vl], 2                \n"
      "add         %[dst_uv], %[dst_uv], %[vl]    \n"
      "slli        %[vl], %[vl], 1                \n"
      "add         %[s], %[s], %[vl]              \n"
      "add         %[t], %[t], %[vl]              \n"
      "bgtz        %[w], 1b                       \n"
      : [w] "+r"(width),        // %[w]
        [s] "+r"(src_uv),       // %[s]
        [t] "+r"(t_uv),         // %[t]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [vl] "=&r"(vl)          // %[vl]
      : [c2] "r"(2)             // %[c2]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v16",
        "v17", "v18", "v19", "v20", "v21", "v22", "v23");
#endif
}

void HalfBoxUVRow_16_Odd_RVV(const uint16_t* src_uv,
                             ptrdiff_t src_stride_uv,
                             uint16_t* dst_uv,
                             int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxUVRow_16_RVV(src_uv, src_stride_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  const uint16_t* t_uv = src_uv + src_stride_uv;
  dst_uv[0] = (uint16_t)(((uint32_t)src_uv[0] + (uint32_t)t_uv[0] + 1u) >> 1);
  dst_uv[1] = (uint16_t)(((uint32_t)src_uv[1] + (uint32_t)t_uv[1] + 1u) >> 1);
}
#endif

#ifdef HAS_HALFUVROW_RVV
void HalfUVRow_RVV(const uint8_t* src_u,
                   ptrdiff_t src_stride_u,
                   const uint8_t* src_v,
                   ptrdiff_t src_stride_v,
                   uint8_t* dst_uv,
                   int width) {
  const uint8_t* src_u1 = src_u + src_stride_u;
  const uint8_t* src_v1 = src_v + src_stride_v;
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vle8.v      v8, (%[src_u])                \n"
      "vle8.v      v16, (%[src_u1])              \n"
      "vaaddu.vv   v8, v8, v16                   \n"
      "vle8.v      v12, (%[src_v])               \n"
      "vle8.v      v20, (%[src_v1])              \n"
      "vaaddu.vv   v12, v12, v20                 \n"
      "vsseg2e8.v  v8, (%[dst_uv])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_u], %[src_u], %[vl]     \n"
      "add         %[src_u1], %[src_u1], %[vl]   \n"
      "add         %[src_v], %[src_v], %[vl]     \n"
      "add         %[src_v1], %[src_v1], %[vl]   \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_uv], %[dst_uv], %[vl]   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_u] "+r"(src_u),    // %[src_u]
        [src_u1] "+r"(src_u1),  // %[src_u1]
        [src_v] "+r"(src_v),    // %[src_v]
        [src_v1] "+r"(src_v1),  // %[src_v1]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl)          // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
#else
  size_t vl2;
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vle8.v      v8, (%[src_u])                \n"
      "vle8.v      v16, (%[src_u1])              \n"
      "vaaddu.vv   v8, v8, v16                   \n"
      "vle8.v      v12, (%[src_v])               \n"
      "vle8.v      v20, (%[src_v1])              \n"
      "vaaddu.vv   v12, v12, v20                 \n"
      "vwaddu.vv   v16, v8, v12                  \n"
      "vwmaccu.vx  v16, %[c255], v12             \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
      "vse8.v      v16, (%[dst_uv])              \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_u], %[src_u], %[vl]     \n"
      "add         %[src_u1], %[src_u1], %[vl]   \n"
      "add         %[src_v], %[src_v], %[vl]     \n"
      "add         %[src_v1], %[src_v1], %[vl]   \n"
      "add         %[dst_uv], %[dst_uv], %[vl2]  \n"
      "bgtz        %[w], 1b                      \n"
      : [src_u] "+r"(src_u),    // %[src_u]
        [src_u1] "+r"(src_u1),  // %[src_u1]
        [src_v] "+r"(src_v),    // %[src_v]
        [src_v1] "+r"(src_v1),  // %[src_v1]
        [dst_uv] "+r"(dst_uv),  // %[dst_uv]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl),         // %[vl]
        [vl2] "=&r"(vl2)        // %[vl2]
      : [c255] "r"(255)         // %[c255]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
#endif
}
#endif

#ifdef HAS_SWAPUVROW_RVV
void SwapUVRow_RVV(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
  assert(width != 0);
  size_t vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v8, (%[src_uv])               \n"
      "vsrl.vi     v12, v8, 8                    \n"
      "vsll.vi     v8, v8, 8                     \n"
      "vor.vv      v8, v8, v12                   \n"
      "vse16.v     v8, (%[dst_vu])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_uv], %[src_uv], %[vl]   \n"
      "add         %[dst_vu], %[dst_vu], %[vl]   \n"
      "bgtz        %[w], 1b                      \n"
      : [src_uv] "+r"(src_uv),  // %[src_uv]
        [dst_vu] "+r"(dst_vu),  // %[dst_vu]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl)          // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

// ARGB expects first 3 values to contain RGB and 4th value is ignored
#ifdef HAS_ARGBTOYMATRIXROW_RVV
void ARGBToYMatrixRow_RVV(const uint8_t* src_argb,
                          uint8_t* dst_y,
                          int width,
                          const struct ArgbConstants* c) {
  assert(width != 0);
  uint16_t add_y;
  __builtin_memcpy(&add_y, &c->kAddY, sizeof(add_y));
  size_t vl, tmp;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "vsetvli     zero, %[w], e16, m4, ta, ma   \n"
      "vmv.v.x     v8, %[add_y]                  \n"
      "lbu         %[vl], 0(%[c])                \n"
      "lbu         %[tmp], 1(%[c])               \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vmv.v.x     v12, %[vl]                    \n"
      "vmv.v.x     v14, %[tmp]                   \n"
      "lbu         %[vl], 2(%[c])                \n"
      "lbu         %[tmp], 3(%[c])               \n"
      "vmv.v.x     v16, %[vl]                    \n"
      "vmv.v.x     v18, %[tmp]                   \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v24, (%[src_argb])            \n"
      "vwmulu.vv   v20, v24, v12                 \n"
      "vwmaccu.vv  v20, v14, v26                 \n"
      "vwmaccu.vv  v20, v16, v28                 \n"
      "vwmaccu.vv  v20, v18, v30                 \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vadd.vv     v20, v20, v8                  \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v24, v20, 8                   \n"
      "vse8.v      v24, (%[dst_y])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_y], %[dst_y], %[vl]     \n"
      "slli        %[tmp], %[vl], 2              \n"
      "add         %[src_argb], %[src_argb], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_y] "+r"(dst_y),        // %[dst_y]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [tmp] "=&r"(tmp)            // %[tmp]
      : [c] "r"(c),                 // %[c]
        [add_y] "r"(add_y)          // %[add_y]
      : "memory", RVV_VL_CLOBBER "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
        "v25", "v26", "v27", "v28", "v29", "v30", "v31");
#else
  asm volatile(
      "vsetvli     zero, %[w], e16, m4, ta, ma   \n"
      "vmv.v.x     v0, %[add_y]                  \n"
      "lbu         %[vl], 0(%[c])                \n"
      "lbu         %[tmp], 1(%[c])               \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vmv.v.x     v4, %[vl]                     \n"
      "vmv.v.x     v6, %[tmp]                    \n"
      "lbu         %[vl], 2(%[c])                \n"
      "lbu         %[tmp], 3(%[c])               \n"
      "vmv.v.x     v8, %[vl]                     \n"
      "vmv.v.x     v10, %[tmp]                   \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "slli        %[tmp], %[vl], 2              \n"
      "vsetvli     zero, %[tmp], e8, m8, ta, ma  \n"
      "vle8.v      v16, (%[src_argb])            \n"
      "vsetvli     zero, %[vl], e16, m4, ta, ma  \n"
      "vnsrl.wi    v24, v16, 0                   \n"
      "vnsrl.wi    v28, v16, 16                  \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "vnsrl.wi    v12, v24, 0                   \n"
      "vnsrl.wi    v14, v24, 8                   \n"
      "vnsrl.wi    v16, v28, 0                   \n"
      "vnsrl.wi    v18, v28, 8                   \n"
      "vwmulu.vv   v20, v12, v4                  \n"
      "vwmaccu.vv  v20, v6, v14                  \n"
      "vwmaccu.vv  v20, v8, v16                  \n"
      "vwmaccu.vv  v20, v10, v18                 \n"
      "vsetvli     zero, %[vl], e16, m4, ta, ma  \n"
      "vadd.vv     v20, v20, v0                  \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "vnsrl.wi    v24, v20, 8                   \n"
      "vse8.v      v24, (%[dst_y])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_y], %[dst_y], %[vl]     \n"
      "add         %[src_argb], %[src_argb], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_y] "+r"(dst_y),        // %[dst_y]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [tmp] "=&r"(tmp)            // %[tmp]
      : [c] "r"(c),                 // %[c]
        [add_y] "r"(add_y)          // %[add_y]
      : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
        "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17",
        "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",
        "v28", "v29", "v30", "v31");
#endif
}
#endif

#ifdef HAS_RGBTOYMATRIXROW_RVV
void RGBToYMatrixRow_RVV(const uint8_t* src_rgb,
                         uint8_t* dst_y,
                         int width,
                         const struct ArgbConstants* c) {
  assert(width != 0);
  uint16_t add_y;
  __builtin_memcpy(&add_y, &c->kAddY, sizeof(add_y));
  size_t vl, tmp;
  asm volatile(
      "vsetvli     zero, %[w], e16, m4, ta, ma   \n"
      "vmv.v.x     v8, %[add_y]                  \n"
      "lbu         %[vl], 0(%[c])                \n"
      "lbu         %[tmp], 1(%[c])               \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vmv.v.x     v12, %[vl]                    \n"
      "vmv.v.x     v14, %[tmp]                   \n"
      "lbu         %[vl], 2(%[c])                \n"
      "vmv.v.x     v16, %[vl]                    \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg3e8.v  v24, (%[src_rgb])             \n"
      "vwmulu.vv   v20, v28, v16                 \n"
      "vwmaccu.vv  v20, v14, v26                 \n"
      "vwmaccu.vv  v20, v12, v24                 \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vadd.vv     v20, v20, v8                  \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v18, v20, 8                   \n"
      "vse8.v      v18, (%[dst_y])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_y], %[dst_y], %[vl]     \n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[src_rgb], %[src_rgb], %[vl] \n"
      "add         %[src_rgb], %[src_rgb], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_rgb] "+r"(src_rgb),  // %[src_rgb]
        [dst_y] "+r"(dst_y),      // %[dst_y]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [tmp] "=&r"(tmp)          // %[tmp]
      : [c] "r"(c),               // %[c]
        [add_y] "r"(add_y)        // %[add_y]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
        "v25", "v26", "v27", "v28", "v29");
}
#endif

#ifdef HAS_ARGBTOUV444MATRIXROW_RVV
void ARGBToUV444MatrixRow_RVV(const uint8_t* src_argb,
                              uint8_t* dst_u,
                              uint8_t* dst_v,
                              int width,
                              const struct ArgbConstants* c) {
  assert(width != 0);
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV, sizeof(add_uv));
  size_t vl, tmp;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "vsetvli     zero, %[w], e16, m4, ta, ma   \n"
      "vmv.v.x     v16, %[add_uv]                \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "lb          %[vl], 16(%[c])               \n"
      "lb          %[tmp], 17(%[c])              \n"
      "vmv.v.x     v0, %[vl]                     \n"
      "vmv.v.x     v2, %[tmp]                    \n"
      "lb          %[vl], 18(%[c])               \n"
      "lb          %[tmp], 19(%[c])              \n"
      "vmv.v.x     v4, %[vl]                     \n"
      "vmv.v.x     v6, %[tmp]                    \n"
      "lb          %[vl], 32(%[c])               \n"
      "lb          %[tmp], 33(%[c])              \n"
      "vmv.v.x     v8, %[vl]                     \n"
      "vmv.v.x     v10, %[tmp]                   \n"
      "lb          %[vl], 34(%[c])               \n"
      "lb          %[tmp], 35(%[c])              \n"
      "vmv.v.x     v12, %[vl]                    \n"
      "vmv.v.x     v14, %[tmp]                   \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v24, (%[src_argb])            \n"
      "vwmulsu.vv  v20, v0, v24                  \n"
      "vwmaccsu.vv v20, v2, v26                  \n"
      "vwmaccsu.vv v20, v4, v28                  \n"
      "vwmaccsu.vv v20, v6, v30                  \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vsub.vv     v20, v16, v20                 \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v20, v20, 8                   \n"
      "vse8.v      v20, (%[dst_u])               \n"
      "vwmulsu.vv  v20, v8, v24                  \n"
      "vwmaccsu.vv v20, v10, v26                 \n"
      "vwmaccsu.vv v20, v12, v28                 \n"
      "vwmaccsu.vv v20, v14, v30                 \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vsub.vv     v20, v16, v20                 \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v20, v20, 8                   \n"
      "vse8.v      v20, (%[dst_v])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_u], %[dst_u], %[vl]     \n"
      "add         %[dst_v], %[dst_v], %[vl]     \n"
      "slli        %[tmp], %[vl], 2              \n"
      "add         %[src_argb], %[src_argb], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_u] "+r"(dst_u),        // %[dst_u]
        [dst_v] "+r"(dst_v),        // %[dst_v]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [tmp] "=&r"(tmp)            // %[tmp]
      : [c] "r"(c),                 // %[c]
        [add_uv] "r"(add_uv)        // %[add_uv]
      : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
        "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17",
        "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",
        "v28", "v29", "v30", "v31");
#else
  asm volatile(
      "vsetvli     zero, %[w], e16, m2, ta, ma   \n"
      "vmv.v.x     v8, %[add_uv]                 \n"
      "vsetvli     zero, zero, e8, m1, ta, ma    \n"
      "lb          %[vl], 16(%[c])               \n"
      "lb          %[tmp], 17(%[c])              \n"
      "vmv.v.x     v10, %[vl]                    \n"
      "vmv.v.x     v11, %[tmp]                   \n"
      "lb          %[vl], 18(%[c])               \n"
      "lb          %[tmp], 19(%[c])              \n"
      "vmv.v.x     v12, %[vl]                    \n"
      "vmv.v.x     v13, %[tmp]                   \n"
      "lb          %[vl], 32(%[c])               \n"
      "lb          %[tmp], 33(%[c])              \n"
      "vmv.v.x     v14, %[vl]                    \n"
      "vmv.v.x     v15, %[tmp]                   \n"
      "lb          %[vl], 34(%[c])               \n"
      "lb          %[tmp], 35(%[c])              \n"
      "vmv.v.x     v16, %[vl]                    \n"
      "vmv.v.x     v17, %[tmp]                   \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m1, ta, ma   \n"
      "slli        %[tmp], %[vl], 2              \n"
      "vsetvli     zero, %[tmp], e8, m4, ta, ma  \n"
      "vle8.v      v0, (%[src_argb])             \n"
      "vsetvli     zero, %[vl], e16, m2, ta, ma  \n"
      "vnsrl.wi    v4, v0, 0                     \n"
      "vnsrl.wi    v6, v0, 16                    \n"
      "vsetvli     zero, %[vl], e8, m1, ta, ma   \n"
      "vnsrl.wi    v20, v4, 0                    \n"
      "vnsrl.wi    v21, v4, 8                    \n"
      "vnsrl.wi    v22, v6, 0                    \n"
      "vnsrl.wi    v23, v6, 8                    \n"
      "vwmulsu.vv  v24, v10, v20                 \n"
      "vwmaccsu.vv v24, v11, v21                 \n"
      "vwmaccsu.vv v24, v12, v22                 \n"
      "vwmaccsu.vv v24, v13, v23                 \n"
      "vwmulsu.vv  v26, v14, v20                 \n"
      "vwmaccsu.vv v26, v15, v21                 \n"
      "vwmaccsu.vv v26, v16, v22                 \n"
      "vwmaccsu.vv v26, v17, v23                 \n"
      "vsetvli     zero, %[vl], e16, m2, ta, ma  \n"
      "vsub.vv     v24, v8, v24                  \n"
      "vsub.vv     v26, v8, v26                  \n"
      "vsetvli     zero, %[vl], e8, m1, ta, ma   \n"
      "vnsrl.wi    v20, v24, 8                   \n"
      "vnsrl.wi    v21, v26, 8                   \n"
      "vse8.v      v20, (%[dst_u])               \n"
      "vse8.v      v21, (%[dst_v])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_u], %[dst_u], %[vl]     \n"
      "add         %[dst_v], %[dst_v], %[vl]     \n"
      "add         %[src_argb], %[src_argb], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_u] "+r"(dst_u),        // %[dst_u]
        [dst_v] "+r"(dst_v),        // %[dst_v]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl),             // %[vl]
        [tmp] "=&r"(tmp)            // %[tmp]
      : [c] "r"(c),                 // %[c]
        [add_uv] "r"(add_uv)        // %[add_uv]
      : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
        "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17",
        "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27");
#endif
}
#endif

#ifdef HAS_ARGBTOUVMATRIXROW_RVV
void ARGBToUVMatrixRow_RVV(const uint8_t* src_argb,
                           int src_stride_argb,
                           uint8_t* dst_u,
                           uint8_t* dst_v,
                           int width,
                           const struct ArgbConstants* c) {
  assert(width != 0);
  const uint8_t* src_argb_0 = src_argb;
  const uint8_t* src_argb_1 = src_argb + src_stride_argb;
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV, sizeof(add_uv));
  int w = width >> 1;
  if (w > 0) {
    size_t w_len = (size_t)w;
    size_t vl, tmp;
#if !defined(LIBYUV_USE_VLSEG) && defined(__riscv_zve64x)
    asm volatile(
        "csrwi       vxrm, 0                       \n"
        "vsetvli     zero, %[w_len], e16, m1, ta, ma\n"
        "vmv.v.x     v8, %[add_uv]                 \n"
        "vsetvli     zero, %[w_len], e8, mf2, ta, ma\n"
        "lb          %[vl], 16(%[c])               \n"
        "lb          %[tmp], 17(%[c])              \n"
        "vmv.v.x     v9, %[vl]                     \n"
        "vmv.v.x     v10, %[tmp]                   \n"
        "lb          %[vl], 18(%[c])               \n"
        "lb          %[tmp], 19(%[c])              \n"
        "vmv.v.x     v11, %[vl]                    \n"
        "vmv.v.x     v12, %[tmp]                   \n"
        "lb          %[vl], 32(%[c])               \n"
        "lb          %[tmp], 33(%[c])              \n"
        "vmv.v.x     v13, %[vl]                    \n"
        "vmv.v.x     v14, %[tmp]                   \n"
        "lb          %[vl], 34(%[c])               \n"
        "lb          %[tmp], 35(%[c])              \n"
        "vmv.v.x     v15, %[vl]                    \n"
        "vmv.v.x     v16, %[tmp]                   \n"

        "1:          \n"
        "vsetvli     %[vl], %[w_len], e64, m4, ta, ma\n"
        "slli        %[tmp], %[vl], 3              \n"
        "vsetvli     zero, %[tmp], e8, m4, ta, ma  \n"
        "vle8.v      v0, (%[src_argb_0])           \n"
        "vle8.v      v4, (%[src_argb_1])           \n"
        "vsetvli     zero, %[vl], e32, m2, ta, ma  \n"
        "vnsrl.wi    v20, v0, 0                    \n"
        "vnsrl.wx    v22, v0, %[c32]               \n"
        "vnsrl.wi    v24, v4, 0                    \n"
        "vnsrl.wx    v26, v4, %[c32]               \n"
        "slli        %[tmp], %[vl], 2              \n"
        "vsetvli     zero, %[tmp], e8, m2, ta, ma  \n"
        "vwaddu.vv   v0, v20, v22                  \n"
        "vwaddu.wv   v0, v0, v24                   \n"
        "vwaddu.wv   v0, v0, v26                   \n"
        "vnclipu.wi  v20, v0, 2                    \n"
        "vsetvli     zero, %[vl], e16, m1, ta, ma  \n"
        "vnsrl.wi    v0, v20, 0                    \n"
        "vnsrl.wi    v1, v20, 16                   \n"
        "vsetvli     zero, %[vl], e8, mf2, ta, ma  \n"
        "vnsrl.wi    v20, v0, 0                    \n"
        "vnsrl.wi    v21, v0, 8                    \n"
        "vnsrl.wi    v22, v1, 0                    \n"
        "vnsrl.wi    v23, v1, 8                    \n"
        "vwmulsu.vv  v0, v9, v20                   \n"
        "vwmaccsu.vv v0, v10, v21                  \n"
        "vwmaccsu.vv v0, v11, v22                  \n"
        "vwmaccsu.vv v0, v12, v23                  \n"
        "vwmulsu.vv  v2, v13, v20                  \n"
        "vwmaccsu.vv v2, v14, v21                  \n"
        "vwmaccsu.vv v2, v15, v22                  \n"
        "vwmaccsu.vv v2, v16, v23                  \n"
        "vsetvli     zero, %[vl], e16, m1, ta, ma  \n"
        "vsub.vv     v0, v8, v0                    \n"
        "vsub.vv     v2, v8, v2                    \n"
        "vsetvli     zero, %[vl], e8, mf2, ta, ma  \n"
        "vnsrl.wi    v20, v0, 8                    \n"
        "vnsrl.wi    v21, v2, 8                    \n"
        "vse8.v      v20, (%[dst_u])               \n"
        "vse8.v      v21, (%[dst_v])               \n"
        "sub         %[w_len], %[w_len], %[vl]     \n"
        "add         %[dst_u], %[dst_u], %[vl]     \n"
        "add         %[dst_v], %[dst_v], %[vl]     \n"
        "slli        %[tmp], %[vl], 3              \n"
        "add         %[src_argb_0], %[src_argb_0], %[tmp]\n"
        "add         %[src_argb_1], %[src_argb_1], %[tmp]\n"
        "bgtz        %[w_len], 1b                  \n"
        : [src_argb_0] "+r"(src_argb_0),  // %[src_argb_0]
          [src_argb_1] "+r"(src_argb_1),  // %[src_argb_1]
          [dst_u] "+r"(dst_u),            // %[dst_u]
          [dst_v] "+r"(dst_v),            // %[dst_v]
          [w_len] "+r"(w_len),            // %[w_len]
          [vl] "=&r"(vl),                 // %[vl]
          [tmp] "=&r"(tmp)                // %[tmp]
        : [c] "r"(c),                     // %[c]
          [add_uv] "r"(add_uv),           // %[add_uv]
          [c32] "r"(32)                   // %[c32]
        : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6",
          "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16",
          "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27");
#else
    asm volatile(
        "csrwi       vxrm, 0                       \n"
        "vsetvli     zero, %[w_len], e16, m2, ta, ma\n"
        "vmv.v.x     v8, %[add_uv]                 \n"
        "vsetvli     zero, zero, e8, m1, ta, ma    \n"
        "lb          %[vl], 16(%[c])               \n"
        "lb          %[tmp], 17(%[c])              \n"
        "vmv.v.x     v10, %[vl]                    \n"
        "vmv.v.x     v11, %[tmp]                   \n"
        "lb          %[vl], 18(%[c])               \n"
        "lb          %[tmp], 19(%[c])              \n"
        "vmv.v.x     v12, %[vl]                    \n"
        "vmv.v.x     v13, %[tmp]                   \n"
        "lb          %[vl], 32(%[c])               \n"
        "lb          %[tmp], 33(%[c])              \n"
        "vmv.v.x     v14, %[vl]                    \n"
        "vmv.v.x     v15, %[tmp]                   \n"
        "lb          %[vl], 34(%[c])               \n"
        "lb          %[tmp], 35(%[c])              \n"
        "vmv.v.x     v16, %[vl]                    \n"
        "vmv.v.x     v17, %[tmp]                   \n"

        "1:          \n"
        "vsetvli     %[vl], %[w_len], e8, m1, ta, ma\n"
        "vlseg8e8.v  v20, (%[src_argb_0])          \n"
        "vlseg8e8.v  v0, (%[src_argb_1])           \n"
        "vwaddu.vv   v18, v20, v24                 \n"
        "vwaddu.wv   v18, v18, v0                  \n"
        "vwaddu.wv   v18, v18, v4                  \n"
        "vnclipu.wi  v20, v18, 2                   \n"
        "vwaddu.vv   v18, v21, v25                 \n"
        "vwaddu.wv   v18, v18, v1                  \n"
        "vwaddu.wv   v18, v18, v5                  \n"
        "vnclipu.wi  v21, v18, 2                   \n"
        "vwaddu.vv   v18, v22, v26                 \n"
        "vwaddu.wv   v18, v18, v2                  \n"
        "vwaddu.wv   v18, v18, v6                  \n"
        "vnclipu.wi  v22, v18, 2                   \n"
        "vwaddu.vv   v18, v23, v27                 \n"
        "vwaddu.wv   v18, v18, v3                  \n"
        "vwaddu.wv   v18, v18, v7                  \n"
        "vnclipu.wi  v23, v18, 2                   \n"
        "vwmulsu.vv  v18, v10, v20                 \n"
        "vwmaccsu.vv v18, v11, v21                 \n"
        "vwmaccsu.vv v18, v12, v22                 \n"
        "vwmaccsu.vv v18, v13, v23                 \n"
        "vwmulsu.vv  v24, v14, v20                 \n"
        "vwmaccsu.vv v24, v15, v21                 \n"
        "vwmaccsu.vv v24, v16, v22                 \n"
        "vwmaccsu.vv v24, v17, v23                 \n"
        "vsetvli     zero, zero, e16, m2, ta, ma   \n"
        "vsub.vv     v18, v8, v18                  \n"
        "vsub.vv     v24, v8, v24                  \n"
        "vsetvli     zero, zero, e8, m1, ta, ma    \n"
        "vnsrl.wi    v20, v18, 8                   \n"
        "vnsrl.wi    v21, v24, 8                   \n"
        "vse8.v      v20, (%[dst_u])               \n"
        "vse8.v      v21, (%[dst_v])               \n"
        "sub         %[w_len], %[w_len], %[vl]     \n"
        "add         %[dst_u], %[dst_u], %[vl]     \n"
        "add         %[dst_v], %[dst_v], %[vl]     \n"
        "slli        %[tmp], %[vl], 3              \n"
        "add         %[src_argb_0], %[src_argb_0], %[tmp]\n"
        "add         %[src_argb_1], %[src_argb_1], %[tmp]\n"
        "bgtz        %[w_len], 1b                  \n"
        : [src_argb_0] "+r"(src_argb_0),  // %[src_argb_0]
          [src_argb_1] "+r"(src_argb_1),  // %[src_argb_1]
          [dst_u] "+r"(dst_u),            // %[dst_u]
          [dst_v] "+r"(dst_v),            // %[dst_v]
          [w_len] "+r"(w_len),            // %[w_len]
          [vl] "=&r"(vl),                 // %[vl]
          [tmp] "=&r"(tmp)                // %[tmp]
        : [c] "r"(c),                     // %[c]
          [add_uv] "r"(add_uv)            // %[add_uv]
        : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6",
          "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16",
          "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26",
          "v27");
#endif
  }
  if (width & 1) {
    const int8_t* ku = (const int8_t*)&c->kRGBToU;
    const int8_t* kv = (const int8_t*)&c->kRGBToV;
    uint8_t b = (src_argb_0[0] + src_argb_1[0] + 1) >> 1;
    uint8_t g = (src_argb_0[1] + src_argb_1[1] + 1) >> 1;
    uint8_t r = (src_argb_0[2] + src_argb_1[2] + 1) >> 1;
    uint8_t a = (src_argb_0[3] + src_argb_1[3] + 1) >> 1;
    dst_u[0] = (add_uv - (ku[0] * b + ku[1] * g + ku[2] * r + ku[3] * a)) >> 8;
    dst_v[0] = (add_uv - (kv[0] * b + kv[1] * g + kv[2] * r + kv[3] * a)) >> 8;
  }
}
#endif

#ifdef HAS_RGBTOUV444MATRIXROW_RVV
void RGBToUV444MatrixRow_RVV(const uint8_t* src_rgb,
                             uint8_t* dst_u,
                             uint8_t* dst_v,
                             int width,
                             const struct ArgbConstants* c) {
  assert(width != 0);
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV, sizeof(add_uv));
  size_t vl, tmp;
  asm volatile(
      "vsetvli     zero, %[w], e16, m4, ta, ma   \n"
      "vmv.v.x     v16, %[add_uv]                \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "lb          %[vl], 16(%[c])               \n"
      "lb          %[tmp], 17(%[c])              \n"
      "vmv.v.x     v0, %[vl]                     \n"
      "vmv.v.x     v2, %[tmp]                    \n"
      "lb          %[vl], 18(%[c])               \n"
      "vmv.v.x     v4, %[vl]                     \n"
      "lb          %[vl], 32(%[c])               \n"
      "lb          %[tmp], 33(%[c])              \n"
      "vmv.v.x     v8, %[vl]                     \n"
      "vmv.v.x     v10, %[tmp]                   \n"
      "lb          %[vl], 34(%[c])               \n"
      "vmv.v.x     v12, %[vl]                    \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg3e8.v  v24, (%[src_rgb])             \n"
      "vwmulsu.vv  v20, v4, v28                  \n"
      "vwmaccsu.vv v20, v2, v26                  \n"
      "vwmaccsu.vv v20, v0, v24                  \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vsub.vv     v20, v16, v20                 \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v20, v20, 8                   \n"
      "vse8.v      v20, (%[dst_u])               \n"
      "vwmulsu.vv  v20, v12, v28                 \n"
      "vwmaccsu.vv v20, v10, v26                 \n"
      "vwmaccsu.vv v20, v8, v24                  \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vsub.vv     v20, v16, v20                 \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v20, v20, 8                   \n"
      "vse8.v      v20, (%[dst_v])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_u], %[dst_u], %[vl]     \n"
      "add         %[dst_v], %[dst_v], %[vl]     \n"
      "slli        %[tmp], %[vl], 1              \n"
      "add         %[src_rgb], %[src_rgb], %[vl] \n"
      "add         %[src_rgb], %[src_rgb], %[tmp]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_rgb] "+r"(src_rgb),  // %[src_rgb]
        [dst_u] "+r"(dst_u),      // %[dst_u]
        [dst_v] "+r"(dst_v),      // %[dst_v]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [tmp] "=&r"(tmp)          // %[tmp]
      : [c] "r"(c),               // %[c]
        [add_uv] "r"(add_uv)      // %[add_uv]
      : RVV_VL_CLOBBER "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v8", "v9",
        "v10", "v11", "v12", "v13", "v16", "v17", "v18", "v19", "v20", "v21",
        "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29");
}
#endif

#ifdef HAS_RGBTOUVMATRIXROW_RVV
void RGBToUVMatrixRow_RVV(const uint8_t* src_rgb,
                          int src_stride_rgb,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width,
                          const struct ArgbConstants* c) {
  assert(width != 0);
  const uint8_t* src_rgb_0 = src_rgb;
  const uint8_t* src_rgb_1 = src_rgb + src_stride_rgb;
  uint16_t add_uv;
  __builtin_memcpy(&add_uv, &c->kAddUV, sizeof(add_uv));
  int w = width >> 1;
  if (w > 0) {
    size_t w_len = (size_t)w;
    size_t vl, tmp;
#if defined(LIBYUV_USE_VLSEG)
    asm volatile(
        "csrwi       vxrm, 0                       \n"
        "vsetvli     zero, %[w_len], e16, m2, ta, ma\n"
        "vmv.v.x     v8, %[add_uv]                 \n"
        "vsetvli     zero, zero, e8, m1, ta, ma    \n"
        "lb          %[vl], 16(%[c])               \n"
        "lb          %[tmp], 17(%[c])              \n"
        "vmv.v.x     v10, %[vl]                    \n"
        "vmv.v.x     v11, %[tmp]                   \n"
        "lb          %[vl], 18(%[c])               \n"
        "vmv.v.x     v12, %[vl]                    \n"
        "lb          %[vl], 32(%[c])               \n"
        "lb          %[tmp], 33(%[c])              \n"
        "vmv.v.x     v14, %[vl]                    \n"
        "vmv.v.x     v15, %[tmp]                   \n"
        "lb          %[vl], 34(%[c])               \n"
        "vmv.v.x     v16, %[vl]                    \n"

        "1:          \n"
        "vsetvli     %[vl], %[w_len], e8, m1, ta, ma\n"
        "vlseg6e8.v  v20, (%[src_rgb_0])           \n"
        "vlseg6e8.v  v0, (%[src_rgb_1])            \n"
        "vwaddu.vv   v18, v20, v23                 \n"
        "vwaddu.wv   v18, v18, v0                  \n"
        "vwaddu.wv   v18, v18, v3                  \n"
        "vnclipu.wi  v20, v18, 2                   \n"
        "vwaddu.vv   v18, v21, v24                 \n"
        "vwaddu.wv   v18, v18, v1                  \n"
        "vwaddu.wv   v18, v18, v4                  \n"
        "vnclipu.wi  v21, v18, 2                   \n"
        "vwaddu.vv   v18, v22, v25                 \n"
        "vwaddu.wv   v18, v18, v2                  \n"
        "vwaddu.wv   v18, v18, v5                  \n"
        "vnclipu.wi  v22, v18, 2                   \n"
        "vwmulsu.vv  v18, v12, v22                 \n"
        "vwmaccsu.vv v18, v11, v21                 \n"
        "vwmaccsu.vv v18, v10, v20                 \n"
        "vwmulsu.vv  v24, v16, v22                 \n"
        "vwmaccsu.vv v24, v15, v21                 \n"
        "vwmaccsu.vv v24, v14, v20                 \n"
        "vsetvli     zero, zero, e16, m2, ta, ma   \n"
        "vsub.vv     v18, v8, v18                  \n"
        "vsub.vv     v24, v8, v24                  \n"
        "vsetvli     zero, zero, e8, m1, ta, ma    \n"
        "vnsrl.wi    v20, v18, 8                   \n"
        "vnsrl.wi    v21, v24, 8                   \n"
        "vse8.v      v20, (%[dst_u])               \n"
        "vse8.v      v21, (%[dst_v])               \n"
        "sub         %[w_len], %[w_len], %[vl]     \n"
        "add         %[dst_u], %[dst_u], %[vl]     \n"
        "add         %[dst_v], %[dst_v], %[vl]     \n"
        "slli        %[tmp], %[vl], 1              \n"
        "add         %[tmp], %[tmp], %[vl]         \n"
        "slli        %[tmp], %[tmp], 1              \n"
        "add         %[src_rgb_0], %[src_rgb_0], %[tmp]\n"
        "add         %[src_rgb_1], %[src_rgb_1], %[tmp]\n"
        "bgtz        %[w_len], 1b                  \n"
        : [src_rgb_0] "+r"(src_rgb_0),  // %[src_rgb_0]
          [src_rgb_1] "+r"(src_rgb_1),  // %[src_rgb_1]
          [dst_u] "+r"(dst_u),          // %[dst_u]
          [dst_v] "+r"(dst_v),          // %[dst_v]
          [w_len] "+r"(w_len),          // %[w_len]
          [vl] "=&r"(vl),               // %[vl]
          [tmp] "=&r"(tmp)              // %[tmp]
        : [c] "r"(c),                   // %[c]
          [add_uv] "r"(add_uv)          // %[add_uv]
        : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v8",
          "v9", "v10", "v11", "v12", "v14", "v15", "v16", "v18", "v19", "v20",
          "v21", "v22", "v23", "v24", "v25");
#else
    size_t max_vl;
    asm volatile(
        "csrwi       vxrm, 0                       \n"
        "vsetvli     %[max_vl], zero, e8, m1, ta, ma\n"
        "divu        %[max_vl], %[max_vl], %[c6]   \n"
        "vsetvli     zero, %[max_vl], e16, m2, ta, ma\n"
        "vmv.v.x     v8, %[add_uv]                 \n"
        "vid.v       v20                           \n"
        "vmul.vx     v20, v20, %[c6]               \n"
        "vadd.vi     v22, v20, 1                   \n"
        "vadd.vi     v24, v20, 2                   \n"
        "vadd.vi     v26, v20, 3                   \n"
        "vadd.vi     v28, v20, 4                   \n"
        "vadd.vi     v30, v20, 5                   \n"
        "vsetvli     zero, %[max_vl], e8, m1, ta, ma\n"
        "lb          %[vl], 16(%[c])               \n"
        "lb          %[tmp], 17(%[c])              \n"
        "vmv.v.x     v10, %[vl]                    \n"
        "vmv.v.x     v11, %[tmp]                   \n"
        "lb          %[vl], 18(%[c])               \n"
        "vmv.v.x     v12, %[vl]                    \n"
        "lb          %[vl], 32(%[c])               \n"
        "lb          %[tmp], 33(%[c])              \n"
        "vmv.v.x     v14, %[vl]                    \n"
        "vmv.v.x     v15, %[tmp]                   \n"
        "lb          %[vl], 34(%[c])               \n"
        "vmv.v.x     v16, %[vl]                    \n"

        "1:          \n"
        "mv          %[vl], %[w_len]               \n"
        "bltu        %[w_len], %[max_vl], 2f       \n"
        "mv          %[vl], %[max_vl]              \n"
        "2:          \n"
        "mul         %[tmp], %[vl], %[c6]          \n"
        "vsetvli     zero, %[tmp], e8, m1, ta, ma  \n"
        "vle8.v      v0, (%[src_rgb_0])            \n"
        "vle8.v      v1, (%[src_rgb_1])            \n"
        "vwaddu.vv   v2, v0, v1                    \n"
        "vsetvli     zero, %[vl], e16, m2, ta, ma  \n"
        "vrgather.vv v4, v2, v20                   \n"
        "vrgather.vv v6, v2, v26                   \n"
        "vadd.vv     v4, v4, v6                    \n"
        "vrgather.vv v6, v2, v22                   \n"
        "vrgather.vv v18, v2, v28                  \n"
        "vadd.vv     v6, v6, v18                   \n"
        "vrgather.vv v18, v2, v24                  \n"
        "vrgather.vv v0, v2, v30                   \n"
        "vadd.vv     v18, v18, v0                  \n"
        "vsetvli     zero, %[vl], e8, m1, ta, ma   \n"
        "vnclipu.wi  v0, v4, 2                     \n"
        "vnclipu.wi  v1, v6, 2                     \n"
        "vnclipu.wi  v2, v18, 2                    \n"
        "vwmulsu.vv  v4, v12, v2                   \n"
        "vwmaccsu.vv v4, v11, v1                   \n"
        "vwmaccsu.vv v4, v10, v0                   \n"
        "vwmulsu.vv  v6, v16, v2                   \n"
        "vwmaccsu.vv v6, v15, v1                   \n"
        "vwmaccsu.vv v6, v14, v0                   \n"
        "vsetvli     zero, %[vl], e16, m2, ta, ma  \n"
        "vsub.vv     v4, v8, v4                    \n"
        "vsub.vv     v6, v8, v6                    \n"
        "vsetvli     zero, %[vl], e8, m1, ta, ma   \n"
        "vnsrl.wi    v0, v4, 8                     \n"
        "vnsrl.wi    v1, v6, 8                     \n"
        "vse8.v      v0, (%[dst_u])                \n"
        "vse8.v      v1, (%[dst_v])                \n"
        "sub         %[w_len], %[w_len], %[vl]     \n"
        "add         %[dst_u], %[dst_u], %[vl]     \n"
        "add         %[dst_v], %[dst_v], %[vl]     \n"
        "add         %[src_rgb_0], %[src_rgb_0], %[tmp]\n"
        "add         %[src_rgb_1], %[src_rgb_1], %[tmp]\n"
        "bgtz        %[w_len], 1b                  \n"
        : [src_rgb_0] "+r"(src_rgb_0),  // %[src_rgb_0]
          [src_rgb_1] "+r"(src_rgb_1),  // %[src_rgb_1]
          [dst_u] "+r"(dst_u),          // %[dst_u]
          [dst_v] "+r"(dst_v),          // %[dst_v]
          [w_len] "+r"(w_len),          // %[w_len]
          [vl] "=&r"(vl),               // %[vl]
          [tmp] "=&r"(tmp),             // %[tmp]
          [max_vl] "=&r"(max_vl)        // %[max_vl]
        : [c] "r"(c),                   // %[c]
          [add_uv] "r"(add_uv),         // %[add_uv]
          [c6] "r"(6)                   // %[c6]
        : "memory", RVV_VL_CLOBBER "v0", "v1", "v2", "v3", "v4", "v5", "v6",
          "v7", "v8", "v9", "v10", "v11", "v12", "v14", "v15", "v16", "v18",
          "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28",
          "v29", "v30", "v31");
#endif
  }
  if (width & 1) {
    const int8_t* ku = (const int8_t*)&c->kRGBToU;
    const int8_t* kv = (const int8_t*)&c->kRGBToV;
    uint8_t b = (src_rgb_0[0] + src_rgb_1[0] + 1) >> 1;
    uint8_t g = (src_rgb_0[1] + src_rgb_1[1] + 1) >> 1;
    uint8_t r = (src_rgb_0[2] + src_rgb_1[2] + 1) >> 1;
    dst_u[0] = (add_uv - (ku[0] * b + ku[1] * g + ku[2] * r)) >> 8;
    dst_v[0] = (add_uv - (kv[0] * b + kv[1] * g + kv[2] * r)) >> 8;
  }
}
#endif

// Blend src_argb over src_argb1 and store to dst_argb.
// dst_argb may be src_argb or src_argb1.
// src_argb: RGB values have already been pre-multiplied by the a.
#ifdef HAS_ARGBBLENDROW_RVV
void ARGBBlendRow_RVV(const uint8_t* src_argb,
                      const uint8_t* src_argb1,
                      uint8_t* dst_argb,
                      int width) {
  size_t vl;
  asm volatile(
      "vsetvli     %[vl], zero, e8, m2, ta, ma   \n"
      "vmv.v.i     v14, -1                       \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v16, (%[src_argb])            \n"
      "vlseg4e8.v  v24, (%[src_argb1])           \n"
      "vnot.v      v22, v22                      \n"
      "vwmulu.vv   v0, v24, v22                  \n"
      "vwaddu.wv   v0, v0, v24                   \n"
      "vnsrl.wi    v8, v0, 8                     \n"
      "vwmulu.vv   v0, v26, v22                  \n"
      "vwaddu.wv   v0, v0, v26                   \n"
      "vnsrl.wi    v10, v0, 8                    \n"
      "vwmulu.vv   v0, v28, v22                  \n"
      "vwaddu.wv   v0, v0, v28                   \n"
      "vnsrl.wi    v12, v0, 8                    \n"
      "vsaddu.vv   v8, v8, v16                   \n"
      "vsaddu.vv   v10, v10, v18                 \n"
      "vsaddu.vv   v12, v12, v20                 \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src_argb], %[src_argb], %[vl]\n"
      "add         %[src_argb1], %[src_argb1], %[vl]\n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),    // %[src_argb]
        [src_argb1] "+r"(src_argb1),  // %[src_argb1]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl)                // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v0", "v1", "v2", "v3", "v8", "v9", "v10",
        "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20",
        "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30",
        "v31");
}
#endif

#ifdef HAS_ARGBADDROW_RVV
void ARGBAddRow_RVV(const uint8_t* src_argb,
                    const uint8_t* src_argb1,
                    uint8_t* dst_argb,
                    int width) {
  size_t vl;
  int w = width * 4;
  asm volatile(
      "1:                                           \n"
      "vsetvli     %[vl], %[w], e8, m8, ta, ma      \n"
      "vle8.v      v8, (%[src0])                    \n"
      "vle8.v      v16, (%[src1])                   \n"
      "vsaddu.vv   v8, v8, v16                      \n"
      "vse8.v      v8, (%[dst])                     \n"
      "sub         %[w], %[w], %[vl]                \n"
      "add         %[src0], %[src0], %[vl]          \n"
      "add         %[src1], %[src1], %[vl]          \n"
      "add         %[dst], %[dst], %[vl]            \n"
      "bgtz        %[w], 1b                         \n"
      : [src0] "+r"(src_argb),   // %[src0]
        [src1] "+r"(src_argb1),  // %[src1]
        [dst] "+r"(dst_argb),    // %[dst]
        [w] "+r"(w),             // %[w]
        [vl] "=&r"(vl)           // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
}
#endif

#ifdef HAS_ARGBSUBTRACTROW_RVV
void ARGBSubtractRow_RVV(const uint8_t* src_argb,
                         const uint8_t* src_argb1,
                         uint8_t* dst_argb,
                         int width) {
  size_t vl;
  int w = width * 4;
  asm volatile(
      "1:                                           \n"
      "vsetvli     %[vl], %[w], e8, m8, ta, ma      \n"
      "vle8.v      v8, (%[src0])                    \n"
      "vle8.v      v16, (%[src1])                   \n"
      "vssubu.vv   v8, v8, v16                      \n"
      "vse8.v      v8, (%[dst])                     \n"
      "sub         %[w], %[w], %[vl]                \n"
      "add         %[src0], %[src0], %[vl]          \n"
      "add         %[src1], %[src1], %[vl]          \n"
      "add         %[dst], %[dst], %[vl]            \n"
      "bgtz        %[w], 1b                         \n"
      : [src0] "+r"(src_argb),   // %[src0]
        [src1] "+r"(src_argb1),  // %[src1]
        [dst] "+r"(dst_argb),    // %[dst]
        [w] "+r"(w),             // %[w]
        [vl] "=&r"(vl)           // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
}
#endif

#ifdef HAS_ARGBUNATTENUATEROW_RVV
void ARGBUnattenuateRow_RVV(const uint8_t* src_argb,
                            uint8_t* dst_argb,
                            int width) {
  size_t vl;
  asm volatile(
      "1:                                           \n"
      "vsetvli     %[vl], %[w], e8, m1, ta, ma      \n"
      "vlseg4e8.v  v8, (%[src_argb])                \n"
      "vwaddu.vv   v12, v11, v11                    \n"
      "vsetvli     zero, zero, e16, m2, ta, ma      \n"
      "vluxei16.v  v14, (%[tbl]), v12               \n"
      "vzext.vf2   v16, v8                          \n"
      "vzext.vf2   v18, v9                          \n"
      "vzext.vf2   v20, v10                         \n"
      "vmul.vx     v16, v16, %[k0101]               \n"
      "vmul.vx     v18, v18, %[k0101]               \n"
      "vmul.vx     v20, v20, %[k0101]               \n"
      "vmulhu.vv   v16, v16, v14                    \n"
      "vmulhu.vv   v18, v18, v14                    \n"
      "vmulhu.vv   v20, v20, v14                    \n"
      "vminu.vx    v16, v16, %[k255]                \n"
      "vminu.vx    v18, v18, %[k255]                \n"
      "vminu.vx    v20, v20, %[k255]                \n"
      "vsetvli     zero, zero, e8, m1, ta, ma       \n"
      "vnsrl.wi    v8, v16, 0                       \n"
      "vnsrl.wi    v9, v18, 0                       \n"
      "vnsrl.wi    v10, v20, 0                      \n"
      "vsseg4e8.v  v8, (%[dst_argb])                \n"
      "sub         %[w], %[w], %[vl]                \n"
      "slli        %[vl], %[vl], 2                  \n"
      "add         %[src_argb], %[src_argb], %[vl]  \n"
      "add         %[dst_argb], %[dst_argb], %[vl]  \n"
      "bgtz        %[w], 1b                         \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      : [tbl] "r"(fixed_invtbl16),  // %[tbl]
        [k0101] "r"(0x0101),        // %[k0101]
        [k255] "r"(255)             // %[k255]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21");
}
#endif

#ifdef HAS_ARGBORROW_RVV
void ARGBOrRow_RVV(const uint8_t* src_argb,
                   uint8_t* dst_argb,
                   uint32_t value,
                   int width) {
  size_t vl;
  int w = width * 4;
  asm volatile(
      "vsetvli     %[vl], zero, e32, m8, ta, ma     \n"
      "vmv.v.x     v16, %[val]                      \n"
      "1:                                           \n"
      "vsetvli     %[vl], %[w], e8, m8, ta, ma      \n"
      "vle8.v      v8, (%[src])                     \n"
      "vor.vv      v8, v8, v16                      \n"
      "vse8.v      v8, (%[dst])                     \n"
      "sub         %[w], %[w], %[vl]                \n"
      "add         %[src], %[src], %[vl]            \n"
      "add         %[dst], %[dst], %[vl]            \n"
      "bgtz        %[w], 1b                         \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(w),           // %[w]
        [vl] "=&r"(vl)         // %[vl]
      : [val] "r"(value)       // %[val]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
}
#endif

#ifdef HAS_ARGBALPHABLENDROW_RVV
void ARGBAlphaBlendRow_RVV(const uint8_t* src_argb,
                           const uint8_t* src_argb1,
                           uint8_t* dst_argb,
                           int width) {
  size_t vl;
  asm volatile(
      "1:                                           \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma      \n"
      "vlseg4e8.v  v16, (%[src_argb])               \n"
      "vlseg4e8.v  v24, (%[src_argb1])              \n"
      "vwmulu.vv   v4, v30, v22                     \n"
      "vwmulu.vx   v8, v22, %[k255]                 \n"
      "vsetvli     zero, zero, e16, m4, ta, ma      \n"
      "vadd.vi     v4, v4, 1                        \n"
      "vmulhu.vx   v4, v4, %[k0101]                 \n"
      "vsetvli     zero, zero, e8, m2, ta, ma       \n"
      "vnsrl.wi    v0, v4, 0                        \n"
      "vsub.vv     v0, v22, v0                      \n"
      "vadd.vv     v30, v30, v0                     \n"
      "vsetvli     zero, zero, e16, m4, ta, ma      \n"
      "vzext.vf2   v12, v30                         \n"
      "vdivu.vv    v8, v8, v12                      \n"
      "vsetvli     zero, zero, e8, m2, ta, ma       \n"
      "vnsrl.wi    v0, v8, 0                        \n"
      "vrsub.vx    v2, v0, %[k255]                  \n"
      "vwmulu.vv   v4, v24, v2                      \n"
      "vwmulu.vv   v8, v26, v2                      \n"
      "vwmulu.vv   v12, v28, v2                     \n"
      "vwmaccu.vv  v4, v0, v16                      \n"
      "vwmaccu.vv  v8, v0, v18                      \n"
      "vwmaccu.vv  v12, v0, v20                     \n"
      "vsetvli     zero, zero, e16, m4, ta, ma      \n"
      "vadd.vi     v4, v4, 1                        \n"
      "vadd.vi     v8, v8, 1                        \n"
      "vadd.vi     v12, v12, 1                      \n"
      "vmulhu.vx   v4, v4, %[k0101]                 \n"
      "vmulhu.vx   v8, v8, %[k0101]                 \n"
      "vmulhu.vx   v12, v12, %[k0101]               \n"
      "vsetvli     zero, zero, e8, m2, ta, ma       \n"
      "vnsrl.wi    v24, v4, 0                       \n"
      "vnsrl.wi    v26, v8, 0                       \n"
      "vnsrl.wi    v28, v12, 0                      \n"
      "vsseg4e8.v  v24, (%[dst_argb])               \n"
      "sub         %[w], %[w], %[vl]                \n"
      "slli        %[vl], %[vl], 2                  \n"
      "add         %[src_argb], %[src_argb], %[vl]  \n"
      "add         %[src_argb1], %[src_argb1], %[vl]\n"
      "add         %[dst_argb], %[dst_argb], %[vl]  \n"
      "bgtz        %[w], 1b                         \n"
      : [src_argb] "+r"(src_argb),    // %[src_argb]
        [src_argb1] "+r"(src_argb1),  // %[src_argb1]
        [dst_argb] "+r"(dst_argb),    // %[dst_argb]
        [w] "+r"(width),              // %[w]
        [vl] "=&r"(vl)                // %[vl]
      : [k0101] "r"(0x0101),          // %[k0101]
        [k255] "r"(255)               // %[k255]
      : RVV_VL_CLOBBER "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
        "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17",
        "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",
        "v28", "v29", "v30", "v31");
}
#endif

#ifdef HAS_BLENDPLANEROW_RVV
void BlendPlaneRow_RVV(const uint8_t* src0,
                       const uint8_t* src1,
                       const uint8_t* alpha,
                       uint8_t* dst,
                       int width) {
  int vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vle8.v      v16, (%[src0])                \n"
      "vle8.v      v18, (%[src1])                \n"
      "vle8.v      v20, (%[alpha])               \n"
      "vrsub.vx    v22, v20, %[k255]             \n"
      "vwmulu.vv   v8, v20, v16                  \n"
      "vwmaccu.vv  v8, v22, v18                  \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vadd.vx     v8, v8, %[k255]               \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v16, v8, 8                    \n"
      "vse8.v      v16, (%[dst])                 \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src0], %[src0], %[vl]       \n"
      "add         %[src1], %[src1], %[vl]       \n"
      "add         %[alpha], %[alpha], %[vl]     \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src0] "+r"(src0),    // %[src0]
        [src1] "+r"(src1),    // %[src1]
        [alpha] "+r"(alpha),  // %[alpha]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(width),      // %[w]
        [vl] "=&r"(vl)        // %[vl]
      : [k255] "r"(255)
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v16", "v17", "v18",
        "v19", "v20", "v21", "v22", "v23");
}
#endif

// Attenuate: (f * a + 255) >> 8
#ifdef HAS_ARGBATTENUATEROW_RVV
void ARGBAttenuateRow_RVV(const uint8_t* src_argb,
                          uint8_t* dst_argb,
                          int width) {
  int vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src_argb])             \n"
      "vwmulu.vv   v16, v8, v14                  \n"
      "vwmulu.vv   v20, v10, v14                 \n"
      "vwmulu.vv   v24, v12, v14                 \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vadd.vx     v16, v16, %[k255]             \n"
      "vadd.vx     v20, v20, %[k255]             \n"
      "vadd.vx     v24, v24, %[k255]             \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v8, v16, 8                    \n"
      "vnsrl.wi    v10, v20, 8                   \n"
      "vnsrl.wi    v12, v24, 8                   \n"
      "vsseg4e8.v  v8, (%[dst_argb])             \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src_argb], %[src_argb], %[vl]\n"
      "add         %[dst_argb], %[dst_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_argb] "+r"(dst_argb),  // %[dst_argb]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      : [k255] "r"(255)
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
        "v25", "v26", "v27");
}
#endif

#ifdef HAS_ARGBMULTIPLYROW_RVV
void ARGBMultiplyRow_RVV(const uint8_t* src_argb,
                         const uint8_t* src_argb1,
                         uint8_t* dst_argb,
                         int width) {
  assert(width != 0);
  size_t w = (size_t)width * 4;
  size_t vl;
  asm volatile(
      "1:                                         \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma    \n"
      "vle8.v      v8, (%[src0])                  \n"
      "vle8.v      v10, (%[src1])                 \n"
      "vwmulu.vv   v12, v8, v10                   \n"
      "vwaddu.wx   v12, v12, %[k128]              \n"
      "vnsrl.wi    v8, v12, 8                     \n"
      "vse8.v      v8, (%[dst])                   \n"
      "sub         %[w], %[w], %[vl]              \n"
      "add         %[src0], %[src0], %[vl]        \n"
      "add         %[src1], %[src1], %[vl]        \n"
      "add         %[dst], %[dst], %[vl]          \n"
      "bgtz        %[w], 1b                       \n"
      : [src0] "+r"(src_argb),   // %[src0]
        [src1] "+r"(src_argb1),  // %[src1]
        [dst] "+r"(dst_argb),    // %[dst]
        [w] "+r"(w),             // %[w]
        [vl] "=&r"(vl)           // %[vl]
      : [k128] "r"(128)          // %[k128]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif

#ifdef HAS_ARGBEXTRACTALPHAROW_RVV
void ARGBExtractAlphaRow_RVV(const uint8_t* src_argb,
                             uint8_t* dst_a,
                             int width) {
  int vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle32.v     v8, (%[src_argb])             \n"
      "vnsrl.wi    v16, v8, 16                   \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wi    v8, v16, 8                    \n"
      "vse8.v      v8, (%[dst_a])                \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_a], %[dst_a], %[vl]     \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src_argb], %[src_argb], %[vl]\n"
      "bgtz        %[w], 1b                      \n"
      : [src_argb] "+r"(src_argb),  // %[src_argb]
        [dst_a] "+r"(dst_a),        // %[dst_a]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      :
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19");
}
#endif

#ifdef HAS_ARGBCOPYYTOALPHAROW_RVV
void ARGBCopyYToAlphaRow_RVV(const uint8_t* src, uint8_t* dst, int width) {
  int vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e32, m8, ta, ma  \n"
      "vle8.v      v16, (%[src])                 \n"
      "vle32.v     v8, (%[dst])                  \n"
      "vzext.vf4   v24, v16                      \n"
      "vand.vx     v8, v8, %[mask]               \n"
      "vsll.vi     v24, v24, 24                  \n"
      "vor.vv      v8, v8, v24                   \n"
      "vse32.v     v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl]         \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src),         // %[src]
        [dst] "+r"(dst),         // %[dst]
        [w] "+r"(width),         // %[w]
        [vl] "=&r"(vl)           // %[vl]
      : [mask] "r"(0x00ffffffu)  // %[mask]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17", "v24", "v25", "v26", "v27", "v28", "v29", "v30",
        "v31");
}
#endif

#ifdef HAS_CONVERT16TO8ROW_RVV
void Convert16To8Row_RVV(const uint16_t* src_y,
                         uint8_t* dst_y,
                         int scale,
                         int width) {
  int vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vle16.v     v8, (%[src_y])                \n"
      "vnsrl.wx    v16, v8, %[shift]             \n"
      "vse8.v      v16, (%[dst_y])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_y], %[dst_y], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),                             // %[src_y]
        [dst_y] "+r"(dst_y),                             // %[dst_y]
        [w] "+r"(width),                                 // %[w]
        [vl] "=&r"(vl)                                   // %[vl]
      : [shift] "r"(__builtin_clz((int32_t)scale) - 15)  // %[shift]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v16", "v17");
}
#endif

#ifdef HAS_CONVERT8TO16ROW_RVV
// Use scale to convert lsb formats to msb, depending how many bits there are:
// 512 = 9 bits
// 1024 = 10 bits
// 4096 = 12 bits
// 65536 = 16 bits
void Convert8To16Row_RVV(const uint8_t* src_y,
                         uint16_t* dst_y,
                         int bits,
                         int width) {
  int vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vle8.v      v16, (%[src_y])               \n"
      "vwcvtu.x.x.v v8, v16                      \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vmul.vx     v8, v8, %[k0101]              \n"
      "vsrl.vx     v8, v8, %[shift]              \n"
      "vse16.v     v8, (%[dst_y])                \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[dst_y], %[dst_y], %[vl]     \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),  // %[src_y]
        [dst_y] "+r"(dst_y),  // %[dst_y]
        [w] "+r"(width),      // %[w]
        [vl] "=&r"(vl)        // %[vl]
      : [shift] "r"(16 - bits), [k0101] "r"(0x0101)
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v16", "v17");
}
#endif

#ifdef HAS_MULTIPLYROW_16_RVV
void MultiplyRow_16_RVV(const uint16_t* src_y,
                        uint16_t* dst_y,
                        int scale,
                        int width) {
  int vl;
  asm volatile(
      "1:          \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v8, (%[src_y])                \n"
      "vmul.vx     v8, v8, %[scale]              \n"
      "vse16.v     v8, (%[dst_y])                \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_y], %[src_y], %[vl]     \n"
      "add         %[dst_y], %[dst_y], %[vl]     \n"
      "bgtz        %[w], 1b                      \n"
      : [src_y] "+r"(src_y),  // %[src_y]
        [dst_y] "+r"(dst_y),  // %[dst_y]
        [w] "+r"(width),      // %[w]
        [vl] "=&r"(vl)        // %[vl]
      : [scale] "r"(scale)    // %[scale]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11");
}
#endif

#ifdef HAS_HALFROW_16TO8_RVV
void HalfRow_16To8_RVV(const uint16_t* src_uv,
                       ptrdiff_t src_uv_stride,
                       uint8_t* dst_uv,
                       int scale,
                       int width) {
  int vl;
  asm volatile(
      "slli        %[src_uv1], %[src_uv1], 1     \n"
      "add         %[src_uv1], %[src_uv], %[src_uv1]\n"
      "csrwi       vxrm, 0                       \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v8, (%[src_uv])               \n"
      "vle16.v     v12, (%[src_uv1])             \n"
      "vaaddu.vv   v8, v8, v12                   \n"
      "vsetvli     zero, zero, e8, m2, ta, ma    \n"
      "vnsrl.wx    v16, v8, %[shift]             \n"
      "vse8.v      v16, (%[dst_uv])              \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_uv], %[dst_uv], %[vl]   \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src_uv], %[src_uv], %[vl]   \n"
      "add         %[src_uv1], %[src_uv1], %[vl] \n"
      "bgtz        %[w], 1b                      \n"
      : [src_uv] "+r"(src_uv),                           // %[src_uv]
        [src_uv1] "+r"(src_uv_stride),                   // %[src_uv1]
        [dst_uv] "+r"(dst_uv),                           // %[dst_uv]
        [w] "+r"(width),                                 // %[w]
        [vl] "=&r"(vl)                                   // %[vl]
      : [shift] "r"(__builtin_clz((int32_t)scale) - 15)  // %[shift]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15", "v16", "v17");
}
#endif

#ifdef HAS_HALFWIDTHROW_16TO8_RVV
void HalfWidthRow_16To8_RVV(const uint16_t* src_uv,
                            ptrdiff_t src_uv_stride,
                            uint8_t* dst_uv,
                            int scale,
                            int width) {
  int vl;
  asm volatile(
      "slli        %[t], %[t], 1                 \n"
      "add         %[t], %[s], %[t]              \n"

      "1:          \n"
      "vsetvli     %[vl], %[w], e16, m2, ta, ma  \n"
      "vlseg2e16.v v16, (%[s])                   \n"
      "vlseg2e16.v v20, (%[t])                   \n"
      "vwaddu.vx   v8, v16, %[c2]                \n"
      "vwaddu.wv   v8, v8, v18                   \n"
      "vwaddu.wv   v8, v8, v20                   \n"
      "vwaddu.wv   v8, v8, v22                   \n"
      "vnsrl.wi    v16, v8, 2                    \n"
      "vsetvli     zero, zero, e8, m1, ta, ma    \n"
      "vnsrl.wx    v8, v16, %[shift]             \n"
      "vse8.v      v8, (%[dst_uv])               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst_uv], %[dst_uv], %[vl]   \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[s], %[s], %[vl]             \n"
      "add         %[t], %[t], %[vl]             \n"
      "bgtz        %[w], 1b                      \n"
      : [s] "+r"(src_uv),                                 // %[s]
        [t] "+r"(src_uv_stride),                          // %[t]
        [dst_uv] "+r"(dst_uv),                            // %[dst_uv]
        [w] "+r"(width),                                  // %[w]
        [vl] "=&r"(vl)                                    // %[vl]
      : [shift] "r"(__builtin_clz((int32_t)scale) - 15),  // %[shift]
        [c2] "r"(2)                                       // %[c2]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v16", "v17", "v18",
        "v19", "v20", "v21", "v22", "v23");
}

// Odd width: the last output pixel is a 1x2 average.
void HalfWidthRow_16To8_Odd_RVV(const uint16_t* src_uv,
                                ptrdiff_t src_uv_stride,
                                uint8_t* dst_uv,
                                int scale,
                                int width) {
  int n = width - 1;
  if (n > 0) {
    HalfWidthRow_16To8_RVV(src_uv, src_uv_stride, dst_uv, scale, n);
    src_uv += (ptrdiff_t)n * 2;
    dst_uv += n;
  }
  HalfWidthRow_16To8_Odd_C(src_uv, src_uv_stride, dst_uv, scale, 1);
}
#endif

#ifdef HAS_ARGBMIRRORROW_RVV
void ARGBMirrorRow_RVV(const uint8_t* src, uint8_t* dst, int width) {
  int vl;
  ptrdiff_t byte_vl;
  if (width <= 0) {
    return;
  }
  src += (ptrdiff_t)width * 4;
  asm volatile(
      "vsetvli     %[vl], %[w], e32, m8, ta, ma  \n"
      "slli        %[byte_vl], %[vl], 2          \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "addi        %[vl], %[vl], 1               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "bltz        %[w], 2f                      \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "1:                                        \n"
      "vle32.v     v8, (%[src])                  \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "sub         %[w], %[w], %[vl]             \n"
      "vrgather.vv v0, v8, v16                   \n"
      "vse32.v     v0, (%[dst])                  \n"
      "add         %[dst], %[dst], %[byte_vl]    \n"
      "bgez        %[w], 1b                      \n"
      "add         %[src], %[src], %[byte_vl]    \n"
      "2:                                        \n"
      "add         %[w], %[w], %[vl]             \n"
      "beqz        %[w], 3f                      \n"
      "vsetvli     %[vl], %[w], e32, m8, ta, ma  \n"
      "slli        %[byte_vl], %[vl], 2          \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "vle32.v     v8, (%[src])                  \n"
      "vrgather.vv v0, v8, v16                   \n"
      "vse32.v     v0, (%[dst])                  \n"
      "3:                                        \n"
      : [src] "+r"(src),          // %[src]
        [dst] "+r"(dst),          // %[dst]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [byte_vl] "=&r"(byte_vl)  // %[byte_vl]
      :
      : RVV_VL_CLOBBER "vtype", "memory", "v0", "v1", "v2", "v3", "v4", "v5",
        "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16",
        "v17", "v18", "v19", "v20", "v21", "v22", "v23");
}
#endif  // HAS_ARGBMIRRORROW_RVV

#ifdef HAS_ARGBSETROW_RVV
void ARGBSetRow_RVV(uint8_t* dst_argb, uint32_t v32, int width) {
  size_t vl;
  asm volatile(
      "vsetvli     %[vl], %[w], e32, m8, ta, ma  \n"
      "vmv.v.x     v8, %[v32]                    \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e32, m8, ta, ma  \n"
      "vse32.v     v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bnez        %[w], 1b                      \n"
      : [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(width),       // %[w]
        [vl] "=&r"(vl)         // %[vl]
      : [v32] "r"(v32)         // %[v32]
      : RVV_VL_CLOBBER "memory", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
        "v15");
}
#endif  // HAS_ARGBSETROW_RVV

#ifdef HAS_ARGBSHUFFLEROW_RVV
// Shuffle the 4 bytes of each ARGB pixel with a 16 byte shuffler table (only
// the first 4 entries are used, the pattern repeats every pixel), e.g.
// {0, 1, 2, 3} is a copy and {2, 1, 0, 3} swaps R and B.
// Byte index vector: index[i] = (i & ~3) + shuffler[i & 3], gathered with a
// 16 bit index so any VLEN works (e8 indices would be limited to 256 bytes).
void ARGBShuffleRow_RVV(const uint8_t* src_argb,
                        uint8_t* dst_argb,
                        const uint8_t* shuffler,
                        int width) {
  if (width <= 0) {
    return;
  }
  size_t vl;
  size_t w = (size_t)width * 4;  // bytes
  // The 4 shuffler bytes packed little endian so a 32 bit broadcast lays them
  // out as s0 s1 s2 s3 s0 s1 s2 s3 ... in memory order.
  uint32_t pattern = (uint32_t)shuffler[0] | ((uint32_t)shuffler[1] << 8) |
                     ((uint32_t)shuffler[2] << 16) |
                     ((uint32_t)shuffler[3] << 24);
  asm volatile(
      // v16..v23 (e16, m8): index = (i & ~3) + shuffler[i & 3].
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "vid.v       v16                           \n"
      "vand.vi     v16, v16, -4                  \n"
      "srli        %[vl], %[vl], 2               \n"
      "vsetvli     zero, %[vl], e32, m4, ta, ma  \n"
      "vmv.v.x     v8, %[pattern]                \n"  // s0 s1 s2 s3 ...
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "vzext.vf2   v24, v8                       \n"  // bytes -> e16
      "vadd.vv     v16, v16, v24                 \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vle8.v      v8, (%[src])                  \n"
      "vrgatherei16.vv v0, v8, v16               \n"
      "vse8.v      v0, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl]         \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bnez        %[w], 1b                      \n"
      : [src] "+r"(src_argb),   // %[src]
        [dst] "+r"(dst_argb),   // %[dst]
        [w] "+r"(w),            // %[w]
        [vl] "=&r"(vl)          // %[vl]
      : [pattern] "r"(pattern)  // %[pattern]
      : RVV_VL_CLOBBER "memory", "v0", "v1", "v2", "v3", "v8", "v9", "v10",
        "v11", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
        "v25", "v26", "v27", "v28", "v29", "v30", "v31");
}
#endif  // HAS_ARGBSHUFFLEROW_RVV

#ifdef HAS_AR64SHUFFLEROW_RVV
// Shuffle the 8 bytes of each AR64 pixel using the first 8 entries of
// shuffler, gathered with 16-bit byte indices so any VLEN works.
void AR64ShuffleRow_RVV(const uint8_t* src_ar64,
                        uint8_t* dst_ar64,
                        const uint8_t* shuffler,
                        int width) {
  if (width <= 0) {
    return;
  }
  size_t vl;
  size_t w = (size_t)width * 8;  // bytes
  asm volatile(
      "vsetivli    zero, 8, e8, m1, ta, ma       \n"
      "vle8.v      v0, (%[shuf])                 \n"
      "vsetivli    zero, 8, e16, m2, ta, ma      \n"
      "vzext.vf2   v8, v0                        \n"
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "vid.v       v16                           \n"
      "vand.vi     v24, v16, 7                   \n"
      "vand.vi     v16, v16, -8                  \n"
      "vrgather.vv v0, v8, v24                   \n"
      "vadd.vv     v16, v16, v0                  \n"

      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vle8.v      v8, (%[src])                  \n"
      "vrgatherei16.vv v0, v8, v16               \n"
      "vse8.v      v0, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl]         \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bnez        %[w], 1b                      \n"
      : [src] "+r"(src_ar64),  // %[src]
        [dst] "+r"(dst_ar64),  // %[dst]
        [w] "+r"(w),           // %[w]
        [vl] "=&r"(vl)         // %[vl]
      : [shuf] "r"(shuffler)   // %[shuf]
      : RVV_VL_CLOBBER "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
        "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19", "v20", "v21",
        "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31");
}
#endif  // HAS_AR64SHUFFLEROW_RVV

#ifdef HAS_MIRRORUVROW_RVV
void MirrorUVRow_RVV(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  int vl;
  ptrdiff_t byte_vl;
  if (width <= 0) {
    return;
  }
  src_uv += (ptrdiff_t)width * 2;
  asm volatile(
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "slli        %[byte_vl], %[vl], 1          \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "addi        %[vl], %[vl], 1               \n"
      "sub         %[w], %[w], %[vl]             \n"
      "bltz        %[w], 2f                      \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "1:                                        \n"
      "vle16.v     v8, (%[src])                  \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "sub         %[w], %[w], %[vl]             \n"
      "vrgather.vv v0, v8, v16                   \n"
      "vse16.v     v0, (%[dst])                  \n"
      "add         %[dst], %[dst], %[byte_vl]    \n"
      "bgez        %[w], 1b                      \n"
      "add         %[src], %[src], %[byte_vl]    \n"
      "2:                                        \n"
      "add         %[w], %[w], %[vl]             \n"
      "beqz        %[w], 3f                      \n"
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "slli        %[byte_vl], %[vl], 1          \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "vle16.v     v8, (%[src])                  \n"
      "vrgather.vv v0, v8, v16                   \n"
      "vse16.v     v0, (%[dst])                  \n"
      "3:                                        \n"
      : [src] "+r"(src_uv),       // %[src]
        [dst] "+r"(dst_uv),       // %[dst]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [byte_vl] "=&r"(byte_vl)  // %[byte_vl]
      :
      : RVV_VL_CLOBBER "vtype", "memory", "v0", "v1", "v2", "v3", "v4", "v5",
        "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16",
        "v17", "v18", "v19", "v20", "v21", "v22", "v23");
}
#endif  // HAS_MIRRORUVROW_RVV

#ifdef HAS_MIRRORROW_RVV
void MirrorRow_RVV(const uint8_t* src, uint8_t* dst, int width) {
  int vl;
  if (width <= 0) {
    return;
  }
  src += width;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vsetvli     zero, %[vl], e16, m8, ta, ma  \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "addi        %[vl], %[vl], 1               \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
      "sub         %[w], %[w], %[vl]             \n"
      "bltz        %[w], 2f                      \n"
      "sub         %[src], %[src], %[vl]         \n"
      "1:                                        \n"
      "vle8.v      v8, (%[src])                  \n"
      "sub         %[src], %[src], %[vl]         \n"
      "sub         %[w], %[w], %[vl]             \n"
      "vrgatherei16.vv v0, v8, v16               \n"
      "vse8.v      v0, (%[dst])                  \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgez        %[w], 1b                      \n"
      "add         %[src], %[src], %[vl]         \n"
      "2:                                        \n"
      "add         %[w], %[w], %[vl]             \n"
      "beqz        %[w], 3f                      \n"
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "addi        %[vl], %[vl], 1               \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
      "sub         %[src], %[src], %[vl]         \n"
      "vle8.v      v8, (%[src])                  \n"
      "vrgatherei16.vv v0, v8, v16               \n"
      "vse8.v      v0, (%[dst])                  \n"
      "3:                                        \n"
      : [src] "+r"(src),  // %[src]
        [dst] "+r"(dst),  // %[dst]
        [w] "+r"(width),  // %[w]
        [vl] "=&r"(vl)    // %[vl]
      :
      : RVV_VL_CLOBBER "vtype", "memory", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
}
#endif  // HAS_MIRRORROW_RVV

#ifdef HAS_MIRRORSPLITUVROW_RVV
void MirrorSplitUVRow_RVV(const uint8_t* src_uv,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width) {
  int vl;
  ptrdiff_t byte_vl;
  if (width <= 0) {
    return;
  }
  src_uv += (ptrdiff_t)width * 2;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vsetvli     zero, %[vl], e16, m8, ta, ma  \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "addi        %[vl], %[vl], 1               \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
      "slli        %[byte_vl], %[vl], 1          \n"
      "sub         %[w], %[w], %[vl]             \n"
      "bltz        %[w], 2f                      \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "1:                                        \n"
      "vlseg2e8.v  v8, (%[src])                  \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "sub         %[w], %[w], %[vl]             \n"
      "vrgatherei16.vv v0, v8, v16               \n"
      "vrgatherei16.vv v4, v12, v16              \n"
      "vse8.v      v0, (%[dst_u])                \n"
      "vse8.v      v4, (%[dst_v])                \n"
      "add         %[dst_u], %[dst_u], %[vl]     \n"
      "add         %[dst_v], %[dst_v], %[vl]     \n"
      "bgez        %[w], 1b                      \n"
      "add         %[src], %[src], %[byte_vl]    \n"
      "2:                                        \n"
      "add         %[w], %[w], %[vl]             \n"
      "beqz        %[w], 3f                      \n"
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "addi        %[vl], %[vl], 1               \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
      "slli        %[byte_vl], %[vl], 1          \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "vlseg2e8.v  v8, (%[src])                  \n"
      "vrgatherei16.vv v0, v8, v16               \n"
      "vrgatherei16.vv v4, v12, v16              \n"
      "vse8.v      v0, (%[dst_u])                \n"
      "vse8.v      v4, (%[dst_v])                \n"
      "3:                                        \n"
      : [src] "+r"(src_uv),       // %[src]
        [dst_u] "+r"(dst_u),      // %[dst_u]
        [dst_v] "+r"(dst_v),      // %[dst_v]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [byte_vl] "=&r"(byte_vl)  // %[byte_vl]
      :
      : RVV_VL_CLOBBER "vtype", "memory", "v0", "v1", "v2", "v3", "v4", "v5",
        "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16",
        "v17", "v18", "v19", "v20", "v21", "v22", "v23");
}
#endif  // HAS_MIRRORSPLITUVROW_RVV

#ifdef HAS_RGB24MIRRORROW_RVV
void RGB24MirrorRow_RVV(const uint8_t* src_rgb24,
                        uint8_t* dst_rgb24,
                        int width) {
  int vl;
  ptrdiff_t byte_vl;
  if (width <= 0) {
    return;
  }
  src_rgb24 += (ptrdiff_t)width * 3;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vsetvli     zero, %[vl], e16, m4, ta, ma  \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "addi        %[vl], %[vl], 1               \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "slli        %[byte_vl], %[vl], 1          \n"
      "add         %[byte_vl], %[byte_vl], %[vl] \n"
      "sub         %[w], %[w], %[vl]             \n"
      "bltz        %[w], 2f                      \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "1:                                        \n"
      "vlseg3e8.v  v8, (%[src])                  \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "sub         %[w], %[w], %[vl]             \n"
      "vrgatherei16.vv v0, v8, v16               \n"
      "vrgatherei16.vv v2, v10, v16              \n"
      "vrgatherei16.vv v4, v12, v16              \n"
      "vsseg3e8.v  v0, (%[dst])                  \n"
      "add         %[dst], %[dst], %[byte_vl]    \n"
      "bgez        %[w], 1b                      \n"
      "add         %[src], %[src], %[byte_vl]    \n"
      "2:                                        \n"
      "add         %[w], %[w], %[vl]             \n"
      "beqz        %[w], 3f                      \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vid.v       v16                           \n"
      "addi        %[vl], %[vl], -1              \n"
      "vrsub.vx    v16, v16, %[vl]               \n"
      "addi        %[vl], %[vl], 1               \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "slli        %[byte_vl], %[vl], 1          \n"
      "add         %[byte_vl], %[byte_vl], %[vl] \n"
      "sub         %[src], %[src], %[byte_vl]    \n"
      "vlseg3e8.v  v8, (%[src])                  \n"
      "vrgatherei16.vv v0, v8, v16               \n"
      "vrgatherei16.vv v2, v10, v16              \n"
      "vrgatherei16.vv v4, v12, v16              \n"
      "vsseg3e8.v  v0, (%[dst])                  \n"
      "3:                                        \n"
      : [src] "+r"(src_rgb24),    // %[src]
        [dst] "+r"(dst_rgb24),    // %[dst]
        [w] "+r"(width),          // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [byte_vl] "=&r"(byte_vl)  // %[byte_vl]
      :
      : RVV_VL_CLOBBER "vtype", "memory", "v0", "v1", "v2", "v3", "v4", "v5",
        "v8", "v9", "v10", "v11", "v12", "v13", "v16", "v17", "v18", "v19");
}
#endif  // HAS_RGB24MIRRORROW_RVV

#ifdef HAS_SSIM4X4ROW_RVV
#define SSIM4X4_LOAD_RVV(ptr_a, ptr_b)          \
  "vlseg4e8.v  v0, (" #ptr_a                    \
  ")             \n"                            \
  "vlseg4e8.v  v4, (" #ptr_b                    \
  ")             \n"                            \
  "vwaddu.vv   v10, v0, v1                  \n" \
  "vwaddu.vv   v11, v2, v3                  \n" \
  "vwaddu.vv   v12, v4, v5                  \n" \
  "vwaddu.vv   v13, v6, v7                  \n" \
  "vwmulu.vv   v14, v0, v4                  \n" \
  "vwmulu.vv   v15, v1, v5                  \n" \
  "vwmulu.vv   v24, v2, v6                  \n" \
  "vwmulu.vv   v25, v3, v7                  \n" \
  "vwmulu.vv   v26, v0, v0                  \n" \
  "vwmulu.vv   v27, v1, v1                  \n" \
  "vwmulu.vv   v28, v2, v2                  \n" \
  "vwmulu.vv   v29, v3, v3                  \n" \
  "vwmulu.vv   v0, v4, v4                   \n" \
  "vwmulu.vv   v1, v5, v5                   \n" \
  "vwmulu.vv   v2, v6, v6                   \n" \
  "vwmulu.vv   v3, v7, v7                   \n"

#define SSIM4X4_ACCUM_RVV(ptr_a, ptr_b) \
  "vsetvli     zero, zero, e8, mf2, ta, ma  \n" \
  SSIM4X4_LOAD_RVV(ptr_a, ptr_b)                \
  "vsetvli     zero, zero, e16, m1, ta, ma  \n" \
  "vadd.vv     v10, v10, v11                \n" \
  "vadd.vv     v12, v12, v13                \n" \
  "vadd.vv     v8, v8, v10                  \n" \
  "vadd.vv     v9, v9, v12                  \n" \
  "vwaddu.wv   v22, v22, v14                \n" \
  "vwaddu.wv   v22, v22, v15                \n" \
  "vwaddu.wv   v22, v22, v24                \n" \
  "vwaddu.wv   v22, v22, v25                \n" \
  "vwaddu.wv   v20, v20, v26                \n" \
  "vwaddu.wv   v20, v20, v27                \n" \
  "vwaddu.wv   v20, v20, v28                \n" \
  "vwaddu.wv   v20, v20, v29                \n" \
  "vwaddu.wv   v20, v20, v0                 \n" \
  "vwaddu.wv   v20, v20, v1                 \n" \
  "vwaddu.wv   v20, v20, v2                 \n" \
  "vwaddu.wv   v20, v20, v3                 \n"

void Ssim4x4Row_RVV(const uint8_t* src_a,
                    int stride_a,
                    const uint8_t* src_b,
                    int stride_b,
                    uint32_t* dst,
                    int width) {
  size_t vl;
  size_t byte_vl;
  const uint8_t* tmp_a;
  const uint8_t* tmp_b;
  width >>= 2;
  asm volatile(
      "1:                                       \n"
      "vsetvli     %[vl], %[w], e8, mf2, ta, ma \n"
      "add         %[tmp_a], %[src_a], %[stride_a]\n"
      "add         %[tmp_b], %[src_b], %[stride_b]\n"
      SSIM4X4_LOAD_RVV(%[src_a], %[src_b])
      "vsetvli     zero, zero, e16, m1, ta, ma  \n"
      "vadd.vv     v8, v10, v11                 \n"
      "vadd.vv     v9, v12, v13                 \n"
      "vwaddu.vv   v22, v14, v15                \n"
      "vwaddu.wv   v22, v22, v24                \n"
      "vwaddu.wv   v22, v22, v25                \n"
      "vwaddu.vv   v20, v26, v27                \n"
      "vwaddu.wv   v20, v20, v28                \n"
      "vwaddu.wv   v20, v20, v29                \n"
      "vwaddu.wv   v20, v20, v0                 \n"
      "vwaddu.wv   v20, v20, v1                 \n"
      "vwaddu.wv   v20, v20, v2                 \n"
      "vwaddu.wv   v20, v20, v3                 \n"
      SSIM4X4_ACCUM_RVV(%[tmp_a], %[tmp_b])
      "add         %[tmp_a], %[tmp_a], %[stride_a]\n"
      "add         %[tmp_b], %[tmp_b], %[stride_b]\n"
      SSIM4X4_ACCUM_RVV(%[tmp_a], %[tmp_b])
      "add         %[tmp_a], %[tmp_a], %[stride_a]\n"
      "add         %[tmp_b], %[tmp_b], %[stride_b]\n"
      SSIM4X4_ACCUM_RVV(%[tmp_a], %[tmp_b])
      "vwcvtu.x.x.v v16, v8                     \n"
      "vwcvtu.x.x.v v18, v9                     \n"
      "vsetvli     zero, zero, e32, m2, ta, ma  \n"
      "vsseg4e32.v v16, (%[dst])                \n"
      "sub         %[w], %[w], %[vl]            \n"
      "slli        %[byte_vl], %[vl], 2         \n"
      "add         %[src_a], %[src_a], %[byte_vl]\n"
      "add         %[src_b], %[src_b], %[byte_vl]\n"
      "slli        %[byte_vl], %[vl], 4         \n"
      "add         %[dst], %[dst], %[byte_vl]   \n"
      "bgtz        %[w], 1b                     \n"
      : [src_a] "+r"(src_a),                  // %[src_a]
        [src_b] "+r"(src_b),                  // %[src_b]
        [dst] "+r"(dst),                      // %[dst]
        [w] "+r"(width),                      // %[w]
        [vl] "=&r"(vl),                       // %[vl]
        [byte_vl] "=&r"(byte_vl),             // %[byte_vl]
        [tmp_a] "=&r"(tmp_a),                 // %[tmp_a]
        [tmp_b] "=&r"(tmp_b)                  // %[tmp_b]
      : [stride_a] "r"((ptrdiff_t)stride_a),  // %[stride_a]
        [stride_b] "r"((ptrdiff_t)stride_b)   // %[stride_b]
      : RVV_VL_CLOBBER "vtype", "memory", "v0", "v1", "v2", "v3", "v4", "v5",
        "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16",
        "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26",
        "v27", "v28", "v29");
}
#undef SSIM4X4_LOAD_RVV
#undef SSIM4X4_ACCUM_RVV
#endif  // HAS_SSIM4X4ROW_RVV

#ifdef HAS_SSIMWINDOWROW_RVV
float SsimWindowRow_RVV(const uint32_t* row0,
                        const uint32_t* row1,
                        int num_windows) {
  size_t vl;
  size_t byte_vl;
  const uint32_t* row0_next = row0 + 4;
  const uint32_t* row1_next = row1 + 4;
  float total;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "vsetvli     %[vl], zero, e32, m2, ta, ma  \n"
      "vmv.v.i     v0, 0                         \n"
      "vmv.s.x     v4, zero                      \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e32, m2, tu, ma  \n"
      "vlseg4e32.v v8, (%[row0])                 \n"
      "vlseg4e32.v v16, (%[row0_next])           \n"
      "vadd.vv     v8, v8, v16                   \n"
      "vadd.vv     v10, v10, v18                 \n"
      "vadd.vv     v12, v12, v20                 \n"
      "vadd.vv     v14, v14, v22                 \n"
      "vlseg4e32.v v16, (%[row1])                \n"
      "vlseg4e32.v v24, (%[row1_next])           \n"
      "vadd.vv     v16, v16, v24                 \n"
      "vadd.vv     v18, v18, v26                 \n"
      "vadd.vv     v20, v20, v28                 \n"
      "vadd.vv     v22, v22, v30                 \n"
      "vadd.vv     v8, v8, v16                   \n"
      "vadd.vv     v10, v10, v18                 \n"
      "vadd.vv     v12, v12, v20                 \n"
      "vadd.vv     v14, v14, v22                 \n"
      "vmul.vv     v16, v8, v10                  \n"
      "vsll.vi     v16, v16, 1                   \n"
      "vmul.vv     v18, v8, v8                   \n"
      "vmacc.vv    v18, v10, v10                 \n"
      "vsll.vi     v14, v14, 7                   \n"
      "vsub.vv     v14, v14, v16                 \n"
      "vadd.vx     v16, v16, %[c1]               \n"
      "vadd.vx     v14, v14, %[c2]               \n"
      "vsll.vi     v12, v12, 6                   \n"
      "vsub.vv     v12, v12, v18                 \n"
      "vadd.vx     v18, v18, %[c1]               \n"
      "vadd.vx     v12, v12, %[c2]               \n"
      "vfcvt.f.x.v v16, v16                      \n"
      "vfcvt.f.x.v v14, v14                      \n"
      "vfcvt.f.x.v v18, v18                      \n"
      "vfcvt.f.x.v v12, v12                      \n"
      "vfmul.vv    v16, v16, v14                 \n"
      "vfmul.vv    v18, v18, v12                 \n"
      "vfdiv.vv    v8, v16, v18                  \n"
      "vfadd.vv    v0, v0, v8                    \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[byte_vl], %[vl], 4          \n"
      "add         %[row0], %[row0], %[byte_vl]  \n"
      "add         %[row0_next], %[row0_next], %[byte_vl]\n"
      "add         %[row1], %[row1], %[byte_vl]  \n"
      "add         %[row1_next], %[row1_next], %[byte_vl]\n"
      "bgtz        %[w], 1b                      \n"
      "vsetvli     %[vl], zero, e32, m2, ta, ma  \n"
      "vfredusum.vs v4, v0, v4                   \n"
      "vfmv.f.s    %[total], v4                  \n"
      : [row0] "+r"(row0),            // %[row0]
        [row0_next] "+r"(row0_next),  // %[row0_next]
        [row1] "+r"(row1),            // %[row1]
        [row1_next] "+r"(row1_next),  // %[row1_next]
        [w] "+r"(num_windows),        // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [byte_vl] "=&r"(byte_vl),     // %[byte_vl]
        [total] "=f"(total)           // %[total]
      : [c1] "r"(26634),              // %[c1]
        [c2] "r"(239708)              // %[c2]
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v4", "v8", "v9", "v10",
        "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20",
        "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30",
        "v31");
#else
  size_t max_w;
  asm volatile(
      "vsetvli     %[max_w], zero, e32, m1, ta, ma\n"
      "vmv.v.i     v0, 0                         \n"
      "vmv.s.x     v4, zero                      \n"
      "vid.v       v24                           \n"
      "vsll.vi     v24, v24, 2                   \n"
      "vadd.vi     v25, v24, 1                   \n"
      "vadd.vi     v26, v24, 2                   \n"
      "vadd.vi     v27, v24, 3                   \n"
      "srli        %[max_w], %[max_w], 2         \n"

      "1:                                        \n"
      "mv          %[vl], %[w]                   \n"
      "bltu        %[w], %[max_w], 2f            \n"
      "mv          %[vl], %[max_w]               \n"
      "2:                                        \n"
      "slli        %[byte_vl], %[vl], 2          \n"
      "vsetvli     zero, %[byte_vl], e32, m1, ta, ma\n"
      "vle32.v     v8, (%[row0])                 \n"
      "vle32.v     v9, (%[row0_next])            \n"
      "vadd.vv     v8, v8, v9                    \n"
      "vle32.v     v9, (%[row1])                 \n"
      "vadd.vv     v8, v8, v9                    \n"
      "vle32.v     v9, (%[row1_next])            \n"
      "vadd.vv     v8, v8, v9                    \n"
      "vsetvli     zero, %[vl], e32, m1, tu, ma  \n"
      "vrgather.vv v10, v8, v24                  \n"
      "vrgather.vv v11, v8, v25                  \n"
      "vrgather.vv v12, v8, v26                  \n"
      "vrgather.vv v14, v8, v27                  \n"
      "vmul.vv     v16, v10, v11                 \n"
      "vsll.vi     v16, v16, 1                   \n"
      "vmul.vv     v18, v10, v10                 \n"
      "vmacc.vv    v18, v11, v11                 \n"
      "vsll.vi     v14, v14, 7                   \n"
      "vsub.vv     v14, v14, v16                 \n"
      "vadd.vx     v16, v16, %[c1]               \n"
      "vadd.vx     v14, v14, %[c2]               \n"
      "vsll.vi     v12, v12, 6                   \n"
      "vsub.vv     v12, v12, v18                 \n"
      "vadd.vx     v18, v18, %[c1]               \n"
      "vadd.vx     v12, v12, %[c2]               \n"
      "vfcvt.f.x.v v16, v16                      \n"
      "vfcvt.f.x.v v14, v14                      \n"
      "vfcvt.f.x.v v18, v18                      \n"
      "vfcvt.f.x.v v12, v12                      \n"
      "vfmul.vv    v16, v16, v14                 \n"
      "vfmul.vv    v18, v18, v12                 \n"
      "vfdiv.vv    v8, v16, v18                  \n"
      "vfadd.vv    v0, v0, v8                    \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[byte_vl], %[vl], 4          \n"
      "add         %[row0], %[row0], %[byte_vl]  \n"
      "add         %[row0_next], %[row0_next], %[byte_vl]\n"
      "add         %[row1], %[row1], %[byte_vl]  \n"
      "add         %[row1_next], %[row1_next], %[byte_vl]\n"
      "bgtz        %[w], 1b                      \n"
      "vsetvli     zero, %[max_w], e32, m1, ta, ma\n"
      "vfredusum.vs v4, v0, v4                   \n"
      "vfmv.f.s    %[total], v4                  \n"
      : [row0] "+r"(row0),            // %[row0]
        [row0_next] "+r"(row0_next),  // %[row0_next]
        [row1] "+r"(row1),            // %[row1]
        [row1_next] "+r"(row1_next),  // %[row1_next]
        [w] "+r"(num_windows),        // %[w]
        [vl] "=&r"(vl),               // %[vl]
        [byte_vl] "=&r"(byte_vl),     // %[byte_vl]
        [max_w] "=&r"(max_w),         // %[max_w]
        [total] "=f"(total)           // %[total]
      : [c1] "r"(26634),              // %[c1]
        [c2] "r"(239708)              // %[c2]
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v4", "v8", "v9", "v10", "v11",
        "v12", "v14", "v16", "v18", "v24", "v25", "v26", "v27");
#endif
  return total;
}
#endif  // HAS_SSIMWINDOWROW_RVV

#ifdef HAS_ARGBDETECTROW_RVV
uint32_t ARGBDetectRow_RVV(const uint8_t* argb, int width) {
  size_t vl;
  size_t byte_vl;
  long idx = -1;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e32, m8, ta, ma  \n"
      "slli        %[byte_vl], %[vl], 2          \n"
      "vsetvli     zero, %[byte_vl], e8, m8, ta, ma \n"
      "vle8.v      v8, (%[argb])                 \n"
      "vsetvli     zero, %[vl], e32, m8, ta, ma  \n"
      "vor.vx      v8, v8, %[mask_rgb]           \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[argb], %[argb], %[byte_vl]  \n"
      "vsetvli     zero, %[byte_vl], e8, m8, ta, ma \n"
      "vmsne.vi    v0, v8, -1                    \n"
      "vfirst.m    %[idx], v0                    \n"
      "bgez        %[idx], 2f                    \n"
      "bgtz        %[w], 1b                      \n"
      "2:                                        \n"
      : [argb] "+r"(argb), [w] "+r"(width), [vl] "=&r"(vl),
        [byte_vl] "=&r"(byte_vl), [idx] "+r"(idx)
      : [mask_rgb] "r"(0x00ffff00u)
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
  if (idx >= 0) {
    return (idx & 3) == 0 ? FOURCC_BGRA : FOURCC_ARGB;
  }
  return 0;
}
#endif  // HAS_ARGBDETECTROW_RVV

#ifdef HAS_SUMSQUAREERROR_RVV
uint32_t SumSquareError_RVV(const uint8_t* src_a,
                            const uint8_t* src_b,
                            int count) {
  size_t vl;
  uint32_t sse;
  asm volatile(
      "vsetvli     %[vl], zero, e32, m8, ta, ma  \n"
      "vmv.v.i     v24, 0                        \n"
      "vmv.s.x     v0, zero                      \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[count], e8, m2, tu, ma \n"
      "vle8.v      v8, (%[src_a])                \n"
      "vle8.v      v10, (%[src_b])               \n"
      "vwsubu.vv   v12, v8, v10                  \n"
      "sub         %[count], %[count], %[vl]     \n"
      "add         %[src_a], %[src_a], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "vsetvli     zero, %[vl], e16, m4, tu, ma  \n"
      "vwmacc.vv   v24, v12, v12                 \n"
      "bgtz        %[count], 1b                  \n"
      "vsetvli     %[vl], zero, e32, m8, ta, ma  \n"
      "vredsum.vs  v0, v24, v0                   \n"
      "vmv.x.s     %[sse], v0                    \n"
      : [src_a] "+r"(src_a), [src_b] "+r"(src_b), [count] "+r"(count),
        [vl] "=&r"(vl), [sse] "=r"(sse)
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v24", "v25", "v26", "v27", "v28", "v29", "v30",
        "v31");
  return sse;
}
#endif  // HAS_SUMSQUAREERROR_RVV

#ifdef HAS_HAMMINGDISTANCE_RVV
static const uint8_t kNibblePopcnt_RVV[16] = {0, 1, 1, 2, 1, 2, 2, 3,
                                              1, 2, 2, 3, 2, 3, 3, 4};

uint32_t HammingDistance_RVV(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count) {
  size_t vl;
  uint32_t diff;
  asm volatile(
      "vsetivli    zero, 16, e8, m1, ta, ma      \n"
      "vle8.v      v4, (%[lut])                  \n"
      "vsetvli     %[vl], zero, e32, m4, ta, ma  \n"
      "vmv.v.i     v24, 0                        \n"
      "vsetvli     %[vl], zero, e32, m1, ta, ma  \n"
      "vmv.s.x     v0, zero                      \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[count], e8, m1, tu, ma \n"
      "vle8.v      v8, (%[src_a])                \n"
      "vle8.v      v9, (%[src_b])                \n"
      "vxor.vv     v8, v8, v9                    \n"
      "vand.vi     v10, v8, 15                   \n"
      "vsrl.vi     v8, v8, 4                     \n"
      "vrgather.vv v11, v4, v10                  \n"
      "vrgather.vv v12, v4, v8                   \n"
      "vwaddu.vv   v16, v11, v12                 \n"
      "sub         %[count], %[count], %[vl]     \n"
      "add         %[src_a], %[src_a], %[vl]     \n"
      "add         %[src_b], %[src_b], %[vl]     \n"
      "vsetvli     zero, %[vl], e16, m2, tu, ma  \n"
      "vwaddu.wv   v24, v24, v16                 \n"
      "bgtz        %[count], 1b                  \n"
      "vsetvli     %[vl], zero, e32, m4, ta, ma  \n"
      "vredsum.vs  v0, v24, v0                   \n"
      "vsetvli     %[vl], zero, e32, m1, ta, ma  \n"
      "vmv.x.s     %[diff], v0                   \n"
      : [src_a] "+r"(src_a), [src_b] "+r"(src_b), [count] "+r"(count),
        [vl] "=&r"(vl), [diff] "=r"(diff)
      : [lut] "r"(kNibblePopcnt_RVV)
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v4", "v8", "v9", "v10", "v11",
        "v12", "v16", "v17", "v24", "v25", "v26", "v27");
  return diff;
}
#endif  // HAS_HAMMINGDISTANCE_RVV

#ifdef HAS_HASHDJB2_RVV
static const uint32_t kDjb2Muls_RVV[17] = {
    0x92d9e201,  // 33 ^ 16
    0x0c3525e1, 0xa3476dc1, 0x3b4039a1, 0x4f5f0981, 0x30f35d61, 0x855cb541,
    0x040a9121, 0x747c7101, 0xec41d4e1, 0x4cfa3cc1, 0x025528a1, 0x00121881,
    0x00008c61, 0x00000441, 0x00000021, 0x00000001,
};

uint32_t HashDjb2_RVV(const uint8_t* src, int count, uint32_t seed) {
  uint32_t hash = seed;
  uint32_t tmp;
  const uint32_t c16 = 0x92d9e201u;
  const int rem = count & 15;
  const uint32_t* tail_muls = &kDjb2Muls_RVV[17 - rem];
  const uint32_t pow_n = kDjb2Muls_RVV[16 - rem];
  asm volatile(
      "vsetivli    zero, 16, e32, m4, ta, ma     \n"
      "vmv.s.x     v4, zero                      \n"
      "addi        %[count], %[count], -16       \n"
      "bltz        %[count], 2f                  \n"
      "vle32.v     v16, (%[muls16])              \n"
      "1:                                        \n"
      "vle8.v      v8, (%[src])                  \n"
      "vzext.vf4   v12, v8                       \n"
      "vmul.vv     v12, v12, v16                 \n"
      "vredsum.vs  v0, v12, v4                   \n"
      "vmv.x.s     %[tmp], v0                    \n"
      "mul         %[hash], %[hash], %[c16]      \n"
      "add         %[hash], %[hash], %[tmp]      \n"
      "addi        %[src], %[src], 16            \n"
      "addi        %[count], %[count], -16       \n"
      "bgez        %[count], 1b                  \n"
      "2:                                        \n"
      "addi        %[count], %[count], 16        \n"
      "beqz        %[count], 3f                  \n"
      "vsetvli     zero, %[count], e32, m4, ta, ma \n"
      "vle32.v     v16, (%[tail_muls])           \n"
      "vle8.v      v8, (%[src])                  \n"
      "vzext.vf4   v12, v8                       \n"
      "vmul.vv     v12, v12, v16                 \n"
      "vredsum.vs  v0, v12, v4                   \n"
      "vmv.x.s     %[tmp], v0                    \n"
      "mul         %[hash], %[hash], %[pow_n]    \n"
      "add         %[hash], %[hash], %[tmp]      \n"
      "3:                                        \n"
      :
      [src] "+r"(src), [count] "+r"(count), [hash] "+r"(hash), [tmp] "=&r"(tmp)
      : [muls16] "r"(kDjb2Muls_RVV + 1), [c16] "r"(c16),
        [tail_muls] "r"(tail_muls), [pow_n] "r"(pow_n)
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v4", "v8", "v12", "v13", "v14",
        "v15", "v16", "v17", "v18", "v19");
  return hash;
}
#endif  // HAS_HASHDJB2_RVV

#ifdef HAS_COPYROW_16_RVV
void CopyRow_16_RVV(const uint16_t* src, uint16_t* dst, int count) {
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "vle16.v     v8, (%[src])                  \n"
      "vse16.v     v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src], %[src], %[vl]         \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src),  // %[src]
        [dst] "+r"(dst),  // %[dst]
        [w] "+r"(count),  // %[w]
        [vl] "=&r"(vl)    // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
}
#endif  // HAS_COPYROW_16_RVV

#ifdef HAS_SETROW_RVV
void SetRow_RVV(uint8_t* dst, uint8_t v8, int width) {
  size_t vl;
  asm volatile(
      "vsetvli     %[vl], %[w], e8, m8, ta, ma   \n"
      "vmv.v.x     v8, %[v8]                     \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m8, ta, ma   \n"
      "vse8.v      v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [dst] "+r"(dst),  // %[dst]
        [w] "+r"(width),  // %[w]
        [vl] "=&r"(vl)    // %[vl]
      : [v8] "r"(v8)      // %[v8]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
}
#endif  // HAS_SETROW_RVV

#ifdef HAS_ARGBCOPYALPHAROW_RVV
void ARGBCopyAlphaRow_RVV(const uint8_t* src, uint8_t* dst, int width) {
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e32, m8, ta, ma  \n"
      "vle32.v     v8, (%[src])                  \n"
      "vle32.v     v16, (%[dst])                 \n"
      "vand.vx     v8, v8, %[amask]              \n"
      "vand.vx     v16, v16, %[rgbmask]          \n"
      "vor.vv      v8, v8, v16                   \n"
      "vse32.v     v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src], %[src], %[vl]         \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src),            // %[src]
        [dst] "+r"(dst),            // %[dst]
        [w] "+r"(width),            // %[w]
        [vl] "=&r"(vl)              // %[vl]
      : [amask] "r"(0xff000000u),   // %[amask]
        [rgbmask] "r"(0x00ffffffu)  // %[rgbmask]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
}
#endif  // HAS_ARGBCOPYALPHAROW_RVV

#ifdef HAS_DIVIDEROW_16_RVV
void DivideRow_16_RVV(const uint16_t* src_y,
                      uint16_t* dst_y,
                      int scale,
                      int width) {
  if (scale == 65536) {  // Divide by 1, so do copy.
    if (src_y != dst_y) {
      CopyRow_16_RVV(src_y, dst_y, width);
    }
    return;
  }
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m8, ta, ma  \n"
      "vle16.v     v8, (%[src])                  \n"
      "vmulhu.vx   v8, v8, %[scale]              \n"
      "vse16.v     v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src], %[src], %[vl]         \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_y),  // %[src]
        [dst] "+r"(dst_y),  // %[dst]
        [w] "+r"(width),    // %[w]
        [vl] "=&r"(vl)      // %[vl]
      : [scale] "r"(scale)  // %[scale]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
}
#endif  // HAS_DIVIDEROW_16_RVV

#ifdef HAS_CONVERT8TO8ROW_RVV
void Convert8To8Row_RVV(const uint8_t* src_y,
                        uint8_t* dst_y,
                        int scale,
                        int bias,
                        int width) {
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m8, ta, ma   \n"
      "vle8.v      v8, (%[src])                  \n"
      "vmulhu.vx   v8, v8, %[scale]              \n"
      "vadd.vx     v8, v8, %[bias]               \n"
      "vse8.v      v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl]         \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_y),   // %[src]
        [dst] "+r"(dst_y),   // %[dst]
        [w] "+r"(width),     // %[w]
        [vl] "=&r"(vl)       // %[vl]
      : [scale] "r"(scale),  // %[scale]
        [bias] "r"(bias)     // %[bias]
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
}
#endif  // HAS_CONVERT8TO8ROW_RVV

#ifdef HAS_HALFMERGEUVROW_RVV
void HalfMergeUVRow_RVV(const uint8_t* src_u,
                        int src_stride_u,
                        const uint8_t* src_v,
                        int src_stride_v,
                        uint8_t* dst_uv,
                        int width) {
  const uint8_t* src_u1 = src_u + src_stride_u;
  const uint8_t* src_v1 = src_v + src_stride_v;
  int pairs = width >> 1;
  if (pairs > 0) {
    size_t vl;
    size_t vl2;
    asm volatile(
        "1:                                        \n"
        "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
        "slli        %[vl2], %[vl], 1              \n"
        "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
        "vle8.v      v8, (%[su])                   \n"
        "vle8.v      v16, (%[su1])                 \n"
        "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
        "vnsrl.wi    v0, v8, 0                     \n"
        "vnsrl.wi    v4, v8, 8                     \n"
        "vwaddu.vx   v8, v0, %[c2]                 \n"
        "vwaddu.wv   v8, v8, v4                    \n"
        "vnsrl.wi    v0, v16, 0                    \n"
        "vnsrl.wi    v4, v16, 8                    \n"
        "vwaddu.wv   v8, v8, v0                    \n"
        "vwaddu.wv   v8, v8, v4                    \n"
        "vnsrl.wi    v24, v8, 2                    \n"
        "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
        "vle8.v      v8, (%[sv])                   \n"
        "vle8.v      v16, (%[sv1])                 \n"
        "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
        "vnsrl.wi    v0, v8, 0                     \n"
        "vnsrl.wi    v4, v8, 8                     \n"
        "vwaddu.vx   v8, v0, %[c2]                 \n"
        "vwaddu.wv   v8, v8, v4                    \n"
        "vnsrl.wi    v0, v16, 0                    \n"
        "vnsrl.wi    v4, v16, 8                    \n"
        "vwaddu.wv   v8, v8, v0                    \n"
        "vwaddu.wv   v8, v8, v4                    \n"
        "vnsrl.wi    v28, v8, 2                    \n"
#if defined(LIBYUV_USE_VLSEG)
        "vsseg2e8.v  v24, (%[dst])                 \n"
#else
        "vwaddu.vv   v8, v24, v28                  \n"
        "vwmaccu.vx  v8, %[c255], v28              \n"
        "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
        "vse8.v      v8, (%[dst])                  \n"
#endif
        "sub         %[w], %[w], %[vl]             \n"
        "add         %[su], %[su], %[vl2]          \n"
        "add         %[su1], %[su1], %[vl2]        \n"
        "add         %[sv], %[sv], %[vl2]          \n"
        "add         %[sv1], %[sv1], %[vl2]        \n"
        "add         %[dst], %[dst], %[vl2]        \n"
        "bgtz        %[w], 1b                      \n"
        : [su] "+r"(src_u),    // %[su]
          [su1] "+r"(src_u1),  // %[su1]
          [sv] "+r"(src_v),    // %[sv]
          [sv1] "+r"(src_v1),  // %[sv1]
          [dst] "+r"(dst_uv),  // %[dst]
          [w] "+r"(pairs),     // %[w]
          [vl] "=&r"(vl),      // %[vl]
          [vl2] "=&r"(vl2)     // %[vl2]
        : [c2] "r"(2)          // %[c2]
#if !defined(LIBYUV_USE_VLSEG)
          ,
          [c255] "r"(255)  // %[c255]
#endif
        : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v4", "v5",
          "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
          "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25",
          "v26", "v27", "v28", "v29", "v30", "v31");
  }
  if (width & 1) {
    dst_uv[0] = (uint8_t)((src_u[0] + src_u1[0] + 1) >> 1);
    dst_uv[1] = (uint8_t)((src_v[0] + src_v1[0] + 1) >> 1);
  }
}
#endif  // HAS_HALFMERGEUVROW_RVV

#ifdef HAS_YUY2TOYROW_RVV
void YUY2ToYRow_RVV(const uint8_t* src_yuy2, uint8_t* dst_y, int width) {
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vlseg2e8.v  v8, (%[src])                  \n"
      "vse8.v      v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src], %[src], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [w] "+r"(width),       // %[w]
        [vl] "=&r"(vl)         // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
#else
  size_t vl2;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src])                  \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
      "vnsrl.wi    v0, v8, 0                     \n"
      "vse8.v      v0, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl2]        \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [w] "+r"(width),       // %[w]
        [vl] "=&r"(vl),        // %[vl]
        [vl2] "=&r"(vl2)       // %[vl2]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15");
#endif
}
#endif  // HAS_YUY2TOYROW_RVV

#ifdef HAS_UYVYTOYROW_RVV
void UYVYToYRow_RVV(const uint8_t* src_uyvy, uint8_t* dst_y, int width) {
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vlseg2e8.v  v8, (%[src])                  \n"
      "vse8.v      v12, (%[dst])                 \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src], %[src], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [w] "+r"(width),       // %[w]
        [vl] "=&r"(vl)         // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
#else
  size_t vl2;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src])                  \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
      "vnsrl.wi    v0, v8, 8                     \n"
      "vse8.v      v0, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl2]        \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [w] "+r"(width),       // %[w]
        [vl] "=&r"(vl),        // %[vl]
        [vl2] "=&r"(vl2)       // %[vl2]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15");
#endif
}
#endif  // HAS_UYVYTOYROW_RVV

#ifdef HAS_YUY2TONVUVROW_RVV
void YUY2ToNVUVRow_RVV(const uint8_t* src_yuy2,
                       int src_stride_yuy2,
                       uint8_t* dst_uv,
                       int width) {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  int uv_bytes = ((width + 1) >> 1) * 2;
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "vlseg2e8.v  v8, (%[src0])                 \n"
      "vlseg2e8.v  v16, (%[src1])                \n"
      "vaaddu.vv   v12, v12, v20                 \n"
      "vse8.v      v12, (%[dst])                 \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src0], %[src0], %[vl]       \n"
      "add         %[src1], %[src1], %[vl]       \n"
      "bgtz        %[w], 1b                      \n"
      : [src0] "+r"(src_yuy2),    // %[src0]
        [src1] "+r"(src_yuy2_1),  // %[src1]
        [dst] "+r"(dst_uv),       // %[dst]
        [w] "+r"(uv_bytes),       // %[w]
        [vl] "=&r"(vl)            // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
#else
  size_t vl2;
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m4, ta, ma   \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src0])                 \n"
      "vle8.v      v16, (%[src1])                \n"
      "vaaddu.vv   v8, v8, v16                   \n"
      "vsetvli     zero, %[vl], e8, m4, ta, ma   \n"
      "vnsrl.wi    v0, v8, 8                     \n"
      "vse8.v      v0, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src0], %[src0], %[vl2]      \n"
      "add         %[src1], %[src1], %[vl2]      \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src0] "+r"(src_yuy2),    // %[src0]
        [src1] "+r"(src_yuy2_1),  // %[src1]
        [dst] "+r"(dst_uv),       // %[dst]
        [w] "+r"(uv_bytes),       // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [vl2] "=&r"(vl2)          // %[vl2]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19",
        "v20", "v21", "v22", "v23");
#endif
}
#endif  // HAS_YUY2TONVUVROW_RVV

#ifdef HAS_YUY2TOUV422ROW_RVV
void YUY2ToUV422Row_RVV(const uint8_t* src_yuy2,
                        uint8_t* dst_u,
                        uint8_t* dst_v,
                        int width) {
  int uv_width = (width + 1) >> 1;
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src])                  \n"
      "vse8.v      v10, (%[du])                  \n"
      "vse8.v      v14, (%[dv])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[du], %[du], %[vl]           \n"
      "add         %[dv], %[dv], %[vl]           \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src], %[src], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [w] "+r"(uv_width),    // %[w]
        [vl] "=&r"(vl)         // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
#else
  size_t vl2;
  size_t vl4;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "slli        %[vl4], %[vl], 2              \n"
      "vsetvli     zero, %[vl4], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src])                  \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m4, ta, ma  \n"
      "vnsrl.wi    v0, v8, 8                     \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "vnsrl.wi    v16, v0, 0                    \n"
      "vnsrl.wi    v18, v0, 8                    \n"
      "vse8.v      v16, (%[du])                  \n"
      "vse8.v      v18, (%[dv])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl4]        \n"
      "add         %[du], %[du], %[vl]           \n"
      "add         %[dv], %[dv], %[vl]           \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_yuy2),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [w] "+r"(uv_width),    // %[w]
        [vl] "=&r"(vl),        // %[vl]
        [vl2] "=&r"(vl2),      // %[vl2]
        [vl4] "=&r"(vl4)       // %[vl4]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19");
#endif
}
#endif  // HAS_YUY2TOUV422ROW_RVV

#ifdef HAS_UYVYTOUV422ROW_RVV
void UYVYToUV422Row_RVV(const uint8_t* src_uyvy,
                        uint8_t* dst_u,
                        uint8_t* dst_v,
                        int width) {
  int uv_width = (width + 1) >> 1;
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src])                  \n"
      "vse8.v      v8, (%[du])                   \n"
      "vse8.v      v12, (%[dv])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[du], %[du], %[vl]           \n"
      "add         %[dv], %[dv], %[vl]           \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src], %[src], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [w] "+r"(uv_width),    // %[w]
        [vl] "=&r"(vl)         // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
#else
  size_t vl2;
  size_t vl4;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "slli        %[vl4], %[vl], 2              \n"
      "vsetvli     zero, %[vl4], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src])                  \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m4, ta, ma  \n"
      "vnsrl.wi    v0, v8, 0                     \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "vnsrl.wi    v16, v0, 0                    \n"
      "vnsrl.wi    v18, v0, 8                    \n"
      "vse8.v      v16, (%[du])                  \n"
      "vse8.v      v18, (%[dv])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl4]        \n"
      "add         %[du], %[du], %[vl]           \n"
      "add         %[dv], %[dv], %[vl]           \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_uyvy),  // %[src]
        [du] "+r"(dst_u),      // %[du]
        [dv] "+r"(dst_v),      // %[dv]
        [w] "+r"(uv_width),    // %[w]
        [vl] "=&r"(vl),        // %[vl]
        [vl2] "=&r"(vl2),      // %[vl2]
        [vl4] "=&r"(vl4)       // %[vl4]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19");
#endif
}
#endif  // HAS_UYVYTOUV422ROW_RVV

#ifdef HAS_YUY2TOUVROW_RVV
void YUY2ToUVRow_RVV(const uint8_t* src_yuy2,
                     int src_stride_yuy2,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  int uv_width = (width + 1) >> 1;
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src0])                 \n"
      "vlseg4e8.v  v16, (%[src1])                \n"
      "vaaddu.vv   v10, v10, v18                 \n"
      "vaaddu.vv   v14, v14, v22                 \n"
      "vse8.v      v10, (%[du])                  \n"
      "vse8.v      v14, (%[dv])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[du], %[du], %[vl]           \n"
      "add         %[dv], %[dv], %[vl]           \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src0], %[src0], %[vl]       \n"
      "add         %[src1], %[src1], %[vl]       \n"
      "bgtz        %[w], 1b                      \n"
      : [src0] "+r"(src_yuy2),    // %[src0]
        [src1] "+r"(src_yuy2_1),  // %[src1]
        [du] "+r"(dst_u),         // %[du]
        [dv] "+r"(dst_v),         // %[dv]
        [w] "+r"(uv_width),       // %[w]
        [vl] "=&r"(vl)            // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
#else
  size_t vl2;
  size_t vl4;
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "slli        %[vl4], %[vl], 2              \n"
      "vsetvli     zero, %[vl4], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src0])                 \n"
      "vle8.v      v16, (%[src1])                \n"
      "vaaddu.vv   v8, v8, v16                   \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m4, ta, ma  \n"
      "vnsrl.wi    v0, v8, 8                     \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "vnsrl.wi    v16, v0, 0                    \n"
      "vnsrl.wi    v18, v0, 8                    \n"
      "vse8.v      v16, (%[du])                  \n"
      "vse8.v      v18, (%[dv])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src0], %[src0], %[vl4]      \n"
      "add         %[src1], %[src1], %[vl4]      \n"
      "add         %[du], %[du], %[vl]           \n"
      "add         %[dv], %[dv], %[vl]           \n"
      "bgtz        %[w], 1b                      \n"
      : [src0] "+r"(src_yuy2),    // %[src0]
        [src1] "+r"(src_yuy2_1),  // %[src1]
        [du] "+r"(dst_u),         // %[du]
        [dv] "+r"(dst_v),         // %[dv]
        [w] "+r"(uv_width),       // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [vl2] "=&r"(vl2),         // %[vl2]
        [vl4] "=&r"(vl4)          // %[vl4]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19",
        "v20", "v21", "v22", "v23");
#endif
}
#endif  // HAS_YUY2TOUVROW_RVV

#ifdef HAS_UYVYTOUVROW_RVV
void UYVYToUVRow_RVV(const uint8_t* src_uyvy,
                     int src_stride_uyvy,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  const uint8_t* src_uyvy_1 = src_uyvy + src_stride_uyvy;
  int uv_width = (width + 1) >> 1;
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src0])                 \n"
      "vlseg4e8.v  v16, (%[src1])                \n"
      "vaaddu.vv   v8, v8, v16                   \n"
      "vaaddu.vv   v12, v12, v20                 \n"
      "vse8.v      v8, (%[du])                   \n"
      "vse8.v      v12, (%[dv])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[du], %[du], %[vl]           \n"
      "add         %[dv], %[dv], %[vl]           \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src0], %[src0], %[vl]       \n"
      "add         %[src1], %[src1], %[vl]       \n"
      "bgtz        %[w], 1b                      \n"
      : [src0] "+r"(src_uyvy),    // %[src0]
        [src1] "+r"(src_uyvy_1),  // %[src1]
        [du] "+r"(dst_u),         // %[du]
        [dv] "+r"(dst_v),         // %[dv]
        [w] "+r"(uv_width),       // %[w]
        [vl] "=&r"(vl)            // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22",
        "v23");
#else
  size_t vl2;
  size_t vl4;
  asm volatile(
      "csrwi       vxrm, 0                       \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "slli        %[vl4], %[vl], 2              \n"
      "vsetvli     zero, %[vl4], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src0])                 \n"
      "vle8.v      v16, (%[src1])                \n"
      "vaaddu.vv   v8, v8, v16                   \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m4, ta, ma  \n"
      "vnsrl.wi    v0, v8, 0                     \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "vnsrl.wi    v16, v0, 0                    \n"
      "vnsrl.wi    v18, v0, 8                    \n"
      "vse8.v      v16, (%[du])                  \n"
      "vse8.v      v18, (%[dv])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src0], %[src0], %[vl4]      \n"
      "add         %[src1], %[src1], %[vl4]      \n"
      "add         %[du], %[du], %[vl]           \n"
      "add         %[dv], %[dv], %[vl]           \n"
      "bgtz        %[w], 1b                      \n"
      : [src0] "+r"(src_uyvy),    // %[src0]
        [src1] "+r"(src_uyvy_1),  // %[src1]
        [du] "+r"(dst_u),         // %[du]
        [dv] "+r"(dst_v),         // %[dv]
        [w] "+r"(uv_width),       // %[w]
        [vl] "=&r"(vl),           // %[vl]
        [vl2] "=&r"(vl2),         // %[vl2]
        [vl4] "=&r"(vl4)          // %[vl4]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19",
        "v20", "v21", "v22", "v23");
#endif
}
#endif  // HAS_UYVYTOUVROW_RVV

#ifdef HAS_AYUVTOYROW_RVV
void AYUVToYRow_RVV(const uint8_t* src_ayuv, uint8_t* dst_y, int width) {
  size_t vl;
#if defined(LIBYUV_USE_VLSEG)
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "vlseg4e8.v  v8, (%[src])                  \n"
      "vse8.v      v12, (%[dst])                 \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[src], %[src], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_ayuv),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [w] "+r"(width),       // %[w]
        [vl] "=&r"(vl)         // %[vl]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v8", "v9", "v10", "v11", "v12",
        "v13", "v14", "v15");
#else
  size_t vl2;
  size_t vl4;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
      "slli        %[vl4], %[vl], 2              \n"
      "vsetvli     zero, %[vl4], e8, m8, ta, ma  \n"
      "vle8.v      v8, (%[src])                  \n"
      "slli        %[vl2], %[vl], 1              \n"
      "vsetvli     zero, %[vl2], e8, m4, ta, ma  \n"
      "vnsrl.wi    v0, v8, 0                     \n"
      "vsetvli     zero, %[vl], e8, m2, ta, ma   \n"
      "vnsrl.wi    v16, v0, 8                    \n"
      "vse8.v      v16, (%[dst])                 \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl4]        \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src_ayuv),  // %[src]
        [dst] "+r"(dst_y),     // %[dst]
        [w] "+r"(width),       // %[w]
        [vl] "=&r"(vl),        // %[vl]
        [vl2] "=&r"(vl2),      // %[vl2]
        [vl4] "=&r"(vl4)       // %[vl4]
      :
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17");
#endif
}
#endif  // HAS_AYUVTOYROW_RVV

#ifdef HAS_AYUVTOUVROW_RVV
void AYUVToUVRow_RVV(const uint8_t* src_ayuv,
                     int src_stride_ayuv,
                     uint8_t* dst_uv,
                     int width) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  int pairs = width >> 1;
  if (pairs > 0) {
    size_t vl;
    size_t vl2;
    size_t vl8;
    asm volatile(
        "1:                                        \n"
        "vsetvli     %[vl], %[w], e8, m1, ta, ma   \n"
        "slli        %[vl8], %[vl], 3              \n"
        "vsetvli     zero, %[vl8], e8, m8, ta, ma  \n"
        "vle8.v      v8, (%[s0])                   \n"
        "vle8.v      v16, (%[s1])                  \n"
        "slli        %[vl2], %[vl], 1              \n"
        "vsetvli     zero, %[vl2], e16, m4, ta, ma \n"
        "vnsrl.wi    v0, v8, 0                     \n"
        "vnsrl.wi    v4, v16, 0                    \n"
        "vsetvli     zero, %[vl], e16, m2, ta, ma  \n"
        "vnsrl.wi    v24, v0, 0                    \n"
        "vnsrl.wi    v26, v0, 16                   \n"
        "vnsrl.wi    v28, v4, 0                    \n"
        "vnsrl.wi    v30, v4, 16                   \n"
        "vsetvli     zero, %[vl2], e8, m2, ta, ma  \n"
        "vwaddu.vx   v8, v24, %[c2]                \n"
        "vwaddu.wv   v8, v8, v26                   \n"
        "vwaddu.wv   v8, v8, v28                   \n"
        "vwaddu.wv   v8, v8, v30                   \n"
        "vnsrl.wi    v0, v8, 2                     \n"
        "vsetvli     zero, %[vl], e16, m2, ta, ma  \n"
        "vsrl.vi     v2, v0, 8                     \n"
        "vsll.vi     v0, v0, 8                     \n"
        "vor.vv      v0, v0, v2                    \n"
        "vsetvli     zero, %[vl2], e8, m2, ta, ma  \n"
        "vse8.v      v0, (%[dst])                  \n"
        "sub         %[w], %[w], %[vl]             \n"
        "add         %[s0], %[s0], %[vl8]          \n"
        "add         %[s1], %[s1], %[vl8]          \n"
        "add         %[dst], %[dst], %[vl2]        \n"
        "bgtz        %[w], 1b                      \n"
        : [s0] "+r"(src_ayuv),   // %[s0]
          [s1] "+r"(src_ayuv1),  // %[s1]
          [dst] "+r"(dst_uv),    // %[dst]
          [w] "+r"(pairs),       // %[w]
          [vl] "=&r"(vl),        // %[vl]
          [vl2] "=&r"(vl2),      // %[vl2]
          [vl8] "=&r"(vl8)       // %[vl8]
        : [c2] "r"(2)            // %[c2]
        : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v4", "v5",
          "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
          "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25",
          "v26", "v27", "v28", "v29", "v30", "v31");
  }
  if (width & 1) {
    dst_uv[0] = (uint8_t)((src_ayuv[1] + src_ayuv1[1] + 1) >> 1);
    dst_uv[1] = (uint8_t)((src_ayuv[0] + src_ayuv1[0] + 1) >> 1);
  }
}
#endif  // HAS_AYUVTOUVROW_RVV

#ifdef HAS_AYUVTOVUROW_RVV
void AYUVToVURow_RVV(const uint8_t* src_ayuv,
                     int src_stride_ayuv,
                     uint8_t* dst_vu,
                     int width) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  int pairs = width >> 1;
  if (pairs > 0) {
    size_t vl;
    size_t vl2;
    size_t vl8;
    asm volatile(
        "1:                                        \n"
        "vsetvli     %[vl], %[w], e8, m1, ta, ma   \n"
        "slli        %[vl8], %[vl], 3              \n"
        "vsetvli     zero, %[vl8], e8, m8, ta, ma  \n"
        "vle8.v      v8, (%[s0])                   \n"
        "vle8.v      v16, (%[s1])                  \n"
        "slli        %[vl2], %[vl], 1              \n"
        "vsetvli     zero, %[vl2], e16, m4, ta, ma \n"
        "vnsrl.wi    v0, v8, 0                     \n"
        "vnsrl.wi    v4, v16, 0                    \n"
        "vsetvli     zero, %[vl], e16, m2, ta, ma  \n"
        "vnsrl.wi    v24, v0, 0                    \n"
        "vnsrl.wi    v26, v0, 16                   \n"
        "vnsrl.wi    v28, v4, 0                    \n"
        "vnsrl.wi    v30, v4, 16                   \n"
        "vsetvli     zero, %[vl2], e8, m2, ta, ma  \n"
        "vwaddu.vx   v8, v24, %[c2]                \n"
        "vwaddu.wv   v8, v8, v26                   \n"
        "vwaddu.wv   v8, v8, v28                   \n"
        "vwaddu.wv   v8, v8, v30                   \n"
        "vnsrl.wi    v0, v8, 2                     \n"
        "vse8.v      v0, (%[dst])                  \n"
        "sub         %[w], %[w], %[vl]             \n"
        "add         %[s0], %[s0], %[vl8]          \n"
        "add         %[s1], %[s1], %[vl8]          \n"
        "add         %[dst], %[dst], %[vl2]        \n"
        "bgtz        %[w], 1b                      \n"
        : [s0] "+r"(src_ayuv),   // %[s0]
          [s1] "+r"(src_ayuv1),  // %[s1]
          [dst] "+r"(dst_vu),    // %[dst]
          [w] "+r"(pairs),       // %[w]
          [vl] "=&r"(vl),        // %[vl]
          [vl2] "=&r"(vl2),      // %[vl2]
          [vl8] "=&r"(vl8)       // %[vl8]
        : [c2] "r"(2)            // %[c2]
        : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v4", "v5",
          "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
          "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25",
          "v26", "v27", "v28", "v29", "v30", "v31");
  }
  if (width & 1) {
    dst_vu[0] = (uint8_t)((src_ayuv[0] + src_ayuv1[0] + 1) >> 1);
    dst_vu[1] = (uint8_t)((src_ayuv[1] + src_ayuv1[1] + 1) >> 1);
  }
}
#endif  // HAS_AYUVTOVUROW_RVV

#ifdef HAS_I422TOYUY2ROW_RVV
void I422ToYUY2Row_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_frame,
                       int width) {
  int pairs = width >> 1;
  if (pairs > 0) {
    size_t vl;
    size_t vl2;
    size_t vl4;
#if defined(LIBYUV_USE_VLSEG)
    asm volatile(
        "1:                                        \n"
        "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
        "vlseg2e8.v  v0, (%[sy])                   \n"
        "vmv2r.v     v8, v0                        \n"
        "vle8.v      v10, (%[su])                  \n"
        "vmv2r.v     v12, v2                       \n"
        "vle8.v      v14, (%[sv])                  \n"
        "vsseg4e8.v  v8, (%[dst])                  \n"
        "sub         %[w], %[w], %[vl]             \n"
        "slli        %[vl2], %[vl], 1              \n"
        "slli        %[vl4], %[vl], 2              \n"
        "add         %[sy], %[sy], %[vl2]          \n"
        "add         %[su], %[su], %[vl]           \n"
        "add         %[sv], %[sv], %[vl]           \n"
        "add         %[dst], %[dst], %[vl4]        \n"
        "bgtz        %[w], 1b                      \n"
        : [sy] "+r"(src_y),       // %[sy]
          [su] "+r"(src_u),       // %[su]
          [sv] "+r"(src_v),       // %[sv]
          [dst] "+r"(dst_frame),  // %[dst]
          [w] "+r"(pairs),        // %[w]
          [vl] "=&r"(vl),         // %[vl]
          [vl2] "=&r"(vl2),       // %[vl2]
          [vl4] "=&r"(vl4)        // %[vl4]
        :
        : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
          "v10", "v11", "v12", "v13", "v14", "v15");
#else
    asm volatile(
        "1:                                        \n"
        "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
        "vle8.v      v0, (%[su])                   \n"
        "vle8.v      v2, (%[sv])                   \n"
        "vwaddu.vv   v4, v0, v2                    \n"
        "vwmaccu.vx  v4, %[c255], v2               \n"
        "slli        %[vl2], %[vl], 1              \n"
        "vsetvli     zero, %[vl2], e8, m4, ta, ma  \n"
        "vle8.v      v16, (%[sy])                  \n"
        "vwaddu.vv   v8, v16, v4                   \n"
        "vwmaccu.vx  v8, %[c255], v4               \n"
        "slli        %[vl4], %[vl], 2              \n"
        "vsetvli     zero, %[vl4], e8, m8, ta, ma  \n"
        "vse8.v      v8, (%[dst])                  \n"
        "sub         %[w], %[w], %[vl]             \n"
        "add         %[sy], %[sy], %[vl2]          \n"
        "add         %[su], %[su], %[vl]           \n"
        "add         %[sv], %[sv], %[vl]           \n"
        "add         %[dst], %[dst], %[vl4]        \n"
        "bgtz        %[w], 1b                      \n"
        : [sy] "+r"(src_y),       // %[sy]
          [su] "+r"(src_u),       // %[su]
          [sv] "+r"(src_v),       // %[sv]
          [dst] "+r"(dst_frame),  // %[dst]
          [w] "+r"(pairs),        // %[w]
          [vl] "=&r"(vl),         // %[vl]
          [vl2] "=&r"(vl2),       // %[vl2]
          [vl4] "=&r"(vl4)        // %[vl4]
        : [c255] "r"(255)         // %[c255]
        : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v4", "v5",
          "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
          "v16", "v17", "v18", "v19");
#endif
  }
  if (width & 1) {
    dst_frame[0] = src_y[0];
    dst_frame[1] = src_u[0];
    dst_frame[2] = 0;
    dst_frame[3] = src_v[0];
  }
}
#endif  // HAS_I422TOYUY2ROW_RVV

#ifdef HAS_I422TOUYVYROW_RVV
void I422ToUYVYRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_frame,
                       int width) {
  int pairs = width >> 1;
  if (pairs > 0) {
    size_t vl;
    size_t vl2;
    size_t vl4;
#if defined(LIBYUV_USE_VLSEG)
    asm volatile(
        "1:                                        \n"
        "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
        "vlseg2e8.v  v0, (%[sy])                   \n"
        "vle8.v      v8, (%[su])                   \n"
        "vmv2r.v     v10, v0                       \n"
        "vle8.v      v12, (%[sv])                  \n"
        "vmv2r.v     v14, v2                       \n"
        "vsseg4e8.v  v8, (%[dst])                  \n"
        "sub         %[w], %[w], %[vl]             \n"
        "slli        %[vl2], %[vl], 1              \n"
        "slli        %[vl4], %[vl], 2              \n"
        "add         %[sy], %[sy], %[vl2]          \n"
        "add         %[su], %[su], %[vl]           \n"
        "add         %[sv], %[sv], %[vl]           \n"
        "add         %[dst], %[dst], %[vl4]        \n"
        "bgtz        %[w], 1b                      \n"
        : [sy] "+r"(src_y),       // %[sy]
          [su] "+r"(src_u),       // %[su]
          [sv] "+r"(src_v),       // %[sv]
          [dst] "+r"(dst_frame),  // %[dst]
          [w] "+r"(pairs),        // %[w]
          [vl] "=&r"(vl),         // %[vl]
          [vl2] "=&r"(vl2),       // %[vl2]
          [vl4] "=&r"(vl4)        // %[vl4]
        :
        : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
          "v10", "v11", "v12", "v13", "v14", "v15");
#else
    asm volatile(
        "1:                                        \n"
        "vsetvli     %[vl], %[w], e8, m2, ta, ma   \n"
        "vle8.v      v0, (%[su])                   \n"
        "vle8.v      v2, (%[sv])                   \n"
        "vwaddu.vv   v4, v0, v2                    \n"
        "vwmaccu.vx  v4, %[c255], v2               \n"
        "slli        %[vl2], %[vl], 1              \n"
        "vsetvli     zero, %[vl2], e8, m4, ta, ma  \n"
        "vle8.v      v16, (%[sy])                  \n"
        "vwaddu.vv   v8, v4, v16                   \n"
        "vwmaccu.vx  v8, %[c255], v16              \n"
        "slli        %[vl4], %[vl], 2              \n"
        "vsetvli     zero, %[vl4], e8, m8, ta, ma  \n"
        "vse8.v      v8, (%[dst])                  \n"
        "sub         %[w], %[w], %[vl]             \n"
        "add         %[sy], %[sy], %[vl2]          \n"
        "add         %[su], %[su], %[vl]           \n"
        "add         %[sv], %[sv], %[vl]           \n"
        "add         %[dst], %[dst], %[vl4]        \n"
        "bgtz        %[w], 1b                      \n"
        : [sy] "+r"(src_y),       // %[sy]
          [su] "+r"(src_u),       // %[su]
          [sv] "+r"(src_v),       // %[sv]
          [dst] "+r"(dst_frame),  // %[dst]
          [w] "+r"(pairs),        // %[w]
          [vl] "=&r"(vl),         // %[vl]
          [vl2] "=&r"(vl2),       // %[vl2]
          [vl4] "=&r"(vl4)        // %[vl4]
        : [c255] "r"(255)         // %[c255]
        : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v4", "v5",
          "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
          "v16", "v17", "v18", "v19");
#endif
  }
  if (width & 1) {
    dst_frame[0] = src_u[0];
    dst_frame[1] = src_y[0];
    dst_frame[2] = src_v[0];
    dst_frame[3] = 0;
  }
}
#endif  // HAS_I422TOUYVYROW_RVV

#ifdef HAS_INTERPOLATEROW_16_RVV
void InterpolateRow_16_RVV(uint16_t* dst_ptr,
                           const uint16_t* src_ptr,
                           ptrdiff_t src_stride,
                           int width,
                           int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  assert(source_y_fraction >= 0);
  assert(source_y_fraction < 256);
  if (y1_fraction == 0) {
    CopyRow_16_RVV(src_ptr, dst_ptr, width);
    return;
  }
  if (y1_fraction == 128) {
    HalfRow_16_RVV(src_ptr, src_stride, dst_ptr, width);
    return;
  }
  size_t vl;
  asm volatile(
      "vsetvli     zero, %[w], e32, m8, ta, ma   \n"
      "vmv.v.x     v16, %[c128]                  \n"
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v0, (%[src0])                 \n"
      "vmv8r.v     v8, v16                       \n"
      "vwmaccu.vx  v8, %[y0], v0                 \n"
      "vle16.v     v0, (%[src1])                 \n"
      "vwmaccu.vx  v8, %[y1], v0                 \n"
      "vnsrl.wi    v0, v8, 8                     \n"
      "vse16.v     v0, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src0], %[src0], %[vl]       \n"
      "add         %[src1], %[src1], %[vl]       \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [dst] "+r"(dst_ptr),    // %[dst]
        [src0] "+r"(src_ptr),   // %[src0]
        [src1] "+r"(src_ptr1),  // %[src1]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl)          // %[vl]
      : [y0] "r"(y0_fraction),  // %[y0]
        [y1] "r"(y1_fraction),  // %[y1]
        [c128] "r"(128)         // %[c128]
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19",
        "v20", "v21", "v22", "v23");
}
#endif  // HAS_INTERPOLATEROW_16_RVV

#ifdef HAS_HALFFLOATROW_RVV
void HalfFloatRow_RVV(const uint16_t* src,
                      uint16_t* dst,
                      float scale,
                      int width) {
  const float kScaleBias = 1.9259299444e-34f;  // 2^-112, see HalfFloatRow_C
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e16, m4, ta, ma  \n"
      "vle16.v     v0, (%[src])                  \n"
      "vfwcvt.f.xu.v v8, v0                      \n"
      "vsetvli     zero, zero, e32, m8, ta, ma   \n"
      "vfmul.vf    v8, v8, %[scale]              \n"
      "vfmul.vf    v8, v8, %[bias]               \n"
      "vsetvli     zero, zero, e16, m4, ta, ma   \n"
      "vnsrl.wi    v0, v8, 13                    \n"
      "vse16.v     v0, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "slli        %[vl], %[vl], 1               \n"
      "add         %[src], %[src], %[vl]         \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src),        // %[src]
        [dst] "+r"(dst),        // %[dst]
        [w] "+r"(width),        // %[w]
        [vl] "=&r"(vl)          // %[vl]
      : [scale] "f"(scale),     // %[scale]
        [bias] "f"(kScaleBias)  // %[bias]
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14", "v15");
}
#endif  // HAS_HALFFLOATROW_RVV

#ifdef HAS_BYTETOFLOATROW_RVV
void ByteToFloatRow_RVV(const uint8_t* src,
                        float* dst,
                        float scale,
                        int width) {
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[w], e32, m8, ta, ma  \n"
      "vle8.v      v0, (%[src])                  \n"
      "vzext.vf4   v8, v0                        \n"
      "vfcvt.f.xu.v v8, v8                       \n"
      "vfmul.vf    v8, v8, %[scale]              \n"
      "vse32.v     v8, (%[dst])                  \n"
      "sub         %[w], %[w], %[vl]             \n"
      "add         %[src], %[src], %[vl]         \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "bgtz        %[w], 1b                      \n"
      : [src] "+r"(src),    // %[src]
        [dst] "+r"(dst),    // %[dst]
        [w] "+r"(width),    // %[w]
        [vl] "=&r"(vl)      // %[vl]
      : [scale] "f"(scale)  // %[scale]
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v8", "v9", "v10", "v11",
        "v12", "v13", "v14", "v15");
}
#endif  // HAS_BYTETOFLOATROW_RVV

#ifdef HAS_COMPUTECUMULATIVESUMROW_RVV
void ComputeCumulativeSumRow_RVV(const uint8_t* row,
                                 int32_t* cumsum,
                                 const int32_t* previous_cumsum,
                                 int width) {
  int rem = width;
  asm volatile(
      "vsetivli    zero, 16, e32, m4, ta, ma     \n"
      "vmv.v.i     v0, 0                         \n"  // running sum [S x4]
      "li          t0, 4                         \n"
      "blt         %[w], t0, 2f                  \n"

      // 4 pixel loop (16 int32 elements per step, works for VLEN >= 128).
      "1:                                        \n"
      "vle8.v      v4, (%[row])                  \n"
      "addi        %[row], %[row], 16            \n"
      "vzext.vf4   v8, v4                        \n"  // [p0, p1, p2, p3]
      "vmv.v.i     v12, 0                        \n"
      "vslideup.vi v12, v8, 4                    \n"
      "vadd.vv     v8, v8, v12                   \n"  // [p0, p01, p12, p23]
      "vmv.v.i     v12, 0                        \n"
      "vslideup.vi v12, v8, 8                    \n"
      "vadd.vv     v8, v8, v12                   \n"  // [p0, p01, p012, p0123]
      "vadd.vv     v16, v8, v0                   \n"
      "vle32.v     v12, (%[prev])                \n"
      "addi        %[prev], %[prev], 64          \n"
      "vadd.vv     v16, v16, v12                 \n"
      "vse32.v     v16, (%[cumsum])              \n"
      "addi        %[cumsum], %[cumsum], 64      \n"
      "vslidedown.vi v12, v8, 12                 \n"  // [p0123, ...]
      "vmv.v.v     v16, v12                      \n"
      "vslideup.vi v12, v16, 4                   \n"  // [p0123, p0123, ...]
      "vmv.v.v     v16, v12                      \n"
      "vslideup.vi v12, v16, 8                   \n"  // [p0123 x4]
      "vadd.vv     v0, v0, v12                   \n"
      "addi        %[w], %[w], -4                \n"
      "bge         %[w], t0, 1b                  \n"

      // 1 pixel tail loop (v0[0..3] holds current 4-channel running sum).
      "2:                                        \n"
      "blez        %[w], 3f                      \n"
      "vsetivli    zero, 4, e32, m1, ta, ma      \n"

      "20:                                       \n"
      "vle8.v      v4, (%[row])                  \n"
      "addi        %[row], %[row], 4             \n"
      "vzext.vf4   v8, v4                        \n"
      "vadd.vv     v0, v0, v8                    \n"
      "vle32.v     v12, (%[prev])                \n"
      "addi        %[prev], %[prev], 16          \n"
      "vadd.vv     v12, v0, v12                  \n"
      "vse32.v     v12, (%[cumsum])              \n"
      "addi        %[cumsum], %[cumsum], 16      \n"
      "addi        %[w], %[w], -1                \n"
      "bgtz        %[w], 20b                     \n"

      "3:                                        \n"
      : [row] "+r"(row), [cumsum] "+r"(cumsum), [prev] "+r"(previous_cumsum),
        [w] "+r"(rem)
      :
      : "memory", "t0", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v4",
        "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17",
        "v18", "v19");
}
#endif  // HAS_COMPUTECUMULATIVESUMROW_RVV

#ifdef HAS_CUMULATIVESUMTOAVERAGEROW_RVV
void CumulativeSumToAverageRow_RVV(const int32_t* topleft,
                                   const int32_t* botleft,
                                   int width,
                                   int area,
                                   uint8_t* dst,
                                   int count) {
  if (count <= 0) {
    return;
  }
  const int32_t* topright = topleft + width;
  const int32_t* botright = botleft + width;
  float ooa = 1.0f / static_cast<float>(area);
  size_t n = static_cast<size_t>(count) * 4;
  size_t vl;
  asm volatile(
      "1:                                        \n"
      "vsetvli     %[vl], %[n], e32, m4, ta, ma  \n"
      "vle32.v     v0, (%[tl])                   \n"
      "vle32.v     v4, (%[tr])                   \n"
      "vsub.vv     v0, v0, v4                    \n"
      "vle32.v     v4, (%[bl])                   \n"
      "vsub.vv     v0, v0, v4                    \n"
      "vle32.v     v4, (%[br])                   \n"
      "vadd.vv     v0, v0, v4                    \n"
      "vfcvt.f.xu.v v0, v0                       \n"
      "vfmul.vf    v0, v0, %[ooa]                \n"
      "vfcvt.rtz.xu.f.v v0, v0                   \n"
      "vsetvli     zero, %[vl], e16, m2, ta, ma  \n"
      "vnclipu.wi  v4, v0, 0                     \n"
      "vsetvli     zero, %[vl], e8, m1, ta, ma   \n"
      "vnclipu.wi  v8, v4, 0                     \n"
      "vse8.v      v8, (%[dst])                  \n"
      "sub         %[n], %[n], %[vl]             \n"
      "add         %[dst], %[dst], %[vl]         \n"
      "slli        %[vl], %[vl], 2               \n"
      "add         %[tl], %[tl], %[vl]           \n"
      "add         %[tr], %[tr], %[vl]           \n"
      "add         %[bl], %[bl], %[vl]           \n"
      "add         %[br], %[br], %[vl]           \n"
      "bgtz        %[n], 1b                      \n"
      : [tl] "+r"(topleft), [tr] "+r"(topright), [bl] "+r"(botleft),
        [br] "+r"(botright), [dst] "+r"(dst), [n] "+r"(n), [vl] "=&r"(vl)
      : [ooa] "f"(ooa)
      : "memory", RVV_VL_CLOBBER "vtype", "v0", "v1", "v2", "v3", "v4", "v5",
        "v6", "v7", "v8");
}
#endif  // HAS_CUMULATIVESUMTOAVERAGEROW_RVV

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_RVV) && defined(__riscv_vector)

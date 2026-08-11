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

#include "libyuv/row.h"

// This module is for RVV (RISC-V Vector extension)
#if !defined(LIBYUV_DISABLE_RVV) && defined(__riscv_vector)
#include <assert.h>
#include <riscv_vector.h>

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#ifdef LIBYUV_RVV_HAS_VXRM_ARG
// Fill YUV -> RGB conversion constants into vectors
#define YUVTORGB_SETUP(yuvconst, ub, vr, ug, vg, yg, bb, bg, br) \
  {                                                              \
    ub = yuvconst->kUVCoeff[0];                                  \
    vr = yuvconst->kUVCoeff[1];                                  \
    ug = yuvconst->kUVCoeff[2];                                  \
    vg = yuvconst->kUVCoeff[3];                                  \
    yg = yuvconst->kRGBCoeffBias[0];                             \
    bb = yuvconst->kRGBCoeffBias[1] + 32;                        \
    bg = yuvconst->kRGBCoeffBias[2] - 32;                        \
    br = yuvconst->kRGBCoeffBias[3] + 32;                        \
  }
#else
// Fill YUV -> RGB conversion constants into vectors
// NOTE: To match behavior on other platforms, vxrm (fixed-point rounding mode
// register) is set to round-to-nearest-up mode(0).
#define YUVTORGB_SETUP(yuvconst, ub, vr, ug, vg, yg, bb, bg, br) \
  {                                                              \
    asm volatile("csrwi vxrm, 0");                               \
    ub = yuvconst->kUVCoeff[0];                                  \
    vr = yuvconst->kUVCoeff[1];                                  \
    ug = yuvconst->kUVCoeff[2];                                  \
    vg = yuvconst->kUVCoeff[3];                                  \
    yg = yuvconst->kRGBCoeffBias[0];                             \
    bb = yuvconst->kRGBCoeffBias[1] + 32;                        \
    bg = yuvconst->kRGBCoeffBias[2] - 32;                        \
    br = yuvconst->kRGBCoeffBias[3] + 32;                        \
  }
#endif
// Read [2*VLEN/8] Y, [VLEN/8] U and [VLEN/8] V from 422
#define READYUV422(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16) \
  {                                                              \
    vuint8m1_t v_tmp0, v_tmp1;                                   \
    vuint8m2_t v_y;                                              \
    vuint16m2_t v_u_16, v_v_16;                                  \
    vl = __riscv_vsetvl_e8m1((w + 1) / 2);                       \
    v_tmp0 = __riscv_vle8_v_u8m1(src_u, vl);                     \
    v_u_16 = __riscv_vwaddu_vx_u16m2(v_tmp0, 0, vl);             \
    v_tmp1 = __riscv_vle8_v_u8m1(src_v, vl);                     \
    v_v_16 = __riscv_vwaddu_vx_u16m2(v_tmp1, 0, vl);             \
    v_v_16 = __riscv_vmul_vx_u16m2(v_v_16, 0x0101, vl);          \
    v_u_16 = __riscv_vmul_vx_u16m2(v_u_16, 0x0101, vl);          \
    v_v = __riscv_vreinterpret_v_u16m2_u8m2(v_v_16);             \
    v_u = __riscv_vreinterpret_v_u16m2_u8m2(v_u_16);             \
    vl = __riscv_vsetvl_e8m2(w);                                 \
    v_y = __riscv_vle8_v_u8m2(src_y, vl);                        \
    v_y_16 = __riscv_vwaddu_vx_u16m4(v_y, 0, vl);                \
  }

// Read [2*VLEN/8] Y, [2*VLEN/8] U, and [2*VLEN/8] V from 444
#define READYUV444(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16) \
  {                                                              \
    vuint8m2_t v_y;                                              \
    vl = __riscv_vsetvl_e8m2(w);                                 \
    v_y = __riscv_vle8_v_u8m2(src_y, vl);                        \
    v_u = __riscv_vle8_v_u8m2(src_u, vl);                        \
    v_v = __riscv_vle8_v_u8m2(src_v, vl);                        \
    v_y_16 = __riscv_vwaddu_vx_u16m4(v_y, 0, vl);                \
  }

// Convert from YUV to fixed point RGB
#define YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16, \
                 v_b_16, v_r_16)                                               \
  {                                                                            \
    vuint16m4_t v_tmp0, v_tmp1, v_tmp2, v_tmp3, v_tmp4;                        \
    vuint32m8_t v_tmp5;                                                        \
    v_tmp0 = __riscv_vwmulu_vx_u16m4(v_u, ug, vl);                             \
    v_y_16 = __riscv_vmul_vx_u16m4(v_y_16, 0x0101, vl);                        \
    v_tmp0 = __riscv_vwmaccu_vx_u16m4(v_tmp0, vg, v_v, vl);                    \
    v_tmp1 = __riscv_vwmulu_vx_u16m4(v_u, ub, vl);                             \
    v_tmp5 = __riscv_vwmulu_vx_u32m8(v_y_16, yg, vl);                          \
    v_tmp2 = __riscv_vnsrl_wx_u16m4(v_tmp5, 16, vl);                           \
    v_tmp3 = __riscv_vadd_vx_u16m4(v_tmp2, bg, vl);                            \
    v_tmp4 = __riscv_vadd_vv_u16m4(v_tmp2, v_tmp1, vl);                        \
    v_tmp2 = __riscv_vwmaccu_vx_u16m4(v_tmp2, vr, v_v, vl);                    \
    v_g_16 = __riscv_vssubu_vv_u16m4(v_tmp3, v_tmp0, vl);                      \
    v_b_16 = __riscv_vssubu_vx_u16m4(v_tmp4, bb, vl);                          \
    v_r_16 = __riscv_vssubu_vx_u16m4(v_tmp2, br, vl);                          \
  }

#ifdef LIBYUV_RVV_HAS_VXRM_ARG
// Convert from fixed point RGB To 8 bit RGB
#define RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r)        \
  {                                                                 \
    v_g = __riscv_vnclipu_wx_u8m2(v_g_16, 6, __RISCV_VXRM_RNU, vl); \
    v_b = __riscv_vnclipu_wx_u8m2(v_b_16, 6, __RISCV_VXRM_RNU, vl); \
    v_r = __riscv_vnclipu_wx_u8m2(v_r_16, 6, __RISCV_VXRM_RNU, vl); \
  }
#else
// Convert from fixed point RGB To 8 bit RGB
#define RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r) \
  {                                                          \
    v_g = __riscv_vnclipu_wx_u8m2(v_g_16, 6, vl);            \
    v_b = __riscv_vnclipu_wx_u8m2(v_b_16, 6, vl);            \
    v_r = __riscv_vnclipu_wx_u8m2(v_r_16, 6, vl);            \
  }
#endif

// Read [2*VLEN/8] Y from src_y; Read [VLEN/8] U and [VLEN/8] V from src_uv
#define READNV12(vl, w, src_y, src_uv, v_u, v_v, v_y_16) \
  {                                                      \
    vuint8m1x2_t v_tmp;                                  \
    vuint8m1_t v_tmp0, v_tmp1;                           \
    vuint8m2_t v_y;                                      \
    vuint16m2_t v_u_16, v_v_16;                          \
    vl = __riscv_vsetvl_e8m1((w + 1) / 2);               \
    v_tmp = __riscv_vlseg2e8_v_u8m1x2(src_uv, vl);       \
    v_tmp0 = __riscv_vget_v_u8m1x2_u8m1(v_tmp, 0);       \
    v_tmp1 = __riscv_vget_v_u8m1x2_u8m1(v_tmp, 1);       \
    v_u_16 = __riscv_vwaddu_vx_u16m2(v_tmp0, 0, vl);     \
    v_v_16 = __riscv_vwaddu_vx_u16m2(v_tmp1, 0, vl);     \
    v_v_16 = __riscv_vmul_vx_u16m2(v_v_16, 0x0101, vl);  \
    v_u_16 = __riscv_vmul_vx_u16m2(v_u_16, 0x0101, vl);  \
    v_v = __riscv_vreinterpret_v_u16m2_u8m2(v_v_16);     \
    v_u = __riscv_vreinterpret_v_u16m2_u8m2(v_u_16);     \
    vl = __riscv_vsetvl_e8m2(w);                         \
    v_y = __riscv_vle8_v_u8m2(src_y, vl);                \
    v_y_16 = __riscv_vwaddu_vx_u16m4(v_y, 0, vl);        \
  }

// Read 2*[VLEN/8] Y from src_y; Read [VLEN/8] U and [VLEN/8] V from src_vu
#define READNV21(vl, w, src_y, src_vu, v_u, v_v, v_y_16) \
  {                                                      \
    vuint8m1x2_t v_tmp;                                  \
    vuint8m1_t v_tmp0, v_tmp1;                           \
    vuint8m2_t v_y;                                      \
    vuint16m2_t v_u_16, v_v_16;                          \
    vl = __riscv_vsetvl_e8m1((w + 1) / 2);               \
    v_tmp = __riscv_vlseg2e8_v_u8m1x2(src_vu, vl);       \
    v_tmp0 = __riscv_vget_v_u8m1x2_u8m1(v_tmp, 0);       \
    v_tmp1 = __riscv_vget_v_u8m1x2_u8m1(v_tmp, 1);       \
    v_u_16 = __riscv_vwaddu_vx_u16m2(v_tmp1, 0, vl);     \
    v_v_16 = __riscv_vwaddu_vx_u16m2(v_tmp0, 0, vl);     \
    v_v_16 = __riscv_vmul_vx_u16m2(v_v_16, 0x0101, vl);  \
    v_u_16 = __riscv_vmul_vx_u16m2(v_u_16, 0x0101, vl);  \
    v_v = __riscv_vreinterpret_v_u16m2_u8m2(v_v_16);     \
    v_u = __riscv_vreinterpret_v_u16m2_u8m2(v_u_16);     \
    vl = __riscv_vsetvl_e8m2(w);                         \
    v_y = __riscv_vle8_v_u8m2(src_y, vl);                \
    v_y_16 = __riscv_vwaddu_vx_u16m4(v_y, 0, vl);        \
  }

#ifdef HAS_ARGBTOAR64ROW_RVV
void ARGBToAR64Row_RVV(const uint8_t* src_argb, uint16_t* dst_ar64, int width) {
  size_t avl = (size_t)4 * width;
  do {
    vuint16m8_t v_ar64;
    vuint8m4_t v_argb;
    size_t vl = __riscv_vsetvl_e8m4(avl);
    v_argb = __riscv_vle8_v_u8m4(src_argb, vl);
    v_ar64 = __riscv_vwaddu_vx_u16m8(v_argb, 0, vl);
    v_ar64 = __riscv_vmul_vx_u16m8(v_ar64, 0x0101, vl);
    __riscv_vse16_v_u16m8(dst_ar64, v_ar64, vl);
    avl -= vl;
    src_argb += vl;
    dst_ar64 += vl;
  } while (avl > 0);
}
#endif

#ifdef HAS_ARGBTOAB64ROW_RVV
void ARGBToAB64Row_RVV(const uint8_t* src_argb, uint16_t* dst_ab64, int width) {
  size_t avl = (size_t)width;
  do {
    vuint16m2x4_t v_dst_ab64;
    vuint16m2_t v_b_16, v_g_16, v_r_16, v_a_16;
    size_t vl = __riscv_vsetvl_e8m1(avl);
    vuint8m1x4_t v_src_argb = __riscv_vlseg4e8_v_u8m1x4(src_argb, vl);
    vuint8m1_t v_b = __riscv_vget_v_u8m1x4_u8m1(v_src_argb, 0);
    vuint8m1_t v_g = __riscv_vget_v_u8m1x4_u8m1(v_src_argb, 1);
    vuint8m1_t v_r = __riscv_vget_v_u8m1x4_u8m1(v_src_argb, 2);
    vuint8m1_t v_a = __riscv_vget_v_u8m1x4_u8m1(v_src_argb, 3);
    v_b_16 = __riscv_vwaddu_vx_u16m2(v_b, 0, vl);
    v_g_16 = __riscv_vwaddu_vx_u16m2(v_g, 0, vl);
    v_r_16 = __riscv_vwaddu_vx_u16m2(v_r, 0, vl);
    v_a_16 = __riscv_vwaddu_vx_u16m2(v_a, 0, vl);
    v_b_16 = __riscv_vmul_vx_u16m2(v_b_16, 0x0101, vl);
    v_g_16 = __riscv_vmul_vx_u16m2(v_g_16, 0x0101, vl);
    v_r_16 = __riscv_vmul_vx_u16m2(v_r_16, 0x0101, vl);
    v_a_16 = __riscv_vmul_vx_u16m2(v_a_16, 0x0101, vl);
    v_dst_ab64 = __riscv_vcreate_v_u16m2x4(v_r_16, v_g_16, v_b_16, v_a_16);
    __riscv_vsseg4e16_v_u16m2x4(dst_ab64, v_dst_ab64, vl);
    avl -= vl;
    src_argb += 4 * vl;
    dst_ab64 += 4 * vl;
  } while (avl > 0);
}
#endif

#ifdef HAS_AR64TOARGBROW_RVV
void AR64ToARGBRow_RVV(const uint16_t* src_ar64, uint8_t* dst_argb, int width) {
  size_t avl = (size_t)4 * width;
  do {
    vuint16m8_t v_ar64;
    vuint8m4_t v_argb;
    size_t vl = __riscv_vsetvl_e16m8(avl);
    v_ar64 = __riscv_vle16_v_u16m8(src_ar64, vl);
    v_argb = __riscv_vnsrl_wx_u8m4(v_ar64, 8, vl);
    __riscv_vse8_v_u8m4(dst_argb, v_argb, vl);
    avl -= vl;
    src_ar64 += vl;
    dst_argb += vl;
  } while (avl > 0);
}
#endif

#ifdef HAS_AR64TOAB64ROW_RVV
void AR64ToAB64Row_RVV(const uint16_t* src_ar64,
                       uint16_t* dst_ab64,
                       int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e16m2(w);
    vuint16m2x4_t v_argb16 = __riscv_vlseg4e16_v_u16m2x4(src_ar64, vl);
    vuint16m2_t v_b = __riscv_vget_v_u16m2x4_u16m2(v_argb16, 0);
    vuint16m2_t v_g = __riscv_vget_v_u16m2x4_u16m2(v_argb16, 1);
    vuint16m2_t v_r = __riscv_vget_v_u16m2x4_u16m2(v_argb16, 2);
    vuint16m2_t v_a = __riscv_vget_v_u16m2x4_u16m2(v_argb16, 3);
    vuint16m2x4_t v_dst_abgr = __riscv_vcreate_v_u16m2x4(v_r, v_g, v_b, v_a);
    __riscv_vsseg4e16_v_u16m2x4(dst_ab64, v_dst_abgr, vl);
    w -= vl;
    src_ar64 += vl * 4;
    dst_ab64 += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_AB64TOARGBROW_RVV
void AB64ToARGBRow_RVV(const uint16_t* src_ab64, uint8_t* dst_argb, int width) {
  size_t avl = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e16m2(avl);
    vuint16m2x4_t v_abgr16 = __riscv_vlseg4e16_v_u16m2x4(src_ab64, vl);
    vuint16m2_t v_r_16 = __riscv_vget_v_u16m2x4_u16m2(v_abgr16, 0);
    vuint16m2_t v_g_16 = __riscv_vget_v_u16m2x4_u16m2(v_abgr16, 1);
    vuint16m2_t v_b_16 = __riscv_vget_v_u16m2x4_u16m2(v_abgr16, 2);
    vuint16m2_t v_a_16 = __riscv_vget_v_u16m2x4_u16m2(v_abgr16, 3);
    vuint8m1_t v_b = __riscv_vnsrl_wx_u8m1(v_b_16, 8, vl);
    vuint8m1_t v_g = __riscv_vnsrl_wx_u8m1(v_g_16, 8, vl);
    vuint8m1_t v_r = __riscv_vnsrl_wx_u8m1(v_r_16, 8, vl);
    vuint8m1_t v_a = __riscv_vnsrl_wx_u8m1(v_a_16, 8, vl);
    vuint8m1x4_t v_dst_argb = __riscv_vcreate_v_u8m1x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m1x4(dst_argb, v_dst_argb, vl);
    avl -= vl;
    src_ab64 += 4 * vl;
    dst_argb += 4 * vl;
  } while (avl > 0);
}
#endif

#ifdef HAS_RAWTOARGBROW_RVV
void RAWToARGBRow_RVV(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    vuint8m2x3_t v_bgr = __riscv_vlseg3e8_v_u8m2x3(src_raw, vl);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 1);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 2);
    vuint8m2x4_t v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_raw += vl * 3;
    dst_argb += vl * 4;
    vl = __riscv_vsetvl_e8m2(w);
  } while (w > 0);
}
#endif

#ifdef HAS_RAWTORGBAROW_RVV
void RAWToRGBARow_RVV(const uint8_t* src_raw, uint8_t* dst_rgba, int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    vuint8m2x3_t v_bgr = __riscv_vlseg3e8_v_u8m2x3(src_raw, vl);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 1);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 2);
    vuint8m2x4_t v_dst_rgba = __riscv_vcreate_v_u8m2x4(v_a, v_b, v_g, v_r);
    __riscv_vsseg4e8_v_u8m2x4(dst_rgba, v_dst_rgba, vl);
    w -= vl;
    src_raw += vl * 3;
    dst_rgba += vl * 4;
    vl = __riscv_vsetvl_e8m2(w);
  } while (w > 0);
}
#endif

#ifdef HAS_RAWTORGB24ROW_RVV
void RAWToRGB24Row_RVV(const uint8_t* src_raw, uint8_t* dst_rgb24, int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x3_t v_bgr = __riscv_vlseg3e8_v_u8m2x3(src_raw, vl);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 1);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x3_u8m2(v_bgr, 2);
    vuint8m2x3_t v_dst_rgb = __riscv_vcreate_v_u8m2x3(v_b, v_g, v_r);
    __riscv_vsseg3e8_v_u8m2x3(dst_rgb24, v_dst_rgb, vl);
    w -= vl;
    src_raw += vl * 3;
    dst_rgb24 += vl * 3;
  } while (w > 0);
}
#endif

#ifdef HAS_ARGBTORAWROW_RVV
void ARGBToRAWRow_RVV(const uint8_t* src_argb, uint8_t* dst_raw, int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 2);
    vuint8m2x3_t v_dst_bgr = __riscv_vcreate_v_u8m2x3(v_r, v_g, v_b);
    __riscv_vsseg3e8_v_u8m2x3(dst_raw, v_dst_bgr, vl);
    w -= vl;
    src_argb += vl * 4;
    dst_raw += vl * 3;
  } while (w > 0);
}
#endif

#ifdef HAS_ARGBTORGB24ROW_RVV
void ARGBToRGB24Row_RVV(const uint8_t* src_argb,
                        uint8_t* dst_rgb24,
                        int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 2);
    vuint8m2x3_t v_dst_rgb = __riscv_vcreate_v_u8m2x3(v_b, v_g, v_r);
    __riscv_vsseg3e8_v_u8m2x3(dst_rgb24, v_dst_rgb, vl);
    w -= vl;
    src_argb += vl * 4;
    dst_rgb24 += vl * 3;
  } while (w > 0);
}
#endif

#ifdef HAS_ARGBTOABGRROW_RVV
void ARGBToABGRRow_RVV(const uint8_t* src_argb, uint8_t* dst_abgr, int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 2);
    vuint8m2_t v_a = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 3);
    vuint8m2x4_t v_dst_abgr = __riscv_vcreate_v_u8m2x4(v_r, v_g, v_b, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_abgr, v_dst_abgr, vl);
    w -= vl;
    src_argb += vl * 4;
    dst_abgr += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_ARGBTOBGRAROW_RVV
void ARGBToBGRARow_RVV(const uint8_t* src_argb, uint8_t* dst_bgra, int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 2);
    vuint8m2_t v_a = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 3);
    vuint8m2x4_t v_dst_bgra = __riscv_vcreate_v_u8m2x4(v_a, v_r, v_g, v_b);
    __riscv_vsseg4e8_v_u8m2x4(dst_bgra, v_dst_bgra, vl);
    w -= vl;
    src_argb += vl * 4;
    dst_bgra += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_ARGBTORGBAROW_RVV
void ARGBToRGBARow_RVV(const uint8_t* src_argb, uint8_t* dst_rgba, int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 2);
    vuint8m2_t v_a = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 3);
    vuint8m2x4_t v_dst_rgba = __riscv_vcreate_v_u8m2x4(v_a, v_b, v_g, v_r);
    __riscv_vsseg4e8_v_u8m2x4(dst_rgba, v_dst_rgba, vl);
    w -= vl;
    src_argb += vl * 4;
    dst_rgba += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_RGBATOARGBROW_RVV
void RGBAToARGBRow_RVV(const uint8_t* src_rgba, uint8_t* dst_argb, int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_rgba = __riscv_vlseg4e8_v_u8m2x4(src_rgba, vl);
    vuint8m2_t v_a = __riscv_vget_v_u8m2x4_u8m2(v_src_rgba, 0);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src_rgba, 1);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src_rgba, 2);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src_rgba, 3);
    vuint8m2x4_t v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_rgba += vl * 4;
    dst_argb += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_RGB24TOARGBROW_RVV
void RGB24ToARGBRow_RVV(const uint8_t* src_rgb24,
                        uint8_t* dst_argb,
                        int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    vuint8m2x3_t v_src_rgb = __riscv_vlseg3e8_v_u8m2x3(src_rgb24, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 2);
    vuint8m2x4_t v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_rgb24 += vl * 3;
    dst_argb += vl * 4;
    vl = __riscv_vsetvl_e8m2(w);
  } while (w > 0);
}
#endif

#ifdef HAS_I444TOARGBROW_RVV
void I444ToARGBRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r, v_a;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  vuint8m2x4_t v_dst_argb;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    READYUV444(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_y += vl;
    src_u += vl;
    src_v += vl;
    dst_argb += vl * 4;
  } while (w > 0);
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
  size_t w = (size_t)width;
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r, v_a;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  do {
    vuint8m2x4_t v_dst_argb;
    READYUV444(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16);
    v_a = __riscv_vle8_v_u8m2(src_a, vl);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_y += vl;
    src_a += vl;
    src_u += vl;
    src_v += vl;
    dst_argb += vl * 4;
  } while (w > 0);
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
  size_t w = (size_t)width;
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  do {
    vuint8m2x3_t v_dst_rgb;
    READYUV444(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_rgb = __riscv_vcreate_v_u8m2x3(v_b, v_g, v_r);
    __riscv_vsseg3e8_v_u8m2x3(dst_rgb24, v_dst_rgb, vl);
    w -= vl;
    src_y += vl;
    src_u += vl;
    src_v += vl;
    dst_rgb24 += vl * 3;
  } while (w > 0);
}
#endif

#ifdef HAS_I422TOARGBROW_RVV
void I422ToARGBRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r, v_a;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  vuint8m2x4_t v_dst_argb;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    READYUV422(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_y += vl;
    src_u += vl / 2;
    src_v += vl / 2;
    dst_argb += vl * 4;
  } while (w > 0);
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
  size_t w = (size_t)width;
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r, v_a;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  do {
    vuint8m2x4_t v_dst_argb;
    READYUV422(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16);
    v_a = __riscv_vle8_v_u8m2(src_a, vl);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_y += vl;
    src_a += vl;
    src_u += vl / 2;
    src_v += vl / 2;
    dst_argb += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_I422TORGBAROW_RVV
void I422ToRGBARow_RVV(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_rgba,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r, v_a;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  vuint8m2x4_t v_dst_rgba;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    READYUV422(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_rgba = __riscv_vcreate_v_u8m2x4(v_a, v_b, v_g, v_r);
    __riscv_vsseg4e8_v_u8m2x4(dst_rgba, v_dst_rgba, vl);
    w -= vl;
    src_y += vl;
    src_u += vl / 2;
    src_v += vl / 2;
    dst_rgba += vl * 4;
  } while (w > 0);
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
  size_t w = (size_t)width;
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  vuint8m2x3_t v_dst_rgb;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  do {
    READYUV422(vl, w, src_y, src_u, src_v, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_rgb = __riscv_vcreate_v_u8m2x3(v_b, v_g, v_r);
    __riscv_vsseg3e8_v_u8m2x3(dst_rgb24, v_dst_rgb, vl);
    w -= vl;
    src_y += vl;
    src_u += vl / 2;
    src_v += vl / 2;
    dst_rgb24 += vl * 3;
  } while (w > 0);
}
#endif

#ifdef HAS_I400TOARGBROW_RVV
#if defined(LIBYUV_RVV_HAS_VXRM_ARG)
void I400ToARGBRow_RVV(const uint8_t* src_y,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  const bool is_yb_positive = (yuvconstants->kRGBCoeffBias[4] >= 0);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  vuint16m4_t v_yg = __riscv_vmv_v_x_u16m4(yuvconstants->kRGBCoeffBias[0], vl);
  vuint8m2x4_t v_dst_argb;
  vuint16m4_t v_yb;
  if (is_yb_positive) {
    v_yb = __riscv_vmv_v_x_u16m4(yuvconstants->kRGBCoeffBias[4] - 32, vl);
  } else {
    v_yb = __riscv_vmv_v_x_u16m4(-yuvconstants->kRGBCoeffBias[4] + 32, vl);
  }
  do {
    vuint8m2_t v_y, v_out;
    vuint16m4_t v_y_16, v_tmp0, v_tmp1, v_tmp2;
    vl = __riscv_vsetvl_e8m2(w);
    v_y = __riscv_vle8_v_u8m2(src_y, vl);
    v_y_16 = __riscv_vwaddu_vx_u16m4(v_y, 0, vl);
    v_tmp0 = __riscv_vmul_vx_u16m4(v_y_16, 0x0101, vl);  // 257 * v_y
    v_tmp1 = __riscv_vmulhu_vv_u16m4(v_tmp0, v_yg, vl);
    if (is_yb_positive) {
      v_tmp2 = __riscv_vsaddu_vv_u16m4(v_tmp1, v_yb, vl);
    } else {
      v_tmp2 = __riscv_vssubu_vv_u16m4(v_tmp1, v_yb, vl);
    }
    v_out = __riscv_vnclipu_wx_u8m2(v_tmp2, 6, __RISCV_VXRM_RNU, vl);
    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_out, v_out, v_out, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_y += vl;
    dst_argb += vl * 4;
  } while (w > 0);
}
#else
void I400ToARGBRow_RVV(const uint8_t* src_y,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  const bool is_yb_positive = (yuvconstants->kRGBCoeffBias[4] >= 0);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  vuint16m4_t v_yb;
  vuint16m4_t v_yg = __riscv_vmv_v_x_u16m4(yuvconstants->kRGBCoeffBias[0], vl);
  // To match behavior on other platforms, vxrm (fixed-point rounding mode
  // register) sets to round-to-nearest-up mode(0).
  asm volatile("csrwi vxrm, 0");
  if (is_yb_positive) {
    v_yb = __riscv_vmv_v_x_u16m4(yuvconstants->kRGBCoeffBias[4] - 32, vl);
  } else {
    v_yb = __riscv_vmv_v_x_u16m4(-yuvconstants->kRGBCoeffBias[4] + 32, vl);
  }
  do {
    vuint8m2_t v_y, v_out;
    vuint16m4_t v_y_16, v_tmp0, v_tmp1, v_tmp2;
    vl = __riscv_vsetvl_e8m2(w);
    v_y = __riscv_vle8_v_u8m2(src_y, vl);
    v_y_16 = __riscv_vwaddu_vx_u16m4(v_y, 0, vl);
    v_tmp0 = __riscv_vmul_vx_u16m4(v_y_16, 0x0101, vl);  // 257 * v_y
    v_tmp1 = __riscv_vmulhu_vv_u16m4(v_tmp0, v_yg, vl);
    if (is_yb_positive) {
      v_tmp2 = __riscv_vsaddu_vv_u16m4(v_tmp1, v_yb, vl);
    } else {
      v_tmp2 = __riscv_vssubu_vv_u16m4(v_tmp1, v_yb, vl);
    }
    v_out = __riscv_vnclipu_wx_u8m2(v_tmp2, 6, vl);
    __riscv_vsseg4e8_v_u8m2(dst_argb, v_out, v_out, v_out, v_a, vl);
    w -= vl;
    src_y += vl;
    dst_argb += vl * 4;
  } while (w > 0);
}
#endif
#endif

#ifdef HAS_J400TOARGBROW_RVV
void J400ToARGBRow_RVV(const uint8_t* src_y, uint8_t* dst_argb, int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    vuint8m2_t v_y = __riscv_vle8_v_u8m2(src_y, vl);
    vuint8m2x4_t v_dst_argb = __riscv_vcreate_v_u8m2x4(v_y, v_y, v_y, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_y += vl;
    dst_argb += vl * 4;
    vl = __riscv_vsetvl_e8m2(w);
  } while (w > 0);
}
#endif

#ifdef HAS_COPYROW_RVV
void CopyRow_RVV(const uint8_t* src, uint8_t* dst, int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m8(w);
    vuint8m8_t v_data = __riscv_vle8_v_u8m8(src, vl);
    __riscv_vse8_v_u8m8(dst, v_data, vl);
    w -= vl;
    src += vl;
    dst += vl;
  } while (w > 0);
}
#endif

#ifdef HAS_NV12TOARGBROW_RVV
void NV12ToARGBRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_uv,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r, v_a;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  vuint8m2x4_t v_dst_argb;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    READNV12(vl, w, src_y, src_uv, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_y += vl;
    src_uv += vl;
    dst_argb += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_NV12TORGB24ROW_RVV
void NV12ToRGB24Row_RVV(const uint8_t* src_y,
                        const uint8_t* src_uv,
                        uint8_t* dst_rgb24,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r;
  vuint8m2x3_t v_dst_rgb;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  do {
    READNV12(vl, w, src_y, src_uv, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_rgb = __riscv_vcreate_v_u8m2x3(v_b, v_g, v_r);
    __riscv_vsseg3e8_v_u8m2x3(dst_rgb24, v_dst_rgb, vl);
    w -= vl;
    src_y += vl;
    src_uv += vl;
    dst_rgb24 += vl * 3;
  } while (w > 0);
}
#endif

#ifdef HAS_NV21TOARGBROW_RVV
void NV21ToARGBRow_RVV(const uint8_t* src_y,
                       const uint8_t* src_vu,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r, v_a;
  vuint8m2x4_t v_dst_argb;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    READNV21(vl, w, src_y, src_vu, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_y += vl;
    src_vu += vl;
    dst_argb += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_NV21TORGB24ROW_RVV
void NV21ToRGB24Row_RVV(const uint8_t* src_y,
                        const uint8_t* src_vu,
                        uint8_t* dst_rgb24,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  uint8_t ub, vr, ug, vg;
  int16_t yg, bb, bg, br;
  vuint8m2_t v_u, v_v;
  vuint8m2_t v_b, v_g, v_r;
  vuint8m2x3_t v_dst_rgb;
  vuint16m4_t v_y_16, v_g_16, v_b_16, v_r_16;
  YUVTORGB_SETUP(yuvconstants, ub, vr, ug, vg, yg, bb, bg, br);
  do {
    READNV21(vl, w, src_y, src_vu, v_u, v_v, v_y_16);
    YUVTORGB(vl, v_u, v_v, ub, vr, ug, vg, yg, bb, bg, br, v_y_16, v_g_16,
             v_b_16, v_r_16);
    RGBTORGB8(vl, v_g_16, v_b_16, v_r_16, v_g, v_b, v_r);
    v_dst_rgb = __riscv_vcreate_v_u8m2x3(v_b, v_g, v_r);
    __riscv_vsseg3e8_v_u8m2x3(dst_rgb24, v_dst_rgb, vl);
    w -= vl;
    src_y += vl;
    src_vu += vl;
    dst_rgb24 += vl * 3;
  } while (w > 0);
}
#endif

// Bilinear filter [VLEN/8]x2 -> [VLEN/8]x1
#ifdef HAS_INTERPOLATEROW_RVV
#ifdef LIBYUV_RVV_HAS_VXRM_ARG
void InterpolateRow_RVV(uint8_t* dst_ptr,
                        const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        int dst_width,
                        int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  size_t dst_w = (size_t)dst_width;
  assert(source_y_fraction >= 0);
  assert(source_y_fraction < 256);
  // Blend 100 / 0 - Copy row unchanged.
  if (y1_fraction == 0) {
    do {
      size_t vl = __riscv_vsetvl_e8m8(dst_w);
      __riscv_vse8_v_u8m8(dst_ptr, __riscv_vle8_v_u8m8(src_ptr, vl), vl);
      dst_w -= vl;
      src_ptr += vl;
      dst_ptr += vl;
    } while (dst_w > 0);
    return;
  }
  // Blend 50 / 50.
  if (y1_fraction == 128) {
    do {
      size_t vl = __riscv_vsetvl_e8m8(dst_w);
      vuint8m8_t row0 = __riscv_vle8_v_u8m8(src_ptr, vl);
      vuint8m8_t row1 = __riscv_vle8_v_u8m8(src_ptr1, vl);
      vuint8m8_t row_out =
          __riscv_vaaddu_vv_u8m8(row0, row1, __RISCV_VXRM_RNU, vl);
      __riscv_vse8_v_u8m8(dst_ptr, row_out, vl);
      dst_w -= vl;
      src_ptr += vl;
      src_ptr1 += vl;
      dst_ptr += vl;
    } while (dst_w > 0);
    return;
  }
  // General purpose row blend.
  do {
    size_t vl = __riscv_vsetvl_e8m4(dst_w);
    vuint8m4_t row0 = __riscv_vle8_v_u8m4(src_ptr, vl);
    vuint16m8_t acc = __riscv_vwmulu_vx_u16m8(row0, y0_fraction, vl);
    vuint8m4_t row1 = __riscv_vle8_v_u8m4(src_ptr1, vl);
    acc = __riscv_vwmaccu_vx_u16m8(acc, y1_fraction, row1, vl);
    __riscv_vse8_v_u8m4(
        dst_ptr, __riscv_vnclipu_wx_u8m4(acc, 8, __RISCV_VXRM_RNU, vl), vl);
    dst_w -= vl;
    src_ptr += vl;
    src_ptr1 += vl;
    dst_ptr += vl;
  } while (dst_w > 0);
}
#else
void InterpolateRow_RVV(uint8_t* dst_ptr,
                        const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        int dst_width,
                        int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  size_t dst_w = (size_t)dst_width;
  assert(source_y_fraction >= 0);
  assert(source_y_fraction < 256);
  // Blend 100 / 0 - Copy row unchanged.
  if (y1_fraction == 0) {
    do {
      size_t vl = __riscv_vsetvl_e8m8(dst_w);
      __riscv_vse8_v_u8m8(dst_ptr, __riscv_vle8_v_u8m8(src_ptr, vl), vl);
      dst_w -= vl;
      src_ptr += vl;
      dst_ptr += vl;
    } while (dst_w > 0);
    return;
  }
  // To match behavior on other platforms, vxrm (fixed-point rounding mode
  // register) is set to round-to-nearest-up(0).
  asm volatile("csrwi vxrm, 0");
  // Blend 50 / 50.
  if (y1_fraction == 128) {
    do {
      size_t vl = __riscv_vsetvl_e8m8(dst_w);
      vuint8m8_t row0 = __riscv_vle8_v_u8m8(src_ptr, vl);
      vuint8m8_t row1 = __riscv_vle8_v_u8m8(src_ptr1, vl);
      // Use round-to-nearest-up mode for averaging add
      vuint8m8_t row_out = __riscv_vaaddu_vv_u8m8(row0, row1, vl);
      __riscv_vse8_v_u8m8(dst_ptr, row_out, vl);
      dst_w -= vl;
      src_ptr += vl;
      src_ptr1 += vl;
      dst_ptr += vl;
    } while (dst_w > 0);
    return;
  }
  // General purpose row blend.
  do {
    size_t vl = __riscv_vsetvl_e8m4(dst_w);
    vuint8m4_t row0 = __riscv_vle8_v_u8m4(src_ptr, vl);
    vuint16m8_t acc = __riscv_vwmulu_vx_u16m8(row0, y0_fraction, vl);
    vuint8m4_t row1 = __riscv_vle8_v_u8m4(src_ptr1, vl);
    acc = __riscv_vwmaccu_vx_u16m8(acc, y1_fraction, row1, vl);
    // Use round-to-nearest-up mode for vnclip
    __riscv_vse8_v_u8m4(dst_ptr, __riscv_vnclipu_wx_u8m4(acc, 8, vl), vl);
    dst_w -= vl;
    src_ptr += vl;
    src_ptr1 += vl;
    dst_ptr += vl;
  } while (dst_w > 0);
}
#endif
#endif

#ifdef HAS_SPLITRGBROW_RVV
void SplitRGBRow_RVV(const uint8_t* src_rgb,
                     uint8_t* dst_r,
                     uint8_t* dst_g,
                     uint8_t* dst_b,
                     int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x3_t v_src = __riscv_vlseg3e8_v_u8m2x3(src_rgb, vl);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x3_u8m2(v_src, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x3_u8m2(v_src, 1);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x3_u8m2(v_src, 2);
    __riscv_vse8_v_u8m2(dst_r, v_r, vl);
    __riscv_vse8_v_u8m2(dst_g, v_g, vl);
    __riscv_vse8_v_u8m2(dst_b, v_b, vl);
    w -= vl;
    dst_r += vl;
    dst_g += vl;
    dst_b += vl;
    src_rgb += vl * 3;
  } while (w > 0);
}
#endif

#ifdef HAS_MERGERGBROW_RVV
void MergeRGBRow_RVV(const uint8_t* src_r,
                     const uint8_t* src_g,
                     const uint8_t* src_b,
                     uint8_t* dst_rgb,
                     int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2_t v_r = __riscv_vle8_v_u8m2(src_r, vl);
    vuint8m2_t v_g = __riscv_vle8_v_u8m2(src_g, vl);
    vuint8m2_t v_b = __riscv_vle8_v_u8m2(src_b, vl);
    vuint8m2x3_t v_dst = __riscv_vcreate_v_u8m2x3(v_r, v_g, v_b);
    __riscv_vsseg3e8_v_u8m2x3(dst_rgb, v_dst, vl);
    w -= vl;
    src_r += vl;
    src_g += vl;
    src_b += vl;
    dst_rgb += vl * 3;
  } while (w > 0);
}
#endif

#ifdef HAS_SPLITARGBROW_RVV
void SplitARGBRow_RVV(const uint8_t* src_argb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      uint8_t* dst_a,
                      int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src, 2);
    vuint8m2_t v_a = __riscv_vget_v_u8m2x4_u8m2(v_src, 3);
    __riscv_vse8_v_u8m2(dst_a, v_a, vl);
    __riscv_vse8_v_u8m2(dst_r, v_r, vl);
    __riscv_vse8_v_u8m2(dst_g, v_g, vl);
    __riscv_vse8_v_u8m2(dst_b, v_b, vl);
    w -= vl;
    dst_a += vl;
    dst_r += vl;
    dst_g += vl;
    dst_b += vl;
    src_argb += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_MERGEARGBROW_RVV
void MergeARGBRow_RVV(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      const uint8_t* src_a,
                      uint8_t* dst_argb,
                      int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2_t v_r = __riscv_vle8_v_u8m2(src_r, vl);
    vuint8m2_t v_g = __riscv_vle8_v_u8m2(src_g, vl);
    vuint8m2_t v_b = __riscv_vle8_v_u8m2(src_b, vl);
    vuint8m2_t v_a = __riscv_vle8_v_u8m2(src_a, vl);
    vuint8m2x4_t v_dst = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst, vl);
    w -= vl;
    src_r += vl;
    src_g += vl;
    src_b += vl;
    src_a += vl;
    dst_argb += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_SPLITXRGBROW_RVV
void SplitXRGBRow_RVV(const uint8_t* src_argb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src, 2);
    __riscv_vse8_v_u8m2(dst_r, v_r, vl);
    __riscv_vse8_v_u8m2(dst_g, v_g, vl);
    __riscv_vse8_v_u8m2(dst_b, v_b, vl);
    w -= vl;
    dst_r += vl;
    dst_g += vl;
    dst_b += vl;
    src_argb += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_MERGEXRGBROW_RVV
void MergeXRGBRow_RVV(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      uint8_t* dst_argb,
                      int width) {
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvl_e8m2(w);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    vuint8m2_t v_r = __riscv_vle8_v_u8m2(src_r, vl);
    vuint8m2_t v_g = __riscv_vle8_v_u8m2(src_g, vl);
    vuint8m2_t v_b = __riscv_vle8_v_u8m2(src_b, vl);
    vuint8m2x4_t v_dst = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst, vl);
    w -= vl;
    src_r += vl;
    src_g += vl;
    src_b += vl;
    dst_argb += vl * 4;
    vl = __riscv_vsetvl_e8m2(w);
  } while (w > 0);
}
#endif

#ifdef HAS_SPLITUVROW_RVV
void SplitUVRow_RVV(const uint8_t* src_uv,
                    uint8_t* dst_u,
                    uint8_t* dst_v,
                    int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m4(w);
    vuint8m4x2_t v_src = __riscv_vlseg2e8_v_u8m4x2(src_uv, vl);
    vuint8m4_t v_u = __riscv_vget_v_u8m4x2_u8m4(v_src, 0);
    vuint8m4_t v_v = __riscv_vget_v_u8m4x2_u8m4(v_src, 1);
    __riscv_vse8_v_u8m4(dst_u, v_u, vl);
    __riscv_vse8_v_u8m4(dst_v, v_v, vl);
    w -= vl;
    dst_u += vl;
    dst_v += vl;
    src_uv += 2 * vl;
  } while (w > 0);
}
#endif

#ifdef HAS_MERGEUVROW_RVV
void MergeUVRow_RVV(const uint8_t* src_u,
                    const uint8_t* src_v,
                    uint8_t* dst_uv,
                    int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m4(w);
    vuint8m4_t v_u = __riscv_vle8_v_u8m4(src_u, vl);
    vuint8m4_t v_v = __riscv_vle8_v_u8m4(src_v, vl);
    vuint8m4x2_t v_dst = __riscv_vcreate_v_u8m4x2(v_u, v_v);
    __riscv_vsseg2e8_v_u8m4x2(dst_uv, v_dst, vl);
    w -= vl;
    src_u += vl;
    src_v += vl;
    dst_uv += 2 * vl;
  } while (w > 0);
}
#endif

// ARGB expects first 3 values to contain RGB and 4th value is ignored
#ifdef HAS_ARGBTOYMATRIXROW_RVV
void ARGBToYMatrixRow_RVV(const uint8_t* src_argb,
                          uint8_t* dst_y,
                          int width,
                          const struct ArgbConstants* c) {
  assert(width != 0);
  size_t w = (size_t)width;
  vuint8m2_t v_y0, v_y1, v_y2, v_y3;
  vuint16m4_t v_addy;
  size_t vl = __riscv_vsetvl_e8m2(w);
  v_y0 = __riscv_vmv_v_x_u8m2(c->kRGBToY[0], vl);
  v_y1 = __riscv_vmv_v_x_u8m2(c->kRGBToY[1], vl);
  v_y2 = __riscv_vmv_v_x_u8m2(c->kRGBToY[2], vl);
  v_y3 = __riscv_vmv_v_x_u8m2(c->kRGBToY[3], vl);
  v_addy = __riscv_vmv_v_x_u16m4(c->kAddY[0], vl);
  do {
    vuint8m2_t v_y;
    vuint16m4_t v_y_u16;
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b0 = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 0);
    vuint8m2_t v_b1 = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 1);
    vuint8m2_t v_b2 = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 2);
    vuint8m2_t v_b3 = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 3);
    v_y_u16 = __riscv_vwmulu_vv_u16m4(v_b0, v_y0, vl);
    v_y_u16 = __riscv_vwmaccu_vv_u16m4(v_y_u16, v_y1, v_b1, vl);
    v_y_u16 = __riscv_vwmaccu_vv_u16m4(v_y_u16, v_y2, v_b2, vl);
    v_y_u16 = __riscv_vwmaccu_vv_u16m4(v_y_u16, v_y3, v_b3, vl);
    v_y_u16 = __riscv_vadd_vv_u16m4(v_y_u16, v_addy, vl);
    v_y = __riscv_vnsrl_wx_u8m2(v_y_u16, 8, vl);
    __riscv_vse8_v_u8m2(dst_y, v_y, vl);
    w -= vl;
    src_argb += 4 * vl;
    dst_y += vl;
  } while (w > 0);
}
#endif

#ifdef HAS_RGBTOYMATRIXROW_RVV
void RGBToYMatrixRow_RVV(const uint8_t* src_rgb,
                         uint8_t* dst_y,
                         int width,
                         const struct ArgbConstants* c) {
  assert(width != 0);
  size_t w = (size_t)width;
  vuint8m2_t v_by, v_gy, v_ry;  // vectors are to store RGBToY constant
  vuint16m4_t v_addy;           // vector is to store kAddY
  size_t vl = __riscv_vsetvl_e8m2(w);
  v_by = __riscv_vmv_v_x_u8m2(c->kRGBToY[0], vl);
  v_gy = __riscv_vmv_v_x_u8m2(c->kRGBToY[1], vl);
  v_ry = __riscv_vmv_v_x_u8m2(c->kRGBToY[2], vl);
  v_addy = __riscv_vmv_v_x_u16m4(c->kAddY[0], vl);
  do {
    vuint8m2_t v_y;
    vuint16m4_t v_y_u16;
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x3_t v_src_rgb = __riscv_vlseg3e8_v_u8m2x3(src_rgb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 2);
    v_y_u16 = __riscv_vwmulu_vv_u16m4(v_r, v_ry, vl);
    v_y_u16 = __riscv_vwmaccu_vv_u16m4(v_y_u16, v_gy, v_g, vl);
    v_y_u16 = __riscv_vwmaccu_vv_u16m4(v_y_u16, v_by, v_b, vl);
    v_y_u16 = __riscv_vadd_vv_u16m4(v_y_u16, v_addy, vl);
    v_y = __riscv_vnsrl_wx_u8m2(v_y_u16, 8, vl);
    __riscv_vse8_v_u8m2(dst_y, v_y, vl);
    w -= vl;
    src_rgb += 3 * vl;
    dst_y += vl;
  } while (w > 0);
}
#endif

#ifdef HAS_ARGBTOUV444MATRIXROW_RVV
void ARGBToUV444MatrixRow_RVV(const uint8_t* src_argb,
                              uint8_t* dst_u,
                              uint8_t* dst_v,
                              int width,
                              const struct ArgbConstants* c) {
  assert(width != 0);
  size_t w = (size_t)width;
  vint8m2_t v_u0, v_u1, v_u2, v_u3;
  vint8m2_t v_v0, v_v1, v_v2, v_v3;
  vint16m4_t v_adduv;
  size_t vl = __riscv_vsetvl_e8m2(w);
  v_u0 = __riscv_vmv_v_x_i8m2(c->kRGBToU[0], vl);
  v_u1 = __riscv_vmv_v_x_i8m2(c->kRGBToU[1], vl);
  v_u2 = __riscv_vmv_v_x_i8m2(c->kRGBToU[2], vl);
  v_u3 = __riscv_vmv_v_x_i8m2(c->kRGBToU[3], vl);
  v_v0 = __riscv_vmv_v_x_i8m2(c->kRGBToV[0], vl);
  v_v1 = __riscv_vmv_v_x_i8m2(c->kRGBToV[1], vl);
  v_v2 = __riscv_vmv_v_x_i8m2(c->kRGBToV[2], vl);
  v_v3 = __riscv_vmv_v_x_i8m2(c->kRGBToV[3], vl);
  v_adduv = __riscv_vmv_v_x_i16m4((int16_t)c->kAddUV[0], vl);
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b0 = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 0);
    vuint8m2_t v_b1 = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 1);
    vuint8m2_t v_b2 = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 2);
    vuint8m2_t v_b3 = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 3);

    vint16m4_t v_u_i16 = __riscv_vwmulsu_vv_i16m4(v_u0, v_b0, vl);
    v_u_i16 = __riscv_vwmaccsu_vv_i16m4(v_u_i16, v_u1, v_b1, vl);
    v_u_i16 = __riscv_vwmaccsu_vv_i16m4(v_u_i16, v_u2, v_b2, vl);
    v_u_i16 = __riscv_vwmaccsu_vv_i16m4(v_u_i16, v_u3, v_b3, vl);
    v_u_i16 = __riscv_vsub_vv_i16m4(v_adduv, v_u_i16, vl);
    vuint8m2_t v_u = __riscv_vnsrl_wx_u8m2(
        __riscv_vreinterpret_v_i16m4_u16m4(v_u_i16), 8, vl);
    __riscv_vse8_v_u8m2(dst_u, v_u, vl);

    vint16m4_t v_v_i16 = __riscv_vwmulsu_vv_i16m4(v_v0, v_b0, vl);
    v_v_i16 = __riscv_vwmaccsu_vv_i16m4(v_v_i16, v_v1, v_b1, vl);
    v_v_i16 = __riscv_vwmaccsu_vv_i16m4(v_v_i16, v_v2, v_b2, vl);
    v_v_i16 = __riscv_vwmaccsu_vv_i16m4(v_v_i16, v_v3, v_b3, vl);
    v_v_i16 = __riscv_vsub_vv_i16m4(v_adduv, v_v_i16, vl);
    vuint8m2_t v_v = __riscv_vnsrl_wx_u8m2(
        __riscv_vreinterpret_v_i16m4_u16m4(v_v_i16), 8, vl);
    __riscv_vse8_v_u8m2(dst_v, v_v, vl);

    w -= vl;
    src_argb += 4 * vl;
    dst_u += vl;
    dst_v += vl;
  } while (w > 0);
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
  int w = width >> 1;
  if (w > 0) {
    size_t w_len = (size_t)w;
    vint8m1_t v_u0, v_u1, v_u2, v_u3;
    vint8m1_t v_v0, v_v1, v_v2, v_v3;
    vint16m2_t v_adduv;
    size_t vl = __riscv_vsetvl_e8m1(w_len);
    v_u0 = __riscv_vmv_v_x_i8m1(c->kRGBToU[0], vl);
    v_u1 = __riscv_vmv_v_x_i8m1(c->kRGBToU[1], vl);
    v_u2 = __riscv_vmv_v_x_i8m1(c->kRGBToU[2], vl);
    v_u3 = __riscv_vmv_v_x_i8m1(c->kRGBToU[3], vl);
    v_v0 = __riscv_vmv_v_x_i8m1(c->kRGBToV[0], vl);
    v_v1 = __riscv_vmv_v_x_i8m1(c->kRGBToV[1], vl);
    v_v2 = __riscv_vmv_v_x_i8m1(c->kRGBToV[2], vl);
    v_v3 = __riscv_vmv_v_x_i8m1(c->kRGBToV[3], vl);
    v_adduv = __riscv_vmv_v_x_i16m2((int16_t)c->kAddUV[0], vl);
    do {
      size_t vl = __riscv_vsetvl_e8m1(w_len);
      vuint8m1x8_t v_src0 = __riscv_vlseg8e8_v_u8m1x8(src_argb_0, vl);
      vuint8m1x8_t v_src1 = __riscv_vlseg8e8_v_u8m1x8(src_argb_1, vl);

      vuint8m1_t b0 = __riscv_vget_v_u8m1x8_u8m1(v_src0, 0);
      vuint8m1_t g0 = __riscv_vget_v_u8m1x8_u8m1(v_src0, 1);
      vuint8m1_t r0 = __riscv_vget_v_u8m1x8_u8m1(v_src0, 2);
      vuint8m1_t a0 = __riscv_vget_v_u8m1x8_u8m1(v_src0, 3);
      vuint8m1_t b1 = __riscv_vget_v_u8m1x8_u8m1(v_src0, 4);
      vuint8m1_t g1 = __riscv_vget_v_u8m1x8_u8m1(v_src0, 5);
      vuint8m1_t r1 = __riscv_vget_v_u8m1x8_u8m1(v_src0, 6);
      vuint8m1_t a1 = __riscv_vget_v_u8m1x8_u8m1(v_src0, 7);

      vuint8m1_t b0_1 = __riscv_vget_v_u8m1x8_u8m1(v_src1, 0);
      vuint8m1_t g0_1 = __riscv_vget_v_u8m1x8_u8m1(v_src1, 1);
      vuint8m1_t r0_1 = __riscv_vget_v_u8m1x8_u8m1(v_src1, 2);
      vuint8m1_t a0_1 = __riscv_vget_v_u8m1x8_u8m1(v_src1, 3);
      vuint8m1_t b1_1 = __riscv_vget_v_u8m1x8_u8m1(v_src1, 4);
      vuint8m1_t g1_1 = __riscv_vget_v_u8m1x8_u8m1(v_src1, 5);
      vuint8m1_t r1_1 = __riscv_vget_v_u8m1x8_u8m1(v_src1, 6);
      vuint8m1_t a1_1 = __riscv_vget_v_u8m1x8_u8m1(v_src1, 7);

      vuint16m2_t sum_b = __riscv_vwaddu_vv_u16m2(b0, b1, vl);
      sum_b = __riscv_vwaddu_wv_u16m2(sum_b, b0_1, vl);
      sum_b = __riscv_vwaddu_wv_u16m2(sum_b, b1_1, vl);
      sum_b = __riscv_vadd_vx_u16m2(sum_b, 2, vl);
      vuint8m1_t b = __riscv_vnsrl_wx_u8m1(sum_b, 2, vl);

      vuint16m2_t sum_g = __riscv_vwaddu_vv_u16m2(g0, g1, vl);
      sum_g = __riscv_vwaddu_wv_u16m2(sum_g, g0_1, vl);
      sum_g = __riscv_vwaddu_wv_u16m2(sum_g, g1_1, vl);
      sum_g = __riscv_vadd_vx_u16m2(sum_g, 2, vl);
      vuint8m1_t g = __riscv_vnsrl_wx_u8m1(sum_g, 2, vl);

      vuint16m2_t sum_r = __riscv_vwaddu_vv_u16m2(r0, r1, vl);
      sum_r = __riscv_vwaddu_wv_u16m2(sum_r, r0_1, vl);
      sum_r = __riscv_vwaddu_wv_u16m2(sum_r, r1_1, vl);
      sum_r = __riscv_vadd_vx_u16m2(sum_r, 2, vl);
      vuint8m1_t r = __riscv_vnsrl_wx_u8m1(sum_r, 2, vl);

      vuint16m2_t sum_a = __riscv_vwaddu_vv_u16m2(a0, a1, vl);
      sum_a = __riscv_vwaddu_wv_u16m2(sum_a, a0_1, vl);
      sum_a = __riscv_vwaddu_wv_u16m2(sum_a, a1_1, vl);
      sum_a = __riscv_vadd_vx_u16m2(sum_a, 2, vl);
      vuint8m1_t a = __riscv_vnsrl_wx_u8m1(sum_a, 2, vl);

      vint16m2_t v_u_i16 = __riscv_vwmulsu_vv_i16m2(v_u0, b, vl);
      v_u_i16 = __riscv_vwmaccsu_vv_i16m2(v_u_i16, v_u1, g, vl);
      v_u_i16 = __riscv_vwmaccsu_vv_i16m2(v_u_i16, v_u2, r, vl);
      v_u_i16 = __riscv_vwmaccsu_vv_i16m2(v_u_i16, v_u3, a, vl);
      v_u_i16 = __riscv_vsub_vv_i16m2(v_adduv, v_u_i16, vl);
      vuint8m1_t v_u = __riscv_vnsrl_wx_u8m1(
          __riscv_vreinterpret_v_i16m2_u16m2(v_u_i16), 8, vl);
      __riscv_vse8_v_u8m1(dst_u, v_u, vl);

      vint16m2_t v_v_i16 = __riscv_vwmulsu_vv_i16m2(v_v0, b, vl);
      v_v_i16 = __riscv_vwmaccsu_vv_i16m2(v_v_i16, v_v1, g, vl);
      v_v_i16 = __riscv_vwmaccsu_vv_i16m2(v_v_i16, v_v2, r, vl);
      v_v_i16 = __riscv_vwmaccsu_vv_i16m2(v_v_i16, v_v3, a, vl);
      v_v_i16 = __riscv_vsub_vv_i16m2(v_adduv, v_v_i16, vl);
      vuint8m1_t v_v = __riscv_vnsrl_wx_u8m1(
          __riscv_vreinterpret_v_i16m2_u16m2(v_v_i16), 8, vl);
      __riscv_vse8_v_u8m1(dst_v, v_v, vl);

      w_len -= vl;
      src_argb_0 += 8 * vl;
      src_argb_1 += 8 * vl;
      dst_u += vl;
      dst_v += vl;
    } while (w_len > 0);
  }
  if (width & 1) {
    uint8_t b = (src_argb_0[0] + src_argb_1[0] + 1) >> 1;
    uint8_t g = (src_argb_0[1] + src_argb_1[1] + 1) >> 1;
    uint8_t r = (src_argb_0[2] + src_argb_1[2] + 1) >> 1;
    uint8_t a = (src_argb_0[3] + src_argb_1[3] + 1) >> 1;
    dst_u[0] = (c->kAddUV[0] - (c->kRGBToU[0] * b + c->kRGBToU[1] * g +
                                c->kRGBToU[2] * r + c->kRGBToU[3] * a)) >>
               8;
    dst_v[0] = (c->kAddUV[0] - (c->kRGBToV[0] * b + c->kRGBToV[1] * g +
                                c->kRGBToV[2] * r + c->kRGBToV[3] * a)) >>
               8;
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
  size_t w = (size_t)width;
  vint8m2_t v_bu, v_gu, v_ru;
  vint8m2_t v_bv, v_gv, v_rv;
  vint16m4_t v_adduv;
  size_t vl = __riscv_vsetvl_e8m2(w);
  v_bu = __riscv_vmv_v_x_i8m2(c->kRGBToU[0], vl);
  v_gu = __riscv_vmv_v_x_i8m2(c->kRGBToU[1], vl);
  v_ru = __riscv_vmv_v_x_i8m2(c->kRGBToU[2], vl);
  v_bv = __riscv_vmv_v_x_i8m2(c->kRGBToV[0], vl);
  v_gv = __riscv_vmv_v_x_i8m2(c->kRGBToV[1], vl);
  v_rv = __riscv_vmv_v_x_i8m2(c->kRGBToV[2], vl);
  v_adduv = __riscv_vmv_v_x_i16m4((int16_t)c->kAddUV[0], vl);
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x3_t v_src_rgb = __riscv_vlseg3e8_v_u8m2x3(src_rgb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x3_u8m2(v_src_rgb, 2);

    vint16m4_t v_u_i16 = __riscv_vwmulsu_vv_i16m4(v_ru, v_r, vl);
    v_u_i16 = __riscv_vwmaccsu_vv_i16m4(v_u_i16, v_gu, v_g, vl);
    v_u_i16 = __riscv_vwmaccsu_vv_i16m4(v_u_i16, v_bu, v_b, vl);
    v_u_i16 = __riscv_vsub_vv_i16m4(v_adduv, v_u_i16, vl);
    vuint8m2_t v_u = __riscv_vnsrl_wx_u8m2(
        __riscv_vreinterpret_v_i16m4_u16m4(v_u_i16), 8, vl);
    __riscv_vse8_v_u8m2(dst_u, v_u, vl);

    vint16m4_t v_v_i16 = __riscv_vwmulsu_vv_i16m4(v_rv, v_r, vl);
    v_v_i16 = __riscv_vwmaccsu_vv_i16m4(v_v_i16, v_gv, v_g, vl);
    v_v_i16 = __riscv_vwmaccsu_vv_i16m4(v_v_i16, v_bv, v_b, vl);
    v_v_i16 = __riscv_vsub_vv_i16m4(v_adduv, v_v_i16, vl);
    vuint8m2_t v_v = __riscv_vnsrl_wx_u8m2(
        __riscv_vreinterpret_v_i16m4_u16m4(v_v_i16), 8, vl);
    __riscv_vse8_v_u8m2(dst_v, v_v, vl);

    w -= vl;
    src_rgb += 3 * vl;
    dst_u += vl;
    dst_v += vl;
  } while (w > 0);
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
  int w = width >> 1;
  if (w > 0) {
    size_t w_len = (size_t)w;
    vint8m1_t v_bu, v_gu, v_ru;
    vint8m1_t v_bv, v_gv, v_rv;
    vint16m2_t v_adduv;
    size_t vl = __riscv_vsetvl_e8m1(w_len);
    v_bu = __riscv_vmv_v_x_i8m1(c->kRGBToU[0], vl);
    v_gu = __riscv_vmv_v_x_i8m1(c->kRGBToU[1], vl);
    v_ru = __riscv_vmv_v_x_i8m1(c->kRGBToU[2], vl);
    v_bv = __riscv_vmv_v_x_i8m1(c->kRGBToV[0], vl);
    v_gv = __riscv_vmv_v_x_i8m1(c->kRGBToV[1], vl);
    v_rv = __riscv_vmv_v_x_i8m1(c->kRGBToV[2], vl);
    v_adduv = __riscv_vmv_v_x_i16m2((int16_t)c->kAddUV[0], vl);
    do {
      size_t vl = __riscv_vsetvl_e8m1(w_len);
      vuint8m1x6_t v_src0 = __riscv_vlseg6e8_v_u8m1x6(src_rgb_0, vl);
      vuint8m1x6_t v_src1 = __riscv_vlseg6e8_v_u8m1x6(src_rgb_1, vl);

      vuint8m1_t b0 = __riscv_vget_v_u8m1x6_u8m1(v_src0, 0);
      vuint8m1_t g0 = __riscv_vget_v_u8m1x6_u8m1(v_src0, 1);
      vuint8m1_t r0 = __riscv_vget_v_u8m1x6_u8m1(v_src0, 2);
      vuint8m1_t b1 = __riscv_vget_v_u8m1x6_u8m1(v_src0, 3);
      vuint8m1_t g1 = __riscv_vget_v_u8m1x6_u8m1(v_src0, 4);
      vuint8m1_t r1 = __riscv_vget_v_u8m1x6_u8m1(v_src0, 5);

      vuint8m1_t b0_1 = __riscv_vget_v_u8m1x6_u8m1(v_src1, 0);
      vuint8m1_t g0_1 = __riscv_vget_v_u8m1x6_u8m1(v_src1, 1);
      vuint8m1_t r0_1 = __riscv_vget_v_u8m1x6_u8m1(v_src1, 2);
      vuint8m1_t b1_1 = __riscv_vget_v_u8m1x6_u8m1(v_src1, 3);
      vuint8m1_t g1_1 = __riscv_vget_v_u8m1x6_u8m1(v_src1, 4);
      vuint8m1_t r1_1 = __riscv_vget_v_u8m1x6_u8m1(v_src1, 5);

      vuint16m2_t sum_b = __riscv_vwaddu_vv_u16m2(b0, b1, vl);
      sum_b = __riscv_vwaddu_wv_u16m2(sum_b, b0_1, vl);
      sum_b = __riscv_vwaddu_wv_u16m2(sum_b, b1_1, vl);
      sum_b = __riscv_vadd_vx_u16m2(sum_b, 2, vl);
      vuint8m1_t b = __riscv_vnsrl_wx_u8m1(sum_b, 2, vl);

      vuint16m2_t sum_g = __riscv_vwaddu_vv_u16m2(g0, g1, vl);
      sum_g = __riscv_vwaddu_wv_u16m2(sum_g, g0_1, vl);
      sum_g = __riscv_vwaddu_wv_u16m2(sum_g, g1_1, vl);
      sum_g = __riscv_vadd_vx_u16m2(sum_g, 2, vl);
      vuint8m1_t g = __riscv_vnsrl_wx_u8m1(sum_g, 2, vl);

      vuint16m2_t sum_r = __riscv_vwaddu_vv_u16m2(r0, r1, vl);
      sum_r = __riscv_vwaddu_wv_u16m2(sum_r, r0_1, vl);
      sum_r = __riscv_vwaddu_wv_u16m2(sum_r, r1_1, vl);
      sum_r = __riscv_vadd_vx_u16m2(sum_r, 2, vl);
      vuint8m1_t r = __riscv_vnsrl_wx_u8m1(sum_r, 2, vl);

      vint16m2_t v_u_i16 = __riscv_vwmulsu_vv_i16m2(v_ru, r, vl);
      v_u_i16 = __riscv_vwmaccsu_vv_i16m2(v_u_i16, v_gu, g, vl);
      v_u_i16 = __riscv_vwmaccsu_vv_i16m2(v_u_i16, v_bu, b, vl);
      v_u_i16 = __riscv_vsub_vv_i16m2(v_adduv, v_u_i16, vl);
      vuint8m1_t v_u = __riscv_vnsrl_wx_u8m1(
          __riscv_vreinterpret_v_i16m2_u16m2(v_u_i16), 8, vl);
      __riscv_vse8_v_u8m1(dst_u, v_u, vl);

      vint16m2_t v_v_i16 = __riscv_vwmulsu_vv_i16m2(v_rv, r, vl);
      v_v_i16 = __riscv_vwmaccsu_vv_i16m2(v_v_i16, v_gv, g, vl);
      v_v_i16 = __riscv_vwmaccsu_vv_i16m2(v_v_i16, v_bv, b, vl);
      v_v_i16 = __riscv_vsub_vv_i16m2(v_adduv, v_v_i16, vl);
      vuint8m1_t v_v = __riscv_vnsrl_wx_u8m1(
          __riscv_vreinterpret_v_i16m2_u16m2(v_v_i16), 8, vl);
      __riscv_vse8_v_u8m1(dst_v, v_v, vl);

      w_len -= vl;
      src_rgb_0 += 6 * vl;
      src_rgb_1 += 6 * vl;
      dst_u += vl;
      dst_v += vl;
    } while (w_len > 0);
  }
  if (width & 1) {
    uint8_t b = (src_rgb_0[0] + src_rgb_1[0] + 1) >> 1;
    uint8_t g = (src_rgb_0[1] + src_rgb_1[1] + 1) >> 1;
    uint8_t r = (src_rgb_0[2] + src_rgb_1[2] + 1) >> 1;
    dst_u[0] = (c->kAddUV[0] - (c->kRGBToU[0] * b + c->kRGBToU[1] * g +
                                c->kRGBToU[2] * r)) >>
               8;
    dst_v[0] = (c->kAddUV[0] - (c->kRGBToV[0] * b + c->kRGBToV[1] * g +
                                c->kRGBToV[2] * r)) >>
               8;
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
  size_t w = (size_t)width;
  size_t vl = __riscv_vsetvlmax_e8m2();
  // clamp255((((256 - a) * b) >> 8) + f)
  // = b * (256 - a) / 256 + f
  // = b - (b * a / 256) + f
  vuint8m2_t v_255 = __riscv_vmv_v_x_u8m2(255, vl);
  do {
    vuint8m2_t v_tmp_b, v_tmp_g, v_tmp_r;
    vuint8m2_t v_dst_b, v_dst_g, v_dst_r;
    vuint8m2x4_t v_dst_argb;
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src0_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_src0_b = __riscv_vget_v_u8m2x4_u8m2(v_src0_argb, 0);
    vuint8m2_t v_src0_g = __riscv_vget_v_u8m2x4_u8m2(v_src0_argb, 1);
    vuint8m2_t v_src0_r = __riscv_vget_v_u8m2x4_u8m2(v_src0_argb, 2);
    vuint8m2_t v_src0_a = __riscv_vget_v_u8m2x4_u8m2(v_src0_argb, 3);
    vuint8m2x4_t v_src1_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb1, vl);
    vuint8m2_t v_src1_b = __riscv_vget_v_u8m2x4_u8m2(v_src1_argb, 0);
    vuint8m2_t v_src1_g = __riscv_vget_v_u8m2x4_u8m2(v_src1_argb, 1);
    vuint8m2_t v_src1_r = __riscv_vget_v_u8m2x4_u8m2(v_src1_argb, 2);

    v_tmp_b = __riscv_vmulhu_vv_u8m2(v_src1_b, v_src0_a, vl);
    v_tmp_g = __riscv_vmulhu_vv_u8m2(v_src1_g, v_src0_a, vl);
    v_tmp_r = __riscv_vmulhu_vv_u8m2(v_src1_r, v_src0_a, vl);

    v_dst_b = __riscv_vsub_vv_u8m2(v_src1_b, v_tmp_b, vl);
    v_dst_g = __riscv_vsub_vv_u8m2(v_src1_g, v_tmp_g, vl);
    v_dst_r = __riscv_vsub_vv_u8m2(v_src1_r, v_tmp_r, vl);

    v_dst_b = __riscv_vsaddu_vv_u8m2(v_dst_b, v_src0_b, vl);
    v_dst_g = __riscv_vsaddu_vv_u8m2(v_dst_g, v_src0_g, vl);
    v_dst_r = __riscv_vsaddu_vv_u8m2(v_dst_r, v_src0_r, vl);

    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_dst_b, v_dst_g, v_dst_r, v_255);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);

    w -= vl;
    src_argb += 4 * vl;
    src_argb1 += 4 * vl;
    dst_argb += 4 * vl;
  } while (w > 0);
}
#endif

#ifdef HAS_BLENDPLANEROW_RVV
void BlendPlaneRow_RVV(const uint8_t* src0,
                       const uint8_t* src1,
                       const uint8_t* alpha,
                       uint8_t* dst,
                       int width) {
  size_t w = (size_t)width;
  do {
    vuint16m8_t v_dst_u16;
    vuint8m4_t v_dst;
    size_t vl = __riscv_vsetvl_e8m4(w);
    vuint8m4_t v_src0 = __riscv_vle8_v_u8m4(src0, vl);
    vuint8m4_t v_src1 = __riscv_vle8_v_u8m4(src1, vl);
    vuint8m4_t v_alpha = __riscv_vle8_v_u8m4(alpha, vl);
    vuint8m4_t v_255_minus_alpha = __riscv_vrsub_vx_u8m4(v_alpha, 255u, vl);

    // (a * foreground) + (1-a) * background
    v_dst_u16 = __riscv_vwmulu_vv_u16m8(v_alpha, v_src0, vl);
    v_dst_u16 =
        __riscv_vwmaccu_vv_u16m8(v_dst_u16, v_255_minus_alpha, v_src1, vl);
    v_dst_u16 = __riscv_vadd_vx_u16m8(v_dst_u16, 255u, vl);
    v_dst = __riscv_vnsrl_wx_u8m4(v_dst_u16, 8, vl);

    __riscv_vse8_v_u8m4(dst, v_dst, vl);
    w -= vl;
    src0 += vl;
    src1 += vl;
    alpha += vl;
    dst += vl;
  } while (w > 0);
}
#endif

// Attenuate: (f * a + 255) >> 8
#ifdef HAS_ARGBATTENUATEROW_RVV
void ARGBAttenuateRow_RVV(const uint8_t* src_argb,
                          uint8_t* dst_argb,
                          int width) {
  size_t w = (size_t)width;
  do {
    vuint16m4_t v_ba_16, v_ga_16, v_ra_16;
    vuint8m2x4_t v_dst_argb;
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_b = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 0);
    vuint8m2_t v_g = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 1);
    vuint8m2_t v_r = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 2);
    vuint8m2_t v_a = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 3);
    // f * a
    v_ba_16 = __riscv_vwmulu_vv_u16m4(v_b, v_a, vl);
    v_ga_16 = __riscv_vwmulu_vv_u16m4(v_g, v_a, vl);
    v_ra_16 = __riscv_vwmulu_vv_u16m4(v_r, v_a, vl);
    // f * a + 255
    v_ba_16 = __riscv_vadd_vx_u16m4(v_ba_16, 255u, vl);
    v_ga_16 = __riscv_vadd_vx_u16m4(v_ga_16, 255u, vl);
    v_ra_16 = __riscv_vadd_vx_u16m4(v_ra_16, 255u, vl);
    // (f * a + 255) >> 8
    v_b = __riscv_vnsrl_wx_u8m2(v_ba_16, 8, vl);
    v_g = __riscv_vnsrl_wx_u8m2(v_ga_16, 8, vl);
    v_r = __riscv_vnsrl_wx_u8m2(v_ra_16, 8, vl);

    v_dst_argb = __riscv_vcreate_v_u8m2x4(v_b, v_g, v_r, v_a);
    __riscv_vsseg4e8_v_u8m2x4(dst_argb, v_dst_argb, vl);
    w -= vl;
    src_argb += vl * 4;
    dst_argb += vl * 4;
  } while (w > 0);
}
#endif

#ifdef HAS_ARGBEXTRACTALPHAROW_RVV
void ARGBExtractAlphaRow_RVV(const uint8_t* src_argb,
                             uint8_t* dst_a,
                             int width) {
  size_t w = (size_t)width;
  do {
    size_t vl = __riscv_vsetvl_e8m2(w);
    vuint8m2x4_t v_src_argb = __riscv_vlseg4e8_v_u8m2x4(src_argb, vl);
    vuint8m2_t v_a = __riscv_vget_v_u8m2x4_u8m2(v_src_argb, 3);
    __riscv_vse8_v_u8m2(dst_a, v_a, vl);
    w -= vl;
    src_argb += vl * 4;
    dst_a += vl;
  } while (w > 0);
}
#endif

#ifdef HAS_ARGBCOPYYTOALPHAROW_RVV
void ARGBCopyYToAlphaRow_RVV(const uint8_t* src, uint8_t* dst, int width) {
  size_t w = (size_t)width;
  const ptrdiff_t dst_stride = 4;
  dst += 3;
  do {
    size_t vl = __riscv_vsetvl_e8m8(w);
    vuint8m8_t v_a = __riscv_vle8_v_u8m8(src, vl);
    __riscv_vsse8_v_u8m8(dst, dst_stride, v_a, vl);
    w -= vl;
    src += vl;
    dst += vl * dst_stride;
  } while (w > 0);
}
#endif

#ifdef HAS_CONVERT16TO8ROW_RVV
void Convert16To8Row_RVV(const uint16_t* src_y,
                         uint8_t* dst_y,
                         int scale,
                         int width) {
  size_t w = (size_t)width;
  const int shift = __builtin_clz((int32_t)scale) - 15;
  do {
    size_t vl = __riscv_vsetvl_e16m4(w);
    vuint16m4_t v_src = __riscv_vle16_v_u16m4(src_y, vl);
    vuint8m2_t v_dst = __riscv_vnsrl_wx_u8m2(v_src, shift, vl);
    __riscv_vse8_v_u8m2(dst_y, v_dst, vl);
    w -= vl;
    src_y += vl;
    dst_y += vl;
  } while (w > 0);
}
#endif

#ifdef HAS_HALFROW_16TO8_RVV
void HalfRow_16To8_RVV(const uint16_t* src_uv,
                       ptrdiff_t src_uv_stride,
                       uint8_t* dst_uv,
                       int scale,
                       int width) {
  size_t w = (size_t)width;
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  const int shift = __builtin_clz((int32_t)scale) - 15;
#ifndef LIBYUV_RVV_HAS_VXRM_ARG
  asm volatile("csrwi vxrm, 0");
#endif
  do {
    size_t vl = __riscv_vsetvl_e16m4(w);
    vuint16m4_t v_src0 = __riscv_vle16_v_u16m4(src_uv, vl);
    vuint16m4_t v_src1 = __riscv_vle16_v_u16m4(src_uv1, vl);
#ifdef LIBYUV_RVV_HAS_VXRM_ARG
    vuint16m4_t v_avg =
        __riscv_vaaddu_vv_u16m4(v_src0, v_src1, __RISCV_VXRM_RNU, vl);
#else
    vuint16m4_t v_avg = __riscv_vaaddu_vv_u16m4(v_src0, v_src1, vl);
#endif
    vuint8m2_t v_dst = __riscv_vnsrl_wx_u8m2(v_avg, shift, vl);
    __riscv_vse8_v_u8m2(dst_uv, v_dst, vl);
    w -= vl;
    src_uv += vl;
    src_uv1 += vl;
    dst_uv += vl;
  } while (w > 0);
}
#endif

#ifdef HAS_HALFWIDTHROW_16TO8_RVV
#if defined(LIBYUV_RVV_HAS_TUPLE_TYPE) && defined(LIBYUV_RVV_HAS_VXRM_ARG)
void HalfWidthRow_16To8_RVV(const uint16_t* src_uv,
                            ptrdiff_t src_uv_stride,
                            uint8_t* dst_uv,
                            int scale,
                            int width) {
  const uint16_t* s = src_uv;
  const uint16_t* t = src_uv + src_uv_stride;
  size_t w = (size_t)width;
  const int shift = __builtin_clz((int32_t)scale) - 15;
  do {
    size_t vl = __riscv_vsetvl_e16m4(w);
    vuint16m4x2_t v_s = __riscv_vlseg2e16_v_u16m4x2(s, vl);
    vuint16m4x2_t v_t = __riscv_vlseg2e16_v_u16m4x2(t, vl);
    vuint16m4_t v_s0 = __riscv_vget_v_u16m4x2_u16m4(v_s, 0);
    vuint16m4_t v_s1 = __riscv_vget_v_u16m4x2_u16m4(v_s, 1);
    vuint16m4_t v_t0 = __riscv_vget_v_u16m4x2_u16m4(v_t, 0);
    vuint16m4_t v_t1 = __riscv_vget_v_u16m4x2_u16m4(v_t, 1);
    vuint32m8_t v_s01 = __riscv_vwaddu_vv_u32m8(v_s0, v_s1, vl);
    vuint32m8_t v_t01 = __riscv_vwaddu_vv_u32m8(v_t0, v_t1, vl);
    vuint32m8_t v_st01 = __riscv_vadd_vv_u32m8(v_s01, v_t01, vl);
    vuint16m4_t v_avg =
        __riscv_vnclipu_wx_u16m4(v_st01, 2, __RISCV_VXRM_RNU, vl);
    vuint8m2_t v_dst = __riscv_vnsrl_wx_u8m2(v_avg, shift, vl);
    __riscv_vse8_v_u8m2(dst_uv, v_dst, vl);
    w -= vl;
    s += 2 * vl;
    t += 2 * vl;
    dst_uv += vl;
  } while (w > 0);
}
#else
void HalfWidthRow_16To8_RVV(const uint16_t* src_uv,
                            ptrdiff_t src_uv_stride,
                            uint8_t* dst_uv,
                            int scale,
                            int width) {
  const uint16_t* s = src_uv;
  const uint16_t* t = src_uv + src_uv_stride;
  size_t w = (size_t)width;
  const int shift = __builtin_clz((int32_t)scale) - 15;
  asm volatile("csrwi vxrm, 0");
  do {
    size_t vl = __riscv_vsetvl_e16m4(w);
    vuint16m4_t v_s0, v_s1, v_t0, v_t1;
    __riscv_vlseg2e16_v_u16m4(&v_s0, &v_s1, s, vl);
    __riscv_vlseg2e16_v_u16m4(&v_t0, &v_t1, t, vl);
    vuint32m8_t v_s01 = __riscv_vwaddu_vv_u32m8(v_s0, v_s1, vl);
    vuint32m8_t v_t01 = __riscv_vwaddu_vv_u32m8(v_t0, v_t1, vl);
    vuint32m8_t v_st01 = __riscv_vadd_vv_u32m8(v_s01, v_t01, vl);
    vuint16m4_t v_avg = __riscv_vnclipu_wx_u16m4(v_st01, 2, vl);
    vuint8m2_t v_dst = __riscv_vnsrl_wx_u8m2(v_avg, shift, vl);
    __riscv_vse8_v_u8m2(dst_uv, v_dst, vl);
    w -= vl;
    s += 2 * vl;
    t += 2 * vl;
    dst_uv += vl;
  } while (w > 0);
}
#endif
#endif

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_RVV) && defined(__riscv_vector)

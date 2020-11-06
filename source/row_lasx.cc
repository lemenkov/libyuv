/*
 *  Copyright 2022 The LibYuv Project Authors. All rights reserved.
 *
 *  Copyright (c) 2022 Loongson Technology Corporation Limited
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"

#if !defined(LIBYUV_DISABLE_LASX) && defined(__loongarch_asx)
#include "libyuv/loongson_intrinsics.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#define ALPHA_VAL (-1)

// Fill YUV -> RGB conversion constants into vectors
#define YUVTORGB_SETUP(yuvconst, ubvr, ugvg, yg, yb)                  \
  {                                                                   \
    __m256i ub, vr, ug, vg;                                           \
                                                                      \
    ub = __lasx_xvreplgr2vr_h(yuvconst->kUVToB[0]);                   \
    vr = __lasx_xvreplgr2vr_h(yuvconst->kUVToR[1]);                   \
    ug = __lasx_xvreplgr2vr_h(yuvconst->kUVToG[0]);                   \
    vg = __lasx_xvreplgr2vr_h(yuvconst->kUVToG[1]);                   \
    yg = __lasx_xvreplgr2vr_h(yuvconst->kYToRgb[0]);                  \
    yb = __lasx_xvreplgr2vr_w(yuvconst->kYBiasToRgb[0]);              \
    ubvr = __lasx_xvilvl_h(ub, vr);                                   \
    ugvg = __lasx_xvilvl_h(ug, vg);                                   \
  }

// Load 32 YUV422 pixel data
#define READYUV422_D(psrc_y, psrc_u, psrc_v, out_y, uv_l, uv_h)       \
  {                                                                   \
    __m256i temp0, temp1;                                             \
                                                                      \
    DUP2_ARG2(__lasx_xvld, psrc_y, 0, psrc_u, 0, out_y, temp0);       \
    temp1 = __lasx_xvld(psrc_v, 0);                                   \
    temp0 = __lasx_xvsub_b(temp0, const_0x80);                        \
    temp1 = __lasx_xvsub_b(temp1, const_0x80);                        \
    temp0 = __lasx_vext2xv_h_b(temp0);                                \
    temp1 = __lasx_vext2xv_h_b(temp1);                                \
    uv_l  = __lasx_xvilvl_h(temp0, temp1);                            \
    uv_h  = __lasx_xvilvh_h(temp0, temp1);                            \
  }

// Load 16 YUV422 pixel data
#define READYUV422(psrc_y, psrc_u, psrc_v, out_y, uv)                 \
  {                                                                   \
    __m256i temp0, temp1;                                             \
                                                                      \
    out_y = __lasx_xvld(psrc_y, 0);                                   \
    temp0 = __lasx_xvldrepl_d(psrc_u, 0);                             \
    temp1 = __lasx_xvldrepl_d(psrc_v, 0);                             \
    uv    = __lasx_xvilvl_b(temp0, temp1);                            \
    uv    = __lasx_xvsub_b(uv, const_0x80);                           \
    uv    = __lasx_vext2xv_h_b(uv);                                   \
  }

// Convert 16 pixels of YUV420 to RGB.
#define YUVTORGB_D(in_y, in_uvl, in_uvh, ubvr, ugvg,                           \
                    yg, yb, b_l, b_h, g_l, g_h, r_l, r_h)                      \
  {                                                                            \
    __m256i u_l, u_h, v_l, v_h;                                                \
    __m256i yl_ev, yl_od, yh_ev, yh_od;                                        \
    __m256i temp0, temp1, temp2, temp3;                                        \
                                                                               \
    temp0 = __lasx_xvilvl_b(in_y, in_y);                                       \
    temp1 = __lasx_xvilvh_b(in_y, in_y);                                       \
    yl_ev = __lasx_xvmulwev_w_hu_h(temp0, yg);                                 \
    yl_od = __lasx_xvmulwod_w_hu_h(temp0, yg);                                 \
    yh_ev = __lasx_xvmulwev_w_hu_h(temp1, yg);                                 \
    yh_od = __lasx_xvmulwod_w_hu_h(temp1, yg);                                 \
    DUP4_ARG2(__lasx_xvsrai_w, yl_ev, 16, yl_od, 16, yh_ev, 16, yh_od, 16,     \
              yl_ev, yl_od, yh_ev, yh_od);                                     \
    yl_ev = __lasx_xvadd_w(yl_ev, yb);                                         \
    yl_od = __lasx_xvadd_w(yl_od, yb);                                         \
    yh_ev = __lasx_xvadd_w(yh_ev, yb);                                         \
    yh_od = __lasx_xvadd_w(yh_od, yb);                                         \
    v_l   = __lasx_xvmulwev_w_h(in_uvl, ubvr);                                 \
    u_l   = __lasx_xvmulwod_w_h(in_uvl, ubvr);                                 \
    v_h   = __lasx_xvmulwev_w_h(in_uvh, ubvr);                                 \
    u_h   = __lasx_xvmulwod_w_h(in_uvh, ubvr);                                 \
    temp0 = __lasx_xvadd_w(yl_ev, u_l);                                        \
    temp1 = __lasx_xvadd_w(yl_od, u_l);                                        \
    temp2 = __lasx_xvadd_w(yh_ev, u_h);                                        \
    temp3 = __lasx_xvadd_w(yh_od, u_h);                                        \
    DUP4_ARG2(__lasx_xvsrai_w, temp0, 6, temp1, 6, temp2, 6, temp3, 6,         \
              temp0, temp1, temp2, temp3);                                     \
    DUP4_ARG1(__lasx_xvclip255_w, temp0, temp1, temp2, temp3,                  \
              temp0, temp1, temp2, temp3);                                     \
    b_l   = __lasx_xvpackev_h(temp1, temp0);                                   \
    b_h   = __lasx_xvpackev_h(temp3, temp2);                                   \
    temp0 = __lasx_xvadd_w(yl_ev, v_l);                                        \
    temp1 = __lasx_xvadd_w(yl_od, v_l);                                        \
    temp2 = __lasx_xvadd_w(yh_ev, v_h);                                        \
    temp3 = __lasx_xvadd_w(yh_od, v_h);                                        \
    DUP4_ARG2(__lasx_xvsrai_w, temp0, 6, temp1, 6, temp2, 6, temp3, 6,         \
              temp0, temp1, temp2, temp3);                                     \
    DUP4_ARG1(__lasx_xvclip255_w, temp0, temp1, temp2, temp3,                  \
              temp0, temp1, temp2, temp3);                                     \
    r_l   = __lasx_xvpackev_h(temp1, temp0);                                   \
    r_h   = __lasx_xvpackev_h(temp3, temp2);                                   \
    DUP2_ARG2(__lasx_xvdp2_w_h, in_uvl, ugvg, in_uvh, ugvg, u_l, u_h);         \
    temp0 = __lasx_xvsub_w(yl_ev, u_l);                                        \
    temp1 = __lasx_xvsub_w(yl_od, u_l);                                        \
    temp2 = __lasx_xvsub_w(yh_ev, u_h);                                        \
    temp3 = __lasx_xvsub_w(yh_od, u_h);                                        \
    DUP4_ARG2(__lasx_xvsrai_w, temp0, 6, temp1, 6, temp2, 6, temp3, 6,         \
              temp0, temp1, temp2, temp3);                                     \
    DUP4_ARG1(__lasx_xvclip255_w, temp0, temp1, temp2, temp3,                  \
              temp0, temp1, temp2, temp3);                                     \
    g_l   = __lasx_xvpackev_h(temp1, temp0);                                   \
    g_h   = __lasx_xvpackev_h(temp3, temp2);                                   \
  }

// Convert 8 pixels of YUV420 to RGB.
#define YUVTORGB(in_y, in_uv, ubvr, ugvg,                                      \
                 yg, yb, out_b, out_g, out_r)                                  \
  {                                                                            \
    __m256i u_l, v_l, yl_ev, yl_od;                                            \
    __m256i temp0, temp1;                                                      \
                                                                               \
    in_y  = __lasx_xvpermi_d(in_y, 0xD8);                                      \
    temp0 = __lasx_xvilvl_b(in_y, in_y);                                       \
    yl_ev = __lasx_xvmulwev_w_hu_h(temp0, yg);                                 \
    yl_od = __lasx_xvmulwod_w_hu_h(temp0, yg);                                 \
    DUP2_ARG2(__lasx_xvsrai_w, yl_ev, 16, yl_od, 16, yl_ev, yl_od);            \
    yl_ev = __lasx_xvadd_w(yl_ev, yb);                                         \
    yl_od = __lasx_xvadd_w(yl_od, yb);                                         \
    v_l   = __lasx_xvmulwev_w_h(in_uv, ubvr);                                  \
    u_l   = __lasx_xvmulwod_w_h(in_uv, ubvr);                                  \
    temp0 = __lasx_xvadd_w(yl_ev, u_l);                                        \
    temp1 = __lasx_xvadd_w(yl_od, u_l);                                        \
    DUP2_ARG2(__lasx_xvsrai_w, temp0, 6, temp1, 6, temp0, temp1);              \
    DUP2_ARG1(__lasx_xvclip255_w, temp0, temp1, temp0, temp1);                 \
    out_b = __lasx_xvpackev_h(temp1, temp0);                                   \
    temp0 = __lasx_xvadd_w(yl_ev, v_l);                                        \
    temp1 = __lasx_xvadd_w(yl_od, v_l);                                        \
    DUP2_ARG2(__lasx_xvsrai_w, temp0, 6, temp1, 6, temp0, temp1);              \
    DUP2_ARG1(__lasx_xvclip255_w, temp0, temp1, temp0, temp1);                 \
    out_r = __lasx_xvpackev_h(temp1, temp0);                                   \
    u_l   = __lasx_xvdp2_w_h(in_uv, ugvg);                                     \
    temp0 = __lasx_xvsub_w(yl_ev, u_l);                                        \
    temp1 = __lasx_xvsub_w(yl_od, u_l);                                        \
    DUP2_ARG2(__lasx_xvsrai_w, temp0, 6, temp1, 6, temp0, temp1);              \
    DUP2_ARG1(__lasx_xvclip255_w, temp0, temp1, temp0, temp1);                 \
    out_g = __lasx_xvpackev_h(temp1, temp0);                                   \
  }

// Pack and Store 16 ARGB values.
#define STOREARGB_D(a_l, a_h, r_l, r_h, g_l, g_h,            \
                     b_l, b_h, pdst_argb)                    \
  {                                                          \
    __m256i temp0, temp1, temp2, temp3;                      \
                                                             \
    temp0 = __lasx_xvpackev_b(g_l, b_l);                     \
    temp1 = __lasx_xvpackev_b(a_l, r_l);                     \
    temp2 = __lasx_xvpackev_b(g_h, b_h);                     \
    temp3 = __lasx_xvpackev_b(a_h, r_h);                     \
    r_l   = __lasx_xvilvl_h(temp1, temp0);                   \
    r_h   = __lasx_xvilvh_h(temp1, temp0);                   \
    g_l   = __lasx_xvilvl_h(temp3, temp2);                   \
    g_h   = __lasx_xvilvh_h(temp3, temp2);                   \
    temp0 = __lasx_xvpermi_q(r_h, r_l, 0x20);                \
    temp1 = __lasx_xvpermi_q(g_h, g_l, 0x20);                \
    temp2 = __lasx_xvpermi_q(r_h, r_l, 0x31);                \
    temp3 = __lasx_xvpermi_q(g_h, g_l, 0x31);                \
    __lasx_xvst(temp0, pdst_argb, 0);                        \
    __lasx_xvst(temp1, pdst_argb, 32);                       \
    __lasx_xvst(temp2, pdst_argb, 64);                       \
    __lasx_xvst(temp3, pdst_argb, 96);                       \
    pdst_argb += 128;                                        \
  }

// Pack and Store 8 ARGB values.
#define STOREARGB(in_a, in_r, in_g, in_b, pdst_argb)         \
  {                                                          \
    __m256i temp0, temp1;                                    \
                                                             \
    temp0 = __lasx_xvpackev_b(in_g, in_b);                   \
    temp1 = __lasx_xvpackev_b(in_a, in_r);                   \
    in_a  = __lasx_xvilvl_h(temp1, temp0);                   \
    in_r  = __lasx_xvilvh_h(temp1, temp0);                   \
    temp0 = __lasx_xvpermi_q(in_r, in_a, 0x20);              \
    temp1 = __lasx_xvpermi_q(in_r, in_a, 0x31);              \
    __lasx_xvst(temp0, pdst_argb, 0);                        \
    __lasx_xvst(temp1, pdst_argb, 32);                       \
    pdst_argb += 64;                                         \
  }

void I422ToARGBRow_LASX(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  int x;
  int len = width / 32;
  __m256i vec_yb, vec_yg;
  __m256i vec_ubvr, vec_ugvg;
  __m256i alpha = __lasx_xvldi(0xFF);
  __m256i const_0x80 = __lasx_xvldi(0x80);

  YUVTORGB_SETUP(yuvconstants, vec_ubvr, vec_ugvg, vec_yg, vec_yb);

  for (x = 0; x < len; x++) {
    __m256i y, uv_l, uv_h, b_l, b_h, g_l, g_h, r_l, r_h;

    READYUV422_D(src_y, src_u, src_v, y, uv_l, uv_h);
    YUVTORGB_D(y, uv_l, uv_h, vec_ubvr, vec_ugvg, vec_yg,
                vec_yb, b_l, b_h, g_l, g_h, r_l, r_h);
    STOREARGB_D(alpha, alpha, r_l, r_h, g_l, g_h, b_l, b_h, dst_argb);
    src_y += 32;
    src_u += 16;
    src_v += 16;
  }
}

void I422ToRGBARow_LASX(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  int x;
  int len = width / 32;
  __m256i vec_yb, vec_yg;
  __m256i vec_ubvr, vec_ugvg;
  __m256i alpha = __lasx_xvldi(0xFF);
  __m256i const_0x80 = __lasx_xvldi(0x80);

  YUVTORGB_SETUP(yuvconstants, vec_ubvr, vec_ugvg, vec_yg, vec_yb);

  for (x = 0; x < len; x++) {
    __m256i y, uv_l, uv_h, b_l, b_h, g_l, g_h, r_l, r_h;

    READYUV422_D(src_y, src_u, src_v, y, uv_l, uv_h);
    YUVTORGB_D(y, uv_l, uv_h, vec_ubvr, vec_ugvg, vec_yg,
                vec_yb, b_l, b_h, g_l, g_h, r_l, r_h);
    STOREARGB_D(r_l, r_h, g_l, g_h, b_l, b_h, alpha, alpha, dst_argb);
    src_y += 32;
    src_u += 16;
    src_v += 16;
  }
}

void I422AlphaToARGBRow_LASX(const uint8_t* src_y,
                             const uint8_t* src_u,
                             const uint8_t* src_v,
                             const uint8_t* src_a,
                             uint8_t* dst_argb,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  int x;
  int len = width / 32;
  int res = width & 31;
  __m256i vec_yb, vec_yg;
  __m256i vec_ubvr, vec_ugvg;
  __m256i zero = __lasx_xvldi(0);
  __m256i const_0x80 = __lasx_xvldi(0x80);

  YUVTORGB_SETUP(yuvconstants, vec_ubvr, vec_ugvg, vec_yg, vec_yb);

  for (x = 0; x < len; x++) {
    __m256i y, uv_l, uv_h, b_l, b_h, g_l, g_h, r_l, r_h, a_l, a_h;

    y   = __lasx_xvld(src_a, 0);
    a_l = __lasx_xvilvl_b(zero, y);
    a_h = __lasx_xvilvh_b(zero, y);
    READYUV422_D(src_y, src_u, src_v, y, uv_l, uv_h);
    YUVTORGB_D(y, uv_l, uv_h, vec_ubvr, vec_ugvg, vec_yg,
                vec_yb, b_l, b_h, g_l, g_h, r_l, r_h);
    STOREARGB_D(a_l, a_h, r_l, r_h, g_l, g_h, b_l, b_h, dst_argb);
    src_y += 32;
    src_u += 16;
    src_v += 16;
    src_a += 32;
  }
  if (res) {
    __m256i y, uv, r, g, b, a;
    a = __lasx_xvld(src_a, 0);
    a = __lasx_vext2xv_hu_bu(a);
    READYUV422(src_y, src_u, src_v, y, uv);
    YUVTORGB(y, uv, vec_ubvr, vec_ugvg, vec_yg, vec_yb, b, g, r);
    STOREARGB(a, r, g, b, dst_argb);
  }
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_LASX) && defined(__loongarch_asx)

/*
 *  Copyright 2016 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/scale_row.h"

// This module is for GCC MSA
#if !defined(LIBYUV_DISABLE_MSA) && defined(__mips_msa)
#include "libyuv/macros_msa.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

void ScaleARGBRowDown2_MSA(const uint8_t* src_argb,
                           ptrdiff_t src_stride,
                           uint8_t* dst_argb,
                           int dst_width) {
  int x;
  v16u8 src0, src1, dst0;

  for (x = 0; x < dst_width; x += 4) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    dst0 = (v16u8)__msa_pckod_w((v4i32)src1, (v4i32)src0);
    ST_UB(dst0, dst_argb);
    src_argb += 32;
    dst_argb += 16;
  }
}

void ScaleARGBRowDown2Linear_MSA(const uint8_t* src_argb,
                                 ptrdiff_t src_stride,
                                 uint8_t* dst_argb,
                                 int dst_width) {
  int x;
  v16u8 src0, src1, vec0, vec1, dst0;

  for (x = 0; x < dst_width; x += 4) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    vec0 = (v16u8)__msa_pckev_w((v4i32)src1, (v4i32)src0);
    vec1 = (v16u8)__msa_pckod_w((v4i32)src1, (v4i32)src0);
    dst0 = (v16u8)__msa_aver_u_b((v16u8)vec0, (v16u8)vec1);
    ST_UB(dst0, dst_argb);
    src_argb += 32;
    dst_argb += 16;
  }
}

void ScaleARGBRowDown2Box_MSA(const uint8_t* src_argb,
                              ptrdiff_t src_stride,
                              uint8_t* dst_argb,
                              int dst_width) {
  int x;
  const uint8_t* s = src_argb;
  const uint8_t* t = src_argb + src_stride;
  v16u8 src0, src1, src2, src3, vec0, vec1, vec2, vec3, dst0;
  v8u16 reg0, reg1, reg2, reg3;
  v16i8 shuffler = {0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15};

  for (x = 0; x < dst_width; x += 4) {
    src0 = (v16u8)__msa_ld_b((v16i8*)s, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)s, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)t, 0);
    src3 = (v16u8)__msa_ld_b((v16i8*)t, 16);
    vec0 = (v16u8)__msa_vshf_b(shuffler, (v16i8)src0, (v16i8)src0);
    vec1 = (v16u8)__msa_vshf_b(shuffler, (v16i8)src1, (v16i8)src1);
    vec2 = (v16u8)__msa_vshf_b(shuffler, (v16i8)src2, (v16i8)src2);
    vec3 = (v16u8)__msa_vshf_b(shuffler, (v16i8)src3, (v16i8)src3);
    reg0 = __msa_hadd_u_h(vec0, vec0);
    reg1 = __msa_hadd_u_h(vec1, vec1);
    reg2 = __msa_hadd_u_h(vec2, vec2);
    reg3 = __msa_hadd_u_h(vec3, vec3);
    reg0 += reg2;
    reg1 += reg3;
    reg0 = (v8u16)__msa_srari_h((v8i16)reg0, 2);
    reg1 = (v8u16)__msa_srari_h((v8i16)reg1, 2);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg1, (v16i8)reg0);
    ST_UB(dst0, dst_argb);
    s += 32;
    t += 32;
    dst_argb += 16;
  }
}

void ScaleARGBRowDownEven_MSA(const uint8_t* src_argb,
                              ptrdiff_t src_stride,
                              int32_t src_stepx,
                              uint8_t* dst_argb,
                              int dst_width) {
  int x;
  int32_t stepx = src_stepx * 4;
  int32_t data0, data1, data2, data3;

  for (x = 0; x < dst_width; x += 4) {
    data0 = LW(src_argb);
    data1 = LW(src_argb + stepx);
    data2 = LW(src_argb + stepx * 2);
    data3 = LW(src_argb + stepx * 3);
    SW(data0, dst_argb);
    SW(data1, dst_argb + 4);
    SW(data2, dst_argb + 8);
    SW(data3, dst_argb + 12);
    src_argb += stepx * 4;
    dst_argb += 16;
  }
}

void ScaleARGBRowDownEvenBox_MSA(const uint8* src_argb,
                                 ptrdiff_t src_stride,
                                 int src_stepx,
                                 uint8* dst_argb,
                                 int dst_width) {
  int x;
  const uint8* nxt_argb = src_argb + src_stride;
  int32_t stepx = src_stepx * 4;
  int64_t data0, data1, data2, data3;
  v16u8 src0 = {0}, src1 = {0}, src2 = {0}, src3 = {0};
  v16u8 vec0, vec1, vec2, vec3;
  v8u16 reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7;
  v16u8 dst0;

  for (x = 0; x < dst_width; x += 4) {
    data0 = LD(src_argb);
    data1 = LD(src_argb + stepx);
    data2 = LD(src_argb + stepx * 2);
    data3 = LD(src_argb + stepx * 3);
    src0 = (v16u8)__msa_insert_d((v2i64)src0, 0, data0);
    src0 = (v16u8)__msa_insert_d((v2i64)src0, 1, data1);
    src1 = (v16u8)__msa_insert_d((v2i64)src1, 0, data2);
    src1 = (v16u8)__msa_insert_d((v2i64)src1, 1, data3);
    data0 = LD(nxt_argb);
    data1 = LD(nxt_argb + stepx);
    data2 = LD(nxt_argb + stepx * 2);
    data3 = LD(nxt_argb + stepx * 3);
    src2 = (v16u8)__msa_insert_d((v2i64)src2, 0, data0);
    src2 = (v16u8)__msa_insert_d((v2i64)src2, 1, data1);
    src3 = (v16u8)__msa_insert_d((v2i64)src3, 0, data2);
    src3 = (v16u8)__msa_insert_d((v2i64)src3, 1, data3);
    vec0 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src0);
    vec1 = (v16u8)__msa_ilvr_b((v16i8)src3, (v16i8)src1);
    vec2 = (v16u8)__msa_ilvl_b((v16i8)src2, (v16i8)src0);
    vec3 = (v16u8)__msa_ilvl_b((v16i8)src3, (v16i8)src1);
    reg0 = __msa_hadd_u_h(vec0, vec0);
    reg1 = __msa_hadd_u_h(vec1, vec1);
    reg2 = __msa_hadd_u_h(vec2, vec2);
    reg3 = __msa_hadd_u_h(vec3, vec3);
    reg4 = (v8u16)__msa_pckev_d((v2i64)reg2, (v2i64)reg0);
    reg5 = (v8u16)__msa_pckev_d((v2i64)reg3, (v2i64)reg1);
    reg6 = (v8u16)__msa_pckod_d((v2i64)reg2, (v2i64)reg0);
    reg7 = (v8u16)__msa_pckod_d((v2i64)reg3, (v2i64)reg1);
    reg4 += reg6;
    reg5 += reg7;
    reg4 = (v8u16)__msa_srari_h((v8i16)reg4, 2);
    reg5 = (v8u16)__msa_srari_h((v8i16)reg5, 2);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg5, (v16i8)reg4);
    ST_UB(dst0, dst_argb);
    src_argb += stepx * 4;
    nxt_argb += stepx * 4;
    dst_argb += 16;
  }
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_MSA) && defined(__mips_msa)

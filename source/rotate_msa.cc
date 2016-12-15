/*
 *  Copyright 2016 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/rotate_row.h"

// This module is for GCC MSA
#if !defined(LIBYUV_DISABLE_MSA) && defined(__mips_msa)
#include "libyuv/macros_msa.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

void TransposeWx8_MSA(const uint8_t* src,
                      int src_stride,
                      uint8_t* dst,
                      int dst_stride,
                      int width) {
  int x;
  uint64_t val0, val1, val2, val3;
  v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
  v16u8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
  v16u8 reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7;
  v16u8 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride), 0);
    src2 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 2), 0);
    src3 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 3), 0);
    src4 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 4), 0);
    src5 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 5), 0);
    src6 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 6), 0);
    src7 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 7), 0);
    vec0 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src0);
    vec1 = (v16u8)__msa_ilvr_b((v16i8)src3, (v16i8)src1);
    vec2 = (v16u8)__msa_ilvr_b((v16i8)src6, (v16i8)src4);
    vec3 = (v16u8)__msa_ilvr_b((v16i8)src7, (v16i8)src5);
    vec4 = (v16u8)__msa_ilvl_b((v16i8)src2, (v16i8)src0);
    vec5 = (v16u8)__msa_ilvl_b((v16i8)src3, (v16i8)src1);
    vec6 = (v16u8)__msa_ilvl_b((v16i8)src6, (v16i8)src4);
    vec7 = (v16u8)__msa_ilvl_b((v16i8)src7, (v16i8)src5);
    reg0 = (v16u8)__msa_ilvr_b((v16i8)vec1, (v16i8)vec0);
    reg1 = (v16u8)__msa_ilvl_b((v16i8)vec1, (v16i8)vec0);
    reg2 = (v16u8)__msa_ilvr_b((v16i8)vec3, (v16i8)vec2);
    reg3 = (v16u8)__msa_ilvl_b((v16i8)vec3, (v16i8)vec2);
    reg4 = (v16u8)__msa_ilvr_b((v16i8)vec5, (v16i8)vec4);
    reg5 = (v16u8)__msa_ilvl_b((v16i8)vec5, (v16i8)vec4);
    reg6 = (v16u8)__msa_ilvr_b((v16i8)vec7, (v16i8)vec6);
    reg7 = (v16u8)__msa_ilvl_b((v16i8)vec7, (v16i8)vec6);
    dst0 = (v16u8)__msa_ilvr_w((v4i32)reg2, (v4i32)reg0);
    dst1 = (v16u8)__msa_ilvl_w((v4i32)reg2, (v4i32)reg0);
    dst2 = (v16u8)__msa_ilvr_w((v4i32)reg3, (v4i32)reg1);
    dst3 = (v16u8)__msa_ilvl_w((v4i32)reg3, (v4i32)reg1);
    dst4 = (v16u8)__msa_ilvr_w((v4i32)reg6, (v4i32)reg4);
    dst5 = (v16u8)__msa_ilvl_w((v4i32)reg6, (v4i32)reg4);
    dst6 = (v16u8)__msa_ilvr_w((v4i32)reg7, (v4i32)reg5);
    dst7 = (v16u8)__msa_ilvl_w((v4i32)reg7, (v4i32)reg5);
    val0 = __msa_copy_s_d((v2i64)dst0, 0);
    val1 = __msa_copy_s_d((v2i64)dst0, 1);
    val2 = __msa_copy_s_d((v2i64)dst1, 0);
    val3 = __msa_copy_s_d((v2i64)dst1, 1);
    SD(val0, dst);
    SD(val1, dst + dst_stride);
    SD(val2, dst + dst_stride * 2);
    SD(val3, dst + dst_stride * 3);
    dst += dst_stride * 4;
    val0 = __msa_copy_s_d((v2i64)dst2, 0);
    val1 = __msa_copy_s_d((v2i64)dst2, 1);
    val2 = __msa_copy_s_d((v2i64)dst3, 0);
    val3 = __msa_copy_s_d((v2i64)dst3, 1);
    SD(val0, dst);
    SD(val1, dst + dst_stride);
    SD(val2, dst + dst_stride * 2);
    SD(val3, dst + dst_stride * 3);
    dst += dst_stride * 4;
    val0 = __msa_copy_s_d((v2i64)dst4, 0);
    val1 = __msa_copy_s_d((v2i64)dst4, 1);
    val2 = __msa_copy_s_d((v2i64)dst5, 0);
    val3 = __msa_copy_s_d((v2i64)dst5, 1);
    SD(val0, dst);
    SD(val1, dst + dst_stride);
    SD(val2, dst + dst_stride * 2);
    SD(val3, dst + dst_stride * 3);
    dst += dst_stride * 4;
    val0 = __msa_copy_s_d((v2i64)dst6, 0);
    val1 = __msa_copy_s_d((v2i64)dst6, 1);
    val2 = __msa_copy_s_d((v2i64)dst7, 0);
    val3 = __msa_copy_s_d((v2i64)dst7, 1);
    SD(val0, dst);
    SD(val1, dst + dst_stride);
    SD(val2, dst + dst_stride * 2);
    SD(val3, dst + dst_stride * 3);
    dst += dst_stride * 4;
    src += 16;
  }
}

void TransposeUVWx8_MSA(const uint8_t* src,
                        int src_stride,
                        uint8_t* dst_a,
                        int dst_stride_a,
                        uint8_t* dst_b,
                        int dst_stride_b,
                        int width) {
  int x;
  uint64_t val0, val1, val2, val3;
  v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
  v16u8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
  v16u8 reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7;
  v16u8 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride), 0);
    src2 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 2), 0);
    src3 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 3), 0);
    src4 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 4), 0);
    src5 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 5), 0);
    src6 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 6), 0);
    src7 = (v16u8)__msa_ld_b((v16i8*)(src + src_stride * 7), 0);
    vec0 = (v16u8)__msa_ilvr_b((v16i8)src1, (v16i8)src0);
    vec1 = (v16u8)__msa_ilvr_b((v16i8)src3, (v16i8)src2);
    vec2 = (v16u8)__msa_ilvr_b((v16i8)src5, (v16i8)src4);
    vec3 = (v16u8)__msa_ilvr_b((v16i8)src7, (v16i8)src6);
    vec4 = (v16u8)__msa_ilvl_b((v16i8)src1, (v16i8)src0);
    vec5 = (v16u8)__msa_ilvl_b((v16i8)src3, (v16i8)src2);
    vec6 = (v16u8)__msa_ilvl_b((v16i8)src5, (v16i8)src4);
    vec7 = (v16u8)__msa_ilvl_b((v16i8)src7, (v16i8)src6);
    reg0 = (v16u8)__msa_ilvr_h((v8i16)vec1, (v8i16)vec0);
    reg1 = (v16u8)__msa_ilvr_h((v8i16)vec3, (v8i16)vec2);
    reg2 = (v16u8)__msa_ilvl_h((v8i16)vec1, (v8i16)vec0);
    reg3 = (v16u8)__msa_ilvl_h((v8i16)vec3, (v8i16)vec2);
    reg4 = (v16u8)__msa_ilvr_h((v8i16)vec5, (v8i16)vec4);
    reg5 = (v16u8)__msa_ilvr_h((v8i16)vec7, (v8i16)vec6);
    reg6 = (v16u8)__msa_ilvl_h((v8i16)vec5, (v8i16)vec5);
    reg7 = (v16u8)__msa_ilvl_h((v8i16)vec7, (v8i16)vec6);
    dst0 = (v16u8)__msa_ilvr_w((v4i32)reg1, (v4i32)reg0);
    dst1 = (v16u8)__msa_ilvl_w((v4i32)reg1, (v4i32)reg0);
    dst2 = (v16u8)__msa_ilvr_w((v4i32)reg3, (v4i32)reg2);
    dst3 = (v16u8)__msa_ilvl_w((v4i32)reg3, (v4i32)reg2);
    dst4 = (v16u8)__msa_ilvr_w((v4i32)reg5, (v4i32)reg4);
    dst5 = (v16u8)__msa_ilvl_w((v4i32)reg5, (v4i32)reg4);
    dst6 = (v16u8)__msa_ilvr_w((v4i32)reg7, (v4i32)reg6);
    dst7 = (v16u8)__msa_ilvl_w((v4i32)reg7, (v4i32)reg6);
    val0 = __msa_copy_s_d((v2i64)dst0, 0);
    val1 = __msa_copy_s_d((v2i64)dst0, 1);
    val2 = __msa_copy_s_d((v2i64)dst1, 0);
    val3 = __msa_copy_s_d((v2i64)dst1, 1);
    SD(val0, dst_a);
    SD(val2, dst_a + dst_stride_a);
    SD(val1, dst_b);
    SD(val3, dst_b + dst_stride_b);
    dst_a += dst_stride_a * 2;
    dst_b += dst_stride_b * 2;
    val0 = __msa_copy_s_d((v2i64)dst2, 0);
    val1 = __msa_copy_s_d((v2i64)dst2, 1);
    val2 = __msa_copy_s_d((v2i64)dst3, 0);
    val3 = __msa_copy_s_d((v2i64)dst3, 1);
    SD(val0, dst_a);
    SD(val2, dst_a + dst_stride_a);
    SD(val1, dst_b);
    SD(val3, dst_b + dst_stride_b);
    dst_a += dst_stride_a * 2;
    dst_b += dst_stride_b * 2;
    val0 = __msa_copy_s_d((v2i64)dst4, 0);
    val1 = __msa_copy_s_d((v2i64)dst4, 1);
    val2 = __msa_copy_s_d((v2i64)dst5, 0);
    val3 = __msa_copy_s_d((v2i64)dst5, 1);
    SD(val0, dst_a);
    SD(val2, dst_a + dst_stride_a);
    SD(val1, dst_b);
    SD(val3, dst_b + dst_stride_b);
    dst_a += dst_stride_a * 2;
    dst_b += dst_stride_b * 2;
    val0 = __msa_copy_s_d((v2i64)dst6, 0);
    val1 = __msa_copy_s_d((v2i64)dst6, 1);
    val2 = __msa_copy_s_d((v2i64)dst7, 0);
    val3 = __msa_copy_s_d((v2i64)dst7, 1);
    SD(val0, dst_a);
    SD(val2, dst_a + dst_stride_a);
    SD(val1, dst_b);
    SD(val3, dst_b + dst_stride_b);
    dst_a += dst_stride_a * 2;
    dst_b += dst_stride_b * 2;
    src += 16;
  }
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_MSA) && defined(__mips_msa)

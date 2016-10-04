/*
 *  Copyright 2016 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"

// This module is for GCC MSA
#if !defined(LIBYUV_DISABLE_MSA) && defined(__mips_msa)
#include "libyuv/macros_msa.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

void MirrorRow_MSA(const uint8* src, uint8* dst, int width) {
  int x;
  v16u8 src0, src1, src2, src3;
  v16u8 dst0, dst1, dst2, dst3;
  v16i8 shuffler = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
  src += width - 64;

  for (x = 0; x < width; x += 64) {
    LD_UB4(src, 16, src3, src2, src1, src0);
    VSHF_B2_UB(src3, src3, src2, src2, shuffler, shuffler, dst3, dst2);
    VSHF_B2_UB(src1, src1, src0, src0, shuffler, shuffler, dst1, dst0);
    ST_UB4(dst0, dst1, dst2, dst3, dst, 16);
    dst += 64;
    src -= 64;
  }
}

void ARGBMirrorRow_MSA(const uint8* src, uint8* dst, int width) {
  int x;
  v16u8 src0, src1, src2, src3;
  v16u8 dst0, dst1, dst2, dst3;
  v16i8 shuffler = { 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 };
  src += width * 4 - 64;

  for (x = 0; x < width; x += 16) {
    LD_UB4(src, 16, src3, src2, src1, src0);
    VSHF_B2_UB(src3, src3, src2, src2, shuffler, shuffler, dst3, dst2);
    VSHF_B2_UB(src1, src1, src0, src0, shuffler, shuffler, dst1, dst0);
    ST_UB4(dst0, dst1, dst2, dst3, dst, 16);
    dst += 64;
    src -= 64;
  }
}

void I422ToYUY2Row_MSA(const uint8* src_y,
                       const uint8* src_u,
                       const uint8* src_v,
                       uint8* dst_yuy2,
                       int width) {
  int x;
  v16u8 src_u0, src_v0, src_y0, src_y1, vec_uv0, vec_uv1;
  v16u8 dst_yuy2_0, dst_yuy2_1, dst_yuy2_2, dst_yuy2_3;

  for (x = 0; x < width; x += 32) {
    src_u0 = LD_UB(src_u);
    src_v0 = LD_UB(src_v);
    LD_UB2(src_y, 16, src_y0, src_y1);
    ILVRL_B2_UB(src_v0, src_u0, vec_uv0, vec_uv1);
    ILVRL_B2_UB(vec_uv0, src_y0, dst_yuy2_0, dst_yuy2_1);
    ILVRL_B2_UB(vec_uv1, src_y1, dst_yuy2_2, dst_yuy2_3);
    ST_UB4(dst_yuy2_0, dst_yuy2_1, dst_yuy2_2, dst_yuy2_3, dst_yuy2, 16);
    src_u += 16;
    src_v += 16;
    src_y += 32;
    dst_yuy2 += 64;
  }
}

void I422ToUYVYRow_MSA(const uint8* src_y,
                       const uint8* src_u,
                       const uint8* src_v,
                       uint8* dst_uyvy,
                       int width) {
  int x;
  v16u8 src_u0, src_v0, src_y0, src_y1, vec_uv0, vec_uv1;
  v16u8 dst_uyvy0, dst_uyvy1, dst_uyvy2, dst_uyvy3;

  for (x = 0; x < width; x += 32) {
    src_u0 = LD_UB(src_u);
    src_v0 = LD_UB(src_v);
    LD_UB2(src_y, 16, src_y0, src_y1);
    ILVRL_B2_UB(src_v0, src_u0, vec_uv0, vec_uv1);
    ILVRL_B2_UB(src_y0, vec_uv0, dst_uyvy0, dst_uyvy1);
    ILVRL_B2_UB(src_y1, vec_uv1, dst_uyvy2, dst_uyvy3);
    ST_UB4(dst_uyvy0, dst_uyvy1, dst_uyvy2, dst_uyvy3, dst_uyvy, 16);
    src_u += 16;
    src_v += 16;
    src_y += 32;
    dst_uyvy += 64;
  }
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_MSA) && defined(__mips_msa)

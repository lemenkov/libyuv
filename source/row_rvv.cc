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
 *
 */

#include <assert.h>

#include "libyuv/row.h"

#if !defined(LIBYUV_DISABLE_RVV) && defined(__riscv)
#include <riscv_vector.h>

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

void RAWToARGBRow_RVV(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  size_t vl = __riscv_vsetvl_e8m2(width);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    vuint8m2_t v_b, v_g, v_r;
    __riscv_vlseg3e8_v_u8m2(&v_r, &v_g, &v_b, src_raw, vl);
    __riscv_vsseg4e8_v_u8m2(dst_argb, v_b, v_g, v_r, v_a, vl);
    width -= vl;
    src_raw += (3 * vl);
    dst_argb += (4 * vl);
    vl = __riscv_vsetvl_e8m2(width);
  } while (width > 0);
}

void RAWToRGBARow_RVV(const uint8_t* src_raw, uint8_t* dst_rgba, int width) {
  size_t vl = __riscv_vsetvl_e8m2(width);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    vuint8m2_t v_b, v_g, v_r;
    __riscv_vlseg3e8_v_u8m2(&v_r, &v_g, &v_b, src_raw, vl);
    __riscv_vsseg4e8_v_u8m2(dst_rgba, v_a, v_b, v_g, v_r, vl);
    width -= vl;
    src_raw += (3 * vl);
    dst_rgba += (4 * vl);
    vl = __riscv_vsetvl_e8m2(width);
  } while (width > 0);
}

void RAWToRGB24Row_RVV(const uint8_t* src_raw, uint8_t* dst_rgb24, int width) {
  do {
    vuint8m2_t v_b, v_g, v_r;
    size_t vl = __riscv_vsetvl_e8m2(width);
    __riscv_vlseg3e8_v_u8m2(&v_b, &v_g, &v_r, src_raw, vl);
    __riscv_vsseg3e8_v_u8m2(dst_rgb24, v_r, v_g, v_b, vl);
    width -= vl;
    src_raw += (3 * vl);
    dst_rgb24 += (3 * vl);
  } while (width > 0);
}

void ARGBToRAWRow_RVV(const uint8_t* src_argb, uint8_t* dst_raw, int width) {
  do {
    vuint8m2_t v_b, v_g, v_r, v_a;
    size_t vl = __riscv_vsetvl_e8m2(width);
    __riscv_vlseg4e8_v_u8m2(&v_b, &v_g, &v_r, &v_a, src_argb, vl);
    __riscv_vsseg3e8_v_u8m2(dst_raw, v_r, v_g, v_b, vl);
    width -= vl;
    src_argb += (4 * vl);
    dst_raw += (3 * vl);
  } while (width > 0);
}

void ARGBToRGB24Row_RVV(const uint8_t* src_argb,
                        uint8_t* dst_rgb24,
                        int width) {
  do {
    vuint8m2_t v_b, v_g, v_r, v_a;
    size_t vl = __riscv_vsetvl_e8m2(width);
    __riscv_vlseg4e8_v_u8m2(&v_b, &v_g, &v_r, &v_a, src_argb, vl);
    __riscv_vsseg3e8_v_u8m2(dst_rgb24, v_b, v_g, v_r, vl);
    width -= vl;
    src_argb += (4 * vl);
    dst_rgb24 += (3 * vl);
  } while (width > 0);
}

void RGB24ToARGBRow_RVV(const uint8_t* src_rgb24,
                        uint8_t* dst_argb,
                        int width) {
  size_t vl = __riscv_vsetvl_e8m2(width);
  vuint8m2_t v_a = __riscv_vmv_v_x_u8m2(255u, vl);
  do {
    vuint8m2_t v_b, v_g, v_r;
    __riscv_vlseg3e8_v_u8m2(&v_b, &v_g, &v_r, src_rgb24, vl);
    __riscv_vsseg4e8_v_u8m2(dst_argb, v_b, v_g, v_r, v_a, vl);
    width -= vl;
    src_rgb24 += (3 * vl);
    dst_argb += (4 * vl);
    vl = __riscv_vsetvl_e8m2(width);
  } while (width > 0);
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_RVV) && defined(__riscv)

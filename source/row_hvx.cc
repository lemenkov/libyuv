/*
 *  Copyright 2026 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"

#if !defined(LIBYUV_DISABLE_HVX) && defined(__HVX__)
#include <hexagon_protos.h>
#include <hexagon_types.h>
#include <hvx_hexagon_protos.h>
#include <string.h>

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

static __inline HVX_Vector vloada(const void* ptr) {
  HVX_Vector v;
  memcpy(&v, (const HVX_Vector*)((uintptr_t)ptr & ~(uintptr_t)127), sizeof(v));
  return v;
}

static __inline HVX_UVector vloadu(const void* ptr) {
  HVX_UVector v;
  memcpy(&v, ptr, sizeof(v));
  return v;
}

static __inline void vstoreu(void* ptr, HVX_UVector v) {
  memcpy(ptr, &v, sizeof(v));
}

static __inline void vstoreu_partial(void* dst, HVX_Vector v, int n) {
  if (n >= 128) {
    vstoreu(dst, v);
  } else if (n > 0) {
    const uintptr_t addr = (uintptr_t)dst;
    const uintptr_t offset = addr & 127;
    HVX_Vector value = Q6_V_vlalign_VVR(v, v, addr);
    HVX_VectorPred ql_not = Q6_Q_vsetq_R(addr);
    HVX_VectorPred qr = Q6_Q_vsetq2_R(offset + (uintptr_t)n);
    if (offset + (uintptr_t)n > 128) {
      Q6_vmem_QRIV(qr, (uint8_t*)dst + 128, value);
      qr = Q6_Q_vcmp_eq_VbVb(value, value);
    }
    ql_not = Q6_Q_or_QQn(ql_not, qr);
    Q6_vmem_QnRIV(ql_not, dst, value);
  }
}

static __inline HVX_Vector vloadu_partial_zero(const void* src, int n) {
  if (n >= 128) {
    return vloadu(src);
  }
  if (n <= 0) {
    return Q6_V_vzero();
  }
  const uintptr_t addr = (uintptr_t)src;
  const uintptr_t offset = addr & 127;
  HVX_Vector v0 = vloada(src);
  HVX_Vector v1 =
      (offset + (uintptr_t)n > 128) ? vloada((const uint8_t*)src + 128) : v0;
  HVX_Vector v = Q6_V_valign_VVR(v1, v0, addr);
  return Q6_V_vmux_QVV(Q6_Q_vsetq_R(n), v, Q6_V_vzero());
}

void CopyRow_HVX(const uint8_t* src, uint8_t* dst, int count) {
  while (count >= 128) {
    vstoreu(dst, vloadu(src));
    src += 128;
    dst += 128;
    count -= 128;
  }
  if (count > 0) {
    vstoreu_partial(dst, vloadu_partial_zero(src, count), count);
  }
}

void CopyRow_16_HVX(const uint16_t* src, uint16_t* dst, int count) {
  CopyRow_HVX((const uint8_t*)src, (uint8_t*)dst, count * 2);
}

void SetRow_HVX(uint8_t* dst, uint8_t v8, int width) {
  HVX_Vector v = Q6_Vb_vsplat_R(v8);
  while (width >= 128) {
    vstoreu(dst, v);
    dst += 128;
    width -= 128;
  }
  if (width > 0) {
    vstoreu_partial(dst, v, width);
  }
}

void ARGBSetRow_HVX(uint8_t* dst_argb, uint32_t v32, int width) {
  HVX_Vector v = Q6_V_vsplat_R(v32);
  int bytes = width * 4;
  while (bytes >= 128) {
    vstoreu(dst_argb, v);
    dst_argb += 128;
    bytes -= 128;
  }
  if (bytes > 0) {
    vstoreu_partial(dst_argb, v, bytes);
  }
}

void ARGBOrRow_HVX(const uint8_t* src_argb,
                   uint8_t* dst_argb,
                   uint32_t value,
                   int width) {
  HVX_Vector v = Q6_V_vsplat_R(value);
  int bytes = width * 4;
  while (bytes >= 128) {
    HVX_Vector s = vloadu(src_argb);
    vstoreu(dst_argb, Q6_V_vor_VV(s, v));
    src_argb += 128;
    dst_argb += 128;
    bytes -= 128;
  }
  if (bytes > 0) {
    HVX_Vector s = vloadu_partial_zero(src_argb, bytes);
    vstoreu_partial(dst_argb, Q6_V_vor_VV(s, v), bytes);
  }
}

void MirrorRow_HVX(const uint8_t* src, uint8_t* dst, int width) {
  HVX_Vector ctrl = Q6_Vb_vsplat_R(0x7F);
  src += width;
  while (width >= 128) {
    src -= 128;
    HVX_Vector v = vloadu(src);
    v = Q6_V_vdelta_VV(v, ctrl);
    vstoreu(dst, v);
    dst += 128;
    width -= 128;
  }
  if (width > 0) {
    src -= width;
    HVX_Vector v = vloadu_partial_zero(src, width);
    v = Q6_V_vdelta_VV(v, ctrl);
    v = Q6_V_valign_VVR(v, v, 128 - width);
    vstoreu_partial(dst, v, width);
  }
}

void MirrorRow_16_HVX(const uint16_t* src, uint16_t* dst, int width) {
  HVX_Vector ctrl = Q6_Vb_vsplat_R(0x7E);
  const uint8_t* s = (const uint8_t*)(src + width);
  uint8_t* d = (uint8_t*)dst;
  int bytes = width * 2;
  while (bytes >= 128) {
    s -= 128;
    HVX_Vector v = vloadu(s);
    v = Q6_V_vdelta_VV(v, ctrl);
    vstoreu(d, v);
    d += 128;
    bytes -= 128;
  }
  if (bytes > 0) {
    s -= bytes;
    HVX_Vector v = vloadu_partial_zero(s, bytes);
    v = Q6_V_vdelta_VV(v, ctrl);
    v = Q6_V_valign_VVR(v, v, 128 - bytes);
    vstoreu_partial(d, v, bytes);
  }
}

void MirrorUVRow_HVX(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  MirrorRow_16_HVX((const uint16_t*)src_uv, (uint16_t*)dst_uv, width);
}

void MirrorSplitUVRow_HVX(const uint8_t* src_uv,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width) {
  HVX_Vector ctrl = Q6_Vb_vsplat_R(0x7F);
  src_uv += width * 2;
  while (width >= 128) {
    src_uv -= 256;
    HVX_Vector s0 = vloadu(src_uv);
    HVX_Vector s1 = vloadu(src_uv + 128);
    HVX_VectorPair uv = Q6_W_vdeal_VVR(s1, s0, -1);
    HVX_Vector u = Q6_V_vdelta_VV(Q6_V_lo_W(uv), ctrl);
    HVX_Vector v = Q6_V_vdelta_VV(Q6_V_hi_W(uv), ctrl);
    vstoreu(dst_u, u);
    vstoreu(dst_v, v);
    dst_u += 128;
    dst_v += 128;
    width -= 128;
  }
  if (width > 0) {
    int src_bytes = width * 2;
    src_uv -= src_bytes;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uv, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_uv + 128, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair uv = Q6_W_vdeal_VVR(s1, s0, -1);
    HVX_Vector u = Q6_V_vdelta_VV(Q6_V_lo_W(uv), ctrl);
    HVX_Vector v = Q6_V_vdelta_VV(Q6_V_hi_W(uv), ctrl);
    u = Q6_V_valign_VVR(u, u, 128 - width);
    v = Q6_V_valign_VVR(v, v, 128 - width);
    vstoreu_partial(dst_u, u, width);
    vstoreu_partial(dst_v, v, width);
  }
}

void ARGBMirrorRow_HVX(const uint8_t* src, uint8_t* dst, int width) {
  HVX_Vector ctrl = Q6_Vb_vsplat_R(0x7C);
  int bytes = width * 4;
  src += bytes;
  while (bytes >= 128) {
    src -= 128;
    HVX_Vector v = vloadu(src);
    v = Q6_V_vdelta_VV(v, ctrl);
    vstoreu(dst, v);
    dst += 128;
    bytes -= 128;
  }
  if (bytes > 0) {
    src -= bytes;
    HVX_Vector v = vloadu_partial_zero(src, bytes);
    v = Q6_V_vdelta_VV(v, ctrl);
    v = Q6_V_valign_VVR(v, v, 128 - bytes);
    vstoreu_partial(dst, v, bytes);
  }
}

void SplitUVRow_HVX(const uint8_t* src_uv,
                    uint8_t* dst_u,
                    uint8_t* dst_v,
                    int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 2;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uv, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_uv + 128, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair uv = Q6_W_vdeal_VVR(s1, s0, -1);
    vstoreu_partial(dst_u, Q6_V_lo_W(uv), w);
    vstoreu_partial(dst_v, Q6_V_hi_W(uv), w);
    src_uv += 256;
    dst_u += 128;
    dst_v += 128;
    width -= 128;
  }
}

void MergeUVRow_HVX(const uint8_t* src_u,
                    const uint8_t* src_v,
                    uint8_t* dst_uv,
                    int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector u = vloadu_partial_zero(src_u, w);
    HVX_Vector v = vloadu_partial_zero(src_v, w);
    HVX_VectorPair uv = Q6_W_vshuff_VVR(v, u, -1);
    int dst_bytes = w * 2;
    vstoreu_partial(dst_uv, Q6_V_lo_W(uv), dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_uv + 128, Q6_V_hi_W(uv), dst_bytes - 128);
    }
    src_u += 128;
    src_v += 128;
    dst_uv += 256;
    width -= 128;
  }
}

void HalfRow_HVX(const uint8_t* src_uv,
                 ptrdiff_t src_uv_stride,
                 uint8_t* dst_uv,
                 int width) {
  const uint8_t* src_uv1 = src_uv + src_uv_stride;
  while (width >= 128) {
    HVX_Vector s0 = vloadu(src_uv);
    HVX_Vector s1 = vloadu(src_uv1);
    vstoreu(dst_uv, Q6_Vub_vavg_VubVub_rnd(s0, s1));
    src_uv += 128;
    src_uv1 += 128;
    dst_uv += 128;
    width -= 128;
  }
  if (width > 0) {
    HVX_Vector s0 = vloadu_partial_zero(src_uv, width);
    HVX_Vector s1 = vloadu_partial_zero(src_uv1, width);
    vstoreu_partial(dst_uv, Q6_Vub_vavg_VubVub_rnd(s0, s1), width);
  }
}

void HalfRow_16_HVX(const uint16_t* src_uv,
                    ptrdiff_t src_uv_stride,
                    uint16_t* dst_uv,
                    int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  while (width >= 64) {
    HVX_Vector s0 = vloadu(src_uv);
    HVX_Vector s1 = vloadu(src_uv1);
    vstoreu(dst_uv, Q6_Vuh_vavg_VuhVuh_rnd(s0, s1));
    src_uv += 64;
    src_uv1 += 64;
    dst_uv += 64;
    width -= 64;
  }
  if (width > 0) {
    int bytes = width * 2;
    HVX_Vector s0 = vloadu_partial_zero(src_uv, bytes);
    HVX_Vector s1 = vloadu_partial_zero(src_uv1, bytes);
    vstoreu_partial(dst_uv, Q6_Vuh_vavg_VuhVuh_rnd(s0, s1), bytes);
  }
}

void HalfWidthRow_HVX(const uint8_t* src_ptr, uint8_t* dst_ptr, int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 2;
    HVX_Vector s0 =
        vloadu_partial_zero(src_ptr, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_ptr + 128, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair d = Q6_W_vdeal_VVR(s1, s0, -1);
    HVX_Vector avg = Q6_Vub_vavg_VubVub_rnd(Q6_V_lo_W(d), Q6_V_hi_W(d));
    vstoreu_partial(dst_ptr, avg, w);
    src_ptr += 256;
    dst_ptr += 128;
    width -= 128;
  }
}

void HalfWidthRow_Odd_HVX(const uint8_t* src_ptr, uint8_t* dst_ptr, int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthRow_HVX(src_ptr, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  dst_ptr[0] = src_ptr[0];
}

void HalfWidthRow_16_HVX(const uint16_t* src_ptr,
                         uint16_t* dst_ptr,
                         int width) {
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_ptr, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_ptr + 64, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair d = Q6_W_vdeal_VVR(s1, s0, -2);
    HVX_Vector avg = Q6_Vuh_vavg_VuhVuh_rnd(Q6_V_lo_W(d), Q6_V_hi_W(d));
    vstoreu_partial(dst_ptr, avg, w * 2);
    src_ptr += 128;
    dst_ptr += 64;
    width -= 64;
  }
}

void HalfWidthRow_16_Odd_HVX(const uint16_t* src_ptr,
                             uint16_t* dst_ptr,
                             int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthRow_16_HVX(src_ptr, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  dst_ptr[0] = src_ptr[0];
}

void HalfWidthUVRow_HVX(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uv, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_uv + 128, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair d = Q6_W_vdeal_VVR(s1, s0, -2);
    HVX_Vector avg = Q6_Vub_vavg_VubVub_rnd(Q6_V_lo_W(d), Q6_V_hi_W(d));
    vstoreu_partial(dst_uv, avg, w * 2);
    src_uv += 256;
    dst_uv += 128;
    width -= 64;
  }
}

void HalfWidthUVRow_Odd_HVX(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthUVRow_HVX(src_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  dst_uv[0] = src_uv[0];
  dst_uv[1] = src_uv[1];
}

void HalfWidthUVRow_16_HVX(const uint16_t* src_uv,
                           uint16_t* dst_uv,
                           int width) {
  while (width > 0) {
    int w = width >= 32 ? 32 : width;
    int src_bytes = w * 8;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uv, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_uv + 64, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair d = Q6_W_vdeal_VVR(s1, s0, -4);
    HVX_Vector avg = Q6_Vuh_vavg_VuhVuh_rnd(Q6_V_lo_W(d), Q6_V_hi_W(d));
    vstoreu_partial(dst_uv, avg, w * 4);
    src_uv += 128;
    dst_uv += 64;
    width -= 32;
  }
}

void HalfWidthUVRow_16_Odd_HVX(const uint16_t* src_uv,
                               uint16_t* dst_uv,
                               int width) {
  width -= 1;
  if (width > 0) {
    HalfWidthUVRow_16_HVX(src_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  dst_uv[0] = src_uv[0];
  dst_uv[1] = src_uv[1];
}

void HalfBoxRow_HVX(const uint8_t* src_ptr,
                    ptrdiff_t src_stride,
                    uint8_t* dst_ptr,
                    int width) {
  const uint8_t* s = src_ptr;
  const uint8_t* t = src_ptr + src_stride;
  const int32_t kOnes = 0x01010101;
  HVX_Vector vtwo = Q6_Vh_vsplat_R(2);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 2;
    HVX_Vector s0 = vloadu_partial_zero(s, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector t0 = vloadu_partial_zero(t, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector sum0 = Q6_Vh_vdmpy_VubRb(s0, kOnes);
    sum0 = Q6_Vh_vdmpyacc_VhVubRb(sum0, t0, kOnes);
    sum0 = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sum0, vtwo), 2);

    HVX_Vector sum1 = Q6_V_vzero();
    if (src_bytes > 128) {
      HVX_Vector s1 = vloadu_partial_zero(s + 128, src_bytes - 128);
      HVX_Vector t1 = vloadu_partial_zero(t + 128, src_bytes - 128);
      sum1 = Q6_Vh_vdmpy_VubRb(s1, kOnes);
      sum1 = Q6_Vh_vdmpyacc_VhVubRb(sum1, t1, kOnes);
      sum1 = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sum1, vtwo), 2);
    }
    HVX_Vector out = Q6_Vub_vpack_VhVh_sat(sum1, sum0);
    vstoreu_partial(dst_ptr, out, w);
    s += 256;
    t += 256;
    dst_ptr += 128;
    width -= 128;
  }
}

void HalfBoxRow_Odd_HVX(const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        uint8_t* dst_ptr,
                        int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxRow_HVX(src_ptr, src_stride, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  const uint8_t* t = src_ptr + src_stride;
  dst_ptr[0] = (uint8_t)((src_ptr[0] + t[0] + 1) >> 1);
}

void HalfBoxRow_16_HVX(const uint16_t* src_ptr,
                       ptrdiff_t src_stride,
                       uint16_t* dst_ptr,
                       int width) {
  const uint16_t* s = src_ptr;
  const uint16_t* t = src_ptr + src_stride;
  HVX_Vector vtwo = Q6_V_vsplat_R(2);
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 = vloadu_partial_zero(s, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector t0 = vloadu_partial_zero(t, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(s + 64, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_Vector t1 = src_bytes > 128
                        ? vloadu_partial_zero(t + 64, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair ds = Q6_W_vdeal_VVR(s1, s0, -2);
    HVX_VectorPair dt = Q6_W_vdeal_VVR(t1, t0, -2);
    HVX_VectorPair s_even = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(ds));
    HVX_VectorPair s_odd = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(ds));
    HVX_VectorPair t_even = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(dt));
    HVX_VectorPair t_odd = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(dt));

    HVX_Vector sum_lo = Q6_Vw_vadd_VwVw(Q6_V_lo_W(s_even), Q6_V_lo_W(s_odd));
    sum_lo = Q6_Vw_vadd_VwVw(sum_lo, Q6_V_lo_W(t_even));
    sum_lo = Q6_Vw_vadd_VwVw(sum_lo, Q6_V_lo_W(t_odd));
    sum_lo = Q6_Vw_vasr_VwR(Q6_Vw_vadd_VwVw(sum_lo, vtwo), 2);

    HVX_Vector sum_hi = Q6_Vw_vadd_VwVw(Q6_V_hi_W(s_even), Q6_V_hi_W(s_odd));
    sum_hi = Q6_Vw_vadd_VwVw(sum_hi, Q6_V_hi_W(t_even));
    sum_hi = Q6_Vw_vadd_VwVw(sum_hi, Q6_V_hi_W(t_odd));
    sum_hi = Q6_Vw_vasr_VwR(Q6_Vw_vadd_VwVw(sum_hi, vtwo), 2);

    HVX_Vector out = Q6_Vuh_vpack_VwVw_sat(sum_hi, sum_lo);
    vstoreu_partial(dst_ptr, out, w * 2);
    s += 128;
    t += 128;
    dst_ptr += 64;
    width -= 64;
  }
}

void HalfBoxRow_16_Odd_HVX(const uint16_t* src_ptr,
                           ptrdiff_t src_stride,
                           uint16_t* dst_ptr,
                           int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxRow_16_HVX(src_ptr, src_stride, dst_ptr, width);
    src_ptr += (ptrdiff_t)width * 2;
    dst_ptr += width;
  }
  const uint16_t* t = src_ptr + src_stride;
  dst_ptr[0] = (uint16_t)(((uint32_t)src_ptr[0] + (uint32_t)t[0] + 1u) >> 1);
}

void HalfBoxUVRow_HVX(const uint8_t* src_uv,
                      ptrdiff_t src_stride_uv,
                      uint8_t* dst_uv,
                      int width) {
  const uint8_t* s = src_uv;
  const uint8_t* t = src_uv + src_stride_uv;
  HVX_Vector vtwo = Q6_Vh_vsplat_R(2);
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 = vloadu_partial_zero(s, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector t0 = vloadu_partial_zero(t, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(s + 128, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_Vector t1 = src_bytes > 128
                        ? vloadu_partial_zero(t + 128, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair ds = Q6_W_vdeal_VVR(s1, s0, -2);
    HVX_VectorPair dt = Q6_W_vdeal_VVR(t1, t0, -2);
    HVX_VectorPair s_even = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(ds));
    HVX_VectorPair s_odd = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(ds));
    HVX_VectorPair t_even = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(dt));
    HVX_VectorPair t_odd = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(dt));

    HVX_Vector sum_lo = Q6_Vh_vadd_VhVh(Q6_V_lo_W(s_even), Q6_V_lo_W(s_odd));
    sum_lo = Q6_Vh_vadd_VhVh(sum_lo, Q6_V_lo_W(t_even));
    sum_lo = Q6_Vh_vadd_VhVh(sum_lo, Q6_V_lo_W(t_odd));
    sum_lo = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sum_lo, vtwo), 2);

    HVX_Vector sum_hi = Q6_Vh_vadd_VhVh(Q6_V_hi_W(s_even), Q6_V_hi_W(s_odd));
    sum_hi = Q6_Vh_vadd_VhVh(sum_hi, Q6_V_hi_W(t_even));
    sum_hi = Q6_Vh_vadd_VhVh(sum_hi, Q6_V_hi_W(t_odd));
    sum_hi = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sum_hi, vtwo), 2);

    HVX_Vector out = Q6_Vub_vpack_VhVh_sat(sum_hi, sum_lo);
    vstoreu_partial(dst_uv, out, w * 2);
    s += 256;
    t += 256;
    dst_uv += 128;
    width -= 64;
  }
}

void HalfBoxUVRow_Odd_HVX(const uint8_t* src_uv,
                          ptrdiff_t src_stride_uv,
                          uint8_t* dst_uv,
                          int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxUVRow_HVX(src_uv, src_stride_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  const uint8_t* t_uv = src_uv + src_stride_uv;
  dst_uv[0] = (uint8_t)((src_uv[0] + t_uv[0] + 1) >> 1);
  dst_uv[1] = (uint8_t)((src_uv[1] + t_uv[1] + 1) >> 1);
}

void HalfBoxUVRow_16_HVX(const uint16_t* src_uv,
                         ptrdiff_t src_stride_uv,
                         uint16_t* dst_uv,
                         int width) {
  const uint16_t* s = src_uv;
  const uint16_t* t = src_uv + src_stride_uv;
  HVX_Vector vtwo = Q6_V_vsplat_R(2);
  while (width > 0) {
    int w = width >= 32 ? 32 : width;
    int src_bytes = w * 8;
    HVX_Vector s0 = vloadu_partial_zero(s, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector t0 = vloadu_partial_zero(t, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(s + 64, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_Vector t1 = src_bytes > 128
                        ? vloadu_partial_zero(t + 64, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_VectorPair ds = Q6_W_vdeal_VVR(s1, s0, -4);
    HVX_VectorPair dt = Q6_W_vdeal_VVR(t1, t0, -4);
    HVX_VectorPair s_even = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(ds));
    HVX_VectorPair s_odd = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(ds));
    HVX_VectorPair t_even = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(dt));
    HVX_VectorPair t_odd = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(dt));

    HVX_Vector sum_lo = Q6_Vw_vadd_VwVw(Q6_V_lo_W(s_even), Q6_V_lo_W(s_odd));
    sum_lo = Q6_Vw_vadd_VwVw(sum_lo, Q6_V_lo_W(t_even));
    sum_lo = Q6_Vw_vadd_VwVw(sum_lo, Q6_V_lo_W(t_odd));
    sum_lo = Q6_Vw_vasr_VwR(Q6_Vw_vadd_VwVw(sum_lo, vtwo), 2);

    HVX_Vector sum_hi = Q6_Vw_vadd_VwVw(Q6_V_hi_W(s_even), Q6_V_hi_W(s_odd));
    sum_hi = Q6_Vw_vadd_VwVw(sum_hi, Q6_V_hi_W(t_even));
    sum_hi = Q6_Vw_vadd_VwVw(sum_hi, Q6_V_hi_W(t_odd));
    sum_hi = Q6_Vw_vasr_VwR(Q6_Vw_vadd_VwVw(sum_hi, vtwo), 2);

    HVX_Vector out = Q6_Vuh_vpack_VwVw_sat(sum_hi, sum_lo);
    vstoreu_partial(dst_uv, out, w * 4);
    s += 128;
    t += 128;
    dst_uv += 64;
    width -= 32;
  }
}

void HalfBoxUVRow_16_Odd_HVX(const uint16_t* src_uv,
                             ptrdiff_t src_stride_uv,
                             uint16_t* dst_uv,
                             int width) {
  width -= 1;
  if (width > 0) {
    HalfBoxUVRow_16_HVX(src_uv, src_stride_uv, dst_uv, width);
    src_uv += (ptrdiff_t)width * 4;
    dst_uv += (ptrdiff_t)width * 2;
  }
  const uint16_t* t_uv = src_uv + src_stride_uv;
  dst_uv[0] = (uint16_t)(((uint32_t)src_uv[0] + (uint32_t)t_uv[0] + 1u) >> 1);
  dst_uv[1] = (uint16_t)(((uint32_t)src_uv[1] + (uint32_t)t_uv[1] + 1u) >> 1);
}

void HalfUVRow_HVX(const uint8_t* src_u,
                   ptrdiff_t src_stride_u,
                   const uint8_t* src_v,
                   ptrdiff_t src_stride_v,
                   uint8_t* dst_uv,
                   int width) {
  const uint8_t* src_u1 = src_u + src_stride_u;
  const uint8_t* src_v1 = src_v + src_stride_v;
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector u = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_u, w),
                                          vloadu_partial_zero(src_u1, w));
    HVX_Vector v = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_v, w),
                                          vloadu_partial_zero(src_v1, w));
    HVX_VectorPair uv = Q6_W_vshuff_VVR(v, u, -1);
    int dst_bytes = w * 2;
    vstoreu_partial(dst_uv, Q6_V_lo_W(uv), dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_uv + 128, Q6_V_hi_W(uv), dst_bytes - 128);
    }
    src_u += 128;
    src_u1 += 128;
    src_v += 128;
    src_v1 += 128;
    dst_uv += 256;
    width -= 128;
  }
}

void SplitUVRow_16_HVX(const uint16_t* src_uv,
                       uint16_t* dst_u,
                       uint16_t* dst_v,
                       int depth,
                       int width) {
  int shift = 16 - depth;
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uv, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero((const uint8_t*)src_uv + 128, src_bytes - 128)
            : Q6_V_vzero();
    HVX_VectorPair uv = Q6_W_vdeal_VVR(s1, s0, -2);
    HVX_Vector u = Q6_Vuh_vlsr_VuhR(Q6_V_lo_W(uv), shift);
    HVX_Vector v = Q6_Vuh_vlsr_VuhR(Q6_V_hi_W(uv), shift);
    vstoreu_partial(dst_u, u, w * 2);
    vstoreu_partial(dst_v, v, w * 2);
    src_uv += 128;
    dst_u += 64;
    dst_v += 64;
    width -= 64;
  }
}

void MergeUVRow_16_HVX(const uint16_t* src_u,
                       const uint16_t* src_v,
                       uint16_t* dst_uv,
                       int depth,
                       int width) {
  int shift = 16 - depth;
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    HVX_Vector u = vloadu_partial_zero(src_u, w * 2);
    HVX_Vector v = vloadu_partial_zero(src_v, w * 2);
    u = Q6_Vh_vasl_VhR(u, shift);
    v = Q6_Vh_vasl_VhR(v, shift);
    HVX_VectorPair uv = Q6_W_vshuff_VVR(v, u, -2);
    int dst_bytes = w * 4;
    vstoreu_partial(dst_uv, Q6_V_lo_W(uv), dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial((uint8_t*)dst_uv + 128, Q6_V_hi_W(uv), dst_bytes - 128);
    }
    src_u += 64;
    src_v += 64;
    dst_uv += 128;
    width -= 64;
  }
}

void SplitARGBRow_HVX(const uint8_t* src_argb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      uint8_t* dst_a,
                      int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_argb, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_argb + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 =
        src_bytes > 256
            ? vloadu_partial_zero(src_argb + 256,
                                  src_bytes > 384 ? 128 : src_bytes - 256)
            : Q6_V_vzero();
    HVX_Vector s3 = src_bytes > 384
                        ? vloadu_partial_zero(src_argb + 384, src_bytes - 384)
                        : Q6_V_vzero();
    HVX_VectorPair p01 = Q6_W_vdeal_VVR(s1, s0, -1);
    HVX_VectorPair p23 = Q6_W_vdeal_VVR(s3, s2, -1);
    HVX_VectorPair br = Q6_W_vdeal_VVR(Q6_V_lo_W(p23), Q6_V_lo_W(p01), -1);
    HVX_VectorPair ga = Q6_W_vdeal_VVR(Q6_V_hi_W(p23), Q6_V_hi_W(p01), -1);
    vstoreu_partial(dst_b, Q6_V_lo_W(br), w);
    vstoreu_partial(dst_g, Q6_V_lo_W(ga), w);
    vstoreu_partial(dst_r, Q6_V_hi_W(br), w);
    vstoreu_partial(dst_a, Q6_V_hi_W(ga), w);
    src_argb += 512;
    dst_b += 128;
    dst_g += 128;
    dst_r += 128;
    dst_a += 128;
    width -= 128;
  }
}

void MergeARGBRow_HVX(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      const uint8_t* src_a,
                      uint8_t* dst_argb,
                      int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector b = vloadu_partial_zero(src_b, w);
    HVX_Vector g = vloadu_partial_zero(src_g, w);
    HVX_Vector r = vloadu_partial_zero(src_r, w);
    HVX_Vector a = vloadu_partial_zero(src_a, w);
    HVX_VectorPair br = Q6_W_vshuff_VVR(r, b, -1);
    HVX_VectorPair ga = Q6_W_vshuff_VVR(a, g, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(ga), Q6_V_lo_W(br), -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(ga), Q6_V_hi_W(br), -1);
    int dst_bytes = w * 4;
    vstoreu_partial(dst_argb, Q6_V_lo_W(p01),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_argb + 128, Q6_V_hi_W(p01),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst_argb + 256, Q6_V_lo_W(p23),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
    }
    if (dst_bytes > 384) {
      vstoreu_partial(dst_argb + 384, Q6_V_hi_W(p23), dst_bytes - 384);
    }
    src_b += 128;
    src_g += 128;
    src_r += 128;
    src_a += 128;
    dst_argb += 512;
    width -= 128;
  }
}

void SplitXRGBRow_HVX(const uint8_t* src_argb,
                      uint8_t* dst_r,
                      uint8_t* dst_g,
                      uint8_t* dst_b,
                      int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_argb, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_argb + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 =
        src_bytes > 256
            ? vloadu_partial_zero(src_argb + 256,
                                  src_bytes > 384 ? 128 : src_bytes - 256)
            : Q6_V_vzero();
    HVX_Vector s3 = src_bytes > 384
                        ? vloadu_partial_zero(src_argb + 384, src_bytes - 384)
                        : Q6_V_vzero();
    HVX_VectorPair p01 = Q6_W_vdeal_VVR(s1, s0, -1);
    HVX_VectorPair p23 = Q6_W_vdeal_VVR(s3, s2, -1);
    HVX_VectorPair br = Q6_W_vdeal_VVR(Q6_V_lo_W(p23), Q6_V_lo_W(p01), -1);
    HVX_VectorPair ga = Q6_W_vdeal_VVR(Q6_V_hi_W(p23), Q6_V_hi_W(p01), -1);
    vstoreu_partial(dst_b, Q6_V_lo_W(br), w);
    vstoreu_partial(dst_g, Q6_V_lo_W(ga), w);
    vstoreu_partial(dst_r, Q6_V_hi_W(br), w);
    src_argb += 512;
    dst_b += 128;
    dst_g += 128;
    dst_r += 128;
    width -= 128;
  }
}

void MergeXRGBRow_HVX(const uint8_t* src_r,
                      const uint8_t* src_g,
                      const uint8_t* src_b,
                      uint8_t* dst_argb,
                      int width) {
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector b = vloadu_partial_zero(src_b, w);
    HVX_Vector g = vloadu_partial_zero(src_g, w);
    HVX_Vector r = vloadu_partial_zero(src_r, w);
    HVX_VectorPair br = Q6_W_vshuff_VVR(r, b, -1);
    HVX_VectorPair ga = Q6_W_vshuff_VVR(a, g, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(ga), Q6_V_lo_W(br), -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(ga), Q6_V_hi_W(br), -1);
    int dst_bytes = w * 4;
    vstoreu_partial(dst_argb, Q6_V_lo_W(p01),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_argb + 128, Q6_V_hi_W(p01),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst_argb + 256, Q6_V_lo_W(p23),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
    }
    if (dst_bytes > 384) {
      vstoreu_partial(dst_argb + 384, Q6_V_hi_W(p23), dst_bytes - 384);
    }
    src_b += 128;
    src_g += 128;
    src_r += 128;
    dst_argb += 512;
    width -= 128;
  }
}

void MergeXR30Row_HVX(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      uint8_t* dst_ar30,
                      int depth,
                      int width) {
  int shift = depth - 10;
  HVX_Vector vmax = Q6_Vh_vsplat_R(1023);
  HVX_Vector valpha = Q6_Vh_vsplat_R(0xc000);
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    HVX_Vector b = vloadu_partial_zero(src_b, w * 2);
    HVX_Vector g = vloadu_partial_zero(src_g, w * 2);
    HVX_Vector r = vloadu_partial_zero(src_r, w * 2);
    b = Q6_Vuh_vmin_VuhVuh(Q6_Vuh_vlsr_VuhR(b, shift), vmax);
    g = Q6_Vuh_vmin_VuhVuh(Q6_Vuh_vlsr_VuhR(g, shift), vmax);
    r = Q6_Vuh_vmin_VuhVuh(Q6_Vuh_vlsr_VuhR(r, shift), vmax);
    // lo = b | (g << 10), hi = (g >> 6) | (r << 4) | 0xc000
    HVX_Vector lo = Q6_Vh_vaslacc_VhVhR(b, g, 10);
    HVX_Vector hi = Q6_Vh_vaslacc_VhVhR(valpha, r, 4);
    hi = Q6_Vh_vasracc_VhVhR(hi, g, 6);
    HVX_VectorPair ar30 = Q6_W_vshuff_VVR(hi, lo, -2);
    int dst_bytes = w * 4;
    vstoreu_partial(dst_ar30, Q6_V_lo_W(ar30),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_ar30 + 128, Q6_V_hi_W(ar30), dst_bytes - 128);
    }
    src_b += 64;
    src_g += 64;
    src_r += 64;
    dst_ar30 += 256;
    width -= 64;
  }
}

void MergeAR64Row_HVX(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      const uint16_t* src_a,
                      uint16_t* dst_ar64,
                      int depth,
                      int width) {
  int shift = 16 - depth;
  HVX_Vector vmax = Q6_Vh_vsplat_R((1 << depth) - 1);
  uint8_t* dst = (uint8_t*)dst_ar64;
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    HVX_Vector b = vloadu_partial_zero(src_b, w * 2);
    HVX_Vector g = vloadu_partial_zero(src_g, w * 2);
    HVX_Vector r = vloadu_partial_zero(src_r, w * 2);
    HVX_Vector a = vloadu_partial_zero(src_a, w * 2);
    b = Q6_Vh_vasl_VhR(Q6_Vuh_vmin_VuhVuh(b, vmax), shift);
    g = Q6_Vh_vasl_VhR(Q6_Vuh_vmin_VuhVuh(g, vmax), shift);
    r = Q6_Vh_vasl_VhR(Q6_Vuh_vmin_VuhVuh(r, vmax), shift);
    a = Q6_Vh_vasl_VhR(Q6_Vuh_vmin_VuhVuh(a, vmax), shift);
    HVX_VectorPair br = Q6_W_vshuff_VVR(r, b, -2);
    HVX_VectorPair ga = Q6_W_vshuff_VVR(a, g, -2);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(ga), Q6_V_lo_W(br), -2);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(ga), Q6_V_hi_W(br), -2);
    int dst_bytes = w * 8;
    vstoreu_partial(dst, Q6_V_lo_W(p01), dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst + 128, Q6_V_hi_W(p01),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst + 256, Q6_V_lo_W(p23),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
    }
    if (dst_bytes > 384) {
      vstoreu_partial(dst + 384, Q6_V_hi_W(p23), dst_bytes - 384);
    }
    src_b += 64;
    src_g += 64;
    src_r += 64;
    src_a += 64;
    dst += 512;
    width -= 64;
  }
}

void MergeXR64Row_HVX(const uint16_t* src_r,
                      const uint16_t* src_g,
                      const uint16_t* src_b,
                      uint16_t* dst_ar64,
                      int depth,
                      int width) {
  int shift = 16 - depth;
  HVX_Vector vmax = Q6_Vh_vsplat_R((1 << depth) - 1);
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  uint8_t* dst = (uint8_t*)dst_ar64;
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    HVX_Vector b = vloadu_partial_zero(src_b, w * 2);
    HVX_Vector g = vloadu_partial_zero(src_g, w * 2);
    HVX_Vector r = vloadu_partial_zero(src_r, w * 2);
    b = Q6_Vh_vasl_VhR(Q6_Vuh_vmin_VuhVuh(b, vmax), shift);
    g = Q6_Vh_vasl_VhR(Q6_Vuh_vmin_VuhVuh(g, vmax), shift);
    r = Q6_Vh_vasl_VhR(Q6_Vuh_vmin_VuhVuh(r, vmax), shift);
    HVX_VectorPair br = Q6_W_vshuff_VVR(r, b, -2);
    HVX_VectorPair ga = Q6_W_vshuff_VVR(a, g, -2);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(ga), Q6_V_lo_W(br), -2);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(ga), Q6_V_hi_W(br), -2);
    int dst_bytes = w * 8;
    vstoreu_partial(dst, Q6_V_lo_W(p01), dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst + 128, Q6_V_hi_W(p01),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst + 256, Q6_V_lo_W(p23),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
    }
    if (dst_bytes > 384) {
      vstoreu_partial(dst + 384, Q6_V_hi_W(p23), dst_bytes - 384);
    }
    src_b += 64;
    src_g += 64;
    src_r += 64;
    dst += 512;
    width -= 64;
  }
}

// Returns bytes of clamp255(lo >> shift) and clamp255(hi >> shift) interleaved.
// The narrowing shift uses only the low 3 bits of shift, so a shift of 8
// selects the high bytes instead.
static __inline HVX_Vector Merge16To8_HVX(HVX_Vector hi,
                                          HVX_Vector lo,
                                          int shift) {
  return shift == 8 ? Q6_Vb_vshuffo_VbVb(hi, lo)
                    : Q6_Vub_vasr_VuhVuhR_sat(hi, lo, shift);
}

void MergeARGB16To8Row_HVX(const uint16_t* src_r,
                           const uint16_t* src_g,
                           const uint16_t* src_b,
                           const uint16_t* src_a,
                           uint8_t* dst_argb,
                           int depth,
                           int width) {
  int shift = depth - 8;
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    HVX_Vector b = vloadu_partial_zero(src_b, w * 2);
    HVX_Vector g = vloadu_partial_zero(src_g, w * 2);
    HVX_Vector r = vloadu_partial_zero(src_r, w * 2);
    HVX_Vector a = vloadu_partial_zero(src_a, w * 2);
    HVX_Vector bg = Merge16To8_HVX(g, b, shift);
    HVX_Vector ra = Merge16To8_HVX(a, r, shift);
    HVX_VectorPair argb = Q6_W_vshuff_VVR(ra, bg, -2);
    int dst_bytes = w * 4;
    vstoreu_partial(dst_argb, Q6_V_lo_W(argb),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_argb + 128, Q6_V_hi_W(argb), dst_bytes - 128);
    }
    src_b += 64;
    src_g += 64;
    src_r += 64;
    src_a += 64;
    dst_argb += 256;
    width -= 64;
  }
}

void MergeXRGB16To8Row_HVX(const uint16_t* src_r,
                           const uint16_t* src_g,
                           const uint16_t* src_b,
                           uint8_t* dst_argb,
                           int depth,
                           int width) {
  int shift = depth - 8;
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    HVX_Vector b = vloadu_partial_zero(src_b, w * 2);
    HVX_Vector g = vloadu_partial_zero(src_g, w * 2);
    HVX_Vector r = vloadu_partial_zero(src_r, w * 2);
    HVX_Vector bg = Merge16To8_HVX(g, b, shift);
    HVX_Vector ra = Merge16To8_HVX(a, r, shift);
    HVX_VectorPair argb = Q6_W_vshuff_VVR(ra, bg, -2);
    int dst_bytes = w * 4;
    vstoreu_partial(dst_argb, Q6_V_lo_W(argb),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_argb + 128, Q6_V_hi_W(argb), dst_bytes - 128);
    }
    src_b += 64;
    src_g += 64;
    src_r += 64;
    dst_argb += 256;
    width -= 64;
  }
}

void SwapUVRow_HVX(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
  HVX_Vector ctrl = Q6_Vb_vsplat_R(0x01);
  int bytes = width * 2;
  while (bytes >= 128) {
    HVX_Vector v = vloadu(src_uv);
    vstoreu(dst_vu, Q6_V_vdelta_VV(v, ctrl));
    src_uv += 128;
    dst_vu += 128;
    bytes -= 128;
  }
  if (bytes > 0) {
    HVX_Vector v = vloadu_partial_zero(src_uv, bytes);
    vstoreu_partial(dst_vu, Q6_V_vdelta_VV(v, ctrl), bytes);
  }
}

void InterpolateRow_HVX(uint8_t* dst_ptr,
                        const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        int width,
                        int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  if (y1_fraction == 0) {
    CopyRow_HVX(src_ptr, dst_ptr, width);
    return;
  }
  if (y1_fraction == 128) {
    while (width >= 128) {
      HVX_Vector s0 = vloadu(src_ptr);
      HVX_Vector s1 = vloadu(src_ptr1);
      vstoreu(dst_ptr, Q6_Vub_vavg_VubVub_rnd(s0, s1));
      src_ptr += 128;
      src_ptr1 += 128;
      dst_ptr += 128;
      width -= 128;
    }
    if (width > 0) {
      HVX_Vector s0 = vloadu_partial_zero(src_ptr, width);
      HVX_Vector s1 = vloadu_partial_zero(src_ptr1, width);
      vstoreu_partial(dst_ptr, Q6_Vub_vavg_VubVub_rnd(s0, s1), width);
    }
    return;
  }
  uint32_t y0_32 = (uint32_t)y0_fraction * 0x01010101u;
  uint32_t y1_32 = (uint32_t)y1_fraction * 0x01010101u;
  HVX_Vector v128 = Q6_Vh_vsplat_R(128);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector s0 = vloadu_partial_zero(src_ptr, w);
    HVX_Vector s1 = vloadu_partial_zero(src_ptr1, w);
    HVX_VectorPair p = Q6_Wuh_vmpy_VubRub(s0, y0_32);
    p = Q6_Wuh_vmpyacc_WuhVubRub(p, s1, y1_32);
    HVX_Vector lo = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_lo_W(p), v128);
    HVX_Vector hi = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_hi_W(p), v128);
    HVX_Vector out = Q6_Vb_vshuffo_VbVb(hi, lo);
    vstoreu_partial(dst_ptr, out, w);
    src_ptr += 128;
    src_ptr1 += 128;
    dst_ptr += 128;
    width -= 128;
  }
}

void InterpolateRow_16_HVX(uint16_t* dst_ptr,
                           const uint16_t* src_ptr,
                           ptrdiff_t src_stride,
                           int width,
                           int source_y_fraction) {
  int y1_fraction = source_y_fraction;
  int y0_fraction = 256 - y1_fraction;
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  if (y1_fraction == 0) {
    CopyRow_16_HVX(src_ptr, dst_ptr, width);
    return;
  }
  if (y1_fraction == 128) {
    int bytes = width * 2;
    const uint8_t* s0_p = (const uint8_t*)src_ptr;
    const uint8_t* s1_p = (const uint8_t*)src_ptr1;
    uint8_t* d_p = (uint8_t*)dst_ptr;
    while (bytes >= 128) {
      HVX_Vector s0 = vloadu(s0_p);
      HVX_Vector s1 = vloadu(s1_p);
      vstoreu(d_p, Q6_Vuh_vavg_VuhVuh_rnd(s0, s1));
      s0_p += 128;
      s1_p += 128;
      d_p += 128;
      bytes -= 128;
    }
    if (bytes > 0) {
      HVX_Vector s0 = vloadu_partial_zero(s0_p, bytes);
      HVX_Vector s1 = vloadu_partial_zero(s1_p, bytes);
      vstoreu_partial(d_p, Q6_Vuh_vavg_VuhVuh_rnd(s0, s1), bytes);
    }
    return;
  }
  uint32_t y0_32 = ((uint32_t)y0_fraction << 16) | (uint32_t)y0_fraction;
  uint32_t y1_32 = ((uint32_t)y1_fraction << 16) | (uint32_t)y1_fraction;
  HVX_Vector v128 = Q6_V_vsplat_R(128);
  while (width >= 64) {
    HVX_Vector s0 = vloadu(src_ptr);
    HVX_Vector s1 = vloadu(src_ptr1);
    HVX_VectorPair p = Q6_Wuw_vmpy_VuhRuh(s0, y0_32);
    p = Q6_Wuw_vmpyacc_WuwVuhRuh(p, s1, y1_32);
    HVX_Vector lo = Q6_Vuw_vlsr_VuwR(Q6_Vw_vadd_VwVw(Q6_V_lo_W(p), v128), 8);
    HVX_Vector hi = Q6_Vuw_vlsr_VuwR(Q6_Vw_vadd_VwVw(Q6_V_hi_W(p), v128), 8);
    vstoreu(dst_ptr, Q6_Vh_vshuffe_VhVh(hi, lo));
    src_ptr += 64;
    src_ptr1 += 64;
    dst_ptr += 64;
    width -= 64;
  }
  if (width > 0) {
    int bytes = width * 2;
    HVX_Vector s0 = vloadu_partial_zero(src_ptr, bytes);
    HVX_Vector s1 = vloadu_partial_zero(src_ptr1, bytes);
    HVX_VectorPair p = Q6_Wuw_vmpy_VuhRuh(s0, y0_32);
    p = Q6_Wuw_vmpyacc_WuwVuhRuh(p, s1, y1_32);
    HVX_Vector lo = Q6_Vuw_vlsr_VuwR(Q6_Vw_vadd_VwVw(Q6_V_lo_W(p), v128), 8);
    HVX_Vector hi = Q6_Vuw_vlsr_VuwR(Q6_Vw_vadd_VwVw(Q6_V_hi_W(p), v128), 8);
    vstoreu_partial(dst_ptr, Q6_Vh_vshuffe_VhVh(hi, lo), bytes);
  }
}

void BlendPlaneRow_HVX(const uint8_t* src0,
                       const uint8_t* src1,
                       const uint8_t* alpha,
                       uint8_t* dst,
                       int width) {
  HVX_Vector v255 = Q6_Vh_vsplat_R(255);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector f = vloadu_partial_zero(src0, w);
    HVX_Vector b = vloadu_partial_zero(src1, w);
    HVX_Vector a = vloadu_partial_zero(alpha, w);
    HVX_Vector not_a = Q6_V_vnot_V(a);
    HVX_VectorPair p = Q6_Wuh_vmpy_VubVub(f, a);
    p = Q6_Wuh_vmpyacc_WuhVubVub(p, b, not_a);
    HVX_Vector lo = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_lo_W(p), v255);
    HVX_Vector hi = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_hi_W(p), v255);
    HVX_Vector out = Q6_Vb_vshuffo_VbVb(hi, lo);
    vstoreu_partial(dst, out, w);
    src0 += 128;
    src1 += 128;
    alpha += 128;
    dst += 128;
    width -= 128;
  }
}

void ARGBBlendRow_HVX(const uint8_t* src_argb,
                      const uint8_t* src_argb1,
                      uint8_t* dst_argb,
                      int width) {
  HVX_Vector v255 = Q6_Vh_vsplat_R(255);
  HVX_Vector v_alpha_mask = Q6_V_vsplat_R(0xFF000000u);
  int bytes = width * 4;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    HVX_Vector fg = vloadu_partial_zero(src_argb, b_cnt);
    HVX_Vector bg = vloadu_partial_zero(src_argb1, b_cnt);
    HVX_Vector t = Q6_Vb_vshuffo_VbVb(fg, fg);
    HVX_Vector a_vec = Q6_Vh_vshuffo_VhVh(t, t);
    HVX_VectorPair p = Q6_Wuh_vmpy_VubVub(bg, a_vec);
    HVX_Vector lo = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_lo_W(p), v255);
    HVX_Vector hi = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_hi_W(p), v255);
    HVX_Vector atten_bg = Q6_Vb_vshuffo_VbVb(hi, lo);
    HVX_Vector scaled_bg = Q6_Vub_vsub_VubVub_sat(bg, atten_bg);
    HVX_Vector out = Q6_Vub_vadd_VubVub_sat(scaled_bg, fg);
    out = Q6_V_vor_VV(out, v_alpha_mask);
    vstoreu_partial(dst_argb, out, b_cnt);
    src_argb += 128;
    src_argb1 += 128;
    dst_argb += 128;
    bytes -= 128;
  }
}

void ARGBAlphaBlendRow_HVX(const uint8_t* src_argb0,
                           const uint8_t* src_argb1,
                           uint8_t* dst_argb,
                           int width) {
  HVX_Vector vzero = Q6_V_vzero();
  HVX_Vector v0001 = Q6_Vh_vsplat_R(1);
  HVX_Vector v00ff = Q6_Vh_vsplat_R(255);
  HVX_Vector vffff = Q6_Vh_vsplat_R(-1);
  HVX_Vector v_rgb_mask = Q6_V_vsplat_R(0x00FFFFFFu);
  int bytes = width * 4;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    HVX_Vector fg = vloadu_partial_zero(src_argb0, b_cnt);
    HVX_Vector bg = vloadu_partial_zero(src_argb1, b_cnt);

    // Broadcast sa and da to all 4 byte lanes of each 32-bit pixel.
    HVX_Vector t_fg = Q6_Vb_vshuffo_VbVb(fg, fg);
    HVX_Vector sa_vec = Q6_Vh_vshuffo_VhVh(t_fg, t_fg);
    HVX_Vector t_bg = Q6_Vb_vshuffo_VbVb(bg, bg);
    HVX_Vector da_vec = Q6_Vh_vshuffo_VhVh(t_bg, t_bg);

    // Both uint16_t halfwords of each pixel hold sa and da (0..255).
    HVX_Vector sa16 = Q6_V_vand_VV(sa_vec, v00ff);
    HVX_Vector da16 = Q6_V_vand_VV(da_vec, v00ff);

    // x = da * sa in both uint16_t halfwords of each pixel.
    HVX_Vector x = Q6_V_lo_W(Q6_Wuh_vmpy_VubVub(da_vec, sa_vec));
    // prod_div255 = (x + 1 + (x >> 8)) >> 8.
    HVX_Vector prod_div255 = Q6_Vuh_vlsr_VuhR(
        Q6_Vh_vadd_VhVh(Q6_Vh_vadd_VhVh(x, v0001), Q6_Vuh_vlsr_VuhR(x, 8)), 8);
    // out_a = da + sa - (da * sa) / 255.
    HVX_Vector out_a16 =
        Q6_Vh_vadd_VhVh(da16, Q6_Vh_vsub_VhVh(sa16, prod_div255));

    // 8-step restoring division: ratio = (da == 0) ? 255 : (sa * 255) / out_a.
    HVX_Vector rem = Q6_Vh_vsub_VhVh(Q6_Vh_vasl_VhR(sa16, 8), sa16);
    HVX_VectorPred q_da_zero = Q6_Q_vcmp_eq_VhVh(da16, vzero);
    rem = Q6_V_vmux_QVV(q_da_zero, vffff, rem);
    HVX_Vector ratio = vzero;

    for (int bit = 7; bit >= 0; --bit) {
      HVX_Vector d_sh = Q6_Vh_vasl_VhR(out_a16, bit);
      HVX_VectorPred q_gt = Q6_Q_vcmp_gt_VuhVuh(d_sh, rem);
      rem = Q6_V_vmux_QVV(q_gt, rem, Q6_Vh_vsub_VhVh(rem, d_sh));
      ratio = Q6_V_vmux_QVV(q_gt, ratio,
                            Q6_V_vor_VV(ratio, Q6_Vb_vsplat_R(1 << bit)));
    }

    // Exact RGB lerp: (bg * (255 - ratio) + fg * ratio) / 255.
    HVX_Vector inv_ratio = Q6_V_vnot_V(ratio);
    HVX_VectorPair p_rgb = Q6_Wuh_vmpy_VubVub(fg, ratio);
    p_rgb = Q6_Wuh_vmpyacc_WuhVubVub(p_rgb, bg, inv_ratio);
    HVX_Vector lo = Q6_V_lo_W(p_rgb);
    HVX_Vector hi = Q6_V_hi_W(p_rgb);
    lo = Q6_Vh_vadd_VhVh(Q6_Vh_vadd_VhVh(lo, v0001), Q6_Vuh_vlsr_VuhR(lo, 8));
    hi = Q6_Vh_vadd_VhVh(Q6_Vh_vadd_VhVh(hi, v0001), Q6_Vuh_vlsr_VuhR(hi, 8));
    HVX_Vector out = Q6_Vb_vshuffo_VbVb(hi, lo);
    out =
        Q6_V_vor_VV(Q6_V_vand_VV(out, v_rgb_mask), Q6_Vw_vasl_VwR(out_a16, 24));
    vstoreu_partial(dst_argb, out, b_cnt);
    src_argb0 += 128;
    src_argb1 += 128;
    dst_argb += 128;
    bytes -= 128;
  }
}

void ARGBAttenuateRow_HVX(const uint8_t* src_argb,
                          uint8_t* dst_argb,
                          int width) {
  HVX_Vector v255 = Q6_Vh_vsplat_R(255);
  HVX_Vector v_rgb_mask = Q6_V_vsplat_R(0x00FFFFFFu);
  HVX_Vector v_a_mask = Q6_V_vsplat_R(0xFF000000u);
  int bytes = width * 4;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    HVX_Vector src = vloadu_partial_zero(src_argb, b_cnt);
    HVX_Vector t = Q6_Vb_vshuffo_VbVb(src, src);
    HVX_Vector a_vec = Q6_Vh_vshuffo_VhVh(t, t);
    HVX_VectorPair p = Q6_Wuh_vmpy_VubVub(src, a_vec);
    HVX_Vector lo = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_lo_W(p), v255);
    HVX_Vector hi = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_hi_W(p), v255);
    HVX_Vector out = Q6_Vb_vshuffo_VbVb(hi, lo);
    out =
        Q6_V_vor_VV(Q6_V_vand_VV(out, v_rgb_mask), Q6_V_vand_VV(src, v_a_mask));
    vstoreu_partial(dst_argb, out, b_cnt);
    src_argb += 128;
    dst_argb += 128;
    bytes -= 128;
  }
}

void ARGBUnattenuateRow_HVX(const uint8_t* src_argb,
                            uint8_t* dst_argb,
                            int width) {
  HVX_Vector v255 = Q6_Vh_vsplat_R(255);
  HVX_Vector v_a_mask = Q6_V_vsplat_R(0xFF000000u);
  HVX_Vector tbl0 = Q6_Vh_vshuff_Vh(vloadu(&fixed_invtbl16[0]));
  HVX_Vector tbl1 = Q6_Vh_vshuff_Vh(vloadu(&fixed_invtbl16[64]));
  HVX_Vector tbl2 = Q6_Vh_vshuff_Vh(vloadu(&fixed_invtbl16[128]));
  HVX_Vector tbl3 = Q6_Vh_vshuff_Vh(vloadu(&fixed_invtbl16[192]));
  int bytes = width * 4;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    HVX_Vector src = vloadu_partial_zero(src_argb, b_cnt);
    HVX_Vector t = Q6_Vb_vshuffo_VbVb(src, src);
    HVX_Vector a_idx = Q6_V_vor_VV(Q6_Vh_vshuffo_VhVh(t, t), v_a_mask);

    HVX_VectorPair ia = Q6_Wh_vlut16_VbVhR(a_idx, tbl0, 0);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl0, 1);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl0, 2);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl0, 3);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl1, 4);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl1, 5);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl1, 6);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl1, 7);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl2, 8);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl2, 9);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl2, 10);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl2, 11);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl3, 12);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl3, 13);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl3, 14);
    ia = Q6_Wh_vlut16or_WhVbVhR(ia, a_idx, tbl3, 15);

    HVX_VectorPair w_src = Q6_Wuh_vzxt_Vub(src);
    HVX_Vector f_even = Q6_V_lo_W(w_src);
    HVX_Vector f_odd = Q6_V_hi_W(w_src);
    f_even = Q6_V_vor_VV(f_even, Q6_Vh_vasl_VhR(f_even, 8));
    f_odd = Q6_V_vor_VV(f_odd, Q6_Vh_vasl_VhR(f_odd, 8));

    HVX_Vector res_even = Q6_Vuh_vmin_VuhVuh(
        Q6_Vuh_vmpy_VuhVuh_rs16(f_even, Q6_V_lo_W(ia)), v255);
    HVX_Vector res_odd = Q6_Vuh_vmin_VuhVuh(
        Q6_Vuh_vmpy_VuhVuh_rs16(f_odd, Q6_V_hi_W(ia)), v255);
    HVX_Vector out = Q6_Vb_vshuffe_VbVb(res_odd, res_even);
    vstoreu_partial(dst_argb, out, b_cnt);
    src_argb += 128;
    dst_argb += 128;
    bytes -= 128;
  }
}

void ARGBMultiplyRow_HVX(const uint8_t* src_argb,
                         const uint8_t* src_argb1,
                         uint8_t* dst_argb,
                         int width) {
  HVX_Vector v128 = Q6_Vh_vsplat_R(128);
  int bytes = width * 4;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    HVX_Vector s0 = vloadu_partial_zero(src_argb, b_cnt);
    HVX_Vector s1 = vloadu_partial_zero(src_argb1, b_cnt);
    HVX_VectorPair p = Q6_Wuh_vmpy_VubVub(s0, s1);
    HVX_Vector lo = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_lo_W(p), v128);
    HVX_Vector hi = Q6_Vuh_vadd_VuhVuh_sat(Q6_V_hi_W(p), v128);
    HVX_Vector out = Q6_Vb_vshuffo_VbVb(hi, lo);
    vstoreu_partial(dst_argb, out, b_cnt);
    src_argb += 128;
    src_argb1 += 128;
    dst_argb += 128;
    bytes -= 128;
  }
}

void ARGBAddRow_HVX(const uint8_t* src_argb,
                    const uint8_t* src_argb1,
                    uint8_t* dst_argb,
                    int width) {
  int bytes = width * 4;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    HVX_Vector s0 = vloadu_partial_zero(src_argb, b_cnt);
    HVX_Vector s1 = vloadu_partial_zero(src_argb1, b_cnt);
    vstoreu_partial(dst_argb, Q6_Vub_vadd_VubVub_sat(s0, s1), b_cnt);
    src_argb += 128;
    src_argb1 += 128;
    dst_argb += 128;
    bytes -= 128;
  }
}

void ARGBSubtractRow_HVX(const uint8_t* src_argb,
                         const uint8_t* src_argb1,
                         uint8_t* dst_argb,
                         int width) {
  int bytes = width * 4;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    HVX_Vector s0 = vloadu_partial_zero(src_argb, b_cnt);
    HVX_Vector s1 = vloadu_partial_zero(src_argb1, b_cnt);
    vstoreu_partial(dst_argb, Q6_Vub_vsub_VubVub_sat(s0, s1), b_cnt);
    src_argb += 128;
    src_argb1 += 128;
    dst_argb += 128;
    bytes -= 128;
  }
}

void ARGBExtractAlphaRow_HVX(const uint8_t* src_argb,
                             uint8_t* dst_a,
                             int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_argb, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_argb + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 =
        src_bytes > 256
            ? vloadu_partial_zero(src_argb + 256,
                                  src_bytes > 384 ? 128 : src_bytes - 256)
            : Q6_V_vzero();
    HVX_Vector s3 = src_bytes > 384
                        ? vloadu_partial_zero(src_argb + 384, src_bytes - 384)
                        : Q6_V_vzero();
    HVX_Vector ga01 = Q6_V_hi_W(Q6_W_vdeal_VVR(s1, s0, -1));
    HVX_Vector ga23 = Q6_V_hi_W(Q6_W_vdeal_VVR(s3, s2, -1));
    HVX_Vector a = Q6_V_hi_W(Q6_W_vdeal_VVR(ga23, ga01, -1));
    vstoreu_partial(dst_a, a, w);
    src_argb += 512;
    dst_a += 128;
    width -= 128;
  }
}

void ARGBCopyAlphaRow_HVX(const uint8_t* src, uint8_t* dst, int width) {
  HVX_VectorPred q_alpha =
      Q6_Q_vcmp_gt_VubVub(Q6_V_vsplat_R(0x01000000u), Q6_V_vzero());
  int bytes = width * 4;
  while (bytes >= 128) {
    HVX_Vector s = vloadu(src);
    HVX_Vector d = vloadu(dst);
    vstoreu(dst, Q6_V_vmux_QVV(q_alpha, s, d));
    src += 128;
    dst += 128;
    bytes -= 128;
  }
  if (bytes > 0) {
    HVX_Vector s = vloadu_partial_zero(src, bytes);
    HVX_Vector d = vloadu_partial_zero(dst, bytes);
    vstoreu_partial(dst, Q6_V_vmux_QVV(q_alpha, s, d), bytes);
  }
}

void ARGBCopyYToAlphaRow_HVX(const uint8_t* src, uint8_t* dst, int width) {
  HVX_Vector v_rgb_mask = Q6_V_vsplat_R(0x00FFFFFFu);
  HVX_Vector z = Q6_V_vzero();
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector y = vloadu_partial_zero(src, w);
    HVX_VectorPair ga = Q6_W_vshuff_VVR(y, z, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(ga), z, -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(ga), z, -1);
    int dst_bytes = w * 4;
    int b0 = dst_bytes > 128 ? 128 : dst_bytes;
    HVX_Vector d0 = vloadu_partial_zero(dst, b0);
    vstoreu_partial(
        dst, Q6_V_vor_VV(Q6_V_vand_VV(d0, v_rgb_mask), Q6_V_lo_W(p01)), b0);
    if (dst_bytes > 128) {
      int b1 = dst_bytes > 256 ? 128 : dst_bytes - 128;
      HVX_Vector d1 = vloadu_partial_zero(dst + 128, b1);
      vstoreu_partial(dst + 128,
                      Q6_V_vor_VV(Q6_V_vand_VV(d1, v_rgb_mask), Q6_V_hi_W(p01)),
                      b1);
    }
    if (dst_bytes > 256) {
      int b2 = dst_bytes > 384 ? 128 : dst_bytes - 256;
      HVX_Vector d2 = vloadu_partial_zero(dst + 256, b2);
      vstoreu_partial(dst + 256,
                      Q6_V_vor_VV(Q6_V_vand_VV(d2, v_rgb_mask), Q6_V_lo_W(p23)),
                      b2);
    }
    if (dst_bytes > 384) {
      int b3 = dst_bytes - 384;
      HVX_Vector d3 = vloadu_partial_zero(dst + 384, b3);
      vstoreu_partial(dst + 384,
                      Q6_V_vor_VV(Q6_V_vand_VV(d3, v_rgb_mask), Q6_V_hi_W(p23)),
                      b3);
    }
    src += 128;
    dst += 512;
    width -= 128;
  }
}

void Convert16To8Row_HVX(const uint16_t* src_y,
                         uint8_t* dst_y,
                         int scale,
                         int width) {
  uint32_t scale32 = ((uint32_t)(uint16_t)scale << 16) | (uint16_t)scale;
  HVX_Vector v255 = Q6_Vh_vsplat_R(255);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 2;
    HVX_Vector s0 =
        vloadu_partial_zero(src_y, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero((const uint8_t*)src_y + 128, src_bytes - 128)
            : Q6_V_vzero();
    HVX_VectorPair p0 = Q6_Wuw_vmpy_VuhRuh(s0, scale32);
    HVX_VectorPair p1 = Q6_Wuw_vmpy_VuhRuh(s1, scale32);
    HVX_Vector h0 = Q6_Vuh_vmin_VuhVuh(
        Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p0), Q6_V_lo_W(p0)), v255);
    HVX_Vector h1 = Q6_Vuh_vmin_VuhVuh(
        Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p1), Q6_V_lo_W(p1)), v255);
    HVX_Vector out = Q6_Vb_vpacke_VhVh(h1, h0);
    vstoreu_partial(dst_y, out, w);
    src_y += 128;
    dst_y += 128;
    width -= 128;
  }
}

void Convert8To16Row_HVX(const uint8_t* src_y,
                         uint16_t* dst_y,
                         int bits,
                         int width) {
  int shift = 16 - bits;
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector s = vloadu_partial_zero(src_y, w);
    HVX_VectorPair d16 = Q6_W_vshuff_VVR(s, s, -1);
    HVX_Vector d0 = Q6_Vuh_vlsr_VuhR(Q6_V_lo_W(d16), shift);
    HVX_Vector d1 = Q6_Vuh_vlsr_VuhR(Q6_V_hi_W(d16), shift);
    int dst_bytes = w * 2;
    vstoreu_partial(dst_y, d0, dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial((uint8_t*)dst_y + 128, d1, dst_bytes - 128);
    }
    src_y += 128;
    dst_y += 128;
    width -= 128;
  }
}

void Convert8To8Row_HVX(const uint8_t* src_y,
                        uint8_t* dst_y,
                        int scale,
                        int bias,
                        int width) {
  uint32_t scale32 = (uint32_t)(uint8_t)scale * 0x01010101u;
  HVX_Vector vbias = Q6_Vb_vsplat_R(bias);
  while (width >= 128) {
    HVX_Vector s = vloadu(src_y);
    HVX_VectorPair p = Q6_Wuh_vmpy_VubRub(s, scale32);
    HVX_Vector out =
        Q6_Vb_vadd_VbVb(Q6_Vb_vshuffo_VbVb(Q6_V_hi_W(p), Q6_V_lo_W(p)), vbias);
    vstoreu(dst_y, out);
    src_y += 128;
    dst_y += 128;
    width -= 128;
  }
  if (width > 0) {
    HVX_Vector s = vloadu_partial_zero(src_y, width);
    HVX_VectorPair p = Q6_Wuh_vmpy_VubRub(s, scale32);
    HVX_Vector out =
        Q6_Vb_vadd_VbVb(Q6_Vb_vshuffo_VbVb(Q6_V_hi_W(p), Q6_V_lo_W(p)), vbias);
    vstoreu_partial(dst_y, out, width);
  }
}

void MultiplyRow_16_HVX(const uint16_t* src_y,
                        uint16_t* dst_y,
                        int scale,
                        int width) {
  uint32_t scale32 = ((uint32_t)(uint16_t)scale << 16) | (uint16_t)scale;
  while (width > 0) {
    int w = width >= 64 ? 64 : width;
    int bytes = w * 2;
    HVX_Vector s = vloadu_partial_zero(src_y, bytes);
    HVX_VectorPair p = Q6_Wuw_vmpy_VuhRuh(s, scale32);
    HVX_Vector out = Q6_Vh_vshuffe_VhVh(Q6_V_hi_W(p), Q6_V_lo_W(p));
    vstoreu_partial(dst_y, out, bytes);
    src_y += 64;
    dst_y += 64;
    width -= 64;
  }
}

void DivideRow_16_HVX(const uint16_t* src_y,
                      uint16_t* dst_y,
                      int scale,
                      int width) {
  if (scale == 65536) {  // Divide by 1, so do copy.
    if (src_y != dst_y) {
      CopyRow_16_HVX(src_y, dst_y, width);
    }
    return;
  }
  uint32_t scale32 = ((uint32_t)(uint16_t)scale << 16) | (uint16_t)scale;
  while (width >= 64) {
    HVX_Vector s = vloadu(src_y);
    HVX_VectorPair p = Q6_Wuw_vmpy_VuhRuh(s, scale32);
    vstoreu(dst_y, Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p), Q6_V_lo_W(p)));
    src_y += 64;
    dst_y += 64;
    width -= 64;
  }
  if (width > 0) {
    int bytes = width * 2;
    HVX_Vector s = vloadu_partial_zero(src_y, bytes);
    HVX_VectorPair p = Q6_Wuw_vmpy_VuhRuh(s, scale32);
    vstoreu_partial(dst_y, Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p), Q6_V_lo_W(p)),
                    bytes);
  }
}

void HalfRow_16To8_HVX(const uint16_t* src_uv,
                       ptrdiff_t src_uv_stride,
                       uint8_t* dst_uv,
                       int scale,
                       int width) {
  uint32_t scale32 = ((uint32_t)(uint16_t)scale << 16) | (uint16_t)scale;
  HVX_Vector v255 = Q6_Vh_vsplat_R(255);
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 2;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uv, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero((const uint8_t*)src_uv + 128, src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector t0 =
        vloadu_partial_zero(src_uv1, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector t1 = src_bytes > 128
                        ? vloadu_partial_zero((const uint8_t*)src_uv1 + 128,
                                              src_bytes - 128)
                        : Q6_V_vzero();
    HVX_Vector a0 = Q6_Vuh_vavg_VuhVuh_rnd(s0, t0);
    HVX_Vector a1 = Q6_Vuh_vavg_VuhVuh_rnd(s1, t1);
    HVX_VectorPair p0 = Q6_Wuw_vmpy_VuhRuh(a0, scale32);
    HVX_VectorPair p1 = Q6_Wuw_vmpy_VuhRuh(a1, scale32);
    HVX_Vector h0 = Q6_Vuh_vmin_VuhVuh(
        Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p0), Q6_V_lo_W(p0)), v255);
    HVX_Vector h1 = Q6_Vuh_vmin_VuhVuh(
        Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p1), Q6_V_lo_W(p1)), v255);
    HVX_Vector out = Q6_Vb_vpacke_VhVh(h1, h0);
    vstoreu_partial(dst_uv, out, w);
    src_uv += 128;
    src_uv1 += 128;
    dst_uv += 128;
    width -= 128;
  }
}

// 2x2 box filter of two pairs of 16 bit source vectors, scaled:
// (((s[0] + s[1] + t[0] + t[1] + 2) >> 2) * scale) >> 16.
// Returns 64 results as 16 bit lanes: the 32 results of s0/t0 in the even
// lanes and the 32 results of s1/t1 in the odd lanes.
static __inline HVX_Vector HalfWidth16To8_HVX(HVX_Vector s0,
                                              HVX_Vector s1,
                                              HVX_Vector t0,
                                              HVX_Vector t1,
                                              HVX_Vector vscale) {
  // Widening vertical add: even columns in lo, odd columns in hi.
  HVX_VectorPair c0 = Q6_Ww_vadd_VuhVuh(s0, t0);
  HVX_VectorPair c1 = Q6_Ww_vadd_VuhVuh(s1, t1);
  HVX_Vector sum0 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(c0), Q6_V_hi_W(c0));
  HVX_Vector sum1 = Q6_Vw_vadd_VwVw(Q6_V_lo_W(c1), Q6_V_hi_W(c1));
  // (sum + 2) >> 2 narrowed to 16 bit: sum0 to even lanes, sum1 to odd lanes.
  HVX_Vector avg = Q6_Vuh_vasr_VwVwR_rnd_sat(sum1, sum0, 2);
  return Q6_Vuh_vmpy_VuhVuh_rs16(avg, vscale);
}

// scale is 1 << (24 - depth) and must not exceed 32768 (depth 9 or more).
void HalfWidthRow_16To8_HVX(const uint16_t* src_uv,
                            ptrdiff_t src_uv_stride,
                            uint8_t* dst_uv,
                            int scale,
                            int width) {
  const uint8_t* s = (const uint8_t*)src_uv;
  const uint8_t* t = (const uint8_t*)(src_uv + src_uv_stride);
  HVX_Vector vscale = Q6_Vh_vsplat_R(scale);
  // Each 128 byte source vector makes 32 pixels.  Vectors 0 and 2 are paired
  // and vectors 1 and 3 are paired, so that after packing to bytes a single
  // deal of even and odd bytes puts the 128 pixels in order.
  while (width >= 128) {
    HVX_Vector h0 = HalfWidth16To8_HVX(vloadu(s), vloadu(s + 256), vloadu(t),
                                       vloadu(t + 256), vscale);
    HVX_Vector h1 =
        HalfWidth16To8_HVX(vloadu(s + 128), vloadu(s + 384), vloadu(t + 128),
                           vloadu(t + 384), vscale);
    vstoreu(dst_uv, Q6_Vb_vdeal_Vb(Q6_Vub_vpack_VhVh_sat(h1, h0)));
    s += 512;
    t += 512;
    dst_uv += 128;
    width -= 128;
  }
  if (width > 0) {
    // Source bytes per row.  vloadu_partial_zero() returns zero without
    // reading when its length is 0 or less.
    int n = width * 4;
    HVX_Vector h0 = HalfWidth16To8_HVX(
        vloadu_partial_zero(s, n), vloadu_partial_zero(s + 256, n - 256),
        vloadu_partial_zero(t, n), vloadu_partial_zero(t + 256, n - 256),
        vscale);
    HVX_Vector h1 =
        HalfWidth16To8_HVX(vloadu_partial_zero(s + 128, n - 128),
                           vloadu_partial_zero(s + 384, n - 384),
                           vloadu_partial_zero(t + 128, n - 128),
                           vloadu_partial_zero(t + 384, n - 384), vscale);
    vstoreu_partial(dst_uv, Q6_Vb_vdeal_Vb(Q6_Vub_vpack_VhVh_sat(h1, h0)),
                    width);
  }
}

void HalfWidthRow_16To8_Odd_HVX(const uint16_t* src_uv,
                                ptrdiff_t src_uv_stride,
                                uint8_t* dst_uv,
                                int scale,
                                int width) {
  int n = width - 1;
  if (n > 0) {
    HalfWidthRow_16To8_HVX(src_uv, src_uv_stride, dst_uv, scale, n);
    src_uv += (ptrdiff_t)n * 2;
    dst_uv += n;
  }
  HalfWidthRow_16To8_Odd_C(src_uv, src_uv_stride, dst_uv, scale, 1);
}

void YUY2ToYRow_HVX(const uint8_t* src_yuy2, uint8_t* dst_y, int width) {
  while (width >= 128) {
    HVX_Vector s0 = vloadu(src_yuy2);
    HVX_Vector s1 = vloadu(src_yuy2 + 128);
    vstoreu(dst_y, Q6_V_lo_W(Q6_W_vdeal_VVR(s1, s0, -1)));
    src_yuy2 += 256;
    dst_y += 128;
    width -= 128;
  }
  if (width > 0) {
    int src_bytes = width * 2;
    HVX_Vector s0 =
        vloadu_partial_zero(src_yuy2, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_yuy2 + 128, src_bytes - 128)
                        : Q6_V_vzero();
    vstoreu_partial(dst_y, Q6_V_lo_W(Q6_W_vdeal_VVR(s1, s0, -1)), width);
  }
}

void YUY2ToUV422Row_HVX(const uint8_t* src_yuy2,
                        uint8_t* dst_u,
                        uint8_t* dst_v,
                        int width) {
  int uv_width = (width + 1) >> 1;
  while (uv_width >= 128) {
    HVX_Vector s0 = vloadu(src_yuy2);
    HVX_Vector s1 = vloadu(src_yuy2 + 128);
    HVX_Vector s2 = vloadu(src_yuy2 + 256);
    HVX_Vector s3 = vloadu(src_yuy2 + 384);
    HVX_Vector uv0 = Q6_V_hi_W(Q6_W_vdeal_VVR(s1, s0, -1));
    HVX_Vector uv1 = Q6_V_hi_W(Q6_W_vdeal_VVR(s3, s2, -1));
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv1, uv0, -1);
    vstoreu(dst_u, Q6_V_lo_W(u_v));
    vstoreu(dst_v, Q6_V_hi_W(u_v));
    src_yuy2 += 512;
    dst_u += 128;
    dst_v += 128;
    uv_width -= 128;
  }
  if (uv_width > 0) {
    int src_bytes = uv_width * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_yuy2, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_yuy2 + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 =
        src_bytes > 256
            ? vloadu_partial_zero(src_yuy2 + 256,
                                  src_bytes > 384 ? 128 : src_bytes - 256)
            : Q6_V_vzero();
    HVX_Vector s3 = src_bytes > 384
                        ? vloadu_partial_zero(src_yuy2 + 384, src_bytes - 384)
                        : Q6_V_vzero();
    HVX_Vector uv0 = Q6_V_hi_W(Q6_W_vdeal_VVR(s1, s0, -1));
    HVX_Vector uv1 = Q6_V_hi_W(Q6_W_vdeal_VVR(s3, s2, -1));
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv1, uv0, -1);
    vstoreu_partial(dst_u, Q6_V_lo_W(u_v), uv_width);
    vstoreu_partial(dst_v, Q6_V_hi_W(u_v), uv_width);
  }
}

void YUY2ToUVRow_HVX(const uint8_t* src_yuy2,
                     int src_stride_yuy2,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  int uv_width = (width + 1) >> 1;
  while (uv_width >= 128) {
    HVX_Vector a0 =
        Q6_Vub_vavg_VubVub_rnd(vloadu(src_yuy2), vloadu(src_yuy2_1));
    HVX_Vector a1 = Q6_Vub_vavg_VubVub_rnd(vloadu(src_yuy2 + 128),
                                           vloadu(src_yuy2_1 + 128));
    HVX_Vector a2 = Q6_Vub_vavg_VubVub_rnd(vloadu(src_yuy2 + 256),
                                           vloadu(src_yuy2_1 + 256));
    HVX_Vector a3 = Q6_Vub_vavg_VubVub_rnd(vloadu(src_yuy2 + 384),
                                           vloadu(src_yuy2_1 + 384));
    HVX_Vector uv0 = Q6_V_hi_W(Q6_W_vdeal_VVR(a1, a0, -1));
    HVX_Vector uv1 = Q6_V_hi_W(Q6_W_vdeal_VVR(a3, a2, -1));
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv1, uv0, -1);
    vstoreu(dst_u, Q6_V_lo_W(u_v));
    vstoreu(dst_v, Q6_V_hi_W(u_v));
    src_yuy2 += 512;
    src_yuy2_1 += 512;
    dst_u += 128;
    dst_v += 128;
    uv_width -= 128;
  }
  if (uv_width > 0) {
    int src_bytes = uv_width * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_yuy2, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector t0 =
        vloadu_partial_zero(src_yuy2_1, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector a0 = Q6_Vub_vavg_VubVub_rnd(s0, t0);
    HVX_Vector a1 = Q6_V_vzero();
    HVX_Vector a2 = Q6_V_vzero();
    HVX_Vector a3 = Q6_V_vzero();
    if (src_bytes > 128) {
      int n = src_bytes > 256 ? 128 : src_bytes - 128;
      a1 = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_yuy2 + 128, n),
                                  vloadu_partial_zero(src_yuy2_1 + 128, n));
    }
    if (src_bytes > 256) {
      int n = src_bytes > 384 ? 128 : src_bytes - 256;
      a2 = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_yuy2 + 256, n),
                                  vloadu_partial_zero(src_yuy2_1 + 256, n));
    }
    if (src_bytes > 384) {
      int n = src_bytes - 384;
      a3 = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_yuy2 + 384, n),
                                  vloadu_partial_zero(src_yuy2_1 + 384, n));
    }
    HVX_Vector uv0 = Q6_V_hi_W(Q6_W_vdeal_VVR(a1, a0, -1));
    HVX_Vector uv1 = Q6_V_hi_W(Q6_W_vdeal_VVR(a3, a2, -1));
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv1, uv0, -1);
    vstoreu_partial(dst_u, Q6_V_lo_W(u_v), uv_width);
    vstoreu_partial(dst_v, Q6_V_hi_W(u_v), uv_width);
  }
}

void UYVYToYRow_HVX(const uint8_t* src_uyvy, uint8_t* dst_y, int width) {
  while (width >= 128) {
    HVX_Vector s0 = vloadu(src_uyvy);
    HVX_Vector s1 = vloadu(src_uyvy + 128);
    vstoreu(dst_y, Q6_V_hi_W(Q6_W_vdeal_VVR(s1, s0, -1)));
    src_uyvy += 256;
    dst_y += 128;
    width -= 128;
  }
  if (width > 0) {
    int src_bytes = width * 2;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uyvy, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_uyvy + 128, src_bytes - 128)
                        : Q6_V_vzero();
    vstoreu_partial(dst_y, Q6_V_hi_W(Q6_W_vdeal_VVR(s1, s0, -1)), width);
  }
}

void UYVYToUV422Row_HVX(const uint8_t* src_uyvy,
                        uint8_t* dst_u,
                        uint8_t* dst_v,
                        int width) {
  int uv_width = (width + 1) >> 1;
  while (uv_width >= 128) {
    HVX_Vector s0 = vloadu(src_uyvy);
    HVX_Vector s1 = vloadu(src_uyvy + 128);
    HVX_Vector s2 = vloadu(src_uyvy + 256);
    HVX_Vector s3 = vloadu(src_uyvy + 384);
    HVX_Vector uv0 = Q6_V_lo_W(Q6_W_vdeal_VVR(s1, s0, -1));
    HVX_Vector uv1 = Q6_V_lo_W(Q6_W_vdeal_VVR(s3, s2, -1));
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv1, uv0, -1);
    vstoreu(dst_u, Q6_V_lo_W(u_v));
    vstoreu(dst_v, Q6_V_hi_W(u_v));
    src_uyvy += 512;
    dst_u += 128;
    dst_v += 128;
    uv_width -= 128;
  }
  if (uv_width > 0) {
    int src_bytes = uv_width * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uyvy, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_uyvy + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 =
        src_bytes > 256
            ? vloadu_partial_zero(src_uyvy + 256,
                                  src_bytes > 384 ? 128 : src_bytes - 256)
            : Q6_V_vzero();
    HVX_Vector s3 = src_bytes > 384
                        ? vloadu_partial_zero(src_uyvy + 384, src_bytes - 384)
                        : Q6_V_vzero();
    HVX_Vector uv0 = Q6_V_lo_W(Q6_W_vdeal_VVR(s1, s0, -1));
    HVX_Vector uv1 = Q6_V_lo_W(Q6_W_vdeal_VVR(s3, s2, -1));
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv1, uv0, -1);
    vstoreu_partial(dst_u, Q6_V_lo_W(u_v), uv_width);
    vstoreu_partial(dst_v, Q6_V_hi_W(u_v), uv_width);
  }
}

void UYVYToUVRow_HVX(const uint8_t* src_uyvy,
                     int src_stride_uyvy,
                     uint8_t* dst_u,
                     uint8_t* dst_v,
                     int width) {
  const uint8_t* src_uyvy_1 = src_uyvy + src_stride_uyvy;
  int uv_width = (width + 1) >> 1;
  while (uv_width >= 128) {
    HVX_Vector a0 =
        Q6_Vub_vavg_VubVub_rnd(vloadu(src_uyvy), vloadu(src_uyvy_1));
    HVX_Vector a1 = Q6_Vub_vavg_VubVub_rnd(vloadu(src_uyvy + 128),
                                           vloadu(src_uyvy_1 + 128));
    HVX_Vector a2 = Q6_Vub_vavg_VubVub_rnd(vloadu(src_uyvy + 256),
                                           vloadu(src_uyvy_1 + 256));
    HVX_Vector a3 = Q6_Vub_vavg_VubVub_rnd(vloadu(src_uyvy + 384),
                                           vloadu(src_uyvy_1 + 384));
    HVX_Vector uv0 = Q6_V_lo_W(Q6_W_vdeal_VVR(a1, a0, -1));
    HVX_Vector uv1 = Q6_V_lo_W(Q6_W_vdeal_VVR(a3, a2, -1));
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv1, uv0, -1);
    vstoreu(dst_u, Q6_V_lo_W(u_v));
    vstoreu(dst_v, Q6_V_hi_W(u_v));
    src_uyvy += 512;
    src_uyvy_1 += 512;
    dst_u += 128;
    dst_v += 128;
    uv_width -= 128;
  }
  if (uv_width > 0) {
    int src_bytes = uv_width * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_uyvy, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector t0 =
        vloadu_partial_zero(src_uyvy_1, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector a0 = Q6_Vub_vavg_VubVub_rnd(s0, t0);
    HVX_Vector a1 = Q6_V_vzero();
    HVX_Vector a2 = Q6_V_vzero();
    HVX_Vector a3 = Q6_V_vzero();
    if (src_bytes > 128) {
      int n = src_bytes > 256 ? 128 : src_bytes - 128;
      a1 = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_uyvy + 128, n),
                                  vloadu_partial_zero(src_uyvy_1 + 128, n));
    }
    if (src_bytes > 256) {
      int n = src_bytes > 384 ? 128 : src_bytes - 256;
      a2 = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_uyvy + 256, n),
                                  vloadu_partial_zero(src_uyvy_1 + 256, n));
    }
    if (src_bytes > 384) {
      int n = src_bytes - 384;
      a3 = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_uyvy + 384, n),
                                  vloadu_partial_zero(src_uyvy_1 + 384, n));
    }
    HVX_Vector uv0 = Q6_V_lo_W(Q6_W_vdeal_VVR(a1, a0, -1));
    HVX_Vector uv1 = Q6_V_lo_W(Q6_W_vdeal_VVR(a3, a2, -1));
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv1, uv0, -1);
    vstoreu_partial(dst_u, Q6_V_lo_W(u_v), uv_width);
    vstoreu_partial(dst_v, Q6_V_hi_W(u_v), uv_width);
  }
}

void J400ToARGBRow_HVX(const uint8_t* src_y, uint8_t* dst_argb, int width) {
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_VectorPair yy = Q6_W_vshuff_VVR(y, y, -1);
    HVX_VectorPair ya = Q6_W_vshuff_VVR(a, y, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(ya), Q6_V_lo_W(yy), -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(ya), Q6_V_hi_W(yy), -1);
    int dst_bytes = w * 4;
    vstoreu_partial(dst_argb, Q6_V_lo_W(p01),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_argb + 128, Q6_V_hi_W(p01),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst_argb + 256, Q6_V_lo_W(p23),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
    }
    if (dst_bytes > 384) {
      vstoreu_partial(dst_argb + 384, Q6_V_hi_W(p23), dst_bytes - 384);
    }
    src_y += 128;
    dst_argb += 512;
    width -= 128;
  }
}

void I400ToARGBRow_HVX(const uint8_t* src_y,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  uint16_t yg = (uint16_t)yuvconstants->kYToRgb[0];
  int16_t yb = yuvconstants->kYBiasToRgb[0];
  uint32_t yg32 = ((uint32_t)yg << 16) | yg;
  HVX_Vector v_yb = Q6_Vh_vsplat_R(yb);
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_Vector y_even_0101 = Q6_Vb_vshuffe_VbVb(y, y);
    HVX_Vector y_odd_0101 = Q6_Vb_vshuffo_VbVb(y, y);
    HVX_VectorPair p_even = Q6_Wuw_vmpy_VuhRuh(y_even_0101, yg32);
    HVX_VectorPair p_odd = Q6_Wuw_vmpy_VuhRuh(y_odd_0101, yg32);
    HVX_Vector y_lo = Q6_Vh_vasr_VhR(
        Q6_Vh_vadd_VhVh(
            Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p_even), Q6_V_lo_W(p_even)), v_yb),
        6);
    HVX_Vector y_hi = Q6_Vh_vasr_VhR(
        Q6_Vh_vadd_VhVh(Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p_odd), Q6_V_lo_W(p_odd)),
                        v_yb),
        6);
    HVX_Vector y_out = Q6_Vub_vsat_VhVh(y_hi, y_lo);
    HVX_VectorPair yy = Q6_W_vshuff_VVR(y_out, y_out, -1);
    HVX_VectorPair ya = Q6_W_vshuff_VVR(a, y_out, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(ya), Q6_V_lo_W(yy), -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(ya), Q6_V_hi_W(yy), -1);
    int dst_bytes = w * 4;
    vstoreu_partial(dst_argb, Q6_V_lo_W(p01),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_argb + 128, Q6_V_hi_W(p01),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst_argb + 256, Q6_V_lo_W(p23),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
    }
    if (dst_bytes > 384) {
      vstoreu_partial(dst_argb + 384, Q6_V_hi_W(p23), dst_bytes - 384);
    }
    src_y += 128;
    dst_argb += 512;
    width -= 128;
  }
}

// Packed (vrdelta_ctrl | (vdelta_ctrl << 8)) table for all 256 4-byte shuffles.
// Each 8-bit half packs four 2-bit control lanes (b0 | b1<<2 | b2<<4 | b3<<6).
// Entries with vrdelta_ctrl == 0 execute a single vdelta instruction.
static const uint16_t kARGBShuffleHVXCtrl[256] = {
    0x6400u, 0x6500u, 0x6204u, 0x6d00u, 0x6000u, 0x6100u, 0x6200u, 0x9d05u,
    0x6704u, 0x6700u, 0x6600u, 0x6f00u, 0x6800u, 0x9805u, 0x6a00u, 0x6900u,
    0xf400u, 0xf500u, 0xa205u, 0xfd00u, 0xf000u, 0xf100u, 0xf200u, 0xad05u,
    0xa705u, 0xf700u, 0xf600u, 0xff00u, 0xf800u, 0xa805u, 0xfa00u, 0xf900u,
    0x8004u, 0x8005u, 0x8204u, 0x8d04u, 0x8505u, 0x8405u, 0x7240u, 0x8d05u,
    0x8704u, 0x7740u, 0x8604u, 0x8f04u, 0x8804u, 0x8805u, 0x8a04u, 0x8904u,
    0x7400u, 0x7500u, 0x7204u, 0x7d00u, 0x7000u, 0x7100u, 0x7200u, 0x7210u,
    0x7704u, 0x7700u, 0x7600u, 0x7f00u, 0x7800u, 0x7710u, 0x7a00u, 0x7900u,
    0xa400u, 0xa500u, 0xf205u, 0xad00u, 0xa000u, 0xa100u, 0xa200u, 0xfd05u,
    0xf705u, 0xa700u, 0xa600u, 0xaf00u, 0xa800u, 0xf805u, 0xaa00u, 0xa900u,
    0x9400u, 0x9500u, 0x6205u, 0x9d00u, 0x9000u, 0x9100u, 0x9200u, 0x6d01u,
    0x6705u, 0x9700u, 0x9600u, 0x9f00u, 0x9800u, 0x6801u, 0x9a00u, 0x9900u,
    0x8400u, 0x8500u, 0x8d40u, 0x8d00u, 0x8000u, 0x8100u, 0x8200u, 0x8d01u,
    0x8840u, 0x8700u, 0x8600u, 0x8f00u, 0x8800u, 0x8801u, 0x8a00u, 0x8900u,
    0x7105u, 0x7005u, 0x7205u, 0x8d10u, 0x7505u, 0x7001u, 0x7201u, 0x7d01u,
    0x7705u, 0x7701u, 0x7601u, 0x7f01u, 0x8810u, 0x7801u, 0x7a01u, 0x7901u,
    0xd004u, 0xd005u, 0xd204u, 0xdd04u, 0xd505u, 0xd405u, 0x2240u, 0xdd05u,
    0xd704u, 0x2740u, 0xd604u, 0xdf04u, 0xd804u, 0xd805u, 0xda04u, 0xd904u,
    0xd400u, 0xd500u, 0xdd40u, 0xdd00u, 0xd000u, 0xd100u, 0xd200u, 0xdd01u,
    0xd840u, 0xd700u, 0xd600u, 0xdf00u, 0xd800u, 0xd801u, 0xda00u, 0xd900u,
    0x4400u, 0x4500u, 0x4204u, 0x4d00u, 0x4000u, 0x4100u, 0x4200u, 0x4d01u,
    0x4704u, 0x4700u, 0x4600u, 0x4f00u, 0x4800u, 0x4801u, 0x4a00u, 0x4900u,
    0x5400u, 0x5500u, 0x5204u, 0x5d00u, 0x5000u, 0x5100u, 0x5200u, 0x5d01u,
    0x5704u, 0x5700u, 0x5600u, 0x5f00u, 0x5800u, 0x5801u, 0x5a00u, 0x5900u,
    0x2400u, 0x2500u, 0x2204u, 0x2d00u, 0x2000u, 0x2100u, 0x2200u, 0x2210u,
    0x2704u, 0x2700u, 0x2600u, 0x2f00u, 0x2800u, 0x2710u, 0x2a00u, 0x2900u,
    0x2105u, 0x2005u, 0x2205u, 0xdd10u, 0x2505u, 0x2001u, 0x2201u, 0x2d01u,
    0x2705u, 0x2701u, 0x2601u, 0x2f01u, 0xd810u, 0x2801u, 0x2a01u, 0x2901u,
    0x0400u, 0x0500u, 0x0204u, 0x0d00u, 0x0000u, 0x0100u, 0x0200u, 0x0d01u,
    0x0704u, 0x0700u, 0x0600u, 0x0f00u, 0x0800u, 0x0801u, 0x0a00u, 0x0900u,
    0x1400u, 0x1500u, 0x1204u, 0x1d00u, 0x1000u, 0x1100u, 0x1200u, 0x1d01u,
    0x1704u, 0x1700u, 0x1600u, 0x1f00u, 0x1800u, 0x1801u, 0x1a00u, 0x1900u,
};

static __inline uint32_t UnpackHVXCtrl4(uint8_t packed) {
  return (uint32_t)(packed & 3u) | ((uint32_t)((packed >> 2) & 3u) << 8) |
         ((uint32_t)((packed >> 4) & 3u) << 16) |
         ((uint32_t)((packed >> 6) & 3u) << 24);
}

static __inline HVX_Vector Splat64_HVX(uint64_t val64) {
  uint64_t buf[16];
  for (int i = 0; i < 16; ++i) {
    buf[i] = val64;
  }
  return vloadu(buf);
}

void ARGBShuffleRow_HVX(const uint8_t* src_argb,
                        uint8_t* dst_argb,
                        const uint8_t* shuffler,
                        int width) {
  uint8_t key =
      (uint8_t)((shuffler[0] & 3u) | ((shuffler[1] & 3u) << 2) |
                ((shuffler[2] & 3u) << 4) | ((shuffler[3] & 3u) << 6));
  uint16_t entry = kARGBShuffleHVXCtrl[key];
  uint8_t p0 = (uint8_t)entry;
  HVX_Vector v_c1 =
      Q6_V_vsplat_R((int32_t)UnpackHVXCtrl4((uint8_t)(entry >> 8)));
  int bytes = width * 4;
  if (p0 == 0) {
    while (bytes >= 128) {
      vstoreu(dst_argb, Q6_V_vdelta_VV(vloadu(src_argb), v_c1));
      src_argb += 128;
      dst_argb += 128;
      bytes -= 128;
    }

    if (bytes > 0) {
      HVX_Vector v = vloadu_partial_zero(src_argb, bytes);
      vstoreu_partial(dst_argb, Q6_V_vdelta_VV(v, v_c1), bytes);
    }
  } else {
    HVX_Vector v_c0 = Q6_V_vsplat_R((int32_t)UnpackHVXCtrl4(p0));
    while (bytes >= 128) {
      HVX_Vector v = Q6_V_vrdelta_VV(vloadu(src_argb), v_c0);
      vstoreu(dst_argb, Q6_V_vdelta_VV(v, v_c1));
      src_argb += 128;
      dst_argb += 128;
      bytes -= 128;
    }

    if (bytes > 0) {
      HVX_Vector v =
          Q6_V_vrdelta_VV(vloadu_partial_zero(src_argb, bytes), v_c0);
      vstoreu_partial(dst_argb, Q6_V_vdelta_VV(v, v_c1), bytes);
    }
  }
}

void ARGBToAR64Row_HVX(const uint8_t* src_argb, uint16_t* dst_ar64, int width) {
  Convert8To16Row_HVX(src_argb, dst_ar64, 16, width * 4);
}

void ARGBToAB64Row_HVX(const uint8_t* src_argb, uint16_t* dst_ab64, int width) {
  HVX_Vector ctrl = Q6_V_vsplat_R(0x00020002);
  int bytes = width * 4;
  uint8_t* dst = (uint8_t*)dst_ab64;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    HVX_Vector v = Q6_V_vdelta_VV(vloadu_partial_zero(src_argb, b_cnt), ctrl);
    HVX_VectorPair d16 = Q6_W_vshuff_VVR(v, v, -1);
    int dst_bytes = b_cnt * 2;
    vstoreu_partial(dst, Q6_V_lo_W(d16), dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst + 128, Q6_V_hi_W(d16), dst_bytes - 128);
    }
    src_argb += 128;
    dst += 256;
    bytes -= 128;
  }
}

void AR64ToARGBRow_HVX(const uint16_t* src_ar64, uint8_t* dst_argb, int width) {
  int bytes = width * 4;
  const uint8_t* src = (const uint8_t*)src_ar64;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    int src_bytes = b_cnt * 2;
    HVX_Vector s0 = vloadu_partial_zero(src, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src + 128, src_bytes - 128)
                        : Q6_V_vzero();
    vstoreu_partial(dst_argb, Q6_Vb_vpacko_VhVh(s1, s0), b_cnt);
    src += 256;
    dst_argb += 128;
    bytes -= 128;
  }
}

void AB64ToARGBRow_HVX(const uint16_t* src_ab64, uint8_t* dst_argb, int width) {
  HVX_Vector ctrl = Q6_V_vsplat_R(0x00020002);
  int bytes = width * 4;
  const uint8_t* src = (const uint8_t*)src_ab64;
  while (bytes > 0) {
    int b_cnt = bytes >= 128 ? 128 : bytes;
    int src_bytes = b_cnt * 2;
    HVX_Vector s0 = vloadu_partial_zero(src, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src + 128, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_Vector out = Q6_V_vdelta_VV(Q6_Vb_vpacko_VhVh(s1, s0), ctrl);
    vstoreu_partial(dst_argb, out, b_cnt);
    src += 256;
    dst_argb += 128;
    bytes -= 128;
  }
}

void AR64ShuffleRow_HVX(const uint8_t* src_ar64,
                        uint8_t* dst_ar64,
                        const uint8_t* shuffler,
                        int width) {
  uint8_t s[8];
  uint8_t cand[8];
  uint8_t test8[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  for (int i = 0; i < 8; ++i) {
    s[i] = shuffler[i] & 7u;
    cand[i] = (uint8_t)(s[i] ^ (uint8_t)i);
  }
  for (int st = 4; st >= 1; st >>= 1) {
    uint8_t next8[8];
    for (int i = 0; i < 8; ++i) {
      next8[i] = (cand[i] & st) ? test8[i ^ st] : test8[i];
    }
    memcpy(test8, next8, 8);
  }
  int bytes = width * 8;
  if (memcmp(test8, s, 8) == 0) {
    uint64_t c64;
    memcpy(&c64, cand, sizeof(c64));
    HVX_Vector v_ctrl = Splat64_HVX(c64);
    while (bytes >= 128) {
      vstoreu(dst_ar64, Q6_V_vdelta_VV(vloadu(src_ar64), v_ctrl));
      src_ar64 += 128;
      dst_ar64 += 128;
      bytes -= 128;
    }

    if (bytes > 0) {
      HVX_Vector v = vloadu_partial_zero(src_ar64, bytes);
      vstoreu_partial(dst_ar64, Q6_V_vdelta_VV(v, v_ctrl), bytes);
    }
  } else {
    uint8_t k_lo = (uint8_t)((s[0] & 3u) | ((s[1] & 3u) << 2) |
                             ((s[2] & 3u) << 4) | ((s[3] & 3u) << 6));
    uint8_t k_hi = (uint8_t)((s[4] & 3u) | ((s[5] & 3u) << 2) |
                             ((s[6] & 3u) << 4) | ((s[7] & 3u) << 6));
    uint16_t e0 = kARGBShuffleHVXCtrl[k_lo];
    uint16_t e1 = kARGBShuffleHVXCtrl[k_hi];
    uint64_t c0_64 = (uint64_t)UnpackHVXCtrl4((uint8_t)e0) |
                     ((uint64_t)UnpackHVXCtrl4((uint8_t)e1) << 32);
    uint64_t c1_64 = (uint64_t)UnpackHVXCtrl4((uint8_t)(e0 >> 8)) |
                     ((uint64_t)UnpackHVXCtrl4((uint8_t)(e1 >> 8)) << 32);
    uint64_t sel_64 = 0;
    for (int i = 0; i < 8; ++i) {
      if (s[i] >= 4u) {
        sel_64 |= (0xffULL << (i * 8));
      }
    }
    HVX_Vector v_swap_lo = Splat64_HVX(0x0404040400000000ULL);
    HVX_Vector v_swap_hi = Splat64_HVX(0x0000000004040404ULL);
    HVX_Vector v_c0 = Splat64_HVX(c0_64);
    HVX_Vector v_c1 = Splat64_HVX(c1_64);
    HVX_VectorPred q_hi =
        Q6_Q_vcmp_gt_VubVub(Splat64_HVX(sel_64), Q6_V_vzero());
    while (bytes >= 128) {
      HVX_Vector v = vloadu(src_ar64);
      HVX_Vector lo = Q6_V_vdelta_VV(
          Q6_V_vrdelta_VV(Q6_V_vdelta_VV(v, v_swap_lo), v_c0), v_c1);
      HVX_Vector hi = Q6_V_vdelta_VV(
          Q6_V_vrdelta_VV(Q6_V_vdelta_VV(v, v_swap_hi), v_c0), v_c1);
      vstoreu(dst_ar64, Q6_V_vmux_QVV(q_hi, hi, lo));
      src_ar64 += 128;
      dst_ar64 += 128;
      bytes -= 128;
    }

    if (bytes > 0) {
      HVX_Vector v = vloadu_partial_zero(src_ar64, bytes);
      HVX_Vector lo = Q6_V_vdelta_VV(
          Q6_V_vrdelta_VV(Q6_V_vdelta_VV(v, v_swap_lo), v_c0), v_c1);
      HVX_Vector hi = Q6_V_vdelta_VV(
          Q6_V_vrdelta_VV(Q6_V_vdelta_VV(v, v_swap_hi), v_c0), v_c1);
      vstoreu_partial(dst_ar64, Q6_V_vmux_QVV(q_hi, hi, lo), bytes);
    }
  }
}

static __inline void YuvToRgb_HVX(HVX_Vector y,
                                  HVX_Vector u,
                                  HVX_Vector v,
                                  HVX_Vector* b_out,
                                  HVX_Vector* g_out,
                                  HVX_Vector* r_out,
                                  const struct YuvConstants* yuvconstants) {
  uint8_t ub = yuvconstants->kUVToB[0];
  uint8_t ug = yuvconstants->kUVToG[0];
  uint8_t vg = yuvconstants->kUVToG[1];
  uint8_t vr = yuvconstants->kUVToR[1];
  uint16_t yg = (uint16_t)yuvconstants->kYToRgb[0];
  int16_t yb = yuvconstants->kYBiasToRgb[0];

  uint32_t yg32 = ((uint32_t)yg << 16) | yg;
  HVX_Vector v_yb = Q6_Vh_vsplat_R(yb);
  HVX_Vector v_ub = Q6_Vb_vsplat_R(ub);
  HVX_Vector v_ug = Q6_Vb_vsplat_R(ug);
  HVX_Vector v_vg = Q6_Vb_vsplat_R(vg);
  HVX_Vector v_vr = Q6_Vb_vsplat_R(vr);
  HVX_Vector v80 = Q6_Vb_vsplat_R(0x80);

  HVX_Vector ui = Q6_Vb_vsub_VbVb(u, v80);
  HVX_Vector vi = Q6_Vb_vsub_VbVb(v, v80);

  HVX_Vector y_even_0101 = Q6_Vb_vshuffe_VbVb(y, y);
  HVX_Vector y_odd_0101 = Q6_Vb_vshuffo_VbVb(y, y);
  HVX_VectorPair p_even = Q6_Wuw_vmpy_VuhRuh(y_even_0101, yg32);
  HVX_VectorPair p_odd = Q6_Wuw_vmpy_VuhRuh(y_odd_0101, yg32);
  HVX_Vector y1_lo = Q6_Vh_vadd_VhVh(
      Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p_even), Q6_V_lo_W(p_even)), v_yb);
  HVX_Vector y1_hi = Q6_Vh_vadd_VhVh(
      Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(p_odd), Q6_V_lo_W(p_odd)), v_yb);

  HVX_VectorPair b_uv = Q6_Wh_vmpy_VubVb(v_ub, ui);
  HVX_Vector b_lo =
      Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh_sat(y1_lo, Q6_V_lo_W(b_uv)), 6);
  HVX_Vector b_hi =
      Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh_sat(y1_hi, Q6_V_hi_W(b_uv)), 6);
  *b_out = Q6_Vub_vsat_VhVh(b_hi, b_lo);

  HVX_VectorPair g_uv =
      Q6_Wh_vmpyacc_WhVubVb(Q6_Wh_vmpy_VubVb(v_ug, ui), v_vg, vi);
  HVX_Vector g_lo =
      Q6_Vh_vasr_VhR(Q6_Vh_vsub_VhVh_sat(y1_lo, Q6_V_lo_W(g_uv)), 6);
  HVX_Vector g_hi =
      Q6_Vh_vasr_VhR(Q6_Vh_vsub_VhVh_sat(y1_hi, Q6_V_hi_W(g_uv)), 6);
  *g_out = Q6_Vub_vsat_VhVh(g_hi, g_lo);

  HVX_VectorPair r_uv = Q6_Wh_vmpy_VubVb(v_vr, vi);
  HVX_Vector r_lo =
      Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh_sat(y1_lo, Q6_V_lo_W(r_uv)), 6);
  HVX_Vector r_hi =
      Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh_sat(y1_hi, Q6_V_hi_W(r_uv)), 6);
  *r_out = Q6_Vub_vsat_VhVh(r_hi, r_lo);
}

static __inline void StoreARGB_HVX(uint8_t* dst_argb,
                                   HVX_Vector b,
                                   HVX_Vector g,
                                   HVX_Vector r,
                                   HVX_Vector a,
                                   int w) {
  HVX_VectorPair br = Q6_W_vshuff_VVR(r, b, -1);
  HVX_VectorPair ga = Q6_W_vshuff_VVR(a, g, -1);
  HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(ga), Q6_V_lo_W(br), -1);
  HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(ga), Q6_V_hi_W(br), -1);
  int dst_bytes = w * 4;
  vstoreu_partial(dst_argb, Q6_V_lo_W(p01), dst_bytes > 128 ? 128 : dst_bytes);
  if (dst_bytes > 128) {
    vstoreu_partial(dst_argb + 128, Q6_V_hi_W(p01),
                    dst_bytes > 256 ? 128 : dst_bytes - 128);
  }
  if (dst_bytes > 256) {
    vstoreu_partial(dst_argb + 256, Q6_V_lo_W(p23),
                    dst_bytes > 384 ? 128 : dst_bytes - 256);
  }
  if (dst_bytes > 384) {
    vstoreu_partial(dst_argb + 384, Q6_V_hi_W(p23), dst_bytes - 384);
  }
}

void I444ToARGBRow_HVX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_Vector u = vloadu_partial_zero(src_u, w);
    HVX_Vector v = vloadu_partial_zero(src_v, w);
    HVX_Vector b, g, r;
    YuvToRgb_HVX(y, u, v, &b, &g, &r, yuvconstants);
    StoreARGB_HVX(dst_argb, b, g, r, a, w);
    src_y += 128;
    src_u += 128;
    src_v += 128;
    dst_argb += 512;
    width -= 128;
  }
}

void I444AlphaToARGBRow_HVX(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            const uint8_t* src_a,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_Vector u = vloadu_partial_zero(src_u, w);
    HVX_Vector v = vloadu_partial_zero(src_v, w);
    HVX_Vector a = vloadu_partial_zero(src_a, w);
    HVX_Vector b, g, r;
    YuvToRgb_HVX(y, u, v, &b, &g, &r, yuvconstants);
    StoreARGB_HVX(dst_argb, b, g, r, a, w);
    src_y += 128;
    src_u += 128;
    src_v += 128;
    src_a += 128;
    dst_argb += 512;
    width -= 128;
  }
}

void I422ToARGBRow_HVX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int uv_w = (w + 1) >> 1;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_Vector u_sub = vloadu_partial_zero(src_u, uv_w);
    HVX_Vector v_sub = vloadu_partial_zero(src_v, uv_w);
    HVX_Vector u = Q6_V_lo_W(Q6_W_vshuff_VVR(u_sub, u_sub, -1));
    HVX_Vector v = Q6_V_lo_W(Q6_W_vshuff_VVR(v_sub, v_sub, -1));
    HVX_Vector b, g, r;
    YuvToRgb_HVX(y, u, v, &b, &g, &r, yuvconstants);
    StoreARGB_HVX(dst_argb, b, g, r, a, w);
    src_y += 128;
    src_u += 64;
    src_v += 64;
    dst_argb += 512;
    width -= 128;
  }
}

void I422AlphaToARGBRow_HVX(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            const uint8_t* src_a,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int uv_w = (w + 1) >> 1;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_Vector u_sub = vloadu_partial_zero(src_u, uv_w);
    HVX_Vector v_sub = vloadu_partial_zero(src_v, uv_w);
    HVX_Vector a = vloadu_partial_zero(src_a, w);
    HVX_Vector u = Q6_V_lo_W(Q6_W_vshuff_VVR(u_sub, u_sub, -1));
    HVX_Vector v = Q6_V_lo_W(Q6_W_vshuff_VVR(v_sub, v_sub, -1));
    HVX_Vector b, g, r;
    YuvToRgb_HVX(y, u, v, &b, &g, &r, yuvconstants);
    StoreARGB_HVX(dst_argb, b, g, r, a, w);
    src_y += 128;
    src_u += 64;
    src_v += 64;
    src_a += 128;
    dst_argb += 512;
    width -= 128;
  }
}

void I422ToRGBARow_HVX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_rgba,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int uv_w = (w + 1) >> 1;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_Vector u_sub = vloadu_partial_zero(src_u, uv_w);
    HVX_Vector v_sub = vloadu_partial_zero(src_v, uv_w);
    HVX_Vector u = Q6_V_lo_W(Q6_W_vshuff_VVR(u_sub, u_sub, -1));
    HVX_Vector v = Q6_V_lo_W(Q6_W_vshuff_VVR(v_sub, v_sub, -1));
    HVX_Vector b, g, r;
    YuvToRgb_HVX(y, u, v, &b, &g, &r, yuvconstants);
    StoreARGB_HVX(dst_rgba, a, b, g, r, w);
    src_y += 128;
    src_u += 64;
    src_v += 64;
    dst_rgba += 512;
    width -= 128;
  }
}

void NV12ToARGBRow_HVX(const uint8_t* src_y,
                       const uint8_t* src_uv,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int uv_bytes = ((w + 1) >> 1) * 2;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_Vector uv = vloadu_partial_zero(src_uv, uv_bytes);
    HVX_VectorPair u_v = Q6_W_vdeal_VVR(uv, uv, -1);
    HVX_Vector u_sub = Q6_V_lo_W(u_v);
    HVX_Vector v_sub = Q6_V_hi_W(u_v);
    HVX_Vector u = Q6_V_lo_W(Q6_W_vshuff_VVR(u_sub, u_sub, -1));
    HVX_Vector v = Q6_V_lo_W(Q6_W_vshuff_VVR(v_sub, v_sub, -1));
    HVX_Vector b, g, r;
    YuvToRgb_HVX(y, u, v, &b, &g, &r, yuvconstants);
    StoreARGB_HVX(dst_argb, b, g, r, a, w);
    src_y += 128;
    src_uv += 128;
    dst_argb += 512;
    width -= 128;
  }
}

void NV21ToARGBRow_HVX(const uint8_t* src_y,
                       const uint8_t* src_vu,
                       uint8_t* dst_argb,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  HVX_Vector a = Q6_Vb_vsplat_R(0xff);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int uv_bytes = ((w + 1) >> 1) * 2;
    HVX_Vector y = vloadu_partial_zero(src_y, w);
    HVX_Vector vu = vloadu_partial_zero(src_vu, uv_bytes);
    HVX_VectorPair v_u = Q6_W_vdeal_VVR(vu, vu, -1);
    HVX_Vector v_sub = Q6_V_lo_W(v_u);
    HVX_Vector u_sub = Q6_V_hi_W(v_u);
    HVX_Vector u = Q6_V_lo_W(Q6_W_vshuff_VVR(u_sub, u_sub, -1));
    HVX_Vector v = Q6_V_lo_W(Q6_W_vshuff_VVR(v_sub, v_sub, -1));
    HVX_Vector b, g, r;
    YuvToRgb_HVX(y, u, v, &b, &g, &r, yuvconstants);
    StoreARGB_HVX(dst_argb, b, g, r, a, w);
    src_y += 128;
    src_vu += 128;
    dst_argb += 512;
    width -= 128;
  }
}

void ARGBToYMatrixRow_HVX(const uint8_t* src_argb,
                          uint8_t* dst_y,
                          int width,
                          const struct ArgbConstants* c) {
  uint32_t coeff_y;
  uint16_t add_y;
  memcpy(&coeff_y, c->kRGBToY, sizeof(coeff_y));
  memcpy(&add_y, c->kAddY, sizeof(add_y));
  HVX_Vector v_add_y = Q6_V_vsplat_R((uint32_t)add_y);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_argb, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_argb + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 =
        src_bytes > 256
            ? vloadu_partial_zero(src_argb + 256,
                                  src_bytes > 384 ? 128 : src_bytes - 256)
            : Q6_V_vzero();
    HVX_Vector s3 = src_bytes > 384
                        ? vloadu_partial_zero(src_argb + 384, src_bytes - 384)
                        : Q6_V_vzero();
    HVX_Vector y0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vadd_VwVw(Q6_Vuw_vrmpy_VubRub(s0, coeff_y), v_add_y), 8);
    HVX_Vector y1 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vadd_VwVw(Q6_Vuw_vrmpy_VubRub(s1, coeff_y), v_add_y), 8);
    HVX_Vector y2 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vadd_VwVw(Q6_Vuw_vrmpy_VubRub(s2, coeff_y), v_add_y), 8);
    HVX_Vector y3 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vadd_VwVw(Q6_Vuw_vrmpy_VubRub(s3, coeff_y), v_add_y), 8);
    HVX_Vector y01 = Q6_Vh_vpacke_VwVw(y1, y0);
    HVX_Vector y23 = Q6_Vh_vpacke_VwVw(y3, y2);
    HVX_Vector out = Q6_Vb_vpacke_VhVh(y23, y01);
    vstoreu_partial(dst_y, out, w);
    src_argb += 512;
    dst_y += 128;
    width -= 128;
  }
}

void ARGBToUV444MatrixRow_HVX(const uint8_t* src_argb,
                              uint8_t* dst_u,
                              uint8_t* dst_v,
                              int width,
                              const struct ArgbConstants* c) {
  int32_t coeff_u;
  int32_t coeff_v;
  uint16_t add_uv;
  memcpy(&coeff_u, c->kRGBToU, sizeof(coeff_u));
  memcpy(&coeff_v, c->kRGBToV, sizeof(coeff_v));
  memcpy(&add_uv, c->kAddUV, sizeof(add_uv));
  HVX_Vector v_add_uv = Q6_V_vsplat_R((uint32_t)add_uv);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_argb, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_argb + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 =
        src_bytes > 256
            ? vloadu_partial_zero(src_argb + 256,
                                  src_bytes > 384 ? 128 : src_bytes - 256)
            : Q6_V_vzero();
    HVX_Vector s3 = src_bytes > 384
                        ? vloadu_partial_zero(src_argb + 384, src_bytes - 384)
                        : Q6_V_vzero();

    HVX_Vector u0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s0, coeff_u)), 8);
    HVX_Vector u1 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s1, coeff_u)), 8);
    HVX_Vector u2 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s2, coeff_u)), 8);
    HVX_Vector u3 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s3, coeff_u)), 8);
    HVX_Vector u_out =
        Q6_Vb_vpacke_VhVh(Q6_Vh_vpacke_VwVw(u3, u2), Q6_Vh_vpacke_VwVw(u1, u0));
    vstoreu_partial(dst_u, u_out, w);

    HVX_Vector v0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s0, coeff_v)), 8);
    HVX_Vector v1 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s1, coeff_v)), 8);
    HVX_Vector v2 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s2, coeff_v)), 8);
    HVX_Vector v3 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s3, coeff_v)), 8);
    HVX_Vector v_out =
        Q6_Vb_vpacke_VhVh(Q6_Vh_vpacke_VwVw(v3, v2), Q6_Vh_vpacke_VwVw(v1, v0));
    vstoreu_partial(dst_v, v_out, w);

    src_argb += 512;
    dst_u += 128;
    dst_v += 128;
    width -= 128;
  }
}

void ARGBToUVMatrixRow_HVX(const uint8_t* src_argb,
                           int src_stride_argb,
                           uint8_t* dst_u,
                           uint8_t* dst_v,
                           int width,
                           const struct ArgbConstants* c) {
  int32_t coeff_u;
  int32_t coeff_v;
  uint16_t add_uv;
  memcpy(&coeff_u, c->kRGBToU, sizeof(coeff_u));
  memcpy(&coeff_v, c->kRGBToV, sizeof(coeff_v));
  memcpy(&add_uv, c->kAddUV, sizeof(add_uv));
  HVX_Vector v_add_uv = Q6_V_vsplat_R((uint32_t)add_uv);
  HVX_Vector vtwo = Q6_Vh_vsplat_R(2);
  const uint8_t* src_argb1 = src_argb + src_stride_argb;

  int even_width = width & ~1;
  while (even_width > 0) {
    int w = even_width >= 64 ? 64 : even_width;
    int uv_w = w >> 1;
    int src_bytes = w * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_argb, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 = src_bytes > 128
                        ? vloadu_partial_zero(src_argb + 128, src_bytes - 128)
                        : Q6_V_vzero();
    HVX_Vector t0 =
        vloadu_partial_zero(src_argb1, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector t1 = src_bytes > 128
                        ? vloadu_partial_zero(src_argb1 + 128, src_bytes - 128)
                        : Q6_V_vzero();

    HVX_VectorPair ds = Q6_W_vdeal_VVR(s1, s0, -4);
    HVX_VectorPair dt = Q6_W_vdeal_VVR(t1, t0, -4);
    HVX_VectorPair s_even = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(ds));
    HVX_VectorPair s_odd = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(ds));
    HVX_VectorPair t_even = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(dt));
    HVX_VectorPair t_odd = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(dt));

    HVX_Vector sum_lo = Q6_Vh_vadd_VhVh(Q6_V_lo_W(s_even), Q6_V_lo_W(s_odd));
    sum_lo = Q6_Vh_vadd_VhVh(sum_lo, Q6_V_lo_W(t_even));
    sum_lo = Q6_Vh_vadd_VhVh(sum_lo, Q6_V_lo_W(t_odd));
    sum_lo = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sum_lo, vtwo), 2);

    HVX_Vector sum_hi = Q6_Vh_vadd_VhVh(Q6_V_hi_W(s_even), Q6_V_hi_W(s_odd));
    sum_hi = Q6_Vh_vadd_VhVh(sum_hi, Q6_V_hi_W(t_even));
    sum_hi = Q6_Vh_vadd_VhVh(sum_hi, Q6_V_hi_W(t_odd));
    sum_hi = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sum_hi, vtwo), 2);

    HVX_Vector avg = Q6_Vub_vpack_VhVh_sat(sum_hi, sum_lo);
    HVX_Vector u0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(avg, coeff_u)), 8);
    HVX_Vector v0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(avg, coeff_v)), 8);
    HVX_Vector u_out =
        Q6_Vb_vpacke_VhVh(Q6_V_vzero(), Q6_Vh_vpacke_VwVw(Q6_V_vzero(), u0));
    HVX_Vector v_out =
        Q6_Vb_vpacke_VhVh(Q6_V_vzero(), Q6_Vh_vpacke_VwVw(Q6_V_vzero(), v0));
    vstoreu_partial(dst_u, u_out, uv_w);
    vstoreu_partial(dst_v, v_out, uv_w);

    src_argb += w * 4;
    src_argb1 += w * 4;
    dst_u += uv_w;
    dst_v += uv_w;
    even_width -= w;
  }
  if (width & 1) {
    HVX_Vector s0 = vloadu_partial_zero(src_argb, 4);
    HVX_Vector t0 = vloadu_partial_zero(src_argb1, 4);
    HVX_Vector avg = Q6_Vub_vavg_VubVub_rnd(s0, t0);
    HVX_Vector u0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(avg, coeff_u)), 8);
    HVX_Vector v0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(avg, coeff_v)), 8);
    vstoreu_partial(dst_u, u0, 1);
    vstoreu_partial(dst_v, v0, 1);
  }
}

// vdelta control vector to expand 32 RGB24 pixels (96 bytes) into 32 4-byte
// lanes (out[4*i + c] = in[3*i + c] for c = 0..2).
static const uint8_t kRGB24ToARGBCtrl[128] __attribute__((aligned(128))) = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x01, 0x06, 0x02, 0x02, 0x06, 0x04,
    0x03, 0x05, 0x0d, 0x0c, 0x04, 0x04, 0x04, 0x00, 0x0d, 0x0f, 0x09, 0x0a,
    0x06, 0x02, 0x0a, 0x08, 0x1b, 0x19, 0x19, 0x18, 0x08, 0x08, 0x08, 0x08,
    0x09, 0x0b, 0x01, 0x06, 0x1a, 0x1a, 0x1e, 0x1c, 0x13, 0x15, 0x15, 0x14,
    0x0c, 0x0c, 0x04, 0x00, 0x15, 0x17, 0x11, 0x12, 0x36, 0x32, 0x32, 0x30,
    0x33, 0x31, 0x31, 0x30, 0x10, 0x10, 0x10, 0x10, 0x11, 0x13, 0x11, 0x16,
    0x12, 0x12, 0x16, 0x14, 0x03, 0x05, 0x0d, 0x0c, 0x34, 0x34, 0x34, 0x30,
    0x3d, 0x3f, 0x39, 0x3a, 0x26, 0x22, 0x2a, 0x28, 0x2b, 0x29, 0x29, 0x28,
    0x18, 0x18, 0x18, 0x18, 0x09, 0x0b, 0x01, 0x06, 0x2a, 0x2a, 0x2e, 0x2c,
    0x23, 0x25, 0x25, 0x24, 0x6c, 0x6c, 0x64, 0x60, 0x65, 0x67, 0x61, 0x62,
    0x66, 0x62, 0x62, 0x60, 0x63, 0x61, 0x61, 0x60,
};

static __inline HVX_Vector ExpandRGB24ToARGB_HVX(const uint8_t* src_rgb,
                                                 int n_pixels,
                                                 HVX_Vector v_ctrl,
                                                 HVX_Vector v_rgb_mask) {
  if (n_pixels <= 0) {
    return Q6_V_vzero();
  }
  int n_bytes = n_pixels >= 32 ? 96 : n_pixels * 3;
  HVX_Vector raw = vloadu_partial_zero(src_rgb, n_bytes);
  return Q6_V_vand_VV(Q6_V_vdelta_VV(raw, v_ctrl), v_rgb_mask);
}

void RGBToYMatrixRow_HVX(const uint8_t* src_rgb,
                         uint8_t* dst_y,
                         int width,
                         const struct ArgbConstants* c) {
  uint32_t coeff_y;
  uint16_t add_y;
  memcpy(&coeff_y, c->kRGBToY, sizeof(coeff_y));
  memcpy(&add_y, c->kAddY, sizeof(add_y));
  HVX_Vector v_add_y = Q6_V_vsplat_R((uint32_t)add_y);
  HVX_Vector v_ctrl = vloada(kRGB24ToARGBCtrl);
  HVX_Vector v_rgb_mask = Q6_V_vsplat_R(0x00FFFFFFu);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector s0 = ExpandRGB24ToARGB_HVX(src_rgb, w, v_ctrl, v_rgb_mask);
    HVX_Vector s1 =
        ExpandRGB24ToARGB_HVX(src_rgb + 96, w - 32, v_ctrl, v_rgb_mask);
    HVX_Vector s2 =
        ExpandRGB24ToARGB_HVX(src_rgb + 192, w - 64, v_ctrl, v_rgb_mask);
    HVX_Vector s3 =
        ExpandRGB24ToARGB_HVX(src_rgb + 288, w - 96, v_ctrl, v_rgb_mask);
    HVX_Vector y0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vadd_VwVw(Q6_Vuw_vrmpy_VubRub(s0, coeff_y), v_add_y), 8);
    HVX_Vector y1 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vadd_VwVw(Q6_Vuw_vrmpy_VubRub(s1, coeff_y), v_add_y), 8);
    HVX_Vector y2 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vadd_VwVw(Q6_Vuw_vrmpy_VubRub(s2, coeff_y), v_add_y), 8);
    HVX_Vector y3 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vadd_VwVw(Q6_Vuw_vrmpy_VubRub(s3, coeff_y), v_add_y), 8);
    HVX_Vector y01 = Q6_Vh_vpacke_VwVw(y1, y0);
    HVX_Vector y23 = Q6_Vh_vpacke_VwVw(y3, y2);
    HVX_Vector out = Q6_Vb_vpacke_VhVh(y23, y01);
    vstoreu_partial(dst_y, out, w);
    src_rgb += 384;
    dst_y += 128;
    width -= 128;
  }
}

void RGBToUV444MatrixRow_HVX(const uint8_t* src_rgb,
                             uint8_t* dst_u,
                             uint8_t* dst_v,
                             int width,
                             const struct ArgbConstants* c) {
  int32_t coeff_u;
  int32_t coeff_v;
  uint16_t add_uv;
  memcpy(&coeff_u, c->kRGBToU, sizeof(coeff_u));
  memcpy(&coeff_v, c->kRGBToV, sizeof(coeff_v));
  memcpy(&add_uv, c->kAddUV, sizeof(add_uv));
  HVX_Vector v_add_uv = Q6_V_vsplat_R((uint32_t)add_uv);
  HVX_Vector v_ctrl = vloada(kRGB24ToARGBCtrl);
  HVX_Vector v_rgb_mask = Q6_V_vsplat_R(0x00FFFFFFu);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector s0 = ExpandRGB24ToARGB_HVX(src_rgb, w, v_ctrl, v_rgb_mask);
    HVX_Vector s1 =
        ExpandRGB24ToARGB_HVX(src_rgb + 96, w - 32, v_ctrl, v_rgb_mask);
    HVX_Vector s2 =
        ExpandRGB24ToARGB_HVX(src_rgb + 192, w - 64, v_ctrl, v_rgb_mask);
    HVX_Vector s3 =
        ExpandRGB24ToARGB_HVX(src_rgb + 288, w - 96, v_ctrl, v_rgb_mask);

    HVX_Vector u0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s0, coeff_u)), 8);
    HVX_Vector u1 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s1, coeff_u)), 8);
    HVX_Vector u2 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s2, coeff_u)), 8);
    HVX_Vector u3 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s3, coeff_u)), 8);
    HVX_Vector u_out =
        Q6_Vb_vpacke_VhVh(Q6_Vh_vpacke_VwVw(u3, u2), Q6_Vh_vpacke_VwVw(u1, u0));
    vstoreu_partial(dst_u, u_out, w);

    HVX_Vector v0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s0, coeff_v)), 8);
    HVX_Vector v1 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s1, coeff_v)), 8);
    HVX_Vector v2 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s2, coeff_v)), 8);
    HVX_Vector v3 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(s3, coeff_v)), 8);
    HVX_Vector v_out =
        Q6_Vb_vpacke_VhVh(Q6_Vh_vpacke_VwVw(v3, v2), Q6_Vh_vpacke_VwVw(v1, v0));
    vstoreu_partial(dst_v, v_out, w);

    src_rgb += 384;
    dst_u += 128;
    dst_v += 128;
    width -= 128;
  }
}

void RGBToUVMatrixRow_HVX(const uint8_t* src_rgb,
                          int src_stride_rgb,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width,
                          const struct ArgbConstants* c) {
  int32_t coeff_u;
  int32_t coeff_v;
  uint16_t add_uv;
  memcpy(&coeff_u, c->kRGBToU, sizeof(coeff_u));
  memcpy(&coeff_v, c->kRGBToV, sizeof(coeff_v));
  memcpy(&add_uv, c->kAddUV, sizeof(add_uv));
  HVX_Vector v_add_uv = Q6_V_vsplat_R((uint32_t)add_uv);
  HVX_Vector v_ctrl = vloada(kRGB24ToARGBCtrl);
  HVX_Vector v_rgb_mask = Q6_V_vsplat_R(0x00FFFFFFu);
  HVX_Vector vtwo = Q6_Vh_vsplat_R(2);
  const uint8_t* src_rgb1 = src_rgb + src_stride_rgb;

  int even_width = width & ~1;
  while (even_width > 0) {
    int w = even_width >= 64 ? 64 : even_width;
    int uv_w = w >> 1;
    HVX_Vector s0 = ExpandRGB24ToARGB_HVX(src_rgb, w, v_ctrl, v_rgb_mask);
    HVX_Vector s1 =
        ExpandRGB24ToARGB_HVX(src_rgb + 96, w - 32, v_ctrl, v_rgb_mask);
    HVX_Vector t0 = ExpandRGB24ToARGB_HVX(src_rgb1, w, v_ctrl, v_rgb_mask);
    HVX_Vector t1 =
        ExpandRGB24ToARGB_HVX(src_rgb1 + 96, w - 32, v_ctrl, v_rgb_mask);

    HVX_VectorPair ds = Q6_W_vdeal_VVR(s1, s0, -4);
    HVX_VectorPair dt = Q6_W_vdeal_VVR(t1, t0, -4);
    HVX_VectorPair s_even = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(ds));
    HVX_VectorPair s_odd = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(ds));
    HVX_VectorPair t_even = Q6_Wuh_vunpack_Vub(Q6_V_lo_W(dt));
    HVX_VectorPair t_odd = Q6_Wuh_vunpack_Vub(Q6_V_hi_W(dt));

    HVX_Vector sum_lo = Q6_Vh_vadd_VhVh(Q6_V_lo_W(s_even), Q6_V_lo_W(s_odd));
    sum_lo = Q6_Vh_vadd_VhVh(sum_lo, Q6_V_lo_W(t_even));
    sum_lo = Q6_Vh_vadd_VhVh(sum_lo, Q6_V_lo_W(t_odd));
    sum_lo = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sum_lo, vtwo), 2);

    HVX_Vector sum_hi = Q6_Vh_vadd_VhVh(Q6_V_hi_W(s_even), Q6_V_hi_W(s_odd));
    sum_hi = Q6_Vh_vadd_VhVh(sum_hi, Q6_V_hi_W(t_even));
    sum_hi = Q6_Vh_vadd_VhVh(sum_hi, Q6_V_hi_W(t_odd));
    sum_hi = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sum_hi, vtwo), 2);

    HVX_Vector avg = Q6_Vub_vpack_VhVh_sat(sum_hi, sum_lo);
    HVX_Vector u0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(avg, coeff_u)), 8);
    HVX_Vector v0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(avg, coeff_v)), 8);
    HVX_Vector u_out =
        Q6_Vb_vpacke_VhVh(Q6_V_vzero(), Q6_Vh_vpacke_VwVw(Q6_V_vzero(), u0));
    HVX_Vector v_out =
        Q6_Vb_vpacke_VhVh(Q6_V_vzero(), Q6_Vh_vpacke_VwVw(Q6_V_vzero(), v0));
    vstoreu_partial(dst_u, u_out, uv_w);
    vstoreu_partial(dst_v, v_out, uv_w);

    src_rgb += w * 3;
    src_rgb1 += w * 3;
    dst_u += uv_w;
    dst_v += uv_w;
    even_width -= w;
  }
  if (width & 1) {
    HVX_Vector s0 = vloadu_partial_zero(src_rgb, 3);
    HVX_Vector t0 = vloadu_partial_zero(src_rgb1, 3);
    HVX_Vector avg = Q6_Vub_vavg_VubVub_rnd(s0, t0);
    HVX_Vector u0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(avg, coeff_u)), 8);
    HVX_Vector v0 = Q6_Vuw_vlsr_VuwR(
        Q6_Vw_vsub_VwVw(v_add_uv, Q6_Vw_vrmpy_VubRb(avg, coeff_v)), 8);
    vstoreu_partial(dst_u, u0, 1);
    vstoreu_partial(dst_v, v0, 1);
  }
}

void SplitRGBRow_HVX(const uint8_t* src_rgb,
                     uint8_t* dst_r,
                     uint8_t* dst_g,
                     uint8_t* dst_b,
                     int width) {
  HVX_Vector v_ctrl = vloada(kRGB24ToARGBCtrl);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    int src_bytes = w * 3;
    HVX_Vector s0 =
        vloadu_partial_zero(src_rgb, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_rgb + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 = src_bytes > 256
                        ? vloadu_partial_zero(src_rgb + 256, src_bytes - 256)
                        : Q6_V_vzero();
    // Expand each 96 bytes of RGB to 32 pixels of RGBX.
    HVX_Vector p0 = Q6_V_vdelta_VV(s0, v_ctrl);
    HVX_Vector p1 = Q6_V_vdelta_VV(Q6_V_valign_VVR(s1, s0, 96), v_ctrl);
    HVX_Vector p2 = Q6_V_vdelta_VV(Q6_V_valign_VVR(s2, s1, 64), v_ctrl);
    HVX_Vector p3 = Q6_V_vdelta_VV(Q6_V_valign_VVR(s2, s2, 32), v_ctrl);
    HVX_VectorPair p01 = Q6_W_vdeal_VVR(p1, p0, -1);
    HVX_VectorPair p23 = Q6_W_vdeal_VVR(p3, p2, -1);
    HVX_VectorPair rb = Q6_W_vdeal_VVR(Q6_V_lo_W(p23), Q6_V_lo_W(p01), -1);
    HVX_VectorPair gx = Q6_W_vdeal_VVR(Q6_V_hi_W(p23), Q6_V_hi_W(p01), -1);
    vstoreu_partial(dst_r, Q6_V_lo_W(rb), w);
    vstoreu_partial(dst_g, Q6_V_lo_W(gx), w);
    vstoreu_partial(dst_b, Q6_V_hi_W(rb), w);
    src_rgb += 384;
    dst_r += 128;
    dst_g += 128;
    dst_b += 128;
    width -= 128;
  }
}

// vrdelta control vectors to pack 32 4-byte lanes into 32 RGB24 pixels
// (96 bytes) starting at output byte 0, 96, 64 or 32, wrapping at 128
// (out[(start + 3*i + c) & 127] = in[4*i + c] for c = 0..2).
static const uint8_t kARGBToRGB24Ctrl0[128] __attribute__((aligned(128))) = {
    0x00, 0x00, 0x00, 0x04, 0x01, 0x03, 0x08, 0x0b, 0x02, 0x04, 0x06, 0x06,
    0x11, 0x11, 0x16, 0x15, 0x04, 0x04, 0x08, 0x08, 0x0d, 0x0f, 0x0c, 0x0b,
    0x22, 0x20, 0x22, 0x22, 0x2d, 0x29, 0x2a, 0x29, 0x08, 0x08, 0x08, 0x0c,
    0x11, 0x13, 0x10, 0x13, 0x1a, 0x1c, 0x1e, 0x1e, 0x19, 0x19, 0x16, 0x15,
    0x44, 0x44, 0x40, 0x40, 0x45, 0x47, 0x44, 0x43, 0x5a, 0x58, 0x52, 0x52,
    0x55, 0x51, 0x52, 0x51, 0x10, 0x10, 0x10, 0x14, 0x11, 0x13, 0x18, 0x1b,
    0x22, 0x24, 0x26, 0x26, 0x21, 0x21, 0x26, 0x25, 0x34, 0x34, 0x38, 0x38,
    0x3d, 0x3f, 0x3c, 0x3b, 0x32, 0x30, 0x32, 0x32, 0x2d, 0x29, 0x2a, 0x29,
    0x08, 0x08, 0x08, 0x0c, 0x01, 0x03, 0x00, 0x03, 0x0a, 0x0c, 0x0e, 0x0e,
    0x09, 0x09, 0x06, 0x05, 0x34, 0x34, 0x30, 0x30, 0x25, 0x27, 0x24, 0x23,
    0x2a, 0x28, 0x22, 0x22, 0x25, 0x21, 0x22, 0x21,
};
static const uint8_t kARGBToRGB24Ctrl96[128] __attribute__((aligned(128))) = {
    0x20, 0x20, 0x20, 0x24, 0x21, 0x23, 0x28, 0x2b, 0x22, 0x24, 0x26, 0x26,
    0x31, 0x31, 0x36, 0x35, 0x44, 0x44, 0x48, 0x48, 0x4d, 0x4f, 0x4c, 0x4b,
    0x42, 0x40, 0x42, 0x42, 0x4d, 0x49, 0x4a, 0x49, 0x68, 0x68, 0x68, 0x6c,
    0x71, 0x73, 0x70, 0x73, 0x7a, 0x7c, 0x7e, 0x7e, 0x79, 0x79, 0x76, 0x75,
    0x64, 0x64, 0x60, 0x60, 0x65, 0x67, 0x64, 0x63, 0x5a, 0x58, 0x52, 0x52,
    0x55, 0x51, 0x52, 0x51, 0x10, 0x10, 0x10, 0x14, 0x11, 0x13, 0x18, 0x1b,
    0x02, 0x04, 0x06, 0x06, 0x01, 0x01, 0x06, 0x05, 0x14, 0x14, 0x18, 0x18,
    0x1d, 0x1f, 0x1c, 0x1b, 0x12, 0x10, 0x12, 0x12, 0x0d, 0x09, 0x0a, 0x09,
    0x68, 0x68, 0x68, 0x6c, 0x61, 0x63, 0x60, 0x63, 0x4a, 0x4c, 0x4e, 0x4e,
    0x49, 0x49, 0x46, 0x45, 0x54, 0x54, 0x50, 0x50, 0x45, 0x47, 0x44, 0x43,
    0x4a, 0x48, 0x42, 0x42, 0x45, 0x41, 0x42, 0x41,
};
static const uint8_t kARGBToRGB24Ctrl64[128] __attribute__((aligned(128))) = {
    0x40, 0x40, 0x40, 0x44, 0x41, 0x43, 0x48, 0x4b, 0x42, 0x44, 0x46, 0x46,
    0x51, 0x51, 0x56, 0x55, 0x44, 0x44, 0x48, 0x48, 0x4d, 0x4f, 0x4c, 0x4b,
    0x62, 0x60, 0x62, 0x62, 0x6d, 0x69, 0x6a, 0x69, 0x08, 0x08, 0x08, 0x0c,
    0x11, 0x13, 0x10, 0x13, 0x1a, 0x1c, 0x1e, 0x1e, 0x19, 0x19, 0x16, 0x15,
    0x04, 0x04, 0x00, 0x00, 0x05, 0x07, 0x04, 0x03, 0x1a, 0x18, 0x12, 0x12,
    0x15, 0x11, 0x12, 0x11, 0x50, 0x50, 0x50, 0x54, 0x51, 0x53, 0x58, 0x5b,
    0x62, 0x64, 0x66, 0x66, 0x61, 0x61, 0x66, 0x65, 0x74, 0x74, 0x78, 0x78,
    0x7d, 0x7f, 0x7c, 0x7b, 0x72, 0x70, 0x72, 0x72, 0x6d, 0x69, 0x6a, 0x69,
    0x48, 0x48, 0x48, 0x4c, 0x41, 0x43, 0x40, 0x43, 0x4a, 0x4c, 0x4e, 0x4e,
    0x49, 0x49, 0x46, 0x45, 0x34, 0x34, 0x30, 0x30, 0x25, 0x27, 0x24, 0x23,
    0x2a, 0x28, 0x22, 0x22, 0x25, 0x21, 0x22, 0x21,
};
static const uint8_t kARGBToRGB24Ctrl32[128] __attribute__((aligned(128))) = {
    0x20, 0x20, 0x20, 0x24, 0x21, 0x23, 0x28, 0x2b, 0x22, 0x24, 0x26, 0x26,
    0x31, 0x31, 0x36, 0x35, 0x04, 0x04, 0x08, 0x08, 0x0d, 0x0f, 0x0c, 0x0b,
    0x02, 0x00, 0x02, 0x02, 0x0d, 0x09, 0x0a, 0x09, 0x28, 0x28, 0x28, 0x2c,
    0x31, 0x33, 0x30, 0x33, 0x3a, 0x3c, 0x3e, 0x3e, 0x39, 0x39, 0x36, 0x35,
    0x24, 0x24, 0x20, 0x20, 0x25, 0x27, 0x24, 0x23, 0x1a, 0x18, 0x12, 0x12,
    0x15, 0x11, 0x12, 0x11, 0x50, 0x50, 0x50, 0x54, 0x51, 0x53, 0x58, 0x5b,
    0x42, 0x44, 0x46, 0x46, 0x41, 0x41, 0x46, 0x45, 0x14, 0x14, 0x18, 0x18,
    0x1d, 0x1f, 0x1c, 0x1b, 0x12, 0x10, 0x12, 0x12, 0x0d, 0x09, 0x0a, 0x09,
    0x28, 0x28, 0x28, 0x2c, 0x21, 0x23, 0x20, 0x23, 0x0a, 0x0c, 0x0e, 0x0e,
    0x09, 0x09, 0x06, 0x05, 0x14, 0x14, 0x10, 0x10, 0x05, 0x07, 0x04, 0x03,
    0x0a, 0x08, 0x02, 0x02, 0x05, 0x01, 0x02, 0x01,
};

void MergeRGBRow_HVX(const uint8_t* src_r,
                     const uint8_t* src_g,
                     const uint8_t* src_b,
                     uint8_t* dst_rgb,
                     int width) {
  HVX_Vector v_ctrl0 = vloada(kARGBToRGB24Ctrl0);
  HVX_Vector v_ctrl1 = vloada(kARGBToRGB24Ctrl96);
  HVX_Vector v_ctrl2 = vloada(kARGBToRGB24Ctrl64);
  HVX_Vector v_ctrl3 = vloada(kARGBToRGB24Ctrl32);
  HVX_VectorPred q96 = Q6_Q_vsetq_R(96);
  HVX_VectorPred q64 = Q6_Q_vsetq_R(64);
  HVX_VectorPred q32 = Q6_Q_vsetq_R(32);
  while (width > 0) {
    int w = width >= 128 ? 128 : width;
    HVX_Vector r = vloadu_partial_zero(src_r, w);
    HVX_Vector g = vloadu_partial_zero(src_g, w);
    HVX_Vector b = vloadu_partial_zero(src_b, w);
    HVX_VectorPair rb = Q6_W_vshuff_VVR(b, r, -1);
    HVX_VectorPair gx = Q6_W_vshuff_VVR(g, g, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(gx), Q6_V_lo_W(rb), -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(gx), Q6_V_hi_W(rb), -1);
    // Pack each 32 pixels of RGBX to 96 bytes of RGB, rotated so that
    // consecutive vectors can be combined with a mux.
    HVX_Vector p0 = Q6_V_vrdelta_VV(Q6_V_lo_W(p01), v_ctrl0);
    HVX_Vector p1 = Q6_V_vrdelta_VV(Q6_V_hi_W(p01), v_ctrl1);
    HVX_Vector p2 = Q6_V_vrdelta_VV(Q6_V_lo_W(p23), v_ctrl2);
    HVX_Vector p3 = Q6_V_vrdelta_VV(Q6_V_hi_W(p23), v_ctrl3);
    int dst_bytes = w * 3;
    vstoreu_partial(dst_rgb, Q6_V_vmux_QVV(q96, p0, p1),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_rgb + 128, Q6_V_vmux_QVV(q64, p1, p2),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst_rgb + 256, Q6_V_vmux_QVV(q32, p2, p3),
                      dst_bytes - 256);
    }
    src_r += 128;
    src_g += 128;
    src_b += 128;
    dst_rgb += 384;
    width -= 128;
  }
}

void RGB24MirrorRow_HVX(const uint8_t* src_rgb24,
                        uint8_t* dst_rgb24,
                        int width) {
  HVX_Vector v_exp = vloada(kRGB24ToARGBCtrl);
  HVX_Vector v_rgb_mask = Q6_V_vsplat_R(0x00FFFFFFu);
  HVX_Vector v_rev = Q6_Vb_vsplat_R(0x7C);
  HVX_Vector v_pack0 = vloada(kARGBToRGB24Ctrl0);
  HVX_Vector v_pack1 = vloada(kARGBToRGB24Ctrl96);
  HVX_Vector v_pack2 = vloada(kARGBToRGB24Ctrl64);
  HVX_Vector v_pack3 = vloada(kARGBToRGB24Ctrl32);
  HVX_VectorPred q96 = Q6_Q_vsetq_R(96);
  HVX_VectorPred q64 = Q6_Q_vsetq_R(64);
  HVX_VectorPred q32 = Q6_Q_vsetq_R(32);
  src_rgb24 += (ptrdiff_t)width * 3;
  while (width >= 128) {
    src_rgb24 -= 384;
    HVX_Vector s3 = Q6_V_vdelta_VV(
        ExpandRGB24ToARGB_HVX(src_rgb24 + 288, 32, v_exp, v_rgb_mask), v_rev);
    HVX_Vector s2 = Q6_V_vdelta_VV(
        ExpandRGB24ToARGB_HVX(src_rgb24 + 192, 32, v_exp, v_rgb_mask), v_rev);
    HVX_Vector s1 = Q6_V_vdelta_VV(
        ExpandRGB24ToARGB_HVX(src_rgb24 + 96, 32, v_exp, v_rgb_mask), v_rev);
    HVX_Vector s0 = Q6_V_vdelta_VV(
        ExpandRGB24ToARGB_HVX(src_rgb24, 32, v_exp, v_rgb_mask), v_rev);
    HVX_Vector p0 = Q6_V_vrdelta_VV(s3, v_pack0);
    HVX_Vector p1 = Q6_V_vrdelta_VV(s2, v_pack1);
    HVX_Vector p2 = Q6_V_vrdelta_VV(s1, v_pack2);
    HVX_Vector p3 = Q6_V_vrdelta_VV(s0, v_pack3);
    vstoreu(dst_rgb24, Q6_V_vmux_QVV(q96, p0, p1));
    vstoreu(dst_rgb24 + 128, Q6_V_vmux_QVV(q64, p1, p2));
    vstoreu(dst_rgb24 + 256, Q6_V_vmux_QVV(q32, p2, p3));
    dst_rgb24 += 384;
    width -= 128;
  }
  while (width > 0) {
    int w = width >= 32 ? 32 : width;
    int bytes = w * 3;
    src_rgb24 -= bytes;
    HVX_Vector s = Q6_V_vdelta_VV(
        ExpandRGB24ToARGB_HVX(src_rgb24, w, v_exp, v_rgb_mask), v_rev);
    if (w < 32) {
      s = Q6_V_valign_VVR(s, s, (32 - w) * 4);
    }
    HVX_Vector p = Q6_V_vrdelta_VV(s, v_pack0);
    vstoreu_partial(dst_rgb24, p, bytes);
    dst_rgb24 += bytes;
    width -= w;
  }
}

void HalfMergeUVRow_HVX(const uint8_t* src_u,
                        int src_stride_u,
                        const uint8_t* src_v,
                        int src_stride_v,
                        uint8_t* dst_uv,
                        int width) {
  const uint8_t* src_u1 = src_u + src_stride_u;
  const uint8_t* src_v1 = src_v + src_stride_v;
  const int32_t kOnes = 0x01010101;
  HVX_Vector vtwo = Q6_Vh_vsplat_R(2);
  int pairs = width >> 1;
  while (pairs >= 64) {
    HVX_Vector su = Q6_Vh_vdmpy_VubRb(vloadu(src_u), kOnes);
    su = Q6_Vh_vdmpyacc_VhVubRb(su, vloadu(src_u1), kOnes);
    su = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(su, vtwo), 2);
    HVX_Vector sv = Q6_Vh_vdmpy_VubRb(vloadu(src_v), kOnes);
    sv = Q6_Vh_vdmpyacc_VhVubRb(sv, vloadu(src_v1), kOnes);
    sv = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sv, vtwo), 2);
    vstoreu(dst_uv, Q6_Vb_vshuffe_VbVb(sv, su));
    src_u += 128;
    src_u1 += 128;
    src_v += 128;
    src_v1 += 128;
    dst_uv += 128;
    pairs -= 64;
  }
  if (pairs > 0) {
    int n = pairs * 2;
    HVX_Vector su = Q6_Vh_vdmpy_VubRb(vloadu_partial_zero(src_u, n), kOnes);
    su = Q6_Vh_vdmpyacc_VhVubRb(su, vloadu_partial_zero(src_u1, n), kOnes);
    su = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(su, vtwo), 2);
    HVX_Vector sv = Q6_Vh_vdmpy_VubRb(vloadu_partial_zero(src_v, n), kOnes);
    sv = Q6_Vh_vdmpyacc_VhVubRb(sv, vloadu_partial_zero(src_v1, n), kOnes);
    sv = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sv, vtwo), 2);
    vstoreu_partial(dst_uv, Q6_Vb_vshuffe_VbVb(sv, su), n);
    src_u += n;
    src_u1 += n;
    src_v += n;
    src_v1 += n;
    dst_uv += n;
  }
  if (width & 1) {
    dst_uv[0] = (uint8_t)((src_u[0] + src_u1[0] + 1) >> 1);
    dst_uv[1] = (uint8_t)((src_v[0] + src_v1[0] + 1) >> 1);
  }
}

void YUY2ToNVUVRow_HVX(const uint8_t* src_yuy2,
                       int src_stride_yuy2,
                       uint8_t* dst_uv,
                       int width) {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  int uv_bytes = ((width + 1) >> 1) * 2;
  while (uv_bytes >= 128) {
    HVX_Vector a0 =
        Q6_Vub_vavg_VubVub_rnd(vloadu(src_yuy2), vloadu(src_yuy2_1));
    HVX_Vector a1 = Q6_Vub_vavg_VubVub_rnd(vloadu(src_yuy2 + 128),
                                           vloadu(src_yuy2_1 + 128));
    vstoreu(dst_uv, Q6_V_hi_W(Q6_W_vdeal_VVR(a1, a0, -1)));
    src_yuy2 += 256;
    src_yuy2_1 += 256;
    dst_uv += 128;
    uv_bytes -= 128;
  }
  if (uv_bytes > 0) {
    int src_bytes = uv_bytes * 2;
    int n0 = src_bytes > 128 ? 128 : src_bytes;
    HVX_Vector a0 = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_yuy2, n0),
                                           vloadu_partial_zero(src_yuy2_1, n0));
    HVX_Vector a1 = Q6_V_vzero();
    if (src_bytes > 128) {
      int n1 = src_bytes - 128;
      a1 = Q6_Vub_vavg_VubVub_rnd(vloadu_partial_zero(src_yuy2 + 128, n1),
                                  vloadu_partial_zero(src_yuy2_1 + 128, n1));
    }
    vstoreu_partial(dst_uv, Q6_V_hi_W(Q6_W_vdeal_VVR(a1, a0, -1)), uv_bytes);
  }
}

void AYUVToYRow_HVX(const uint8_t* src_ayuv, uint8_t* dst_y, int width) {
  while (width >= 128) {
    HVX_Vector s0 = vloadu(src_ayuv);
    HVX_Vector s1 = vloadu(src_ayuv + 128);
    HVX_Vector s2 = vloadu(src_ayuv + 256);
    HVX_Vector s3 = vloadu(src_ayuv + 384);
    HVX_VectorPair p01 = Q6_W_vdeal_VVR(s1, s0, -1);
    HVX_VectorPair p23 = Q6_W_vdeal_VVR(s3, s2, -1);
    HVX_VectorPair vy = Q6_W_vdeal_VVR(Q6_V_lo_W(p23), Q6_V_lo_W(p01), -1);
    vstoreu(dst_y, Q6_V_hi_W(vy));
    src_ayuv += 512;
    dst_y += 128;
    width -= 128;
  }
  if (width > 0) {
    int src_bytes = width * 4;
    HVX_Vector s0 =
        vloadu_partial_zero(src_ayuv, src_bytes > 128 ? 128 : src_bytes);
    HVX_Vector s1 =
        src_bytes > 128
            ? vloadu_partial_zero(src_ayuv + 128,
                                  src_bytes > 256 ? 128 : src_bytes - 128)
            : Q6_V_vzero();
    HVX_Vector s2 =
        src_bytes > 256
            ? vloadu_partial_zero(src_ayuv + 256,
                                  src_bytes > 384 ? 128 : src_bytes - 256)
            : Q6_V_vzero();
    HVX_Vector s3 = src_bytes > 384
                        ? vloadu_partial_zero(src_ayuv + 384, src_bytes - 384)
                        : Q6_V_vzero();
    HVX_VectorPair p01 = Q6_W_vdeal_VVR(s1, s0, -1);
    HVX_VectorPair p23 = Q6_W_vdeal_VVR(s3, s2, -1);
    HVX_VectorPair vy = Q6_W_vdeal_VVR(Q6_V_lo_W(p23), Q6_V_lo_W(p01), -1);
    vstoreu_partial(dst_y, Q6_V_hi_W(vy), width);
  }
}

static __inline void AYUVToUVOrVURow_HVX(const uint8_t* src_ayuv,
                                         int src_stride_ayuv,
                                         uint8_t* dst_uv,
                                         int width,
                                         int swap_uv) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  const int32_t kOnes = 0x01010101;
  HVX_Vector vtwo = Q6_Vh_vsplat_R(2);
  int pairs = width >> 1;
  while (pairs >= 64) {
    HVX_Vector vu0a =
        Q6_V_lo_W(Q6_W_vdeal_VVR(vloadu(src_ayuv + 128), vloadu(src_ayuv), -2));
    HVX_Vector vu0b = Q6_V_lo_W(
        Q6_W_vdeal_VVR(vloadu(src_ayuv + 384), vloadu(src_ayuv + 256), -2));
    HVX_VectorPair r0 = Q6_W_vdeal_VVR(vu0b, vu0a, -1);
    HVX_Vector vu1a = Q6_V_lo_W(
        Q6_W_vdeal_VVR(vloadu(src_ayuv1 + 128), vloadu(src_ayuv1), -2));
    HVX_Vector vu1b = Q6_V_lo_W(
        Q6_W_vdeal_VVR(vloadu(src_ayuv1 + 384), vloadu(src_ayuv1 + 256), -2));
    HVX_VectorPair r1 = Q6_W_vdeal_VVR(vu1b, vu1a, -1);
    HVX_Vector sv = Q6_Vh_vdmpy_VubRb(Q6_V_lo_W(r0), kOnes);
    sv = Q6_Vh_vdmpyacc_VhVubRb(sv, Q6_V_lo_W(r1), kOnes);
    sv = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sv, vtwo), 2);
    HVX_Vector su = Q6_Vh_vdmpy_VubRb(Q6_V_hi_W(r0), kOnes);
    su = Q6_Vh_vdmpyacc_VhVubRb(su, Q6_V_hi_W(r1), kOnes);
    su = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(su, vtwo), 2);
    vstoreu(dst_uv,
            swap_uv ? Q6_Vb_vshuffe_VbVb(sv, su) : Q6_Vb_vshuffe_VbVb(su, sv));
    src_ayuv += 512;
    src_ayuv1 += 512;
    dst_uv += 128;
    pairs -= 64;
  }
  if (pairs > 0) {
    int src_bytes = pairs * 8;
    int dst_bytes = pairs * 2;
    int n0 = src_bytes > 128 ? 128 : src_bytes;
    int n1 = src_bytes > 128 ? (src_bytes > 256 ? 128 : src_bytes - 128) : 0;
    int n2 = src_bytes > 256 ? (src_bytes > 384 ? 128 : src_bytes - 256) : 0;
    int n3 = src_bytes > 384 ? src_bytes - 384 : 0;
    HVX_Vector vu0a =
        Q6_V_lo_W(Q6_W_vdeal_VVR(vloadu_partial_zero(src_ayuv + 128, n1),
                                 vloadu_partial_zero(src_ayuv, n0), -2));
    HVX_Vector vu0b =
        Q6_V_lo_W(Q6_W_vdeal_VVR(vloadu_partial_zero(src_ayuv + 384, n3),
                                 vloadu_partial_zero(src_ayuv + 256, n2), -2));
    HVX_VectorPair r0 = Q6_W_vdeal_VVR(vu0b, vu0a, -1);
    HVX_Vector vu1a =
        Q6_V_lo_W(Q6_W_vdeal_VVR(vloadu_partial_zero(src_ayuv1 + 128, n1),
                                 vloadu_partial_zero(src_ayuv1, n0), -2));
    HVX_Vector vu1b =
        Q6_V_lo_W(Q6_W_vdeal_VVR(vloadu_partial_zero(src_ayuv1 + 384, n3),
                                 vloadu_partial_zero(src_ayuv1 + 256, n2), -2));
    HVX_VectorPair r1 = Q6_W_vdeal_VVR(vu1b, vu1a, -1);
    HVX_Vector sv = Q6_Vh_vdmpy_VubRb(Q6_V_lo_W(r0), kOnes);
    sv = Q6_Vh_vdmpyacc_VhVubRb(sv, Q6_V_lo_W(r1), kOnes);
    sv = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(sv, vtwo), 2);
    HVX_Vector su = Q6_Vh_vdmpy_VubRb(Q6_V_hi_W(r0), kOnes);
    su = Q6_Vh_vdmpyacc_VhVubRb(su, Q6_V_hi_W(r1), kOnes);
    su = Q6_Vh_vasr_VhR(Q6_Vh_vadd_VhVh(su, vtwo), 2);
    vstoreu_partial(
        dst_uv,
        swap_uv ? Q6_Vb_vshuffe_VbVb(sv, su) : Q6_Vb_vshuffe_VbVb(su, sv),
        dst_bytes);
    src_ayuv += src_bytes;
    src_ayuv1 += src_bytes;
    dst_uv += dst_bytes;
  }
  if (width & 1) {
    if (swap_uv) {
      dst_uv[0] = (uint8_t)((src_ayuv[1] + src_ayuv1[1] + 1) >> 1);
      dst_uv[1] = (uint8_t)((src_ayuv[0] + src_ayuv1[0] + 1) >> 1);
    } else {
      dst_uv[0] = (uint8_t)((src_ayuv[0] + src_ayuv1[0] + 1) >> 1);
      dst_uv[1] = (uint8_t)((src_ayuv[1] + src_ayuv1[1] + 1) >> 1);
    }
  }
}

void AYUVToUVRow_HVX(const uint8_t* src_ayuv,
                     int src_stride_ayuv,
                     uint8_t* dst_uv,
                     int width) {
  AYUVToUVOrVURow_HVX(src_ayuv, src_stride_ayuv, dst_uv, width, 1);
}

void AYUVToVURow_HVX(const uint8_t* src_ayuv,
                     int src_stride_ayuv,
                     uint8_t* dst_vu,
                     int width) {
  AYUVToUVOrVURow_HVX(src_ayuv, src_stride_ayuv, dst_vu, width, 0);
}

void I422ToYUY2Row_HVX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_frame,
                       int width) {
  int pairs = width >> 1;
  while (pairs >= 128) {
    HVX_Vector y0 = vloadu(src_y);
    HVX_Vector y1 = vloadu(src_y + 128);
    HVX_Vector u = vloadu(src_u);
    HVX_Vector v = vloadu(src_v);
    HVX_VectorPair uv = Q6_W_vshuff_VVR(v, u, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(uv), y0, -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(uv), y1, -1);
    vstoreu(dst_frame, Q6_V_lo_W(p01));
    vstoreu(dst_frame + 128, Q6_V_hi_W(p01));
    vstoreu(dst_frame + 256, Q6_V_lo_W(p23));
    vstoreu(dst_frame + 384, Q6_V_hi_W(p23));
    src_y += 256;
    src_u += 128;
    src_v += 128;
    dst_frame += 512;
    pairs -= 128;
  }
  if (pairs > 0) {
    int y_bytes = pairs * 2;
    int dst_bytes = pairs * 4;
    HVX_Vector y0 = vloadu_partial_zero(src_y, y_bytes > 128 ? 128 : y_bytes);
    HVX_Vector y1 = y_bytes > 128
                        ? vloadu_partial_zero(src_y + 128, y_bytes - 128)
                        : Q6_V_vzero();
    HVX_Vector u = vloadu_partial_zero(src_u, pairs);
    HVX_Vector v = vloadu_partial_zero(src_v, pairs);
    HVX_VectorPair uv = Q6_W_vshuff_VVR(v, u, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(Q6_V_lo_W(uv), y0, -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(Q6_V_hi_W(uv), y1, -1);
    vstoreu_partial(dst_frame, Q6_V_lo_W(p01),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_frame + 128, Q6_V_hi_W(p01),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst_frame + 256, Q6_V_lo_W(p23),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
    }
    if (dst_bytes > 384) {
      vstoreu_partial(dst_frame + 384, Q6_V_hi_W(p23), dst_bytes - 384);
    }
    src_y += y_bytes;
    src_u += pairs;
    src_v += pairs;
    dst_frame += dst_bytes;
  }
  if (width & 1) {
    dst_frame[0] = src_y[0];
    dst_frame[1] = src_u[0];
    dst_frame[2] = 0;
    dst_frame[3] = src_v[0];
  }
}

void I422ToUYVYRow_HVX(const uint8_t* src_y,
                       const uint8_t* src_u,
                       const uint8_t* src_v,
                       uint8_t* dst_frame,
                       int width) {
  int pairs = width >> 1;
  while (pairs >= 128) {
    HVX_Vector y0 = vloadu(src_y);
    HVX_Vector y1 = vloadu(src_y + 128);
    HVX_Vector u = vloadu(src_u);
    HVX_Vector v = vloadu(src_v);
    HVX_VectorPair uv = Q6_W_vshuff_VVR(v, u, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(y0, Q6_V_lo_W(uv), -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(y1, Q6_V_hi_W(uv), -1);
    vstoreu(dst_frame, Q6_V_lo_W(p01));
    vstoreu(dst_frame + 128, Q6_V_hi_W(p01));
    vstoreu(dst_frame + 256, Q6_V_lo_W(p23));
    vstoreu(dst_frame + 384, Q6_V_hi_W(p23));
    src_y += 256;
    src_u += 128;
    src_v += 128;
    dst_frame += 512;
    pairs -= 128;
  }
  if (pairs > 0) {
    int y_bytes = pairs * 2;
    int dst_bytes = pairs * 4;
    HVX_Vector y0 = vloadu_partial_zero(src_y, y_bytes > 128 ? 128 : y_bytes);
    HVX_Vector y1 = y_bytes > 128
                        ? vloadu_partial_zero(src_y + 128, y_bytes - 128)
                        : Q6_V_vzero();
    HVX_Vector u = vloadu_partial_zero(src_u, pairs);
    HVX_Vector v = vloadu_partial_zero(src_v, pairs);
    HVX_VectorPair uv = Q6_W_vshuff_VVR(v, u, -1);
    HVX_VectorPair p01 = Q6_W_vshuff_VVR(y0, Q6_V_lo_W(uv), -1);
    HVX_VectorPair p23 = Q6_W_vshuff_VVR(y1, Q6_V_hi_W(uv), -1);
    vstoreu_partial(dst_frame, Q6_V_lo_W(p01),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_frame + 128, Q6_V_hi_W(p01),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      vstoreu_partial(dst_frame + 256, Q6_V_lo_W(p23),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
    }
    if (dst_bytes > 384) {
      vstoreu_partial(dst_frame + 384, Q6_V_hi_W(p23), dst_bytes - 384);
    }
    src_y += y_bytes;
    src_u += pairs;
    src_v += pairs;
    dst_frame += dst_bytes;
  }
  if (width & 1) {
    dst_frame[0] = src_u[0];
    dst_frame[1] = src_y[0];
    dst_frame[2] = src_v[0];
    dst_frame[3] = 0;
  }
}

#if defined(HAS_HALFFLOATROW_HVX) || defined(HAS_BYTETOFLOATROW_HVX)
#if defined(__HVX_ARCH__) && __HVX_ARCH__ < 81
// Exact IEEE-754 float32 product of uint16_t values (in uint32_t lanes) by a
// normal finite float scale (1 <= exp_s <= 238), avoiding the 1-ULP rounding
// error of pre-v81 qf32 multiply instructions.
static __inline HVX_Vector hvx_u16_mul_scale_f32_exact(HVX_Vector u32,
                                                       HVX_Vector vm_hi,
                                                       HVX_Vector vm_lo,
                                                       HVX_Vector vexp_base,
                                                       HVX_Vector vsign) {
  const HVX_Vector vzero = Q6_V_vzero();
  const HVX_Vector vone = Q6_V_vsplat_R(1);
  const HVX_Vector v7 = Q6_V_vsplat_R(7);
  const HVX_Vector v8 = Q6_V_vsplat_R(8);
  const HVX_Vector v24 = Q6_V_vsplat_R(24);
  const HVX_Vector v0x80 = Q6_V_vsplat_R(0x80);
  const HVX_Vector v0xff = Q6_V_vsplat_R(0xff);
  HVX_Vector hi = Q6_Vw_vmpyie_VwVuh(vm_hi, u32);
  HVX_Vector lo = Q6_Vw_vmpyie_VwVuh(vm_lo, u32);
  HVX_Vector p_hi = Q6_Vw_vadd_VwVw(hi, Q6_Vuw_vlsr_VuwR(lo, 8));
  HVX_Vector p_lo = Q6_V_vand_VV(lo, v0xff);
  HVX_Vector lz = Q6_Vuw_vcl0_Vuw(p_hi);
  HVX_Vector top = Q6_Vw_vasl_VwVw(p_hi, lz);
  HVX_VectorPred lz_ge_8 = Q6_Q_vcmp_gt_VwVw(lz, v7);
  HVX_Vector lo_in =
      Q6_V_vmux_QVV(lz_ge_8, Q6_Vw_vasl_VwVw(p_lo, Q6_Vw_vsub_VwVw(lz, v8)),
                    Q6_Vw_vasr_VwVw(p_lo, Q6_Vw_vsub_VwVw(v8, lz)));
  HVX_Vector lo_out = Q6_V_vmux_QVV(
      lz_ge_8, vzero, Q6_Vw_vasl_VwVw(p_lo, Q6_Vw_vadd_VwVw(v24, lz)));
  HVX_Vector norm = Q6_V_vor_VV(top, lo_in);
  HVX_Vector mant24 = Q6_Vuw_vlsr_VuwR(norm, 8);
  HVX_Vector has_lo =
      Q6_V_vmux_QVV(Q6_Q_vcmp_eq_VwVw(lo_out, vzero), vzero, vone);
  HVX_Vector rem = Q6_V_vor_VV(Q6_V_vand_VV(norm, v0xff), has_lo);
  HVX_VectorPred round_up = Q6_Q_vcmp_gt_VuwVuw(
      Q6_Vw_vadd_VwVw(rem, Q6_V_vand_VV(mant24, vone)), v0x80);
  HVX_Vector mant =
      Q6_Vw_vadd_VwVw(mant24, Q6_V_vmux_QVV(round_up, vone, vzero));
  HVX_Vector exp_bits = Q6_Vw_vasl_VwR(Q6_Vw_vsub_VwVw(vexp_base, lz), 23);
  HVX_Vector f32_abs = Q6_Vw_vadd_VwVw(exp_bits, mant);
  HVX_VectorPred is_zero = Q6_Q_vcmp_eq_VwVw(u32, vzero);
  return Q6_V_vor_VV(Q6_V_vmux_QVV(is_zero, vzero, f32_abs), vsign);
}
#endif
#endif

#ifdef HAS_HALFFLOATROW_HVX
#if defined(__HVX_ARCH__) && __HVX_ARCH__ >= 81
static __inline HVX_Vector hvx_f32_to_f16_bits(HVX_Vector u32,
                                               HVX_Vector vscale,
                                               HVX_Vector v_min_norm,
                                               HVX_Vector v_exp_bias) {
  // Multiply in normal float32 range, then bias exponent without producing
  // float32 subnormals (which HVX flushes to zero). For x < 2^-14, adding
  // 2^-14 aligns x to ULP 2^-37 == 2^-149 / 2^-112 before >> 13.
  HVX_Vector x = Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(u32), vscale);
  HVX_VectorPred is_sub = Q6_Q_vcmp_gt_VuwVuw(v_min_norm, x);
  HVX_Vector norm = Q6_Vw_vsub_VwVw(x, v_exp_bias);
  HVX_Vector sub =
      Q6_Vw_vsub_VwVw(Q6_Vsf_vadd_VsfVsf(x, v_min_norm), v_min_norm);
  HVX_Vector res = Q6_V_vmux_QVV(is_sub, sub, norm);
  return Q6_Vuw_vlsr_VuwR(res, 13);
}
#else
static __inline HVX_Vector hvx_f32_to_f16_bits_exact(HVX_Vector x) {
  const HVX_Vector vzero = Q6_V_vzero();
  const HVX_Vector vone = Q6_V_vsplat_R(1);
  const HVX_Vector v24 = Q6_V_vsplat_R(24);
  const HVX_Vector v32 = Q6_V_vsplat_R(32);
  const HVX_Vector v113 = Q6_V_vsplat_R(113);
  const HVX_Vector v_min_norm = Q6_V_vsplat_R(0x38800000u);  // 2^-14
  const HVX_Vector v_exp_bias = Q6_V_vsplat_R(0x38000000u);  // 112 << 23
  const HVX_Vector v_mant_mask = Q6_V_vsplat_R(0x007fffffu);
  const HVX_Vector v_hidden = Q6_V_vsplat_R(0x00800000u);
  const HVX_Vector v_half = Q6_V_vsplat_R(0x80000000u);
  HVX_VectorPred is_zero = Q6_Q_vcmp_eq_VwVw(x, vzero);
  HVX_VectorPred is_sub = Q6_Q_vcmp_gt_VuwVuw(v_min_norm, x);
  HVX_Vector norm_fp16 = Q6_Vuw_vlsr_VuwR(Q6_Vw_vsub_VwVw(x, v_exp_bias), 13);
  HVX_Vector exp_e = Q6_Vuw_vlsr_VuwR(x, 23);
  HVX_Vector shift = Q6_Vw_vsub_VwVw(v113, exp_e);
  HVX_Vector sig = Q6_V_vor_VV(Q6_V_vand_VV(x, v_mant_mask), v_hidden);
  HVX_Vector q = Q6_Vw_vasr_VwVw(sig, shift);
  HVX_Vector rem = Q6_Vw_vasl_VwVw(sig, Q6_Vw_vsub_VwVw(v32, shift));
  HVX_VectorPred gt_half = Q6_Q_vcmp_gt_VuwVuw(rem, v_half);
  HVX_VectorPred eq_half = Q6_Q_vcmp_eq_VwVw(rem, v_half);
  HVX_VectorPred q_odd = Q6_Q_vcmp_eq_VwVw(Q6_V_vand_VV(q, vone), vone);
  HVX_VectorPred sub_ru = Q6_Q_or_QQ(gt_half, Q6_Q_and_QQ(eq_half, q_odd));
  HVX_Vector sub_f32 = Q6_Vw_vadd_VwVw(q, Q6_V_vmux_QVV(sub_ru, vone, vzero));
  HVX_VectorPred shift_ok = Q6_Q_vcmp_gt_VwVw(v24, shift);
  HVX_Vector sub_fp16 =
      Q6_V_vmux_QVV(shift_ok, Q6_Vuw_vlsr_VuwR(sub_f32, 13), vzero);
  return Q6_V_vmux_QVV(is_zero, vzero,
                       Q6_V_vmux_QVV(is_sub, sub_fp16, norm_fp16));
}
#endif

void HalfFloatRow_HVX(const uint16_t* src,
                      uint16_t* dst,
                      float scale,
                      int width) {
  uint32_t scale_bits;
  memcpy(&scale_bits, &scale, sizeof(scale_bits));
#if defined(__HVX_ARCH__) && __HVX_ARCH__ >= 81
  HVX_Vector vscale = Q6_V_vsplat_R(scale_bits);
  HVX_Vector v_min_norm = Q6_V_vsplat_R(0x38800000u);  // 2^-14
  HVX_Vector v_exp_bias = Q6_V_vsplat_R(0x38000000u);  // 112 << 23
  while (width >= 64) {
    HVX_VectorPair u = Q6_Wuw_vunpack_Vuh(vloadu(src));
    HVX_Vector lo =
        hvx_f32_to_f16_bits(Q6_V_lo_W(u), vscale, v_min_norm, v_exp_bias);
    HVX_Vector hi =
        hvx_f32_to_f16_bits(Q6_V_hi_W(u), vscale, v_min_norm, v_exp_bias);
    vstoreu(dst, Q6_Vh_vpacke_VwVw(hi, lo));
    src += 64;
    dst += 64;
    width -= 64;
  }
  if (width > 0) {
    int bytes = width * 2;
    HVX_VectorPair u = Q6_Wuw_vunpack_Vuh(vloadu_partial_zero(src, bytes));
    HVX_Vector lo =
        hvx_f32_to_f16_bits(Q6_V_lo_W(u), vscale, v_min_norm, v_exp_bias);
    HVX_Vector hi =
        hvx_f32_to_f16_bits(Q6_V_hi_W(u), vscale, v_min_norm, v_exp_bias);
    vstoreu_partial(dst, Q6_Vh_vpacke_VwVw(hi, lo), bytes);
  }
#else
  int exp_s = (int)((scale_bits >> 23) & 0xffu);
  if ((scale_bits & 0x80000000u) || exp_s < 1 || exp_s > 238) {
    HalfFloatRow_C(src, dst, scale, width);
    return;
  }
  uint32_t m = (scale_bits & 0x007fffffu) | 0x00800000u;
  HVX_Vector vm_hi = Q6_V_vsplat_R(m >> 8);
  HVX_Vector vm_lo = Q6_V_vsplat_R(m & 0xffu);
  HVX_Vector vexp_base = Q6_V_vsplat_R(exp_s + 15);
  HVX_Vector vsign = Q6_V_vzero();
  while (width >= 64) {
    HVX_VectorPair u = Q6_Wuw_vunpack_Vuh(vloadu(src));
    HVX_Vector lo = hvx_f32_to_f16_bits_exact(hvx_u16_mul_scale_f32_exact(
        Q6_V_lo_W(u), vm_hi, vm_lo, vexp_base, vsign));
    HVX_Vector hi = hvx_f32_to_f16_bits_exact(hvx_u16_mul_scale_f32_exact(
        Q6_V_hi_W(u), vm_hi, vm_lo, vexp_base, vsign));
    vstoreu(dst, Q6_Vh_vpacke_VwVw(hi, lo));
    src += 64;
    dst += 64;
    width -= 64;
  }
  if (width > 0) {
    int bytes = width * 2;
    HVX_VectorPair u = Q6_Wuw_vunpack_Vuh(vloadu_partial_zero(src, bytes));
    HVX_Vector lo = hvx_f32_to_f16_bits_exact(hvx_u16_mul_scale_f32_exact(
        Q6_V_lo_W(u), vm_hi, vm_lo, vexp_base, vsign));
    HVX_Vector hi = hvx_f32_to_f16_bits_exact(hvx_u16_mul_scale_f32_exact(
        Q6_V_hi_W(u), vm_hi, vm_lo, vexp_base, vsign));
    vstoreu_partial(dst, Q6_Vh_vpacke_VwVw(hi, lo), bytes);
  }
#endif
}
#endif  // HAS_HALFFLOATROW_HVX

#ifdef HAS_BYTETOFLOATROW_HVX
void ByteToFloatRow_HVX(const uint8_t* src,
                        float* dst,
                        float scale,
                        int width) {
  uint32_t scale_bits;
  memcpy(&scale_bits, &scale, sizeof(scale_bits));
  uint8_t* dst_b = (uint8_t*)dst;
#if defined(__HVX_ARCH__) && __HVX_ARCH__ >= 81
  HVX_Vector vscale = Q6_V_vsplat_R(scale_bits);
  while (width >= 128) {
    HVX_VectorPair h = Q6_Wuh_vunpack_Vub(vloadu(src));
    HVX_VectorPair w01 = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(h));
    HVX_VectorPair w23 = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(h));
    HVX_Vector f0 =
        Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(Q6_V_lo_W(w01)), vscale);
    HVX_Vector f1 =
        Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(Q6_V_hi_W(w01)), vscale);
    HVX_Vector f2 =
        Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(Q6_V_lo_W(w23)), vscale);
    HVX_Vector f3 =
        Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(Q6_V_hi_W(w23)), vscale);
    vstoreu(dst_b, f0);
    vstoreu(dst_b + 128, f1);
    vstoreu(dst_b + 256, f2);
    vstoreu(dst_b + 384, f3);
    src += 128;
    dst_b += 512;
    width -= 128;
  }
  if (width > 0) {
    int dst_bytes = width * 4;
    HVX_VectorPair h = Q6_Wuh_vunpack_Vub(vloadu_partial_zero(src, width));
    HVX_VectorPair w01 = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(h));
    HVX_Vector f0 =
        Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(Q6_V_lo_W(w01)), vscale);
    vstoreu_partial(dst_b, f0, dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      HVX_Vector f1 =
          Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(Q6_V_hi_W(w01)), vscale);
      vstoreu_partial(dst_b + 128, f1, dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      HVX_VectorPair w23 = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(h));
      HVX_Vector f2 =
          Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(Q6_V_lo_W(w23)), vscale);
      vstoreu_partial(dst_b + 256, f2, dst_bytes > 384 ? 128 : dst_bytes - 256);
      if (dst_bytes > 384) {
        HVX_Vector f3 =
            Q6_Vsf_vmpy_VsfVsf(Q6_Vsf_equals_Vw(Q6_V_hi_W(w23)), vscale);
        vstoreu_partial(dst_b + 384, f3, dst_bytes - 384);
      }
    }
  }
#else
#if defined(__HVX_ARCH__) && __HVX_ARCH__ >= 73
  if (scale_bits == 0x3f800000u) {
    while (width >= 128) {
      HVX_VectorPair h = Q6_Wuh_vunpack_Vub(vloadu(src));
      HVX_VectorPair w01 = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(h));
      HVX_VectorPair w23 = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(h));
      vstoreu(dst_b, Q6_Vsf_equals_Vw(Q6_V_lo_W(w01)));
      vstoreu(dst_b + 128, Q6_Vsf_equals_Vw(Q6_V_hi_W(w01)));
      vstoreu(dst_b + 256, Q6_Vsf_equals_Vw(Q6_V_lo_W(w23)));
      vstoreu(dst_b + 384, Q6_Vsf_equals_Vw(Q6_V_hi_W(w23)));
      src += 128;
      dst_b += 512;
      width -= 128;
    }
    if (width > 0) {
      int dst_bytes = width * 4;
      HVX_VectorPair h = Q6_Wuh_vunpack_Vub(vloadu_partial_zero(src, width));
      HVX_VectorPair w01 = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(h));
      vstoreu_partial(dst_b, Q6_Vsf_equals_Vw(Q6_V_lo_W(w01)),
                      dst_bytes > 128 ? 128 : dst_bytes);
      if (dst_bytes > 128) {
        vstoreu_partial(dst_b + 128, Q6_Vsf_equals_Vw(Q6_V_hi_W(w01)),
                        dst_bytes > 256 ? 128 : dst_bytes - 128);
      }
      if (dst_bytes > 256) {
        HVX_VectorPair w23 = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(h));
        vstoreu_partial(dst_b + 256, Q6_Vsf_equals_Vw(Q6_V_lo_W(w23)),
                        dst_bytes > 384 ? 128 : dst_bytes - 256);
        if (dst_bytes > 384) {
          vstoreu_partial(dst_b + 384, Q6_Vsf_equals_Vw(Q6_V_hi_W(w23)),
                          dst_bytes - 384);
        }
      }
    }
    return;
  }
#endif
  int exp_s = (int)((scale_bits >> 23) & 0xffu);
  if (exp_s < 1 || exp_s > 246) {
    ByteToFloatRow_C(src, dst, scale, width);
    return;
  }
  uint32_t m = (scale_bits & 0x007fffffu) | 0x00800000u;
  HVX_Vector vm_hi = Q6_V_vsplat_R(m >> 8);
  HVX_Vector vm_lo = Q6_V_vsplat_R(m & 0xffu);
  HVX_Vector vexp_base = Q6_V_vsplat_R(exp_s + 15);
  HVX_Vector vsign = Q6_V_vsplat_R(scale_bits & 0x80000000u);
  while (width >= 128) {
    HVX_VectorPair h = Q6_Wuh_vunpack_Vub(vloadu(src));
    HVX_VectorPair w01 = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(h));
    HVX_VectorPair w23 = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(h));
    vstoreu(dst_b, hvx_u16_mul_scale_f32_exact(Q6_V_lo_W(w01), vm_hi, vm_lo,
                                               vexp_base, vsign));
    vstoreu(dst_b + 128, hvx_u16_mul_scale_f32_exact(Q6_V_hi_W(w01), vm_hi,
                                                     vm_lo, vexp_base, vsign));
    vstoreu(dst_b + 256, hvx_u16_mul_scale_f32_exact(Q6_V_lo_W(w23), vm_hi,
                                                     vm_lo, vexp_base, vsign));
    vstoreu(dst_b + 384, hvx_u16_mul_scale_f32_exact(Q6_V_hi_W(w23), vm_hi,
                                                     vm_lo, vexp_base, vsign));
    src += 128;
    dst_b += 512;
    width -= 128;
  }
  if (width > 0) {
    int dst_bytes = width * 4;
    HVX_VectorPair h = Q6_Wuh_vunpack_Vub(vloadu_partial_zero(src, width));
    HVX_VectorPair w01 = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(h));
    vstoreu_partial(dst_b,
                    hvx_u16_mul_scale_f32_exact(Q6_V_lo_W(w01), vm_hi, vm_lo,
                                                vexp_base, vsign),
                    dst_bytes > 128 ? 128 : dst_bytes);
    if (dst_bytes > 128) {
      vstoreu_partial(dst_b + 128,
                      hvx_u16_mul_scale_f32_exact(Q6_V_hi_W(w01), vm_hi, vm_lo,
                                                  vexp_base, vsign),
                      dst_bytes > 256 ? 128 : dst_bytes - 128);
    }
    if (dst_bytes > 256) {
      HVX_VectorPair w23 = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(h));
      vstoreu_partial(dst_b + 256,
                      hvx_u16_mul_scale_f32_exact(Q6_V_lo_W(w23), vm_hi, vm_lo,
                                                  vexp_base, vsign),
                      dst_bytes > 384 ? 128 : dst_bytes - 256);
      if (dst_bytes > 384) {
        vstoreu_partial(dst_b + 384,
                        hvx_u16_mul_scale_f32_exact(Q6_V_hi_W(w23), vm_hi,
                                                    vm_lo, vexp_base, vsign),
                        dst_bytes - 384);
      }
    }
  }
#endif
}
#endif  // HAS_BYTETOFLOATROW_HVX

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_HVX) && defined(__HVX__)

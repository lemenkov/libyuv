/*
 *  Copyright 2026 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/compare_row.h"

#if !defined(LIBYUV_DISABLE_HVX) && defined(__HVX__)

#include <hexagon_protos.h>
#include <hexagon_types.h>
#include <hvx_hexagon_protos.h>
#include <string.h>

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

static inline HVX_Vector vloada(const void* ptr) {
  HVX_Vector v;
  memcpy(&v, (const HVX_Vector*)ptr, sizeof(v));
  return v;
}

static inline HVX_UVector vloadu(const void* ptr) {
  HVX_UVector v;
  memcpy(&v, ptr, sizeof(v));
  return v;
}

static inline void vstoreu(void* ptr, HVX_UVector v) {
  memcpy(ptr, &v, sizeof(v));
}

// Partial load of n bytes (1 <= n <= 128), zeroing lanes past n.
static inline HVX_Vector vloadu_partial_zero(const void* ptr, int n) {
  if (n >= 128) {
    return vloadu(ptr);
  }
  const uintptr_t addr = (uintptr_t)ptr;
  const uintptr_t offset = addr & 127;
  HVX_Vector v0 = vloada(ptr);
  HVX_Vector v1 =
      (offset + (uintptr_t)n > 128) ? vloada((const uint8_t*)ptr + 128) : v0;
  HVX_Vector aligned = Q6_V_valign_VVR(v1, v0, addr);
  HVX_VectorPred mask = Q6_Q_vsetq_R(n);
  return Q6_V_vmux_QVV(mask, aligned, Q6_V_vzero());
}

// Partial store of n bytes (1 <= n <= 128) without overwriting past n.
static inline void vstoreu_partial(void* ptr, HVX_Vector v, int n) {
  if (n >= 128) {
    vstoreu(ptr, v);
    return;
  }
  const uintptr_t addr = (uintptr_t)ptr;
  const uintptr_t offset = addr & 127;
  HVX_Vector value = Q6_V_vlalign_VVR(v, v, addr);
  HVX_VectorPred ql_not = Q6_Q_vsetq_R(addr);
  HVX_VectorPred qr = Q6_Q_vsetq2_R(offset + (uintptr_t)n);
  if (offset + (uintptr_t)n > 128) {
    Q6_vmem_QRIV(qr, (uint8_t*)ptr + 128, value);
    qr = Q6_Q_vcmp_eq_VbVb(value, value);
  }
  ql_not = Q6_Q_or_QQn(ql_not, qr);
  Q6_vmem_QnRIV(ql_not, ptr, value);
}

static inline uint32_t horizontal_sum_u32(HVX_Vector x) {
  x = Q6_Vw_vadd_VwVw(x, Q6_V_vror_VR(x, 64));
  x = Q6_Vw_vadd_VwVw(x, Q6_V_vror_VR(x, 32));
  x = Q6_Vw_vadd_VwVw(x, Q6_V_vror_VR(x, 16));
  x = Q6_Vw_vadd_VwVw(x, Q6_V_vror_VR(x, 8));
  x = Q6_Vw_vadd_VwVw(x, Q6_V_vror_VR(x, 4));
  uint32_t result;
  memcpy(&result, &x, sizeof(result));
  return result;
}

#if defined(HAS_HAMMINGDISTANCE_HVX)
uint32_t HammingDistance_HVX(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count) {
  HVX_Vector sum = Q6_V_vzero();
  while (count >= 128) {
    HVX_Vector x = Q6_V_vxor_VV(vloadu(src_a), vloadu(src_b));
    HVX_Vector c = Q6_Vh_vpopcount_Vh(x);
    sum = Q6_Vuw_vrmpyacc_VuwVubRub(sum, c, 0x01010101);
    src_a += 128;
    src_b += 128;
    count -= 128;
  }
  if (count > 0) {
    HVX_Vector x = Q6_V_vxor_VV(vloadu_partial_zero(src_a, count),
                                vloadu_partial_zero(src_b, count));
    HVX_Vector c = Q6_Vh_vpopcount_Vh(x);
    sum = Q6_Vuw_vrmpyacc_VuwVubRub(sum, c, 0x01010101);
  }
  return horizontal_sum_u32(sum);
}
#endif  // defined(HAS_HAMMINGDISTANCE_HVX)

#if defined(HAS_SUMSQUAREERROR_HVX)
uint32_t SumSquareError_HVX(const uint8_t* src_a,
                            const uint8_t* src_b,
                            int count) {
  HVX_Vector sum = Q6_V_vzero();
  while (count >= 128) {
    HVX_Vector d = Q6_Vub_vabsdiff_VubVub(vloadu(src_a), vloadu(src_b));
    sum = Q6_Vuw_vrmpyacc_VuwVubVub(sum, d, d);
    src_a += 128;
    src_b += 128;
    count -= 128;
  }
  if (count > 0) {
    HVX_Vector d = Q6_Vub_vabsdiff_VubVub(vloadu_partial_zero(src_a, count),
                                          vloadu_partial_zero(src_b, count));
    sum = Q6_Vuw_vrmpyacc_VuwVubVub(sum, d, d);
  }
  return horizontal_sum_u32(sum);
}
#endif  // defined(HAS_SUMSQUAREERROR_HVX)

#if defined(HAS_HASHDJB2_HVX)
static const uint32_t kHashMul32[32] __attribute__((aligned(128))) = {
    0x655ec7e1, 0xccc4cfc1, 0x99995ba1, 0xd61beb81,  // 33 ^ 31 .. 28
    0x829bff61, 0x13791741, 0x2f22b321, 0xac185301,  // 33 ^ 27 .. 24
    0xcee976e1, 0xc8359ec1, 0x72ac4aa1, 0x510cfa81,  // 33 ^ 23 .. 20
    0xcc272e61, 0xb0da6641, 0xee162221, 0x92d9e201,  // 33 ^ 19 .. 16
    0x0c3525e1, 0xa3476dc1, 0x3b4039a1, 0x4f5f0981,  // 33 ^ 15 .. 12
    0x30f35d61, 0x855cb541, 0x040a9121, 0x747c7101,  // 33 ^ 11 .. 8
    0xec41d4e1, 0x4cfa3cc1, 0x025528a1, 0x00121881,  // 33 ^ 7 .. 4
    0x00008c61, 0x00000441, 0x00000021, 0x00000001,  // 33 ^ 3 .. 0
};

static inline HVX_Vector vmpy_u32(HVX_Vector a, HVX_Vector b) {
  HVX_Vector hi = Q6_Vw_vmpyieo_VhVh(a, b);
  return Q6_Vw_vmpyieacc_VwVwVuh(hi, a, b);
}

static uint32_t Pow33(int n) {
  uint32_t r = 1u;
  uint32_t b = 33u;
  while (n) {
    if (n & 1) {
      r *= b;
    }
    b *= b;
    n >>= 1;
  }
  return r;
}

static inline uint32_t HashSum128_HVX(HVX_Vector s,
                                      HVX_Vector mul0,
                                      HVX_Vector mul1,
                                      HVX_Vector mul2,
                                      HVX_Vector mul3) {
  HVX_VectorPair h01 = Q6_Wuh_vunpack_Vub(s);
  HVX_VectorPair w01 = Q6_Wuw_vunpack_Vuh(Q6_V_lo_W(h01));
  HVX_VectorPair w23 = Q6_Wuw_vunpack_Vuh(Q6_V_hi_W(h01));
  HVX_Vector sum = Q6_Vw_vmpyie_VwVuh(mul0, Q6_V_lo_W(w01));
  sum = Q6_Vw_vmpyieacc_VwVwVuh(sum, mul1, Q6_V_hi_W(w01));
  sum = Q6_Vw_vmpyieacc_VwVwVuh(sum, mul2, Q6_V_lo_W(w23));
  sum = Q6_Vw_vmpyieacc_VwVwVuh(sum, mul3, Q6_V_hi_W(w23));
  return horizontal_sum_u32(sum);
}

uint32_t HashDjb2_HVX(const uint8_t* src, int count, uint32_t seed) {
  const HVX_Vector c32 = Q6_V_vsplat_R(0x1137c401u);  // 33 ^ 32
  const HVX_Vector mul3 = vloada(kHashMul32);
  const HVX_Vector mul2 = vmpy_u32(mul3, c32);
  const HVX_Vector mul1 = vmpy_u32(mul2, c32);
  const HVX_Vector mul0 = vmpy_u32(mul1, c32);
  while (count >= 128) {
    seed = seed * 0x193f1001u +  // 33 ^ 128
           HashSum128_HVX(vloadu(src), mul0, mul1, mul2, mul3);
    src += 128;
    count -= 128;
  }
  if (count > 0) {
    HVX_Vector v = vloadu_partial_zero(src, count);
    HVX_Vector s = Q6_V_valign_VVR(v, Q6_V_vzero(), count);
    seed = seed * Pow33(count) + HashSum128_HVX(s, mul0, mul1, mul2, mul3);
  }
  return seed;
}
#endif  // defined(HAS_HASHDJB2_HVX)

#if defined(HAS_SSIM4X4ROW_HVX)
#define SSIM_4X4_HVX(va0, vb0, va1, vb1, va2, vb2, va3, vb3)                 \
  HVX_Vector sum_a = Q6_Vuw_vrmpy_VubRub(va0, 0x01010101);                   \
  sum_a = Q6_Vuw_vrmpyacc_VuwVubRub(sum_a, va1, 0x01010101);                 \
  sum_a = Q6_Vuw_vrmpyacc_VuwVubRub(sum_a, va2, 0x01010101);                 \
  sum_a = Q6_Vuw_vrmpyacc_VuwVubRub(sum_a, va3, 0x01010101);                 \
  HVX_Vector sum_b = Q6_Vuw_vrmpy_VubRub(vb0, 0x01010101);                   \
  sum_b = Q6_Vuw_vrmpyacc_VuwVubRub(sum_b, vb1, 0x01010101);                 \
  sum_b = Q6_Vuw_vrmpyacc_VuwVubRub(sum_b, vb2, 0x01010101);                 \
  sum_b = Q6_Vuw_vrmpyacc_VuwVubRub(sum_b, vb3, 0x01010101);                 \
  HVX_Vector sum_sq = Q6_Vuw_vrmpy_VubVub(va0, va0);                         \
  sum_sq = Q6_Vuw_vrmpyacc_VuwVubVub(sum_sq, vb0, vb0);                      \
  sum_sq = Q6_Vuw_vrmpyacc_VuwVubVub(sum_sq, va1, va1);                      \
  sum_sq = Q6_Vuw_vrmpyacc_VuwVubVub(sum_sq, vb1, vb1);                      \
  sum_sq = Q6_Vuw_vrmpyacc_VuwVubVub(sum_sq, va2, va2);                      \
  sum_sq = Q6_Vuw_vrmpyacc_VuwVubVub(sum_sq, vb2, vb2);                      \
  sum_sq = Q6_Vuw_vrmpyacc_VuwVubVub(sum_sq, va3, va3);                      \
  sum_sq = Q6_Vuw_vrmpyacc_VuwVubVub(sum_sq, vb3, vb3);                      \
  HVX_Vector sum_axb = Q6_Vuw_vrmpy_VubVub(va0, vb0);                        \
  sum_axb = Q6_Vuw_vrmpyacc_VuwVubVub(sum_axb, va1, vb1);                    \
  sum_axb = Q6_Vuw_vrmpyacc_VuwVubVub(sum_axb, va2, vb2);                    \
  sum_axb = Q6_Vuw_vrmpyacc_VuwVubVub(sum_axb, va3, vb3);                    \
  HVX_VectorPair ab = Q6_W_vshuff_VVR(sum_b, sum_a, -4);                     \
  HVX_VectorPair sqx = Q6_W_vshuff_VVR(sum_axb, sum_sq, -4);                 \
  HVX_VectorPair out01 = Q6_W_vshuff_VVR(Q6_V_lo_W(sqx), Q6_V_lo_W(ab), -8); \
  HVX_VectorPair out23 = Q6_W_vshuff_VVR(Q6_V_hi_W(sqx), Q6_V_hi_W(ab), -8)

// Width is a multiple of 4.
void Ssim4x4Row_HVX(const uint8_t* src_a,
                    int stride_a,
                    const uint8_t* src_b,
                    int stride_b,
                    uint32_t* dst,
                    int width) {
  const ptrdiff_t sa = stride_a;
  const ptrdiff_t sb = stride_b;
  const uint8_t* a0 = src_a;
  const uint8_t* a1 = a0 + sa;
  const uint8_t* a2 = a1 + sa;
  const uint8_t* a3 = a2 + sa;
  const uint8_t* b0 = src_b;
  const uint8_t* b1 = b0 + sb;
  const uint8_t* b2 = b1 + sb;
  const uint8_t* b3 = b2 + sb;

  while (width >= 128) {
    SSIM_4X4_HVX(vloadu(a0), vloadu(b0), vloadu(a1), vloadu(b1), vloadu(a2),
                 vloadu(b2), vloadu(a3), vloadu(b3));
    vstoreu(dst + 0, Q6_V_lo_W(out01));
    vstoreu(dst + 32, Q6_V_hi_W(out01));
    vstoreu(dst + 64, Q6_V_lo_W(out23));
    vstoreu(dst + 96, Q6_V_hi_W(out23));
    a0 += 128;
    a1 += 128;
    a2 += 128;
    a3 += 128;
    b0 += 128;
    b1 += 128;
    b2 += 128;
    b3 += 128;
    dst += 128;
    width -= 128;
  }
  if (width > 0) {
    SSIM_4X4_HVX(vloadu_partial_zero(a0, width), vloadu_partial_zero(b0, width),
                 vloadu_partial_zero(a1, width), vloadu_partial_zero(b1, width),
                 vloadu_partial_zero(a2, width), vloadu_partial_zero(b2, width),
                 vloadu_partial_zero(a3, width),
                 vloadu_partial_zero(b3, width));
    int bytes = width * 4;
    if (bytes > 0) {
      vstoreu_partial(dst + 0, Q6_V_lo_W(out01), bytes < 128 ? bytes : 128);
    }
    if (bytes > 128) {
      vstoreu_partial(dst + 32, Q6_V_hi_W(out01),
                      bytes - 128 < 128 ? bytes - 128 : 128);
    }
    if (bytes > 256) {
      vstoreu_partial(dst + 64, Q6_V_lo_W(out23),
                      bytes - 256 < 128 ? bytes - 256 : 128);
    }
    if (bytes > 384) {
      vstoreu_partial(dst + 96, Q6_V_hi_W(out23), bytes - 384);
    }
  }
}
#undef SSIM_4X4_HVX
#endif  // defined(HAS_SSIM4X4ROW_HVX)

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_HVX) && defined(__HVX__)

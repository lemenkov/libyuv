/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"
#include "libyuv/convert_from_argb.h"  // For ArgbConstants

// This module is for Visual C 32/64 bit
#if !defined(LIBYUV_DISABLE_X86) && \
    (defined(__x86_64__) || defined(__i386__) || \
     defined(_M_X64) || defined(_M_X86)) && \
    ((defined(_MSC_VER) && !defined(__clang__)) || \
     defined(LIBYUV_ENABLE_ROWWIN))

#include <emmintrin.h>
#include <tmmintrin.h>  // For _mm_maddubs_epi16
#include <immintrin.h>  // For AVX2 intrinsics

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Read 8 UV from 444
#define READYUV444                                    \
  xmm3 = _mm_loadl_epi64((__m128i*)u_buf);            \
  xmm1 = _mm_loadl_epi64((__m128i*)(u_buf + offset)); \
  xmm3 = _mm_unpacklo_epi8(xmm3, xmm1);               \
  u_buf += 8;                                         \
  xmm4 = _mm_loadl_epi64((__m128i*)y_buf);            \
  xmm4 = _mm_unpacklo_epi8(xmm4, xmm4);               \
  y_buf += 8;

// Read 8 UV from 444, With 8 Alpha.
#define READYUVA444                                   \
  xmm3 = _mm_loadl_epi64((__m128i*)u_buf);            \
  xmm1 = _mm_loadl_epi64((__m128i*)(u_buf + offset)); \
  xmm3 = _mm_unpacklo_epi8(xmm3, xmm1);               \
  u_buf += 8;                                         \
  xmm4 = _mm_loadl_epi64((__m128i*)y_buf);            \
  xmm4 = _mm_unpacklo_epi8(xmm4, xmm4);               \
  y_buf += 8;                                         \
  xmm5 = _mm_loadl_epi64((__m128i*)a_buf);            \
  a_buf += 8;

// Read 4 UV from 422, upsample to 8 UV.
#define READYUV422                                        \
  xmm3 = _mm_cvtsi32_si128(*(uint32_t*)u_buf);            \
  xmm1 = _mm_cvtsi32_si128(*(uint32_t*)(u_buf + offset)); \
  xmm3 = _mm_unpacklo_epi8(xmm3, xmm1);                   \
  xmm3 = _mm_unpacklo_epi16(xmm3, xmm3);                  \
  u_buf += 4;                                             \
  xmm4 = _mm_loadl_epi64((__m128i*)y_buf);                \
  xmm4 = _mm_unpacklo_epi8(xmm4, xmm4);                   \
  y_buf += 8;

// Read 4 UV from 422, upsample to 8 UV.  With 8 Alpha.
#define READYUVA422                                       \
  xmm3 = _mm_cvtsi32_si128(*(uint32_t*)u_buf);            \
  xmm1 = _mm_cvtsi32_si128(*(uint32_t*)(u_buf + offset)); \
  xmm3 = _mm_unpacklo_epi8(xmm3, xmm1);                   \
  xmm3 = _mm_unpacklo_epi16(xmm3, xmm3);                  \
  u_buf += 4;                                             \
  xmm4 = _mm_loadl_epi64((__m128i*)y_buf);                \
  xmm4 = _mm_unpacklo_epi8(xmm4, xmm4);                   \
  y_buf += 8;                                             \
  xmm5 = _mm_loadl_epi64((__m128i*)a_buf);                \
  a_buf += 8;

// Convert 8 pixels: 8 UV and 8 Y.
#define YUVTORGB(yuvconstants)                                      \
  xmm3 = _mm_sub_epi8(xmm3, _mm_set1_epi8((char)0x80));             \
  xmm4 = _mm_mulhi_epu16(xmm4, *(__m128i*)yuvconstants->kYToRgb);   \
  xmm4 = _mm_add_epi16(xmm4, *(__m128i*)yuvconstants->kYBiasToRgb); \
  xmm0 = _mm_maddubs_epi16(*(__m128i*)yuvconstants->kUVToB, xmm3);  \
  xmm1 = _mm_maddubs_epi16(*(__m128i*)yuvconstants->kUVToG, xmm3);  \
  xmm2 = _mm_maddubs_epi16(*(__m128i*)yuvconstants->kUVToR, xmm3);  \
  xmm0 = _mm_adds_epi16(xmm4, xmm0);                                \
  xmm1 = _mm_subs_epi16(xmm4, xmm1);                                \
  xmm2 = _mm_adds_epi16(xmm4, xmm2);                                \
  xmm0 = _mm_srai_epi16(xmm0, 6);                                   \
  xmm1 = _mm_srai_epi16(xmm1, 6);                                   \
  xmm2 = _mm_srai_epi16(xmm2, 6);                                   \
  xmm0 = _mm_packus_epi16(xmm0, xmm0);                              \
  xmm1 = _mm_packus_epi16(xmm1, xmm1);                              \
  xmm2 = _mm_packus_epi16(xmm2, xmm2);

// Store 8 ARGB values.
#define STOREARGB                                    \
  xmm0 = _mm_unpacklo_epi8(xmm0, xmm1);              \
  xmm2 = _mm_unpacklo_epi8(xmm2, xmm5);              \
  xmm1 = _mm_loadu_si128(&xmm0);                     \
  xmm0 = _mm_unpacklo_epi16(xmm0, xmm2);             \
  xmm1 = _mm_unpackhi_epi16(xmm1, xmm2);             \
  _mm_storeu_si128((__m128i*)dst_argb, xmm0);        \
  _mm_storeu_si128((__m128i*)(dst_argb + 16), xmm1); \
  dst_argb += 32;

#if defined(HAS_I422TOARGBROW_SSSE3)

#endif

#if defined(HAS_I422ALPHATOARGBROW_SSSE3)

#endif

#if defined(HAS_I444TOARGBROW_SSSE3)

#endif

#if defined(HAS_I444ALPHATOARGBROW_SSSE3)

#endif

#if defined(HAS_ARGBTOYROW_AVX2)

#if defined(__clang__) || defined(__GNUC__)
#define LIBYUV_TARGET_AVX2 __attribute__((target("avx2")))
#define LIBYUV_TARGET_AVX512BW __attribute__((target("avx512bw,avx512vl,avx512f")))
#else
#define LIBYUV_TARGET_AVX2
#define LIBYUV_TARGET_AVX512BW
#endif

LIBYUV_TARGET_AVX2
void ARGBToYMatrixRow_AVX2(const uint8_t* src_argb,
                           uint8_t* dst_y,
                           int width,
                           const struct ArgbConstants* c) {
  __m256i ymm5 = _mm256_set1_epi8((char)0x80);
  __m128i kRGBToY = _mm_loadu_si128((const __m128i*)c->kRGBToY);
  __m256i ymm4 = _mm256_broadcastsi128_si256(kRGBToY);
  __m128i kAddY = _mm_loadu_si128((const __m128i*)c->kAddY);
  __m256i ymm7 = _mm256_broadcastsi128_si256(kAddY);
  __m256i ymm6 = _mm256_maddubs_epi16(ymm4, ymm5);
  ymm6 = _mm256_hadd_epi16(ymm6, ymm6);
  ymm7 = _mm256_sub_epi16(ymm7, ymm6);
  __m256i perm_mask = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb);
    __m256i ymm1 = _mm256_loadu_si256((const __m256i*)(src_argb + 32));
    __m256i ymm2 = _mm256_loadu_si256((const __m256i*)(src_argb + 64));
    __m256i ymm3 = _mm256_loadu_si256((const __m256i*)(src_argb + 96));
    src_argb += 128;

    ymm0 = _mm256_sub_epi8(ymm0, ymm5);
    ymm1 = _mm256_sub_epi8(ymm1, ymm5);
    ymm2 = _mm256_sub_epi8(ymm2, ymm5);
    ymm3 = _mm256_sub_epi8(ymm3, ymm5);

    ymm0 = _mm256_maddubs_epi16(ymm4, ymm0);
    ymm1 = _mm256_maddubs_epi16(ymm4, ymm1);
    ymm2 = _mm256_maddubs_epi16(ymm4, ymm2);
    ymm3 = _mm256_maddubs_epi16(ymm4, ymm3);

    ymm0 = _mm256_hadd_epi16(ymm0, ymm1);
    ymm2 = _mm256_hadd_epi16(ymm2, ymm3);

    ymm0 = _mm256_add_epi16(ymm0, ymm7);
    ymm2 = _mm256_add_epi16(ymm2, ymm7);

    ymm0 = _mm256_srli_epi16(ymm0, 8);
    ymm2 = _mm256_srli_epi16(ymm2, 8);

    ymm0 = _mm256_packus_epi16(ymm0, ymm2);
    ymm0 = _mm256_permutevar8x32_epi32(ymm0, perm_mask);

    _mm256_storeu_si256((__m256i*)dst_y, ymm0);
    dst_y += 32;
    width -= 32;
  }
}

LIBYUV_TARGET_AVX2
void ARGBToYRow_AVX2(const uint8_t* src_argb, uint8_t* dst_y, int width) {
  ARGBToYMatrixRow_AVX2(src_argb, dst_y, width, &kArgbI601Constants);
}

LIBYUV_TARGET_AVX2
void ABGRToYRow_AVX2(const uint8_t* src_abgr, uint8_t* dst_y, int width) {
  ARGBToYMatrixRow_AVX2(src_abgr, dst_y, width, &kAbgrI601Constants);
}

LIBYUV_TARGET_AVX2
void ARGBToYJRow_AVX2(const uint8_t* src_argb, uint8_t* dst_y, int width) {
  ARGBToYMatrixRow_AVX2(src_argb, dst_y, width, &kArgbJPEGConstants);
}

LIBYUV_TARGET_AVX2
void ABGRToYJRow_AVX2(const uint8_t* src_abgr, uint8_t* dst_y, int width) {
  ARGBToYMatrixRow_AVX2(src_abgr, dst_y, width, &kAbgrJPEGConstants);
}

LIBYUV_TARGET_AVX2
void RGBAToYJRow_AVX2(const uint8_t* src_rgba, uint8_t* dst_y, int width) {
  ARGBToYMatrixRow_AVX2(src_rgba, dst_y, width, &kRgbaJPEGConstants);
}

LIBYUV_TARGET_AVX2
void RGBAToYRow_AVX2(const uint8_t* src_rgba, uint8_t* dst_y, int width) {
  ARGBToYMatrixRow_AVX2(src_rgba, dst_y, width, &kRgbaI601Constants);
}

LIBYUV_TARGET_AVX2
void BGRAToYRow_AVX2(const uint8_t* src_bgra, uint8_t* dst_y, int width) {
  ARGBToYMatrixRow_AVX2(src_bgra, dst_y, width, &kBgraI601Constants);
}

#ifdef HAS_RAWTOARGBROW_AVX2
LIBYUV_TARGET_AVX2
void RAWToARGBRow_AVX2(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  __m256i ymm_alpha = _mm256_set1_epi32(0xff000000);
  __m128i shuf_low = _mm_set_epi8(-1, 9, 10, 11, -1, 6, 7, 8, -1, 3, 4, 5, -1, 0, 1, 2);
  __m128i shuf_high = _mm_set_epi8(-1, 13, 14, 15, -1, 10, 11, 12, -1, 7, 8, 9, -1, 4, 5, 6);
  __m256i ymm_shuf = _mm256_broadcastsi128_si256(shuf_low);
  __m256i ymm_shuf2 = _mm256_broadcastsi128_si256(shuf_high);

  while (width > 0) {
    __m128i xmm0 = _mm_loadu_si128((const __m128i*)src_raw);
    __m256i ymm0 = _mm256_castsi128_si256(xmm0);
    ymm0 = _mm256_inserti128_si256(ymm0, _mm_loadu_si128((const __m128i*)(src_raw + 12)), 1);

    __m128i xmm1 = _mm_loadu_si128((const __m128i*)(src_raw + 24));
    __m256i ymm1 = _mm256_castsi128_si256(xmm1);
    ymm1 = _mm256_inserti128_si256(ymm1, _mm_loadu_si128((const __m128i*)(src_raw + 36)), 1);

    __m128i xmm2 = _mm_loadu_si128((const __m128i*)(src_raw + 48));
    __m256i ymm2 = _mm256_castsi128_si256(xmm2);
    ymm2 = _mm256_inserti128_si256(ymm2, _mm_loadu_si128((const __m128i*)(src_raw + 60)), 1);

    __m128i xmm3 = _mm_loadu_si128((const __m128i*)(src_raw + 68));
    __m256i ymm3 = _mm256_castsi128_si256(xmm3);
    ymm3 = _mm256_inserti128_si256(ymm3, _mm_loadu_si128((const __m128i*)(src_raw + 80)), 1);

    ymm0 = _mm256_shuffle_epi8(ymm0, ymm_shuf);
    ymm1 = _mm256_shuffle_epi8(ymm1, ymm_shuf);
    ymm2 = _mm256_shuffle_epi8(ymm2, ymm_shuf);
    ymm3 = _mm256_shuffle_epi8(ymm3, ymm_shuf2);

    ymm0 = _mm256_or_si256(ymm0, ymm_alpha);
    ymm1 = _mm256_or_si256(ymm1, ymm_alpha);
    ymm2 = _mm256_or_si256(ymm2, ymm_alpha);
    ymm3 = _mm256_or_si256(ymm3, ymm_alpha);

    _mm256_storeu_si256((__m256i*)dst_argb, ymm0);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), ymm1);
    _mm256_storeu_si256((__m256i*)(dst_argb + 64), ymm2);
    _mm256_storeu_si256((__m256i*)(dst_argb + 96), ymm3);

    src_raw += 96;
    dst_argb += 128;
    width -= 32;
  }
}
#endif

#ifdef HAS_RAWTOARGBROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void RGBToARGBRow_AVX512BW(const uint8_t* src_raw, uint8_t* dst_argb, const __m128i* shuffler, int width) {
  __m512i zmm_alpha = _mm512_set1_epi32(0xff000000);
  __m512i zmm_perm = _mm512_set_epi32(
      12, 11, 10, 9, 9, 8, 7, 6, 6, 5, 4, 3, 3, 2, 1, 0);
  __m512i zmm_shuf = _mm512_broadcast_i32x4(_mm_loadu_si128(shuffler));

  while (width > 0) {
    __m512i zmm0 = _mm512_maskz_loadu_epi8(0xffffffffffffull, src_raw);
    __m512i zmm1 = _mm512_maskz_loadu_epi8(0xffffffffffffull, src_raw + 48);
    __m512i zmm2 = _mm512_maskz_loadu_epi8(0xffffffffffffull, src_raw + 96);
    __m512i zmm3 = _mm512_maskz_loadu_epi8(0xffffffffffffull, src_raw + 144);

    zmm0 = _mm512_permutexvar_epi32(zmm_perm, zmm0);
    zmm1 = _mm512_permutexvar_epi32(zmm_perm, zmm1);
    zmm2 = _mm512_permutexvar_epi32(zmm_perm, zmm2);
    zmm3 = _mm512_permutexvar_epi32(zmm_perm, zmm3);

    zmm0 = _mm512_shuffle_epi8(zmm0, zmm_shuf);
    zmm1 = _mm512_shuffle_epi8(zmm1, zmm_shuf);
    zmm2 = _mm512_shuffle_epi8(zmm2, zmm_shuf);
    zmm3 = _mm512_shuffle_epi8(zmm3, zmm_shuf);

    zmm0 = _mm512_or_si512(zmm0, zmm_alpha);
    zmm1 = _mm512_or_si512(zmm1, zmm_alpha);
    zmm2 = _mm512_or_si512(zmm2, zmm_alpha);
    zmm3 = _mm512_or_si512(zmm3, zmm_alpha);

    _mm512_storeu_si512(dst_argb, zmm0);
    _mm512_storeu_si512(dst_argb + 64, zmm1);
    _mm512_storeu_si512(dst_argb + 128, zmm2);
    _mm512_storeu_si512(dst_argb + 192, zmm3);

    src_raw += 192;
    dst_argb += 256;
    width -= 64;
  }
}

LIBYUV_TARGET_AVX512BW
void RAWToARGBRow_AVX512BW(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  __m128i shuf = _mm_set_epi8(-1, 9, 10, 11, -1, 6, 7, 8, -1, 3, 4, 5, -1, 0, 1, 2);
  RGBToARGBRow_AVX512BW(src_raw, dst_argb, &shuf, width);
}

LIBYUV_TARGET_AVX512BW
void RGB24ToARGBRow_AVX512BW(const uint8_t* src_rgb24, uint8_t* dst_argb, int width) {
  __m128i shuf = _mm_set_epi8(-1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0);
  RGBToARGBRow_AVX512BW(src_rgb24, dst_argb, &shuf, width);
}
#endif

#ifdef HAS_ARGBTOUVMATRIXROW_AVX2
LIBYUV_TARGET_AVX2 __attribute__((no_sanitize("cfi-icall")))
void ARGBToUVMatrixRow_AVX2(const uint8_t* src_argb,
                            int src_stride_argb,
                            uint8_t* dst_u,
                            uint8_t* dst_v,
                            int width,
                            const struct ArgbConstants* c) {
  __m256i ymm_u = _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)c->kRGBToU));
  __m256i ymm_v = _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)c->kRGBToV));
  __m256i ymm_0101 = _mm256_set1_epi16(0x0101);
  __m256i ymm_shuf = _mm256_setr_epi8(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15,
                                      0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);
  __m256i ymm_8000 = _mm256_set1_epi16((short)0x8000);
  __m256i ymm_zero = _mm256_setzero_si256();

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb);
    __m256i ymm1 = _mm256_loadu_si256((const __m256i*)(src_argb + 32));
    __m256i ymm2 = _mm256_loadu_si256((const __m256i*)(src_argb + src_stride_argb));
    __m256i ymm3 = _mm256_loadu_si256((const __m256i*)(src_argb + src_stride_argb + 32));

    ymm0 = _mm256_shuffle_epi8(ymm0, ymm_shuf);
    ymm1 = _mm256_shuffle_epi8(ymm1, ymm_shuf);
    ymm2 = _mm256_shuffle_epi8(ymm2, ymm_shuf);
    ymm3 = _mm256_shuffle_epi8(ymm3, ymm_shuf);

    ymm0 = _mm256_maddubs_epi16(ymm0, ymm_0101);
    ymm1 = _mm256_maddubs_epi16(ymm1, ymm_0101);
    ymm2 = _mm256_maddubs_epi16(ymm2, ymm_0101);
    ymm3 = _mm256_maddubs_epi16(ymm3, ymm_0101);

    ymm0 = _mm256_add_epi16(ymm0, ymm2);
    ymm1 = _mm256_add_epi16(ymm1, ymm3);

    ymm0 = _mm256_srli_epi16(ymm0, 1);
    ymm1 = _mm256_srli_epi16(ymm1, 1);
    ymm0 = _mm256_avg_epu16(ymm0, ymm_zero);
    ymm1 = _mm256_avg_epu16(ymm1, ymm_zero);

    ymm0 = _mm256_packus_epi16(ymm0, ymm1);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);

    ymm1 = _mm256_maddubs_epi16(ymm0, ymm_v);
    ymm0 = _mm256_maddubs_epi16(ymm0, ymm_u);

    ymm0 = _mm256_hadd_epi16(ymm0, ymm1);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    ymm0 = _mm256_sub_epi16(ymm_8000, ymm0);
    ymm0 = _mm256_srli_epi16(ymm0, 8);
    ymm0 = _mm256_packus_epi16(ymm0, ymm0);

    __m128i xmm_u = _mm256_castsi256_si128(ymm0);
    __m128i xmm_v = _mm256_extracti128_si256(ymm0, 1);

    _mm_storel_epi64((__m128i*)dst_u, xmm_u);
    _mm_storel_epi64((__m128i*)dst_v, xmm_v);

    src_argb += 64;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  }
}
#endif

#ifdef HAS_MERGEUVROW_AVX2
LIBYUV_TARGET_AVX2
void MergeUVRow_AVX2(const uint8_t* src_u,
                     const uint8_t* src_v,
                     uint8_t* dst_uv,
                     int width) {
  while (width > 0) {
    __m256i ymm0 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)src_u));
    __m256i ymm1 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)src_v));

    ymm1 = _mm256_slli_epi16(ymm1, 8);
    ymm0 = _mm256_or_si256(ymm0, ymm1);

    _mm256_storeu_si256((__m256i*)dst_uv, ymm0);

    src_u += 16;
    src_v += 16;
    dst_uv += 32;
    width -= 16;
  }
}
#endif

#endif


#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_X86) && (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_X86)) && ((defined(_MSC_VER) && !defined(__clang__)) || defined(LIBYUV_ENABLE_ROWWIN))

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
#else
#define LIBYUV_TARGET_AVX2
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
#endif


#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_X86) && (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_X86)) && ((defined(_MSC_VER) && !defined(__clang__)) || defined(LIBYUV_ENABLE_ROWWIN))

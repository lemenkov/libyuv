/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/convert_from_argb.h"  // For ArgbConstants
#include "libyuv/row.h"

// This module is for Visual C 32/64 bit
#if !defined(LIBYUV_DISABLE_X86) &&                                 \
    (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || \
     defined(_M_IX86)) &&                                           \
    ((defined(_MSC_VER) && !defined(__clang__)) ||                  \
     defined(LIBYUV_ENABLE_ROWWIN))

#include <emmintrin.h>
#include <immintrin.h>  // For AVX2 intrinsics
#include <tmmintrin.h>  // For _mm_maddubs_epi16
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if defined(__clang__) || defined(__GNUC__)
#define LIBYUV_TARGET_SSSE3 __attribute__((target("ssse3")))
#define LIBYUV_TARGET_AVX2 __attribute__((target("avx2")))
#define LIBYUV_TARGET_AVX512BW \
  __attribute__((target("avx512bw,avx512vl,avx512f")))
#define LIBYUV_TARGET_AVX512VBMI \
  __attribute__((target("avx512vbmi,avx512bw,avx512vl,avx512f")))
#else
#define LIBYUV_TARGET_SSSE3
#define LIBYUV_TARGET_AVX2
#define LIBYUV_TARGET_AVX512BW
#define LIBYUV_TARGET_AVX512VBMI
#endif  // defined(__clang__) || defined(__GNUC__)

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

// Store 8 AR30 values.
#define STOREAR30                                    \
  xmm1 = _mm_and_si128(xmm1, xmm7);                  \
  xmm2 = _mm_and_si128(xmm2, xmm7);                  \
  xmm0 = _mm_srli_epi16(xmm0, 4);                    \
  xmm3 = _mm_slli_epi16(xmm1, 6);                    \
  xmm0 = _mm_or_si128(xmm0, xmm3);                   \
  xmm1 = _mm_srli_epi16(xmm1, 10);                   \
  xmm2 = _mm_or_si128(xmm2, xmm6);                   \
  xmm2 = _mm_or_si128(xmm2, xmm1);                   \
  xmm1 = _mm_unpackhi_epi16(xmm0, xmm2);             \
  xmm0 = _mm_unpacklo_epi16(xmm0, xmm2);             \
  _mm_storeu_si128((__m128i*)dst_ar30, xmm0);        \
  _mm_storeu_si128((__m128i*)(dst_ar30 + 16), xmm1); \
  dst_ar30 += 32;

// Convert 32 ARGB pixels (128 bytes) to 32 UV444 values.
#if defined(HAS_ARGBTOYMATRIXROW_AVX2) || defined(HAS_ARGBTOUV444MATRIXROW_AVX2)
LIBYUV_TARGET_AVX2
void ARGBToUV444MatrixRow_AVX2(const uint8_t* src_argb,
                               uint8_t* dst_u,
                               uint8_t* dst_v,
                               int width,
                               const struct ArgbConstants* c) {
  __m256i ymm_u =
      _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)c->kRGBToU));
  __m256i ymm_v =
      _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)c->kRGBToV));
  __m256i ymm5 = _mm256_set1_epi16((short)0x8000);
  __m256i perm_mask = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb);
    __m256i ymm1 = _mm256_loadu_si256((const __m256i*)(src_argb + 32));
    __m256i ymm2 = _mm256_loadu_si256((const __m256i*)(src_argb + 64));
    __m256i ymm3 = _mm256_loadu_si256((const __m256i*)(src_argb + 96));
    src_argb += 128;

    __m256i ymm0_u = _mm256_maddubs_epi16(ymm0, ymm_u);
    __m256i ymm1_u = _mm256_maddubs_epi16(ymm1, ymm_u);
    __m256i ymm2_u = _mm256_maddubs_epi16(ymm2, ymm_u);
    __m256i ymm3_u = _mm256_maddubs_epi16(ymm3, ymm_u);

    __m256i ymm0_v = _mm256_maddubs_epi16(ymm0, ymm_v);
    __m256i ymm1_v = _mm256_maddubs_epi16(ymm1, ymm_v);
    __m256i ymm2_v = _mm256_maddubs_epi16(ymm2, ymm_v);
    __m256i ymm3_v = _mm256_maddubs_epi16(ymm3, ymm_v);

    ymm0_u = _mm256_hadd_epi16(ymm0_u, ymm1_u);
    ymm2_u = _mm256_hadd_epi16(ymm2_u, ymm3_u);

    ymm0_v = _mm256_hadd_epi16(ymm0_v, ymm1_v);
    ymm2_v = _mm256_hadd_epi16(ymm2_v, ymm3_v);

    ymm0_u = _mm256_sub_epi16(ymm5, ymm0_u);
    ymm2_u = _mm256_sub_epi16(ymm5, ymm2_u);

    ymm0_v = _mm256_sub_epi16(ymm5, ymm0_v);
    ymm2_v = _mm256_sub_epi16(ymm5, ymm2_v);

    ymm0_u = _mm256_srli_epi16(ymm0_u, 8);
    ymm2_u = _mm256_srli_epi16(ymm2_u, 8);

    ymm0_v = _mm256_srli_epi16(ymm0_v, 8);
    ymm2_v = _mm256_srli_epi16(ymm2_v, 8);

    ymm0_u = _mm256_packus_epi16(ymm0_u, ymm2_u);
    ymm0_u = _mm256_permutevar8x32_epi32(ymm0_u, perm_mask);

    ymm0_v = _mm256_packus_epi16(ymm0_v, ymm2_v);
    ymm0_v = _mm256_permutevar8x32_epi32(ymm0_v, perm_mask);

    _mm256_storeu_si256((__m256i*)dst_u, ymm0_u);
    _mm256_storeu_si256((__m256i*)dst_v, ymm0_v);
    dst_u += 32;
    dst_v += 32;
    width -= 32;
  }
  _mm256_zeroupper();
}
#endif  // defined(HAS_ARGBTOYMATRIXROW_AVX2) ||
        // defined(HAS_ARGBTOUV444MATRIXROW_AVX2)

#if defined(HAS_ARGBTOYMATRIXROW_AVX2)
LIBYUV_TARGET_AVX2
void ARGBToYMatrixRow_AVX2(const uint8_t* src_argb,
                           uint8_t* dst_y,
                           int width,
                           const struct ArgbConstants* c) {
  __m256i ymm5 = _mm256_set1_epi8((char)0x80);
  __m256i ymm4 =
      _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)c->kRGBToY));
  __m256i ymm7 =
      _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)c->kAddY));
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
  _mm256_zeroupper();
}
#endif  // defined(HAS_ARGBTOYMATRIXROW_AVX2)

#ifdef HAS_RAWTOARGBROW_AVX2
LIBYUV_TARGET_AVX2
void RAWToARGBRow_AVX2(const uint8_t* src_raw, uint8_t* dst_argb, int width) {
  __m256i ymm_alpha = _mm256_set1_epi32(0xff000000);
  __m128i shuf_low =
      _mm_set_epi8(-1, 9, 10, 11, -1, 6, 7, 8, -1, 3, 4, 5, -1, 0, 1, 2);
  __m128i shuf_high =
      _mm_set_epi8(-1, 13, 14, 15, -1, 10, 11, 12, -1, 7, 8, 9, -1, 4, 5, 6);
  __m256i ymm_shuf = _mm256_broadcastsi128_si256(shuf_low);
  __m256i ymm_shuf2 = _mm256_broadcastsi128_si256(shuf_high);

  while (width > 0) {
    __m128i xmm0 = _mm_loadu_si128((const __m128i*)src_raw);
    __m256i ymm0 = _mm256_castsi128_si256(xmm0);
    ymm0 = _mm256_inserti128_si256(
        ymm0, _mm_loadu_si128((const __m128i*)(src_raw + 12)), 1);

    __m128i xmm1 = _mm_loadu_si128((const __m128i*)(src_raw + 24));
    __m256i ymm1 = _mm256_castsi128_si256(xmm1);
    ymm1 = _mm256_inserti128_si256(
        ymm1, _mm_loadu_si128((const __m128i*)(src_raw + 36)), 1);

    __m128i xmm2 = _mm_loadu_si128((const __m128i*)(src_raw + 48));
    __m256i ymm2 = _mm256_castsi128_si256(xmm2);
    ymm2 = _mm256_inserti128_si256(
        ymm2, _mm_loadu_si128((const __m128i*)(src_raw + 60)), 1);

    __m128i xmm3 = _mm_loadu_si128((const __m128i*)(src_raw + 68));
    __m256i ymm3 = _mm256_castsi128_si256(xmm3);
    ymm3 = _mm256_inserti128_si256(
        ymm3, _mm_loadu_si128((const __m128i*)(src_raw + 80)), 1);

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
  _mm256_zeroupper();
}
#endif  // HAS_RAWTOARGBROW_AVX2

#if defined(HAS_RAWTOARGBROW_AVX512BW) ||   \
    defined(HAS_RGB24TOARGBROW_AVX512BW) || \
    defined(HAS_RAWTORGB24ROW_AVX512BW) ||  \
    defined(HAS_RGBTOYMATRIXROW_AVX512BW)
static const uint64_t kPermqRGB24ToARGB0_AVX512BW[8] = {0, 1, 1, 2, 3, 4, 4, 5};
static const uint64_t kPermqRGB24ToARGB1_AVX512BW[8] = {6, 7,  7,  8,
                                                        9, 10, 10, 11};
static const uint64_t kPermqRGB24ToARGB2_AVX512BW[8] = {4, 5, 5, 6, 7, 8, 8, 9};
static const uint64_t kPermqRGB24ToARGB3_AVX512BW[8] = {2, 3, 3, 4, 5, 6, 6, 7};
#endif  // defined(HAS_RAWTOARGBROW_AVX512BW) ||
        // defined(HAS_RGB24TOARGBROW_AVX512BW) ||
        // defined(HAS_RAWTORGB24ROW_AVX512BW) ||
        // defined(HAS_RGBTOYMATRIXROW_AVX512BW)

#if defined(HAS_RGB24TOARGBROW_AVX512BW) || \
    defined(HAS_RGBTOYMATRIXROW_AVX512BW)
static const uint8_t kShuffleMaskRGB24ToARGB_AVX512BW[32] = {
    0u,   1u,   2u,  128u, 3u,   4u,   5u,  128u, 6u,   7u,  8u,
    128u, 9u,   10u, 11u,  128u, 4u,   5u,  6u,   128u, 7u,  8u,
    9u,   128u, 10u, 11u,  12u,  128u, 13u, 14u,  15u,  128u};
#endif  // defined(HAS_RGB24TOARGBROW_AVX512BW) ||
        // defined(HAS_RGBTOYMATRIXROW_AVX512BW)

#if defined(HAS_RAWTOARGBROW_AVX512BW)
static const uint8_t kShuffleMaskRAWToARGB_AVX512BW[32] = {
    2u,   1u,   0u,  128u, 5u,   4u,   3u,  128u, 8u,   7u,  6u,
    128u, 11u,  10u, 9u,   128u, 6u,   5u,  4u,   128u, 9u,  8u,
    7u,   128u, 12u, 11u,  10u,  128u, 15u, 14u,  13u,  128u};
#endif  // defined(HAS_RAWTOARGBROW_AVX512BW)

#if defined(HAS_RAWTORGB24ROW_SSSE3)
// Shuffle table for converting RAW to RGB24.  First 8.
static const uint8_t kShuffleMaskRAWToRGB24_0[16] = {
    2u,   1u,   0u,   5u,   4u,   3u,   8u,   7u,
    128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u};

// Shuffle table for converting RAW to RGB24.  Middle 8.
static const uint8_t kShuffleMaskRAWToRGB24_1[16] = {
    2u,   7u,   6u,   5u,   10u,  9u,   8u,   13u,
    128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u};

// Shuffle table for converting RAW to RGB24.  Last 8.
static const uint8_t kShuffleMaskRAWToRGB24_2[16] = {
    8u,   7u,   12u,  11u,  10u,  15u,  14u,  13u,
    128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u};
#endif  // defined(HAS_RAWTORGB24ROW_SSSE3)

#if defined(HAS_RAWTORGB24ROW_AVX2) || defined(HAS_RAWTORGB24ROW_AVX512BW)
static const uint8_t kShuffleMaskRAWToRGB24_AVX2[32] = {
    2u,  1u,   0u,   5u,   4u,   3u,  8u,   7u,   6u,   11u, 10u,
    9u,  128u, 128u, 128u, 128u, 6u,  5u,   4u,   9u,   8u,  7u,
    12u, 11u,  10u,  15u,  14u,  13u, 128u, 128u, 128u, 128u};
#endif  // defined(HAS_RAWTORGB24ROW_AVX2) ||
        // defined(HAS_RAWTORGB24ROW_AVX512BW)

#if defined(HAS_RAWTORGB24ROW_AVX2)
static const uint32_t kPermd0_AVX2[8] = {0, 1, 2, 4, 5, 6, 3, 7};
static const uint32_t kPermd1_AVX2[8] = {2, 4, 5, 6, 3, 7, 0, 1};
static const uint32_t kPermd2_AVX2[8] = {5, 6, 3, 7, 0, 1, 2, 4};
#endif  // defined(HAS_RAWTORGB24ROW_AVX2)

#if defined(HAS_RAWTORGB24ROW_AVX512BW)
static const uint32_t kPermdRGB24_0_AVX512BW[16] = {
    0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 16, 17, 18, 20};
static const uint32_t kPermdRGB24_1_AVX512BW[16] = {
    5, 6, 8, 9, 10, 12, 13, 14, 16, 17, 18, 20, 21, 22, 24, 25};
static const uint32_t kPermdRGB24_2_AVX512BW[16] = {
    10, 12, 13, 14, 16, 17, 18, 20, 21, 22, 24, 25, 26, 28, 29, 30};
#endif  // defined(HAS_RAWTORGB24ROW_AVX512BW)
#if defined(HAS_RAWTORGB24ROW_AVX512VBMI)
static const uint8_t kPermRAWToRGB24_0[64] = {
    0x02, 0x01, 0x00, 0x05, 0x04, 0x03, 0x08, 0x07, 0x06, 0x0b, 0x0a,
    0x09, 0x0e, 0x0d, 0x0c, 0x11, 0x10, 0x0f, 0x14, 0x13, 0x12, 0x17,
    0x16, 0x15, 0x1a, 0x19, 0x18, 0x1d, 0x1c, 0x1b, 0x20, 0x1f, 0x1e,
    0x23, 0x22, 0x21, 0x26, 0x25, 0x24, 0x29, 0x28, 0x27, 0x2c, 0x2b,
    0x2a, 0x2f, 0x2e, 0x2d, 0x32, 0x31, 0x30, 0x35, 0x34, 0x33, 0x38,
    0x37, 0x36, 0x3b, 0x3a, 0x39, 0x3e, 0x3d, 0x3c, 0x41};
static const uint8_t kPermRAWToRGB24_1[64] = {
    0x00, 0x7f, 0x04, 0x03, 0x02, 0x07, 0x06, 0x05, 0x0a, 0x09, 0x08,
    0x0d, 0x0c, 0x0b, 0x10, 0x0f, 0x0e, 0x13, 0x12, 0x11, 0x16, 0x15,
    0x14, 0x19, 0x18, 0x17, 0x1c, 0x1b, 0x1a, 0x1f, 0x1e, 0x1d, 0x22,
    0x21, 0x20, 0x25, 0x24, 0x23, 0x28, 0x27, 0x26, 0x2b, 0x2a, 0x29,
    0x2e, 0x2d, 0x2c, 0x31, 0x30, 0x2f, 0x34, 0x33, 0x32, 0x37, 0x36,
    0x35, 0x3a, 0x39, 0x38, 0x3d, 0x3c, 0x3b, 0x00, 0x3f};
static const uint8_t kPermRAWToRGB24_2[64] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x40, 0x3f};
static const uint8_t kPermRAWToRGB24_3[64] = {
    0x3e, 0x43, 0x42, 0x41, 0x46, 0x45, 0x44, 0x49, 0x48, 0x47, 0x4c,
    0x4b, 0x4a, 0x4f, 0x4e, 0x4d, 0x52, 0x51, 0x50, 0x55, 0x54, 0x53,
    0x58, 0x57, 0x56, 0x5b, 0x5a, 0x59, 0x5e, 0x5d, 0x5c, 0x61, 0x60,
    0x5f, 0x64, 0x63, 0x62, 0x67, 0x66, 0x65, 0x6a, 0x69, 0x68, 0x6d,
    0x6c, 0x6b, 0x70, 0x6f, 0x6e, 0x73, 0x72, 0x71, 0x76, 0x75, 0x74,
    0x79, 0x78, 0x77, 0x7c, 0x7b, 0x7a, 0x7f, 0x7e, 0x7d};
#endif  // defined(HAS_RAWTORGB24ROW_AVX512VBMI)

#if defined(HAS_RAWTOARGBROW_AVX512BW) || defined(HAS_RGB24TOARGBROW_AVX512BW)
LIBYUV_TARGET_AVX512BW
static void RGBToARGBRow_AVX512BW(const uint8_t* src_raw,
                                  uint8_t* dst_argb,
                                  const uint32_t* shuffler,
                                  int width) {
  __m512i zmm_alpha = _mm512_set1_epi32(0xff000000);
  __m512i zmm_q0 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB0_AVX512BW);
  __m512i zmm_q1 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB1_AVX512BW);
  __m512i zmm_q2 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB2_AVX512BW);
  __m512i zmm_q3 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB3_AVX512BW);
  __m512i zmm_shuf =
      _mm512_broadcast_i64x4(_mm256_loadu_si256((const __m256i*)shuffler));

  while (width >= 64) {
    __m512i zmm_raw0 = _mm512_loadu_si512((const __m512i*)src_raw);
    __m512i zmm_raw1 = _mm512_loadu_si512((const __m512i*)(src_raw + 64));
    __m512i zmm_raw2 = _mm512_loadu_si512((const __m512i*)(src_raw + 128));

    __m512i zmm0 = _mm512_permutexvar_epi64(zmm_q0, zmm_raw0);
    __m512i zmm1 = _mm512_permutex2var_epi64(zmm_raw0, zmm_q1, zmm_raw1);
    __m512i zmm2 = _mm512_permutex2var_epi64(zmm_raw1, zmm_q2, zmm_raw2);
    __m512i zmm3 = _mm512_permutexvar_epi64(zmm_q3, zmm_raw2);

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
  if (width > 0) {
    uint64_t dst_mask = (1ULL << width) - 1ULL;
    int src_bytes = width * 3;
    __mmask64 src_mask0 =
        (src_bytes >= 64) ? ~0ULL : ((1ULL << src_bytes) - 1ULL);
    __mmask64 src_mask1 =
        (src_bytes >= 128)
            ? ~0ULL
            : ((src_bytes > 64) ? ((1ULL << (src_bytes - 64)) - 1ULL) : 0ULL);
    __mmask64 src_mask2 =
        (src_bytes > 128) ? ((1ULL << (src_bytes - 128)) - 1ULL) : 0ULL;

    __m512i zmm_raw0 = _mm512_maskz_loadu_epi8(src_mask0, src_raw);
    __m512i zmm_raw1 = _mm512_maskz_loadu_epi8(src_mask1, src_raw + 64);
    __m512i zmm_raw2 = _mm512_maskz_loadu_epi8(src_mask2, src_raw + 128);

    __m512i zmm0 = _mm512_permutexvar_epi64(zmm_q0, zmm_raw0);
    __m512i zmm1 = _mm512_permutex2var_epi64(zmm_raw0, zmm_q1, zmm_raw1);
    __m512i zmm2 = _mm512_permutex2var_epi64(zmm_raw1, zmm_q2, zmm_raw2);
    __m512i zmm3 = _mm512_permutexvar_epi64(zmm_q3, zmm_raw2);

    zmm0 = _mm512_shuffle_epi8(zmm0, zmm_shuf);
    zmm1 = _mm512_shuffle_epi8(zmm1, zmm_shuf);
    zmm2 = _mm512_shuffle_epi8(zmm2, zmm_shuf);
    zmm3 = _mm512_shuffle_epi8(zmm3, zmm_shuf);

    zmm0 = _mm512_or_si512(zmm0, zmm_alpha);
    zmm1 = _mm512_or_si512(zmm1, zmm_alpha);
    zmm2 = _mm512_or_si512(zmm2, zmm_alpha);
    zmm3 = _mm512_or_si512(zmm3, zmm_alpha);

    _mm512_mask_storeu_epi32(dst_argb, (__mmask16)dst_mask, zmm0);
    _mm512_mask_storeu_epi32(dst_argb + 64, (__mmask16)(dst_mask >> 16), zmm1);
    _mm512_mask_storeu_epi32(dst_argb + 128, (__mmask16)(dst_mask >> 32), zmm2);
    _mm512_mask_storeu_epi32(dst_argb + 192, (__mmask16)(dst_mask >> 48), zmm3);
  }
  _mm256_zeroupper();
}
#endif  // defined(HAS_RAWTOARGBROW_AVX512BW) ||
        // defined(HAS_RGB24TOARGBROW_AVX512BW)

#ifdef HAS_RAWTOARGBROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void RAWToARGBRow_AVX512BW(const uint8_t* src_raw,
                           uint8_t* dst_argb,
                           int width) {
  RGBToARGBRow_AVX512BW(src_raw, dst_argb,
                        (const uint32_t*)kShuffleMaskRAWToARGB_AVX512BW, width);
}
#endif  // HAS_RAWTOARGBROW_AVX512BW

#ifdef HAS_RGB24TOARGBROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void RGB24ToARGBRow_AVX512BW(const uint8_t* src_rgb24,
                             uint8_t* dst_argb,
                             int width) {
  RGBToARGBRow_AVX512BW(src_rgb24, dst_argb,
                        (const uint32_t*)kShuffleMaskRGB24ToARGB_AVX512BW,
                        width);
}
#endif  // HAS_RGB24TOARGBROW_AVX512BW

#ifdef HAS_RAWTORGB24ROW_SSSE3
LIBYUV_TARGET_SSSE3
void RAWToRGB24Row_SSSE3(const uint8_t* src_raw,
                         uint8_t* dst_rgb24,
                         int width) {
  __m128i shuf0 = _mm_loadu_si128((const __m128i*)kShuffleMaskRAWToRGB24_0);
  __m128i shuf1 = _mm_loadu_si128((const __m128i*)kShuffleMaskRAWToRGB24_1);
  __m128i shuf2 = _mm_loadu_si128((const __m128i*)kShuffleMaskRAWToRGB24_2);

  while (width > 0) {
    __m128i raw0 = _mm_loadu_si128((const __m128i*)src_raw);
    __m128i raw1 = _mm_loadu_si128((const __m128i*)(src_raw + 4));
    __m128i raw2 = _mm_loadu_si128((const __m128i*)(src_raw + 8));

    raw0 = _mm_shuffle_epi8(raw0, shuf0);
    raw1 = _mm_shuffle_epi8(raw1, shuf1);
    raw2 = _mm_shuffle_epi8(raw2, shuf2);

    _mm_storel_epi64((__m128i*)dst_rgb24, raw0);
    _mm_storel_epi64((__m128i*)(dst_rgb24 + 8), raw1);
    _mm_storel_epi64((__m128i*)(dst_rgb24 + 16), raw2);

    src_raw += 24;
    dst_rgb24 += 24;
    width -= 8;
  }
}
#endif  // HAS_RAWTORGB24ROW_SSSE3

#ifdef HAS_RAWTORGB24ROW_AVX2
LIBYUV_TARGET_AVX2
void RAWToRGB24Row_AVX2(const uint8_t* src_raw, uint8_t* dst_rgb24, int width) {
  __m256i ymm_shuf =
      _mm256_loadu_si256((const __m256i*)kShuffleMaskRAWToRGB24_AVX2);
  __m256i ymm_p0 = _mm256_loadu_si256((const __m256i*)kPermd0_AVX2);
  __m256i ymm_p1 = _mm256_loadu_si256((const __m256i*)kPermd1_AVX2);
  __m256i ymm_p2 = _mm256_loadu_si256((const __m256i*)kPermd2_AVX2);

  while (width > 0) {
    __m256i raw0 = _mm256_loadu_si256((const __m256i*)src_raw);
    __m256i raw1 = _mm256_loadu_si256((const __m256i*)(src_raw + 32));
    __m256i raw2 = _mm256_loadu_si256((const __m256i*)(src_raw + 64));

    __m256i m01 = _mm256_permute2x128_si256(raw0, raw1, 0x21);
    __m256i b0 = _mm256_permute4x64_epi64(raw0, 0x94);
    __m256i m12 = _mm256_permute2x128_si256(raw1, raw2, 0x21);
    __m256i b3 = _mm256_permute4x64_epi64(raw2, 0xe9);
    __m256i b1 = _mm256_permute4x64_epi64(m01, 0xe9);
    __m256i b2 = _mm256_permute4x64_epi64(m12, 0x94);

    b0 = _mm256_shuffle_epi8(b0, ymm_shuf);
    b1 = _mm256_shuffle_epi8(b1, ymm_shuf);
    b2 = _mm256_shuffle_epi8(b2, ymm_shuf);
    b3 = _mm256_shuffle_epi8(b3, ymm_shuf);

    __m256i d0 = _mm256_permutevar8x32_epi32(b0, ymm_p0);
    __m256i d1 = _mm256_permutevar8x32_epi32(b1, ymm_p1);
    __m256i d2 = _mm256_permutevar8x32_epi32(b2, ymm_p2);
    __m256i d3 = _mm256_permutevar8x32_epi32(b3, ymm_p0);

    // Dst0 = blend(d0, d1)
    __m256i dst0 = _mm256_blend_epi32(d0, d1, 0xc0);
    _mm256_storeu_si256((__m256i*)dst_rgb24, dst0);

    // Dst1 = blend(d1, d2)
    __m256i dst1 = _mm256_blend_epi32(d1, d2, 0xf0);
    d3 = _mm256_permute4x64_epi64(d3, 0x90);
    _mm256_storeu_si256((__m256i*)(dst_rgb24 + 32), dst1);

    // Dst2 = blend(d2, d3)
    __m256i dst2 = _mm256_blend_epi32(d2, d3, 0xfc);
    _mm256_storeu_si256((__m256i*)(dst_rgb24 + 64), dst2);

    src_raw += 96;
    dst_rgb24 += 96;
    width -= 32;
  }
  _mm256_zeroupper();
}
#endif  // HAS_RAWTORGB24ROW_AVX2

#ifdef HAS_RAWTORGB24ROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void RAWToRGB24Row_AVX512BW(const uint8_t* src_raw,
                            uint8_t* dst_rgb24,
                            int width) {
  __m512i zmm_q0 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB0_AVX512BW);
  __m512i zmm_q1 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB1_AVX512BW);
  __m512i zmm_q2 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB2_AVX512BW);
  __m512i zmm_q3 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB3_AVX512BW);
  __m512i zmm_shuf = _mm512_broadcast_i64x4(
      _mm256_loadu_si256((const __m256i*)kShuffleMaskRAWToRGB24_AVX2));
  __m512i zmm_d0 = _mm512_loadu_si512((const __m512i*)kPermdRGB24_0_AVX512BW);
  __m512i zmm_d1 = _mm512_loadu_si512((const __m512i*)kPermdRGB24_1_AVX512BW);
  __m512i zmm_d2 = _mm512_loadu_si512((const __m512i*)kPermdRGB24_2_AVX512BW);

  while (width > 0) {
    __m512i zmm_raw0 = _mm512_loadu_si512((const __m512i*)src_raw);
    __m512i zmm_raw1 = _mm512_loadu_si512((const __m512i*)(src_raw + 64));
    __m512i zmm_raw2 = _mm512_loadu_si512((const __m512i*)(src_raw + 128));

    __m512i zmm0 = _mm512_permutexvar_epi64(zmm_q0, zmm_raw0);
    __m512i zmm1 = _mm512_permutex2var_epi64(zmm_raw0, zmm_q1, zmm_raw1);
    __m512i zmm2 = _mm512_permutex2var_epi64(zmm_raw1, zmm_q2, zmm_raw2);
    __m512i zmm3 = _mm512_permutexvar_epi64(zmm_q3, zmm_raw2);

    zmm0 = _mm512_shuffle_epi8(zmm0, zmm_shuf);
    zmm1 = _mm512_shuffle_epi8(zmm1, zmm_shuf);
    zmm2 = _mm512_shuffle_epi8(zmm2, zmm_shuf);
    zmm3 = _mm512_shuffle_epi8(zmm3, zmm_shuf);

    __m512i dst0 = _mm512_permutex2var_epi32(zmm0, zmm_d0, zmm1);
    __m512i dst1 = _mm512_permutex2var_epi32(zmm1, zmm_d1, zmm2);
    __m512i dst2 = _mm512_permutex2var_epi32(zmm2, zmm_d2, zmm3);

    _mm512_storeu_si512((__m512i*)dst_rgb24, dst0);
    _mm512_storeu_si512((__m512i*)(dst_rgb24 + 64), dst1);
    _mm512_storeu_si512((__m512i*)(dst_rgb24 + 128), dst2);

    src_raw += 192;
    dst_rgb24 += 192;
    width -= 64;
  }
  _mm256_zeroupper();
}
#endif  // HAS_RAWTORGB24ROW_AVX512BW

#ifdef HAS_RAWTORGB24ROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void RAWToRGB24Row_AVX512VBMI(const uint8_t* src_raw,
                              uint8_t* dst_rgb24,
                              int width) {
  __m512i zmm_p0 = _mm512_loadu_si512((const __m512i*)kPermRAWToRGB24_0);
  __m512i zmm_p1 = _mm512_loadu_si512((const __m512i*)kPermRAWToRGB24_1);
  __m512i zmm_p2 = _mm512_loadu_si512((const __m512i*)kPermRAWToRGB24_2);
  __m512i zmm_p3 = _mm512_loadu_si512((const __m512i*)kPermRAWToRGB24_3);

  while (width > 0) {
    __m512i zmm_raw0 = _mm512_loadu_si512((const __m512i*)src_raw);
    __m512i zmm_raw1 = _mm512_loadu_si512((const __m512i*)(src_raw + 64));
    __m512i zmm_raw2 = _mm512_loadu_si512((const __m512i*)(src_raw + 128));

    __m512i tmp1 = _mm512_permutex2var_epi8(zmm_raw1, zmm_p1, zmm_raw0);
    __m512i dst0 = _mm512_permutex2var_epi8(zmm_raw0, zmm_p0, zmm_raw1);
    __m512i dst2 = _mm512_permutex2var_epi8(zmm_raw1, zmm_p3, zmm_raw2);
    __m512i dst1 = _mm512_permutex2var_epi8(tmp1, zmm_p2, zmm_raw2);

    _mm512_storeu_si512((__m512i*)dst_rgb24, dst0);
    _mm512_storeu_si512((__m512i*)(dst_rgb24 + 64), dst1);
    _mm512_storeu_si512((__m512i*)(dst_rgb24 + 128), dst2);

    src_raw += 192;
    dst_rgb24 += 192;
    width -= 64;
  }
  _mm256_zeroupper();
}
#endif  // HAS_RAWTORGB24ROW_AVX512VBMI

#ifdef HAS_RGBTOYMATRIXROW_AVX512BW
static const uint32_t kPermdARGBToY_AVX512BW[16] = {0, 4, 8,  12, 1, 5, 9,  13,
                                                    2, 6, 10, 14, 3, 7, 11, 15};

LIBYUV_TARGET_AVX512BW
void RGBToYMatrixRow_AVX512BW(const uint8_t* src_rgb,
                              uint8_t* dst_y,
                              int width,
                              const struct ArgbConstants* c) {
  __m512i zmm_q0 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB0_AVX512BW);
  __m512i zmm_q1 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB1_AVX512BW);
  __m512i zmm_q2 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB2_AVX512BW);
  __m512i zmm_q3 =
      _mm512_loadu_si512((const __m512i*)kPermqRGB24ToARGB3_AVX512BW);
  __m512i zmm_shuf = _mm512_broadcast_i64x4(
      _mm256_loadu_si256((const __m256i*)kShuffleMaskRGB24ToARGB_AVX512BW));
  __m512i zmm_perm_y =
      _mm512_loadu_si512((const __m512i*)kPermdARGBToY_AVX512BW);

  __m512i zmm_80 = _mm512_set1_epi8((char)0x80);
  __m512i zmm_one = _mm512_set1_epi16(1);
  __m512i zmm_coeff =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)c->kRGBToY));
  __m512i zmm_add =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)c->kAddY));

  // Compute bias adjusted for 0x80 subtraction:
  // bias = kAddY - pmaddwd(pmaddubsw(kRGBToY, 0x80), 1)
  __m512i zmm_bias = _mm512_maddubs_epi16(zmm_coeff, zmm_80);
  zmm_bias = _mm512_madd_epi16(zmm_bias, zmm_one);
  zmm_bias = _mm512_packs_epi32(zmm_bias, zmm_bias);
  zmm_add = _mm512_sub_epi16(zmm_add, zmm_bias);

  while (width > 0) {
    __m512i zmm_raw0 = _mm512_loadu_si512((const __m512i*)src_rgb);
    __m512i zmm_raw1 = _mm512_loadu_si512((const __m512i*)(src_rgb + 64));
    __m512i zmm_raw2 = _mm512_loadu_si512((const __m512i*)(src_rgb + 128));

    __m512i zmm0 = _mm512_permutexvar_epi64(zmm_q0, zmm_raw0);
    __m512i zmm1 = _mm512_permutex2var_epi64(zmm_raw0, zmm_q1, zmm_raw1);
    __m512i zmm2 = _mm512_permutex2var_epi64(zmm_raw1, zmm_q2, zmm_raw2);
    __m512i zmm3 = _mm512_permutexvar_epi64(zmm_q3, zmm_raw2);

    zmm0 = _mm512_shuffle_epi8(zmm0, zmm_shuf);
    zmm1 = _mm512_shuffle_epi8(zmm1, zmm_shuf);
    zmm2 = _mm512_shuffle_epi8(zmm2, zmm_shuf);
    zmm3 = _mm512_shuffle_epi8(zmm3, zmm_shuf);

    zmm0 = _mm512_xor_si512(zmm0, zmm_80);
    zmm1 = _mm512_xor_si512(zmm1, zmm_80);
    zmm2 = _mm512_xor_si512(zmm2, zmm_80);
    zmm3 = _mm512_xor_si512(zmm3, zmm_80);

    zmm0 = _mm512_maddubs_epi16(zmm_coeff, zmm0);
    zmm1 = _mm512_maddubs_epi16(zmm_coeff, zmm1);
    zmm2 = _mm512_maddubs_epi16(zmm_coeff, zmm2);
    zmm3 = _mm512_maddubs_epi16(zmm_coeff, zmm3);

    zmm0 = _mm512_madd_epi16(zmm0, zmm_one);
    zmm1 = _mm512_madd_epi16(zmm1, zmm_one);
    zmm0 = _mm512_packs_epi32(zmm0, zmm1);

    zmm2 = _mm512_madd_epi16(zmm2, zmm_one);
    zmm3 = _mm512_madd_epi16(zmm3, zmm_one);
    zmm2 = _mm512_packs_epi32(zmm2, zmm3);

    zmm0 = _mm512_add_epi16(zmm0, zmm_add);
    zmm2 = _mm512_add_epi16(zmm2, zmm_add);

    zmm0 = _mm512_srli_epi16(zmm0, 8);
    zmm2 = _mm512_srli_epi16(zmm2, 8);

    zmm0 = _mm512_packus_epi16(zmm0, zmm2);
    zmm0 = _mm512_permutexvar_epi32(zmm_perm_y, zmm0);

    _mm512_storeu_si512((__m512i*)dst_y, zmm0);

    src_rgb += 192;
    dst_y += 64;
    width -= 64;
  }
  _mm256_zeroupper();
}
#endif  // HAS_RGBTOYMATRIXROW_AVX512BW

#ifdef HAS_ARGBTOUVMATRIXROW_AVX2
LIBYUV_TARGET_AVX2
void ARGBToUVMatrixRow_AVX2(const uint8_t* src_argb,
                            int src_stride_argb,
                            uint8_t* dst_u,
                            uint8_t* dst_v,
                            int width,
                            const struct ArgbConstants* c) {
  __m256i ymm_u =
      _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)c->kRGBToU));
  __m256i ymm_v =
      _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)c->kRGBToV));
  __m256i ymm_0101 = _mm256_set1_epi16(0x0101);
  __m256i ymm_shuf =
      _mm256_setr_epi8(0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15, 0,
                       4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15);
  __m256i ymm_8000 = _mm256_set1_epi16((short)0x8000);
  __m256i ymm_zero = _mm256_setzero_si256();

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb);
    __m256i ymm1 = _mm256_loadu_si256((const __m256i*)(src_argb + 32));
    __m256i ymm2 =
        _mm256_loadu_si256((const __m256i*)(src_argb + src_stride_argb));
    __m256i ymm3 =
        _mm256_loadu_si256((const __m256i*)(src_argb + src_stride_argb + 32));

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
  _mm256_zeroupper();
}
#endif  // HAS_ARGBTOUVMATRIXROW_AVX2

#ifdef HAS_CONVERT8TO16ROW_AVX2
LIBYUV_TARGET_AVX2
void Convert8To16Row_AVX2(const uint8_t* src_y,
                          uint16_t* dst_y,
                          int bits,
                          int width) {
  const int shift = 16 - bits;
  __m128i shift128 = _mm_cvtsi32_si128(shift);
  do {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_y);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    __m256i row0 = _mm256_unpacklo_epi8(ymm0, ymm0);
    __m256i row1 = _mm256_unpackhi_epi8(ymm0, ymm0);
    row0 = _mm256_srl_epi16(row0, shift128);
    row1 = _mm256_srl_epi16(row1, shift128);
    _mm256_storeu_si256((__m256i*)dst_y, row0);
    _mm256_storeu_si256((__m256i*)(dst_y + 16), row1);
    src_y += 32;
    dst_y += 32;
    width -= 32;
  } while (width > 0);
  _mm256_zeroupper();
}
#endif  // HAS_CONVERT8TO16ROW_AVX2

#ifdef HAS_MULTIPLYROW_16_AVX2
LIBYUV_TARGET_AVX2
void MultiplyRow_16_AVX2(const uint16_t* src_y,
                         uint16_t* dst_y,
                         int scale,
                         int width) {
  __m256i scale256 = _mm256_set1_epi16((short)scale);
  do {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_y);
    __m256i ymm1 = _mm256_loadu_si256((const __m256i*)(src_y + 16));
    ymm0 = _mm256_mullo_epi16(ymm0, scale256);
    ymm1 = _mm256_mullo_epi16(ymm1, scale256);
    _mm256_storeu_si256((__m256i*)dst_y, ymm0);
    _mm256_storeu_si256((__m256i*)(dst_y + 16), ymm1);
    src_y += 32;
    dst_y += 32;
    width -= 32;
  } while (width > 0);
  _mm256_zeroupper();
}
#endif  // HAS_MULTIPLYROW_16_AVX2

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
  _mm256_zeroupper();
}
#endif  // HAS_MERGEUVROW_AVX2

#ifdef HAS_MIRRORROW_AVX2
LIBYUV_TARGET_AVX2
void MirrorRow_AVX2(const uint8_t* src, uint8_t* dst, int width) {
  __m256i ymm_shuf = _mm256_broadcastsi128_si256(
      _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0));
  src += width;
  while (width > 0) {
    src -= 32;
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src);
    ymm0 = _mm256_shuffle_epi8(ymm0, ymm_shuf);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0x4e);
    _mm256_storeu_si256((__m256i*)dst, ymm0);
    dst += 32;
    width -= 32;
  }
  _mm256_zeroupper();
}
#endif  // HAS_MIRRORROW_AVX2

#ifdef HAS_MIRRORUVROW_AVX2
LIBYUV_TARGET_AVX2
void MirrorUVRow_AVX2(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  __m256i ymm_shuf = _mm256_broadcastsi128_si256(
      _mm_setr_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1));
  src_uv += width * 2;
  while (width > 0) {
    src_uv -= 32;
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_uv);
    ymm0 = _mm256_shuffle_epi8(ymm0, ymm_shuf);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0x4e);
    _mm256_storeu_si256((__m256i*)dst_uv, ymm0);
    dst_uv += 32;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_MIRRORUVROW_AVX2

#ifdef HAS_MIRRORUVROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void MirrorUVRow_AVX512BW(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  const __m512i zmm_idx = _mm512_set_epi16(
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
      21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
  src_uv += width * 2;
  while (width > 0) {
    src_uv -= 64;
    __m512i zmm0 = _mm512_loadu_si512((const __m512i*)src_uv);
    zmm0 = _mm512_permutexvar_epi16(zmm_idx, zmm0);
    _mm512_storeu_si512((__m512i*)dst_uv, zmm0);
    dst_uv += 64;
    width -= 32;
  }
  _mm256_zeroupper();
}
#endif  // HAS_MIRRORUVROW_AVX512BW

#ifdef HAS_SWAPUVROW_AVX2
LIBYUV_TARGET_AVX2
void SwapUVRow_AVX2(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
  __m256i ymm_shuf = _mm256_broadcastsi128_si256(
      _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14));
  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_uv);
    __m256i ymm1 = _mm256_loadu_si256((const __m256i*)(src_uv + 32));
    ymm0 = _mm256_shuffle_epi8(ymm0, ymm_shuf);
    ymm1 = _mm256_shuffle_epi8(ymm1, ymm_shuf);
    _mm256_storeu_si256((__m256i*)dst_vu, ymm0);
    _mm256_storeu_si256((__m256i*)(dst_vu + 32), ymm1);
    src_uv += 64;
    dst_vu += 64;
    width -= 32;
  }
  _mm256_zeroupper();
}
#endif  // HAS_SWAPUVROW_AVX2

#ifdef HAS_MIRRORSPLITUVROW_AVX2
LIBYUV_TARGET_AVX2
void MirrorSplitUVRow_AVX2(const uint8_t* src_uv,
                           uint8_t* dst_u,
                           uint8_t* dst_v,
                           int width) {
  __m256i ymm_shuf = _mm256_broadcastsi128_si256(
      _mm_setr_epi8(14, 12, 10, 8, 6, 4, 2, 0, 15, 13, 11, 9, 7, 5, 3, 1));
  src_uv += width * 2;
  while (width > 0) {
    src_uv -= 32;
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_uv);
    ymm0 = _mm256_shuffle_epi8(ymm0, ymm_shuf);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0x72);
    _mm_storeu_si128((__m128i*)dst_u, _mm256_castsi256_si128(ymm0));
    _mm_storeu_si128((__m128i*)dst_v, _mm256_extracti128_si256(ymm0, 1));
    dst_u += 16;
    dst_v += 16;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_MIRRORSPLITUVROW_AVX2

#ifdef HAS_RGB24MIRRORROW_AVX2
LIBYUV_TARGET_AVX2
void RGB24MirrorRow_AVX2(const uint8_t* src_rgb24,
                         uint8_t* dst_rgb24,
                         int width) {
  __m256i shuf0 =
      _mm256_setr_epi8(-1, 12, 13, 14, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2, -1,
                       12, 13, 14, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2);
  __m128i shuf1 =
      _mm_setr_epi8(13, 14, 15, 10, 11, 12, 7, 8, 9, 4, 5, 6, 1, 2, 3, -1);

  src_rgb24 += width * 3 - 96;
  while (width > 0) {
    __m128i v0_lo = _mm_loadu_si128((const __m128i*)(src_rgb24 + 0));
    __m128i v0_hi = _mm_loadu_si128((const __m128i*)(src_rgb24 + 15));
    __m256i v0 =
        _mm256_inserti128_si256(_mm256_castsi128_si256(v0_lo), v0_hi, 1);

    __m128i v1_lo = _mm_loadu_si128((const __m128i*)(src_rgb24 + 30));
    __m128i v1_hi = _mm_loadu_si128((const __m128i*)(src_rgb24 + 45));
    __m256i v1 =
        _mm256_inserti128_si256(_mm256_castsi128_si256(v1_lo), v1_hi, 1);

    __m128i v2_lo = _mm_loadu_si128((const __m128i*)(src_rgb24 + 60));
    __m128i v2_hi = _mm_loadu_si128((const __m128i*)(src_rgb24 + 75));
    __m256i v2 =
        _mm256_inserti128_si256(_mm256_castsi128_si256(v2_lo), v2_hi, 1);

    __m128i v3 = _mm_loadu_si128((const __m128i*)(src_rgb24 + 80));

    v0 = _mm256_shuffle_epi8(v0, shuf0);
    v1 = _mm256_shuffle_epi8(v1, shuf0);
    v2 = _mm256_shuffle_epi8(v2, shuf0);
    v3 = _mm_shuffle_epi8(v3, shuf1);

    _mm_storeu_si128((__m128i*)(dst_rgb24 + 80), _mm256_castsi256_si128(v0));
    _mm_storeu_si128((__m128i*)(dst_rgb24 + 65),
                     _mm256_extracti128_si256(v0, 1));
    _mm_storeu_si128((__m128i*)(dst_rgb24 + 50), _mm256_castsi256_si128(v1));
    _mm_storeu_si128((__m128i*)(dst_rgb24 + 35),
                     _mm256_extracti128_si256(v1, 1));
    _mm_storeu_si128((__m128i*)(dst_rgb24 + 20), _mm256_castsi256_si128(v2));
    _mm_storeu_si128((__m128i*)(dst_rgb24 + 5),
                     _mm256_extracti128_si256(v2, 1));
    _mm_storel_epi64((__m128i*)(dst_rgb24 + 0), v3);

    src_rgb24 -= 96;
    dst_rgb24 += 96;
    width -= 32;
  }
  _mm256_zeroupper();
}
#endif  // HAS_RGB24MIRRORROW_AVX2

#ifdef HAS_INTERPOLATEROW_AVX2
LIBYUV_TARGET_AVX2
void InterpolateRow_AVX2(uint8_t* dst_ptr,
                         const uint8_t* src_ptr,
                         ptrdiff_t src_stride,
                         int width,
                         int source_y_fraction) {
  int y1 = source_y_fraction;
  int y0 = 256 - y1;
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  __m256i ymm_y = _mm256_set1_epi16((y1 << 8) | y0);
  __m256i ymm_8080 = _mm256_set1_epi16(0x8080);
  int i;

  if (y1 == 0) {
    for (i = 0; i < width; i += 32) {
      _mm256_storeu_si256((__m256i*)(dst_ptr + i),
                          _mm256_loadu_si256((const __m256i*)(src_ptr + i)));
    }
  } else if (y1 == 128) {
    for (i = 0; i < width; i += 32) {
      __m256i row0 = _mm256_loadu_si256((const __m256i*)(src_ptr + i));
      __m256i row1 = _mm256_loadu_si256((const __m256i*)(src_ptr1 + i));
      _mm256_storeu_si256((__m256i*)(dst_ptr + i), _mm256_avg_epu8(row0, row1));
    }
  } else {
    for (i = 0; i < width; i += 32) {
      __m256i row0 = _mm256_loadu_si256((const __m256i*)(src_ptr + i));
      __m256i row1 = _mm256_loadu_si256((const __m256i*)(src_ptr1 + i));
      __m256i low = _mm256_unpacklo_epi8(row0, row1);
      __m256i high = _mm256_unpackhi_epi8(row0, row1);
      low = _mm256_sub_epi8(low, ymm_8080);
      high = _mm256_sub_epi8(high, ymm_8080);
      low = _mm256_maddubs_epi16(ymm_y, low);
      high = _mm256_maddubs_epi16(ymm_y, high);
      low = _mm256_add_epi16(low, ymm_8080);
      high = _mm256_add_epi16(high, ymm_8080);
      low = _mm256_srli_epi16(low, 8);
      high = _mm256_srli_epi16(high, 8);
      _mm256_storeu_si256((__m256i*)(dst_ptr + i),
                          _mm256_packus_epi16(low, high));
    }
  }
  _mm256_zeroupper();
}
#endif  // HAS_INTERPOLATEROW_AVX2

#ifdef HAS_INTERPOLATEROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void InterpolateRow_AVX512BW(uint8_t* dst_ptr,
                             const uint8_t* src_ptr,
                             ptrdiff_t src_stride,
                             int width,
                             int source_y_fraction) {
  int y1 = source_y_fraction;
  int y0 = 256 - y1;
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  __m512i zmm_y = _mm512_set1_epi16((short)((y1 << 8) | y0));
  __m512i zmm_8080 = _mm512_set1_epi16((short)0x8080);
  int i;

  if (y1 == 0) {
    for (i = 0; i <= width - 64; i += 64) {
      _mm512_storeu_si512((__m512i*)(dst_ptr + i),
                          _mm512_loadu_si512((const __m512i*)(src_ptr + i)));
    }
    if (i < width) {
      __mmask64 mask = (__mmask64)((1ull << (width - i)) - 1ull);
      _mm512_mask_storeu_epi8(dst_ptr + i, mask,
                              _mm512_maskz_loadu_epi8(mask, src_ptr + i));
    }
  } else if (y1 == 128) {
    for (i = 0; i <= width - 64; i += 64) {
      __m512i row0 = _mm512_loadu_si512((const __m512i*)(src_ptr + i));
      __m512i row1 = _mm512_loadu_si512((const __m512i*)(src_ptr1 + i));
      _mm512_storeu_si512((__m512i*)(dst_ptr + i), _mm512_avg_epu8(row0, row1));
    }
    if (i < width) {
      __mmask64 mask = (__mmask64)((1ull << (width - i)) - 1ull);
      __m512i row0 = _mm512_maskz_loadu_epi8(mask, src_ptr + i);
      __m512i row1 = _mm512_maskz_loadu_epi8(mask, src_ptr1 + i);
      _mm512_mask_storeu_epi8(dst_ptr + i, mask, _mm512_avg_epu8(row0, row1));
    }
  } else {
    for (i = 0; i <= width - 64; i += 64) {
      __m512i row0 = _mm512_loadu_si512((const __m512i*)(src_ptr + i));
      __m512i row1 = _mm512_loadu_si512((const __m512i*)(src_ptr1 + i));
      __m512i low = _mm512_unpacklo_epi8(row0, row1);
      __m512i high = _mm512_unpackhi_epi8(row0, row1);
      low = _mm512_sub_epi8(low, zmm_8080);
      high = _mm512_sub_epi8(high, zmm_8080);
      low = _mm512_maddubs_epi16(zmm_y, low);
      high = _mm512_maddubs_epi16(zmm_y, high);
      low = _mm512_add_epi16(low, zmm_8080);
      high = _mm512_add_epi16(high, zmm_8080);
      low = _mm512_srli_epi16(low, 8);
      high = _mm512_srli_epi16(high, 8);
      _mm512_storeu_si512((__m512i*)(dst_ptr + i),
                          _mm512_packus_epi16(low, high));
    }
    if (i < width) {
      __mmask64 mask = (__mmask64)((1ull << (width - i)) - 1ull);
      __m512i row0 = _mm512_maskz_loadu_epi8(mask, src_ptr + i);
      __m512i row1 = _mm512_maskz_loadu_epi8(mask, src_ptr1 + i);
      __m512i low = _mm512_unpacklo_epi8(row0, row1);
      __m512i high = _mm512_unpackhi_epi8(row0, row1);
      low = _mm512_sub_epi8(low, zmm_8080);
      high = _mm512_sub_epi8(high, zmm_8080);
      low = _mm512_maddubs_epi16(zmm_y, low);
      high = _mm512_maddubs_epi16(zmm_y, high);
      low = _mm512_add_epi16(low, zmm_8080);
      high = _mm512_add_epi16(high, zmm_8080);
      low = _mm512_srli_epi16(low, 8);
      high = _mm512_srli_epi16(high, 8);
      _mm512_mask_storeu_epi8(dst_ptr + i, mask,
                              _mm512_packus_epi16(low, high));
    }
  }
  _mm256_zeroupper();
}
#endif  // HAS_INTERPOLATEROW_AVX512BW

#ifdef HAS_INTERPOLATEROW_16_AVX2
LIBYUV_TARGET_AVX2
void InterpolateRow_16_AVX2(uint16_t* dst_ptr,
                            const uint16_t* src_ptr,
                            ptrdiff_t src_stride,
                            int width,
                            int source_y_fraction) {
  int y1 = source_y_fraction;
  int y0 = 256 - y1;
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  __m256i ymm_y = _mm256_set1_epi32((y1 << 16) | y0);
  __m256i ymm_8000 = _mm256_set1_epi16((short)0x8000);
  __m256i ymm_round = _mm256_set1_epi32(8388736);  // 0x800000 + 128
  int i;

  if (y1 == 0) {
    for (i = 0; i < width; i += 16) {
      _mm256_storeu_si256((__m256i*)(dst_ptr + i),
                          _mm256_loadu_si256((const __m256i*)(src_ptr + i)));
    }
  } else if (y1 == 128) {
    for (i = 0; i < width; i += 16) {
      __m256i row0 = _mm256_loadu_si256((const __m256i*)(src_ptr + i));
      __m256i row1 = _mm256_loadu_si256((const __m256i*)(src_ptr1 + i));
      _mm256_storeu_si256((__m256i*)(dst_ptr + i),
                          _mm256_avg_epu16(row0, row1));
    }
  } else {
    for (i = 0; i < width; i += 16) {
      __m256i row0 = _mm256_loadu_si256((const __m256i*)(src_ptr + i));
      __m256i row1 = _mm256_loadu_si256((const __m256i*)(src_ptr1 + i));
      __m256i row0l = _mm256_unpacklo_epi16(row0, row1);
      __m256i row0h = _mm256_unpackhi_epi16(row0, row1);
      row0l = _mm256_sub_epi16(row0l, ymm_8000);
      row0h = _mm256_sub_epi16(row0h, ymm_8000);
      __m256i resl = _mm256_madd_epi16(row0l, ymm_y);
      __m256i resh = _mm256_madd_epi16(row0h, ymm_y);
      resl = _mm256_add_epi32(resl, ymm_round);
      resh = _mm256_add_epi32(resh, ymm_round);
      resl = _mm256_srai_epi32(resl, 8);
      resh = _mm256_srai_epi32(resh, 8);
      _mm256_storeu_si256((__m256i*)(dst_ptr + i),
                          _mm256_packus_epi32(resl, resh));
    }
  }
  _mm256_zeroupper();
}
#endif  // HAS_INTERPOLATEROW_16_AVX2

#ifdef HAS_ARGBMIRRORROW_AVX2
LIBYUV_TARGET_AVX2
void ARGBMirrorRow_AVX2(const uint8_t* src, uint8_t* dst, int width) {
  __m256i ymm_shuf = _mm256_setr_epi32(7, 6, 5, 4, 3, 2, 1, 0);
  src += width * 4;
  while (width > 0) {
    src -= 32;
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src);
    ymm0 = _mm256_permutevar8x32_epi32(ymm0, ymm_shuf);
    _mm256_storeu_si256((__m256i*)dst, ymm0);
    dst += 32;
    width -= 8;
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBMIRRORROW_AVX2

#ifdef HAS_J400TOARGBROW_AVX2
static const uint8_t kShuffleMaskJ400ToARGB_0[32] = {
    0u, 0u, 0u, 128u, 1u, 1u, 1u, 128u, 2u, 2u, 2u, 128u, 3u, 3u, 3u, 128u,
    4u, 4u, 4u, 128u, 5u, 5u, 5u, 128u, 6u, 6u, 6u, 128u, 7u, 7u, 7u, 128u};
static const uint8_t kShuffleMaskJ400ToARGB_1[32] = {
    8u,   8u,   8u,  128u, 9u,   9u,   9u,  128u, 10u,  10u, 10u,
    128u, 11u,  11u, 11u,  128u, 12u,  12u, 12u,  128u, 13u, 13u,
    13u,  128u, 14u, 14u,  14u,  128u, 15u, 15u,  15u,  128u};

LIBYUV_TARGET_AVX2
void J400ToARGBRow_AVX2(const uint8_t* src_y, uint8_t* dst_argb, int width) {
  __m256i ymm_mask0 =
      _mm256_loadu_si256((const __m256i*)kShuffleMaskJ400ToARGB_0);
  __m256i ymm_mask1 =
      _mm256_loadu_si256((const __m256i*)kShuffleMaskJ400ToARGB_1);
  __m256i ymm_alpha = _mm256_set1_epi32((int)0xff000000u);

  while (width > 0) {
    __m256i ymm0 =
        _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)src_y));

    __m256i ymm1 = _mm256_shuffle_epi8(ymm0, ymm_mask0);
    __m256i ymm2 = _mm256_shuffle_epi8(ymm0, ymm_mask1);

    ymm1 = _mm256_or_si256(ymm1, ymm_alpha);
    ymm2 = _mm256_or_si256(ymm2, ymm_alpha);

    _mm256_storeu_si256((__m256i*)dst_argb, ymm1);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), ymm2);

    src_y += 16;
    dst_argb += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_J400TOARGBROW_AVX2

#ifdef HAS_RGB24TOARGBROW_AVX2
static const uint8_t kShuffleMaskRGB24ToARGB[2][16] = {
    {0u, 1u, 2u, 128u, 3u, 4u, 5u, 128u, 6u, 7u, 8u, 128u, 9u, 10u, 11u, 128u},
    {4u, 5u, 6u, 128u, 7u, 8u, 9u, 128u, 10u, 11u, 12u, 128u, 13u, 14u, 15u,
     128u}};
#endif  // HAS_RGB24TOARGBROW_AVX2

#ifdef HAS_RGB565TOARGBROW_AVX2
LIBYUV_TARGET_AVX2
void RGB565ToARGBRow_AVX2(const uint8_t* src_rgb565,
                          uint8_t* dst_argb,
                          int width) {
  __m256i ymm_scale_rb = _mm256_set1_epi32(0x01080108);
  __m256i ymm_scale_g = _mm256_set1_epi32(0x20802080);
  __m256i ymm_mask_b = _mm256_set1_epi16((short)0xf800);
  __m256i ymm_mask_g = _mm256_set1_epi16(0x07e0);
  __m256i ymm_mask_a = _mm256_set1_epi16((short)0xff00);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_rgb565);
    __m256i ymm1 = ymm0;
    __m256i ymm2 = ymm0;

    ymm1 = _mm256_and_si256(ymm1, ymm_mask_b);
    ymm2 = _mm256_slli_epi16(ymm2, 11);
    ymm1 = _mm256_mulhi_epu16(ymm1, ymm_scale_rb);
    ymm2 = _mm256_mulhi_epu16(ymm2, ymm_scale_rb);
    ymm1 = _mm256_slli_epi16(ymm1, 8);
    ymm1 = _mm256_or_si256(ymm1, ymm2);  // RB

    ymm0 = _mm256_and_si256(ymm0, ymm_mask_g);
    ymm0 = _mm256_mulhi_epu16(ymm0, ymm_scale_g);
    ymm0 = _mm256_or_si256(ymm0, ymm_mask_a);  // GA

    ymm2 = _mm256_unpacklo_epi8(ymm1, ymm0);
    ymm1 = _mm256_unpackhi_epi8(ymm1, ymm0);

    ymm0 = _mm256_permute2x128_si256(ymm2, ymm1, 0x20);
    ymm1 = _mm256_permute2x128_si256(ymm2, ymm1, 0x31);

    _mm256_storeu_si256((__m256i*)dst_argb, ymm0);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), ymm1);

    src_rgb565 += 32;
    dst_argb += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_RGB565TOARGBROW_AVX2

#ifdef HAS_ARGB1555TOARGBROW_AVX2
LIBYUV_TARGET_AVX2
void ARGB1555ToARGBRow_AVX2(const uint8_t* src_argb1555,
                            uint8_t* dst_argb,
                            int width) {
  __m256i ymm_scale_rb = _mm256_set1_epi32(0x01080108);
  __m256i ymm_scale_g = _mm256_set1_epi32(0x42004200);
  __m256i ymm_mask_b = _mm256_set1_epi16((short)0xf800);
  __m256i ymm_mask_g = _mm256_set1_epi16(0x03e0);
  __m256i ymm_mask_a = _mm256_set1_epi16((short)0xff00);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb1555);
    __m256i ymm1 = ymm0;
    __m256i ymm2 = ymm0;

    ymm1 = _mm256_slli_epi16(ymm1, 1);
    ymm2 = _mm256_slli_epi16(ymm2, 11);
    ymm1 = _mm256_and_si256(ymm1, ymm_mask_b);
    ymm2 = _mm256_mulhi_epu16(ymm2, ymm_scale_rb);
    ymm1 = _mm256_mulhi_epu16(ymm1, ymm_scale_rb);
    ymm1 = _mm256_slli_epi16(ymm1, 8);
    ymm1 = _mm256_or_si256(ymm1, ymm2);  // RB

    ymm2 = ymm0;
    ymm0 = _mm256_and_si256(ymm0, ymm_mask_g);
    ymm2 = _mm256_srai_epi16(ymm2, 8);
    ymm0 = _mm256_mulhi_epu16(ymm0, ymm_scale_g);
    ymm2 = _mm256_and_si256(ymm2, ymm_mask_a);
    ymm0 = _mm256_or_si256(ymm0, ymm2);  // GA

    ymm2 = _mm256_unpacklo_epi8(ymm1, ymm0);
    ymm1 = _mm256_unpackhi_epi8(ymm1, ymm0);

    ymm0 = _mm256_permute2x128_si256(ymm2, ymm1, 0x20);
    ymm1 = _mm256_permute2x128_si256(ymm2, ymm1, 0x31);

    _mm256_storeu_si256((__m256i*)dst_argb, ymm0);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), ymm1);

    src_argb1555 += 32;
    dst_argb += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGB1555TOARGBROW_AVX2

#ifdef HAS_ARGB4444TOARGBROW_AVX2
LIBYUV_TARGET_AVX2
void ARGB4444ToARGBRow_AVX2(const uint8_t* src_argb4444,
                            uint8_t* dst_argb,
                            int width) {
  __m256i ymm_mask = _mm256_set1_epi32(0x0f0f0f0f);
  __m256i ymm_mask2 = _mm256_slli_epi32(ymm_mask, 4);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb4444);
    __m256i ymm2 = ymm0;

    ymm0 = _mm256_and_si256(ymm0, ymm_mask);
    ymm2 = _mm256_and_si256(ymm2, ymm_mask2);

    __m256i ymm1 = ymm0;
    __m256i ymm3 = ymm2;

    ymm1 = _mm256_slli_epi16(ymm1, 4);
    ymm3 = _mm256_srli_epi16(ymm3, 4);

    ymm0 = _mm256_or_si256(ymm0, ymm1);
    ymm2 = _mm256_or_si256(ymm2, ymm3);

    ymm1 = ymm0;
    ymm0 = _mm256_unpacklo_epi8(ymm0, ymm2);
    ymm1 = _mm256_unpackhi_epi8(ymm1, ymm2);

    ymm2 = _mm256_permute2x128_si256(ymm0, ymm1, 0x20);
    ymm1 = _mm256_permute2x128_si256(ymm0, ymm1, 0x31);

    _mm256_storeu_si256((__m256i*)dst_argb, ymm2);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), ymm1);

    src_argb4444 += 32;
    dst_argb += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGB4444TOARGBROW_AVX2

#ifdef HAS_ARGBTORGB565DITHERROW_AVX2
LIBYUV_TARGET_AVX2
void ARGBToRGB565DitherRow_AVX2(const uint8_t* src_argb,
                                uint8_t* dst_rgb,
                                uint32_t dither4,
                                int width) {
  __m128i dither_shuf =
      _mm_setr_epi8(0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3);
  __m128i d128 = _mm_shuffle_epi8(_mm_cvtsi32_si128((int)dither4), dither_shuf);
  __m256i ymm_dither = _mm256_broadcastsi128_si256(d128);
  __m256i mask_b = _mm256_set1_epi32(0x0000001f);
  __m256i mask_g = _mm256_set1_epi32(0x000007e0);
  __m256i mask_r = _mm256_set1_epi32(0x0000f800);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb);
    ymm0 = _mm256_adds_epu8(ymm0, ymm_dither);
    __m256i ymm2 = _mm256_srli_epi32(ymm0, 5);
    __m256i ymm1 = _mm256_srli_epi32(ymm0, 3);
    ymm0 = _mm256_srli_epi32(ymm0, 8);
    ymm2 = _mm256_and_si256(ymm2, mask_g);
    ymm1 = _mm256_and_si256(ymm1, mask_b);
    ymm0 = _mm256_and_si256(ymm0, mask_r);
    ymm1 = _mm256_or_si256(ymm2, ymm1);
    ymm0 = _mm256_or_si256(ymm1, ymm0);
    ymm0 = _mm256_packus_epi32(ymm0, ymm0);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    _mm_storeu_si128((__m128i*)dst_rgb, _mm256_castsi256_si128(ymm0));
    src_argb += 32;
    dst_rgb += 16;
    width -= 8;
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBTORGB565DITHERROW_AVX2

#ifdef HAS_ARGBTORGB565ROW_AVX2
LIBYUV_TARGET_AVX2
void ARGBToRGB565Row_AVX2(const uint8_t* src_argb,
                          uint8_t* dst_rgb,
                          int width) {
  __m256i mask_b = _mm256_set1_epi32(0x0000001f);
  __m256i mask_g = _mm256_set1_epi32(0x000007e0);
  __m256i mask_r = _mm256_set1_epi32(0x0000f800);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb);
    __m256i ymm2 = _mm256_srli_epi32(ymm0, 5);
    __m256i ymm1 = _mm256_srli_epi32(ymm0, 3);
    ymm0 = _mm256_srli_epi32(ymm0, 8);
    ymm2 = _mm256_and_si256(ymm2, mask_g);
    ymm1 = _mm256_and_si256(ymm1, mask_b);
    ymm0 = _mm256_and_si256(ymm0, mask_r);
    ymm1 = _mm256_or_si256(ymm2, ymm1);
    ymm0 = _mm256_or_si256(ymm1, ymm0);
    ymm0 = _mm256_packus_epi32(ymm0, ymm0);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    _mm_storeu_si128((__m128i*)dst_rgb, _mm256_castsi256_si128(ymm0));
    src_argb += 32;
    dst_rgb += 16;
    width -= 8;
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBTORGB565ROW_AVX2

#ifdef HAS_ARGBTOARGB1555ROW_AVX2
LIBYUV_TARGET_AVX2
void ARGBToARGB1555Row_AVX2(const uint8_t* src_argb,
                            uint8_t* dst_argb1555,
                            int width) {
  __m256i mask_b = _mm256_set1_epi32(0x0000001f);
  __m256i mask_g = _mm256_set1_epi32(0x000003e0);
  __m256i mask_r = _mm256_set1_epi32(0x00007c00);
  __m256i mask_a = _mm256_set1_epi32(0x00008000);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb);
    __m256i ymm2 = _mm256_srli_epi32(ymm0, 6);
    __m256i ymm1 = _mm256_srli_epi32(ymm0, 3);
    __m256i ymm7 = _mm256_srli_epi32(ymm0, 16);
    ymm0 = _mm256_srli_epi32(ymm0, 9);
    ymm2 = _mm256_and_si256(ymm2, mask_g);
    ymm1 = _mm256_and_si256(ymm1, mask_b);
    ymm7 = _mm256_and_si256(ymm7, mask_a);
    ymm0 = _mm256_and_si256(ymm0, mask_r);
    ymm1 = _mm256_or_si256(ymm2, ymm1);
    ymm0 = _mm256_or_si256(ymm7, ymm0);
    ymm0 = _mm256_or_si256(ymm1, ymm0);
    ymm0 = _mm256_packus_epi32(ymm0, ymm0);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    _mm_storeu_si128((__m128i*)dst_argb1555, _mm256_castsi256_si128(ymm0));
    src_argb += 32;
    dst_argb1555 += 16;
    width -= 8;
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBTOARGB1555ROW_AVX2

#ifdef HAS_ARGBTOARGB4444ROW_AVX2
LIBYUV_TARGET_AVX2
void ARGBToARGB4444Row_AVX2(const uint8_t* src_argb,
                            uint8_t* dst_argb4444,
                            int width) {
  __m256i mask_b = _mm256_set1_epi32(0x0000000f);
  __m256i mask_g = _mm256_set1_epi32(0x000000f0);
  __m256i mask_r = _mm256_set1_epi32(0x00000f00);
  __m256i mask_a = _mm256_set1_epi32(0x0000f000);

  while (width > 0) {
    __m256i ymm0 = _mm256_loadu_si256((const __m256i*)src_argb);
    __m256i ymm2 = _mm256_srli_epi32(ymm0, 8);
    __m256i ymm1 = _mm256_srli_epi32(ymm0, 4);
    __m256i ymm7 = _mm256_srli_epi32(ymm0, 16);
    ymm0 = _mm256_srli_epi32(ymm0, 12);
    ymm2 = _mm256_and_si256(ymm2, mask_g);
    ymm1 = _mm256_and_si256(ymm1, mask_b);
    ymm7 = _mm256_and_si256(ymm7, mask_a);
    ymm0 = _mm256_and_si256(ymm0, mask_r);
    ymm1 = _mm256_or_si256(ymm2, ymm1);
    ymm0 = _mm256_or_si256(ymm7, ymm0);
    ymm0 = _mm256_or_si256(ymm1, ymm0);
    ymm0 = _mm256_packus_epi32(ymm0, ymm0);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    _mm_storeu_si128((__m128i*)dst_argb4444, _mm256_castsi256_si128(ymm0));
    src_argb += 32;
    dst_argb4444 += 16;
    width -= 8;
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBTOARGB4444ROW_AVX2

#ifdef HAS_RGB24TOARGBROW_AVX2
LIBYUV_TARGET_AVX2
void RGB24ToARGBRow_AVX2(const uint8_t* src_rgb24,
                         uint8_t* dst_argb,
                         int width) {
  __m256i ymm_alpha = _mm256_set1_epi32(0xff000000);
  __m256i ymm_shuf = _mm256_broadcastsi128_si256(
      _mm_loadu_si128((const __m128i*)kShuffleMaskRGB24ToARGB[0]));
  __m256i ymm_shuf2 = _mm256_broadcastsi128_si256(
      _mm_loadu_si128((const __m128i*)kShuffleMaskRGB24ToARGB[1]));

  while (width > 0) {
    __m128i xmm0 = _mm_loadu_si128((const __m128i*)src_rgb24);
    __m256i ymm0 = _mm256_castsi128_si256(xmm0);
    ymm0 = _mm256_inserti128_si256(
        ymm0, _mm_loadu_si128((const __m128i*)(src_rgb24 + 12)), 1);

    __m128i xmm1 = _mm_loadu_si128((const __m128i*)(src_rgb24 + 24));
    __m256i ymm1 = _mm256_castsi128_si256(xmm1);
    ymm1 = _mm256_inserti128_si256(
        ymm1, _mm_loadu_si128((const __m128i*)(src_rgb24 + 36)), 1);

    __m128i xmm2 = _mm_loadu_si128((const __m128i*)(src_rgb24 + 48));
    __m256i ymm2 = _mm256_castsi128_si256(xmm2);
    ymm2 = _mm256_inserti128_si256(
        ymm2, _mm_loadu_si128((const __m128i*)(src_rgb24 + 60)), 1);

    __m128i xmm3 = _mm_loadu_si128((const __m128i*)(src_rgb24 + 68));
    __m256i ymm3 = _mm256_castsi128_si256(xmm3);
    ymm3 = _mm256_inserti128_si256(
        ymm3, _mm_loadu_si128((const __m128i*)(src_rgb24 + 80)), 1);

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

    src_rgb24 += 96;
    dst_argb += 128;
    width -= 32;
  }
  _mm256_zeroupper();
}
#endif  // HAS_RGB24TOARGBROW_AVX2

#ifdef HAS_ARGBSHUFFLEROW_AVX2
LIBYUV_TARGET_AVX2
void ARGBShuffleRow_AVX2(const uint8_t* src_argb,
                         uint8_t* dst_argb,
                         const uint8_t* shuffler,
                         int width) {
  __m256i control =
      _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)shuffler));
  while (width >= 16) {
    __m256i row = _mm256_loadu_si256((const __m256i*)src_argb);
    __m256i row1 = _mm256_loadu_si256((const __m256i*)(src_argb + 32));
    row = _mm256_shuffle_epi8(row, control);
    row1 = _mm256_shuffle_epi8(row1, control);
    _mm256_storeu_si256((__m256i*)dst_argb, row);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), row1);
    src_argb += 64;
    dst_argb += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBSHUFFLEROW_AVX2

#ifdef HAS_ARGBSHUFFLEROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBShuffleRow_AVX512BW(const uint8_t* src_argb,
                             uint8_t* dst_argb,
                             const uint8_t* shuffler,
                             int width) {
  __m512i control =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)shuffler));
  while (width >= 32) {
    __m512i row = _mm512_loadu_si512((const __m512i*)src_argb);
    __m512i row1 = _mm512_loadu_si512((const __m512i*)(src_argb + 64));
    row = _mm512_shuffle_epi8(row, control);
    row1 = _mm512_shuffle_epi8(row1, control);
    _mm512_storeu_si512((__m512i*)dst_argb, row);
    _mm512_storeu_si512((__m512i*)(dst_argb + 64), row1);
    src_argb += 128;
    dst_argb += 128;
    width -= 32;
  }
  if (width > 0) {
    uint32_t mask = (1u << width) - 1u;
    __mmask16 mask0 = (__mmask16)mask;
    __mmask16 mask1 = (__mmask16)(mask >> 16);
    __m512i row = _mm512_maskz_loadu_epi32(mask0, src_argb);
    __m512i row1 = _mm512_maskz_loadu_epi32(mask1, src_argb + 64);
    row = _mm512_shuffle_epi8(row, control);
    row1 = _mm512_shuffle_epi8(row1, control);
    _mm512_mask_storeu_epi32(dst_argb, mask0, row);
    _mm512_mask_storeu_epi32(dst_argb + 64, mask1, row1);
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBSHUFFLEROW_AVX512BW

#ifdef HAS_I422TOARGBROW_AVX2
LIBYUV_TARGET_AVX2
void I422ToARGBRow_AVX2(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  __m256i ymm_kUVToB = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToB);
  __m256i ymm_kUVToG = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToG);
  __m256i ymm_kUVToR = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToR);
  __m256i ymm_kYToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYToRgb);
  __m256i ymm_kYBiasToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYBiasToRgb);
  __m256i ymm_128 = _mm256_set1_epi8((char)0x80);
  __m256i ymm_alpha = _mm256_set1_epi8((char)0xff);
  __m256i ymm_u_zero = _mm256_setzero_si256();

  ptrdiff_t offset = src_v - src_u;

  while (width >= 16) {
    __m128i xmm_u = _mm_loadl_epi64((const __m128i*)src_u);
    __m128i xmm_v = _mm_loadl_epi64((const __m128i*)(src_u + offset));
    src_u += 8;

    __m256i ymm3 = _mm256_insertf128_si256(ymm_u_zero, xmm_u, 0);
    __m256i ymm1 = _mm256_insertf128_si256(ymm_u_zero, xmm_v, 0);

    ymm3 = _mm256_unpacklo_epi8(ymm3, ymm1);
    ymm3 = _mm256_permute4x64_epi64(ymm3, 0xd8);
    ymm3 = _mm256_unpacklo_epi16(ymm3, ymm3);

    __m128i xmm_y = _mm_loadu_si128((const __m128i*)src_y);
    src_y += 16;
    __m256i ymm4 = _mm256_insertf128_si256(ymm_u_zero, xmm_y, 0);
    ymm4 = _mm256_permute4x64_epi64(ymm4, 0xd8);
    ymm4 = _mm256_unpacklo_epi8(ymm4, ymm4);

    ymm3 = _mm256_sub_epi8(ymm3, ymm_128);
    ymm4 = _mm256_mulhi_epu16(ymm4, ymm_kYToRgb);

    __m256i ymm0 = _mm256_maddubs_epi16(ymm_kUVToB, ymm3);
    ymm1 = _mm256_maddubs_epi16(ymm_kUVToG, ymm3);
    __m256i ymm2 = _mm256_maddubs_epi16(ymm_kUVToR, ymm3);

    ymm4 = _mm256_add_epi16(ymm4, ymm_kYBiasToRgb);

    ymm0 = _mm256_adds_epi16(ymm0, ymm4);
    ymm1 = _mm256_subs_epi16(ymm4, ymm1);
    ymm2 = _mm256_adds_epi16(ymm2, ymm4);

    ymm0 = _mm256_srai_epi16(ymm0, 6);
    ymm1 = _mm256_srai_epi16(ymm1, 6);
    ymm2 = _mm256_srai_epi16(ymm2, 6);

    ymm0 = _mm256_packus_epi16(ymm0, ymm0);
    ymm1 = _mm256_packus_epi16(ymm1, ymm1);
    ymm2 = _mm256_packus_epi16(ymm2, ymm2);

    ymm0 = _mm256_unpacklo_epi8(ymm0, ymm1);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    ymm2 = _mm256_unpacklo_epi8(ymm2, ymm_alpha);
    ymm2 = _mm256_permute4x64_epi64(ymm2, 0xd8);

    __m256i argb0 = _mm256_unpacklo_epi16(ymm0, ymm2);
    __m256i argb1 = _mm256_unpackhi_epi16(ymm0, ymm2);

    _mm256_storeu_si256((__m256i*)dst_argb, argb0);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), argb1);

    dst_argb += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_I422TOARGBROW_AVX2

#ifdef HAS_I422TOAR30ROW_AVX2
LIBYUV_TARGET_AVX2
void I422ToAR30Row_AVX2(const uint8_t* src_y,
                        const uint8_t* src_u,
                        const uint8_t* src_v,
                        uint8_t* dst_ar30,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  __m256i ymm_kUVToB = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToB);
  __m256i ymm_kUVToG = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToG);
  __m256i ymm_kUVToR = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToR);
  __m256i ymm_kYToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYToRgb);
  __m256i ymm_kYBiasToRgb = _mm256_sub_epi16(
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYBiasToRgb),
      _mm256_set1_epi16(24));
  __m256i ymm_128 = _mm256_set1_epi8((char)0x80);
  __m256i ymm_3ff0 = _mm256_set1_epi16((short)0x3ff0);
  __m256i ymm_c000 = _mm256_set1_epi16((short)0xc000);
  __m256i ymm_u_zero = _mm256_setzero_si256();

  ptrdiff_t offset = src_v - src_u;

  while (width >= 16) {
    __m128i xmm_u = _mm_loadl_epi64((const __m128i*)src_u);
    __m128i xmm_v = _mm_loadl_epi64((const __m128i*)(src_u + offset));
    src_u += 8;

    __m256i ymm3 = _mm256_insertf128_si256(ymm_u_zero, xmm_u, 0);
    __m256i ymm1 = _mm256_insertf128_si256(ymm_u_zero, xmm_v, 0);

    ymm3 = _mm256_unpacklo_epi8(ymm3, ymm1);
    ymm3 = _mm256_permute4x64_epi64(ymm3, 0xd8);
    ymm3 = _mm256_unpacklo_epi16(ymm3, ymm3);

    __m128i xmm_y = _mm_loadu_si128((const __m128i*)src_y);
    src_y += 16;
    __m256i ymm4 = _mm256_insertf128_si256(ymm_u_zero, xmm_y, 0);
    ymm4 = _mm256_permute4x64_epi64(ymm4, 0xd8);
    ymm4 = _mm256_unpacklo_epi8(ymm4, ymm4);

    ymm3 = _mm256_sub_epi8(ymm3, ymm_128);
    ymm4 = _mm256_mulhi_epu16(ymm4, ymm_kYToRgb);

    __m256i ymm0 = _mm256_maddubs_epi16(ymm_kUVToB, ymm3);
    ymm1 = _mm256_maddubs_epi16(ymm_kUVToG, ymm3);
    __m256i ymm2 = _mm256_maddubs_epi16(ymm_kUVToR, ymm3);

    ymm4 = _mm256_add_epi16(ymm4, ymm_kYBiasToRgb);

    ymm0 = _mm256_adds_epi16(ymm0, ymm4);
    ymm1 = _mm256_subs_epi16(ymm4, ymm1);
    ymm2 = _mm256_adds_epi16(ymm2, ymm4);

    ymm0 = _mm256_min_epi16(ymm0, ymm_3ff0);
    ymm1 = _mm256_min_epi16(ymm1, ymm_3ff0);
    ymm2 = _mm256_min_epi16(ymm2, ymm_3ff0);
    ymm0 = _mm256_max_epi16(ymm0, ymm_u_zero);
    ymm1 = _mm256_max_epi16(ymm1, ymm_u_zero);
    ymm2 = _mm256_max_epi16(ymm2, ymm_u_zero);

    __m256i ymm_g_masked = _mm256_and_si256(ymm1, ymm_3ff0);
    __m256i ymm_r_masked = _mm256_and_si256(ymm2, ymm_3ff0);
    ymm0 = _mm256_srli_epi16(ymm0, 4);
    ymm3 = _mm256_slli_epi16(ymm_g_masked, 6);
    ymm0 = _mm256_or_si256(ymm0, ymm3);
    ymm1 = _mm256_srli_epi16(ymm1, 10);
    ymm2 = _mm256_or_si256(ymm_r_masked, ymm_c000);
    ymm2 = _mm256_or_si256(ymm2, ymm1);

    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    ymm2 = _mm256_permute4x64_epi64(ymm2, 0xd8);

    __m256i ar30_lo = _mm256_unpacklo_epi16(ymm0, ymm2);
    __m256i ar30_hi = _mm256_unpackhi_epi16(ymm0, ymm2);

    _mm256_storeu_si256((__m256i*)dst_ar30, ar30_lo);
    _mm256_storeu_si256((__m256i*)(dst_ar30 + 32), ar30_hi);

    dst_ar30 += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_I422TOAR30ROW_AVX2

#ifdef HAS_NV12TOARGBROW_AVX2
LIBYUV_TARGET_AVX2
void NV12ToARGBRow_AVX2(const uint8_t* src_y,
                        const uint8_t* src_uv,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  __m256i ymm_kUVToB = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToB);
  __m256i ymm_kUVToG = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToG);
  __m256i ymm_kUVToR = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToR);
  __m256i ymm_kYToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYToRgb);
  __m256i ymm_kYBiasToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYBiasToRgb);
  __m256i ymm_128 = _mm256_set1_epi8((char)0x80);
  __m256i ymm_alpha = _mm256_set1_epi8((char)0xff);
  __m256i ymm_u_zero = _mm256_setzero_si256();

  while (width >= 16) {
    __m128i xmm_uv = _mm_loadu_si128((const __m128i*)src_uv);
    src_uv += 16;

    __m256i ymm3 = _mm256_insertf128_si256(ymm_u_zero, xmm_uv, 0);
    ymm3 = _mm256_permute4x64_epi64(ymm3, 0xd8);
    ymm3 = _mm256_unpacklo_epi16(ymm3, ymm3);

    __m128i xmm_y = _mm_loadu_si128((const __m128i*)src_y);
    src_y += 16;
    __m256i ymm4 = _mm256_insertf128_si256(ymm_u_zero, xmm_y, 0);
    ymm4 = _mm256_permute4x64_epi64(ymm4, 0xd8);
    ymm4 = _mm256_unpacklo_epi8(ymm4, ymm4);

    ymm3 = _mm256_sub_epi8(ymm3, ymm_128);
    ymm4 = _mm256_mulhi_epu16(ymm4, ymm_kYToRgb);

    __m256i ymm0 = _mm256_maddubs_epi16(ymm_kUVToB, ymm3);
    __m256i ymm1 = _mm256_maddubs_epi16(ymm_kUVToG, ymm3);
    __m256i ymm2 = _mm256_maddubs_epi16(ymm_kUVToR, ymm3);

    ymm4 = _mm256_add_epi16(ymm4, ymm_kYBiasToRgb);

    ymm0 = _mm256_adds_epi16(ymm0, ymm4);
    ymm1 = _mm256_subs_epi16(ymm4, ymm1);
    ymm2 = _mm256_adds_epi16(ymm2, ymm4);

    ymm0 = _mm256_srai_epi16(ymm0, 6);
    ymm1 = _mm256_srai_epi16(ymm1, 6);
    ymm2 = _mm256_srai_epi16(ymm2, 6);

    ymm0 = _mm256_packus_epi16(ymm0, ymm0);
    ymm1 = _mm256_packus_epi16(ymm1, ymm1);
    ymm2 = _mm256_packus_epi16(ymm2, ymm2);

    ymm0 = _mm256_unpacklo_epi8(ymm0, ymm1);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    ymm2 = _mm256_unpacklo_epi8(ymm2, ymm_alpha);
    ymm2 = _mm256_permute4x64_epi64(ymm2, 0xd8);

    __m256i argb0 = _mm256_unpacklo_epi16(ymm0, ymm2);
    __m256i argb1 = _mm256_unpackhi_epi16(ymm0, ymm2);

    _mm256_storeu_si256((__m256i*)dst_argb, argb0);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), argb1);

    dst_argb += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_NV12TOARGBROW_AVX2

#ifdef HAS_NV21TOARGBROW_AVX2
static const uint8_t kShuffleNV21_AVX2[32] = {
    1u, 0u, 3u, 2u, 5u, 4u, 7u, 6u, 9u, 8u, 11u, 10u, 13u, 12u, 15u, 14u,
    1u, 0u, 3u, 2u, 5u, 4u, 7u, 6u, 9u, 8u, 11u, 10u, 13u, 12u, 15u, 14u};

LIBYUV_TARGET_AVX2
void NV21ToARGBRow_AVX2(const uint8_t* src_y,
                        const uint8_t* src_vu,
                        uint8_t* dst_argb,
                        const struct YuvConstants* yuvconstants,
                        int width) {
  __m256i ymm_kUVToB = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToB);
  __m256i ymm_kUVToG = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToG);
  __m256i ymm_kUVToR = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToR);
  __m256i ymm_kYToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYToRgb);
  __m256i ymm_kYBiasToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYBiasToRgb);
  __m256i ymm_128 = _mm256_set1_epi8((char)0x80);
  __m256i ymm_alpha = _mm256_set1_epi8((char)0xff);
  __m256i ymm_shuf_nv21 = _mm256_loadu_si256((const __m256i*)kShuffleNV21_AVX2);
  __m256i ymm_u_zero = _mm256_setzero_si256();

  while (width >= 16) {
    __m128i xmm_vu = _mm_loadu_si128((const __m128i*)src_vu);
    src_vu += 16;

    __m256i ymm3 = _mm256_insertf128_si256(ymm_u_zero, xmm_vu, 0);
    ymm3 = _mm256_permute4x64_epi64(ymm3, 0xd8);
    ymm3 = _mm256_shuffle_epi8(ymm3, ymm_shuf_nv21);
    ymm3 = _mm256_unpacklo_epi16(ymm3, ymm3);

    __m128i xmm_y = _mm_loadu_si128((const __m128i*)src_y);
    src_y += 16;
    __m256i ymm4 = _mm256_insertf128_si256(ymm_u_zero, xmm_y, 0);
    ymm4 = _mm256_permute4x64_epi64(ymm4, 0xd8);
    ymm4 = _mm256_unpacklo_epi8(ymm4, ymm4);

    ymm3 = _mm256_sub_epi8(ymm3, ymm_128);
    ymm4 = _mm256_mulhi_epu16(ymm4, ymm_kYToRgb);

    __m256i ymm0 = _mm256_maddubs_epi16(ymm_kUVToB, ymm3);
    __m256i ymm1 = _mm256_maddubs_epi16(ymm_kUVToG, ymm3);
    __m256i ymm2 = _mm256_maddubs_epi16(ymm_kUVToR, ymm3);

    ymm4 = _mm256_add_epi16(ymm4, ymm_kYBiasToRgb);

    ymm0 = _mm256_adds_epi16(ymm0, ymm4);
    ymm1 = _mm256_subs_epi16(ymm4, ymm1);
    ymm2 = _mm256_adds_epi16(ymm2, ymm4);

    ymm0 = _mm256_srai_epi16(ymm0, 6);
    ymm1 = _mm256_srai_epi16(ymm1, 6);
    ymm2 = _mm256_srai_epi16(ymm2, 6);

    ymm0 = _mm256_packus_epi16(ymm0, ymm0);
    ymm1 = _mm256_packus_epi16(ymm1, ymm1);
    ymm2 = _mm256_packus_epi16(ymm2, ymm2);

    ymm0 = _mm256_unpacklo_epi8(ymm0, ymm1);
    ymm0 = _mm256_permute4x64_epi64(ymm0, 0xd8);
    ymm2 = _mm256_unpacklo_epi8(ymm2, ymm_alpha);
    ymm2 = _mm256_permute4x64_epi64(ymm2, 0xd8);

    __m256i argb0 = _mm256_unpacklo_epi16(ymm0, ymm2);
    __m256i argb1 = _mm256_unpackhi_epi16(ymm0, ymm2);

    _mm256_storeu_si256((__m256i*)dst_argb, argb0);
    _mm256_storeu_si256((__m256i*)(dst_argb + 32), argb1);

    dst_argb += 64;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // HAS_NV21TOARGBROW_AVX2

#if defined(HAS_I422TORGB24ROW_AVX2) || defined(HAS_I422TORGB24ROW_AVX512BW)
static const uint8_t kShuffleMaskARGBToRGB24[2][16] = {
    {0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u, 128u, 128u, 128u,
     128u},
    {0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 128u, 128u, 128u, 128u, 10u, 12u, 13u,
     14u}};

LIBYUV_TARGET_AVX2
void I422ToRGB24Row_AVX2(const uint8_t* src_y,
                         const uint8_t* src_u,
                         const uint8_t* src_v,
                         uint8_t* dst_rgb24,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  // Constants
  __m256i ymm_kUVToB = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToB);
  __m256i ymm_kUVToG = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToG);
  __m256i ymm_kUVToR = _mm256_loadu_si256((const __m256i*)yuvconstants->kUVToR);
  __m256i ymm_kYToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYToRgb);
  __m256i ymm_kYBiasToRgb =
      _mm256_loadu_si256((const __m256i*)yuvconstants->kYBiasToRgb);
  __m256i ymm_128 = _mm256_set1_epi8((char)0x80);

  __m256i ymm_shuf0 = _mm256_broadcastsi128_si256(
      _mm_loadu_si128((const __m128i*)kShuffleMaskARGBToRGB24[1]));
  __m256i ymm_shuf1 = _mm256_broadcastsi128_si256(
      _mm_loadu_si128((const __m128i*)kShuffleMaskARGBToRGB24[0]));
  __m256i ymm_u_zero = _mm256_setzero_si256();

  ptrdiff_t offset = src_v - src_u;

  while (width >= 16) {
    // READYUV422_AVX2
    __m128i xmm_u = _mm_loadl_epi64((const __m128i*)src_u);
    __m128i xmm_v = _mm_loadl_epi64((const __m128i*)(src_u + offset));
    src_u += 8;

    __m256i ymm3 = _mm256_insertf128_si256(ymm_u_zero, xmm_u, 0);
    __m256i ymm1 = _mm256_insertf128_si256(ymm_u_zero, xmm_v, 0);

    ymm3 = _mm256_unpacklo_epi8(ymm3, ymm1);
    ymm3 = _mm256_permute4x64_epi64(ymm3, 0xd8);
    ymm3 = _mm256_unpacklo_epi16(ymm3, ymm3);

    __m128i xmm_y = _mm_loadu_si128((const __m128i*)src_y);
    src_y += 16;
    __m256i ymm4 = _mm256_insertf128_si256(ymm_u_zero, xmm_y, 0);
    ymm4 = _mm256_permute4x64_epi64(ymm4, 0xd8);
    ymm4 = _mm256_unpacklo_epi8(ymm4, ymm4);

    // YUVTORGB_AVX2
    ymm3 = _mm256_sub_epi8(ymm3, ymm_128);
    ymm4 = _mm256_mulhi_epu16(ymm4, ymm_kYToRgb);

    __m256i ymm0 = _mm256_maddubs_epi16(ymm_kUVToB, ymm3);
    ymm1 = _mm256_maddubs_epi16(ymm_kUVToG, ymm3);
    __m256i ymm2 = _mm256_maddubs_epi16(ymm_kUVToR, ymm3);

    ymm4 = _mm256_add_epi16(ymm4, ymm_kYBiasToRgb);

    ymm0 = _mm256_adds_epi16(ymm0, ymm4);
    ymm1 = _mm256_subs_epi16(ymm4, ymm1);
    ymm2 = _mm256_adds_epi16(ymm2, ymm4);

    ymm0 = _mm256_srai_epi16(ymm0, 6);
    ymm1 = _mm256_srai_epi16(ymm1, 6);
    ymm2 = _mm256_srai_epi16(ymm2, 6);

    ymm0 = _mm256_packus_epi16(ymm0, ymm0);
    ymm1 = _mm256_packus_epi16(ymm1, ymm1);
    ymm2 = _mm256_packus_epi16(ymm2, ymm2);

    // STORERGB24_AVX2
    __m256i ymm0_packed = _mm256_unpacklo_epi8(ymm0, ymm1);
    __m256i ymm2_packed = _mm256_unpacklo_epi8(ymm2, ymm2);
    __m256i ymm1_packed = ymm0_packed;

    ymm0_packed = _mm256_unpacklo_epi16(ymm0_packed, ymm2_packed);
    ymm1_packed = _mm256_unpackhi_epi16(ymm1_packed, ymm2_packed);

    ymm0_packed = _mm256_shuffle_epi8(ymm0_packed, ymm_shuf0);
    ymm1_packed = _mm256_shuffle_epi8(ymm1_packed, ymm_shuf1);

    ymm1_packed = _mm256_alignr_epi8(ymm1_packed, ymm0_packed, 0xc);

    __m128i xmm0_store = _mm256_castsi256_si128(ymm0_packed);
    __m128i xmm1_store = _mm256_castsi256_si128(ymm1_packed);
    __m128i xmm2_store = _mm256_extractf128_si256(ymm0_packed, 1);
    __m128i xmm3_store = _mm256_extractf128_si256(ymm1_packed, 1);

    xmm0_store = _mm_unpacklo_epi64(xmm0_store, xmm1_store);
    xmm2_store = _mm_alignr_epi8(xmm2_store, xmm1_store, 8);

    _mm_storeu_si128((__m128i*)dst_rgb24, xmm0_store);
    _mm_storeu_si128((__m128i*)(dst_rgb24 + 16), xmm2_store);
    _mm_storeu_si128((__m128i*)(dst_rgb24 + 32), xmm3_store);

    dst_rgb24 += 48;
    width -= 16;
  }
  _mm256_zeroupper();
}
#endif  // defined(HAS_I422TORGB24ROW_AVX2) ||
        // defined(HAS_I422TORGB24ROW_AVX512BW)

#if defined(HAS_I422TORGB24ROW_AVX512VBMI) || \
    defined(HAS_I422TORGB24ROW_AVX512BW)
#define READYUV422_AVX512BW                                          \
  __m128i xmm_u = _mm_loadu_si128((const __m128i*)src_u);            \
  __m128i xmm_v = _mm_loadu_si128((const __m128i*)(src_u + offset)); \
  src_u += 16;                                                       \
  __m512i zmm_u_val = _mm512_castsi128_si512(xmm_u);                 \
  __m512i zmm_v_val = _mm512_castsi128_si512(xmm_v);                 \
  zmm_u_val = _mm512_permutexvar_epi64(zmm_split, zmm_u_val);        \
  zmm_v_val = _mm512_permutexvar_epi64(zmm_split, zmm_v_val);        \
  __m512i zmm3 = _mm512_unpacklo_epi8(zmm_u_val, zmm_v_val);         \
  zmm3 = _mm512_permutex_epi64(zmm3, 0xd8);                          \
  zmm3 = _mm512_unpacklo_epi16(zmm3, zmm3);                          \
  __m256i ymm_y = _mm256_loadu_si256((const __m256i*)src_y);         \
  src_y += 32;                                                       \
  __m512i zmm4 = _mm512_castsi256_si512(ymm_y);                      \
  zmm4 = _mm512_permutexvar_epi64(zmm_split_y, zmm4);                \
  zmm4 = _mm512_permutex_epi64(zmm4, 0xd8);                          \
  zmm4 = _mm512_unpacklo_epi8(zmm4, zmm4);

#define READYUV422_MASK_AVX512BW                                    \
  __mmask16 mask_uv = (__mmask16)((1u << ((width + 1) >> 1)) - 1u); \
  __mmask32 mask_y = (__mmask32)((1u << width) - 1u);               \
  __m128i xmm_u = _mm_maskz_loadu_epi8(mask_uv, src_u);             \
  __m128i xmm_v = _mm_maskz_loadu_epi8(mask_uv, src_u + offset);    \
  __m512i zmm_u_val = _mm512_castsi128_si512(xmm_u);                \
  __m512i zmm_v_val = _mm512_castsi128_si512(xmm_v);                \
  zmm_u_val = _mm512_permutexvar_epi64(zmm_split, zmm_u_val);       \
  zmm_v_val = _mm512_permutexvar_epi64(zmm_split, zmm_v_val);       \
  __m512i zmm3 = _mm512_unpacklo_epi8(zmm_u_val, zmm_v_val);        \
  zmm3 = _mm512_permutex_epi64(zmm3, 0xd8);                         \
  zmm3 = _mm512_unpacklo_epi16(zmm3, zmm3);                         \
  __m256i ymm_y = _mm256_maskz_loadu_epi8(mask_y, src_y);           \
  __m512i zmm4 = _mm512_castsi256_si512(ymm_y);                     \
  zmm4 = _mm512_permutexvar_epi64(zmm_split_y, zmm4);               \
  zmm4 = _mm512_permutex_epi64(zmm4, 0xd8);                         \
  zmm4 = _mm512_unpacklo_epi8(zmm4, zmm4);

#define YUVTORGB_AVX512BW                                \
  zmm3 = _mm512_sub_epi8(zmm3, zmm_128);                 \
  zmm4 = _mm512_mulhi_epu16(zmm4, zmm_kYToRgb);          \
  __m512i zmm0 = _mm512_maddubs_epi16(zmm_kUVToB, zmm3); \
  __m512i zmm1 = _mm512_maddubs_epi16(zmm_kUVToG, zmm3); \
  __m512i zmm2 = _mm512_maddubs_epi16(zmm_kUVToR, zmm3); \
  zmm4 = _mm512_add_epi16(zmm4, zmm_kYBiasToRgb);        \
  zmm0 = _mm512_adds_epi16(zmm0, zmm4);                  \
  zmm1 = _mm512_subs_epi16(zmm4, zmm1);                  \
  zmm2 = _mm512_adds_epi16(zmm2, zmm4);                  \
  zmm0 = _mm512_srai_epi16(zmm0, 6);                     \
  zmm1 = _mm512_srai_epi16(zmm1, 6);                     \
  zmm2 = _mm512_srai_epi16(zmm2, 6);                     \
  zmm0 = _mm512_packus_epi16(zmm0, zmm0);                \
  zmm1 = _mm512_packus_epi16(zmm1, zmm1);                \
  zmm2 = _mm512_packus_epi16(zmm2, zmm2);

#define STORERGB24_AVX512VBMI                                               \
  __m512i zmm_BG = _mm512_permutex2var_epi8(zmm0, zmm_mask_BG, zmm1);       \
  __m512i zmm_dst0 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST0, zmm2); \
  __m512i zmm_dst1 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST1, zmm2); \
  _mm512_storeu_si512((__m512i*)dst_rgb24, zmm_dst0);                       \
  _mm256_storeu_si256((__m256i*)(dst_rgb24 + 64),                           \
                      _mm512_castsi512_si256(zmm_dst1));                    \
  dst_rgb24 += 96;

#define STORERGB24_MASK_AVX512VBMI                                            \
  int dst_bytes = width * 3;                                                  \
  __mmask64 mask_dst0 = dst_bytes >= 64 ? ~0ull : (1ull << dst_bytes) - 1ull; \
  __mmask32 mask_dst1 = dst_bytes > 64 ? (1u << (dst_bytes - 64)) - 1u : 0u;  \
  __m512i zmm_BG = _mm512_permutex2var_epi8(zmm0, zmm_mask_BG, zmm1);         \
  __m512i zmm_dst0 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST0, zmm2);   \
  __m512i zmm_dst1 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST1, zmm2);   \
  _mm512_mask_storeu_epi8(dst_rgb24, mask_dst0, zmm_dst0);                    \
  _mm256_mask_storeu_epi8(dst_rgb24 + 64, mask_dst1,                          \
                          _mm512_castsi512_si256(zmm_dst1));

#define STORERGB24_AVX512BW                                                  \
  __m512i zmm_bg = _mm512_unpacklo_epi8(zmm0, zmm1);                         \
  __m512i zmm_rr = _mm512_unpacklo_epi8(zmm2, zmm2);                         \
  __m512i zmm_lo = _mm512_unpacklo_epi16(zmm_bg, zmm_rr);                    \
  __m512i zmm_hi = _mm512_unpackhi_epi16(zmm_bg, zmm_rr);                    \
  zmm_lo = _mm512_shuffle_epi8(zmm_lo, zmm_shuf0);                           \
  zmm_hi = _mm512_shuffle_epi8(zmm_hi, zmm_shuf1);                           \
  zmm_hi = _mm512_alignr_epi8(zmm_hi, zmm_lo, 12);                           \
  __m512i zmm_dst0 = _mm512_permutex2var_epi64(zmm_lo, zmm_stitch0, zmm_hi); \
  __m512i zmm_dst1 = _mm512_permutex2var_epi64(zmm_lo, zmm_stitch1, zmm_hi); \
  _mm512_storeu_si512((__m512i*)dst_rgb24, zmm_dst0);                        \
  _mm256_storeu_si256((__m256i*)(dst_rgb24 + 64),                            \
                      _mm512_castsi512_si256(zmm_dst1));                     \
  dst_rgb24 += 96;

#define STORERGB24_MASK_AVX512BW                                              \
  int dst_bytes = width * 3;                                                  \
  __mmask64 mask_dst0 = dst_bytes >= 64 ? ~0ull : (1ull << dst_bytes) - 1ull; \
  __mmask32 mask_dst1 = dst_bytes > 64 ? (1u << (dst_bytes - 64)) - 1u : 0u;  \
  __m512i zmm_bg = _mm512_unpacklo_epi8(zmm0, zmm1);                          \
  __m512i zmm_rr = _mm512_unpacklo_epi8(zmm2, zmm2);                          \
  __m512i zmm_lo = _mm512_unpacklo_epi16(zmm_bg, zmm_rr);                     \
  __m512i zmm_hi = _mm512_unpackhi_epi16(zmm_bg, zmm_rr);                     \
  zmm_lo = _mm512_shuffle_epi8(zmm_lo, zmm_shuf0);                            \
  zmm_hi = _mm512_shuffle_epi8(zmm_hi, zmm_shuf1);                            \
  zmm_hi = _mm512_alignr_epi8(zmm_hi, zmm_lo, 12);                            \
  __m512i zmm_dst0 = _mm512_permutex2var_epi64(zmm_lo, zmm_stitch0, zmm_hi);  \
  __m512i zmm_dst1 = _mm512_permutex2var_epi64(zmm_lo, zmm_stitch1, zmm_hi);  \
  _mm512_mask_storeu_epi8(dst_rgb24, mask_dst0, zmm_dst0);                    \
  _mm256_mask_storeu_epi8(dst_rgb24 + 64, mask_dst1,                          \
                          _mm512_castsi512_si256(zmm_dst1));
#endif  // defined(HAS_I422TORGB24ROW_AVX512VBMI) ||
        // defined(HAS_I422TORGB24ROW_AVX512BW)

#ifdef HAS_I422TORGB24ROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void I422ToRGB24Row_AVX512VBMI(const uint8_t* src_y,
                               const uint8_t* src_u,
                               const uint8_t* src_v,
                               uint8_t* dst_rgb24,
                               const struct YuvConstants* yuvconstants,
                               int width) {
  // Masks
  static const uint8_t kMaskBG[64] = {
      0x00, 0x40, 0x01, 0x41, 0x02, 0x42, 0x03, 0x43, 0x04, 0x44, 0x05,
      0x45, 0x06, 0x46, 0x07, 0x47, 0x10, 0x50, 0x11, 0x51, 0x12, 0x52,
      0x13, 0x53, 0x14, 0x54, 0x15, 0x55, 0x16, 0x56, 0x17, 0x57, 0x20,
      0x60, 0x21, 0x61, 0x22, 0x62, 0x23, 0x63, 0x24, 0x64, 0x25, 0x65,
      0x26, 0x66, 0x27, 0x67, 0x30, 0x70, 0x31, 0x71, 0x32, 0x72, 0x33,
      0x73, 0x34, 0x74, 0x35, 0x75, 0x36, 0x76, 0x37, 0x77};
  static const uint8_t kMaskDST0[64] = {
      0x00, 0x01, 0x40, 0x02, 0x03, 0x41, 0x04, 0x05, 0x42, 0x06, 0x07,
      0x43, 0x08, 0x09, 0x44, 0x0a, 0x0b, 0x45, 0x0c, 0x0d, 0x46, 0x0e,
      0x0f, 0x47, 0x10, 0x11, 0x50, 0x12, 0x13, 0x51, 0x14, 0x15, 0x52,
      0x16, 0x17, 0x53, 0x18, 0x19, 0x54, 0x1a, 0x1b, 0x55, 0x1c, 0x1d,
      0x56, 0x1e, 0x1f, 0x57, 0x20, 0x21, 0x60, 0x22, 0x23, 0x61, 0x24,
      0x25, 0x62, 0x26, 0x27, 0x63, 0x28, 0x29, 0x64, 0x2a};
  static const uint8_t kMaskDST1[64] = {
      0x2b, 0x65, 0x2c, 0x2d, 0x66, 0x2e, 0x2f, 0x67, 0x30, 0x31, 0x70,
      0x32, 0x33, 0x71, 0x34, 0x35, 0x72, 0x36, 0x37, 0x73, 0x38, 0x39,
      0x74, 0x3a, 0x3b, 0x75, 0x3c, 0x3d, 0x76, 0x3e, 0x3f, 0x77, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  static const uint64_t kSplitQuadWords[8] = {0, 2, 2, 2, 1, 2, 2, 2};
  static const uint64_t kSplitDoubleQuadWords[8] = {0, 1, 4, 4, 2, 3, 4, 4};

  // Constants
  __m512i zmm_kUVToB = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToB));
  __m512i zmm_kUVToG = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToG));
  __m512i zmm_kUVToR = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToR));
  __m512i zmm_kYToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYToRgb));
  __m512i zmm_kYBiasToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYBiasToRgb));
  __m512i zmm_128 = _mm512_set1_epi8((char)0x80);

  __m512i zmm_mask_BG = _mm512_loadu_si512((const __m512i*)kMaskBG);
  __m512i zmm_mask_DST0 = _mm512_loadu_si512((const __m512i*)kMaskDST0);
  __m512i zmm_mask_DST1 = _mm512_loadu_si512((const __m512i*)kMaskDST1);
  __m512i zmm_split = _mm512_loadu_si512((const __m512i*)kSplitQuadWords);
  __m512i zmm_split_y =
      _mm512_loadu_si512((const __m512i*)kSplitDoubleQuadWords);

  ptrdiff_t offset = src_v - src_u;

  while (width >= 32) {
    READYUV422_AVX512BW
    YUVTORGB_AVX512BW
    STORERGB24_AVX512VBMI
    width -= 32;
  }
  if (width > 0) {
    READYUV422_MASK_AVX512BW
    YUVTORGB_AVX512BW
    STORERGB24_MASK_AVX512VBMI
  }
  _mm256_zeroupper();
}
#endif  // HAS_I422TORGB24ROW_AVX512VBMI

#ifdef HAS_I422TORGB24ROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void I422ToRGB24Row_AVX512BW(const uint8_t* src_y,
                             const uint8_t* src_u,
                             const uint8_t* src_v,
                             uint8_t* dst_rgb24,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  static const uint64_t kStitchRGB24_0[8] = {0, 8, 9, 2, 10, 11, 4, 12};
  static const uint64_t kStitchRGB24_1[8] = {13, 6, 14, 15, 0, 0, 0, 0};
  static const uint64_t kSplitQuadWords[8] = {0, 2, 2, 2, 1, 2, 2, 2};
  static const uint64_t kSplitDoubleQuadWords[8] = {0, 1, 4, 4, 2, 3, 4, 4};

  // Constants
  __m512i zmm_kUVToB = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToB));
  __m512i zmm_kUVToG = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToG));
  __m512i zmm_kUVToR = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToR));
  __m512i zmm_kYToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYToRgb));
  __m512i zmm_kYBiasToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYBiasToRgb));
  __m512i zmm_128 = _mm512_set1_epi8((char)0x80);

  __m512i zmm_shuf0 = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)kShuffleMaskARGBToRGB24[1]));
  __m512i zmm_shuf1 = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)kShuffleMaskARGBToRGB24[0]));
  __m512i zmm_stitch0 = _mm512_loadu_si512((const __m512i*)kStitchRGB24_0);
  __m512i zmm_stitch1 = _mm512_loadu_si512((const __m512i*)kStitchRGB24_1);
  __m512i zmm_split = _mm512_loadu_si512((const __m512i*)kSplitQuadWords);
  __m512i zmm_split_y =
      _mm512_loadu_si512((const __m512i*)kSplitDoubleQuadWords);

  ptrdiff_t offset = src_v - src_u;

  while (width >= 32) {
    READYUV422_AVX512BW
    YUVTORGB_AVX512BW
    STORERGB24_AVX512BW
    width -= 32;
  }
  if (width > 0) {
    READYUV422_MASK_AVX512BW
    YUVTORGB_AVX512BW
    STORERGB24_MASK_AVX512BW
  }
  _mm256_zeroupper();
}
#endif  // HAS_I422TORGB24ROW_AVX512BW

#if defined(HAS_I422TORGB24ROW_AVX512VBMI) || \
    defined(HAS_I422TORGB24ROW_AVX512BW)
#undef READYUV422_AVX512BW
#undef READYUV422_MASK_AVX512BW
#undef YUVTORGB_AVX512BW
#undef STORERGB24_AVX512VBMI
#undef STORERGB24_MASK_AVX512VBMI
#undef STORERGB24_AVX512BW
#undef STORERGB24_MASK_AVX512BW
#endif

#ifdef HAS_I422TOAR30ROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void I422ToAR30Row_AVX512BW(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_ar30,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  static const uint64_t kSplitQuadWords[8] = {0, 2, 2, 2, 1, 2, 2, 2};
  static const uint64_t kSplitDoubleQuadWords[8] = {0, 1, 4, 4, 2, 3, 4, 4};
  static const uint16_t kPermAR30_0[32] = {
      0, 32, 1, 33, 2,  34, 3,  35, 4,  36, 5,  37, 6,  38, 7,  39,
      8, 40, 9, 41, 10, 42, 11, 43, 12, 44, 13, 45, 14, 46, 15, 47};
  static const uint16_t kPermAR30_1[32] = {
      16, 48, 17, 49, 18, 50, 19, 51, 20, 52, 21, 53, 22, 54, 23, 55,
      24, 56, 25, 57, 26, 58, 27, 59, 28, 60, 29, 61, 30, 62, 31, 63};

  // Constants
  __m512i zmm_kUVToB = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToB));
  __m512i zmm_kUVToG = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToG));
  __m512i zmm_kUVToR = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToR));
  __m512i zmm_kYToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYToRgb));
  __m512i zmm_kYBiasToRgb = _mm512_sub_epi16(
      _mm512_broadcast_i32x4(
          _mm_loadu_si128((const __m128i*)yuvconstants->kYBiasToRgb)),
      _mm512_set1_epi16(24));
  __m512i zmm_128 = _mm512_set1_epi8((char)0x80);
  __m512i zmm_3ff0 = _mm512_set1_epi16((short)0x3ff0);
  __m512i zmm_fc00 = _mm512_set1_epi16((short)0xfc00);
  __m512i zmm_c000 = _mm512_set1_epi16((short)0xc000);
  __m512i zmm_zero = _mm512_setzero_si512();

  __m512i zmm_perm0 = _mm512_loadu_si512((const __m512i*)kPermAR30_0);
  __m512i zmm_perm1 = _mm512_loadu_si512((const __m512i*)kPermAR30_1);
  __m512i zmm_split = _mm512_loadu_si512((const __m512i*)kSplitQuadWords);
  __m512i zmm_split_y =
      _mm512_loadu_si512((const __m512i*)kSplitDoubleQuadWords);

  ptrdiff_t offset = src_v - src_u;

  while (width >= 32) {
    // READYUV422_AVX512BW
    __m128i xmm_u = _mm_loadu_si128((const __m128i*)src_u);
    __m128i xmm_v = _mm_loadu_si128((const __m128i*)(src_u + offset));
    src_u += 16;

    __m512i zmm_u_val = _mm512_castsi128_si512(xmm_u);
    __m512i zmm_v_val = _mm512_castsi128_si512(xmm_v);

    zmm_u_val = _mm512_permutexvar_epi64(zmm_split, zmm_u_val);
    zmm_v_val = _mm512_permutexvar_epi64(zmm_split, zmm_v_val);

    __m512i zmm3 = _mm512_unpacklo_epi8(zmm_u_val, zmm_v_val);
    zmm3 = _mm512_permutex_epi64(zmm3, 0xd8);
    zmm3 = _mm512_unpacklo_epi16(zmm3, zmm3);

    __m256i ymm_y = _mm256_loadu_si256((const __m256i*)src_y);
    src_y += 32;
    __m512i zmm4 = _mm512_castsi256_si512(ymm_y);
    zmm4 = _mm512_permutexvar_epi64(zmm_split_y, zmm4);
    zmm4 = _mm512_permutex_epi64(zmm4, 0xd8);
    zmm4 = _mm512_unpacklo_epi8(zmm4, zmm4);

    // YUVTORGB_AVX512BW
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_mulhi_epu16(zmm4, zmm_kYToRgb);

    __m512i zmm0 = _mm512_maddubs_epi16(zmm_kUVToB, zmm3);
    __m512i zmm1 = _mm512_maddubs_epi16(zmm_kUVToG, zmm3);
    __m512i zmm2 = _mm512_maddubs_epi16(zmm_kUVToR, zmm3);

    zmm4 = _mm512_add_epi16(zmm4, zmm_kYBiasToRgb);

    zmm0 = _mm512_adds_epi16(zmm0, zmm4);
    zmm1 = _mm512_subs_epi16(zmm4, zmm1);
    zmm2 = _mm512_adds_epi16(zmm2, zmm4);

    // Clamp to [0, 0x3ff0] (1023 * 16)
    zmm0 = _mm512_min_epi16(zmm0, zmm_3ff0);
    zmm1 = _mm512_min_epi16(zmm1, zmm_3ff0);
    zmm2 = _mm512_min_epi16(zmm2, zmm_3ff0);
    zmm0 = _mm512_max_epi16(zmm0, zmm_zero);
    zmm1 = _mm512_max_epi16(zmm1, zmm_zero);
    zmm2 = _mm512_max_epi16(zmm2, zmm_zero);

    // STOREAR30_AVX512BW:
    zmm0 = _mm512_srli_epi16(zmm0, 4);
    zmm3 = _mm512_srli_epi16(zmm1, 10);
    zmm1 = _mm512_slli_epi16(zmm1, 6);
    zmm0 = _mm512_ternarylogic_epi32(zmm0, zmm1, zmm_fc00, 0xf8);
    zmm2 = _mm512_ternarylogic_epi32(zmm2, zmm3, zmm_3ff0, 0xe4);
    zmm2 = _mm512_or_si512(zmm2, zmm_c000);

    __m512i zmm_dst0 = _mm512_permutex2var_epi16(zmm0, zmm_perm0, zmm2);
    __m512i zmm_dst1 = _mm512_permutex2var_epi16(zmm0, zmm_perm1, zmm2);

    _mm512_storeu_si512((__m512i*)dst_ar30, zmm_dst0);
    _mm512_storeu_si512((__m512i*)(dst_ar30 + 64), zmm_dst1);

    dst_ar30 += 128;
    width -= 32;
  }
  if (width > 0) {
    __mmask16 mask_uv = (__mmask16)((1u << ((width + 1) >> 1)) - 1u);
    __mmask32 mask_y = (__mmask32)((1u << width) - 1u);
    __m128i xmm_u = _mm_maskz_loadu_epi8(mask_uv, src_u);
    __m128i xmm_v = _mm_maskz_loadu_epi8(mask_uv, src_u + offset);

    __m512i zmm_u_val = _mm512_castsi128_si512(xmm_u);
    __m512i zmm_v_val = _mm512_castsi128_si512(xmm_v);

    zmm_u_val = _mm512_permutexvar_epi64(zmm_split, zmm_u_val);
    zmm_v_val = _mm512_permutexvar_epi64(zmm_split, zmm_v_val);

    __m512i zmm3 = _mm512_unpacklo_epi8(zmm_u_val, zmm_v_val);
    zmm3 = _mm512_permutex_epi64(zmm3, 0xd8);
    zmm3 = _mm512_unpacklo_epi16(zmm3, zmm3);

    __m256i ymm_y = _mm256_maskz_loadu_epi8(mask_y, src_y);
    __m512i zmm4 = _mm512_castsi256_si512(ymm_y);
    zmm4 = _mm512_permutexvar_epi64(zmm_split_y, zmm4);
    zmm4 = _mm512_permutex_epi64(zmm4, 0xd8);
    zmm4 = _mm512_unpacklo_epi8(zmm4, zmm4);

    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_mulhi_epu16(zmm4, zmm_kYToRgb);

    __m512i zmm0 = _mm512_maddubs_epi16(zmm_kUVToB, zmm3);
    __m512i zmm1 = _mm512_maddubs_epi16(zmm_kUVToG, zmm3);
    __m512i zmm2 = _mm512_maddubs_epi16(zmm_kUVToR, zmm3);

    zmm4 = _mm512_add_epi16(zmm4, zmm_kYBiasToRgb);

    zmm0 = _mm512_adds_epi16(zmm0, zmm4);
    zmm1 = _mm512_subs_epi16(zmm4, zmm1);
    zmm2 = _mm512_adds_epi16(zmm2, zmm4);

    zmm0 = _mm512_min_epi16(zmm0, zmm_3ff0);
    zmm1 = _mm512_min_epi16(zmm1, zmm_3ff0);
    zmm2 = _mm512_min_epi16(zmm2, zmm_3ff0);
    zmm0 = _mm512_max_epi16(zmm0, zmm_zero);
    zmm1 = _mm512_max_epi16(zmm1, zmm_zero);
    zmm2 = _mm512_max_epi16(zmm2, zmm_zero);

    zmm0 = _mm512_srli_epi16(zmm0, 4);
    zmm3 = _mm512_srli_epi16(zmm1, 10);
    zmm1 = _mm512_slli_epi16(zmm1, 6);
    zmm0 = _mm512_ternarylogic_epi32(zmm0, zmm1, zmm_fc00, 0xf8);
    zmm2 = _mm512_ternarylogic_epi32(zmm2, zmm3, zmm_3ff0, 0xe4);
    zmm2 = _mm512_or_si512(zmm2, zmm_c000);

    __m512i zmm_dst0 = _mm512_permutex2var_epi16(zmm0, zmm_perm0, zmm2);
    __m512i zmm_dst1 = _mm512_permutex2var_epi16(zmm0, zmm_perm1, zmm2);

    _mm512_mask_storeu_epi32(dst_ar30, (__mmask16)mask_y, zmm_dst0);
    _mm512_mask_storeu_epi32(dst_ar30 + 64, (__mmask16)(mask_y >> 16),
                             zmm_dst1);
  }
  _mm256_zeroupper();
}
#endif  // HAS_I422TOAR30ROW_AVX512BW

#ifdef HAS_ARGBBLENDROW_SSSE3
static const uint8_t kShuffleAlpha[16] = {3u,  0x80, 3u,  0x80, 7u,  0x80,
                                          7u,  0x80, 11u, 0x80, 11u, 0x80,
                                          15u, 0x80, 15u, 0x80};

void ARGBBlendRow_SSSE3(const uint8_t* src_argb,
                        const uint8_t* src_argb1,
                        uint8_t* dst_argb,
                        int width) {
  __m128i shuff = _mm_loadu_si128((const __m128i*)kShuffleAlpha);
  __m128i xmm_1 = _mm_set1_epi16(1);
  __m128i mask_lo = _mm_set1_epi16(0x00ff);
  __m128i mask_hi = _mm_set1_epi16((short)0xff00);
  __m128i mask_alpha = _mm_set1_epi32((int)0xff000000);

  while (width >= 4) {
    __m128i xmm_s0 = _mm_loadu_si128((const __m128i*)src_argb);
    __m128i xmm_s1 = _mm_loadu_si128((const __m128i*)src_argb1);
    src_argb += 16;
    src_argb1 += 16;

    __m128i xmm_a = _mm_xor_si128(xmm_s0, mask_alpha);
    xmm_a = _mm_shuffle_epi8(xmm_a, shuff);
    xmm_a = _mm_add_epi16(xmm_a, xmm_1);

    __m128i xmm_s1_lo = _mm_and_si128(xmm_s1, mask_lo);
    __m128i xmm_s1_hi = _mm_srli_epi16(xmm_s1, 8);

    xmm_s1_lo = _mm_mullo_epi16(xmm_s1_lo, xmm_a);
    xmm_s1_hi = _mm_mullo_epi16(xmm_s1_hi, xmm_a);

    xmm_s1_lo = _mm_srli_epi16(xmm_s1_lo, 8);
    xmm_s1_hi = _mm_and_si128(xmm_s1_hi, mask_hi);

    __m128i res = _mm_or_si128(xmm_s0, mask_alpha);
    res = _mm_adds_epu8(res, xmm_s1_lo);
    res = _mm_adds_epu8(res, xmm_s1_hi);

    _mm_storeu_si128((__m128i*)dst_argb, res);
    dst_argb += 16;
    width -= 4;
  }

  while (width > 0) {
    uint32_t s0 = *(const uint32_t*)src_argb;
    uint32_t s1 = *(const uint32_t*)src_argb1;
    src_argb += 4;
    src_argb1 += 4;

    uint32_t a = 256 - (s0 >> 24);
    uint32_t b = (((s1 & 0x000000ff) * a) >> 8) + (s0 & 0x000000ff);
    uint32_t g = (((s1 & 0x0000ff00) * a) >> 8) + (s0 & 0x0000ff00);
    uint32_t r = (((s1 & 0x00ff0000) >> 8) * a) + (s0 & 0x00ff0000);

    b = (b > 255) ? 255 : b;
    g = (g > 0xff00) ? 0xff00 : (g & 0xff00);
    r = (r > 0x00ff0000) ? 0x00ff0000 : (r & 0x00ff0000);

    *(uint32_t*)dst_argb = 0xff000000 | r | g | b;
    dst_argb += 4;
    --width;
  }
}
#endif  // HAS_ARGBBLENDROW_SSSE3

#ifdef HAS_BLENDPLANEROW_SSSE3
void BlendPlaneRow_SSSE3(const uint8_t* src0,
                         const uint8_t* src1,
                         const uint8_t* alpha,
                         uint8_t* dst,
                         int width) {
  __m128i xmm_ff00 = _mm_set1_epi16((short)0xff00);
  __m128i xmm_80 = _mm_set1_epi8((char)0x80);
  __m128i xmm_round = _mm_set1_epi16((short)0x807f);

  while (width > 0) {
    __m128i xmm_a = _mm_loadl_epi64((const __m128i*)alpha);
    __m128i xmm_s0 = _mm_loadl_epi64((const __m128i*)src0);
    __m128i xmm_s1 = _mm_loadl_epi64((const __m128i*)src1);
    alpha += 8;
    src0 += 8;
    src1 += 8;

    xmm_a = _mm_unpacklo_epi8(xmm_a, xmm_a);
    xmm_a = _mm_xor_si128(xmm_a, xmm_ff00);

    __m128i xmm_s = _mm_unpacklo_epi8(xmm_s0, xmm_s1);
    xmm_s = _mm_sub_epi8(xmm_s, xmm_80);

    __m128i xmm_p = _mm_maddubs_epi16(xmm_a, xmm_s);
    xmm_p = _mm_add_epi16(xmm_p, xmm_round);
    xmm_p = _mm_srli_epi16(xmm_p, 8);

    __m128i xmm_d = _mm_packus_epi16(xmm_p, xmm_p);
    _mm_storel_epi64((__m128i*)dst, xmm_d);
    dst += 8;
    width -= 8;
  }
}
#endif  // HAS_BLENDPLANEROW_SSSE3

#ifdef HAS_BLENDPLANEROW_AVX2
LIBYUV_TARGET_AVX2
void BlendPlaneRow_AVX2(const uint8_t* src0,
                        const uint8_t* src1,
                        const uint8_t* alpha,
                        uint8_t* dst,
                        int width) {
  __m256i ymm_ff00 = _mm256_set1_epi16((short)0xff00);
  __m256i ymm_80 = _mm256_set1_epi8((char)0x80);
  __m256i ymm_round = _mm256_set1_epi16((short)0x807f);

  while (width > 0) {
    __m256i ymm_a = _mm256_loadu_si256((const __m256i*)alpha);
    __m256i ymm_s0 = _mm256_loadu_si256((const __m256i*)src0);
    __m256i ymm_s1 = _mm256_loadu_si256((const __m256i*)src1);
    alpha += 32;
    src0 += 32;
    src1 += 32;

    __m256i ymm_a_lo = _mm256_unpacklo_epi8(ymm_a, ymm_a);
    __m256i ymm_a_hi = _mm256_unpackhi_epi8(ymm_a, ymm_a);
    ymm_a_lo = _mm256_xor_si256(ymm_a_lo, ymm_ff00);
    ymm_a_hi = _mm256_xor_si256(ymm_a_hi, ymm_ff00);

    __m256i ymm_s_lo = _mm256_unpacklo_epi8(ymm_s0, ymm_s1);
    __m256i ymm_s_hi = _mm256_unpackhi_epi8(ymm_s0, ymm_s1);
    ymm_s_lo = _mm256_sub_epi8(ymm_s_lo, ymm_80);
    ymm_s_hi = _mm256_sub_epi8(ymm_s_hi, ymm_80);

    __m256i ymm_p_lo = _mm256_maddubs_epi16(ymm_a_lo, ymm_s_lo);
    __m256i ymm_p_hi = _mm256_maddubs_epi16(ymm_a_hi, ymm_s_hi);

    ymm_p_lo = _mm256_add_epi16(ymm_p_lo, ymm_round);
    ymm_p_hi = _mm256_add_epi16(ymm_p_hi, ymm_round);

    ymm_p_lo = _mm256_srli_epi16(ymm_p_lo, 8);
    ymm_p_hi = _mm256_srli_epi16(ymm_p_hi, 8);

    __m256i ymm_d = _mm256_packus_epi16(ymm_p_lo, ymm_p_hi);
    _mm256_storeu_si256((__m256i*)dst, ymm_d);
    dst += 32;
    width -= 32;
  }
  _mm256_zeroupper();
}
#endif  // HAS_BLENDPLANEROW_AVX2

#ifdef HAS_BLENDPLANEROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void BlendPlaneRow_AVX512BW(const uint8_t* src0,
                            const uint8_t* src1,
                            const uint8_t* alpha,
                            uint8_t* dst,
                            int width) {
  __m512i zmm_ff00 = _mm512_set1_epi16((short)0xff00);
  __m512i zmm_80 = _mm512_set1_epi8((char)0x80);
  __m512i zmm_round = _mm512_set1_epi16((short)0x807f);

  while (width >= 64) {
    __m512i zmm_a = _mm512_loadu_si512((const __m512i*)alpha);
    __m512i zmm_s0 = _mm512_loadu_si512((const __m512i*)src0);
    __m512i zmm_s1 = _mm512_loadu_si512((const __m512i*)src1);
    alpha += 64;
    src0 += 64;
    src1 += 64;

    __m512i zmm_a_lo = _mm512_unpacklo_epi8(zmm_a, zmm_a);
    __m512i zmm_a_hi = _mm512_unpackhi_epi8(zmm_a, zmm_a);
    zmm_a_lo = _mm512_xor_si512(zmm_a_lo, zmm_ff00);
    zmm_a_hi = _mm512_xor_si512(zmm_a_hi, zmm_ff00);

    __m512i zmm_s_lo = _mm512_unpacklo_epi8(zmm_s0, zmm_s1);
    __m512i zmm_s_hi = _mm512_unpackhi_epi8(zmm_s0, zmm_s1);
    zmm_s_lo = _mm512_sub_epi8(zmm_s_lo, zmm_80);
    zmm_s_hi = _mm512_sub_epi8(zmm_s_hi, zmm_80);

    __m512i zmm_p_lo = _mm512_maddubs_epi16(zmm_a_lo, zmm_s_lo);
    __m512i zmm_p_hi = _mm512_maddubs_epi16(zmm_a_hi, zmm_s_hi);

    zmm_p_lo = _mm512_add_epi16(zmm_p_lo, zmm_round);
    zmm_p_hi = _mm512_add_epi16(zmm_p_hi, zmm_round);

    zmm_p_lo = _mm512_srli_epi16(zmm_p_lo, 8);
    zmm_p_hi = _mm512_srli_epi16(zmm_p_hi, 8);

    __m512i zmm_d = _mm512_packus_epi16(zmm_p_lo, zmm_p_hi);
    _mm512_storeu_si512((__m512i*)dst, zmm_d);
    dst += 64;
    width -= 64;
  }
  if (width > 0) {
    __mmask64 mask = (1ULL << width) - 1ULL;
    __m512i zmm_a = _mm512_maskz_loadu_epi8(mask, alpha);
    __m512i zmm_s0 = _mm512_maskz_loadu_epi8(mask, src0);
    __m512i zmm_s1 = _mm512_maskz_loadu_epi8(mask, src1);

    __m512i zmm_a_lo = _mm512_unpacklo_epi8(zmm_a, zmm_a);
    __m512i zmm_a_hi = _mm512_unpackhi_epi8(zmm_a, zmm_a);
    zmm_a_lo = _mm512_xor_si512(zmm_a_lo, zmm_ff00);
    zmm_a_hi = _mm512_xor_si512(zmm_a_hi, zmm_ff00);

    __m512i zmm_s_lo = _mm512_unpacklo_epi8(zmm_s0, zmm_s1);
    __m512i zmm_s_hi = _mm512_unpackhi_epi8(zmm_s0, zmm_s1);
    zmm_s_lo = _mm512_sub_epi8(zmm_s_lo, zmm_80);
    zmm_s_hi = _mm512_sub_epi8(zmm_s_hi, zmm_80);

    __m512i zmm_p_lo = _mm512_maddubs_epi16(zmm_a_lo, zmm_s_lo);
    __m512i zmm_p_hi = _mm512_maddubs_epi16(zmm_a_hi, zmm_s_hi);

    zmm_p_lo = _mm512_add_epi16(zmm_p_lo, zmm_round);
    zmm_p_hi = _mm512_add_epi16(zmm_p_hi, zmm_round);

    zmm_p_lo = _mm512_srli_epi16(zmm_p_lo, 8);
    zmm_p_hi = _mm512_srli_epi16(zmm_p_hi, 8);

    __m512i zmm_d = _mm512_packus_epi16(zmm_p_lo, zmm_p_hi);
    _mm512_mask_storeu_epi8(dst, mask, zmm_d);
  }
  _mm256_zeroupper();
}
#endif  // HAS_BLENDPLANEROW_AVX512BW

#ifdef HAS_ARGBEXTRACTALPHAROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBExtractAlphaRow_AVX512BW(const uint8_t* src_argb,
                                  uint8_t* dst_a,
                                  int width) {
  while (width >= 32) {
    __m512i z0 = _mm512_loadu_si512((const __m512i*)src_argb);
    __m512i z1 = _mm512_loadu_si512((const __m512i*)(src_argb + 64));
    __m128i a0 = _mm512_cvtepi32_epi8(_mm512_srli_epi32(z0, 24));
    __m128i a1 = _mm512_cvtepi32_epi8(_mm512_srli_epi32(z1, 24));
    _mm_storeu_si128((__m128i*)dst_a, a0);
    _mm_storeu_si128((__m128i*)(dst_a + 16), a1);
    src_argb += 128;
    dst_a += 32;
    width -= 32;
  }
  if (width >= 16) {
    __m512i z0 = _mm512_loadu_si512((const __m512i*)src_argb);
    __m128i a0 = _mm512_cvtepi32_epi8(_mm512_srli_epi32(z0, 24));
    _mm_storeu_si128((__m128i*)dst_a, a0);
    src_argb += 64;
    dst_a += 16;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 mask = (__mmask16)((1u << width) - 1u);
    __m512i z0 = _mm512_maskz_loadu_epi32(mask, src_argb);
    _mm512_mask_cvtepi32_storeu_epi8(dst_a, mask, _mm512_srli_epi32(z0, 24));
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBEXTRACTALPHAROW_AVX512BW

#ifdef HAS_ARGBCOPYYTOALPHAROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBCopyYToAlphaRow_AVX512BW(const uint8_t* src, uint8_t* dst, int width) {
  __m512i rgb_mask = _mm512_set1_epi32(0x00ffffff);
  while (width >= 32) {
    __m512i a0 = _mm512_slli_epi32(
        _mm512_cvtepu8_epi32(_mm_loadu_si128((const __m128i*)src)), 24);
    __m512i a1 = _mm512_slli_epi32(
        _mm512_cvtepu8_epi32(_mm_loadu_si128((const __m128i*)(src + 16))), 24);
    a0 = _mm512_ternarylogic_epi32(
        a0, rgb_mask, _mm512_loadu_si512((const __m512i*)dst), 0xf8);
    a1 = _mm512_ternarylogic_epi32(
        a1, rgb_mask, _mm512_loadu_si512((const __m512i*)(dst + 64)), 0xf8);
    _mm512_storeu_si512((__m512i*)dst, a0);
    _mm512_storeu_si512((__m512i*)(dst + 64), a1);
    src += 32;
    dst += 128;
    width -= 32;
  }
  if (width >= 16) {
    __m512i a0 = _mm512_slli_epi32(
        _mm512_cvtepu8_epi32(_mm_loadu_si128((const __m128i*)src)), 24);
    a0 = _mm512_ternarylogic_epi32(
        a0, rgb_mask, _mm512_loadu_si512((const __m512i*)dst), 0xf8);
    _mm512_storeu_si512((__m512i*)dst, a0);
    src += 16;
    dst += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 mask = (__mmask16)((1u << width) - 1u);
    __m512i a0 = _mm512_slli_epi32(
        _mm512_maskz_cvtepu8_epi32(mask, _mm_maskz_loadu_epi8(mask, src)), 24);
    a0 = _mm512_ternarylogic_epi32(a0, rgb_mask,
                                   _mm512_maskz_loadu_epi32(mask, dst), 0xf8);
    _mm512_mask_storeu_epi32(dst, mask, a0);
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBCOPYYTOALPHAROW_AVX512BW

#ifdef HAS_ARGBMIRRORROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBMirrorRow_AVX512BW(const uint8_t* src, uint8_t* dst, int width) {
  const __m512i rev =
      _mm512_set_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
  const uint8_t* s = src + (ptrdiff_t)width * 4;
  while (width >= 16) {
    s -= 64;
    _mm512_storeu_si512(
        (__m512i*)dst,
        _mm512_permutexvar_epi32(rev, _mm512_loadu_si512((const __m512i*)s)));
    dst += 64;
    width -= 16;
  }
  if (width > 0) {
    // Remaining 1 to 15 pixels are at the start of src.
    __mmask16 mask = (__mmask16)((1u << width) - 1u);
    __m512i idx = _mm512_sub_epi32(rev, _mm512_set1_epi32(16 - width));
    __m512i v = _mm512_maskz_loadu_epi32(mask, src);
    _mm512_mask_storeu_epi32(dst, mask, _mm512_permutexvar_epi32(idx, v));
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBMIRRORROW_AVX512BW

#ifdef HAS_RGB24MIRRORROW_AVX512VBMI
static const uint8_t kPermMirrorRGB24_0[64] = {
    125u, 126u, 127u, 122u, 123u, 124u, 119u, 120u, 121u, 116u, 117u,
    118u, 113u, 114u, 115u, 110u, 111u, 112u, 107u, 108u, 109u, 104u,
    105u, 106u, 101u, 102u, 103u, 98u,  99u,  100u, 95u,  96u,  97u,
    92u,  93u,  94u,  89u,  90u,  91u,  86u,  87u,  88u,  83u,  84u,
    85u,  80u,  81u,  82u,  77u,  78u,  79u,  74u,  75u,  76u,  71u,
    72u,  73u,  68u,  69u,  70u,  65u,  66u,  67u,  62u};
static const uint8_t kPermMirrorRGB24_1[64] = {
    127u, 128u, 123u, 124u, 125u, 120u, 121u, 122u, 117u, 118u, 119u,
    114u, 115u, 116u, 111u, 112u, 113u, 108u, 109u, 110u, 105u, 106u,
    107u, 102u, 103u, 104u, 99u,  100u, 101u, 96u,  97u,  98u,  93u,
    94u,  95u,  90u,  91u,  92u,  87u,  88u,  89u,  84u,  85u,  86u,
    81u,  82u,  83u,  78u,  79u,  80u,  75u,  76u,  77u,  72u,  73u,
    74u,  69u,  70u,  71u,  66u,  67u,  68u,  63u,  64u};
static const uint8_t kPermMirrorRGB24_2[64] = {
    65u, 60u, 61u, 62u, 57u, 58u, 59u, 54u, 55u, 56u, 51u, 52u, 53u,
    48u, 49u, 50u, 45u, 46u, 47u, 42u, 43u, 44u, 39u, 40u, 41u, 36u,
    37u, 38u, 33u, 34u, 35u, 30u, 31u, 32u, 27u, 28u, 29u, 24u, 25u,
    26u, 21u, 22u, 23u, 18u, 19u, 20u, 15u, 16u, 17u, 12u, 13u, 14u,
    9u,  10u, 11u, 6u,  7u,  8u,  3u,  4u,  5u,  0u,  1u,  2u};

// Byte mask with the low n bits set, clamped to [0, 64].
static inline __mmask64 RGB24MirrorMask(int n) {
  return n <= 0 ? 0ull : n >= 64 ? ~0ull : (1ull << n) - 1ull;
}

// Mirror 64 RGB24 pixels (3 full zmm vectors) per loop.  The remaining 1 to
// 63 pixels use masked loads and stores that access only valid bytes.
LIBYUV_TARGET_AVX512VBMI
void RGB24MirrorRow_AVX512VBMI(const uint8_t* src_rgb24,
                               uint8_t* dst_rgb24,
                               int width) {
  const __m512i idx0 = _mm512_loadu_si512((const __m512i*)kPermMirrorRGB24_0);
  const __m512i idx1 = _mm512_loadu_si512((const __m512i*)kPermMirrorRGB24_1);
  const __m512i idx2 = _mm512_loadu_si512((const __m512i*)kPermMirrorRGB24_2);
  int r = width & 63;
  int w = width - r;
  if (w > 0) {
    const uint8_t* src = src_rgb24 + (ptrdiff_t)width * 3 - 192;
    do {
      __m512i a = _mm512_loadu_si512((const __m512i*)src);
      __m512i b = _mm512_loadu_si512((const __m512i*)(src + 64));
      __m512i c = _mm512_loadu_si512((const __m512i*)(src + 128));
      __m512i d0 = _mm512_permutex2var_epi8(b, idx0, c);
      __m512i d1 = _mm512_permutex2var_epi8(a, idx1, b);
      __m512i d2 = _mm512_permutex2var_epi8(a, idx2, b);
      d1 = _mm512_mask_broadcastb_epi8(d1, (__mmask64)2,
                                       _mm512_castsi512_si128(c));
      _mm512_storeu_si512((__m512i*)dst_rgb24, d0);
      _mm512_storeu_si512((__m512i*)(dst_rgb24 + 64), d1);
      _mm512_storeu_si512((__m512i*)(dst_rgb24 + 128), d2);
      src -= 192;
      dst_rgb24 += 192;
      w -= 64;
    } while (w > 0);
  }
  if (r) {
    int nb = r * 3;
    int off = 192 - nb;
    __mmask64 m0 = RGB24MirrorMask(nb);
    __mmask64 m1 = RGB24MirrorMask(nb - 64);
    __mmask64 m2 = RGB24MirrorMask(nb - 128);
    __m512i a = _mm512_maskz_loadu_epi8(m0, src_rgb24);
    __m512i b = _mm512_maskz_loadu_epi8(m1, src_rgb24 + 64);
    __m512i c = _mm512_maskz_loadu_epi8(m2, src_rgb24 + 128);
    __m512i t0 = _mm512_sub_epi8(idx0, _mm512_set1_epi8((char)(off - 64)));
    __m512i t1 = _mm512_sub_epi8(idx1, _mm512_set1_epi8((char)off));
    __m512i t2 = _mm512_sub_epi8(idx2, _mm512_set1_epi8((char)off));
    __m512i d0 = _mm512_mask_permutexvar_epi8(
        _mm512_permutex2var_epi8(a, t0, b), _mm512_movepi8_mask(t0), t0, c);
    __m512i d1 = _mm512_mask_permutexvar_epi8(
        _mm512_permutex2var_epi8(a, t1, b), _mm512_movepi8_mask(t1), t1, c);
    __m512i d2 = _mm512_mask_permutexvar_epi8(
        _mm512_permutex2var_epi8(a, t2, b), _mm512_movepi8_mask(t2), t2, c);
    _mm512_mask_storeu_epi8(dst_rgb24, m0, d0);
    _mm512_mask_storeu_epi8(dst_rgb24 + 64, m1, d1);
    _mm512_mask_storeu_epi8(dst_rgb24 + 128, m2, d2);
  }
  _mm256_zeroupper();
}
#endif  // HAS_RGB24MIRRORROW_AVX512VBMI

// ===========================================================================
// Module 1: AYUV to Planar Y / UV / VU (AVX512VBMI)
// ===========================================================================

#ifdef HAS_AYUVTOYROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void AYUVToYRow_AVX512VBMI(const uint8_t* src_ayuv, uint8_t* dst_y, int width) {
  static const uint8_t kPermAYUVToY32[64] = {
      2,  6,  10, 14, 18, 22, 26, 30, 34, 38,  42,  46,  50,  54,  58,  62,
      66, 70, 74, 78, 82, 86, 90, 94, 98, 102, 106, 110, 114, 118, 122, 126,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,   0,   0,   0,   0,   0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,   0,   0,   0,   0,   0};
  const __m512i zmm_perm = _mm512_loadu_si512((const __m512i*)kPermAYUVToY32);
  while (width >= 32) {
    __m512i zmm0 = _mm512_loadu_si512((const __m512i*)src_ayuv);
    __m512i zmm1 = _mm512_loadu_si512((const __m512i*)(src_ayuv + 64));
    src_ayuv += 128;
    __m512i out = _mm512_permutex2var_epi8(zmm0, zmm_perm, zmm1);
    _mm256_storeu_si256((__m256i*)dst_y, _mm512_castsi512_si256(out));
    dst_y += 32;
    width -= 32;
  }
  if (width > 0) {
    uint32_t m32 = (1u << width) - 1u;
    __mmask16 k1 = (__mmask16)m32;
    __mmask16 k2 = (__mmask16)(m32 >> 16);
    __m512i zmm0 = _mm512_maskz_loadu_epi32(k1, src_ayuv);
    __m512i zmm1 = _mm512_maskz_loadu_epi32(k2, src_ayuv + 64);
    __m512i out = _mm512_permutex2var_epi8(zmm0, zmm_perm, zmm1);
    _mm256_mask_storeu_epi8(dst_y, (__mmask32)m32, _mm512_castsi512_si256(out));
  }
  _mm256_zeroupper();
}
#endif  // HAS_AYUVTOYROW_AVX512VBMI

#if defined(HAS_AYUVTOUVROW_AVX512VBMI) || defined(HAS_AYUVTOVUROW_AVX512VBMI)
static const uint8_t kPermAYUVToUV_Win[64] = {
    1,   5,   0,   4,   9,   13,  8,   12,  17,  21,  16,  20, 25,
    29,  24,  28,  33,  37,  32,  36,  41,  45,  40,  44,  49, 53,
    48,  52,  57,  61,  56,  60,  65,  69,  64,  68,  73,  77, 72,
    76,  81,  85,  80,  84,  89,  93,  88,  92,  97,  101, 96, 100,
    105, 109, 104, 108, 113, 117, 112, 116, 121, 125, 120, 124};

static const uint8_t kPermAYUVToVU_Win[64] = {
    0,   4,   1,   5,   8,   12,  9,   13,  16,  20,  17,  21, 24,
    28,  25,  29,  32,  36,  33,  37,  40,  44,  41,  45,  48, 52,
    49,  53,  56,  60,  57,  61,  64,  68,  65,  69,  72,  76, 73,
    77,  80,  84,  81,  85,  88,  92,  89,  93,  96,  100, 97, 101,
    104, 108, 105, 109, 112, 116, 113, 117, 120, 124, 121, 125};

LIBYUV_TARGET_AVX512VBMI
static inline void AYUVToUVOrVURow_Win_AVX512VBMI(const uint8_t* src_ayuv,
                                                  int src_stride_ayuv,
                                                  uint8_t* dst_uv,
                                                  const uint8_t* perm_table,
                                                  int width) {
  const __m512i zmm_perm = _mm512_loadu_si512((const __m512i*)perm_table);
  const __m512i zmm_0101 = _mm512_set1_epi16(0x0101);
  const __m512i zmm_zero = _mm512_setzero_si512();
  while (width >= 32) {
    __m512i s00 = _mm512_loadu_si512((const __m512i*)src_ayuv);
    __m512i s01 = _mm512_loadu_si512((const __m512i*)(src_ayuv + 64));
    __m512i s10 =
        _mm512_loadu_si512((const __m512i*)(src_ayuv + src_stride_ayuv));
    __m512i s11 =
        _mm512_loadu_si512((const __m512i*)(src_ayuv + src_stride_ayuv + 64));
    src_ayuv += 128;
    __m512i p0 = _mm512_permutex2var_epi8(s00, zmm_perm, s01);
    __m512i p1 = _mm512_permutex2var_epi8(s10, zmm_perm, s11);
    p0 = _mm512_maddubs_epi16(p0, zmm_0101);
    p1 = _mm512_maddubs_epi16(p1, zmm_0101);
    p0 = _mm512_avg_epu16(_mm512_srli_epi16(_mm512_add_epi16(p0, p1), 1),
                          zmm_zero);
    _mm256_storeu_si256((__m256i*)dst_uv, _mm512_cvtepi16_epi8(p0));
    dst_uv += 32;
    width -= 32;
  }
  if (width > 0) {
    uint32_t m32 = (1u << width) - 1u;
    __mmask16 k1 = (__mmask16)m32;
    __mmask16 k2 = (__mmask16)(m32 >> 16);
    __m512i s00 = _mm512_maskz_loadu_epi32(k1, src_ayuv);
    __m512i s01 = _mm512_maskz_loadu_epi32(k2, src_ayuv + 64);
    __m512i s10 = _mm512_maskz_loadu_epi32(k1, src_ayuv + src_stride_ayuv);
    __m512i s11 = _mm512_maskz_loadu_epi32(k2, src_ayuv + src_stride_ayuv + 64);
    __m512i p0 = _mm512_permutex2var_epi8(s00, zmm_perm, s01);
    __m512i p1 = _mm512_permutex2var_epi8(s10, zmm_perm, s11);
    __mmask16 k_odd = (__mmask16)((width & 1) << (width >> 1));
    p0 = _mm512_mask_or_epi32(p0, k_odd, p0, _mm512_slli_epi32(p0, 8));
    p1 = _mm512_mask_or_epi32(p1, k_odd, p1, _mm512_slli_epi32(p1, 8));
    p0 = _mm512_maddubs_epi16(p0, zmm_0101);
    p1 = _mm512_maddubs_epi16(p1, zmm_0101);
    p0 = _mm512_avg_epu16(_mm512_srli_epi16(_mm512_add_epi16(p0, p1), 1),
                          zmm_zero);
    uint32_t out_bytes = (uint32_t)((width + 1) & ~1);
    __mmask32 k_out =
        out_bytes >= 32 ? ~0u : (__mmask32)((1u << out_bytes) - 1u);
    _mm256_mask_storeu_epi8(dst_uv, k_out, _mm512_cvtepi16_epi8(p0));
  }
  _mm256_zeroupper();
}
#endif

#ifdef HAS_AYUVTOUVROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void AYUVToUVRow_AVX512VBMI(const uint8_t* src_ayuv,
                            int src_stride_ayuv,
                            uint8_t* dst_uv,
                            int width) {
  AYUVToUVOrVURow_Win_AVX512VBMI(src_ayuv, src_stride_ayuv, dst_uv,
                                 kPermAYUVToUV_Win, width);
}
#endif  // HAS_AYUVTOUVROW_AVX512VBMI

#ifdef HAS_AYUVTOVUROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void AYUVToVURow_AVX512VBMI(const uint8_t* src_ayuv,
                            int src_stride_ayuv,
                            uint8_t* dst_vu,
                            int width) {
  AYUVToUVOrVURow_Win_AVX512VBMI(src_ayuv, src_stride_ayuv, dst_vu,
                                 kPermAYUVToVU_Win, width);
}
#endif  // HAS_AYUVTOVUROW_AVX512VBMI

// ===========================================================================
// Module 2: 10-bit / 30-bit AR30 & XR30 Pack / Unpack (AVX512BW)
// ===========================================================================

#ifdef HAS_AR30TOARGBROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void AR30ToARGBRow_AVX512BW(const uint8_t* src_ar30,
                            uint8_t* dst_argb,
                            int width) {
  const __m512i m0 = _mm512_set1_epi32((int)0xc0000000u);
  const __m512i m1 = _mm512_set1_epi32(0x300000ff);
  const __m512i m2 = _mm512_set1_epi32(0x0c00ff00);
  const __m512i m3 = _mm512_set1_epi32(0x03ff0000);
  while (width >= 16) {
    __m512i v = _mm512_loadu_si512((const __m512i*)src_ar30);
    src_ar30 += 64;
    __m512i out = _mm512_or_si512(
        _mm512_or_si512(_mm512_and_si512(v, m0),
                        _mm512_and_si512(_mm512_srli_epi32(v, 2), m1)),
        _mm512_or_si512(_mm512_and_si512(_mm512_srli_epi32(v, 4), m2),
                        _mm512_and_si512(_mm512_srli_epi32(v, 6), m3)));
    _mm512_storeu_si512((__m512i*)dst_argb, out);
    dst_argb += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i v = _mm512_maskz_loadu_epi32(k, src_ar30);
    __m512i out = _mm512_or_si512(
        _mm512_or_si512(_mm512_and_si512(v, m0),
                        _mm512_and_si512(_mm512_srli_epi32(v, 2), m1)),
        _mm512_or_si512(_mm512_and_si512(_mm512_srli_epi32(v, 4), m2),
                        _mm512_and_si512(_mm512_srli_epi32(v, 6), m3)));
    _mm512_mask_storeu_epi32(dst_argb, k, out);
  }
  _mm256_zeroupper();
}
#endif  // HAS_AR30TOARGBROW_AVX512BW

#ifdef HAS_AR30TOABGRROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void AR30ToABGRRow_AVX512BW(const uint8_t* src_ar30,
                            uint8_t* dst_abgr,
                            int width) {
  static const uint8_t kShufSwapRB16[16] = {2,  1, 0, 3,  6,  5,  4,  7,
                                            10, 9, 8, 11, 14, 13, 12, 15};
  const __m512i m0 = _mm512_set1_epi32((int)0xc0000000u);
  const __m512i m1 = _mm512_set1_epi32(0x300000ff);
  const __m512i m2 = _mm512_set1_epi32(0x0c00ff00);
  const __m512i m3 = _mm512_set1_epi32(0x03ff0000);
  const __m512i shuf =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)kShufSwapRB16));
  while (width >= 16) {
    __m512i v = _mm512_loadu_si512((const __m512i*)src_ar30);
    src_ar30 += 64;
    __m512i out = _mm512_or_si512(
        _mm512_or_si512(_mm512_and_si512(v, m0),
                        _mm512_and_si512(_mm512_srli_epi32(v, 2), m1)),
        _mm512_or_si512(_mm512_and_si512(_mm512_srli_epi32(v, 4), m2),
                        _mm512_and_si512(_mm512_srli_epi32(v, 6), m3)));
    out = _mm512_shuffle_epi8(out, shuf);
    _mm512_storeu_si512((__m512i*)dst_abgr, out);
    dst_abgr += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i v = _mm512_maskz_loadu_epi32(k, src_ar30);
    __m512i out = _mm512_or_si512(
        _mm512_or_si512(_mm512_and_si512(v, m0),
                        _mm512_and_si512(_mm512_srli_epi32(v, 2), m1)),
        _mm512_or_si512(_mm512_and_si512(_mm512_srli_epi32(v, 4), m2),
                        _mm512_and_si512(_mm512_srli_epi32(v, 6), m3)));
    out = _mm512_shuffle_epi8(out, shuf);
    _mm512_mask_storeu_epi32(dst_abgr, k, out);
  }
  _mm256_zeroupper();
}
#endif  // HAS_AR30TOABGRROW_AVX512BW

#ifdef HAS_AR30TOAB30ROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void AR30ToAB30Row_AVX512BW(const uint8_t* src_ar30,
                            uint8_t* dst_ab30,
                            int width) {
  const __m512i m0 = _mm512_set1_epi32((int)0xc00ffc00u);
  const __m512i m1 = _mm512_set1_epi32(0x3ff00000);
  const __m512i m2 = _mm512_set1_epi32(0x000003ff);
  while (width >= 16) {
    __m512i v = _mm512_loadu_si512((const __m512i*)src_ar30);
    src_ar30 += 64;
    __m512i out = _mm512_or_si512(
        _mm512_and_si512(v, m0),
        _mm512_or_si512(_mm512_and_si512(_mm512_slli_epi32(v, 20), m1),
                        _mm512_and_si512(_mm512_srli_epi32(v, 20), m2)));
    _mm512_storeu_si512((__m512i*)dst_ab30, out);
    dst_ab30 += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i v = _mm512_maskz_loadu_epi32(k, src_ar30);
    __m512i out = _mm512_or_si512(
        _mm512_and_si512(v, m0),
        _mm512_or_si512(_mm512_and_si512(_mm512_slli_epi32(v, 20), m1),
                        _mm512_and_si512(_mm512_srli_epi32(v, 20), m2)));
    _mm512_mask_storeu_epi32(dst_ab30, k, out);
  }
  _mm256_zeroupper();
}
#endif  // HAS_AR30TOAB30ROW_AVX512BW

#ifdef HAS_MERGEXR30ROW_10_AVX512BW
LIBYUV_TARGET_AVX512BW
void MergeXR30Row_10_AVX512BW(const uint16_t* src_r,
                              const uint16_t* src_g,
                              const uint16_t* src_b,
                              uint8_t* dst_ar30,
                              int depth,
                              int width) {
  (void)depth;
  const __m512i max10 = _mm512_set1_epi32(1023);
  const __m512i alpha = _mm512_set1_epi32((int)0xc0000000u);
  while (width >= 16) {
    __m512i r = _mm512_min_epu32(
        _mm512_cvtepu16_epi32(_mm256_loadu_si256((const __m256i*)src_r)),
        max10);
    __m512i g = _mm512_min_epu32(
        _mm512_cvtepu16_epi32(_mm256_loadu_si256((const __m256i*)src_g)),
        max10);
    __m512i b = _mm512_min_epu32(
        _mm512_cvtepu16_epi32(_mm256_loadu_si256((const __m256i*)src_b)),
        max10);
    src_r += 16;
    src_g += 16;
    src_b += 16;
    __m512i out = _mm512_or_si512(
        _mm512_or_si512(_mm512_slli_epi32(r, 20), _mm512_slli_epi32(g, 10)),
        _mm512_or_si512(b, alpha));
    _mm512_storeu_si512((__m512i*)dst_ar30, out);
    dst_ar30 += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i r = _mm512_min_epu32(
        _mm512_cvtepu16_epi32(_mm256_maskz_loadu_epi16(k, src_r)), max10);
    __m512i g = _mm512_min_epu32(
        _mm512_cvtepu16_epi32(_mm256_maskz_loadu_epi16(k, src_g)), max10);
    __m512i b = _mm512_min_epu32(
        _mm512_cvtepu16_epi32(_mm256_maskz_loadu_epi16(k, src_b)), max10);
    __m512i out = _mm512_or_si512(
        _mm512_or_si512(_mm512_slli_epi32(r, 20), _mm512_slli_epi32(g, 10)),
        _mm512_or_si512(b, alpha));
    _mm512_mask_storeu_epi32(dst_ar30, k, out);
  }
  _mm256_zeroupper();
}
#endif  // HAS_MERGEXR30ROW_10_AVX512BW

#if defined(HAS_ARGBTOAR30ROW_AVX512BW) || defined(HAS_ABGRTOAR30ROW_AVX512BW)
static const uint8_t kShuffleRB30_Win[16] = {128u, 0u,  128u, 2u, 128u, 4u,
                                             128u, 6u,  128u, 8u, 128u, 10u,
                                             128u, 12u, 128u, 14u};

static const uint8_t kShuffleBR30_Win[16] = {128u, 2u,  128u, 0u,  128u, 6u,
                                             128u, 4u,  128u, 10u, 128u, 8u,
                                             128u, 14u, 128u, 12u};

LIBYUV_TARGET_AVX512BW
static inline void ARGBOrABGRToAR30Row_Win_AVX512BW(const uint8_t* src,
                                                    uint8_t* dst,
                                                    const uint8_t* shuf_rb,
                                                    int width) {
  const __m512i zmm_shuf =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)shuf_rb));
  const __m512i zmm_mul_rb = _mm512_set1_epi32(1028 * 16 * 65536 + 1028);
  const __m512i zmm_mask_rb = _mm512_set1_epi32(0x3ff003ff);
  const __m512i zmm_mask_ag = _mm512_set1_epi32((int)0xc000ff00u);
  const __m512i zmm_mul_ag = _mm512_set1_epi32(64 * 65536 + 1028);
  while (width >= 16) {
    __m512i v = _mm512_loadu_si512((const __m512i*)src);
    src += 64;
    __m512i rb = _mm512_and_si512(
        _mm512_mulhi_epu16(_mm512_shuffle_epi8(v, zmm_shuf), zmm_mul_rb),
        zmm_mask_rb);
    __m512i ag = _mm512_slli_epi32(
        _mm512_mulhi_epu16(_mm512_and_si512(v, zmm_mask_ag), zmm_mul_ag), 10);
    _mm512_storeu_si512((__m512i*)dst, _mm512_or_si512(ag, rb));
    dst += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i v = _mm512_maskz_loadu_epi32(k, src);
    __m512i rb = _mm512_and_si512(
        _mm512_mulhi_epu16(_mm512_shuffle_epi8(v, zmm_shuf), zmm_mul_rb),
        zmm_mask_rb);
    __m512i ag = _mm512_slli_epi32(
        _mm512_mulhi_epu16(_mm512_and_si512(v, zmm_mask_ag), zmm_mul_ag), 10);
    _mm512_mask_storeu_epi32(dst, k, _mm512_or_si512(ag, rb));
  }
  _mm256_zeroupper();
}
#endif

#ifdef HAS_ARGBTOAR30ROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBToAR30Row_AVX512BW(const uint8_t* src, uint8_t* dst, int width) {
  ARGBOrABGRToAR30Row_Win_AVX512BW(src, dst, kShuffleRB30_Win, width);
}
#endif  // HAS_ARGBTOAR30ROW_AVX512BW

#ifdef HAS_ABGRTOAR30ROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ABGRToAR30Row_AVX512BW(const uint8_t* src, uint8_t* dst, int width) {
  ARGBOrABGRToAR30Row_Win_AVX512BW(src, dst, kShuffleBR30_Win, width);
}
#endif  // HAS_ABGRTOAR30ROW_AVX512BW

// ===========================================================================
// Module 3: Planar / Channel Pack & Mirror / Interpolate_16 (AVX512VBMI / BW)
// ===========================================================================

#ifdef HAS_MERGERGBROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void MergeRGBRow_AVX512VBMI(const uint8_t* src_r,
                            const uint8_t* src_g,
                            const uint8_t* src_b,
                            uint8_t* dst_rgb,
                            int width) {
  static const uint8_t kPermMergeRGB64_0[64] = {
      0x00, 0x20, 0x40, 0x01, 0x21, 0x41, 0x02, 0x22, 0x42, 0x03, 0x23,
      0x43, 0x04, 0x24, 0x44, 0x05, 0x25, 0x45, 0x06, 0x26, 0x46, 0x07,
      0x27, 0x47, 0x08, 0x28, 0x48, 0x09, 0x29, 0x49, 0x0a, 0x2a, 0x4a,
      0x0b, 0x2b, 0x4b, 0x0c, 0x2c, 0x4c, 0x0d, 0x2d, 0x4d, 0x0e, 0x2e,
      0x4e, 0x0f, 0x2f, 0x4f, 0x10, 0x30, 0x50, 0x11, 0x31, 0x51, 0x12,
      0x32, 0x52, 0x13, 0x33, 0x53, 0x14, 0x34, 0x54, 0x15};
  static const uint8_t kPermMergeRGB64_1[64] = {
      0x35, 0x55, 0x16, 0x36, 0x56, 0x17, 0x37, 0x57, 0x18, 0x38, 0x58,
      0x19, 0x39, 0x59, 0x1a, 0x3a, 0x5a, 0x1b, 0x3b, 0x5b, 0x1c, 0x3c,
      0x5c, 0x1d, 0x3d, 0x5d, 0x1e, 0x3e, 0x5e, 0x1f, 0x3f, 0x5f, 0x60,
      0x00, 0x20, 0x61, 0x01, 0x21, 0x62, 0x02, 0x22, 0x63, 0x03, 0x23,
      0x64, 0x04, 0x24, 0x65, 0x05, 0x25, 0x66, 0x06, 0x26, 0x67, 0x07,
      0x27, 0x68, 0x08, 0x28, 0x69, 0x09, 0x29, 0x6a, 0x0a};
  static const uint8_t kPermMergeRGB64_2[64] = {
      0x6a, 0x2b, 0x4b, 0x6b, 0x2c, 0x4c, 0x6c, 0x2d, 0x4d, 0x6d, 0x2e,
      0x4e, 0x6e, 0x2f, 0x4f, 0x6f, 0x30, 0x50, 0x70, 0x31, 0x51, 0x71,
      0x32, 0x52, 0x72, 0x33, 0x53, 0x73, 0x34, 0x54, 0x74, 0x35, 0x55,
      0x75, 0x36, 0x56, 0x76, 0x37, 0x57, 0x77, 0x38, 0x58, 0x78, 0x39,
      0x59, 0x79, 0x3a, 0x5a, 0x7a, 0x3b, 0x5b, 0x7b, 0x3c, 0x5c, 0x7c,
      0x3d, 0x5d, 0x7d, 0x3e, 0x5e, 0x7e, 0x3f, 0x5f, 0x7f};
  const __m512i m_rgb0 = _mm512_loadu_si512((const __m512i*)kPermMergeRGB64_0);
  const __m512i m_rgb1 = _mm512_loadu_si512((const __m512i*)kPermMergeRGB64_1);
  const __m512i m_rgb2 = _mm512_loadu_si512((const __m512i*)kPermMergeRGB64_2);
  while (width >= 64) {
    __m512i rg0 = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_loadu_si256((const __m256i*)src_r)),
        _mm256_loadu_si256((const __m256i*)src_g), 1);
    __m512i br = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_loadu_si256((const __m256i*)src_b)),
        _mm256_loadu_si256((const __m256i*)(src_r + 32)), 1);
    __m512i gb1 = _mm512_inserti64x4(
        _mm512_castsi256_si512(
            _mm256_loadu_si256((const __m256i*)(src_g + 32))),
        _mm256_loadu_si256((const __m256i*)(src_b + 32)), 1);
    src_r += 64;
    src_g += 64;
    src_b += 64;
    __m512i d0 = _mm512_permutex2var_epi8(rg0, m_rgb0, br);
    __m512i d1 =
        _mm512_mask_permutexvar_epi8(_mm512_permutex2var_epi8(rg0, m_rgb1, br),
                                     0xb6db6db600000000ull, m_rgb1, gb1);
    __m512i d2 = _mm512_permutex2var_epi8(br, m_rgb2, gb1);
    _mm512_storeu_si512((__m512i*)dst_rgb, d0);
    _mm512_storeu_si512((__m512i*)(dst_rgb + 64), d1);
    _mm512_storeu_si512((__m512i*)(dst_rgb + 128), d2);
    dst_rgb += 192;
    width -= 64;
  }
  if (width >= 32) {
    __m512i rg = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_loadu_si256((const __m256i*)src_r)),
        _mm256_loadu_si256((const __m256i*)src_g), 1);
    __m512i b =
        _mm512_castsi256_si512(_mm256_loadu_si256((const __m256i*)src_b));
    src_r += 32;
    src_g += 32;
    src_b += 32;
    __m512i d0 = _mm512_permutex2var_epi8(rg, m_rgb0, b);
    __m512i d1 = _mm512_permutex2var_epi8(rg, m_rgb1, b);
    _mm512_storeu_si512((__m512i*)dst_rgb, d0);
    _mm256_storeu_si256((__m256i*)(dst_rgb + 64), _mm512_castsi512_si256(d1));
    dst_rgb += 96;
    width -= 32;
  }
  if (width > 0) {
    __mmask32 k_in = (__mmask32)((1u << width) - 1u);
    __m512i rg = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_maskz_loadu_epi8(k_in, src_r)),
        _mm256_maskz_loadu_epi8(k_in, src_g), 1);
    __m512i b = _mm512_castsi256_si512(_mm256_maskz_loadu_epi8(k_in, src_b));
    __m512i d0 = _mm512_permutex2var_epi8(rg, m_rgb0, b);
    __m512i d1 = _mm512_permutex2var_epi8(rg, m_rgb1, b);
    int dst_bytes = width * 3;
    __mmask64 k0 = dst_bytes >= 64 ? ~0ull : (1ull << dst_bytes) - 1ull;
    __mmask32 k1 = dst_bytes > 64 ? (1u << (dst_bytes - 64)) - 1u : 0u;
    _mm512_mask_storeu_epi8(dst_rgb, k0, d0);
    _mm256_mask_storeu_epi8(dst_rgb + 64, k1, _mm512_castsi512_si256(d1));
  }
  _mm256_zeroupper();
}
#endif  // HAS_MERGERGBROW_AVX512VBMI

#ifdef HAS_RAWTORGBAROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void RAWToRGBARow_AVX512VBMI(const uint8_t* src_raw,
                             uint8_t* dst_rgba,
                             int width) {
  static const uint8_t kPermRAWToRGBA0[64] = {
      0, 2,  1,  0,  0, 5,  4,  3,  0, 8,  7,  6,  0, 11, 10, 9,
      0, 14, 13, 12, 0, 17, 16, 15, 0, 20, 19, 18, 0, 23, 22, 21,
      0, 26, 25, 24, 0, 29, 28, 27, 0, 32, 31, 30, 0, 35, 34, 33,
      0, 38, 37, 36, 0, 41, 40, 39, 0, 44, 43, 42, 0, 47, 46, 45};
  static const uint8_t kPermRAWToRGBA1[64] = {
      0, 50, 49, 48, 0, 53, 52, 51, 0, 56, 55, 54, 0, 59, 58, 57,
      0, 62, 61, 60, 0, 65, 64, 63, 0, 68, 67, 66, 0, 71, 70, 69,
      0, 74, 73, 72, 0, 77, 76, 75, 0, 80, 79, 78, 0, 83, 82, 81,
      0, 86, 85, 84, 0, 89, 88, 87, 0, 92, 91, 90, 0, 95, 94, 93};
  const __m512i p0 = _mm512_loadu_si512((const __m512i*)kPermRAWToRGBA0);
  const __m512i p1 = _mm512_loadu_si512((const __m512i*)kPermRAWToRGBA1);
  const __m512i alpha = _mm512_set1_epi32(0x000000ff);
  while (width >= 32) {
    __m512i s0 = _mm512_loadu_si512((const __m512i*)src_raw);
    __m512i s1 = _mm512_castsi256_si512(
        _mm256_loadu_si256((const __m256i*)(src_raw + 64)));
    src_raw += 96;
    __m512i d0 = _mm512_or_si512(_mm512_permutexvar_epi8(p0, s0), alpha);
    __m512i d1 = _mm512_or_si512(_mm512_permutex2var_epi8(s0, p1, s1), alpha);
    _mm512_storeu_si512((__m512i*)dst_rgba, d0);
    _mm512_storeu_si512((__m512i*)(dst_rgba + 64), d1);
    dst_rgba += 128;
    width -= 32;
  }
  if (width > 0) {
    int src_bytes = width * 3;
    __mmask64 ks0 = src_bytes >= 64 ? ~0ull : (1ull << src_bytes) - 1ull;
    __mmask32 ks1 = src_bytes > 64 ? (1u << (src_bytes - 64)) - 1u : 0u;
    __m512i s0 = _mm512_maskz_loadu_epi8(ks0, src_raw);
    __m512i s1 =
        _mm512_castsi256_si512(_mm256_maskz_loadu_epi8(ks1, src_raw + 64));
    __m512i d0 = _mm512_or_si512(_mm512_permutexvar_epi8(p0, s0), alpha);
    __m512i d1 = _mm512_or_si512(_mm512_permutex2var_epi8(s0, p1, s1), alpha);
    uint32_t m32 = (1u << width) - 1u;
    _mm512_mask_storeu_epi32(dst_rgba, (__mmask16)m32, d0);
    _mm512_mask_storeu_epi32(dst_rgba + 64, (__mmask16)(m32 >> 16), d1);
  }
  _mm256_zeroupper();
}
#endif  // HAS_RAWTORGBAROW_AVX512VBMI

#ifdef HAS_SWAPUVROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void SwapUVRow_AVX512BW(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
  static const uint8_t kSwapUV16[16] = {1, 0, 3,  2,  5,  4,  7,  6,
                                        9, 8, 11, 10, 13, 12, 15, 14};
  const __m512i swap =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)kSwapUV16));
  while (width >= 32) {
    __m512i v = _mm512_loadu_si512((const __m512i*)src_uv);
    src_uv += 64;
    _mm512_storeu_si512((__m512i*)dst_vu, _mm512_shuffle_epi8(v, swap));
    dst_vu += 64;
    width -= 32;
  }
  if (width > 0) {
    __mmask32 k = (__mmask32)((1u << width) - 1u);
    __m512i v = _mm512_maskz_loadu_epi16(k, src_uv);
    _mm512_mask_storeu_epi16(dst_vu, k, _mm512_shuffle_epi8(v, swap));
  }
  _mm256_zeroupper();
}
#endif  // HAS_SWAPUVROW_AVX512BW

#ifdef HAS_MIRRORROW_16_AVX512BW
LIBYUV_TARGET_AVX512BW
void MirrorRow_16_AVX512BW(const uint16_t* src, uint16_t* dst, int width) {
  static const uint16_t kMirror16Idx[32] = {
      31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
      15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0};
  const __m512i rev = _mm512_loadu_si512((const __m512i*)kMirror16Idx);
  src += width;
  while (width >= 32) {
    src -= 32;
    __m512i v = _mm512_loadu_si512((const __m512i*)src);
    _mm512_storeu_si512((__m512i*)dst, _mm512_permutexvar_epi16(rev, v));
    dst += 32;
    width -= 32;
  }
  if (width > 0) {
    __mmask32 k = (__mmask32)((1u << width) - 1u);
    __m512i idx = _mm512_sub_epi16(rev, _mm512_set1_epi16((short)(32 - width)));
    __m512i v = _mm512_maskz_loadu_epi16(k, src - width);
    _mm512_mask_storeu_epi16(dst, k, _mm512_permutexvar_epi16(idx, v));
  }
  _mm256_zeroupper();
}
#endif  // HAS_MIRRORROW_16_AVX512BW

#ifdef HAS_HALFMERGEUVROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void HalfMergeUVRow_AVX512BW(const uint8_t* src_u,
                             int src_stride_u,
                             const uint8_t* src_v,
                             int src_stride_v,
                             uint8_t* dst_uv,
                             int width) {
  const __m512i c0101 = _mm512_set1_epi16(0x0101);
  const __m512i czero = _mm512_setzero_si512();
  const __m512i c00ff = _mm512_set1_epi16(0x00ff);
  while (width >= 64) {
    __m512i u0 = _mm512_loadu_si512((const __m512i*)src_u);
    __m512i u1 = _mm512_loadu_si512((const __m512i*)(src_u + src_stride_u));
    __m512i v0 = _mm512_loadu_si512((const __m512i*)src_v);
    __m512i v1 = _mm512_loadu_si512((const __m512i*)(src_v + src_stride_v));
    src_u += 64;
    src_v += 64;
    u0 = _mm512_avg_epu16(
        _mm512_srli_epi16(_mm512_add_epi16(_mm512_maddubs_epi16(u0, c0101),
                                           _mm512_maddubs_epi16(u1, c0101)),
                          1),
        czero);
    v0 = _mm512_avg_epu16(
        _mm512_srli_epi16(_mm512_add_epi16(_mm512_maddubs_epi16(v0, c0101),
                                           _mm512_maddubs_epi16(v1, c0101)),
                          1),
        czero);
    _mm512_storeu_si512((__m512i*)dst_uv,
                        _mm512_or_si512(u0, _mm512_slli_epi16(v0, 8)));
    dst_uv += 64;
    width -= 64;
  }
  if (width > 0) {
    __mmask64 k = width >= 64 ? ~0ull : (1ull << width) - 1ull;
    __m512i u0 = _mm512_maskz_loadu_epi8(k, src_u);
    __m512i u1 = _mm512_maskz_loadu_epi8(k, src_u + src_stride_u);
    __m512i v0 = _mm512_maskz_loadu_epi8(k, src_v);
    __m512i v1 = _mm512_maskz_loadu_epi8(k, src_v + src_stride_v);
    __mmask32 k_odd = (__mmask32)((uint32_t)(width & 1) << (width >> 1));
    u0 = _mm512_mask_mullo_epi16(u0, k_odd, _mm512_and_si512(u0, c00ff), c0101);
    u1 = _mm512_mask_mullo_epi16(u1, k_odd, _mm512_and_si512(u1, c00ff), c0101);
    v0 = _mm512_mask_mullo_epi16(v0, k_odd, _mm512_and_si512(v0, c00ff), c0101);
    v1 = _mm512_mask_mullo_epi16(v1, k_odd, _mm512_and_si512(v1, c00ff), c0101);
    u0 = _mm512_avg_epu16(
        _mm512_srli_epi16(_mm512_add_epi16(_mm512_maddubs_epi16(u0, c0101),
                                           _mm512_maddubs_epi16(u1, c0101)),
                          1),
        czero);
    v0 = _mm512_avg_epu16(
        _mm512_srli_epi16(_mm512_add_epi16(_mm512_maddubs_epi16(v0, c0101),
                                           _mm512_maddubs_epi16(v1, c0101)),
                          1),
        czero);
    int out_words = (width + 1) >> 1;
    __mmask32 k_out =
        out_words >= 32 ? ~0u : (__mmask32)((1u << out_words) - 1u);
    _mm512_mask_storeu_epi16(dst_uv, k_out,
                             _mm512_or_si512(u0, _mm512_slli_epi16(v0, 8)));
  }
  _mm256_zeroupper();
}
#endif  // HAS_HALFMERGEUVROW_AVX512BW

#ifdef HAS_INTERPOLATEROW_16_AVX512BW
LIBYUV_TARGET_AVX512BW
void InterpolateRow_16_AVX512BW(uint16_t* dst_ptr,
                                const uint16_t* src_ptr,
                                ptrdiff_t src_stride,
                                int width,
                                int source_y_fraction) {
  int y1 = source_y_fraction;
  int y0 = 256 - y1;
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  __m512i zmm_y = _mm512_set1_epi32((y1 << 16) | y0);
  __m512i zmm_8000 = _mm512_set1_epi16((short)0x8000);
  __m512i zmm_round = _mm512_set1_epi32(8388736);  // 0x800000 + 128
  int i;

  if (y1 == 0) {
    for (i = 0; i <= width - 32; i += 32) {
      _mm512_storeu_si512((__m512i*)(dst_ptr + i),
                          _mm512_loadu_si512((const __m512i*)(src_ptr + i)));
    }
    if (i < width) {
      __mmask32 mask = (__mmask32)((1u << (width - i)) - 1u);
      _mm512_mask_storeu_epi16(dst_ptr + i, mask,
                               _mm512_maskz_loadu_epi16(mask, src_ptr + i));
    }
  } else if (y1 == 128) {
    for (i = 0; i <= width - 32; i += 32) {
      __m512i row0 = _mm512_loadu_si512((const __m512i*)(src_ptr + i));
      __m512i row1 = _mm512_loadu_si512((const __m512i*)(src_ptr1 + i));
      _mm512_storeu_si512((__m512i*)(dst_ptr + i),
                          _mm512_avg_epu16(row0, row1));
    }
    if (i < width) {
      __mmask32 mask = (__mmask32)((1u << (width - i)) - 1u);
      __m512i row0 = _mm512_maskz_loadu_epi16(mask, src_ptr + i);
      __m512i row1 = _mm512_maskz_loadu_epi16(mask, src_ptr1 + i);
      _mm512_mask_storeu_epi16(dst_ptr + i, mask, _mm512_avg_epu16(row0, row1));
    }
  } else {
    for (i = 0; i <= width - 32; i += 32) {
      __m512i row0 = _mm512_loadu_si512((const __m512i*)(src_ptr + i));
      __m512i row1 = _mm512_loadu_si512((const __m512i*)(src_ptr1 + i));
      __m512i row0l = _mm512_unpacklo_epi16(row0, row1);
      __m512i row0h = _mm512_unpackhi_epi16(row0, row1);
      row0l = _mm512_sub_epi16(row0l, zmm_8000);
      row0h = _mm512_sub_epi16(row0h, zmm_8000);
      __m512i resl = _mm512_madd_epi16(row0l, zmm_y);
      __m512i resh = _mm512_madd_epi16(row0h, zmm_y);
      resl = _mm512_add_epi32(resl, zmm_round);
      resh = _mm512_add_epi32(resh, zmm_round);
      resl = _mm512_srai_epi32(resl, 8);
      resh = _mm512_srai_epi32(resh, 8);
      _mm512_storeu_si512((__m512i*)(dst_ptr + i),
                          _mm512_packus_epi32(resl, resh));
    }
    if (i < width) {
      __mmask32 mask = (__mmask32)((1u << (width - i)) - 1u);
      __m512i row0 = _mm512_maskz_loadu_epi16(mask, src_ptr + i);
      __m512i row1 = _mm512_maskz_loadu_epi16(mask, src_ptr1 + i);
      __m512i row0l = _mm512_unpacklo_epi16(row0, row1);
      __m512i row0h = _mm512_unpackhi_epi16(row0, row1);
      row0l = _mm512_sub_epi16(row0l, zmm_8000);
      row0h = _mm512_sub_epi16(row0h, zmm_8000);
      __m512i resl = _mm512_madd_epi16(row0l, zmm_y);
      __m512i resh = _mm512_madd_epi16(row0h, zmm_y);
      resl = _mm512_add_epi32(resl, zmm_round);
      resh = _mm512_add_epi32(resh, zmm_round);
      resl = _mm512_srai_epi32(resl, 8);
      resh = _mm512_srai_epi32(resh, 8);
      _mm512_mask_storeu_epi16(dst_ptr + i, mask,
                               _mm512_packus_epi32(resl, resh));
    }
  }
  _mm256_zeroupper();
}
#endif  // HAS_INTERPOLATEROW_16_AVX512BW

// ===========================================================================
// Module 4: NV12 / NV21 & I422Alpha to ARGB / RGB24 (AVX512BW / AVX512VBMI)
// ===========================================================================

#if defined(HAS_NV12TOARGBROW_AVX512BW) ||    \
    defined(HAS_NV21TOARGBROW_AVX512BW) ||    \
    defined(HAS_NV12TORGB24ROW_AVX512VBMI) || \
    defined(HAS_NV21TORGB24ROW_AVX512VBMI) || \
    defined(HAS_I422ALPHATOARGBROW_AVX512BW)
static const uint16_t kSplitQuadWords_Win[32] = {
    0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,
    8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15};
static const uint64_t kUnpermuteAVX512_Win[8] = {0, 4, 1, 5, 2, 6, 3, 7};
#endif

#ifdef HAS_NV12TOARGBROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void NV12ToARGBRow_AVX512BW(const uint8_t* y_buf,
                            const uint8_t* uv_buf,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  const __m512i zmm_kUVToB = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToB));
  const __m512i zmm_kUVToG = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToG));
  const __m512i zmm_kUVToR = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToR));
  const __m512i zmm_kYToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYToRgb));
  const __m512i zmm_kYBiasToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYBiasToRgb));
  const __m512i zmm_128 = _mm512_set1_epi8((char)0x80);
  const __m512i zmm_quad =
      _mm512_loadu_si512((const __m512i*)kSplitQuadWords_Win);
  const __m512i zmm_unperm =
      _mm512_loadu_si512((const __m512i*)kUnpermuteAVX512_Win);
  const __m512i zmm5 = _mm512_set1_epi32(-1);
  while (width >= 32) {
    __m512i zmm3 = _mm512_permutexvar_epi16(
        zmm_quad,
        _mm512_castsi256_si512(_mm256_loadu_si256((const __m256i*)uv_buf)));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)y_buf));
    uv_buf += 32;
    y_buf += 32;
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6),
        zmm5);
    __m512i zmm1 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6),
        zmm5);
    __m512i zmm2 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6),
        zmm5);
    __m512i bg =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm0, zmm1));
    __m512i ra =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm2, zmm5));
    _mm512_storeu_si512((__m512i*)dst_argb, _mm512_unpacklo_epi16(bg, ra));
    _mm512_storeu_si512((__m512i*)(dst_argb + 64),
                        _mm512_unpackhi_epi16(bg, ra));
    dst_argb += 128;
    width -= 32;
  }
  if (width > 0) {
    uint32_t m32 = (1u << width) - 1u;
    __mmask16 k_uv = (__mmask16)((1u << ((width + 1) >> 1)) - 1u);
    __m512i zmm3 = _mm512_permutexvar_epi16(
        zmm_quad,
        _mm512_castsi256_si512(_mm256_maskz_loadu_epi16(k_uv, uv_buf)));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8((__mmask32)m32, y_buf));
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6),
        zmm5);
    __m512i zmm1 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6),
        zmm5);
    __m512i zmm2 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6),
        zmm5);
    __m512i bg =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm0, zmm1));
    __m512i ra =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm2, zmm5));
    _mm512_mask_storeu_epi32(dst_argb, (__mmask16)m32,
                             _mm512_unpacklo_epi16(bg, ra));
    _mm512_mask_storeu_epi32(dst_argb + 64, (__mmask16)(m32 >> 16),
                             _mm512_unpackhi_epi16(bg, ra));
  }
  _mm256_zeroupper();
}
#endif  // HAS_NV12TOARGBROW_AVX512BW

#ifdef HAS_NV21TOARGBROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void NV21ToARGBRow_AVX512BW(const uint8_t* y_buf,
                            const uint8_t* vu_buf,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  const __m512i zmm_kUVToB = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToB));
  const __m512i zmm_kUVToG = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToG));
  const __m512i zmm_kUVToR = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToR));
  const __m512i zmm_kYToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYToRgb));
  const __m512i zmm_kYBiasToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYBiasToRgb));
  const __m512i zmm_128 = _mm512_set1_epi8((char)0x80);
  const __m512i zmm_quad =
      _mm512_loadu_si512((const __m512i*)kSplitQuadWords_Win);
  const __m512i zmm_unperm =
      _mm512_loadu_si512((const __m512i*)kUnpermuteAVX512_Win);
  const __m512i zmm5 = _mm512_set1_epi32(-1);
  while (width >= 32) {
    __m512i zmm3 = _mm512_permutexvar_epi16(
        zmm_quad,
        _mm512_castsi256_si512(_mm256_or_si256(
            _mm256_slli_epi16(_mm256_loadu_si256((const __m256i*)vu_buf), 8),
            _mm256_srli_epi16(_mm256_loadu_si256((const __m256i*)vu_buf), 8))));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)y_buf));
    vu_buf += 32;
    y_buf += 32;
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6),
        zmm5);
    __m512i zmm1 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6),
        zmm5);
    __m512i zmm2 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6),
        zmm5);
    __m512i bg =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm0, zmm1));
    __m512i ra =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm2, zmm5));
    _mm512_storeu_si512((__m512i*)dst_argb, _mm512_unpacklo_epi16(bg, ra));
    _mm512_storeu_si512((__m512i*)(dst_argb + 64),
                        _mm512_unpackhi_epi16(bg, ra));
    dst_argb += 128;
    width -= 32;
  }
  if (width > 0) {
    uint32_t m32 = (1u << width) - 1u;
    __mmask16 k_uv = (__mmask16)((1u << ((width + 1) >> 1)) - 1u);
    __m512i zmm3 = _mm512_permutexvar_epi16(
        zmm_quad,
        _mm512_castsi256_si512(_mm256_or_si256(
            _mm256_slli_epi16(_mm256_maskz_loadu_epi16(k_uv, vu_buf), 8),
            _mm256_srli_epi16(_mm256_maskz_loadu_epi16(k_uv, vu_buf), 8))));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8((__mmask32)m32, y_buf));
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6),
        zmm5);
    __m512i zmm1 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6),
        zmm5);
    __m512i zmm2 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6),
        zmm5);
    __m512i bg =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm0, zmm1));
    __m512i ra =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm2, zmm5));
    _mm512_mask_storeu_epi32(dst_argb, (__mmask16)m32,
                             _mm512_unpacklo_epi16(bg, ra));
    _mm512_mask_storeu_epi32(dst_argb + 64, (__mmask16)(m32 >> 16),
                             _mm512_unpackhi_epi16(bg, ra));
  }
  _mm256_zeroupper();
}
#endif  // HAS_NV21TOARGBROW_AVX512BW

#if defined(HAS_NV12TORGB24ROW_AVX512VBMI) || \
    defined(HAS_NV21TORGB24ROW_AVX512VBMI)
static const uint8_t kMaskBG_NV[64] = {
    0x00, 0x40, 0x01, 0x41, 0x02, 0x42, 0x03, 0x43, 0x04, 0x44, 0x05,
    0x45, 0x06, 0x46, 0x07, 0x47, 0x10, 0x50, 0x11, 0x51, 0x12, 0x52,
    0x13, 0x53, 0x14, 0x54, 0x15, 0x55, 0x16, 0x56, 0x17, 0x57, 0x20,
    0x60, 0x21, 0x61, 0x22, 0x62, 0x23, 0x63, 0x24, 0x64, 0x25, 0x65,
    0x26, 0x66, 0x27, 0x67, 0x30, 0x70, 0x31, 0x71, 0x32, 0x72, 0x33,
    0x73, 0x34, 0x74, 0x35, 0x75, 0x36, 0x76, 0x37, 0x77};
static const uint8_t kMaskDST0_NV[64] = {
    0x00, 0x01, 0x40, 0x02, 0x03, 0x41, 0x04, 0x05, 0x42, 0x06, 0x07,
    0x43, 0x08, 0x09, 0x44, 0x0a, 0x0b, 0x45, 0x0c, 0x0d, 0x46, 0x0e,
    0x0f, 0x47, 0x10, 0x11, 0x50, 0x12, 0x13, 0x51, 0x14, 0x15, 0x52,
    0x16, 0x17, 0x53, 0x18, 0x19, 0x54, 0x1a, 0x1b, 0x55, 0x1c, 0x1d,
    0x56, 0x1e, 0x1f, 0x57, 0x20, 0x21, 0x60, 0x22, 0x23, 0x61, 0x24,
    0x25, 0x62, 0x26, 0x27, 0x63, 0x28, 0x29, 0x64, 0x2a};
static const uint8_t kMaskDST1_NV[64] = {
    0x2b, 0x65, 0x2c, 0x2d, 0x66, 0x2e, 0x2f, 0x67, 0x30, 0x31, 0x70,
    0x32, 0x33, 0x71, 0x34, 0x35, 0x72, 0x36, 0x37, 0x73, 0x38, 0x39,
    0x74, 0x3a, 0x3b, 0x75, 0x3c, 0x3d, 0x76, 0x3e, 0x3f, 0x77, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

#ifdef HAS_NV12TORGB24ROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void NV12ToRGB24Row_AVX512VBMI(const uint8_t* y_buf,
                               const uint8_t* uv_buf,
                               uint8_t* dst_rgb24,
                               const struct YuvConstants* yuvconstants,
                               int width) {
  const __m512i zmm_kUVToB = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToB));
  const __m512i zmm_kUVToG = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToG));
  const __m512i zmm_kUVToR = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToR));
  const __m512i zmm_kYToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYToRgb));
  const __m512i zmm_kYBiasToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYBiasToRgb));
  const __m512i zmm_128 = _mm512_set1_epi8((char)0x80);
  const __m512i zmm_quad =
      _mm512_loadu_si512((const __m512i*)kSplitQuadWords_Win);
  const __m512i zmm_mask_BG = _mm512_loadu_si512((const __m512i*)kMaskBG_NV);
  const __m512i zmm_mask_DST0 =
      _mm512_loadu_si512((const __m512i*)kMaskDST0_NV);
  const __m512i zmm_mask_DST1 =
      _mm512_loadu_si512((const __m512i*)kMaskDST1_NV);
  while (width >= 32) {
    __m512i zmm3 = _mm512_permutexvar_epi16(
        zmm_quad,
        _mm512_castsi256_si512(_mm256_loadu_si256((const __m256i*)uv_buf)));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)y_buf));
    uv_buf += 32;
    y_buf += 32;
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6);
    __m512i zmm1 = _mm512_srai_epi16(
        _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6);
    __m512i zmm2 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6);
    zmm0 = _mm512_packus_epi16(zmm0, zmm0);
    zmm1 = _mm512_packus_epi16(zmm1, zmm1);
    zmm2 = _mm512_packus_epi16(zmm2, zmm2);
    __m512i zmm_BG = _mm512_permutex2var_epi8(zmm0, zmm_mask_BG, zmm1);
    __m512i zmm_dst0 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST0, zmm2);
    __m512i zmm_dst1 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST1, zmm2);
    _mm512_storeu_si512((__m512i*)dst_rgb24, zmm_dst0);
    _mm256_storeu_si256((__m256i*)(dst_rgb24 + 64),
                        _mm512_castsi512_si256(zmm_dst1));
    dst_rgb24 += 96;
    width -= 32;
  }
  if (width > 0) {
    uint32_t m32 = (1u << width) - 1u;
    __mmask16 k_uv = (__mmask16)((1u << ((width + 1) >> 1)) - 1u);
    __m512i zmm3 = _mm512_permutexvar_epi16(
        zmm_quad,
        _mm512_castsi256_si512(_mm256_maskz_loadu_epi16(k_uv, uv_buf)));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8((__mmask32)m32, y_buf));
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6);
    __m512i zmm1 = _mm512_srai_epi16(
        _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6);
    __m512i zmm2 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6);
    zmm0 = _mm512_packus_epi16(zmm0, zmm0);
    zmm1 = _mm512_packus_epi16(zmm1, zmm1);
    zmm2 = _mm512_packus_epi16(zmm2, zmm2);
    __m512i zmm_BG = _mm512_permutex2var_epi8(zmm0, zmm_mask_BG, zmm1);
    __m512i zmm_dst0 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST0, zmm2);
    __m512i zmm_dst1 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST1, zmm2);
    int dst_bytes = width * 3;
    __mmask64 k0 = dst_bytes >= 64 ? ~0ull : (1ull << dst_bytes) - 1ull;
    __mmask32 k1 = dst_bytes > 64 ? (1u << (dst_bytes - 64)) - 1u : 0u;
    _mm512_mask_storeu_epi8(dst_rgb24, k0, zmm_dst0);
    _mm256_mask_storeu_epi8(dst_rgb24 + 64, k1,
                            _mm512_castsi512_si256(zmm_dst1));
  }
  _mm256_zeroupper();
}
#endif  // HAS_NV12TORGB24ROW_AVX512VBMI

#ifdef HAS_NV21TORGB24ROW_AVX512VBMI
LIBYUV_TARGET_AVX512VBMI
void NV21ToRGB24Row_AVX512VBMI(const uint8_t* y_buf,
                               const uint8_t* vu_buf,
                               uint8_t* dst_rgb24,
                               const struct YuvConstants* yuvconstants,
                               int width) {
  const __m512i zmm_kUVToB = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToB));
  const __m512i zmm_kUVToG = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToG));
  const __m512i zmm_kUVToR = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToR));
  const __m512i zmm_kYToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYToRgb));
  const __m512i zmm_kYBiasToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYBiasToRgb));
  const __m512i zmm_128 = _mm512_set1_epi8((char)0x80);
  const __m512i zmm_quad =
      _mm512_loadu_si512((const __m512i*)kSplitQuadWords_Win);
  const __m512i zmm_mask_BG = _mm512_loadu_si512((const __m512i*)kMaskBG_NV);
  const __m512i zmm_mask_DST0 =
      _mm512_loadu_si512((const __m512i*)kMaskDST0_NV);
  const __m512i zmm_mask_DST1 =
      _mm512_loadu_si512((const __m512i*)kMaskDST1_NV);
  while (width >= 32) {
    __m512i zmm3 = _mm512_permutexvar_epi16(
        zmm_quad,
        _mm512_castsi256_si512(_mm256_or_si256(
            _mm256_slli_epi16(_mm256_loadu_si256((const __m256i*)vu_buf), 8),
            _mm256_srli_epi16(_mm256_loadu_si256((const __m256i*)vu_buf), 8))));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)y_buf));
    vu_buf += 32;
    y_buf += 32;
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6);
    __m512i zmm1 = _mm512_srai_epi16(
        _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6);
    __m512i zmm2 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6);
    zmm0 = _mm512_packus_epi16(zmm0, zmm0);
    zmm1 = _mm512_packus_epi16(zmm1, zmm1);
    zmm2 = _mm512_packus_epi16(zmm2, zmm2);
    __m512i zmm_BG = _mm512_permutex2var_epi8(zmm0, zmm_mask_BG, zmm1);
    __m512i zmm_dst0 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST0, zmm2);
    __m512i zmm_dst1 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST1, zmm2);
    _mm512_storeu_si512((__m512i*)dst_rgb24, zmm_dst0);
    _mm256_storeu_si256((__m256i*)(dst_rgb24 + 64),
                        _mm512_castsi512_si256(zmm_dst1));
    dst_rgb24 += 96;
    width -= 32;
  }
  if (width > 0) {
    uint32_t m32 = (1u << width) - 1u;
    __mmask16 k_uv = (__mmask16)((1u << ((width + 1) >> 1)) - 1u);
    __m512i zmm3 = _mm512_permutexvar_epi16(
        zmm_quad,
        _mm512_castsi256_si512(_mm256_or_si256(
            _mm256_slli_epi16(_mm256_maskz_loadu_epi16(k_uv, vu_buf), 8),
            _mm256_srli_epi16(_mm256_maskz_loadu_epi16(k_uv, vu_buf), 8))));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8((__mmask32)m32, y_buf));
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6);
    __m512i zmm1 = _mm512_srai_epi16(
        _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6);
    __m512i zmm2 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6);
    zmm0 = _mm512_packus_epi16(zmm0, zmm0);
    zmm1 = _mm512_packus_epi16(zmm1, zmm1);
    zmm2 = _mm512_packus_epi16(zmm2, zmm2);
    __m512i zmm_BG = _mm512_permutex2var_epi8(zmm0, zmm_mask_BG, zmm1);
    __m512i zmm_dst0 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST0, zmm2);
    __m512i zmm_dst1 = _mm512_permutex2var_epi8(zmm_BG, zmm_mask_DST1, zmm2);
    int dst_bytes = width * 3;
    __mmask64 k0 = dst_bytes >= 64 ? ~0ull : (1ull << dst_bytes) - 1ull;
    __mmask32 k1 = dst_bytes > 64 ? (1u << (dst_bytes - 64)) - 1u : 0u;
    _mm512_mask_storeu_epi8(dst_rgb24, k0, zmm_dst0);
    _mm256_mask_storeu_epi8(dst_rgb24 + 64, k1,
                            _mm512_castsi512_si256(zmm_dst1));
  }
  _mm256_zeroupper();
}
#endif  // HAS_NV21TORGB24ROW_AVX512VBMI

#ifdef HAS_I422ALPHATOARGBROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void I422AlphaToARGBRow_AVX512BW(const uint8_t* y_buf,
                                 const uint8_t* u_buf,
                                 const uint8_t* v_buf,
                                 const uint8_t* a_buf,
                                 uint8_t* dst_argb,
                                 const struct YuvConstants* yuvconstants,
                                 int width) {
  const __m512i zmm_kUVToB = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToB));
  const __m512i zmm_kUVToG = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToG));
  const __m512i zmm_kUVToR = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kUVToR));
  const __m512i zmm_kYToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYToRgb));
  const __m512i zmm_kYBiasToRgb = _mm512_broadcast_i32x4(
      _mm_loadu_si128((const __m128i*)yuvconstants->kYBiasToRgb));
  const __m512i zmm_128 = _mm512_set1_epi8((char)0x80);
  const __m512i zmm_quad =
      _mm512_loadu_si512((const __m512i*)kSplitQuadWords_Win);
  const __m512i zmm_unperm =
      _mm512_loadu_si512((const __m512i*)kUnpermuteAVX512_Win);
  ptrdiff_t offset = v_buf - u_buf;
  while (width >= 32) {
    __m128i xu = _mm_loadu_si128((const __m128i*)u_buf);
    __m128i xv = _mm_loadu_si128((const __m128i*)(u_buf + offset));
    u_buf += 16;
    __m256i yuv = _mm256_inserti128_si256(
        _mm256_castsi128_si256(_mm_unpacklo_epi8(xu, xv)),
        _mm_unpackhi_epi8(xu, xv), 1);
    __m512i zmm3 =
        _mm512_permutexvar_epi16(zmm_quad, _mm512_castsi256_si512(yuv));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)y_buf));
    __m512i zmm5 = _mm512_packus_epi16(
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)a_buf)),
        _mm512_setzero_si512());
    y_buf += 32;
    a_buf += 32;
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6),
        zmm5);
    __m512i zmm1 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6),
        zmm5);
    __m512i zmm2 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6),
        zmm5);
    __m512i bg =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm0, zmm1));
    __m512i ra =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm2, zmm5));
    _mm512_storeu_si512((__m512i*)dst_argb, _mm512_unpacklo_epi16(bg, ra));
    _mm512_storeu_si512((__m512i*)(dst_argb + 64),
                        _mm512_unpackhi_epi16(bg, ra));
    dst_argb += 128;
    width -= 32;
  }
  if (width > 0) {
    uint32_t m32 = (1u << width) - 1u;
    __mmask16 k_uv = (__mmask16)((1u << ((width + 1) >> 1)) - 1u);
    __m128i xu = _mm_maskz_loadu_epi8(k_uv, u_buf);
    __m128i xv = _mm_maskz_loadu_epi8(k_uv, u_buf + offset);
    __m256i yuv = _mm256_inserti128_si256(
        _mm256_castsi128_si256(_mm_unpacklo_epi8(xu, xv)),
        _mm_unpackhi_epi8(xu, xv), 1);
    __m512i zmm3 =
        _mm512_permutexvar_epi16(zmm_quad, _mm512_castsi256_si512(yuv));
    __m512i zmm4 =
        _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8((__mmask32)m32, y_buf));
    __m512i zmm5 = _mm512_packus_epi16(
        _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8((__mmask32)m32, a_buf)),
        _mm512_setzero_si512());
    zmm4 = _mm512_or_si512(zmm4, _mm512_slli_epi16(zmm4, 8));
    zmm3 = _mm512_sub_epi8(zmm3, zmm_128);
    zmm4 = _mm512_add_epi16(_mm512_mulhi_epu16(zmm4, zmm_kYToRgb),
                            zmm_kYBiasToRgb);
    __m512i zmm0 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToB, zmm3), zmm4), 6),
        zmm5);
    __m512i zmm1 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_subs_epi16(zmm4, _mm512_maddubs_epi16(zmm_kUVToG, zmm3)), 6),
        zmm5);
    __m512i zmm2 = _mm512_packus_epi16(
        _mm512_srai_epi16(
            _mm512_adds_epi16(_mm512_maddubs_epi16(zmm_kUVToR, zmm3), zmm4), 6),
        zmm5);
    __m512i bg =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm0, zmm1));
    __m512i ra =
        _mm512_permutexvar_epi64(zmm_unperm, _mm512_unpacklo_epi8(zmm2, zmm5));
    _mm512_mask_storeu_epi32(dst_argb, (__mmask16)m32,
                             _mm512_unpacklo_epi16(bg, ra));
    _mm512_mask_storeu_epi32(dst_argb + 64, (__mmask16)(m32 >> 16),
                             _mm512_unpackhi_epi16(bg, ra));
  }
  _mm256_zeroupper();
}
#endif  // HAS_I422ALPHATOARGBROW_AVX512BW

// ===========================================================================
// Module 5: I400ToARGB & ARGB Shaders (Attenuate, Multiply, Gray, Blend)
// ===========================================================================

#ifdef HAS_I400TOARGBROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void I400ToARGBRow_AVX512BW(const uint8_t* y_buf,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  static const uint64_t kLinearizePackQ[8] = {0, 2, 4, 6, 1, 3, 5, 7};
  static const uint8_t kBcastGrayByte[16] = {0, 0, 0, 0x80, 4,  4,  4,  0x80,
                                             8, 8, 8, 0x80, 12, 12, 12, 0x80};
  const __m512i zmm_yg =
      _mm512_set1_epi16(*(const int16_t*)yuvconstants->kYToRgb);
  const __m512i zmm_ygb =
      _mm512_set1_epi16(*(const int16_t*)yuvconstants->kYBiasToRgb);
  const __m512i zmm_lin = _mm512_loadu_si512((const __m512i*)kLinearizePackQ);
  const __m512i zmm_bcast =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)kBcastGrayByte));
  const __m512i zmm_alpha = _mm512_set1_epi32((int)0xff000000u);
  while (width >= 32) {
    __m512i y16 =
        _mm512_cvtepu8_epi16(_mm256_loadu_si256((const __m256i*)y_buf));
    y_buf += 32;
    y16 = _mm512_or_si512(y16, _mm512_slli_epi16(y16, 8));
    y16 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_mulhi_epu16(y16, zmm_yg), zmm_ygb), 6);
    __m512i g =
        _mm512_permutexvar_epi64(zmm_lin, _mm512_packus_epi16(y16, y16));
    __m256i g256 = _mm512_castsi512_si256(g);
    __m512i d0 = _mm512_or_si512(
        _mm512_shuffle_epi8(_mm512_cvtepu8_epi32(_mm256_castsi256_si128(g256)),
                            zmm_bcast),
        zmm_alpha);
    __m512i d1 = _mm512_or_si512(
        _mm512_shuffle_epi8(
            _mm512_cvtepu8_epi32(_mm256_extracti128_si256(g256, 1)), zmm_bcast),
        zmm_alpha);
    _mm512_storeu_si512((__m512i*)dst_argb, d0);
    _mm512_storeu_si512((__m512i*)(dst_argb + 64), d1);
    dst_argb += 128;
    width -= 32;
  }
  if (width > 0) {
    uint32_t m32 = (1u << width) - 1u;
    __m512i y16 =
        _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8((__mmask32)m32, y_buf));
    y16 = _mm512_or_si512(y16, _mm512_slli_epi16(y16, 8));
    y16 = _mm512_srai_epi16(
        _mm512_adds_epi16(_mm512_mulhi_epu16(y16, zmm_yg), zmm_ygb), 6);
    __m512i g =
        _mm512_permutexvar_epi64(zmm_lin, _mm512_packus_epi16(y16, y16));
    __m256i g256 = _mm512_castsi512_si256(g);
    __m512i d0 = _mm512_or_si512(
        _mm512_shuffle_epi8(_mm512_cvtepu8_epi32(_mm256_castsi256_si128(g256)),
                            zmm_bcast),
        zmm_alpha);
    __m512i d1 = _mm512_or_si512(
        _mm512_shuffle_epi8(
            _mm512_cvtepu8_epi32(_mm256_extracti128_si256(g256, 1)), zmm_bcast),
        zmm_alpha);
    _mm512_mask_storeu_epi32(dst_argb, (__mmask16)m32, d0);
    _mm512_mask_storeu_epi32(dst_argb + 64, (__mmask16)(m32 >> 16), d1);
  }
  _mm256_zeroupper();
}
#endif  // HAS_I400TOARGBROW_AVX512BW

#ifdef HAS_ARGBATTENUATEROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBAttenuateRow_AVX512BW(const uint8_t* src_argb,
                               uint8_t* dst_argb,
                               int width) {
  static const int8_t kAttenShuf16[16] = {6,    -128, 6,    -128, 6,  -128,
                                          -128, -128, 14,   -128, 14, -128,
                                          14,   -128, -128, -128};
  const __m512i zmm_shuf =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)kAttenShuf16));
  const __m512i zmm_alpha = _mm512_set1_epi32((int)0xff000000u);
  const __m512i zmm_bias = _mm512_set1_epi16(0x00ff);
  while (width >= 16) {
    __m512i s = _mm512_loadu_si512((const __m512i*)src_argb);
    src_argb += 64;
    __m512i lo = _mm512_unpacklo_epi8(s, zmm_alpha);
    __m512i hi = _mm512_unpackhi_epi8(s, zmm_alpha);
    lo = _mm512_srli_epi16(
        _mm512_add_epi16(
            _mm512_mullo_epi16(lo, _mm512_shuffle_epi8(lo, zmm_shuf)),
            zmm_bias),
        8);
    hi = _mm512_srli_epi16(
        _mm512_add_epi16(
            _mm512_mullo_epi16(hi, _mm512_shuffle_epi8(hi, zmm_shuf)),
            zmm_bias),
        8);
    __m512i out = _mm512_or_si512(_mm512_packus_epi16(lo, hi),
                                  _mm512_and_si512(s, zmm_alpha));
    _mm512_storeu_si512((__m512i*)dst_argb, out);
    dst_argb += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i s = _mm512_maskz_loadu_epi32(k, src_argb);
    __m512i lo = _mm512_unpacklo_epi8(s, zmm_alpha);
    __m512i hi = _mm512_unpackhi_epi8(s, zmm_alpha);
    lo = _mm512_srli_epi16(
        _mm512_add_epi16(
            _mm512_mullo_epi16(lo, _mm512_shuffle_epi8(lo, zmm_shuf)),
            zmm_bias),
        8);
    hi = _mm512_srli_epi16(
        _mm512_add_epi16(
            _mm512_mullo_epi16(hi, _mm512_shuffle_epi8(hi, zmm_shuf)),
            zmm_bias),
        8);
    __m512i out = _mm512_or_si512(_mm512_packus_epi16(lo, hi),
                                  _mm512_and_si512(s, zmm_alpha));
    _mm512_mask_storeu_epi32(dst_argb, k, out);
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBATTENUATEROW_AVX512BW

#ifdef HAS_ARGBMULTIPLYROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBMultiplyRow_AVX512BW(const uint8_t* src_argb,
                              const uint8_t* src_argb1,
                              uint8_t* dst_argb,
                              int width) {
  const __m512i zero = _mm512_setzero_si512();
  while (width >= 16) {
    __m512i s0 = _mm512_loadu_si512((const __m512i*)src_argb);
    __m512i s1 = _mm512_loadu_si512((const __m512i*)src_argb1);
    src_argb += 64;
    src_argb1 += 64;
    __m512i lo = _mm512_mulhrs_epi16(
        _mm512_srli_epi16(_mm512_unpacklo_epi8(zero, s0), 1),
        _mm512_unpacklo_epi8(s1, zero));
    __m512i hi = _mm512_mulhrs_epi16(
        _mm512_srli_epi16(_mm512_unpackhi_epi8(zero, s0), 1),
        _mm512_unpackhi_epi8(s1, zero));
    _mm512_storeu_si512((__m512i*)dst_argb, _mm512_packus_epi16(lo, hi));
    dst_argb += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i s0 = _mm512_maskz_loadu_epi32(k, src_argb);
    __m512i s1 = _mm512_maskz_loadu_epi32(k, src_argb1);
    __m512i lo = _mm512_mulhrs_epi16(
        _mm512_srli_epi16(_mm512_unpacklo_epi8(zero, s0), 1),
        _mm512_unpacklo_epi8(s1, zero));
    __m512i hi = _mm512_mulhrs_epi16(
        _mm512_srli_epi16(_mm512_unpackhi_epi8(zero, s0), 1),
        _mm512_unpackhi_epi8(s1, zero));
    _mm512_mask_storeu_epi32(dst_argb, k, _mm512_packus_epi16(lo, hi));
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBMULTIPLYROW_AVX512BW

#ifdef HAS_ARGBGRAYROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBGrayRow_AVX512BW(const uint8_t* src_argb,
                          uint8_t* dst_argb,
                          int width) {
  static const uint8_t kShufGrayDword[16] = {0, 0, 0, 0x80, 4,  4,  4,  0x80,
                                             8, 8, 8, 0x80, 12, 12, 12, 0x80};
  const __m512i c_coeff = _mm512_set1_epi32(0x004d961d);
  const __m512i c_sub128 = _mm512_set1_epi8((char)0x80);
  const __m512i c_one16 = _mm512_set1_epi16(1);
  const __m512i c_bias = _mm512_set1_epi32(32896);
  const __m512i c_shuf =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)kShufGrayDword));
  const __m512i c_alpha = _mm512_set1_epi32((int)0xff000000u);
  while (width >= 16) {
    __m512i s = _mm512_loadu_si512((const __m512i*)src_argb);
    src_argb += 64;
    __m512i y16 = _mm512_maddubs_epi16(c_coeff, _mm512_sub_epi8(s, c_sub128));
    __m512i y32 = _mm512_srli_epi32(
        _mm512_add_epi32(_mm512_madd_epi16(y16, c_one16), c_bias), 8);
    __m512i out = _mm512_or_si512(_mm512_and_si512(s, c_alpha),
                                  _mm512_shuffle_epi8(y32, c_shuf));
    _mm512_storeu_si512((__m512i*)dst_argb, out);
    dst_argb += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i s = _mm512_maskz_loadu_epi32(k, src_argb);
    __m512i y16 = _mm512_maddubs_epi16(c_coeff, _mm512_sub_epi8(s, c_sub128));
    __m512i y32 = _mm512_srli_epi32(
        _mm512_add_epi32(_mm512_madd_epi16(y16, c_one16), c_bias), 8);
    __m512i out = _mm512_or_si512(_mm512_and_si512(s, c_alpha),
                                  _mm512_shuffle_epi8(y32, c_shuf));
    _mm512_mask_storeu_epi32(dst_argb, k, out);
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBGRAYROW_AVX512BW

#ifdef HAS_ARGBBLENDROW_AVX512BW
LIBYUV_TARGET_AVX512BW
void ARGBBlendRow_AVX512BW(const uint8_t* src_argb,
                           const uint8_t* src_argb1,
                           uint8_t* dst_argb,
                           int width) {
  static const uint8_t kShuffleAlpha16[16] = {3u,  0x80, 3u,  0x80, 7u,  0x80,
                                              7u,  0x80, 11u, 0x80, 11u, 0x80,
                                              15u, 0x80, 15u, 0x80};
  const __m512i zmm_shuf =
      _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)kShuffleAlpha16));
  const __m512i zmm_alpha = _mm512_set1_epi32((int)0xff000000u);
  const __m512i zmm_ff00 = _mm512_set1_epi16((short)0xff00u);
  const __m512i zmm_00ff = _mm512_set1_epi16(0x00ff);
  const __m512i zmm_0001 = _mm512_set1_epi16(0x0001);
  while (width >= 16) {
    __m512i s0 = _mm512_loadu_si512((const __m512i*)src_argb);
    __m512i s1 = _mm512_loadu_si512((const __m512i*)src_argb1);
    src_argb += 64;
    src_argb1 += 64;
    __m512i inv_a = _mm512_add_epi16(
        _mm512_shuffle_epi8(_mm512_xor_si512(s0, zmm_alpha), zmm_shuf),
        zmm_0001);
    __m512i even = _mm512_srli_epi16(
        _mm512_mullo_epi16(_mm512_and_si512(s1, zmm_00ff), inv_a), 8);
    __m512i odd = _mm512_and_si512(
        _mm512_mullo_epi16(_mm512_srli_epi16(s1, 8), inv_a), zmm_ff00);
    __m512i out = _mm512_adds_epu8(
        _mm512_adds_epu8(_mm512_or_si512(s0, zmm_alpha), even), odd);
    _mm512_storeu_si512((__m512i*)dst_argb, out);
    dst_argb += 64;
    width -= 16;
  }
  if (width > 0) {
    __mmask16 k = (__mmask16)((1u << width) - 1u);
    __m512i s0 = _mm512_maskz_loadu_epi32(k, src_argb);
    __m512i s1 = _mm512_maskz_loadu_epi32(k, src_argb1);
    __m512i inv_a = _mm512_add_epi16(
        _mm512_shuffle_epi8(_mm512_xor_si512(s0, zmm_alpha), zmm_shuf),
        zmm_0001);
    __m512i even = _mm512_srli_epi16(
        _mm512_mullo_epi16(_mm512_and_si512(s1, zmm_00ff), inv_a), 8);
    __m512i odd = _mm512_and_si512(
        _mm512_mullo_epi16(_mm512_srli_epi16(s1, 8), inv_a), zmm_ff00);
    __m512i out = _mm512_adds_epu8(
        _mm512_adds_epu8(_mm512_or_si512(s0, zmm_alpha), even), odd);
    _mm512_mask_storeu_epi32(dst_argb, k, out);
  }
  _mm256_zeroupper();
}
#endif  // HAS_ARGBBLENDROW_AVX512BW

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_X86) && (defined(__x86_64__) ||
        // defined(__i386__) || defined(_M_X64) || defined(_M_IX86)) &&
        // ((defined(_MSC_VER) && !defined(__clang__)) ||
        // defined(LIBYUV_ENABLE_ROWWIN))

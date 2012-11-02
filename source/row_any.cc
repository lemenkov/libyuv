/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"

#include <string.h>  // For memcpy

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// YUV to RGB does multiple of 8 with SIMD and remainder with C.
#define YANY(NAMEANY, I420TORGB_SIMD, I420TORGB_C, UV_SHIFT, BPP, MASK)        \
    void NAMEANY(const uint8* y_buf,                                           \
                 const uint8* u_buf,                                           \
                 const uint8* v_buf,                                           \
                 uint8* rgb_buf,                                               \
                 int width) {                                                  \
      int n = width & ~MASK;                                                   \
      I420TORGB_SIMD(y_buf, u_buf, v_buf, rgb_buf, n);                         \
      I420TORGB_C(y_buf + n,                                                   \
                  u_buf + (n >> UV_SHIFT),                                     \
                  v_buf + (n >> UV_SHIFT),                                     \
                  rgb_buf + n * BPP, width & MASK);                            \
    }

#ifdef HAS_I422TOARGBROW_SSSE3
YANY(I444ToARGBRow_Any_SSSE3, I444ToARGBRow_Unaligned_SSSE3, I444ToARGBRow_C,
     0, 4, 7)
YANY(I422ToARGBRow_Any_SSSE3, I422ToARGBRow_Unaligned_SSSE3, I422ToARGBRow_C,
     1, 4, 7)
YANY(I411ToARGBRow_Any_SSSE3, I411ToARGBRow_Unaligned_SSSE3, I411ToARGBRow_C,
     2, 4, 7)
YANY(I422ToBGRARow_Any_SSSE3, I422ToBGRARow_Unaligned_SSSE3, I422ToBGRARow_C,
     1, 4, 7)
YANY(I422ToABGRRow_Any_SSSE3, I422ToABGRRow_Unaligned_SSSE3, I422ToABGRRow_C,
     1, 4, 7)
YANY(I422ToRGBARow_Any_SSSE3, I422ToRGBARow_Unaligned_SSSE3, I422ToRGBARow_C,
     1, 4, 7)
// I422ToRGB565Row_SSSE3 is unaligned.
YANY(I422ToARGB4444Row_Any_SSSE3, I422ToARGB4444Row_SSSE3, I422ToARGB4444Row_C,
     1, 2, 7)
YANY(I422ToARGB1555Row_Any_SSSE3, I422ToARGB1555Row_SSSE3, I422ToARGB1555Row_C,
     1, 2, 7)
YANY(I422ToRGB565Row_Any_SSSE3, I422ToRGB565Row_SSSE3, I422ToRGB565Row_C,
     1, 2, 7)
// I422ToRGB24Row_SSSE3 is unaligned.
YANY(I422ToRGB24Row_Any_SSSE3, I422ToRGB24Row_SSSE3, I422ToRGB24Row_C, 1, 3, 7)
YANY(I422ToRAWRow_Any_SSSE3, I422ToRAWRow_SSSE3, I422ToRAWRow_C, 1, 3, 7)
YANY(I422ToYUY2Row_Any_SSE2, I422ToYUY2Row_SSE2, I422ToYUY2Row_C, 1, 2, 15)
YANY(I422ToUYVYRow_Any_SSE2, I422ToUYVYRow_SSE2, I422ToUYVYRow_C, 1, 2, 15)
#endif  // HAS_I422TOARGBROW_SSSE3
#ifdef HAS_I422TOARGBROW_NEON
YANY(I422ToARGBRow_Any_NEON, I422ToARGBRow_NEON, I422ToARGBRow_C, 1, 4, 7)
YANY(I422ToBGRARow_Any_NEON, I422ToBGRARow_NEON, I422ToBGRARow_C, 1, 4, 7)
YANY(I422ToABGRRow_Any_NEON, I422ToABGRRow_NEON, I422ToABGRRow_C, 1, 4, 7)
YANY(I422ToRGBARow_Any_NEON, I422ToRGBARow_NEON, I422ToRGBARow_C, 1, 4, 7)
YANY(I422ToRGB24Row_Any_NEON, I422ToRGB24Row_NEON, I422ToRGB24Row_C, 1, 3, 7)
YANY(I422ToRAWRow_Any_NEON, I422ToRAWRow_NEON, I422ToRAWRow_C, 1, 3, 7)
YANY(I422ToARGB4444Row_Any_NEON, I422ToARGB4444Row_NEON, I422ToARGB4444Row_C,
     1, 2, 7)
YANY(I422ToARGB1555Row_Any_NEON, I422ToARGB1555Row_NEON, I422ToARGB1555Row_C,
     1, 2, 7)
YANY(I422ToRGB565Row_Any_NEON, I422ToRGB565Row_NEON, I422ToRGB565Row_C, 1, 2, 7)
YANY(I422ToYUY2Row_Any_NEON, I422ToYUY2Row_NEON, I422ToYUY2Row_C, 1, 2, 15)
YANY(I422ToUYVYRow_Any_NEON, I422ToUYVYRow_NEON, I422ToUYVYRow_C, 1, 2, 15)
#endif  // HAS_I422TOARGBROW_NEON
#undef YANY

// Wrappers to handle odd width
#define NV2NY(NAMEANY, NV12TORGB_SIMD, NV12TORGB_C, UV_SHIFT, BPP)             \
    void NAMEANY(const uint8* y_buf,                                           \
                 const uint8* uv_buf,                                          \
                 uint8* rgb_buf,                                               \
                 int width) {                                                  \
      int n = width & ~7;                                                      \
      NV12TORGB_SIMD(y_buf, uv_buf, rgb_buf, n);                               \
      NV12TORGB_C(y_buf + n,                                                   \
                  uv_buf + (n >> UV_SHIFT),                                    \
                  rgb_buf + n * BPP, width & 7);                               \
    }

#ifdef HAS_NV12TOARGBROW_SSSE3
NV2NY(NV12ToARGBRow_Any_SSSE3, NV12ToARGBRow_Unaligned_SSSE3, NV12ToARGBRow_C,
      0, 4)
NV2NY(NV21ToARGBRow_Any_SSSE3, NV21ToARGBRow_Unaligned_SSSE3, NV21ToARGBRow_C,
      0, 4)
#endif  // HAS_NV12TOARGBROW_SSSE3
#ifdef HAS_NV12TOARGBROW_NEON
NV2NY(NV12ToARGBRow_Any_NEON, NV12ToARGBRow_NEON, NV12ToARGBRow_C, 0, 4)
NV2NY(NV21ToARGBRow_Any_NEON, NV21ToARGBRow_NEON, NV21ToARGBRow_C, 0, 4)
#endif  // HAS_NV12TOARGBROW_NEON
#ifdef HAS_NV12TORGB565ROW_SSSE3
NV2NY(NV12ToRGB565Row_Any_SSSE3, NV12ToRGB565Row_SSSE3, NV12ToRGB565Row_C,
      0, 2)
NV2NY(NV21ToRGB565Row_Any_SSSE3, NV21ToRGB565Row_SSSE3, NV21ToRGB565Row_C,
      0, 2)
#endif  // HAS_NV12TORGB565ROW_SSSE3
#ifdef HAS_NV12TORGB565ROW_NEON
NV2NY(NV12ToRGB565Row_Any_NEON, NV12ToRGB565Row_NEON, NV12ToRGB565Row_C, 0, 2)
NV2NY(NV21ToRGB565Row_Any_NEON, NV21ToRGB565Row_NEON, NV21ToRGB565Row_C, 0, 2)
#endif  // HAS_NV12TORGB565ROW_NEON
#undef NVANY

// RGB to RGB does multiple of 16 pixels with SIMD and remainder with C.
// SSSE3 RGB24 is multiple of 16 pixels, aligned source and destination.
// SSE2 RGB565 is multiple of 4 pixels, ARGB must be aligned to 16 bytes.
// NEON RGB24 is multiple of 8 pixels, unaligned source and destination.
// I400 To ARGB does multiple of 8 pixels with SIMD and remainder with C.
#define RGBANY(NAMEANY, ARGBTORGB_SIMD, ARGBTORGB_C, MASK, SBPP, BPP)          \
    void NAMEANY(const uint8* argb_buf,                                        \
                 uint8* rgb_buf,                                               \
                 int width) {                                                  \
      int n = width & ~MASK;                                                   \
      ARGBTORGB_SIMD(argb_buf, rgb_buf, n);                                    \
      ARGBTORGB_C(argb_buf + n * SBPP, rgb_buf + n * BPP, width & MASK);       \
    }

#if defined(HAS_ARGBTORGB24ROW_SSSE3)
RGBANY(ARGBToRGB24Row_Any_SSSE3, ARGBToRGB24Row_SSSE3, ARGBToRGB24Row_C,
       15, 4, 3)
RGBANY(ARGBToRAWRow_Any_SSSE3, ARGBToRAWRow_SSSE3, ARGBToRAWRow_C,
       15, 4, 3)
RGBANY(ARGBToRGB565Row_Any_SSE2, ARGBToRGB565Row_SSE2, ARGBToRGB565Row_C,
       3, 4, 2)
RGBANY(ARGBToARGB1555Row_Any_SSE2, ARGBToARGB1555Row_SSE2, ARGBToARGB1555Row_C,
       3, 4, 2)
RGBANY(ARGBToARGB4444Row_Any_SSE2, ARGBToARGB4444Row_SSE2, ARGBToARGB4444Row_C,
       3, 4, 2)
RGBANY(I400ToARGBRow_Any_SSE2, I400ToARGBRow_Unaligned_SSE2, I400ToARGBRow_C,
       7, 1, 4)
#endif
#if defined(HAS_ARGBTORGB24ROW_NEON)
RGBANY(ARGBToRGB24Row_Any_NEON, ARGBToRGB24Row_NEON, ARGBToRGB24Row_C, 7, 4, 3)
RGBANY(ARGBToRAWRow_Any_NEON, ARGBToRAWRow_NEON, ARGBToRAWRow_C, 7, 4, 3)
RGBANY(ARGBToRGB565Row_Any_NEON, ARGBToRGB565Row_NEON, ARGBToRGB565Row_C,
       7, 4, 2)
RGBANY(ARGBToARGB1555Row_Any_NEON, ARGBToARGB1555Row_NEON, ARGBToARGB1555Row_C,
       7, 4, 2)
RGBANY(ARGBToARGB4444Row_Any_NEON, ARGBToARGB4444Row_NEON, ARGBToARGB4444Row_C,
       7, 4, 2)
RGBANY(I400ToARGBRow_Any_NEON, I400ToARGBRow_NEON, I400ToARGBRow_C,
       7, 1, 4)
#endif
#undef RGBANY

// RGB/YUV to Y does multiple of 16 with SIMD and last 16 with SIMD.
#define YANY(NAMEANY, ARGBTOY_SIMD, BPP, NUM)                                  \
    void NAMEANY(const uint8* src_argb, uint8* dst_y, int width) {             \
      ARGBTOY_SIMD(src_argb, dst_y, width - NUM);                              \
      ARGBTOY_SIMD(src_argb + (width - NUM) * BPP, dst_y + (width - NUM), NUM);\
    }

#ifdef HAS_ARGBTOYROW_SSSE3
YANY(ARGBToYRow_Any_SSSE3, ARGBToYRow_Unaligned_SSSE3, 4, 16)
YANY(BGRAToYRow_Any_SSSE3, BGRAToYRow_Unaligned_SSSE3, 4, 16)
YANY(ABGRToYRow_Any_SSSE3, ABGRToYRow_Unaligned_SSSE3, 4, 16)
#endif
#ifdef HAS_RGBATOYROW_SSSE3
YANY(RGBAToYRow_Any_SSSE3, RGBAToYRow_Unaligned_SSSE3, 4, 16)
#endif
#ifdef HAS_ARGBTOYROW_NEON
YANY(ARGBToYRow_Any_NEON, ARGBToYRow_NEON, 4, 8)
#endif
#ifdef HAS_YUY2TOYROW_SSE2
YANY(YUY2ToYRow_Any_SSE2, YUY2ToYRow_Unaligned_SSE2, 2, 16)
YANY(UYVYToYRow_Any_SSE2, UYVYToYRow_Unaligned_SSE2, 2, 16)
#endif
#ifdef HAS_YUY2TOYROW_NEON
YANY(YUY2ToYRow_Any_NEON, YUY2ToYRow_NEON, 2, 16)
YANY(UYVYToYRow_Any_NEON, UYVYToYRow_NEON, 2, 16)
#endif
#undef YANY

// RGB/YUV to UV does multiple of 16 with SIMD and remainder with C.
#define UVANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, BPP)                           \
    void NAMEANY(const uint8* src_argb, int src_stride_argb,                   \
                 uint8* dst_u, uint8* dst_v, int width) {                      \
      int n = width & ~15;                                                     \
      ANYTOUV_SIMD(src_argb, src_stride_argb, dst_u, dst_v, n);                \
      ANYTOUV_C(src_argb  + n * BPP, src_stride_argb,                          \
                 dst_u + (n >> 1),                                             \
                 dst_v + (n >> 1),                                             \
                 width & 15);                                                  \
    }

#ifdef HAS_ARGBTOUVROW_SSSE3
UVANY(ARGBToUVRow_Any_SSSE3, ARGBToUVRow_Unaligned_SSSE3, ARGBToUVRow_C, 4)
UVANY(BGRAToUVRow_Any_SSSE3, BGRAToUVRow_Unaligned_SSSE3, BGRAToUVRow_C, 4)
UVANY(ABGRToUVRow_Any_SSSE3, ABGRToUVRow_Unaligned_SSSE3, ABGRToUVRow_C, 4)
#endif
#ifdef HAS_RGBATOYROW_SSSE3
UVANY(RGBAToUVRow_Any_SSSE3, RGBAToUVRow_Unaligned_SSSE3, RGBAToUVRow_C, 4)
#endif
#ifdef HAS_YUY2TOUVROW_SSE2
UVANY(YUY2ToUVRow_Any_SSE2, YUY2ToUVRow_Unaligned_SSE2, YUY2ToUVRow_C, 2)
UVANY(UYVYToUVRow_Any_SSE2, UYVYToUVRow_Unaligned_SSE2, UYVYToUVRow_C, 2)
#endif
#ifdef HAS_YUY2TOUVROW_NEON
UVANY(YUY2ToUVRow_Any_NEON, YUY2ToUVRow_NEON, YUY2ToUVRow_C, 2)
UVANY(UYVYToUVRow_Any_NEON, UYVYToUVRow_NEON, UYVYToUVRow_C, 2)
#endif
#undef UVANY

#define UV422ANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, BPP)                        \
    void NAMEANY(const uint8* src_uv,                                          \
                 uint8* dst_u, uint8* dst_v, int width) {                      \
      int n = width & ~15;                                                     \
      ANYTOUV_SIMD(src_uv, dst_u, dst_v, n);                                   \
      ANYTOUV_C(src_uv  + n * BPP,                                             \
                 dst_u + (n >> 1),                                             \
                 dst_v + (n >> 1),                                             \
                 width & 15);                                                  \
    }

#ifdef HAS_YUY2TOUV422ROW_SSE2
UV422ANY(YUY2ToUV422Row_Any_SSE2, YUY2ToUV422Row_Unaligned_SSE2,
         YUY2ToUV422Row_C, 2)
UV422ANY(UYVYToUV422Row_Any_SSE2, UYVYToUV422Row_Unaligned_SSE2,
         UYVYToUV422Row_C, 2)
#endif
#ifdef HAS_YUY2TOUV422ROW_NEON
UV422ANY(YUY2ToUV422Row_Any_NEON, YUY2ToUV422Row_NEON,
         YUY2ToUV422Row_C, 2)
UV422ANY(UYVYToUV422Row_Any_NEON, UYVYToUV422Row_NEON,
         UYVYToUV422Row_C, 2)
#endif
#undef UV422ANY

#define SPLITUVANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, MASK)                     \
    void NAMEANY(const uint8* src_uv,                                          \
                 uint8* dst_u, uint8* dst_v, int width) {                      \
      int n = width & ~MASK;                                                   \
      ANYTOUV_SIMD(src_uv, dst_u, dst_v, n);                                   \
      ANYTOUV_C(src_uv + n * 2,                                                \
                dst_u + n,                                                     \
                dst_v + n,                                                     \
                width & MASK);                                                 \
    }

#ifdef HAS_SPLITUV_SSE2
SPLITUVANY(SplitUV_Any_SSE2, SplitUV_Unaligned_SSE2, SplitUV_C, 15)
#endif
#ifdef HAS_SPLITUV_AVX2
SPLITUVANY(SplitUV_Any_AVX2, SplitUV_Unaligned_AVX2, SplitUV_C, 31)
#endif
#ifdef HAS_SPLITUV_NEON
SPLITUVANY(SplitUV_Any_NEON, SplitUV_Unaligned_NEON, SplitUV_C, 15)
#endif
#ifdef HAS_SPLITUV_MIPS_DSPR2
SPLITUVANY(SplitUV_Any_MIPS_DSPR2, SplitUV_Unaligned_MIPS_DSPR2, SplitUV_C, 15)
#endif
#undef SPLITUVANY

#define MERGEUVANY(NAMEANY, ANYTOUV_SIMD, ANYTOUV_C, MASK)                     \
    void NAMEANY(const uint8* src_u, const uint8* src_v,                       \
                 uint8* dst_uv, int width) {                                   \
      int n = width & ~MASK;                                                   \
      ANYTOUV_SIMD(src_u, src_v, dst_uv, n);                                   \
      ANYTOUV_C(src_u + n,                                                     \
                src_v + n,                                                     \
                dst_uv + n * 2,                                                \
                width & MASK);                                                 \
    }

#ifdef HAS_MERGEUV_SSE2
MERGEUVANY(MergeUV_Any_SSE2, MergeUV_Unaligned_SSE2, MergeUV_C, 15)
#endif
#ifdef HAS_MERGEUV_AVX2
MERGEUVANY(MergeUV_Any_AVX2, MergeUV_Unaligned_AVX2, MergeUV_C, 31)
#endif
#ifdef HAS_MERGEUV_NEON
MERGEUVANY(MergeUV_Any_NEON, MergeUV_Unaligned_NEON, MergeUV_C, 15)
#endif
#undef MERGEUVANY

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

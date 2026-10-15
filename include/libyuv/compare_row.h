/*
 *  Copyright 2013 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef INCLUDE_LIBYUV_COMPARE_ROW_H_
#define INCLUDE_LIBYUV_COMPARE_ROW_H_

#include <stddef.h>
#include <stdint.h>

#include "libyuv/cpu_support.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// The following are available for Visual C 32/64 bit, and for clang with
// LIBYUV_ENABLE_ROWWIN, which is also enabled for MemorySanitizer. These are
// intrinsics in compare_win.cc.
#if !defined(LIBYUV_DISABLE_X86) &&                                 \
    (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || \
     defined(_M_IX86)) &&                                           \
    ((defined(_MSC_VER) && !defined(__clang__)) ||                  \
     defined(LIBYUV_ENABLE_ROWWIN))
#define HAS_HAMMINGDISTANCE_SSE42
#define HAS_SUMSQUAREERROR_SSE2
#define HAS_HASHDJB2_SSE41
#define HAS_SSIM4X4ROW_SSE2
#define HAS_HAMMINGDISTANCE_AVX2
#define HAS_SUMSQUAREERROR_AVX2
#define HAS_HASHDJB2_AVX2
#define HAS_SSIM4X4ROW_AVX2
#define HAS_HAMMINGDISTANCE_AVX512BW
#define HAS_SUMSQUAREERROR_AVX512BW
#define HAS_HASHDJB2_AVX512BW
#define HAS_SSIM4X4ROW_AVX512BW
#endif

// The following are available for GCC and clang 32/64 bit, as inline assembly
// in compare_gcc.cc:
#if !defined(LIBYUV_DISABLE_X86) &&               \
    (defined(__x86_64__) || defined(__i386__)) && \
    !defined(LIBYUV_ENABLE_ROWWIN)
#define HAS_HAMMINGDISTANCE_SSE42
#define HAS_HAMMINGDISTANCE_SSSE3
#define HAS_SUMSQUAREERROR_SSE2
#define HAS_HASHDJB2_SSE41
#define HAS_SSIM4X4ROW_SSE2
#if defined(CLANG_HAS_AVX2) || defined(GCC_HAS_AVX2)
#define HAS_HAMMINGDISTANCE_AVX2
#define HAS_SUMSQUAREERROR_AVX2
#define HAS_HASHDJB2_AVX2
#define HAS_SSIM4X4ROW_AVX2
#endif
#if defined(CLANG_HAS_AVX512)
#define HAS_HAMMINGDISTANCE_AVX512BW
#define HAS_SUMSQUAREERROR_AVX512BW
#define HAS_HASHDJB2_AVX512BW
#define HAS_SSIM4X4ROW_AVX512BW
#endif
#endif

// The following are available for Neon:
#if !defined(LIBYUV_DISABLE_NEON) && \
    (defined(__ARM_NEON__) || defined(LIBYUV_NEON) || defined(__aarch64__))
#define HAS_HAMMINGDISTANCE_NEON
#define HAS_SUMSQUAREERROR_NEON
#define HAS_SSIM4X4ROW_NEON
#endif

// The following are available for AArch64 Neon:
#if !defined(LIBYUV_DISABLE_NEON) && defined(__aarch64__)
#define HAS_HASHDJB2_NEON

#define HAS_HAMMINGDISTANCE_NEON_DOTPROD
#define HAS_SUMSQUAREERROR_NEON_DOTPROD
#define HAS_SSIM4X4ROW_NEON_DOTPROD
#endif

// The following are available for AArch64 SVE:
#if !defined(LIBYUV_DISABLE_SVE) && defined(__aarch64__)
#define HAS_SSIM4X4ROW_SVE2
#endif

// The following are available for AArch64 SME:
#if !defined(LIBYUV_DISABLE_SME) && defined(CLANG_HAS_SME) && \
    defined(__aarch64__)
#define HAS_SSIM4X4ROW_SME
#endif

// The following are available for RISC-V Vector (RVV):
#if !defined(LIBYUV_DISABLE_RVV) && \
    (defined(__riscv_vector) || defined(LIBYUV_RVV))
#define HAS_SSIM4X4ROW_RVV
#endif

// The following are available for LoongArch LSX:
#if !defined(LIBYUV_DISABLE_LSX) && defined(__loongarch_sx)
#define HAS_SSIM4X4ROW_LSX
#endif

// The following are available for LoongArch LASX:
#if !defined(LIBYUV_DISABLE_LASX) && defined(__loongarch_asx)
#define HAS_SSIM4X4ROW_LASX
#endif

// The following are available for WebAssembly SIMD128:
#if !defined(LIBYUV_DISABLE_WASM) && defined(__wasm_simd128__)
#define HAS_HAMMINGDISTANCE_WASMSIMD
#define HAS_SUMSQUAREERROR_WASMSIMD
#define HAS_HASHDJB2_WASMSIMD
#define HAS_SSIM4X4ROW_WASMSIMD
#endif

uint32_t HammingDistance_C(const uint8_t* src_a,
                           const uint8_t* src_b,
                           int count);
uint32_t HammingDistance_SSE42(const uint8_t* src_a,
                               const uint8_t* src_b,
                               int count);
uint32_t HammingDistance_SSSE3(const uint8_t* src_a,
                               const uint8_t* src_b,
                               int count);
uint32_t HammingDistance_AVX2(const uint8_t* src_a,
                              const uint8_t* src_b,
                              int count);
uint32_t HammingDistance_AVX512BW(const uint8_t* src_a,
                                  const uint8_t* src_b,
                                  int count);
uint32_t HammingDistance_NEON(const uint8_t* src_a,
                              const uint8_t* src_b,
                              int count);
uint32_t HammingDistance_NEON_DotProd(const uint8_t* src_a,
                                      const uint8_t* src_b,
                                      int count);
uint32_t HammingDistance_WASMSIMD(const uint8_t* src_a,
                                  const uint8_t* src_b,
                                  int count);
uint32_t SumSquareError_C(const uint8_t* src_a,
                          const uint8_t* src_b,
                          int count);
uint32_t SumSquareError_SSE2(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count);
uint32_t SumSquareError_AVX2(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count);
uint32_t SumSquareError_AVX512BW(const uint8_t* src_a,
                                 const uint8_t* src_b,
                                 int count);
uint32_t SumSquareError_NEON(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count);
uint32_t SumSquareError_NEON_DotProd(const uint8_t* src_a,
                                     const uint8_t* src_b,
                                     int count);
uint32_t SumSquareError_WASMSIMD(const uint8_t* src_a,
                                 const uint8_t* src_b,
                                 int count);

uint32_t HashDjb2_C(const uint8_t* src, int count, uint32_t seed);
uint32_t HashDjb2_SSE41(const uint8_t* src, int count, uint32_t seed);
uint32_t HashDjb2_AVX2(const uint8_t* src, int count, uint32_t seed);
uint32_t HashDjb2_AVX512BW(const uint8_t* src, int count, uint32_t seed);
uint32_t HashDjb2_NEON(const uint8_t* src, int count, uint32_t seed);
uint32_t HashDjb2_WASMSIMD(const uint8_t* src, int count, uint32_t seed);

void Ssim4x4Row_C(const uint8_t* src_a,
                  int stride_a,
                  const uint8_t* src_b,
                  int stride_b,
                  uint32_t* dst,
                  int width);
void Ssim4x4Row_SSE2(const uint8_t* src_a,
                     int stride_a,
                     const uint8_t* src_b,
                     int stride_b,
                     uint32_t* dst,
                     int width);
void Ssim4x4Row_AVX2(const uint8_t* src_a,
                     int stride_a,
                     const uint8_t* src_b,
                     int stride_b,
                     uint32_t* dst,
                     int width);
void Ssim4x4Row_AVX512BW(const uint8_t* src_a,
                         int stride_a,
                         const uint8_t* src_b,
                         int stride_b,
                         uint32_t* dst,
                         int width);
void Ssim4x4Row_NEON(const uint8_t* src_a,
                     int stride_a,
                     const uint8_t* src_b,
                     int stride_b,
                     uint32_t* dst,
                     int width);
void Ssim4x4Row_NEON_DotProd(const uint8_t* src_a,
                             int stride_a,
                             const uint8_t* src_b,
                             int stride_b,
                             uint32_t* dst,
                             int width);
void Ssim4x4Row_SVE2(const uint8_t* src_a,
                     int stride_a,
                     const uint8_t* src_b,
                     int stride_b,
                     uint32_t* dst,
                     int width);
void Ssim4x4Row_SME(const uint8_t* src_a,
                    int stride_a,
                    const uint8_t* src_b,
                    int stride_b,
                    uint32_t* dst,
                    int width);
void Ssim4x4Row_RVV(const uint8_t* src_a,
                    int stride_a,
                    const uint8_t* src_b,
                    int stride_b,
                    uint32_t* dst,
                    int width);
void Ssim4x4Row_LSX(const uint8_t* src_a,
                    int stride_a,
                    const uint8_t* src_b,
                    int stride_b,
                    uint32_t* dst,
                    int width);
void Ssim4x4Row_LASX(const uint8_t* src_a,
                     int stride_a,
                     const uint8_t* src_b,
                     int stride_b,
                     uint32_t* dst,
                     int width);
void Ssim4x4Row_WASMSIMD(const uint8_t* src_a,
                         int stride_a,
                         const uint8_t* src_b,
                         int stride_b,
                         uint32_t* dst,
                         int width);
double SsimWindowRow_C(const uint32_t* row0,
                       const uint32_t* row1,
                       int num_windows);

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // INCLUDE_LIBYUV_COMPARE_ROW_H_

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

#if (!defined(LIBYUV_DISABLE_WASMSIMD) ||     \
     !defined(LIBYUV_DISABLE_RELAXEDSIMD)) && \
    defined(__wasm_simd128__)
#include <wasm_simd128.h>

#ifdef __cplusplus
namespace libyuv {
#endif

template <bool kRelaxed>
static inline v128_t WasmSwizzle(v128_t v, v128_t m) {
#if !defined(LIBYUV_DISABLE_RELAXEDSIMD) && defined(__wasm_relaxed_simd__)
  if (kRelaxed) {
    return wasm_i8x16_relaxed_swizzle(v, m);
  }
#endif
  return wasm_i8x16_swizzle(v, m);
}

// V8 materializes v128.const values and arbitrary i8x16.shuffle masks with
// several instructions at every use, including inside loops. These helpers
// make constants opaque to clang so they are built once, before the loop,
// and kept in registers. Use swizzle with a LoadConst() table instead of
// i8x16.shuffle unless the pattern maps to a native instruction (unpack,
// 32-bit lane shuffle, concat/palignr, even/odd byte unzip).
// Loops that access ptr + 16 etc. use asm("" : "+r"(ptr)) so LLVM keeps the
// constant offsets in the load/store instructions instead of creating extra
// pointer induction variables.
static inline v128_t LoadConst(const void* p) {
  asm("" : "+r"(p));
  return wasm_v128_load(p);
}
static inline v128_t SplatConst8(int v) {
  asm("" : "+r"(v));
  return wasm_i8x16_splat(v);
}
static inline v128_t SplatConst16(int v) {
  asm("" : "+r"(v));
  return wasm_i16x8_splat(v);
}
static inline v128_t SplatConst32(int v) {
  asm("" : "+r"(v));
  return wasm_i32x4_splat(v);
}

#if defined(HAS_RAWTORGB24ROW_WASMSIMD) || \
    defined(HAS_RAWTORGB24ROW_RELAXEDSIMD)
static const uint8_t kRawToRGB24SwizzleTable[96] = {
    2,   1,   0,   5,   4,   3,   8,   7,   6,   11,  10,  9,   14,  13,  12,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 1,   0,   128, 4,   3,   2,   7,   6,   5,   10,  9,   8,   13,  12,
    11,  128, 15,  128, 1,   128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 2,   128, 128, 3,   2,   1,   6,   5,   4,   9,   8,   7,   12,
    11,  10,  15,  14,  13,  14,  128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128};

template <bool kRelaxed>
static inline void RAWToRGB24Row_Wasm(const uint8_t* src_raw,
                                      uint8_t* dst_rgb24,
                                      int width) {
  const uint8_t* shuf = kRawToRGB24SwizzleTable;
  // Load shuffle masks into Wasm locals outside the loop so Binaryen's
  // precompute-propagate pass does not sink v128.const into the loop.
  asm("" : "+r"(shuf));
  v128_t kShuf0_0 = wasm_v128_load(shuf);
  v128_t kShuf0_1 = wasm_v128_load(shuf + 16);
  v128_t kShuf1_1 = wasm_v128_load(shuf + 32);
  v128_t kShuf1_02 = wasm_v128_load(shuf + 48);
  v128_t kShuf2_2 = wasm_v128_load(shuf + 64);
  v128_t kShuf2_1 = wasm_v128_load(shuf + 80);

  while (width >= 32) {
    // Prevent LLVM LSR from splitting constant load/store offsets into separate
    // pointer induction variables.
    asm("" : "+r"(src_raw), "+r"(dst_rgb24));
    v128_t s0 = wasm_v128_load(src_raw);
    v128_t s1 = wasm_v128_load(src_raw + 16);
    v128_t s2 = wasm_v128_load(src_raw + 32);
    v128_t s3 = wasm_v128_load(src_raw + 48);
    v128_t s4 = wasm_v128_load(src_raw + 64);
    v128_t s5 = wasm_v128_load(src_raw + 80);

    v128_t s02 = wasm_i8x16_shuffle(s0, s2, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                                    23, 24, 25, 26, 27, 28, 29);
    v128_t s35 = wasm_i8x16_shuffle(s3, s5, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                                    23, 24, 25, 26, 27, 28, 29);

    v128_t d0 = wasm_v128_or(WasmSwizzle<kRelaxed>(s0, kShuf0_0),
                             WasmSwizzle<kRelaxed>(s1, kShuf0_1));
    v128_t d1 = wasm_v128_or(WasmSwizzle<kRelaxed>(s1, kShuf1_1),
                             WasmSwizzle<kRelaxed>(s02, kShuf1_02));
    v128_t d2 = wasm_v128_or(WasmSwizzle<kRelaxed>(s2, kShuf2_2),
                             WasmSwizzle<kRelaxed>(s1, kShuf2_1));
    v128_t d3 = wasm_v128_or(WasmSwizzle<kRelaxed>(s3, kShuf0_0),
                             WasmSwizzle<kRelaxed>(s4, kShuf0_1));
    v128_t d4 = wasm_v128_or(WasmSwizzle<kRelaxed>(s4, kShuf1_1),
                             WasmSwizzle<kRelaxed>(s35, kShuf1_02));
    v128_t d5 = wasm_v128_or(WasmSwizzle<kRelaxed>(s5, kShuf2_2),
                             WasmSwizzle<kRelaxed>(s4, kShuf2_1));

    wasm_v128_store(dst_rgb24, d0);
    wasm_v128_store(dst_rgb24 + 16, d1);
    wasm_v128_store(dst_rgb24 + 32, d2);
    wasm_v128_store(dst_rgb24 + 48, d3);
    wasm_v128_store(dst_rgb24 + 64, d4);
    wasm_v128_store(dst_rgb24 + 80, d5);

    src_raw += 96;
    dst_rgb24 += 96;
    width -= 32;
  }
  if (width > 0) {
    asm("" : "+r"(src_raw), "+r"(dst_rgb24));
    v128_t s0 = wasm_v128_load(src_raw);
    v128_t s1 = wasm_v128_load(src_raw + 16);
    v128_t s2 = wasm_v128_load(src_raw + 32);

    v128_t s02 = wasm_i8x16_shuffle(s0, s2, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                                    23, 24, 25, 26, 27, 28, 29);

    v128_t d0 = wasm_v128_or(WasmSwizzle<kRelaxed>(s0, kShuf0_0),
                             WasmSwizzle<kRelaxed>(s1, kShuf0_1));
    v128_t d1 = wasm_v128_or(WasmSwizzle<kRelaxed>(s1, kShuf1_1),
                             WasmSwizzle<kRelaxed>(s02, kShuf1_02));
    v128_t d2 = wasm_v128_or(WasmSwizzle<kRelaxed>(s2, kShuf2_2),
                             WasmSwizzle<kRelaxed>(s1, kShuf2_1));

    wasm_v128_store(dst_rgb24, d0);
    wasm_v128_store(dst_rgb24 + 16, d1);
    wasm_v128_store(dst_rgb24 + 32, d2);
  }
}

#ifdef HAS_RAWTORGB24ROW_WASMSIMD
void RAWToRGB24Row_WASMSIMD(const uint8_t* src_raw,
                            uint8_t* dst_rgb24,
                            int width) {
  RAWToRGB24Row_Wasm<false>(src_raw, dst_rgb24, width);
}
#endif  // HAS_RAWTORGB24ROW_WASMSIMD
#ifdef HAS_RAWTORGB24ROW_RELAXEDSIMD
void RAWToRGB24Row_RELAXEDSIMD(const uint8_t* src_raw,
                               uint8_t* dst_rgb24,
                               int width) {
  RAWToRGB24Row_Wasm<true>(src_raw, dst_rgb24, width);
}
#endif  // HAS_RAWTORGB24ROW_RELAXEDSIMD
#endif  // HAS_RAWTORGB24ROW_WASMSIMD || HAS_RAWTORGB24ROW_RELAXEDSIMD

#if defined(HAS_RAWTOARGBROW_WASMSIMD) ||    \
    defined(HAS_RAWTOARGBROW_RELAXEDSIMD) || \
    defined(HAS_RAWTORGBAROW_WASMSIMD) ||    \
    defined(HAS_RAWTORGBAROW_RELAXEDSIMD) || \
    defined(HAS_RGB24TOARGBROW_WASMSIMD) ||  \
    defined(HAS_RGB24TOARGBROW_RELAXEDSIMD)
// 16 pixels of 3 bytes to 4 bytes. shuf holds 2 masks: one for loads at
// offsets 0, 12, 24 and one for the load at offset 32 (avoids over-read).
template <bool kRelaxed>
static void RGB3ToARGBRow_Wasm(const uint8_t* src,
                               uint8_t* dst_argb,
                               int width,
                               const uint8_t* shuf,
                               uint32_t alpha_mask) {
  v128_t m0 = LoadConst(shuf);
  v128_t m1 = LoadConst(shuf + 16);
  v128_t alpha = SplatConst32((int)alpha_mask);
  do {
    asm("" : "+r"(src), "+r"(dst_argb));
    v128_t d0 = WasmSwizzle<kRelaxed>(wasm_v128_load(src), m0);
    v128_t d1 = WasmSwizzle<kRelaxed>(wasm_v128_load(src + 12), m0);
    v128_t d2 = WasmSwizzle<kRelaxed>(wasm_v128_load(src + 24), m0);
    v128_t d3 = WasmSwizzle<kRelaxed>(wasm_v128_load(src + 32), m1);
    wasm_v128_store(dst_argb, wasm_v128_or(d0, alpha));
    wasm_v128_store(dst_argb + 16, wasm_v128_or(d1, alpha));
    wasm_v128_store(dst_argb + 32, wasm_v128_or(d2, alpha));
    wasm_v128_store(dst_argb + 48, wasm_v128_or(d3, alpha));
    src += 48;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif

#if defined(HAS_RAWTOARGBROW_WASMSIMD) || defined(HAS_RAWTOARGBROW_RELAXEDSIMD)
static const uint8_t kShuffleRAWToARGB[32] = {
    2, 1, 0, 128, 5, 4, 3, 128, 8,  7,  6,  128, 11, 10, 9,  128,
    6, 5, 4, 128, 9, 8, 7, 128, 12, 11, 10, 128, 15, 14, 13, 128};
#ifdef HAS_RAWTOARGBROW_WASMSIMD
void RAWToARGBRow_WASMSIMD(const uint8_t* src_raw,
                           uint8_t* dst_argb,
                           int width) {
  RGB3ToARGBRow_Wasm<false>(src_raw, dst_argb, width, kShuffleRAWToARGB,
                            0xff000000u);
}
#endif  // HAS_RAWTOARGBROW_WASMSIMD
#ifdef HAS_RAWTOARGBROW_RELAXEDSIMD
void RAWToARGBRow_RELAXEDSIMD(const uint8_t* src_raw,
                              uint8_t* dst_argb,
                              int width) {
  RGB3ToARGBRow_Wasm<true>(src_raw, dst_argb, width, kShuffleRAWToARGB,
                           0xff000000u);
}
#endif  // HAS_RAWTOARGBROW_RELAXEDSIMD
#endif  // HAS_RAWTOARGBROW_WASMSIMD || HAS_RAWTOARGBROW_RELAXEDSIMD

#if defined(HAS_RAWTORGBAROW_WASMSIMD) || defined(HAS_RAWTORGBAROW_RELAXEDSIMD)
static const uint8_t kShuffleRAWToRGBA[32] = {
    128, 2, 1, 0, 128, 5, 4, 3, 128, 8,  7,  6,  128, 11, 10, 9,
    128, 6, 5, 4, 128, 9, 8, 7, 128, 12, 11, 10, 128, 15, 14, 13};
#ifdef HAS_RAWTORGBAROW_WASMSIMD
void RAWToRGBARow_WASMSIMD(const uint8_t* src_raw,
                           uint8_t* dst_rgba,
                           int width) {
  RGB3ToARGBRow_Wasm<false>(src_raw, dst_rgba, width, kShuffleRAWToRGBA,
                            0x000000ffu);
}
#endif  // HAS_RAWTORGBAROW_WASMSIMD
#ifdef HAS_RAWTORGBAROW_RELAXEDSIMD
void RAWToRGBARow_RELAXEDSIMD(const uint8_t* src_raw,
                              uint8_t* dst_rgba,
                              int width) {
  RGB3ToARGBRow_Wasm<true>(src_raw, dst_rgba, width, kShuffleRAWToRGBA,
                           0x000000ffu);
}
#endif  // HAS_RAWTORGBAROW_RELAXEDSIMD
#endif  // HAS_RAWTORGBAROW_WASMSIMD || HAS_RAWTORGBAROW_RELAXEDSIMD

#if defined(HAS_RGB24TOARGBROW_WASMSIMD) || \
    defined(HAS_RGB24TOARGBROW_RELAXEDSIMD)
static const uint8_t kShuffleRGB24ToARGB[32] = {
    0, 1, 2, 128, 3, 4, 5, 128, 6,  7,  8,  128, 9,  10, 11, 128,
    4, 5, 6, 128, 7, 8, 9, 128, 10, 11, 12, 128, 13, 14, 15, 128};
#ifdef HAS_RGB24TOARGBROW_WASMSIMD
void RGB24ToARGBRow_WASMSIMD(const uint8_t* src_rgb24,
                             uint8_t* dst_argb,
                             int width) {
  RGB3ToARGBRow_Wasm<false>(src_rgb24, dst_argb, width, kShuffleRGB24ToARGB,
                            0xff000000u);
}
#endif  // HAS_RGB24TOARGBROW_WASMSIMD
#ifdef HAS_RGB24TOARGBROW_RELAXEDSIMD
void RGB24ToARGBRow_RELAXEDSIMD(const uint8_t* src_rgb24,
                                uint8_t* dst_argb,
                                int width) {
  RGB3ToARGBRow_Wasm<true>(src_rgb24, dst_argb, width, kShuffleRGB24ToARGB,
                           0xff000000u);
}
#endif  // HAS_RGB24TOARGBROW_RELAXEDSIMD
#endif  // HAS_RGB24TOARGBROW_WASMSIMD || HAS_RGB24TOARGBROW_RELAXEDSIMD

#if defined(HAS_ARGBTORGB24ROW_WASMSIMD) ||    \
    defined(HAS_ARGBTORGB24ROW_RELAXEDSIMD) || \
    defined(HAS_ARGBTORAWROW_WASMSIMD) ||      \
    defined(HAS_ARGBTORAWROW_RELAXEDSIMD)
// 16 pixels of 4 bytes to 3 bytes (48 bytes output).
template <bool kRelaxed>
static void ARGBToRGB3Row_Wasm(const uint8_t* src_argb,
                               uint8_t* dst_rgb,
                               int width,
                               const uint8_t* shuf) {
  v128_t m0a = LoadConst(shuf);
  v128_t m0b = LoadConst(shuf + 16);
  v128_t m1a = LoadConst(shuf + 32);
  v128_t m1b = LoadConst(shuf + 48);
  v128_t m2a = LoadConst(shuf + 64);
  v128_t m2b = LoadConst(shuf + 80);
  do {
    asm("" : "+r"(src_argb), "+r"(dst_rgb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    v128_t s2 = wasm_v128_load(src_argb + 32);
    v128_t s3 = wasm_v128_load(src_argb + 48);
    v128_t d0 = wasm_v128_or(WasmSwizzle<kRelaxed>(s0, m0a),
                             WasmSwizzle<kRelaxed>(s1, m0b));
    v128_t d1 = wasm_v128_or(WasmSwizzle<kRelaxed>(s1, m1a),
                             WasmSwizzle<kRelaxed>(s2, m1b));
    v128_t d2 = wasm_v128_or(WasmSwizzle<kRelaxed>(s2, m2a),
                             WasmSwizzle<kRelaxed>(s3, m2b));
    wasm_v128_store(dst_rgb, d0);
    wasm_v128_store(dst_rgb + 16, d1);
    wasm_v128_store(dst_rgb + 32, d2);
    src_argb += 64;
    dst_rgb += 48;
    width -= 16;
  } while (width > 0);
}
#endif

#if defined(HAS_ARGBTORGB24ROW_WASMSIMD) || \
    defined(HAS_ARGBTORGB24ROW_RELAXEDSIMD)
static const uint8_t kShuffleARGBToRGB24[96] = {
    0,   1,   2,   4,   5,   6,   8,   9,   10,  12,  13,  14,  128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    0,   1,   2,   4,   5,   6,   8,   9,   10,  12,  13,  14,  128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    0,   1,   2,   4,   5,   6,   8,   9,   10,  12,  13,  14,  128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    0,   1,   2,   4,   5,   6,   8,   9,   10,  12,  13,  14};
#ifdef HAS_ARGBTORGB24ROW_WASMSIMD
void ARGBToRGB24Row_WASMSIMD(const uint8_t* src_argb,
                             uint8_t* dst_rgb24,
                             int width) {
  ARGBToRGB3Row_Wasm<false>(src_argb, dst_rgb24, width, kShuffleARGBToRGB24);
}
#endif  // HAS_ARGBTORGB24ROW_WASMSIMD
#ifdef HAS_ARGBTORGB24ROW_RELAXEDSIMD
void ARGBToRGB24Row_RELAXEDSIMD(const uint8_t* src_argb,
                                uint8_t* dst_rgb24,
                                int width) {
  ARGBToRGB3Row_Wasm<true>(src_argb, dst_rgb24, width, kShuffleARGBToRGB24);
}
#endif  // HAS_ARGBTORGB24ROW_RELAXEDSIMD
#endif  // HAS_ARGBTORGB24ROW_WASMSIMD || HAS_ARGBTORGB24ROW_RELAXEDSIMD

#if defined(HAS_ARGBTORAWROW_WASMSIMD) || defined(HAS_ARGBTORAWROW_RELAXEDSIMD)
static const uint8_t kShuffleARGBToRAW[96] = {
    2,   1,   0,   6,   5,   4,   10,  9,   8,   14,  13,  12,  128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    2,   1,   0,   6,   5,   4,   10,  9,   8,   14,  13,  12,  128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    2,   1,   0,   6,   5,   4,   10,  9,   8,   14,  13,  12,  128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    2,   1,   0,   6,   5,   4,   10,  9,   8,   14,  13,  12};
#ifdef HAS_ARGBTORAWROW_WASMSIMD
void ARGBToRAWRow_WASMSIMD(const uint8_t* src_argb,
                           uint8_t* dst_raw,
                           int width) {
  ARGBToRGB3Row_Wasm<false>(src_argb, dst_raw, width, kShuffleARGBToRAW);
}
#endif  // HAS_ARGBTORAWROW_WASMSIMD
#ifdef HAS_ARGBTORAWROW_RELAXEDSIMD
void ARGBToRAWRow_RELAXEDSIMD(const uint8_t* src_argb,
                              uint8_t* dst_raw,
                              int width) {
  ARGBToRGB3Row_Wasm<true>(src_argb, dst_raw, width, kShuffleARGBToRAW);
}
#endif  // HAS_ARGBTORAWROW_RELAXEDSIMD
#endif  // HAS_ARGBTORAWROW_WASMSIMD || HAS_ARGBTORAWROW_RELAXEDSIMD

#if defined(HAS_ARGBTOYMATRIXROW_WASMSIMD) ||        \
    defined(HAS_ARGBTOUV444MATRIXROW_WASMSIMD) ||    \
    defined(HAS_ARGBTOUV444MATRIXROW_RELAXEDSIMD) || \
    defined(HAS_ARGBTOUVMATRIXROW_WASMSIMD) ||       \
    defined(HAS_ARGBTOUVMATRIXROW_RELAXEDSIMD) ||    \
    defined(HAS_RGBTOYMATRIXROW_WASMSIMD) ||         \
    defined(HAS_RGBTOYMATRIXROW_RELAXEDSIMD) ||      \
    defined(HAS_RGBTOUV444MATRIXROW_WASMSIMD) ||     \
    defined(HAS_RGBTOUV444MATRIXROW_RELAXEDSIMD) ||  \
    defined(HAS_RGBTOUVMATRIXROW_WASMSIMD) ||        \
    defined(HAS_RGBTOUVMATRIXROW_RELAXEDSIMD) ||     \
    defined(HAS_RGB565TOYMATRIXROW_WASMSIMD) ||      \
    defined(HAS_ARGB1555TOYMATRIXROW_WASMSIMD) ||    \
    defined(HAS_ARGB4444TOYMATRIXROW_WASMSIMD) ||    \
    defined(HAS_RGB565TOUVMATRIXROW_WASMSIMD) ||     \
    defined(HAS_ARGB1555TOUVMATRIXROW_WASMSIMD) ||   \
    defined(HAS_ARGB4444TOUVMATRIXROW_WASMSIMD)
// RGB to YUV. Math is in 16 bit lanes, which wrap, but sum + bias fits in
// 16 bits so the results match the C code. i32x4.dot_i16x8_s would be best
// on x86 but is 3 instructions on Arm, so i16x8.mul (mul + mla on Arm) and
// horizontal adds that are cheap on both are used instead.

// lo * k0 + hi * k1.
static inline v128_t MulAdd16(v128_t lo, v128_t hi, v128_t k0, v128_t k1) {
  return wasm_i16x8_add(wasm_i16x8_mul(lo, k0), wasm_i16x8_mul(hi, k1));
}

// Sums adjacent 16 bit lanes of t0 and t1.
static inline v128_t HAddPairs16(v128_t t0, v128_t t1) {
  return wasm_i16x8_add(wasm_i16x8_shuffle(t0, t1, 0, 2, 4, 6, 8, 10, 12, 14),
                        wasm_i16x8_shuffle(t0, t1, 1, 3, 5, 7, 9, 11, 13, 15));
}

// Splats 16 bit {lo, hi} into each 32 bit lane.
static inline v128_t SplatPair16(int lo, int hi) {
  return SplatConst32((lo & 0xffff) | (hi << 16));
}

// (bias - sum) >> 8 for U and V.
static inline v128_t SubShr8(v128_t bias, v128_t sum) {
  return wasm_u16x8_shr(wasm_i16x8_sub(bias, sum), 8);
}
#endif

#if defined(HAS_ARGBTOUV444MATRIXROW_WASMSIMD) ||    \
    defined(HAS_ARGBTOUV444MATRIXROW_RELAXEDSIMD) || \
    defined(HAS_RGBTOYMATRIXROW_WASMSIMD) ||         \
    defined(HAS_RGBTOYMATRIXROW_RELAXEDSIMD) ||      \
    defined(HAS_RGBTOUV444MATRIXROW_WASMSIMD) ||     \
    defined(HAS_RGBTOUV444MATRIXROW_RELAXEDSIMD)
// Swizzles 4 pixels to 16 bit {c0 x4, c2 x4} and {c1 x4, c3 x4}, so the 2
// halves of the products are summed with a 64 bit interleave.
static const uint8_t kShuffleARGBToPlanar[32] = {
    0, 128, 4, 128, 8, 128, 12, 128, 2, 128, 6, 128, 10, 128, 14, 128,
    1, 128, 5, 128, 9, 128, 13, 128, 3, 128, 7, 128, 11, 128, 15, 128};
// RGB24 to {b x4, r x4} and {g x4, 0 x4}. The second pair of masks is for
// the load at offset 32 (avoids over-read).
static const uint8_t kShuffleRGBToPlanar[64] = {
    0, 128, 3, 128, 6,  128, 9,  128, 2,   128, 5,   128, 8,   128, 11,  128,
    1, 128, 4, 128, 7,  128, 10, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    4, 128, 7, 128, 10, 128, 13, 128, 6,   128, 9,   128, 12,  128, 15,  128,
    5, 128, 8, 128, 11, 128, 14, 128, 128, 128, 128, 128, 128, 128, 128, 128};

// {a x4, b x4} as 16 bit lanes.
static inline v128_t Splat4x2(int a, int b) {
  asm("" : "+r"(a), "+r"(b));
  return wasm_i16x8_make(a, a, a, a, b, b, b, b);
}

// Sums the 64 bit halves of t0 and t1.
static inline v128_t HAddHalves64(v128_t t0, v128_t t1) {
  return wasm_i16x8_add(wasm_i64x2_shuffle(t0, t1, 0, 2),
                        wasm_i64x2_shuffle(t0, t1, 1, 3));
}

// 16 U and V from 16 pixels swizzled by m (m[0], m[1] for the first 3
// vectors, m[2], m[3] for the last).
template <bool kRelaxed>
static inline void PlanarToUV16(const v128_t* s,
                                const v128_t* m,
                                v128_t ku02,
                                v128_t ku13,
                                v128_t kv02,
                                v128_t kv13,
                                v128_t bias_u,
                                v128_t bias_v,
                                uint8_t* dst_u,
                                uint8_t* dst_v) {
  v128_t u[4], v[4];
  for (int i = 0; i < 4; ++i) {
    v128_t lo = WasmSwizzle<kRelaxed>(s[i], m[i == 3 ? 2 : 0]);
    v128_t hi = WasmSwizzle<kRelaxed>(s[i], m[i == 3 ? 3 : 1]);
    u[i] = MulAdd16(lo, hi, ku02, ku13);
    v[i] = MulAdd16(lo, hi, kv02, kv13);
  }
  wasm_v128_store(dst_u, wasm_u8x16_narrow_i16x8(
                             SubShr8(bias_u, HAddHalves64(u[0], u[1])),
                             SubShr8(bias_u, HAddHalves64(u[2], u[3]))));
  wasm_v128_store(dst_v, wasm_u8x16_narrow_i16x8(
                             SubShr8(bias_v, HAddHalves64(v[0], v[1])),
                             SubShr8(bias_v, HAddHalves64(v[2], v[3]))));
}
#endif

#if defined(HAS_ARGBTOUVMATRIXROW_WASMSIMD) ||    \
    defined(HAS_ARGBTOUVMATRIXROW_RELAXEDSIMD) || \
    defined(HAS_RGBTOUVMATRIXROW_WASMSIMD) ||     \
    defined(HAS_RGBTOUVMATRIXROW_RELAXEDSIMD)
// Rounded 2x2 average of 8 pixels held as 16 bit {c0, c2} or {c1, c3}
// pairs. a = row 0, b = row 1, 4 pixels per vector. Returns 4 pixels.
static inline v128_t Avg2x2Pairs(v128_t a0,
                                 v128_t a1,
                                 v128_t b0,
                                 v128_t b1,
                                 v128_t two) {
  v128_t s0 = wasm_i16x8_add(a0, b0);
  v128_t s1 = wasm_i16x8_add(a1, b1);
  v128_t s = wasm_i16x8_add(wasm_i32x4_shuffle(s0, s1, 0, 2, 4, 6),
                            wasm_i32x4_shuffle(s0, s1, 1, 3, 5, 7));
  return wasm_u16x8_shr(wasm_i16x8_add(s, two), 2);
}

// Stores 8 U and 8 V from 2 vectors each of {c0, c2} and {c1, c3} pairs.
static inline void PairsToUV8(const v128_t* lo,
                              const v128_t* hi,
                              v128_t ku02,
                              v128_t ku13,
                              v128_t kv02,
                              v128_t kv13,
                              v128_t bias_u,
                              v128_t bias_v,
                              uint8_t* dst_u,
                              uint8_t* dst_v) {
  v128_t u = HAddPairs16(MulAdd16(lo[0], hi[0], ku02, ku13),
                         MulAdd16(lo[1], hi[1], ku02, ku13));
  v128_t v = HAddPairs16(MulAdd16(lo[0], hi[0], kv02, kv13),
                         MulAdd16(lo[1], hi[1], kv02, kv13));
  v128_t uv = wasm_u8x16_narrow_i16x8(SubShr8(bias_u, u), SubShr8(bias_v, v));
  wasm_v128_store64_lane(dst_u, uv, 0);
  wasm_v128_store64_lane(dst_v, uv, 1);
}
#endif

#ifdef HAS_ARGBTOYMATRIXROW_WASMSIMD
void ARGBToYMatrixRow_WASMSIMD(const uint8_t* src_argb,
                               uint8_t* dst_y,
                               int width,
                               const struct ArgbConstants* c) {
  v128_t mask = SplatConst16(0xff);
  v128_t k02 = SplatPair16(c->kRGBToY[0], c->kRGBToY[2]);
  v128_t k13 = SplatPair16(c->kRGBToY[1], c->kRGBToY[3]);
  v128_t bias = SplatConst16(c->kAddY[0]);
  do {
    v128_t t[4];
    for (int i = 0; i < 4; ++i) {
      v128_t s = wasm_v128_load(src_argb + i * 16);
      t[i] = MulAdd16(wasm_v128_and(s, mask), wasm_u16x8_shr(s, 8), k02, k13);
    }
    v128_t y0 = wasm_i16x8_add(HAddPairs16(t[0], t[1]), bias);
    v128_t y1 = wasm_i16x8_add(HAddPairs16(t[2], t[3]), bias);
    wasm_v128_store(dst_y, wasm_u8x16_narrow_i16x8(wasm_u16x8_shr(y0, 8),
                                                   wasm_u16x8_shr(y1, 8)));
    src_argb += 64;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_ARGBTOYMATRIXROW_WASMSIMD

#if defined(HAS_RGBTOYMATRIXROW_WASMSIMD) || \
    defined(HAS_RGBTOYMATRIXROW_RELAXEDSIMD)
// RGB24 is alpha 255 in the C version, so 255 * kRGBToY[3] is in the bias.
template <bool kRelaxed>
static inline void RGBToYMatrixRow_Wasm(const uint8_t* src_rgb,
                                        uint8_t* dst_y,
                                        int width,
                                        const struct ArgbConstants* c) {
  v128_t m[4];
  for (int i = 0; i < 4; ++i) {
    m[i] = LoadConst(kShuffleRGBToPlanar + i * 16);
  }
  v128_t k02 = Splat4x2(c->kRGBToY[0], c->kRGBToY[2]);
  v128_t k13 = Splat4x2(c->kRGBToY[1], 0);
  v128_t bias = SplatConst16(c->kAddY[0] + 255 * c->kRGBToY[3]);
  do {
    v128_t t[4];
    for (int i = 0; i < 4; ++i) {
      v128_t s = wasm_v128_load(src_rgb + (i == 3 ? 32 : i * 12));
      t[i] = MulAdd16(WasmSwizzle<kRelaxed>(s, m[i == 3 ? 2 : 0]),
                      WasmSwizzle<kRelaxed>(s, m[i == 3 ? 3 : 1]), k02, k13);
    }
    v128_t y0 = wasm_i16x8_add(HAddHalves64(t[0], t[1]), bias);
    v128_t y1 = wasm_i16x8_add(HAddHalves64(t[2], t[3]), bias);
    wasm_v128_store(dst_y, wasm_u8x16_narrow_i16x8(wasm_u16x8_shr(y0, 8),
                                                   wasm_u16x8_shr(y1, 8)));
    src_rgb += 48;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#ifdef HAS_RGBTOYMATRIXROW_WASMSIMD
void RGBToYMatrixRow_WASMSIMD(const uint8_t* src_rgb,
                              uint8_t* dst_y,
                              int width,
                              const struct ArgbConstants* c) {
  RGBToYMatrixRow_Wasm<false>(src_rgb, dst_y, width, c);
}
#endif  // HAS_RGBTOYMATRIXROW_WASMSIMD
#ifdef HAS_RGBTOYMATRIXROW_RELAXEDSIMD
void RGBToYMatrixRow_RELAXEDSIMD(const uint8_t* src_rgb,
                                 uint8_t* dst_y,
                                 int width,
                                 const struct ArgbConstants* c) {
  RGBToYMatrixRow_Wasm<true>(src_rgb, dst_y, width, c);
}
#endif  // HAS_RGBTOYMATRIXROW_RELAXEDSIMD
#endif  // HAS_RGBTOYMATRIXROW_WASMSIMD || HAS_RGBTOYMATRIXROW_RELAXEDSIMD

#if defined(HAS_ARGBTOUV444MATRIXROW_WASMSIMD) || \
    defined(HAS_ARGBTOUV444MATRIXROW_RELAXEDSIMD)
template <bool kRelaxed>
static inline void ARGBToUV444MatrixRow_Wasm(const uint8_t* src_argb,
                                             uint8_t* dst_u,
                                             uint8_t* dst_v,
                                             int width,
                                             const struct ArgbConstants* c) {
  v128_t m[4];
  m[0] = m[2] = LoadConst(kShuffleARGBToPlanar);
  m[1] = m[3] = LoadConst(kShuffleARGBToPlanar + 16);
  v128_t ku02 = Splat4x2(c->kRGBToU[0], c->kRGBToU[2]);
  v128_t ku13 = Splat4x2(c->kRGBToU[1], c->kRGBToU[3]);
  v128_t kv02 = Splat4x2(c->kRGBToV[0], c->kRGBToV[2]);
  v128_t kv13 = Splat4x2(c->kRGBToV[1], c->kRGBToV[3]);
  v128_t bias = SplatConst16(c->kAddUV[0]);
  do {
    v128_t s[4];
    for (int i = 0; i < 4; ++i) {
      s[i] = wasm_v128_load(src_argb + i * 16);
    }
    PlanarToUV16<kRelaxed>(s, m, ku02, ku13, kv02, kv13, bias, bias, dst_u,
                           dst_v);
    src_argb += 64;
    dst_u += 16;
    dst_v += 16;
    width -= 16;
  } while (width > 0);
}
#ifdef HAS_ARGBTOUV444MATRIXROW_WASMSIMD
void ARGBToUV444MatrixRow_WASMSIMD(const uint8_t* src_argb,
                                   uint8_t* dst_u,
                                   uint8_t* dst_v,
                                   int width,
                                   const struct ArgbConstants* c) {
  ARGBToUV444MatrixRow_Wasm<false>(src_argb, dst_u, dst_v, width, c);
}
#endif  // HAS_ARGBTOUV444MATRIXROW_WASMSIMD
#ifdef HAS_ARGBTOUV444MATRIXROW_RELAXEDSIMD
void ARGBToUV444MatrixRow_RELAXEDSIMD(const uint8_t* src_argb,
                                      uint8_t* dst_u,
                                      uint8_t* dst_v,
                                      int width,
                                      const struct ArgbConstants* c) {
  ARGBToUV444MatrixRow_Wasm<true>(src_argb, dst_u, dst_v, width, c);
}
#endif  // HAS_ARGBTOUV444MATRIXROW_RELAXEDSIMD
#endif  // HAS_ARGBTOUV444MATRIXROW_WASMSIMD ||
        // HAS_ARGBTOUV444MATRIXROW_RELAXEDSIMD

#if defined(HAS_RGBTOUV444MATRIXROW_WASMSIMD) || \
    defined(HAS_RGBTOUV444MATRIXROW_RELAXEDSIMD)
template <bool kRelaxed>
static inline void RGBToUV444MatrixRow_Wasm(const uint8_t* src_rgb,
                                            uint8_t* dst_u,
                                            uint8_t* dst_v,
                                            int width,
                                            const struct ArgbConstants* c) {
  v128_t m[4];
  for (int i = 0; i < 4; ++i) {
    m[i] = LoadConst(kShuffleRGBToPlanar + i * 16);
  }
  v128_t ku02 = Splat4x2(c->kRGBToU[0], c->kRGBToU[2]);
  v128_t ku13 = Splat4x2(c->kRGBToU[1], 0);
  v128_t kv02 = Splat4x2(c->kRGBToV[0], c->kRGBToV[2]);
  v128_t kv13 = Splat4x2(c->kRGBToV[1], 0);
  v128_t bias_u = SplatConst16(c->kAddUV[0] - 255 * c->kRGBToU[3]);
  v128_t bias_v = SplatConst16(c->kAddUV[0] - 255 * c->kRGBToV[3]);
  do {
    v128_t s[4];
    for (int i = 0; i < 4; ++i) {
      s[i] = wasm_v128_load(src_rgb + (i == 3 ? 32 : i * 12));
    }
    PlanarToUV16<kRelaxed>(s, m, ku02, ku13, kv02, kv13, bias_u, bias_v, dst_u,
                           dst_v);
    src_rgb += 48;
    dst_u += 16;
    dst_v += 16;
    width -= 16;
  } while (width > 0);
}
#ifdef HAS_RGBTOUV444MATRIXROW_WASMSIMD
void RGBToUV444MatrixRow_WASMSIMD(const uint8_t* src_rgb,
                                  uint8_t* dst_u,
                                  uint8_t* dst_v,
                                  int width,
                                  const struct ArgbConstants* c) {
  RGBToUV444MatrixRow_Wasm<false>(src_rgb, dst_u, dst_v, width, c);
}
#endif  // HAS_RGBTOUV444MATRIXROW_WASMSIMD
#ifdef HAS_RGBTOUV444MATRIXROW_RELAXEDSIMD
void RGBToUV444MatrixRow_RELAXEDSIMD(const uint8_t* src_rgb,
                                     uint8_t* dst_u,
                                     uint8_t* dst_v,
                                     int width,
                                     const struct ArgbConstants* c) {
  RGBToUV444MatrixRow_Wasm<true>(src_rgb, dst_u, dst_v, width, c);
}
#endif  // HAS_RGBTOUV444MATRIXROW_RELAXEDSIMD
#endif  // HAS_RGBTOUV444MATRIXROW_WASMSIMD ||
        // HAS_RGBTOUV444MATRIXROW_RELAXEDSIMD

#ifdef HAS_ARGBTOUVMATRIXROW_WASMSIMD
void ARGBToUVMatrixRow_WASMSIMD(const uint8_t* src_argb,
                                int src_stride_argb,
                                uint8_t* dst_u,
                                uint8_t* dst_v,
                                int width,
                                const struct ArgbConstants* c) {
  const uint8_t* src_argb1 = src_argb + src_stride_argb;
  v128_t mask = SplatConst16(0xff);
  v128_t two = SplatConst16(2);
  v128_t ku02 = SplatPair16(c->kRGBToU[0], c->kRGBToU[2]);
  v128_t ku13 = SplatPair16(c->kRGBToU[1], c->kRGBToU[3]);
  v128_t kv02 = SplatPair16(c->kRGBToV[0], c->kRGBToV[2]);
  v128_t kv13 = SplatPair16(c->kRGBToV[1], c->kRGBToV[3]);
  v128_t bias = SplatConst16(c->kAddUV[0]);
  do {
    v128_t lo[2], hi[2];
    for (int i = 0; i < 2; ++i) {
      v128_t a0 = wasm_v128_load(src_argb + i * 32);
      v128_t a1 = wasm_v128_load(src_argb + i * 32 + 16);
      v128_t b0 = wasm_v128_load(src_argb1 + i * 32);
      v128_t b1 = wasm_v128_load(src_argb1 + i * 32 + 16);
      lo[i] =
          Avg2x2Pairs(wasm_v128_and(a0, mask), wasm_v128_and(a1, mask),
                      wasm_v128_and(b0, mask), wasm_v128_and(b1, mask), two);
      hi[i] = Avg2x2Pairs(wasm_u16x8_shr(a0, 8), wasm_u16x8_shr(a1, 8),
                          wasm_u16x8_shr(b0, 8), wasm_u16x8_shr(b1, 8), two);
    }
    PairsToUV8(lo, hi, ku02, ku13, kv02, kv13, bias, bias, dst_u, dst_v);
    src_argb += 64;
    src_argb1 += 64;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_ARGBTOUVMATRIXROW_WASMSIMD

#ifdef HAS_ARGBTOUVMATRIXROW_RELAXEDSIMD
static const uint8_t kShuffleARGBPairs[32] = {0, 4, 2, 6, 8, 12, 10, 14,
                                              1, 5, 3, 7, 9, 13, 11, 15};
void ARGBToUVMatrixRow_RELAXEDSIMD(const uint8_t* src_argb,
                                   int src_stride_argb,
                                   uint8_t* dst_u,
                                   uint8_t* dst_v,
                                   int width,
                                   const struct ArgbConstants* c) {
  const uint8_t* src_argb1 = src_argb + src_stride_argb;
  v128_t shuf = LoadConst(kShuffleARGBPairs);
  v128_t x80 = SplatConst8(-128);
  v128_t one = SplatConst8(1);
  v128_t bias514 = SplatConst16(512 + 2);
  v128_t ku02 = SplatPair16(c->kRGBToU[0], c->kRGBToU[2]);
  v128_t ku13 = SplatPair16(c->kRGBToU[1], c->kRGBToU[3]);
  v128_t kv02 = SplatPair16(c->kRGBToV[0], c->kRGBToV[2]);
  v128_t kv13 = SplatPair16(c->kRGBToV[1], c->kRGBToV[3]);
  v128_t bias = SplatConst16(c->kAddUV[0]);
  do {
    v128_t lo[2], hi[2];
    for (int i = 0; i < 2; ++i) {
      v128_t a0 =
          wasm_i8x16_relaxed_swizzle(wasm_v128_load(src_argb + i * 32), shuf);
      v128_t a1 = wasm_i8x16_relaxed_swizzle(
          wasm_v128_load(src_argb + i * 32 + 16), shuf);
      v128_t b0 =
          wasm_i8x16_relaxed_swizzle(wasm_v128_load(src_argb1 + i * 32), shuf);
      v128_t b1 = wasm_i8x16_relaxed_swizzle(
          wasm_v128_load(src_argb1 + i * 32 + 16), shuf);
      v128_t s0 = wasm_i16x8_add(
          wasm_i16x8_relaxed_dot_i8x16_i7x16(wasm_v128_xor(a0, x80), one),
          wasm_i16x8_relaxed_dot_i8x16_i7x16(wasm_v128_xor(b0, x80), one));
      v128_t s1 = wasm_i16x8_add(
          wasm_i16x8_relaxed_dot_i8x16_i7x16(wasm_v128_xor(a1, x80), one),
          wasm_i16x8_relaxed_dot_i8x16_i7x16(wasm_v128_xor(b1, x80), one));
      s0 = wasm_u16x8_shr(wasm_i16x8_add(s0, bias514), 2);
      s1 = wasm_u16x8_shr(wasm_i16x8_add(s1, bias514), 2);
      lo[i] = wasm_i64x2_shuffle(s0, s1, 0, 2);
      hi[i] = wasm_i64x2_shuffle(s0, s1, 1, 3);
    }
    PairsToUV8(lo, hi, ku02, ku13, kv02, kv13, bias, bias, dst_u, dst_v);
    src_argb += 64;
    src_argb1 += 64;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_ARGBTOUVMATRIXROW_RELAXEDSIMD

#if defined(HAS_RGBTOUVMATRIXROW_WASMSIMD) || \
    defined(HAS_RGBTOUVMATRIXROW_RELAXEDSIMD)
// Swizzles 4 RGB24 pixels to 16 bit {b, r} and {g, 0} pairs. The second
// pair of masks is for the load at offset 32 (avoids over-read).
static const uint8_t kShuffleRGBToPairs[64] = {
    0, 128, 2,   128, 3, 128, 5,   128, 6,  128, 8,   128, 9,  128, 11,  128,
    1, 128, 128, 128, 4, 128, 128, 128, 7,  128, 128, 128, 10, 128, 128, 128,
    4, 128, 6,   128, 7, 128, 9,   128, 10, 128, 12,  128, 13, 128, 15,  128,
    5, 128, 128, 128, 8, 128, 128, 128, 11, 128, 128, 128, 14, 128, 128, 128};

template <bool kRelaxed>
static inline void RGBToUVMatrixRow_Wasm(const uint8_t* src_rgb,
                                         int src_stride_rgb,
                                         uint8_t* dst_u,
                                         uint8_t* dst_v,
                                         int width,
                                         const struct ArgbConstants* c) {
  const uint8_t* src_rgb1 = src_rgb + src_stride_rgb;
  v128_t m[4];
  for (int i = 0; i < 4; ++i) {
    m[i] = LoadConst(kShuffleRGBToPairs + i * 16);
  }
  v128_t two = SplatConst16(2);
  v128_t ku02 = SplatPair16(c->kRGBToU[0], c->kRGBToU[2]);
  v128_t ku13 = SplatPair16(c->kRGBToU[1], 0);
  v128_t kv02 = SplatPair16(c->kRGBToV[0], c->kRGBToV[2]);
  v128_t kv13 = SplatPair16(c->kRGBToV[1], 0);
  v128_t bias_u = SplatConst16(c->kAddUV[0] - 255 * c->kRGBToU[3]);
  v128_t bias_v = SplatConst16(c->kAddUV[0] - 255 * c->kRGBToV[3]);
  do {
    v128_t lo[2], hi[2];
    for (int i = 0; i < 2; ++i) {
      // Pixels 8 * i to 8 * i + 3 at offset 24 * i, the rest at 12 or 32.
      int off = i ? 32 : 12;
      v128_t a0 = wasm_v128_load(src_rgb + i * 24);
      v128_t a1 = wasm_v128_load(src_rgb + off);
      v128_t b0 = wasm_v128_load(src_rgb1 + i * 24);
      v128_t b1 = wasm_v128_load(src_rgb1 + off);
      v128_t ml = m[i * 2];
      v128_t mh = m[i * 2 + 1];
      lo[i] = Avg2x2Pairs(
          WasmSwizzle<kRelaxed>(a0, m[0]), WasmSwizzle<kRelaxed>(a1, ml),
          WasmSwizzle<kRelaxed>(b0, m[0]), WasmSwizzle<kRelaxed>(b1, ml), two);
      hi[i] = Avg2x2Pairs(
          WasmSwizzle<kRelaxed>(a0, m[1]), WasmSwizzle<kRelaxed>(a1, mh),
          WasmSwizzle<kRelaxed>(b0, m[1]), WasmSwizzle<kRelaxed>(b1, mh), two);
    }
    PairsToUV8(lo, hi, ku02, ku13, kv02, kv13, bias_u, bias_v, dst_u, dst_v);
    src_rgb += 48;
    src_rgb1 += 48;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}
#ifdef HAS_RGBTOUVMATRIXROW_WASMSIMD
void RGBToUVMatrixRow_WASMSIMD(const uint8_t* src_rgb,
                               int src_stride_rgb,
                               uint8_t* dst_u,
                               uint8_t* dst_v,
                               int width,
                               const struct ArgbConstants* c) {
  RGBToUVMatrixRow_Wasm<false>(src_rgb, src_stride_rgb, dst_u, dst_v, width, c);
}
#endif  // HAS_RGBTOUVMATRIXROW_WASMSIMD
#ifdef HAS_RGBTOUVMATRIXROW_RELAXEDSIMD
void RGBToUVMatrixRow_RELAXEDSIMD(const uint8_t* src_rgb,
                                  int src_stride_rgb,
                                  uint8_t* dst_u,
                                  uint8_t* dst_v,
                                  int width,
                                  const struct ArgbConstants* c) {
  RGBToUVMatrixRow_Wasm<true>(src_rgb, src_stride_rgb, dst_u, dst_v, width, c);
}
#endif  // HAS_RGBTOUVMATRIXROW_RELAXEDSIMD
#endif  // HAS_RGBTOUVMATRIXROW_WASMSIMD || HAS_RGBTOUVMATRIXROW_RELAXEDSIMD

// 16 bit pixel formats keep each channel in its own 16 bit lane.
// 5 and 6 bit channels expand as (v * 33) >> 2 and (v * 65) >> 4, and 4 bit
// channels as v * 17, which match the C bit replication.

#if defined(HAS_RGB565TOUVMATRIXROW_WASMSIMD) ||   \
    defined(HAS_ARGB1555TOUVMATRIXROW_WASMSIMD) || \
    defined(HAS_ARGB4444TOUVMATRIXROW_WASMSIMD)
// Rounded 2x2 average of 16 pixels of 1 channel. a = row 0, b = row 1.
static inline v128_t Avg2x2Lanes(v128_t a0,
                                 v128_t a1,
                                 v128_t b0,
                                 v128_t b1,
                                 v128_t two) {
  v128_t s = HAddPairs16(wasm_i16x8_add(a0, b0), wasm_i16x8_add(a1, b1));
  return wasm_u16x8_shr(wasm_i16x8_add(s, two), 2);
}

// Stores 8 U and 8 V from 8 pixels of 16 bit channels. k holds U
// coefficients for b, g, r, a, then V coefficients, then bias U and V.
static inline void ChannelsToUV8(v128_t b,
                                 v128_t g,
                                 v128_t r,
                                 v128_t a,
                                 const v128_t* k,
                                 uint8_t* dst_u,
                                 uint8_t* dst_v) {
  v128_t u =
      wasm_i16x8_add(MulAdd16(b, g, k[0], k[1]), MulAdd16(r, a, k[2], k[3]));
  v128_t v =
      wasm_i16x8_add(MulAdd16(b, g, k[4], k[5]), MulAdd16(r, a, k[6], k[7]));
  v128_t uv = wasm_u8x16_narrow_i16x8(SubShr8(k[8], u), SubShr8(k[9], v));
  wasm_v128_store64_lane(dst_u, uv, 0);
  wasm_v128_store64_lane(dst_v, uv, 1);
}

// U and V coefficients for ChannelsToUV8. The alpha term is folded into the
// bias if alpha is always 255 (opaque).
static inline void LoadUVChannelConstants(const struct ArgbConstants* c,
                                          int opaque,
                                          v128_t* k) {
  for (int i = 0; i < 4; ++i) {
    k[i] = SplatConst16(c->kRGBToU[i]);
    k[i + 4] = SplatConst16(c->kRGBToV[i]);
  }
  k[8] = SplatConst16(c->kAddUV[0] - opaque * 255 * c->kRGBToU[3]);
  k[9] = SplatConst16(c->kAddUV[0] - opaque * 255 * c->kRGBToV[3]);
}
#endif
#ifdef HAS_RGB565TOYMATRIXROW_WASMSIMD
void RGB565ToYMatrixRow_WASMSIMD(const uint8_t* src_rgb565,
                                 uint8_t* dst_y,
                                 int width,
                                 const struct ArgbConstants* c) {
  v128_t m5 = SplatConst16(0x1f);
  v128_t m6 = SplatConst16(0x3f);
  v128_t k33 = SplatConst16(33);
  v128_t k65 = SplatConst16(65);
  v128_t kb = SplatConst16(c->kRGBToY[0]);
  v128_t kg = SplatConst16(c->kRGBToY[1]);
  v128_t kr = SplatConst16(c->kRGBToY[2]);
  v128_t bias = SplatConst16(c->kAddY[0] + 255 * c->kRGBToY[3]);
  do {
    v128_t y[2];
    for (int i = 0; i < 2; ++i) {
      v128_t x = wasm_v128_load(src_rgb565 + i * 16);
      v128_t b = wasm_u16x8_shr(wasm_i16x8_mul(wasm_v128_and(x, m5), k33), 2);
      v128_t g = wasm_u16x8_shr(
          wasm_i16x8_mul(wasm_v128_and(wasm_u16x8_shr(x, 5), m6), k65), 4);
      v128_t r = wasm_u16x8_shr(wasm_i16x8_mul(wasm_u16x8_shr(x, 11), k33), 2);
      v128_t s = wasm_i16x8_add(wasm_i16x8_mul(b, kb), bias);
      y[i] = wasm_u16x8_shr(wasm_i16x8_add(s, MulAdd16(g, r, kg, kr)), 8);
    }
    wasm_v128_store(dst_y, wasm_u8x16_narrow_i16x8(y[0], y[1]));
    src_rgb565 += 32;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_RGB565TOYMATRIXROW_WASMSIMD

#ifdef HAS_ARGB1555TOYMATRIXROW_WASMSIMD
void ARGB1555ToYMatrixRow_WASMSIMD(const uint8_t* src_argb1555,
                                   uint8_t* dst_y,
                                   int width,
                                   const struct ArgbConstants* c) {
  v128_t m5 = SplatConst16(0x1f);
  v128_t k33 = SplatConst16(33);
  v128_t kb = SplatConst16(c->kRGBToY[0]);
  v128_t kg = SplatConst16(c->kRGBToY[1]);
  v128_t kr = SplatConst16(c->kRGBToY[2]);
  v128_t ka = SplatConst16(255 * c->kRGBToY[3]);
  v128_t bias = SplatConst16(c->kAddY[0]);
  do {
    v128_t y[2];
    for (int i = 0; i < 2; ++i) {
      v128_t x = wasm_v128_load(src_argb1555 + i * 16);
      v128_t b = wasm_u16x8_shr(wasm_i16x8_mul(wasm_v128_and(x, m5), k33), 2);
      v128_t g = wasm_u16x8_shr(
          wasm_i16x8_mul(wasm_v128_and(wasm_u16x8_shr(x, 5), m5), k33), 2);
      v128_t r = wasm_u16x8_shr(
          wasm_i16x8_mul(wasm_v128_and(wasm_u16x8_shr(x, 10), m5), k33), 2);
      // Alpha is 0 or 255.
      v128_t s = wasm_i16x8_add(wasm_v128_and(wasm_i16x8_shr(x, 15), ka), bias);
      s = wasm_i16x8_add(s, wasm_i16x8_mul(b, kb));
      y[i] = wasm_u16x8_shr(wasm_i16x8_add(s, MulAdd16(g, r, kg, kr)), 8);
    }
    wasm_v128_store(dst_y, wasm_u8x16_narrow_i16x8(y[0], y[1]));
    src_argb1555 += 32;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_ARGB1555TOYMATRIXROW_WASMSIMD

#ifdef HAS_ARGB4444TOYMATRIXROW_WASMSIMD
// 4 bit channels expand as v * 17, so the coefficients are scaled by 17.
void ARGB4444ToYMatrixRow_WASMSIMD(const uint8_t* src_argb4444,
                                   uint8_t* dst_y,
                                   int width,
                                   const struct ArgbConstants* c) {
  v128_t m4 = SplatConst16(0x0f);
  v128_t kb = SplatConst16(c->kRGBToY[0] * 17);
  v128_t kg = SplatConst16(c->kRGBToY[1] * 17);
  v128_t kr = SplatConst16(c->kRGBToY[2] * 17);
  v128_t ka = SplatConst16(c->kRGBToY[3] * 17);
  v128_t bias = SplatConst16(c->kAddY[0]);
  do {
    v128_t y[2];
    for (int i = 0; i < 2; ++i) {
      v128_t x = wasm_v128_load(src_argb4444 + i * 16);
      v128_t s = wasm_i16x8_add(
          MulAdd16(wasm_v128_and(x, m4), wasm_u16x8_shr(x, 12), kb, ka), bias);
      s = wasm_i16x8_add(
          s, MulAdd16(wasm_v128_and(wasm_u16x8_shr(x, 4), m4),
                      wasm_v128_and(wasm_u16x8_shr(x, 8), m4), kg, kr));
      y[i] = wasm_u16x8_shr(s, 8);
    }
    wasm_v128_store(dst_y, wasm_u8x16_narrow_i16x8(y[0], y[1]));
    src_argb4444 += 32;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_ARGB4444TOYMATRIXROW_WASMSIMD

#ifdef HAS_RGB565TOUVMATRIXROW_WASMSIMD
void RGB565ToUVMatrixRow_WASMSIMD(const uint8_t* src_rgb565,
                                  int src_stride_rgb565,
                                  uint8_t* dst_u,
                                  uint8_t* dst_v,
                                  int width,
                                  const struct ArgbConstants* c) {
  const uint8_t* src_rgb565_1 = src_rgb565 + src_stride_rgb565;
  v128_t m5 = SplatConst16(0x1f);
  v128_t m6 = SplatConst16(0x3f);
  v128_t k33 = SplatConst16(33);
  v128_t k65 = SplatConst16(65);
  v128_t two = SplatConst16(2);
  v128_t k[10];
  LoadUVChannelConstants(c, 1, k);
  do {
    v128_t b[4], g[4], r[4];
    for (int i = 0; i < 4; ++i) {
      v128_t x =
          wasm_v128_load((i & 2 ? src_rgb565_1 : src_rgb565) + (i & 1) * 16);
      b[i] = wasm_u16x8_shr(wasm_i16x8_mul(wasm_v128_and(x, m5), k33), 2);
      g[i] = wasm_u16x8_shr(
          wasm_i16x8_mul(wasm_v128_and(wasm_u16x8_shr(x, 5), m6), k65), 4);
      r[i] = wasm_u16x8_shr(wasm_i16x8_mul(wasm_u16x8_shr(x, 11), k33), 2);
    }
    ChannelsToUV8(Avg2x2Lanes(b[0], b[1], b[2], b[3], two),
                  Avg2x2Lanes(g[0], g[1], g[2], g[3], two),
                  Avg2x2Lanes(r[0], r[1], r[2], r[3], two),
                  wasm_i16x8_const_splat(0), k, dst_u, dst_v);
    src_rgb565 += 32;
    src_rgb565_1 += 32;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_RGB565TOUVMATRIXROW_WASMSIMD

#ifdef HAS_ARGB1555TOUVMATRIXROW_WASMSIMD
void ARGB1555ToUVMatrixRow_WASMSIMD(const uint8_t* src_argb1555,
                                    int src_stride_argb1555,
                                    uint8_t* dst_u,
                                    uint8_t* dst_v,
                                    int width,
                                    const struct ArgbConstants* c) {
  const uint8_t* src_argb1555_1 = src_argb1555 + src_stride_argb1555;
  v128_t m5 = SplatConst16(0x1f);
  v128_t k33 = SplatConst16(33);
  v128_t two = SplatConst16(2);
  v128_t k[10];
  LoadUVChannelConstants(c, 0, k);
  do {
    v128_t b[4], g[4], r[4], a[4];
    for (int i = 0; i < 4; ++i) {
      v128_t x = wasm_v128_load((i & 2 ? src_argb1555_1 : src_argb1555) +
                                (i & 1) * 16);
      b[i] = wasm_u16x8_shr(wasm_i16x8_mul(wasm_v128_and(x, m5), k33), 2);
      g[i] = wasm_u16x8_shr(
          wasm_i16x8_mul(wasm_v128_and(wasm_u16x8_shr(x, 5), m5), k33), 2);
      r[i] = wasm_u16x8_shr(
          wasm_i16x8_mul(wasm_v128_and(wasm_u16x8_shr(x, 10), m5), k33), 2);
      a[i] = wasm_u16x8_shr(wasm_i16x8_shr(x, 15), 8);
    }
    ChannelsToUV8(Avg2x2Lanes(b[0], b[1], b[2], b[3], two),
                  Avg2x2Lanes(g[0], g[1], g[2], g[3], two),
                  Avg2x2Lanes(r[0], r[1], r[2], r[3], two),
                  Avg2x2Lanes(a[0], a[1], a[2], a[3], two), k, dst_u, dst_v);
    src_argb1555 += 32;
    src_argb1555_1 += 32;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_ARGB1555TOUVMATRIXROW_WASMSIMD

#ifdef HAS_ARGB4444TOUVMATRIXROW_WASMSIMD
// Sums the nibble pairs {b, r} and {g, a} of 2x2 pixels in bytes, then
// expands the sums by 17 before rounding.
void ARGB4444ToUVMatrixRow_WASMSIMD(const uint8_t* src_argb4444,
                                    int src_stride_argb4444,
                                    uint8_t* dst_u,
                                    uint8_t* dst_v,
                                    int width,
                                    const struct ArgbConstants* c) {
  const uint8_t* src_argb4444_1 = src_argb4444 + src_stride_argb4444;
  v128_t m4 = SplatConst16(0x0f0f);
  v128_t m8 = SplatConst16(0xff);
  v128_t k17 = SplatConst16(17);
  v128_t two = SplatConst16(2);
  v128_t k[10];
  LoadUVChannelConstants(c, 0, k);
  do {
    v128_t s[2], t[2];
    for (int i = 0; i < 2; ++i) {
      v128_t a = wasm_v128_load(src_argb4444 + i * 16);
      v128_t b = wasm_v128_load(src_argb4444_1 + i * 16);
      s[i] = wasm_i16x8_add(wasm_v128_and(a, m4), wasm_v128_and(b, m4));
      t[i] = wasm_i16x8_add(wasm_v128_and(wasm_u16x8_shr(a, 4), m4),
                            wasm_v128_and(wasm_u16x8_shr(b, 4), m4));
    }
    v128_t br = HAddPairs16(s[0], s[1]);
    v128_t ga = HAddPairs16(t[0], t[1]);
    v128_t ch[4] = {wasm_v128_and(br, m8), wasm_v128_and(ga, m8),
                    wasm_u16x8_shr(br, 8), wasm_u16x8_shr(ga, 8)};
    for (int i = 0; i < 4; ++i) {
      ch[i] =
          wasm_u16x8_shr(wasm_i16x8_add(wasm_i16x8_mul(ch[i], k17), two), 2);
    }
    ChannelsToUV8(ch[0], ch[1], ch[2], ch[3], k, dst_u, dst_v);
    src_argb4444 += 32;
    src_argb4444_1 += 32;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_ARGB4444TOUVMATRIXROW_WASMSIMD

#ifdef HAS_CONVERT8TO16ROW_WASMSIMD
// For bits 8 to 16, (y * 0x0101) >> (16 - bits) == p + (p >> 8) where
// p = y << (bits - 8). This avoids a variable shift count.
void Convert8To16Row_WASMSIMD(const uint8_t* src_y,
                              uint16_t* dst_y,
                              int bits,
                              int width) {
  if (bits < 8) {
    Convert8To16Row_C(src_y, dst_y, bits, width);
    return;
  }
  v128_t scale = SplatConst16(1 << (bits - 8));
  do {
    asm("" : "+r"(dst_y));
    v128_t y = wasm_v128_load(src_y);
    v128_t y0 = wasm_i16x8_mul(wasm_u16x8_extend_low_u8x16(y), scale);
    v128_t y1 = wasm_i16x8_mul(wasm_u16x8_extend_high_u8x16(y), scale);
    wasm_v128_store(dst_y, wasm_i16x8_add(y0, wasm_u16x8_shr(y0, 8)));
    wasm_v128_store(dst_y + 8, wasm_i16x8_add(y1, wasm_u16x8_shr(y1, 8)));
    src_y += 16;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_CONVERT8TO16ROW_WASMSIMD

#ifdef HAS_MULTIPLYROW_16_WASMSIMD
void MultiplyRow_16_WASMSIMD(const uint16_t* src_y,
                             uint16_t* dst_y,
                             int scale,
                             int width) {
  v128_t vscale = wasm_i16x8_splat(scale);
  do {
    asm("" : "+r"(src_y), "+r"(dst_y));
    v128_t y0 = wasm_v128_load(src_y);
    v128_t y1 = wasm_v128_load(src_y + 8);
    y0 = wasm_i16x8_mul(y0, vscale);
    y1 = wasm_i16x8_mul(y1, vscale);
    wasm_v128_store(dst_y, y0);
    wasm_v128_store(dst_y + 8, y1);
    src_y += 16;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_MULTIPLYROW_16_WASMSIMD

#ifdef HAS_DIVIDEROW_16_WASMSIMD
void DivideRow_16_WASMSIMD(const uint16_t* src_y,
                           uint16_t* dst_y,
                           int scale,
                           int width) {
  v128_t vscale = wasm_i32x4_splat(scale);
  do {
    asm("" : "+r"(src_y), "+r"(dst_y));
    v128_t y0 = wasm_v128_load(src_y);
    v128_t y1 = wasm_v128_load(src_y + 8);
    v128_t lo0 = wasm_i32x4_mul(wasm_u32x4_extend_low_u16x8(y0), vscale);
    v128_t hi0 = wasm_i32x4_mul(wasm_u32x4_extend_high_u16x8(y0), vscale);
    v128_t lo1 = wasm_i32x4_mul(wasm_u32x4_extend_low_u16x8(y1), vscale);
    v128_t hi1 = wasm_i32x4_mul(wasm_u32x4_extend_high_u16x8(y1), vscale);
    wasm_v128_store(dst_y,
                    wasm_i16x8_shuffle(lo0, hi0, 1, 3, 5, 7, 9, 11, 13, 15));
    wasm_v128_store(dst_y + 8,
                    wasm_i16x8_shuffle(lo1, hi1, 1, 3, 5, 7, 9, 11, 13, 15));
    src_y += 16;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_DIVIDEROW_16_WASMSIMD

#ifdef HAS_CONVERT16TO8ROW_WASMSIMD
void Convert16To8Row_WASMSIMD(const uint16_t* src_y,
                              uint8_t* dst_y,
                              int scale,
                              int width) {
  v128_t vscale = wasm_i16x8_splat(scale);
  do {
    asm("" : "+r"(src_y));
    v128_t y0 = wasm_v128_load(src_y);
    v128_t y1 = wasm_v128_load(src_y + 8);
    v128_t lo0 = wasm_u32x4_extmul_low_u16x8(y0, vscale);
    v128_t hi0 = wasm_u32x4_extmul_high_u16x8(y0, vscale);
    v128_t lo1 = wasm_u32x4_extmul_low_u16x8(y1, vscale);
    v128_t hi1 = wasm_u32x4_extmul_high_u16x8(y1, vscale);
    v128_t r0 = wasm_i16x8_shuffle(lo0, hi0, 1, 3, 5, 7, 9, 11, 13, 15);
    v128_t r1 = wasm_i16x8_shuffle(lo1, hi1, 1, 3, 5, 7, 9, 11, 13, 15);
    wasm_v128_store(dst_y, wasm_u8x16_narrow_i16x8(r0, r1));
    src_y += 16;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_CONVERT16TO8ROW_WASMSIMD

#ifdef HAS_HALFMERGEUVROW_WASMSIMD
void HalfMergeUVRow_WASMSIMD(const uint8_t* src_u,
                             int src_stride_u,
                             const uint8_t* src_v,
                             int src_stride_v,
                             uint8_t* dst_uv,
                             int width) {
  const uint8_t* src_u1 = src_u + src_stride_u;
  const uint8_t* src_v1 = src_v + src_stride_v;
  v128_t c2 = SplatConst16(2);
  do {
    asm("" : "+r"(src_u), "+r"(src_u1), "+r"(src_v), "+r"(src_v1));
    v128_t u = wasm_i16x8_add(
        wasm_u16x8_extadd_pairwise_u8x16(wasm_v128_load(src_u)),
        wasm_u16x8_extadd_pairwise_u8x16(wasm_v128_load(src_u1)));
    v128_t v = wasm_i16x8_add(
        wasm_u16x8_extadd_pairwise_u8x16(wasm_v128_load(src_v)),
        wasm_u16x8_extadd_pairwise_u8x16(wasm_v128_load(src_v1)));
    u = wasm_u16x8_shr(wasm_i16x8_add(u, c2), 2);
    v = wasm_u16x8_shr(wasm_i16x8_add(v, c2), 2);
    wasm_v128_store(dst_uv, wasm_v128_or(u, wasm_i16x8_shl(v, 8)));
    src_u += 16;
    src_u1 += 16;
    src_v += 16;
    src_v1 += 16;
    dst_uv += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_HALFMERGEUVROW_WASMSIMD

#ifdef HAS_MERGEUVROW_WASMSIMD
void MergeUVRow_WASMSIMD(const uint8_t* src_u,
                         const uint8_t* src_v,
                         uint8_t* dst_uv,
                         int width) {
  do {
    asm("" : "+r"(dst_uv));
    v128_t u = wasm_v128_load(src_u);
    v128_t v = wasm_v128_load(src_v);
    v128_t uv0 = wasm_i8x16_shuffle(u, v, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                    21, 6, 22, 7, 23);
    v128_t uv1 = wasm_i8x16_shuffle(u, v, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28,
                                    13, 29, 14, 30, 15, 31);
    wasm_v128_store(dst_uv, uv0);
    wasm_v128_store(dst_uv + 16, uv1);
    src_u += 16;
    src_v += 16;
    dst_uv += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_MERGEUVROW_WASMSIMD

#ifdef HAS_MERGEUVROW_16_WASMSIMD
void MergeUVRow_16_WASMSIMD(const uint16_t* src_u,
                            const uint16_t* src_v,
                            uint16_t* dst_uv,
                            int depth,
                            int width) {
  int shift = 16 - depth;
  do {
    asm("" : "+r"(dst_uv));
    v128_t u = wasm_i16x8_shl(wasm_v128_load(src_u), shift);
    v128_t v = wasm_i16x8_shl(wasm_v128_load(src_v), shift);
    v128_t uv0 = wasm_i16x8_shuffle(u, v, 0, 8, 1, 9, 2, 10, 3, 11);
    v128_t uv1 = wasm_i16x8_shuffle(u, v, 4, 12, 5, 13, 6, 14, 7, 15);
    wasm_v128_store(dst_uv, uv0);
    wasm_v128_store(dst_uv + 8, uv1);
    src_u += 8;
    src_v += 8;
    dst_uv += 16;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_MERGEUVROW_16_WASMSIMD

#ifdef HAS_SPLITUVROW_WASMSIMD
void SplitUVRow_WASMSIMD(const uint8_t* src_uv,
                         uint8_t* dst_u,
                         uint8_t* dst_v,
                         int width) {
  do {
    asm("" : "+r"(src_uv));
    v128_t uv0 = wasm_v128_load(src_uv);
    v128_t uv1 = wasm_v128_load(src_uv + 16);
    v128_t u = wasm_i8x16_shuffle(uv0, uv1, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18,
                                  20, 22, 24, 26, 28, 30);
    v128_t v = wasm_i8x16_shuffle(uv0, uv1, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
                                  21, 23, 25, 27, 29, 31);
    wasm_v128_store(dst_u, u);
    wasm_v128_store(dst_v, v);
    src_uv += 32;
    dst_u += 16;
    dst_v += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_SPLITUVROW_WASMSIMD

#ifdef HAS_SPLITUVROW_16_WASMSIMD
void SplitUVRow_16_WASMSIMD(const uint16_t* src_uv,
                            uint16_t* dst_u,
                            uint16_t* dst_v,
                            int depth,
                            int width) {
  int shift = 16 - depth;
  do {
    asm("" : "+r"(src_uv));
    v128_t uv0 = wasm_v128_load(src_uv);
    v128_t uv1 = wasm_v128_load(src_uv + 8);
    v128_t u = wasm_u16x8_shr(
        wasm_i16x8_shuffle(uv0, uv1, 0, 2, 4, 6, 8, 10, 12, 14), shift);
    v128_t v = wasm_u16x8_shr(
        wasm_i16x8_shuffle(uv0, uv1, 1, 3, 5, 7, 9, 11, 13, 15), shift);
    wasm_v128_store(dst_u, u);
    wasm_v128_store(dst_v, v);
    src_uv += 16;
    dst_u += 8;
    dst_v += 8;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_SPLITUVROW_16_WASMSIMD

#if defined(HAS_SPLITRGBROW_WASMSIMD) || defined(HAS_SPLITRGBROW_RELAXEDSIMD)
static const uint8_t kShuffleSplitRGB[16] = {1, 4,  7,  10, 13, 0, 3,  6,
                                             9, 12, 15, 2,  5,  8, 11, 14};

template <bool kRelaxed>
static inline void SplitRGBRow_Wasm(const uint8_t* src_rgb,
                                    uint8_t* dst_r,
                                    uint8_t* dst_g,
                                    uint8_t* dst_b,
                                    int width) {
  v128_t shuf = LoadConst(kShuffleSplitRGB);
  do {
    asm("" : "+r"(src_rgb));
    v128_t s0 = WasmSwizzle<kRelaxed>(wasm_v128_load(src_rgb), shuf);
    v128_t s1 = WasmSwizzle<kRelaxed>(wasm_v128_load(src_rgb + 16), shuf);
    v128_t s2 = WasmSwizzle<kRelaxed>(wasm_v128_load(src_rgb + 32), shuf);
    v128_t s12_a = wasm_i8x16_shuffle(s1, s2, 11, 12, 13, 14, 15, 16, 17, 18,
                                      19, 20, 0, 0, 0, 0, 0, 0);
    v128_t r = wasm_i8x16_shuffle(s0, s12_a, 5, 6, 7, 8, 9, 10, 16, 17, 18, 19,
                                  20, 21, 22, 23, 24, 25);
    v128_t s01_b = wasm_i8x16_shuffle(s0, s1, 0, 1, 2, 3, 4, 21, 22, 23, 24, 25,
                                      26, 0, 0, 0, 0, 0);
    v128_t g = wasm_i8x16_shuffle(s01_b, s2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                  27, 28, 29, 30, 31);
    v128_t s01_c = wasm_i8x16_shuffle(s0, s1, 11, 12, 13, 14, 15, 16, 17, 18,
                                      19, 20, 0, 0, 0, 0, 0, 0);
    v128_t b = wasm_i8x16_shuffle(s01_c, s2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 21,
                                  22, 23, 24, 25, 26);
    wasm_v128_store(dst_r, r);
    wasm_v128_store(dst_g, g);
    wasm_v128_store(dst_b, b);
    src_rgb += 48;
    dst_r += 16;
    dst_g += 16;
    dst_b += 16;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_SPLITRGBROW_WASMSIMD
void SplitRGBRow_WASMSIMD(const uint8_t* src_rgb,
                          uint8_t* dst_r,
                          uint8_t* dst_g,
                          uint8_t* dst_b,
                          int width) {
  SplitRGBRow_Wasm<false>(src_rgb, dst_r, dst_g, dst_b, width);
}
#endif  // HAS_SPLITRGBROW_WASMSIMD

#ifdef HAS_SPLITRGBROW_RELAXEDSIMD
void SplitRGBRow_RELAXEDSIMD(const uint8_t* src_rgb,
                             uint8_t* dst_r,
                             uint8_t* dst_g,
                             uint8_t* dst_b,
                             int width) {
  SplitRGBRow_Wasm<true>(src_rgb, dst_r, dst_g, dst_b, width);
}
#endif  // HAS_SPLITRGBROW_RELAXEDSIMD
#endif  // HAS_SPLITRGBROW_WASMSIMD || HAS_SPLITRGBROW_RELAXEDSIMD

#if defined(HAS_MERGERGBROW_WASMSIMD) || defined(HAS_MERGERGBROW_RELAXEDSIMD)
static const uint8_t kMergeRGBSwizzle[96] = {
    0,   1,   128, 2,   3,   128, 4,   5,   128, 6,   7,   128, 8,   9,
    128, 10,  128, 128, 0,   128, 128, 1,   128, 128, 2,   128, 128, 3,
    128, 128, 4,   128, 3,   128, 4,   5,   128, 6,   7,   128, 8,   9,
    128, 10,  11,  128, 12,  13,  128, 5,   128, 128, 6,   128, 128, 7,
    128, 128, 8,   128, 128, 9,   128, 128, 128, 6,   7,   128, 8,   9,
    128, 10,  11,  128, 12,  13,  128, 14,  15,  128, 10,  128, 128, 11,
    128, 128, 12,  128, 128, 13,  128, 128, 14,  128, 128, 15};

template <bool kRelaxed>
static inline void MergeRGBRow_Wasm(const uint8_t* src_r,
                                    const uint8_t* src_g,
                                    const uint8_t* src_b,
                                    uint8_t* dst_rgb,
                                    int width) {
  v128_t m0 = LoadConst(kMergeRGBSwizzle);
  v128_t m1 = LoadConst(kMergeRGBSwizzle + 16);
  v128_t m2 = LoadConst(kMergeRGBSwizzle + 32);
  v128_t m3 = LoadConst(kMergeRGBSwizzle + 48);
  v128_t m4 = LoadConst(kMergeRGBSwizzle + 64);
  v128_t m5 = LoadConst(kMergeRGBSwizzle + 80);
  do {
    asm("" : "+r"(dst_rgb));
    v128_t r = wasm_v128_load(src_r);
    v128_t g = wasm_v128_load(src_g);
    v128_t b = wasm_v128_load(src_b);
    v128_t rg0 = wasm_i8x16_shuffle(r, g, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                    21, 6, 22, 7, 23);
    v128_t rg2 = wasm_i8x16_shuffle(r, g, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28,
                                    13, 29, 14, 30, 15, 31);
    v128_t rg1 = wasm_i8x16_shuffle(rg0, rg2, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                                    17, 18, 19, 20, 21, 22, 23);
    wasm_v128_store(dst_rgb, wasm_v128_or(WasmSwizzle<kRelaxed>(rg0, m0),
                                          WasmSwizzle<kRelaxed>(b, m1)));
    wasm_v128_store(dst_rgb + 16, wasm_v128_or(WasmSwizzle<kRelaxed>(rg1, m2),
                                               WasmSwizzle<kRelaxed>(b, m3)));
    wasm_v128_store(dst_rgb + 32, wasm_v128_or(WasmSwizzle<kRelaxed>(rg2, m4),
                                               WasmSwizzle<kRelaxed>(b, m5)));
    src_r += 16;
    src_g += 16;
    src_b += 16;
    dst_rgb += 48;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_MERGERGBROW_WASMSIMD
void MergeRGBRow_WASMSIMD(const uint8_t* src_r,
                          const uint8_t* src_g,
                          const uint8_t* src_b,
                          uint8_t* dst_rgb,
                          int width) {
  MergeRGBRow_Wasm<false>(src_r, src_g, src_b, dst_rgb, width);
}
#endif  // HAS_MERGERGBROW_WASMSIMD

#ifdef HAS_MERGERGBROW_RELAXEDSIMD
void MergeRGBRow_RELAXEDSIMD(const uint8_t* src_r,
                             const uint8_t* src_g,
                             const uint8_t* src_b,
                             uint8_t* dst_rgb,
                             int width) {
  MergeRGBRow_Wasm<true>(src_r, src_g, src_b, dst_rgb, width);
}
#endif  // HAS_MERGERGBROW_RELAXEDSIMD
#endif  // HAS_MERGERGBROW_WASMSIMD || HAS_MERGERGBROW_RELAXEDSIMD

#ifdef HAS_SPLITARGBROW_WASMSIMD
void SplitARGBRow_WASMSIMD(const uint8_t* src_argb,
                           uint8_t* dst_r,
                           uint8_t* dst_g,
                           uint8_t* dst_b,
                           uint8_t* dst_a,
                           int width) {
  do {
    asm("" : "+r"(src_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    v128_t s2 = wasm_v128_load(src_argb + 32);
    v128_t s3 = wasm_v128_load(src_argb + 48);
    v128_t bg01 = wasm_i8x16_shuffle(s0, s1, 0, 4, 8, 12, 16, 20, 24, 28, 1, 5,
                                     9, 13, 17, 21, 25, 29);
    v128_t bg23 = wasm_i8x16_shuffle(s2, s3, 0, 4, 8, 12, 16, 20, 24, 28, 1, 5,
                                     9, 13, 17, 21, 25, 29);
    v128_t ra01 = wasm_i8x16_shuffle(s0, s1, 2, 6, 10, 14, 18, 22, 26, 30, 3, 7,
                                     11, 15, 19, 23, 27, 31);
    v128_t ra23 = wasm_i8x16_shuffle(s2, s3, 2, 6, 10, 14, 18, 22, 26, 30, 3, 7,
                                     11, 15, 19, 23, 27, 31);
    wasm_v128_store(dst_b, wasm_i64x2_shuffle(bg01, bg23, 0, 2));
    wasm_v128_store(dst_g, wasm_i64x2_shuffle(bg01, bg23, 1, 3));
    wasm_v128_store(dst_r, wasm_i64x2_shuffle(ra01, ra23, 0, 2));
    wasm_v128_store(dst_a, wasm_i64x2_shuffle(ra01, ra23, 1, 3));
    src_argb += 64;
    dst_r += 16;
    dst_g += 16;
    dst_b += 16;
    dst_a += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_SPLITARGBROW_WASMSIMD

#ifdef HAS_MERGEARGBROW_WASMSIMD
void MergeARGBRow_WASMSIMD(const uint8_t* src_r,
                           const uint8_t* src_g,
                           const uint8_t* src_b,
                           const uint8_t* src_a,
                           uint8_t* dst_argb,
                           int width) {
  do {
    asm("" : "+r"(dst_argb));
    v128_t r = wasm_v128_load(src_r);
    v128_t g = wasm_v128_load(src_g);
    v128_t b = wasm_v128_load(src_b);
    v128_t a = wasm_v128_load(src_a);
    v128_t bg0 = wasm_i8x16_shuffle(b, g, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                    21, 6, 22, 7, 23);
    v128_t bg1 = wasm_i8x16_shuffle(b, g, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28,
                                    13, 29, 14, 30, 15, 31);
    v128_t ra0 = wasm_i8x16_shuffle(r, a, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                    21, 6, 22, 7, 23);
    v128_t ra1 = wasm_i8x16_shuffle(r, a, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28,
                                    13, 29, 14, 30, 15, 31);
    wasm_v128_store(dst_argb,
                    wasm_i16x8_shuffle(bg0, ra0, 0, 8, 1, 9, 2, 10, 3, 11));
    wasm_v128_store(dst_argb + 16,
                    wasm_i16x8_shuffle(bg0, ra0, 4, 12, 5, 13, 6, 14, 7, 15));
    wasm_v128_store(dst_argb + 32,
                    wasm_i16x8_shuffle(bg1, ra1, 0, 8, 1, 9, 2, 10, 3, 11));
    wasm_v128_store(dst_argb + 48,
                    wasm_i16x8_shuffle(bg1, ra1, 4, 12, 5, 13, 6, 14, 7, 15));
    src_r += 16;
    src_g += 16;
    src_b += 16;
    src_a += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_MERGEARGBROW_WASMSIMD

#ifdef HAS_SPLITXRGBROW_WASMSIMD
void SplitXRGBRow_WASMSIMD(const uint8_t* src_argb,
                           uint8_t* dst_r,
                           uint8_t* dst_g,
                           uint8_t* dst_b,
                           int width) {
  do {
    asm("" : "+r"(src_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    v128_t s2 = wasm_v128_load(src_argb + 32);
    v128_t s3 = wasm_v128_load(src_argb + 48);
    v128_t bg01 = wasm_i8x16_shuffle(s0, s1, 0, 4, 8, 12, 16, 20, 24, 28, 1, 5,
                                     9, 13, 17, 21, 25, 29);
    v128_t bg23 = wasm_i8x16_shuffle(s2, s3, 0, 4, 8, 12, 16, 20, 24, 28, 1, 5,
                                     9, 13, 17, 21, 25, 29);
    v128_t r01 = wasm_i8x16_shuffle(s0, s1, 2, 6, 10, 14, 18, 22, 26, 30, 2, 6,
                                    10, 14, 18, 22, 26, 30);
    v128_t r23 = wasm_i8x16_shuffle(s2, s3, 2, 6, 10, 14, 18, 22, 26, 30, 2, 6,
                                    10, 14, 18, 22, 26, 30);
    wasm_v128_store(dst_b, wasm_i64x2_shuffle(bg01, bg23, 0, 2));
    wasm_v128_store(dst_g, wasm_i64x2_shuffle(bg01, bg23, 1, 3));
    wasm_v128_store(dst_r, wasm_i64x2_shuffle(r01, r23, 0, 2));
    src_argb += 64;
    dst_r += 16;
    dst_g += 16;
    dst_b += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_SPLITXRGBROW_WASMSIMD

#ifdef HAS_MERGEXRGBROW_WASMSIMD
void MergeXRGBRow_WASMSIMD(const uint8_t* src_r,
                           const uint8_t* src_g,
                           const uint8_t* src_b,
                           uint8_t* dst_argb,
                           int width) {
  v128_t a = SplatConst8(0xff);
  do {
    asm("" : "+r"(dst_argb));
    v128_t r = wasm_v128_load(src_r);
    v128_t g = wasm_v128_load(src_g);
    v128_t b = wasm_v128_load(src_b);
    v128_t bg0 = wasm_i8x16_shuffle(b, g, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                    21, 6, 22, 7, 23);
    v128_t bg1 = wasm_i8x16_shuffle(b, g, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28,
                                    13, 29, 14, 30, 15, 31);
    v128_t ra0 = wasm_i8x16_shuffle(r, a, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                    21, 6, 22, 7, 23);
    v128_t ra1 = wasm_i8x16_shuffle(r, a, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28,
                                    13, 29, 14, 30, 15, 31);
    wasm_v128_store(dst_argb,
                    wasm_i16x8_shuffle(bg0, ra0, 0, 8, 1, 9, 2, 10, 3, 11));
    wasm_v128_store(dst_argb + 16,
                    wasm_i16x8_shuffle(bg0, ra0, 4, 12, 5, 13, 6, 14, 7, 15));
    wasm_v128_store(dst_argb + 32,
                    wasm_i16x8_shuffle(bg1, ra1, 0, 8, 1, 9, 2, 10, 3, 11));
    wasm_v128_store(dst_argb + 48,
                    wasm_i16x8_shuffle(bg1, ra1, 4, 12, 5, 13, 6, 14, 7, 15));
    src_r += 16;
    src_g += 16;
    src_b += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_MERGEXRGBROW_WASMSIMD

#if defined(HAS_MIRRORROW_WASMSIMD) || defined(HAS_MIRRORROW_RELAXEDSIMD)
static const uint8_t kShuffleMirror[16] = {15, 14, 13, 12, 11, 10, 9, 8,
                                           7,  6,  5,  4,  3,  2,  1, 0};

template <bool kRelaxed>
static inline void MirrorRow_Wasm(const uint8_t* src, uint8_t* dst, int width) {
  v128_t shuf = LoadConst(kShuffleMirror);
  src += width - 16;
  do {
    wasm_v128_store(dst, WasmSwizzle<kRelaxed>(wasm_v128_load(src), shuf));
    src -= 16;
    dst += 16;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_MIRRORROW_WASMSIMD
void MirrorRow_WASMSIMD(const uint8_t* src, uint8_t* dst, int width) {
  MirrorRow_Wasm<false>(src, dst, width);
}
#endif  // HAS_MIRRORROW_WASMSIMD

#ifdef HAS_MIRRORROW_RELAXEDSIMD
void MirrorRow_RELAXEDSIMD(const uint8_t* src, uint8_t* dst, int width) {
  MirrorRow_Wasm<true>(src, dst, width);
}
#endif  // HAS_MIRRORROW_RELAXEDSIMD
#endif  // HAS_MIRRORROW_WASMSIMD || HAS_MIRRORROW_RELAXEDSIMD

#if defined(HAS_MIRRORUVROW_WASMSIMD) || defined(HAS_MIRRORUVROW_RELAXEDSIMD)
static const uint8_t kShuffleMirrorUV[16] = {14, 15, 12, 13, 10, 11, 8, 9,
                                             6,  7,  4,  5,  2,  3,  0, 1};

template <bool kRelaxed>
static inline void MirrorUVRow_Wasm(const uint8_t* src_uv,
                                    uint8_t* dst_uv,
                                    int width) {
  v128_t shuf = LoadConst(kShuffleMirrorUV);
  src_uv += (width - 8) * 2;
  do {
    wasm_v128_store(dst_uv,
                    WasmSwizzle<kRelaxed>(wasm_v128_load(src_uv), shuf));
    src_uv -= 16;
    dst_uv += 16;
    width -= 8;
  } while (width > 0);
}

#ifdef HAS_MIRRORUVROW_WASMSIMD
void MirrorUVRow_WASMSIMD(const uint8_t* src_uv, uint8_t* dst_uv, int width) {
  MirrorUVRow_Wasm<false>(src_uv, dst_uv, width);
}
#endif  // HAS_MIRRORUVROW_WASMSIMD

#ifdef HAS_MIRRORUVROW_RELAXEDSIMD
void MirrorUVRow_RELAXEDSIMD(const uint8_t* src_uv,
                             uint8_t* dst_uv,
                             int width) {
  MirrorUVRow_Wasm<true>(src_uv, dst_uv, width);
}
#endif  // HAS_MIRRORUVROW_RELAXEDSIMD
#endif  // HAS_MIRRORUVROW_WASMSIMD || HAS_MIRRORUVROW_RELAXEDSIMD

#ifdef HAS_SWAPUVROW_WASMSIMD
void SwapUVRow_WASMSIMD(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
  do {
    // Keep constant offsets in the load/store instructions.
    asm("" : "+r"(src_uv), "+r"(dst_vu));
    v128_t uv0 = wasm_v128_load(src_uv);
    v128_t uv1 = wasm_v128_load(src_uv + 16);
    // 16 bit byte swap: rev16 on Arm, shifts on x86.
    wasm_v128_store(
        dst_vu, wasm_v128_or(wasm_i16x8_shl(uv0, 8), wasm_u16x8_shr(uv0, 8)));
    wasm_v128_store(dst_vu + 16, wasm_v128_or(wasm_i16x8_shl(uv1, 8),
                                              wasm_u16x8_shr(uv1, 8)));
    src_uv += 32;
    dst_vu += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_SWAPUVROW_WASMSIMD

#if defined(HAS_MIRRORSPLITUVROW_WASMSIMD) || \
    defined(HAS_MIRRORSPLITUVROW_RELAXEDSIMD)
// Reverses 8 UV pairs into 8 U then 8 V.
static const uint8_t kShuffleMirrorSplitUV[16] = {14, 12, 10, 8, 6, 4, 2, 0,
                                                  15, 13, 11, 9, 7, 5, 3, 1};

template <bool kRelaxed>
static inline void MirrorSplitUVRow_Wasm(const uint8_t* src_uv,
                                         uint8_t* dst_u,
                                         uint8_t* dst_v,
                                         int width) {
  v128_t shuf = LoadConst(kShuffleMirrorSplitUV);
  src_uv += (width - 16) * 2;
  do {
    asm("" : "+r"(src_uv));
    v128_t a = WasmSwizzle<kRelaxed>(wasm_v128_load(src_uv), shuf);
    v128_t b = WasmSwizzle<kRelaxed>(wasm_v128_load(src_uv + 16), shuf);
    wasm_v128_store(dst_u, wasm_i64x2_shuffle(b, a, 0, 2));
    wasm_v128_store(dst_v, wasm_i64x2_shuffle(b, a, 1, 3));
    src_uv -= 32;
    dst_u += 16;
    dst_v += 16;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_MIRRORSPLITUVROW_WASMSIMD
void MirrorSplitUVRow_WASMSIMD(const uint8_t* src_uv,
                               uint8_t* dst_u,
                               uint8_t* dst_v,
                               int width) {
  MirrorSplitUVRow_Wasm<false>(src_uv, dst_u, dst_v, width);
}
#endif  // HAS_MIRRORSPLITUVROW_WASMSIMD

#ifdef HAS_MIRRORSPLITUVROW_RELAXEDSIMD
void MirrorSplitUVRow_RELAXEDSIMD(const uint8_t* src_uv,
                                  uint8_t* dst_u,
                                  uint8_t* dst_v,
                                  int width) {
  MirrorSplitUVRow_Wasm<true>(src_uv, dst_u, dst_v, width);
}
#endif  // HAS_MIRRORSPLITUVROW_RELAXEDSIMD
#endif  // HAS_MIRRORSPLITUVROW_WASMSIMD || HAS_MIRRORSPLITUVROW_RELAXEDSIMD

#if defined(HAS_RGB24MIRRORROW_WASMSIMD) || \
    defined(HAS_RGB24MIRRORROW_RELAXEDSIMD)
// Each 16 byte output needs 18 source bytes, so it is the OR of 2 swizzles.
static const uint8_t kShuffleRGB24Mirror[96] = {
    13,  14,  15,  10,  11,  12,  7,   8,   9,   4,   5,   6,   1,   2,
    3,   128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 14,  15,  128, 11,  12,  13,  8,   9,   10,  5,   6,
    7,   2,   3,   4,   128, 0,   128, 1,   128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 0,   128, 128, 12,  13,  14,  9,   10,
    11,  6,   7,   8,   3,   4,   5,   0,   1,   2,   1,   128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128};

template <bool kRelaxed>
static inline void RGB24MirrorRow_Wasm(const uint8_t* src_rgb24,
                                       uint8_t* dst_rgb24,
                                       int width) {
  const uint8_t* shuf = kShuffleRGB24Mirror;
  v128_t m0a = LoadConst(shuf);
  v128_t m0b = LoadConst(shuf + 16);
  v128_t m1a = LoadConst(shuf + 32);
  v128_t m1b = LoadConst(shuf + 48);
  v128_t m2a = LoadConst(shuf + 64);
  v128_t m2b = LoadConst(shuf + 80);
  src_rgb24 += (width - 16) * 3;
  do {
    asm("" : "+r"(src_rgb24), "+r"(dst_rgb24));
    v128_t s0 = wasm_v128_load(src_rgb24);
    v128_t s1 = wasm_v128_load(src_rgb24 + 16);
    v128_t s2 = wasm_v128_load(src_rgb24 + 32);
    // Bytes 15 and 32 of the 48 source bytes.
    v128_t s02 = wasm_i8x16_shuffle(s0, s2, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                    24, 25, 26, 27, 28, 29, 30);
    wasm_v128_store(dst_rgb24, wasm_v128_or(WasmSwizzle<kRelaxed>(s2, m0a),
                                            WasmSwizzle<kRelaxed>(s1, m0b)));
    wasm_v128_store(dst_rgb24 + 16,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(s1, m1a),
                                 WasmSwizzle<kRelaxed>(s02, m1b)));
    wasm_v128_store(dst_rgb24 + 32,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(s0, m2a),
                                 WasmSwizzle<kRelaxed>(s1, m2b)));
    src_rgb24 -= 48;
    dst_rgb24 += 48;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_RGB24MIRRORROW_WASMSIMD
void RGB24MirrorRow_WASMSIMD(const uint8_t* src_rgb24,
                             uint8_t* dst_rgb24,
                             int width) {
  RGB24MirrorRow_Wasm<false>(src_rgb24, dst_rgb24, width);
}
#endif  // HAS_RGB24MIRRORROW_WASMSIMD

#ifdef HAS_RGB24MIRRORROW_RELAXEDSIMD
void RGB24MirrorRow_RELAXEDSIMD(const uint8_t* src_rgb24,
                                uint8_t* dst_rgb24,
                                int width) {
  RGB24MirrorRow_Wasm<true>(src_rgb24, dst_rgb24, width);
}
#endif  // HAS_RGB24MIRRORROW_RELAXEDSIMD
#endif  // HAS_RGB24MIRRORROW_WASMSIMD || HAS_RGB24MIRRORROW_RELAXEDSIMD

#ifdef HAS_ARGBMIRRORROW_WASMSIMD
void ARGBMirrorRow_WASMSIMD(const uint8_t* src, uint8_t* dst, int width) {
  src += (width - 4) * 4;
  do {
    v128_t v = wasm_v128_load(src);
    v = wasm_i32x4_shuffle(v, v, 3, 2, 1, 0);
    wasm_v128_store(dst, v);
    src -= 16;
    dst += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBMIRRORROW_WASMSIMD

#ifdef HAS_INTERPOLATEROW_WASMSIMD
void InterpolateRow_WASMSIMD(uint8_t* dst_ptr,
                             const uint8_t* src_ptr,
                             ptrdiff_t src_stride,
                             int width,
                             int source_y_fraction) {
  const uint8_t* src_ptr1 = src_ptr + src_stride;
  if (source_y_fraction == 0) {
    do {
      wasm_v128_store(dst_ptr, wasm_v128_load(src_ptr));
      src_ptr += 16;
      dst_ptr += 16;
      width -= 16;
    } while (width > 0);
    return;
  }
  if (source_y_fraction == 128) {
    do {
      v128_t s0 = wasm_v128_load(src_ptr);
      v128_t s1 = wasm_v128_load(src_ptr1);
      wasm_v128_store(dst_ptr, wasm_u8x16_avgr(s0, s1));
      src_ptr += 16;
      src_ptr1 += 16;
      dst_ptr += 16;
      width -= 16;
    } while (width > 0);
    return;
  }
  // s0 * (256 - f) + s1 * f + 128 fits in 16 bits. Both weights fit in 8
  // bits since f is 1 to 255. The high bytes are the result (uzp2 on Arm).
  v128_t w0 = SplatConst8(256 - source_y_fraction);
  v128_t w1 = SplatConst8(source_y_fraction);
  v128_t round = SplatConst16(128);
  do {
    v128_t s0 = wasm_v128_load(src_ptr);
    v128_t s1 = wasm_v128_load(src_ptr1);
    v128_t lo = wasm_i16x8_add(wasm_u16x8_extmul_low_u8x16(s0, w0),
                               wasm_u16x8_extmul_low_u8x16(s1, w1));
    v128_t hi = wasm_i16x8_add(wasm_u16x8_extmul_high_u8x16(s0, w0),
                               wasm_u16x8_extmul_high_u8x16(s1, w1));
    lo = wasm_i16x8_add(lo, round);
    hi = wasm_i16x8_add(hi, round);
    wasm_v128_store(
        dst_ptr, wasm_i8x16_shuffle(lo, hi, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
                                    21, 23, 25, 27, 29, 31));
    src_ptr += 16;
    src_ptr1 += 16;
    dst_ptr += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_INTERPOLATEROW_WASMSIMD

#ifdef HAS_INTERPOLATEROW_16_WASMSIMD
void InterpolateRow_16_WASMSIMD(uint16_t* dst_ptr,
                                const uint16_t* src_ptr,
                                ptrdiff_t src_stride,
                                int width,
                                int source_y_fraction) {
  const uint16_t* src_ptr1 = src_ptr + src_stride;
  if (source_y_fraction == 0) {
    do {
      wasm_v128_store(dst_ptr, wasm_v128_load(src_ptr));
      src_ptr += 8;
      dst_ptr += 8;
      width -= 8;
    } while (width > 0);
    return;
  }
  if (source_y_fraction == 128) {
    do {
      v128_t s0 = wasm_v128_load(src_ptr);
      v128_t s1 = wasm_v128_load(src_ptr1);
      wasm_v128_store(dst_ptr, wasm_u16x8_avgr(s0, s1));
      src_ptr += 8;
      src_ptr1 += 8;
      dst_ptr += 8;
      width -= 8;
    } while (width > 0);
    return;
  }
  // s0 * (256 - f) + s1 * f + 128 fits in 24 bits. Shift the result into
  // the high 16 bits of each lane, then take the odd 16 bit lanes.
  v128_t w0 = SplatConst16(256 - source_y_fraction);
  v128_t w1 = SplatConst16(source_y_fraction);
  v128_t round = SplatConst32(128);
  do {
    v128_t s0 = wasm_v128_load(src_ptr);
    v128_t s1 = wasm_v128_load(src_ptr1);
    v128_t lo = wasm_i32x4_add(wasm_u32x4_extmul_low_u16x8(s0, w0),
                               wasm_u32x4_extmul_low_u16x8(s1, w1));
    v128_t hi = wasm_i32x4_add(wasm_u32x4_extmul_high_u16x8(s0, w0),
                               wasm_u32x4_extmul_high_u16x8(s1, w1));
    lo = wasm_i32x4_shl(wasm_i32x4_add(lo, round), 8);
    hi = wasm_i32x4_shl(wasm_i32x4_add(hi, round), 8);
    wasm_v128_store(dst_ptr,
                    wasm_i16x8_shuffle(lo, hi, 1, 3, 5, 7, 9, 11, 13, 15));
    src_ptr += 8;
    src_ptr1 += 8;
    dst_ptr += 8;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_INTERPOLATEROW_16_WASMSIMD

#if defined(HAS_J400TOARGBROW_WASMSIMD) ||    \
    defined(HAS_J400TOARGBROW_RELAXEDSIMD) || \
    defined(HAS_I400TOARGBROW_WASMSIMD) ||    \
    defined(HAS_I400TOARGBROW_RELAXEDSIMD) || \
    defined(HAS_SOBELROW_WASMSIMD) || defined(HAS_SOBELROW_RELAXEDSIMD)
static const uint8_t kShuffleJ400ToARGB[64] = {
    0,  0,  0,  128, 1,  1,  1,  128, 2,  2,  2,  128, 3,  3,  3,  128,
    4,  4,  4,  128, 5,  5,  5,  128, 6,  6,  6,  128, 7,  7,  7,  128,
    8,  8,  8,  128, 9,  9,  9,  128, 10, 10, 10, 128, 11, 11, 11, 128,
    12, 12, 12, 128, 13, 13, 13, 128, 14, 14, 14, 128, 15, 15, 15, 128};
#endif

#if defined(HAS_J400TOARGBROW_WASMSIMD) || \
    defined(HAS_J400TOARGBROW_RELAXEDSIMD)
template <bool kRelaxed>
static inline void J400ToARGBRow_Wasm(const uint8_t* src_y,
                                      uint8_t* dst_argb,
                                      int width) {
  v128_t shuf0 = LoadConst(kShuffleJ400ToARGB);
  v128_t shuf1 = LoadConst(kShuffleJ400ToARGB + 16);
  v128_t shuf2 = LoadConst(kShuffleJ400ToARGB + 32);
  v128_t shuf3 = LoadConst(kShuffleJ400ToARGB + 48);
  v128_t alpha = SplatConst32((int)0xff000000);
  do {
    asm("" : "+r"(dst_argb));
    v128_t y = wasm_v128_load(src_y);
    wasm_v128_store(dst_argb,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(y, shuf0), alpha));
    wasm_v128_store(dst_argb + 16,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(y, shuf1), alpha));
    wasm_v128_store(dst_argb + 32,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(y, shuf2), alpha));
    wasm_v128_store(dst_argb + 48,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(y, shuf3), alpha));
    src_y += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_J400TOARGBROW_WASMSIMD
void J400ToARGBRow_WASMSIMD(const uint8_t* src_y,
                            uint8_t* dst_argb,
                            int width) {
  J400ToARGBRow_Wasm<false>(src_y, dst_argb, width);
}
#endif  // HAS_J400TOARGBROW_WASMSIMD

#ifdef HAS_J400TOARGBROW_RELAXEDSIMD
void J400ToARGBRow_RELAXEDSIMD(const uint8_t* src_y,
                               uint8_t* dst_argb,
                               int width) {
  J400ToARGBRow_Wasm<true>(src_y, dst_argb, width);
}
#endif  // HAS_J400TOARGBROW_RELAXEDSIMD
#endif  // HAS_J400TOARGBROW_WASMSIMD || HAS_J400TOARGBROW_RELAXEDSIMD

#if defined(HAS_I400TOARGBROW_WASMSIMD) || \
    defined(HAS_I400TOARGBROW_RELAXEDSIMD)
template <bool kRelaxed>
static inline void I400ToARGBRow_Wasm(const uint8_t* src_y,
                                      uint8_t* dst_argb,
                                      const struct YuvConstants* yuvconstants,
                                      int width) {
  v128_t shuf0 = LoadConst(kShuffleJ400ToARGB);
  v128_t shuf1 = LoadConst(kShuffleJ400ToARGB + 16);
  v128_t shuf2 = LoadConst(kShuffleJ400ToARGB + 32);
  v128_t shuf3 = LoadConst(kShuffleJ400ToARGB + 48);
  v128_t yg = wasm_i16x8_splat(yuvconstants->kYToRgb[0]);
  v128_t yb = wasm_i16x8_splat(yuvconstants->kYBiasToRgb[0]);
  v128_t k0101 = SplatConst16(0x0101);
  v128_t alpha = SplatConst32((int)0xff000000);
  do {
    asm("" : "+r"(dst_argb));
    asm volatile("" : "+r"(yg));
    v128_t y = wasm_v128_load(src_y);
    v128_t y0 = wasm_i16x8_mul(wasm_u16x8_extend_low_u8x16(y), k0101);
    v128_t y1 = wasm_i16x8_mul(wasm_u16x8_extend_high_u8x16(y), k0101);
    v128_t lo0 = wasm_u32x4_extmul_low_u16x8(y0, yg);
    v128_t hi0 = wasm_u32x4_extmul_high_u16x8(y0, yg);
    v128_t lo1 = wasm_u32x4_extmul_low_u16x8(y1, yg);
    v128_t hi1 = wasm_u32x4_extmul_high_u16x8(y1, yg);
    y0 = wasm_i16x8_shr(
        wasm_i16x8_add(wasm_i16x8_shuffle(lo0, hi0, 1, 3, 5, 7, 9, 11, 13, 15),
                       yb),
        6);
    y1 = wasm_i16x8_shr(
        wasm_i16x8_add(wasm_i16x8_shuffle(lo1, hi1, 1, 3, 5, 7, 9, 11, 13, 15),
                       yb),
        6);
    v128_t y_scaled = wasm_u8x16_narrow_i16x8(y0, y1);
    wasm_v128_store(
        dst_argb, wasm_v128_or(WasmSwizzle<kRelaxed>(y_scaled, shuf0), alpha));
    wasm_v128_store(
        dst_argb + 16,
        wasm_v128_or(WasmSwizzle<kRelaxed>(y_scaled, shuf1), alpha));
    wasm_v128_store(
        dst_argb + 32,
        wasm_v128_or(WasmSwizzle<kRelaxed>(y_scaled, shuf2), alpha));
    wasm_v128_store(
        dst_argb + 48,
        wasm_v128_or(WasmSwizzle<kRelaxed>(y_scaled, shuf3), alpha));
    src_y += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_I400TOARGBROW_WASMSIMD
void I400ToARGBRow_WASMSIMD(const uint8_t* src_y,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  I400ToARGBRow_Wasm<false>(src_y, dst_argb, yuvconstants, width);
}
#endif  // HAS_I400TOARGBROW_WASMSIMD

#ifdef HAS_I400TOARGBROW_RELAXEDSIMD
void I400ToARGBRow_RELAXEDSIMD(const uint8_t* src_y,
                               uint8_t* dst_argb,
                               const struct YuvConstants* yuvconstants,
                               int width) {
  I400ToARGBRow_Wasm<true>(src_y, dst_argb, yuvconstants, width);
}
#endif  // HAS_I400TOARGBROW_RELAXEDSIMD
#endif  // HAS_I400TOARGBROW_WASMSIMD || HAS_I400TOARGBROW_RELAXEDSIMD

#ifdef HAS_RGB565TOARGBROW_WASMSIMD
void RGB565ToARGBRow_WASMSIMD(const uint8_t* src_rgb565,
                              uint8_t* dst_argb,
                              int width) {
  v128_t mask_b5 = SplatConst16(0x001f);
  v128_t mask_g6 = SplatConst16(0x07e0);
  v128_t mask_r5 = SplatConst16(0xf800);
  v128_t mask_hi = SplatConst16(0xff00);
  do {
    asm("" : "+r"(dst_argb));
    v128_t p = wasm_v128_load(src_rgb565);
    v128_t b5 = wasm_v128_and(p, mask_b5);
    v128_t b8 = wasm_v128_or(wasm_i16x8_shl(b5, 3), wasm_u16x8_shr(b5, 2));
    v128_t g6 = wasm_v128_and(p, mask_g6);
    v128_t g8_hi = wasm_v128_and(
        wasm_v128_or(wasm_i16x8_shl(g6, 5), wasm_u16x8_shr(g6, 1)), mask_hi);
    v128_t bg = wasm_v128_or(b8, g8_hi);

    v128_t r8 = wasm_v128_or(wasm_u16x8_shr(wasm_v128_and(p, mask_r5), 8),
                             wasm_u16x8_shr(p, 13));
    v128_t ra = wasm_v128_or(r8, mask_hi);

    v128_t d0 = wasm_i16x8_shuffle(bg, ra, 0, 8, 1, 9, 2, 10, 3, 11);
    v128_t d1 = wasm_i16x8_shuffle(bg, ra, 4, 12, 5, 13, 6, 14, 7, 15);
    wasm_v128_store(dst_argb, d0);
    wasm_v128_store(dst_argb + 16, d1);
    src_rgb565 += 16;
    dst_argb += 32;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_RGB565TOARGBROW_WASMSIMD

#ifdef HAS_ARGB1555TOARGBROW_WASMSIMD
// 5 to 8 bit replication with multiplies: (v * 0x108) >> 5 == v << 3 | v >> 2.
void ARGB1555ToARGBRow_WASMSIMD(const uint8_t* src_argb1555,
                                uint8_t* dst_argb,
                                int width) {
  v128_t mask_b5 = SplatConst16(0x001f);
  v128_t mask_g5 = SplatConst16(0x03e0);
  v128_t mask_hi = SplatConst16(0xff00);
  v128_t mul_br = SplatConst16(0x0108);
  v128_t mul_g = SplatConst16(0x0042);
  do {
    asm("" : "+r"(dst_argb));
    v128_t p = wasm_v128_load(src_argb1555);
    v128_t b8 =
        wasm_u16x8_shr(wasm_i16x8_mul(wasm_v128_and(p, mask_b5), mul_br), 5);
    // g5 << 5 times 0x42 is g5 << 11 | g5 << 6; the high byte is g8.
    v128_t g8_hi = wasm_v128_and(
        wasm_i16x8_mul(wasm_v128_and(p, mask_g5), mul_g), mask_hi);
    v128_t bg = wasm_v128_or(b8, g8_hi);
    v128_t r5 = wasm_u16x8_shr(wasm_i16x8_shl(p, 1), 11);
    v128_t r8 = wasm_u16x8_shr(wasm_i16x8_mul(r5, mul_br), 5);
    v128_t a8_hi = wasm_v128_and(wasm_i16x8_shr(p, 15), mask_hi);
    v128_t ra = wasm_v128_or(r8, a8_hi);

    v128_t d0 = wasm_i16x8_shuffle(bg, ra, 0, 8, 1, 9, 2, 10, 3, 11);
    v128_t d1 = wasm_i16x8_shuffle(bg, ra, 4, 12, 5, 13, 6, 14, 7, 15);
    wasm_v128_store(dst_argb, d0);
    wasm_v128_store(dst_argb + 16, d1);
    src_argb1555 += 16;
    dst_argb += 32;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_ARGB1555TOARGBROW_WASMSIMD

#ifdef HAS_ARGB4444TOARGBROW_WASMSIMD
void ARGB4444ToARGBRow_WASMSIMD(const uint8_t* src_argb4444,
                                uint8_t* dst_argb,
                                int width) {
  v128_t mask_lo = SplatConst8(0x0f);
  v128_t mask_hi = SplatConst8(0xf0);
  do {
    asm("" : "+r"(dst_argb));
    v128_t v = wasm_v128_load(src_argb4444);
    v128_t v_lo = wasm_v128_and(v, mask_lo);
    v_lo = wasm_v128_or(v_lo, wasm_i16x8_shl(v_lo, 4));
    v128_t v_hi = wasm_v128_and(v, mask_hi);
    v_hi = wasm_v128_or(v_hi, wasm_u16x8_shr(v_hi, 4));

    v128_t d0 = wasm_i8x16_shuffle(v_lo, v_hi, 0, 16, 1, 17, 2, 18, 3, 19, 4,
                                   20, 5, 21, 6, 22, 7, 23);
    v128_t d1 = wasm_i8x16_shuffle(v_lo, v_hi, 8, 24, 9, 25, 10, 26, 11, 27, 12,
                                   28, 13, 29, 14, 30, 15, 31);
    wasm_v128_store(dst_argb, d0);
    wasm_v128_store(dst_argb + 16, d1);
    src_argb4444 += 16;
    dst_argb += 32;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_ARGB4444TOARGBROW_WASMSIMD

#ifdef HAS_ARGBTORGB565DITHERROW_WASMSIMD
void ARGBToRGB565DitherRow_WASMSIMD(const uint8_t* src_argb,
                                    uint8_t* dst_rgb,
                                    uint32_t dither4,
                                    int width) {
  v128_t dither = wasm_i32x4_splat((int32_t)dither4);
  dither = wasm_i8x16_shuffle(dither, dither, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                              2, 3, 3, 3, 3);
  v128_t mask_b = SplatConst32(0x0000001f);
  v128_t mask_g = SplatConst32(0x000007e0);
  v128_t mask_r = SplatConst32(0x0000f800);
  do {
    asm("" : "+r"(src_argb));
    v128_t s0 = wasm_u8x16_add_sat(wasm_v128_load(src_argb), dither);
    v128_t s1 = wasm_u8x16_add_sat(wasm_v128_load(src_argb + 16), dither);
    v128_t b0 = wasm_v128_and(wasm_u32x4_shr(s0, 3), mask_b);
    v128_t g0 = wasm_v128_and(wasm_u32x4_shr(s0, 5), mask_g);
    v128_t r0 = wasm_v128_and(wasm_u32x4_shr(s0, 8), mask_r);
    v128_t b1 = wasm_v128_and(wasm_u32x4_shr(s1, 3), mask_b);
    v128_t g1 = wasm_v128_and(wasm_u32x4_shr(s1, 5), mask_g);
    v128_t r1 = wasm_v128_and(wasm_u32x4_shr(s1, 8), mask_r);
    v128_t p0 = wasm_v128_or(wasm_v128_or(b0, g0), r0);
    v128_t p1 = wasm_v128_or(wasm_v128_or(b1, g1), r1);
    wasm_v128_store(dst_rgb, wasm_u16x8_narrow_i32x4(p0, p1));
    src_argb += 32;
    dst_rgb += 16;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_ARGBTORGB565DITHERROW_WASMSIMD

#ifdef HAS_ARGBTORGB565ROW_WASMSIMD
void ARGBToRGB565Row_WASMSIMD(const uint8_t* src_argb,
                              uint8_t* dst_rgb,
                              int width) {
  v128_t mask_b = SplatConst32(0x0000001f);
  v128_t mask_g = SplatConst32(0x000007e0);
  v128_t mask_r = SplatConst32(0x0000f800);
  do {
    asm("" : "+r"(src_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    v128_t b0 = wasm_v128_and(wasm_u32x4_shr(s0, 3), mask_b);
    v128_t g0 = wasm_v128_and(wasm_u32x4_shr(s0, 5), mask_g);
    v128_t r0 = wasm_v128_and(wasm_u32x4_shr(s0, 8), mask_r);
    v128_t b1 = wasm_v128_and(wasm_u32x4_shr(s1, 3), mask_b);
    v128_t g1 = wasm_v128_and(wasm_u32x4_shr(s1, 5), mask_g);
    v128_t r1 = wasm_v128_and(wasm_u32x4_shr(s1, 8), mask_r);
    v128_t p0 = wasm_v128_or(wasm_v128_or(b0, g0), r0);
    v128_t p1 = wasm_v128_or(wasm_v128_or(b1, g1), r1);
    wasm_v128_store(dst_rgb, wasm_u16x8_narrow_i32x4(p0, p1));
    src_argb += 32;
    dst_rgb += 16;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_ARGBTORGB565ROW_WASMSIMD

#ifdef HAS_ARGBTOARGB1555ROW_WASMSIMD
void ARGBToARGB1555Row_WASMSIMD(const uint8_t* src_argb,
                                uint8_t* dst_argb1555,
                                int width) {
  v128_t mask_b = SplatConst32(0x0000001f);
  v128_t mask_g = SplatConst32(0x000003e0);
  v128_t mask_r = SplatConst32(0x00007c00);
  v128_t mask_a = SplatConst32(0x00008000);
  do {
    asm("" : "+r"(src_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    v128_t b0 = wasm_v128_and(wasm_u32x4_shr(s0, 3), mask_b);
    v128_t g0 = wasm_v128_and(wasm_u32x4_shr(s0, 6), mask_g);
    v128_t r0 = wasm_v128_and(wasm_u32x4_shr(s0, 9), mask_r);
    v128_t a0 = wasm_v128_and(wasm_u32x4_shr(s0, 16), mask_a);
    v128_t b1 = wasm_v128_and(wasm_u32x4_shr(s1, 3), mask_b);
    v128_t g1 = wasm_v128_and(wasm_u32x4_shr(s1, 6), mask_g);
    v128_t r1 = wasm_v128_and(wasm_u32x4_shr(s1, 9), mask_r);
    v128_t a1 = wasm_v128_and(wasm_u32x4_shr(s1, 16), mask_a);
    v128_t p0 = wasm_v128_or(wasm_v128_or(b0, g0), wasm_v128_or(r0, a0));
    v128_t p1 = wasm_v128_or(wasm_v128_or(b1, g1), wasm_v128_or(r1, a1));
    wasm_v128_store(dst_argb1555, wasm_u16x8_narrow_i32x4(p0, p1));
    src_argb += 32;
    dst_argb1555 += 16;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_ARGBTOARGB1555ROW_WASMSIMD

#ifdef HAS_ARGBTOARGB4444ROW_WASMSIMD
void ARGBToARGB4444Row_WASMSIMD(const uint8_t* src_argb,
                                uint8_t* dst_argb4444,
                                int width) {
  v128_t mask_lo = SplatConst16(0x000f);
  v128_t mask_hi = SplatConst16(0x00f0);
  do {
    asm("" : "+r"(src_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    v128_t n0 = wasm_v128_or(wasm_v128_and(wasm_u16x8_shr(s0, 4), mask_lo),
                             wasm_v128_and(wasm_u16x8_shr(s0, 8), mask_hi));
    v128_t n1 = wasm_v128_or(wasm_v128_and(wasm_u16x8_shr(s1, 4), mask_lo),
                             wasm_v128_and(wasm_u16x8_shr(s1, 8), mask_hi));
    wasm_v128_store(dst_argb4444, wasm_u8x16_narrow_i16x8(n0, n1));
    src_argb += 32;
    dst_argb4444 += 16;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_ARGBTOARGB4444ROW_WASMSIMD

#if defined(HAS_ARGBSHUFFLEROW_WASMSIMD) || \
    defined(HAS_ARGBSHUFFLEROW_RELAXEDSIMD)
template <bool kRelaxed>
static inline void ARGBShuffleRow_Wasm(const uint8_t* src_argb,
                                       uint8_t* dst_argb,
                                       const uint8_t* shuffler,
                                       int width) {
  v128_t shuf = wasm_v128_load(shuffler);
  while (width >= 8) {
    asm("" : "+r"(src_argb), "+r"(dst_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    wasm_v128_store(dst_argb, WasmSwizzle<kRelaxed>(s0, shuf));
    wasm_v128_store(dst_argb + 16, WasmSwizzle<kRelaxed>(s1, shuf));
    src_argb += 32;
    dst_argb += 32;
    width -= 8;
  }
  if (width > 0) {
    v128_t s0 = wasm_v128_load(src_argb);
    wasm_v128_store(dst_argb, WasmSwizzle<kRelaxed>(s0, shuf));
  }
}

#ifdef HAS_ARGBSHUFFLEROW_WASMSIMD
void ARGBShuffleRow_WASMSIMD(const uint8_t* src_argb,
                             uint8_t* dst_argb,
                             const uint8_t* shuffler,
                             int width) {
  ARGBShuffleRow_Wasm<false>(src_argb, dst_argb, shuffler, width);
}
#endif  // HAS_ARGBSHUFFLEROW_WASMSIMD

#ifdef HAS_ARGBSHUFFLEROW_RELAXEDSIMD
void ARGBShuffleRow_RELAXEDSIMD(const uint8_t* src_argb,
                                uint8_t* dst_argb,
                                const uint8_t* shuffler,
                                int width) {
  ARGBShuffleRow_Wasm<true>(src_argb, dst_argb, shuffler, width);
}
#endif  // HAS_ARGBSHUFFLEROW_RELAXEDSIMD
#endif  // HAS_ARGBSHUFFLEROW_WASMSIMD || HAS_ARGBSHUFFLEROW_RELAXEDSIMD

#ifdef HAS_COPYROW_WASMSIMD
void CopyRow_WASMSIMD(const uint8_t* src, uint8_t* dst, int count) {
  do {
    asm("" : "+r"(src), "+r"(dst));
    v128_t v0 = wasm_v128_load(src);
    v128_t v1 = wasm_v128_load(src + 16);
    v128_t v2 = wasm_v128_load(src + 32);
    v128_t v3 = wasm_v128_load(src + 48);
    wasm_v128_store(dst, v0);
    wasm_v128_store(dst + 16, v1);
    wasm_v128_store(dst + 32, v2);
    wasm_v128_store(dst + 48, v3);
    src += 64;
    dst += 64;
    count -= 64;
  } while (count > 0);
}
#endif  // HAS_COPYROW_WASMSIMD

#ifdef HAS_COPYROW_16_WASMSIMD
void CopyRow_16_WASMSIMD(const uint16_t* src, uint16_t* dst, int count) {
  do {
    asm("" : "+r"(src), "+r"(dst));
    v128_t v0 = wasm_v128_load(src);
    v128_t v1 = wasm_v128_load(src + 8);
    v128_t v2 = wasm_v128_load(src + 16);
    v128_t v3 = wasm_v128_load(src + 24);
    wasm_v128_store(dst, v0);
    wasm_v128_store(dst + 8, v1);
    wasm_v128_store(dst + 16, v2);
    wasm_v128_store(dst + 24, v3);
    src += 32;
    dst += 32;
    count -= 32;
  } while (count > 0);
}
#endif  // HAS_COPYROW_16_WASMSIMD

#ifdef HAS_SETROW_WASMSIMD
void SetRow_WASMSIMD(uint8_t* dst, uint8_t v8, int width) {
  v128_t v = wasm_i8x16_splat(v8);
  do {
    wasm_v128_store(dst, v);
    dst += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_SETROW_WASMSIMD

#ifdef HAS_ARGBSETROW_WASMSIMD
void ARGBSetRow_WASMSIMD(uint8_t* dst_argb, uint32_t v32, int width) {
  v128_t v = wasm_i32x4_splat((int32_t)v32);
  do {
    wasm_v128_store(dst_argb, v);
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBSETROW_WASMSIMD

#ifdef HAS_HALFROW_WASMSIMD
void HalfRow_WASMSIMD(const uint8_t* src_uv,
                      ptrdiff_t src_uv_stride,
                      uint8_t* dst_uv,
                      int width) {
  const uint8_t* src_uv1 = src_uv + src_uv_stride;
  do {
    wasm_v128_store(dst_uv, wasm_u8x16_avgr(wasm_v128_load(src_uv),
                                            wasm_v128_load(src_uv1)));
    src_uv += 16;
    src_uv1 += 16;
    dst_uv += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_HALFROW_WASMSIMD

#ifdef HAS_HALFROW_16_WASMSIMD
void HalfRow_16_WASMSIMD(const uint16_t* src_uv,
                         ptrdiff_t src_uv_stride,
                         uint16_t* dst_uv,
                         int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  do {
    wasm_v128_store(dst_uv, wasm_u16x8_avgr(wasm_v128_load(src_uv),
                                            wasm_v128_load(src_uv1)));
    src_uv += 8;
    src_uv1 += 8;
    dst_uv += 8;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_HALFROW_16_WASMSIMD

#ifdef HAS_HALFROW_16TO8_WASMSIMD
void HalfRow_16To8_WASMSIMD(const uint16_t* src_uv,
                            ptrdiff_t src_uv_stride,
                            uint8_t* dst_uv,
                            int scale,
                            int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  v128_t vscale = wasm_i16x8_splat(scale);
  do {
    asm("" : "+r"(src_uv), "+r"(src_uv1));
    v128_t a0 =
        wasm_u16x8_avgr(wasm_v128_load(src_uv), wasm_v128_load(src_uv1));
    v128_t a1 = wasm_u16x8_avgr(wasm_v128_load(src_uv + 8),
                                wasm_v128_load(src_uv1 + 8));
    v128_t s0 = wasm_i16x8_shuffle(wasm_u32x4_extmul_low_u16x8(a0, vscale),
                                   wasm_u32x4_extmul_high_u16x8(a0, vscale), 1,
                                   3, 5, 7, 9, 11, 13, 15);
    v128_t s1 = wasm_i16x8_shuffle(wasm_u32x4_extmul_low_u16x8(a1, vscale),
                                   wasm_u32x4_extmul_high_u16x8(a1, vscale), 1,
                                   3, 5, 7, 9, 11, 13, 15);
    wasm_v128_store(dst_uv, wasm_u8x16_narrow_i16x8(s0, s1));
    src_uv += 16;
    src_uv1 += 16;
    dst_uv += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_HALFROW_16TO8_WASMSIMD

#ifdef HAS_HALFWIDTHROW_16TO8_WASMSIMD
void HalfWidthRow_16To8_WASMSIMD(const uint16_t* src_uv,
                                 ptrdiff_t src_uv_stride,
                                 uint8_t* dst_uv,
                                 int scale,
                                 int width) {
  const uint16_t* src_uv1 = src_uv + src_uv_stride;
  v128_t vscale = wasm_i16x8_splat(scale);
  do {
    asm("" : "+r"(src_uv), "+r"(src_uv1));
    v128_t r0a = wasm_v128_load(src_uv);
    v128_t r0b = wasm_v128_load(src_uv + 8);
    v128_t r1a = wasm_v128_load(src_uv1);
    v128_t r1b = wasm_v128_load(src_uv1 + 8);
    v128_t sum0 = wasm_i32x4_add(wasm_u32x4_extadd_pairwise_u16x8(r0a),
                                 wasm_u32x4_extadd_pairwise_u16x8(r1a));
    v128_t sum1 = wasm_i32x4_add(wasm_u32x4_extadd_pairwise_u16x8(r0b),
                                 wasm_u32x4_extadd_pairwise_u16x8(r1b));
    v128_t avg0 = wasm_u32x4_shr(wasm_i32x4_add(sum0, SplatConst32(2)), 2);
    v128_t avg1 = wasm_u32x4_shr(wasm_i32x4_add(sum1, SplatConst32(2)), 2);
    v128_t avg = wasm_i16x8_shuffle(avg0, avg1, 0, 2, 4, 6, 8, 10, 12, 14);
    v128_t scaled = wasm_i16x8_shuffle(
        wasm_u32x4_extmul_low_u16x8(avg, vscale),
        wasm_u32x4_extmul_high_u16x8(avg, vscale), 1, 3, 5, 7, 9, 11, 13, 15);
    wasm_v128_store64_lane(dst_uv, wasm_u8x16_narrow_i16x8(scaled, scaled), 0);
    src_uv += 16;
    src_uv1 += 16;
    dst_uv += 8;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_HALFWIDTHROW_16TO8_WASMSIMD

#ifdef HAS_CONVERT8TO8ROW_WASMSIMD
void Convert8To8Row_WASMSIMD(const uint8_t* src_y,
                             uint8_t* dst_y,
                             int scale,
                             int bias,
                             int width) {
  v128_t vscale = wasm_i8x16_splat(scale);
  v128_t vbias = wasm_i16x8_splat(bias);
  do {
    v128_t y = wasm_v128_load(src_y);
    v128_t s0 = wasm_i16x8_add(
        wasm_u16x8_shr(wasm_u16x8_extmul_low_u8x16(y, vscale), 8), vbias);
    v128_t s1 = wasm_i16x8_add(
        wasm_u16x8_shr(wasm_u16x8_extmul_high_u8x16(y, vscale), 8), vbias);
    wasm_v128_store(dst_y, wasm_u8x16_narrow_i16x8(s0, s1));
    src_y += 16;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_CONVERT8TO8ROW_WASMSIMD

#ifdef HAS_I422TOYUY2ROW_WASMSIMD
void I422ToYUY2Row_WASMSIMD(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_yuy2,
                            int width) {
  do {
    asm("" : "+r"(dst_yuy2));
    v128_t y = wasm_v128_load(src_y);
    v128_t u = wasm_v128_load64_zero(src_u);
    v128_t v = wasm_v128_load64_zero(src_v);
    v128_t uv = wasm_i8x16_shuffle(u, v, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                   21, 6, 22, 7, 23);
    wasm_v128_store(dst_yuy2,
                    wasm_i8x16_shuffle(y, uv, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20,
                                       5, 21, 6, 22, 7, 23));
    wasm_v128_store(dst_yuy2 + 16,
                    wasm_i8x16_shuffle(y, uv, 8, 24, 9, 25, 10, 26, 11, 27, 12,
                                       28, 13, 29, 14, 30, 15, 31));
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_yuy2 += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422TOYUY2ROW_WASMSIMD

#ifdef HAS_I422TOUYVYROW_WASMSIMD
void I422ToUYVYRow_WASMSIMD(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_uyvy,
                            int width) {
  do {
    asm("" : "+r"(dst_uyvy));
    v128_t y = wasm_v128_load(src_y);
    v128_t u = wasm_v128_load64_zero(src_u);
    v128_t v = wasm_v128_load64_zero(src_v);
    v128_t uv = wasm_i8x16_shuffle(u, v, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                   21, 6, 22, 7, 23);
    wasm_v128_store(dst_uyvy,
                    wasm_i8x16_shuffle(uv, y, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20,
                                       5, 21, 6, 22, 7, 23));
    wasm_v128_store(dst_uyvy + 16,
                    wasm_i8x16_shuffle(uv, y, 8, 24, 9, 25, 10, 26, 11, 27, 12,
                                       28, 13, 29, 14, 30, 15, 31));
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_uyvy += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422TOUYVYROW_WASMSIMD

#ifdef HAS_YUY2TONVUVROW_WASMSIMD
void YUY2ToNVUVRow_WASMSIMD(const uint8_t* src_yuy2,
                            int src_stride_yuy2,
                            uint8_t* dst_uv,
                            int width) {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  do {
    asm("" : "+r"(src_yuy2), "+r"(src_yuy2_1));
    v128_t r0 =
        wasm_u8x16_avgr(wasm_v128_load(src_yuy2), wasm_v128_load(src_yuy2_1));
    v128_t r1 = wasm_u8x16_avgr(wasm_v128_load(src_yuy2 + 16),
                                wasm_v128_load(src_yuy2_1 + 16));
    wasm_v128_store(
        dst_uv, wasm_i8x16_shuffle(r0, r1, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
                                   21, 23, 25, 27, 29, 31));
    src_yuy2 += 32;
    src_yuy2_1 += 32;
    dst_uv += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_YUY2TONVUVROW_WASMSIMD

#ifdef HAS_AYUVTOYROW_WASMSIMD
void AYUVToYRow_WASMSIMD(const uint8_t* src_ayuv, uint8_t* dst_y, int width) {
  do {
    asm("" : "+r"(src_ayuv));
    v128_t s0 = wasm_v128_load(src_ayuv);
    v128_t s1 = wasm_v128_load(src_ayuv + 16);
    v128_t s2 = wasm_v128_load(src_ayuv + 32);
    v128_t s3 = wasm_v128_load(src_ayuv + 48);
    v128_t y01 = wasm_i8x16_shuffle(s0, s1, 2, 6, 10, 14, 18, 22, 26, 30, 2, 6,
                                    10, 14, 18, 22, 26, 30);
    v128_t y23 = wasm_i8x16_shuffle(s2, s3, 2, 6, 10, 14, 18, 22, 26, 30, 2, 6,
                                    10, 14, 18, 22, 26, 30);
    wasm_v128_store(dst_y, wasm_i64x2_shuffle(y01, y23, 0, 2));
    src_ayuv += 64;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_AYUVTOYROW_WASMSIMD

#if defined(HAS_AYUVTOUVROW_WASMSIMD) ||    \
    defined(HAS_AYUVTOUVROW_RELAXEDSIMD) || \
    defined(HAS_AYUVTOVUROW_WASMSIMD) || defined(HAS_AYUVTOVUROW_RELAXEDSIMD)
static const uint8_t kShuffleAYUVToUV[16] = {1, 0, 5, 4, 9, 8, 13, 12,
                                             1, 0, 5, 4, 9, 8, 13, 12};
static const uint8_t kShuffleAYUVToVU[16] = {0, 1, 4, 5, 8, 9, 12, 13,
                                             0, 1, 4, 5, 8, 9, 12, 13};

template <bool kRelaxed>
static inline void AYUVToUVRowImpl_Wasm(const uint8_t* src_ayuv,
                                        int src_stride_ayuv,
                                        uint8_t* dst_uv,
                                        int width,
                                        const uint8_t* shuf_table) {
  const uint8_t* src_ayuv1 = src_ayuv + src_stride_ayuv;
  v128_t shuf = LoadConst(shuf_table);
  do {
    asm("" : "+r"(src_ayuv), "+r"(src_ayuv1));
    v128_t r00 = wasm_v128_load(src_ayuv);
    v128_t r01 = wasm_v128_load(src_ayuv + 16);
    v128_t r02 = wasm_v128_load(src_ayuv + 32);
    v128_t r03 = wasm_v128_load(src_ayuv + 48);
    v128_t r10 = wasm_v128_load(src_ayuv1);
    v128_t r11 = wasm_v128_load(src_ayuv1 + 16);
    v128_t r12 = wasm_v128_load(src_ayuv1 + 32);
    v128_t r13 = wasm_v128_load(src_ayuv1 + 48);
    v128_t uv00 = WasmSwizzle<kRelaxed>(r00, shuf);
    v128_t uv01 = WasmSwizzle<kRelaxed>(r01, shuf);
    v128_t uv02 = WasmSwizzle<kRelaxed>(r02, shuf);
    v128_t uv03 = WasmSwizzle<kRelaxed>(r03, shuf);
    v128_t uv10 = WasmSwizzle<kRelaxed>(r10, shuf);
    v128_t uv11 = WasmSwizzle<kRelaxed>(r11, shuf);
    v128_t uv12 = WasmSwizzle<kRelaxed>(r12, shuf);
    v128_t uv13 = WasmSwizzle<kRelaxed>(r13, shuf);
    v128_t row0_a = wasm_i64x2_shuffle(uv00, uv01, 0, 2);
    v128_t row0_b = wasm_i64x2_shuffle(uv02, uv03, 0, 2);
    v128_t row1_a = wasm_i64x2_shuffle(uv10, uv11, 0, 2);
    v128_t row1_b = wasm_i64x2_shuffle(uv12, uv13, 0, 2);
    v128_t even_0 =
        wasm_i16x8_shuffle(row0_a, row0_b, 0, 2, 4, 6, 8, 10, 12, 14);
    v128_t odd_0 =
        wasm_i16x8_shuffle(row0_a, row0_b, 1, 3, 5, 7, 9, 11, 13, 15);
    v128_t even_1 =
        wasm_i16x8_shuffle(row1_a, row1_b, 0, 2, 4, 6, 8, 10, 12, 14);
    v128_t odd_1 =
        wasm_i16x8_shuffle(row1_a, row1_b, 1, 3, 5, 7, 9, 11, 13, 15);
    v128_t s_lo =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_low_u8x16(even_0),
                                      wasm_u16x8_extend_low_u8x16(odd_0)),
                       wasm_i16x8_add(wasm_u16x8_extend_low_u8x16(even_1),
                                      wasm_u16x8_extend_low_u8x16(odd_1)));
    v128_t s_hi =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_high_u8x16(even_0),
                                      wasm_u16x8_extend_high_u8x16(odd_0)),
                       wasm_i16x8_add(wasm_u16x8_extend_high_u8x16(even_1),
                                      wasm_u16x8_extend_high_u8x16(odd_1)));
    s_lo = wasm_u16x8_shr(wasm_i16x8_add(s_lo, SplatConst16(2)), 2);
    s_hi = wasm_u16x8_shr(wasm_i16x8_add(s_hi, SplatConst16(2)), 2);
    wasm_v128_store(dst_uv, wasm_u8x16_narrow_i16x8(s_lo, s_hi));
    src_ayuv += 64;
    src_ayuv1 += 64;
    dst_uv += 16;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_AYUVTOUVROW_WASMSIMD
void AYUVToUVRow_WASMSIMD(const uint8_t* src_ayuv,
                          int src_stride_ayuv,
                          uint8_t* dst_uv,
                          int width) {
  AYUVToUVRowImpl_Wasm<false>(src_ayuv, src_stride_ayuv, dst_uv, width,
                              kShuffleAYUVToUV);
}
#endif  // HAS_AYUVTOUVROW_WASMSIMD

#ifdef HAS_AYUVTOUVROW_RELAXEDSIMD
void AYUVToUVRow_RELAXEDSIMD(const uint8_t* src_ayuv,
                             int src_stride_ayuv,
                             uint8_t* dst_uv,
                             int width) {
  AYUVToUVRowImpl_Wasm<true>(src_ayuv, src_stride_ayuv, dst_uv, width,
                             kShuffleAYUVToUV);
}
#endif  // HAS_AYUVTOUVROW_RELAXEDSIMD

#ifdef HAS_AYUVTOVUROW_WASMSIMD
void AYUVToVURow_WASMSIMD(const uint8_t* src_ayuv,
                          int src_stride_ayuv,
                          uint8_t* dst_vu,
                          int width) {
  AYUVToUVRowImpl_Wasm<false>(src_ayuv, src_stride_ayuv, dst_vu, width,
                              kShuffleAYUVToVU);
}
#endif  // HAS_AYUVTOVUROW_WASMSIMD

#ifdef HAS_AYUVTOVUROW_RELAXEDSIMD
void AYUVToVURow_RELAXEDSIMD(const uint8_t* src_ayuv,
                             int src_stride_ayuv,
                             uint8_t* dst_vu,
                             int width) {
  AYUVToUVRowImpl_Wasm<true>(src_ayuv, src_stride_ayuv, dst_vu, width,
                             kShuffleAYUVToVU);
}
#endif  // HAS_AYUVTOVUROW_RELAXEDSIMD
#endif  // HAS_AYUVTOUVROW_* || HAS_AYUVTOVUROW_*

#ifdef HAS_DETILEROW_WASMSIMD
void DetileRow_WASMSIMD(const uint8_t* src,
                        ptrdiff_t src_tile_stride,
                        uint8_t* dst,
                        int width) {
  do {
    wasm_v128_store(dst, wasm_v128_load(src));
    src += src_tile_stride;
    dst += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_DETILEROW_WASMSIMD

#ifdef HAS_DETILEROW_16_WASMSIMD
void DetileRow_16_WASMSIMD(const uint16_t* src,
                           ptrdiff_t src_tile_stride,
                           uint16_t* dst,
                           int width) {
  do {
    asm("" : "+r"(dst));
    v128_t v0 = wasm_v128_load(src);
    v128_t v1 = wasm_v128_load(src + 8);
    wasm_v128_store(dst, v0);
    wasm_v128_store(dst + 8, v1);
    src += src_tile_stride;
    dst += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_DETILEROW_16_WASMSIMD

#ifdef HAS_DETILESPLITUVROW_WASMSIMD
void DetileSplitUVRow_WASMSIMD(const uint8_t* src_uv,
                               ptrdiff_t src_tile_stride,
                               uint8_t* dst_u,
                               uint8_t* dst_v,
                               int width) {
  do {
    v128_t uv = wasm_v128_load(src_uv);
    v128_t u_v = wasm_i8x16_shuffle(uv, uv, 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5,
                                    7, 9, 11, 13, 15);
    wasm_v128_store64_lane(dst_u, u_v, 0);
    wasm_v128_store64_lane(dst_v, u_v, 1);
    src_uv += src_tile_stride;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_DETILESPLITUVROW_WASMSIMD

#ifdef HAS_DETILETOYUY2_WASMSIMD
void DetileToYUY2_WASMSIMD(const uint8_t* src_y,
                           ptrdiff_t src_y_tile_stride,
                           const uint8_t* src_uv,
                           ptrdiff_t src_uv_tile_stride,
                           uint8_t* dst_yuy2,
                           int width) {
  do {
    asm("" : "+r"(dst_yuy2));
    v128_t y = wasm_v128_load(src_y);
    v128_t uv = wasm_v128_load(src_uv);
    wasm_v128_store(dst_yuy2,
                    wasm_i8x16_shuffle(y, uv, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20,
                                       5, 21, 6, 22, 7, 23));
    wasm_v128_store(dst_yuy2 + 16,
                    wasm_i8x16_shuffle(y, uv, 8, 24, 9, 25, 10, 26, 11, 27, 12,
                                       28, 13, 29, 14, 30, 15, 31));
    src_y += src_y_tile_stride;
    src_uv += src_uv_tile_stride;
    dst_yuy2 += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_DETILETOYUY2_WASMSIMD

#if defined(HAS_AB64TOARGBROW_WASMSIMD) || defined(HAS_AR64TOARGBROW_WASMSIMD)
void AR64ToARGBRow_WASMSIMD(const uint16_t* src_ar64,
                            uint8_t* dst_argb,
                            int width) {
  do {
    asm("" : "+r"(src_ar64), "+r"(dst_argb));
    v128_t s0 = wasm_v128_load(src_ar64);
    v128_t s1 = wasm_v128_load(src_ar64 + 8);
    v128_t s2 = wasm_v128_load(src_ar64 + 16);
    v128_t s3 = wasm_v128_load(src_ar64 + 24);
    wasm_v128_store(
        dst_argb, wasm_i8x16_shuffle(s0, s1, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
                                     21, 23, 25, 27, 29, 31));
    wasm_v128_store(dst_argb + 16,
                    wasm_i8x16_shuffle(s2, s3, 1, 3, 5, 7, 9, 11, 13, 15, 17,
                                       19, 21, 23, 25, 27, 29, 31));
    src_ar64 += 32;
    dst_argb += 32;
    width -= 8;
  } while (width > 0);
}

void AB64ToARGBRow_WASMSIMD(const uint16_t* src_ab64,
                            uint8_t* dst_argb,
                            int width) {
  do {
    asm("" : "+r"(src_ab64), "+r"(dst_argb));
    v128_t s0 = wasm_v128_load(src_ab64);
    v128_t s1 = wasm_v128_load(src_ab64 + 8);
    v128_t s2 = wasm_v128_load(src_ab64 + 16);
    v128_t s3 = wasm_v128_load(src_ab64 + 24);
    wasm_v128_store(
        dst_argb, wasm_i8x16_shuffle(s0, s1, 5, 3, 1, 7, 13, 11, 9, 15, 21, 19,
                                     17, 23, 29, 27, 25, 31));
    wasm_v128_store(dst_argb + 16,
                    wasm_i8x16_shuffle(s2, s3, 5, 3, 1, 7, 13, 11, 9, 15, 21,
                                       19, 17, 23, 29, 27, 25, 31));
    src_ab64 += 32;
    dst_argb += 32;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_AB64TOARGBROW_WASMSIMD || HAS_AR64TOARGBROW_WASMSIMD

#if defined(HAS_ARGBTOAR64ROW_WASMSIMD) || defined(HAS_ARGBTOAB64ROW_WASMSIMD)
void ARGBToAR64Row_WASMSIMD(const uint8_t* src_argb,
                            uint16_t* dst_ar64,
                            int width) {
  do {
    asm("" : "+r"(src_argb), "+r"(dst_ar64));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    wasm_v128_store(dst_ar64, wasm_i8x16_shuffle(s0, s0, 0, 0, 1, 1, 2, 2, 3, 3,
                                                 4, 4, 5, 5, 6, 6, 7, 7));
    wasm_v128_store(dst_ar64 + 8,
                    wasm_i8x16_shuffle(s0, s0, 8, 8, 9, 9, 10, 10, 11, 11, 12,
                                       12, 13, 13, 14, 14, 15, 15));
    wasm_v128_store(dst_ar64 + 16,
                    wasm_i8x16_shuffle(s1, s1, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5,
                                       5, 6, 6, 7, 7));
    wasm_v128_store(dst_ar64 + 24,
                    wasm_i8x16_shuffle(s1, s1, 8, 8, 9, 9, 10, 10, 11, 11, 12,
                                       12, 13, 13, 14, 14, 15, 15));
    src_argb += 32;
    dst_ar64 += 32;
    width -= 8;
  } while (width > 0);
}

void ARGBToAB64Row_WASMSIMD(const uint8_t* src_argb,
                            uint16_t* dst_ab64,
                            int width) {
  do {
    asm("" : "+r"(src_argb), "+r"(dst_ab64));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    wasm_v128_store(dst_ab64, wasm_i8x16_shuffle(s0, s0, 2, 2, 1, 1, 0, 0, 3, 3,
                                                 6, 6, 5, 5, 4, 4, 7, 7));
    wasm_v128_store(dst_ab64 + 8,
                    wasm_i8x16_shuffle(s0, s0, 10, 10, 9, 9, 8, 8, 11, 11, 14,
                                       14, 13, 13, 12, 12, 15, 15));
    wasm_v128_store(dst_ab64 + 16,
                    wasm_i8x16_shuffle(s1, s1, 2, 2, 1, 1, 0, 0, 3, 3, 6, 6, 5,
                                       5, 4, 4, 7, 7));
    wasm_v128_store(dst_ab64 + 24,
                    wasm_i8x16_shuffle(s1, s1, 10, 10, 9, 9, 8, 8, 11, 11, 14,
                                       14, 13, 13, 12, 12, 15, 15));
    src_argb += 32;
    dst_ab64 += 32;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_ARGBTOAR64ROW_WASMSIMD || HAS_ARGBTOAB64ROW_WASMSIMD

#if defined(HAS_AR30TOARGBROW_WASMSIMD) || \
    defined(HAS_AR30TOABGRROW_WASMSIMD) || defined(HAS_AR30TOAB30ROW_WASMSIMD)
void AR30ToARGBRow_WASMSIMD(const uint8_t* src_ar30,
                            uint8_t* dst_argb,
                            int width) {
  v128_t mask_b = SplatConst32(0x000000ff);
  v128_t mask_g = SplatConst32(0x0000ff00);
  v128_t mask_r = SplatConst32(0x00ff0000);
  v128_t mask_a3 = SplatConst32((int)0xc0000000);
  v128_t mul_a = SplatConst32(0x01550000);
  do {
    v128_t v = wasm_v128_load(src_ar30);
    v128_t b = wasm_v128_and(wasm_u32x4_shr(v, 2), mask_b);
    v128_t g = wasm_v128_and(wasm_u32x4_shr(v, 4), mask_g);
    v128_t r = wasm_v128_and(wasm_u32x4_shr(v, 6), mask_r);
    v128_t a =
        wasm_i32x4_mul(wasm_u32x4_shr(wasm_v128_and(v, mask_a3), 22), mul_a);
    wasm_v128_store(dst_argb,
                    wasm_v128_or(wasm_v128_or(b, g), wasm_v128_or(r, a)));
    src_ar30 += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}

void AR30ToABGRRow_WASMSIMD(const uint8_t* src_ar30,
                            uint8_t* dst_abgr,
                            int width) {
  v128_t mask_r = SplatConst32(0x000000ff);
  v128_t mask_g = SplatConst32(0x0000ff00);
  v128_t mask_b = SplatConst32(0x00ff0000);
  v128_t mask_a3 = SplatConst32((int)0xc0000000);
  v128_t mul_a = SplatConst32(0x01550000);
  do {
    v128_t v = wasm_v128_load(src_ar30);
    v128_t b = wasm_v128_and(wasm_i32x4_shl(v, 14), mask_b);
    v128_t g = wasm_v128_and(wasm_u32x4_shr(v, 4), mask_g);
    v128_t r = wasm_v128_and(wasm_u32x4_shr(v, 22), mask_r);
    v128_t a =
        wasm_i32x4_mul(wasm_u32x4_shr(wasm_v128_and(v, mask_a3), 22), mul_a);
    wasm_v128_store(dst_abgr,
                    wasm_v128_or(wasm_v128_or(b, g), wasm_v128_or(r, a)));
    src_ar30 += 16;
    dst_abgr += 16;
    width -= 4;
  } while (width > 0);
}

void AR30ToAB30Row_WASMSIMD(const uint8_t* src_ar30,
                            uint8_t* dst_ab30,
                            int width) {
  v128_t mask_b = SplatConst32(0x000003ff);
  v128_t mask_ga = SplatConst32((int)0xc00ffc00);
  v128_t mask_r = SplatConst32(0x3ff00000);
  do {
    v128_t v = wasm_v128_load(src_ar30);
    v128_t b_to_r = wasm_i32x4_shl(wasm_v128_and(v, mask_b), 20);
    v128_t r_to_b = wasm_u32x4_shr(wasm_v128_and(v, mask_r), 20);
    v128_t ga = wasm_v128_and(v, mask_ga);
    wasm_v128_store(dst_ab30, wasm_v128_or(ga, wasm_v128_or(b_to_r, r_to_b)));
    src_ar30 += 16;
    dst_ab30 += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_AR30TOARGBROW_WASMSIMD

#if defined(HAS_ARGBTOAR30ROW_WASMSIMD) || defined(HAS_ABGRTOAR30ROW_WASMSIMD)
void ARGBToAR30Row_WASMSIMD(const uint8_t* src_argb,
                            uint8_t* dst_ar30,
                            int width) {
  v128_t mask_b = SplatConst32(0x000000ff);
  v128_t mask_g = SplatConst32(0x0000ff00);
  v128_t mask_r = SplatConst32(0x00ff0000);
  v128_t mask_a = SplatConst32((int)0xc0000000);
  do {
    v128_t v = wasm_v128_load(src_argb);
    v128_t b = wasm_v128_and(v, mask_b);
    b = wasm_v128_or(wasm_i32x4_shl(b, 2), wasm_u32x4_shr(b, 6));
    v128_t g = wasm_v128_and(v, mask_g);
    g = wasm_v128_or(wasm_i32x4_shl(g, 4),
                     wasm_i32x4_shl(wasm_u32x4_shr(g, 14), 10));
    v128_t r = wasm_v128_and(v, mask_r);
    r = wasm_v128_or(wasm_i32x4_shl(r, 6),
                     wasm_i32x4_shl(wasm_u32x4_shr(r, 22), 20));
    v128_t a = wasm_v128_and(v, mask_a);
    wasm_v128_store(dst_ar30,
                    wasm_v128_or(wasm_v128_or(b, g), wasm_v128_or(r, a)));
    src_argb += 16;
    dst_ar30 += 16;
    width -= 4;
  } while (width > 0);
}

void ABGRToAR30Row_WASMSIMD(const uint8_t* src_abgr,
                            uint8_t* dst_ar30,
                            int width) {
  v128_t mask_r = SplatConst32(0x000000ff);
  v128_t mask_g = SplatConst32(0x0000ff00);
  v128_t mask_b = SplatConst32(0x00ff0000);
  v128_t mask_a = SplatConst32((int)0xc0000000);
  do {
    v128_t v = wasm_v128_load(src_abgr);
    v128_t r = wasm_v128_and(v, mask_r);
    r = wasm_v128_or(wasm_i32x4_shl(r, 22),
                     wasm_i32x4_shl(wasm_u32x4_shr(r, 6), 20));
    v128_t g = wasm_v128_and(v, mask_g);
    g = wasm_v128_or(wasm_i32x4_shl(g, 4),
                     wasm_i32x4_shl(wasm_u32x4_shr(g, 14), 10));
    v128_t b = wasm_v128_and(v, mask_b);
    b = wasm_v128_or(wasm_u32x4_shr(b, 14), wasm_u32x4_shr(b, 22));
    v128_t a = wasm_v128_and(v, mask_a);
    wasm_v128_store(dst_ar30,
                    wasm_v128_or(wasm_v128_or(b, g), wasm_v128_or(r, a)));
    src_abgr += 16;
    dst_ar30 += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBTOAR30ROW_WASMSIMD || HAS_ABGRTOAR30ROW_WASMSIMD

#if defined(HAS_I422TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I422ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_I422TORGBAROW_WASMSIMD) ||      \
    defined(HAS_I422TOAR30ROW_WASMSIMD) ||      \
    defined(HAS_NV12TOARGBROW_WASMSIMD) ||      \
    defined(HAS_NV21TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I444TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I444ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_I422TORGB24ROW_WASMSIMD) ||     \
    defined(HAS_I422TORGB24ROW_RELAXEDSIMD) ||  \
    defined(HAS_NV12TORGB24ROW_WASMSIMD) ||     \
    defined(HAS_NV12TORGB24ROW_RELAXEDSIMD) ||  \
    defined(HAS_NV21TORGB24ROW_WASMSIMD) ||     \
    defined(HAS_NV21TORGB24ROW_RELAXEDSIMD) ||  \
    defined(HAS_I444TORGB24ROW_WASMSIMD) ||     \
    defined(HAS_I444TORGB24ROW_RELAXEDSIMD) ||  \
    defined(HAS_YUY2TOARGBROW_WASMSIMD) ||      \
    defined(HAS_UYVYTOARGBROW_WASMSIMD) ||      \
    defined(HAS_I422TORGB565ROW_WASMSIMD) ||    \
    defined(HAS_I422TOARGB1555ROW_WASMSIMD) ||  \
    defined(HAS_I422TOARGB4444ROW_WASMSIMD) ||  \
    defined(HAS_NV12TORGB565ROW_WASMSIMD) ||    \
    defined(HAS_I210TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I210ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_I410TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I410ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_I210TOAR30ROW_WASMSIMD) || defined(HAS_I410TOAR30ROW_WASMSIMD)
// Interleaves map to punpck on x64 and zip on arm64 when the two inputs
// differ. V8 arm64 lowers shuffle(x, x) to tbl with a rebuilt mask.
static __inline v128_t ZipLo8_WASMSIMD(v128_t a, v128_t b) {
  return wasm_i8x16_shuffle(a, b, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6,
                            22, 7, 23);
}
static __inline v128_t ZipHi8_WASMSIMD(v128_t a, v128_t b) {
  return wasm_i8x16_shuffle(a, b, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29,
                            14, 30, 15, 31);
}

// YuvConstants splatted to 16-bit lanes, built once per row.
struct YuvCoefs_WASMSIMD {
  v128_t ub, ug, vg, vr, yg, yb, x80, k00ff, k0101;
};

static __inline void LoadYuvCoefs_WASMSIMD(const struct YuvConstants* yc,
                                           int yb_adjust,
                                           struct YuvCoefs_WASMSIMD* c) {
  c->ub = wasm_i16x8_splat(yc->kUVToB[0]);
  c->ug = wasm_i16x8_splat(yc->kUVToG[0]);
  c->vg = wasm_i16x8_splat(yc->kUVToG[1]);
  c->vr = wasm_i16x8_splat(yc->kUVToR[1]);
  c->yg = wasm_i16x8_splat(yc->kYToRgb[0]);
  c->yb = wasm_i16x8_splat(yc->kYBiasToRgb[0] - yb_adjust);
  c->x80 = SplatConst8(0x80);
  c->k00ff = SplatConst16(0x00ff);
  c->k0101 = SplatConst16(0x0101);
}

// Returns ((y16 * yg) >> 16) + yb for 8 16-bit Y values.
static __inline v128_t Y16ToRgb_WASMSIMD(v128_t y16,
                                         const struct YuvCoefs_WASMSIMD* c) {
  // The barrier keeps clang from widening yg outside the loop, which would
  // turn extmul (umull on arm64) into extend + i32x4.mul.
  v128_t yg = c->yg;
  asm volatile("" : "+r"(yg));
  v128_t lo = wasm_u32x4_extmul_low_u16x8(y16, yg);
  v128_t hi = wasm_u32x4_extmul_high_u16x8(y16, yg);
  return wasm_i16x8_add(wasm_i16x8_shuffle(lo, hi, 1, 3, 5, 7, 9, 11, 13, 15),
                        c->yb);
}

// Returns ((y * 0x0101 * yg) >> 16) + yb for 8 Y in 16-bit lanes.
static __inline v128_t YToRgb_WASMSIMD(v128_t y,
                                       const struct YuvCoefs_WASMSIMD* c) {
  return Y16ToRgb_WASMSIMD(wasm_i16x8_mul(y, c->k0101), c);
}

// Converts 16 pixels to B, G, R with 6 fractional bits (unclamped int16).
// y = 16 Y. uv = 8 U/V pairs, U in the low byte of each 16-bit lane (V if vu).
// Even and odd pixels are kept apart so each pair shares its chroma terms:
// rgb[0], rgb[2], rgb[4] = even B, G, R and rgb[1], rgb[3], rgb[5] = odd.
static __inline void YuvToRgb16_WASMSIMD(v128_t y,
                                         v128_t uv,
                                         int vu,
                                         const struct YuvCoefs_WASMSIMD* c,
                                         v128_t* rgb) {
  v128_t ye = YToRgb_WASMSIMD(wasm_v128_and(y, c->k00ff), c);
  v128_t yo = YToRgb_WASMSIMD(wasm_u16x8_shr(y, 8), c);
  // u - 128 and v - 128 as int8, sign extended to int16.
  v128_t uvs = wasm_v128_xor(uv, c->x80);
  v128_t lo = wasm_i16x8_shr(wasm_i16x8_shl(uvs, 8), 8);
  v128_t hi = wasm_i16x8_shr(uvs, 8);
  v128_t ui = vu ? hi : lo;
  v128_t vi = vu ? lo : hi;
  v128_t b = wasm_i16x8_mul(ui, c->ub);
  v128_t g =
      wasm_i16x8_add(wasm_i16x8_mul(ui, c->ug), wasm_i16x8_mul(vi, c->vg));
  v128_t r = wasm_i16x8_mul(vi, c->vr);
  rgb[0] = wasm_i16x8_add_sat(ye, b);
  rgb[1] = wasm_i16x8_add_sat(yo, b);
  rgb[2] = wasm_i16x8_sub_sat(ye, g);
  rgb[3] = wasm_i16x8_sub_sat(yo, g);
  rgb[4] = wasm_i16x8_add_sat(ye, r);
  rgb[5] = wasm_i16x8_add_sat(yo, r);
}

// Converts 16 4:4:4 pixels to even/odd B, G, R with 6 fractional bits.
static __inline void Yuv444ToRgb16_WASMSIMD(v128_t y,
                                            v128_t u,
                                            v128_t v,
                                            const struct YuvCoefs_WASMSIMD* c,
                                            v128_t* rgb) {
  v128_t ye = YToRgb_WASMSIMD(wasm_v128_and(y, c->k00ff), c);
  v128_t yo = YToRgb_WASMSIMD(wasm_u16x8_shr(y, 8), c);
  v128_t us = wasm_v128_xor(u, c->x80);
  v128_t vs = wasm_v128_xor(v, c->x80);
  v128_t ue = wasm_i16x8_shr(wasm_i16x8_shl(us, 8), 8);
  v128_t uo = wasm_i16x8_shr(us, 8);
  v128_t ve = wasm_i16x8_shr(wasm_i16x8_shl(vs, 8), 8);
  v128_t vo = wasm_i16x8_shr(vs, 8);
  rgb[0] = wasm_i16x8_add_sat(ye, wasm_i16x8_mul(ue, c->ub));
  rgb[1] = wasm_i16x8_add_sat(yo, wasm_i16x8_mul(uo, c->ub));
  rgb[2] = wasm_i16x8_sub_sat(
      ye, wasm_i16x8_add(wasm_i16x8_mul(ue, c->ug), wasm_i16x8_mul(ve, c->vg)));
  rgb[3] = wasm_i16x8_sub_sat(
      yo, wasm_i16x8_add(wasm_i16x8_mul(uo, c->ug), wasm_i16x8_mul(vo, c->vg)));
  rgb[4] = wasm_i16x8_add_sat(ye, wasm_i16x8_mul(ve, c->vr));
  rgb[5] = wasm_i16x8_add_sat(yo, wasm_i16x8_mul(vo, c->vr));
}

// Loads 16 Y and 8 U/V, or 8 Y and 4 U/V when width < 16.
static __inline void ReadI422_WASMSIMD(const uint8_t* src_y,
                                       const uint8_t* src_u,
                                       const uint8_t* src_v,
                                       int width,
                                       v128_t* y,
                                       v128_t* uv) {
  if (width >= 16) {
    *y = wasm_v128_load(src_y);
    *uv = ZipLo8_WASMSIMD(wasm_v128_load64_zero(src_u),
                          wasm_v128_load64_zero(src_v));
  } else {
    *y = wasm_v128_load64_zero(src_y);
    *uv = ZipLo8_WASMSIMD(wasm_v128_load32_zero(src_u),
                          wasm_v128_load32_zero(src_v));
  }
}

// Loads 16 Y and 8 UV pairs, or 8 Y and 4 UV pairs when width < 16.
static __inline void ReadNV12_WASMSIMD(const uint8_t* src_y,
                                       const uint8_t* src_uv,
                                       int width,
                                       v128_t* y,
                                       v128_t* uv) {
  if (width >= 16) {
    *y = wasm_v128_load(src_y);
    *uv = wasm_v128_load(src_uv);
  } else {
    *y = wasm_v128_load64_zero(src_y);
    *uv = wasm_v128_load64_zero(src_uv);
  }
}

// Loads 16 Y, 16 U, and 16 V, or 8 of each when width < 16.
static __inline void ReadI444_WASMSIMD(const uint8_t* src_y,
                                       const uint8_t* src_u,
                                       const uint8_t* src_v,
                                       int width,
                                       v128_t* y,
                                       v128_t* u,
                                       v128_t* v) {
  if (width >= 16) {
    *y = wasm_v128_load(src_y);
    *u = wasm_v128_load(src_u);
    *v = wasm_v128_load(src_v);
  } else {
    *y = wasm_v128_load64_zero(src_y);
    *u = wasm_v128_load64_zero(src_u);
    *v = wasm_v128_load64_zero(src_v);
  }
}
#endif

#if defined(HAS_I422TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I422ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_NV12TOARGBROW_WASMSIMD) ||      \
    defined(HAS_NV21TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I444TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I444ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_YUY2TOARGBROW_WASMSIMD) ||      \
    defined(HAS_UYVYTOARGBROW_WASMSIMD) ||      \
    defined(HAS_I210TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I210ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_I410TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I410ALPHATOARGBROW_WASMSIMD)
// Clamps even/odd B, G, R to 8 bits and stores 16 ARGB pixels, or 8 when
// width < 16. ff holds 8 odd alphas in bytes 0..7 and 8 even alphas in 8..15.
static __inline void StoreARGB_WASMSIMD(const v128_t* rgb,
                                        v128_t ff,
                                        uint8_t* dst_argb,
                                        int width) {
  v128_t bg_e = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[0], 6),
                                        wasm_i16x8_shr(rgb[2], 6));
  v128_t gb_o = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[3], 6),
                                        wasm_i16x8_shr(rgb[1], 6));
  v128_t rr = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[4], 6),
                                      wasm_i16x8_shr(rgb[5], 6));
  v128_t br_e = ZipLo8_WASMSIMD(bg_e, rr);
  v128_t ga_e = ZipHi8_WASMSIMD(bg_e, ff);
  v128_t br_o = ZipHi8_WASMSIMD(gb_o, rr);
  v128_t ga_o = ZipLo8_WASMSIMD(gb_o, ff);
  v128_t e = ZipLo8_WASMSIMD(br_e, ga_e);  // Pixels 0, 2, 4, 6.
  v128_t o = ZipLo8_WASMSIMD(br_o, ga_o);  // Pixels 1, 3, 5, 7.
  wasm_v128_store(dst_argb, wasm_i32x4_shuffle(e, o, 0, 4, 1, 5));
  wasm_v128_store(dst_argb + 16, wasm_i32x4_shuffle(e, o, 2, 6, 3, 7));
  if (width >= 16) {
    e = ZipHi8_WASMSIMD(br_e, ga_e);
    o = ZipHi8_WASMSIMD(br_o, ga_o);
    wasm_v128_store(dst_argb + 32, wasm_i32x4_shuffle(e, o, 0, 4, 1, 5));
    wasm_v128_store(dst_argb + 48, wasm_i32x4_shuffle(e, o, 2, 6, 3, 7));
  }
}

static __inline v128_t PackAlphaOddEven_WASMSIMD(v128_t a) {
  return wasm_i8x16_shuffle(a, a, 1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10,
                            12, 14);
}
#endif

#ifdef HAS_I422TORGBAROW_WASMSIMD
// Clamps even/odd B, G, R to 8 bits and stores 16 RGBA pixels, or 8 when
// width < 16.
static __inline void StoreRGBA_WASMSIMD(const v128_t* rgb,
                                        v128_t ff,
                                        uint8_t* dst_rgba,
                                        int width) {
  v128_t bg_e = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[0], 6),
                                        wasm_i16x8_shr(rgb[2], 6));
  v128_t gb_o = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[3], 6),
                                        wasm_i16x8_shr(rgb[1], 6));
  v128_t rr = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[4], 6),
                                      wasm_i16x8_shr(rgb[5], 6));
  v128_t br_e = ZipLo8_WASMSIMD(bg_e, rr);
  v128_t ag_e = ZipHi8_WASMSIMD(ff, bg_e);
  v128_t br_o = ZipHi8_WASMSIMD(gb_o, rr);
  v128_t ag_o = ZipLo8_WASMSIMD(ff, gb_o);
  v128_t e = ZipLo8_WASMSIMD(ag_e, br_e);  // Pixels 0, 2, 4, 6.
  v128_t o = ZipLo8_WASMSIMD(ag_o, br_o);  // Pixels 1, 3, 5, 7.
  wasm_v128_store(dst_rgba, wasm_i32x4_shuffle(e, o, 0, 4, 1, 5));
  wasm_v128_store(dst_rgba + 16, wasm_i32x4_shuffle(e, o, 2, 6, 3, 7));
  if (width >= 16) {
    e = ZipHi8_WASMSIMD(ag_e, br_e);
    o = ZipHi8_WASMSIMD(ag_o, br_o);
    wasm_v128_store(dst_rgba + 32, wasm_i32x4_shuffle(e, o, 0, 4, 1, 5));
    wasm_v128_store(dst_rgba + 48, wasm_i32x4_shuffle(e, o, 2, 6, 3, 7));
  }
}
#endif

#ifdef HAS_I422TOARGBROW_WASMSIMD
void I422ToARGBRow_WASMSIMD(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadI422_WASMSIMD(src_y, src_u, src_v, width, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    StoreARGB_WASMSIMD(rgb, ff, dst_argb, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422TOARGBROW_WASMSIMD

#ifdef HAS_I422ALPHATOARGBROW_WASMSIMD
void I422AlphaToARGBRow_WASMSIMD(const uint8_t* src_y,
                                 const uint8_t* src_u,
                                 const uint8_t* src_v,
                                 const uint8_t* src_a,
                                 uint8_t* dst_argb,
                                 const struct YuvConstants* yuvconstants,
                                 int width) {
  struct YuvCoefs_WASMSIMD c;
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadI422_WASMSIMD(src_y, src_u, src_v, width, &y, &uv);
    v128_t a =
        (width >= 16) ? wasm_v128_load(src_a) : wasm_v128_load64_zero(src_a);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    StoreARGB_WASMSIMD(rgb, PackAlphaOddEven_WASMSIMD(a), dst_argb, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    src_a += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422ALPHATOARGBROW_WASMSIMD

#ifdef HAS_I422TORGBAROW_WASMSIMD
void I422ToRGBARow_WASMSIMD(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_rgba,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadI422_WASMSIMD(src_y, src_u, src_v, width, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    StoreRGBA_WASMSIMD(rgb, ff, dst_rgba, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_rgba += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422TORGBAROW_WASMSIMD

#if defined(HAS_I422TOAR30ROW_WASMSIMD) || \
    defined(HAS_I210TOAR30ROW_WASMSIMD) || defined(HAS_I410TOAR30ROW_WASMSIMD)
// Clamps B, G, R with 6 fractional bits to 10 bits and packs them into the
// low and high 16 bits of AR30, without the alpha bits.
static __inline void RgbToAR30_WASMSIMD(v128_t b,
                                        v128_t g,
                                        v128_t r,
                                        v128_t zero,
                                        v128_t k3ff0,
                                        v128_t* lo,
                                        v128_t* hi) {
  b = wasm_i16x8_max(wasm_i16x8_min(b, k3ff0), zero);
  g = wasm_i16x8_max(wasm_i16x8_min(g, k3ff0), zero);
  r = wasm_i16x8_max(wasm_i16x8_min(r, k3ff0), zero);
  *lo = wasm_v128_or(wasm_u16x8_shr(b, 4),
                     wasm_i16x8_shl(wasm_v128_and(g, k3ff0), 6));
  *hi = wasm_v128_or(wasm_v128_and(r, k3ff0), wasm_u16x8_shr(g, 10));
}

static __inline void StoreAR30_WASMSIMD(const v128_t* rgb,
                                        v128_t zero,
                                        v128_t k3ff0,
                                        v128_t kc000,
                                        uint8_t* dst_ar30,
                                        int width) {
  v128_t lo_e, hi_e, lo_o, hi_o;
  RgbToAR30_WASMSIMD(rgb[0], rgb[2], rgb[4], zero, k3ff0, &lo_e, &hi_e);
  RgbToAR30_WASMSIMD(rgb[1], rgb[3], rgb[5], zero, k3ff0, &lo_o, &hi_o);
  hi_e = wasm_v128_or(hi_e, kc000);
  hi_o = wasm_v128_or(hi_o, kc000);
  // Even pixels 0, 2, 4, 6 and odd pixels 1, 3, 5, 7 as 32 bits.
  v128_t e = wasm_i16x8_shuffle(lo_e, hi_e, 0, 8, 1, 9, 2, 10, 3, 11);
  v128_t o = wasm_i16x8_shuffle(lo_o, hi_o, 0, 8, 1, 9, 2, 10, 3, 11);
  wasm_v128_store(dst_ar30, wasm_i32x4_shuffle(e, o, 0, 4, 1, 5));
  wasm_v128_store(dst_ar30 + 16, wasm_i32x4_shuffle(e, o, 2, 6, 3, 7));
  if (width >= 16) {
    e = wasm_i16x8_shuffle(lo_e, hi_e, 4, 12, 5, 13, 6, 14, 7, 15);
    o = wasm_i16x8_shuffle(lo_o, hi_o, 4, 12, 5, 13, 6, 14, 7, 15);
    wasm_v128_store(dst_ar30 + 32, wasm_i32x4_shuffle(e, o, 0, 4, 1, 5));
    wasm_v128_store(dst_ar30 + 48, wasm_i32x4_shuffle(e, o, 2, 6, 3, 7));
  }
}
#endif

#ifdef HAS_I422TOAR30ROW_WASMSIMD
void I422ToAR30Row_WASMSIMD(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_ar30,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t zero = SplatConst16(0);
  v128_t k3ff0 = SplatConst16(0x3ff0);
  v128_t kc000 = SplatConst16(0xc000);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 24, &c);  // AR30 bias.
  do {
    v128_t y, uv, rgb[6];
    ReadI422_WASMSIMD(src_y, src_u, src_v, width, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    StoreAR30_WASMSIMD(rgb, zero, k3ff0, kc000, dst_ar30, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_ar30 += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422TOAR30ROW_WASMSIMD

#ifdef HAS_NV12TOARGBROW_WASMSIMD
void NV12ToARGBRow_WASMSIMD(const uint8_t* src_y,
                            const uint8_t* src_uv,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadNV12_WASMSIMD(src_y, src_uv, width, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    StoreARGB_WASMSIMD(rgb, ff, dst_argb, width);
    src_y += 16;
    src_uv += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_NV12TOARGBROW_WASMSIMD

#ifdef HAS_NV21TOARGBROW_WASMSIMD
void NV21ToARGBRow_WASMSIMD(const uint8_t* src_y,
                            const uint8_t* src_vu,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, vu, rgb[6];
    ReadNV12_WASMSIMD(src_y, src_vu, width, &y, &vu);
    YuvToRgb16_WASMSIMD(y, vu, 1, &c, rgb);
    StoreARGB_WASMSIMD(rgb, ff, dst_argb, width);
    src_y += 16;
    src_vu += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_NV21TOARGBROW_WASMSIMD

#ifdef HAS_I444TOARGBROW_WASMSIMD
void I444ToARGBRow_WASMSIMD(const uint8_t* src_y,
                            const uint8_t* src_u,
                            const uint8_t* src_v,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, u, v, rgb[6];
    ReadI444_WASMSIMD(src_y, src_u, src_v, width, &y, &u, &v);
    Yuv444ToRgb16_WASMSIMD(y, u, v, &c, rgb);
    StoreARGB_WASMSIMD(rgb, ff, dst_argb, width);
    src_y += 16;
    src_u += 16;
    src_v += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I444TOARGBROW_WASMSIMD

#ifdef HAS_I444ALPHATOARGBROW_WASMSIMD
void I444AlphaToARGBRow_WASMSIMD(const uint8_t* src_y,
                                 const uint8_t* src_u,
                                 const uint8_t* src_v,
                                 const uint8_t* src_a,
                                 uint8_t* dst_argb,
                                 const struct YuvConstants* yuvconstants,
                                 int width) {
  struct YuvCoefs_WASMSIMD c;
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, u, v, rgb[6];
    ReadI444_WASMSIMD(src_y, src_u, src_v, width, &y, &u, &v);
    v128_t a =
        (width >= 16) ? wasm_v128_load(src_a) : wasm_v128_load64_zero(src_a);
    Yuv444ToRgb16_WASMSIMD(y, u, v, &c, rgb);
    StoreARGB_WASMSIMD(rgb, PackAlphaOddEven_WASMSIMD(a), dst_argb, width);
    src_y += 16;
    src_u += 16;
    src_v += 16;
    src_a += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I444ALPHATOARGBROW_WASMSIMD

#if defined(HAS_I422TORGB24ROW_WASMSIMD) ||    \
    defined(HAS_I422TORGB24ROW_RELAXEDSIMD) || \
    defined(HAS_NV12TORGB24ROW_WASMSIMD) ||    \
    defined(HAS_NV12TORGB24ROW_RELAXEDSIMD) || \
    defined(HAS_NV21TORGB24ROW_WASMSIMD) ||    \
    defined(HAS_NV21TORGB24ROW_RELAXEDSIMD) || \
    defined(HAS_I444TORGB24ROW_WASMSIMD) ||    \
    defined(HAS_I444TORGB24ROW_RELAXEDSIMD)
// Swizzles that interleave B/G pairs and R into 3 vectors of RGB24. R is
// 8 even pixels followed by 8 odd pixels.
static const uint8_t kI422ToRGB24Swizzle[96] = {
    0,   1,   128, 2,   3,   128, 4,   5,   128, 6,   7,   128, 8,   9,
    128, 10,  128, 128, 0,   128, 128, 8,   128, 128, 1,   128, 128, 9,
    128, 128, 2,   128, 3,   128, 4,   5,   128, 6,   7,   128, 8,   9,
    128, 10,  11,  128, 12,  13,  128, 10,  128, 128, 3,   128, 128, 11,
    128, 128, 4,   128, 128, 12,  128, 128, 128, 6,   7,   128, 8,   9,
    128, 10,  11,  128, 12,  13,  128, 14,  15,  128, 5,   128, 128, 13,
    128, 128, 6,   128, 128, 14,  128, 128, 7,   128, 128, 15};

template <bool kRelaxed>
static __inline void StoreRGB24_Wasm(const v128_t* rgb,
                                     const v128_t* m,
                                     uint8_t* dst_rgb24) {
  v128_t bg_e = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[0], 6),
                                        wasm_i16x8_shr(rgb[2], 6));
  v128_t bg_o = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[1], 6),
                                        wasm_i16x8_shr(rgb[3], 6));
  v128_t r = wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(rgb[4], 6),
                                     wasm_i16x8_shr(rgb[5], 6));
  v128_t b = ZipLo8_WASMSIMD(bg_e, bg_o);
  v128_t g = ZipHi8_WASMSIMD(bg_e, bg_o);
  v128_t bg0 = ZipLo8_WASMSIMD(b, g);
  v128_t bg2 = ZipHi8_WASMSIMD(b, g);
  v128_t bg1 = wasm_i8x16_shuffle(bg0, bg2, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                                  17, 18, 19, 20, 21, 22, 23);
  wasm_v128_store(dst_rgb24, wasm_v128_or(WasmSwizzle<kRelaxed>(bg0, m[0]),
                                          WasmSwizzle<kRelaxed>(r, m[1])));
  wasm_v128_store(dst_rgb24 + 16, wasm_v128_or(WasmSwizzle<kRelaxed>(bg1, m[2]),
                                               WasmSwizzle<kRelaxed>(r, m[3])));
  wasm_v128_store(dst_rgb24 + 32, wasm_v128_or(WasmSwizzle<kRelaxed>(bg2, m[4]),
                                               WasmSwizzle<kRelaxed>(r, m[5])));
}

static __inline void LoadRGB24Swizzle_WASMSIMD(v128_t* m) {
  for (int i = 0; i < 6; ++i) {
    m[i] = LoadConst(kI422ToRGB24Swizzle + i * 16);
  }
}
#endif

#if defined(HAS_I422TORGB24ROW_WASMSIMD) || \
    defined(HAS_I422TORGB24ROW_RELAXEDSIMD)
template <bool kRelaxed>
static inline void I422ToRGB24Row_Wasm(const uint8_t* src_y,
                                       const uint8_t* src_u,
                                       const uint8_t* src_v,
                                       uint8_t* dst_rgb24,
                                       const struct YuvConstants* yuvconstants,
                                       int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t m[6];
  LoadRGB24Swizzle_WASMSIMD(m);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadI422_WASMSIMD(src_y, src_u, src_v, 16, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    StoreRGB24_Wasm<kRelaxed>(rgb, m, dst_rgb24);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_rgb24 += 48;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_I422TORGB24ROW_WASMSIMD
void I422ToRGB24Row_WASMSIMD(const uint8_t* src_y,
                             const uint8_t* src_u,
                             const uint8_t* src_v,
                             uint8_t* dst_rgb24,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  I422ToRGB24Row_Wasm<false>(src_y, src_u, src_v, dst_rgb24, yuvconstants,
                             width);
}
#endif  // HAS_I422TORGB24ROW_WASMSIMD

#ifdef HAS_I422TORGB24ROW_RELAXEDSIMD
void I422ToRGB24Row_RELAXEDSIMD(const uint8_t* src_y,
                                const uint8_t* src_u,
                                const uint8_t* src_v,
                                uint8_t* dst_rgb24,
                                const struct YuvConstants* yuvconstants,
                                int width) {
  I422ToRGB24Row_Wasm<true>(src_y, src_u, src_v, dst_rgb24, yuvconstants,
                            width);
}
#endif  // HAS_I422TORGB24ROW_RELAXEDSIMD
#endif  // HAS_I422TORGB24ROW_WASMSIMD || HAS_I422TORGB24ROW_RELAXEDSIMD

#if defined(HAS_NV12TORGB24ROW_WASMSIMD) ||    \
    defined(HAS_NV12TORGB24ROW_RELAXEDSIMD) || \
    defined(HAS_NV21TORGB24ROW_WASMSIMD) ||    \
    defined(HAS_NV21TORGB24ROW_RELAXEDSIMD)
template <bool kRelaxed, int kVu>
static inline void NVToRGB24Row_Wasm(const uint8_t* src_y,
                                     const uint8_t* src_uv,
                                     uint8_t* dst_rgb24,
                                     const struct YuvConstants* yuvconstants,
                                     int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t m[6];
  LoadRGB24Swizzle_WASMSIMD(m);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadNV12_WASMSIMD(src_y, src_uv, 16, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, kVu, &c, rgb);
    StoreRGB24_Wasm<kRelaxed>(rgb, m, dst_rgb24);
    src_y += 16;
    src_uv += 16;
    dst_rgb24 += 48;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_NV12TORGB24ROW_WASMSIMD
void NV12ToRGB24Row_WASMSIMD(const uint8_t* src_y,
                             const uint8_t* src_uv,
                             uint8_t* dst_rgb24,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  NVToRGB24Row_Wasm<false, 0>(src_y, src_uv, dst_rgb24, yuvconstants, width);
}
#endif  // HAS_NV12TORGB24ROW_WASMSIMD

#ifdef HAS_NV12TORGB24ROW_RELAXEDSIMD
void NV12ToRGB24Row_RELAXEDSIMD(const uint8_t* src_y,
                                const uint8_t* src_uv,
                                uint8_t* dst_rgb24,
                                const struct YuvConstants* yuvconstants,
                                int width) {
  NVToRGB24Row_Wasm<true, 0>(src_y, src_uv, dst_rgb24, yuvconstants, width);
}
#endif  // HAS_NV12TORGB24ROW_RELAXEDSIMD

#ifdef HAS_NV21TORGB24ROW_WASMSIMD
void NV21ToRGB24Row_WASMSIMD(const uint8_t* src_y,
                             const uint8_t* src_vu,
                             uint8_t* dst_rgb24,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  NVToRGB24Row_Wasm<false, 1>(src_y, src_vu, dst_rgb24, yuvconstants, width);
}
#endif  // HAS_NV21TORGB24ROW_WASMSIMD

#ifdef HAS_NV21TORGB24ROW_RELAXEDSIMD
void NV21ToRGB24Row_RELAXEDSIMD(const uint8_t* src_y,
                                const uint8_t* src_vu,
                                uint8_t* dst_rgb24,
                                const struct YuvConstants* yuvconstants,
                                int width) {
  NVToRGB24Row_Wasm<true, 1>(src_y, src_vu, dst_rgb24, yuvconstants, width);
}
#endif  // HAS_NV21TORGB24ROW_RELAXEDSIMD
#endif

#if defined(HAS_I444TORGB24ROW_WASMSIMD) || \
    defined(HAS_I444TORGB24ROW_RELAXEDSIMD)
template <bool kRelaxed>
static inline void I444ToRGB24Row_Wasm(const uint8_t* src_y,
                                       const uint8_t* src_u,
                                       const uint8_t* src_v,
                                       uint8_t* dst_rgb24,
                                       const struct YuvConstants* yuvconstants,
                                       int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t m[6];
  LoadRGB24Swizzle_WASMSIMD(m);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, u, v, rgb[6];
    ReadI444_WASMSIMD(src_y, src_u, src_v, 16, &y, &u, &v);
    Yuv444ToRgb16_WASMSIMD(y, u, v, &c, rgb);
    StoreRGB24_Wasm<kRelaxed>(rgb, m, dst_rgb24);
    src_y += 16;
    src_u += 16;
    src_v += 16;
    dst_rgb24 += 48;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_I444TORGB24ROW_WASMSIMD
void I444ToRGB24Row_WASMSIMD(const uint8_t* src_y,
                             const uint8_t* src_u,
                             const uint8_t* src_v,
                             uint8_t* dst_rgb24,
                             const struct YuvConstants* yuvconstants,
                             int width) {
  I444ToRGB24Row_Wasm<false>(src_y, src_u, src_v, dst_rgb24, yuvconstants,
                             width);
}
#endif  // HAS_I444TORGB24ROW_WASMSIMD

#ifdef HAS_I444TORGB24ROW_RELAXEDSIMD
void I444ToRGB24Row_RELAXEDSIMD(const uint8_t* src_y,
                                const uint8_t* src_u,
                                const uint8_t* src_v,
                                uint8_t* dst_rgb24,
                                const struct YuvConstants* yuvconstants,
                                int width) {
  I444ToRGB24Row_Wasm<true>(src_y, src_u, src_v, dst_rgb24, yuvconstants,
                            width);
}
#endif  // HAS_I444TORGB24ROW_RELAXEDSIMD
#endif  // HAS_I444TORGB24ROW_WASMSIMD || HAS_I444TORGB24ROW_RELAXEDSIMD

#ifdef HAS_YUY2TOARGBROW_WASMSIMD
void YUY2ToARGBRow_WASMSIMD(const uint8_t* src_yuy2,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t s0 = wasm_v128_load(src_yuy2);
    v128_t s1 = (width >= 16) ? wasm_v128_load(src_yuy2 + 16) : s0;
    v128_t y = wasm_i8x16_shuffle(s0, s1, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
                                  22, 24, 26, 28, 30);
    v128_t uv = wasm_i8x16_shuffle(s0, s1, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
                                   21, 23, 25, 27, 29, 31);
    v128_t rgb[6];
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    StoreARGB_WASMSIMD(rgb, ff, dst_argb, width);
    src_yuy2 += 32;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_YUY2TOARGBROW_WASMSIMD

#ifdef HAS_UYVYTOARGBROW_WASMSIMD
void UYVYToARGBRow_WASMSIMD(const uint8_t* src_uyvy,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t s0 = wasm_v128_load(src_uyvy);
    v128_t s1 = (width >= 16) ? wasm_v128_load(src_uyvy + 16) : s0;
    v128_t uv = wasm_i8x16_shuffle(s0, s1, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18,
                                   20, 22, 24, 26, 28, 30);
    v128_t y = wasm_i8x16_shuffle(s0, s1, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21,
                                  23, 25, 27, 29, 31);
    v128_t rgb[6];
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    StoreARGB_WASMSIMD(rgb, ff, dst_argb, width);
    src_uyvy += 32;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_UYVYTOARGBROW_WASMSIMD

#if defined(HAS_I210TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I210ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_I410TOARGBROW_WASMSIMD) ||      \
    defined(HAS_I410ALPHATOARGBROW_WASMSIMD) || \
    defined(HAS_I210TOAR30ROW_WASMSIMD) || defined(HAS_I410TOAR30ROW_WASMSIMD)
// Loads 16 10-bit Y (or 8 when width < 16) and separates even and odd pixels,
// expanding 10-bit Y to 16 bits via (y << 6) | (y >> 4).
static __inline void ReadY10_WASMSIMD(const uint16_t* src_y,
                                      int width,
                                      const struct YuvCoefs_WASMSIMD* c,
                                      v128_t* ye,
                                      v128_t* yo) {
  v128_t y0 = wasm_v128_load(src_y);
  v128_t y1 = (width >= 16) ? wasm_v128_load(src_y + 8) : y0;
  y0 = wasm_v128_or(wasm_i16x8_shl(y0, 6), wasm_u16x8_shr(y0, 4));
  y1 = wasm_v128_or(wasm_i16x8_shl(y1, 6), wasm_u16x8_shr(y1, 4));
  *ye = Y16ToRgb_WASMSIMD(wasm_i16x8_shuffle(y0, y1, 0, 2, 4, 6, 8, 10, 12, 14),
                          c);
  *yo = Y16ToRgb_WASMSIMD(wasm_i16x8_shuffle(y0, y1, 1, 3, 5, 7, 9, 11, 13, 15),
                          c);
}

// Loads 8 10-bit U and V (or 4 when width < 16), shifts right by 2, clamps to
// [0, 255], and interleaves into 8 U/V pairs.
static __inline v128_t ReadUV210_WASMSIMD(const uint16_t* src_u,
                                          const uint16_t* src_v,
                                          int width) {
  v128_t u16 = wasm_u16x8_shr(
      (width >= 16) ? wasm_v128_load(src_u) : wasm_v128_load64_zero(src_u), 2);
  v128_t v16 = wasm_u16x8_shr(
      (width >= 16) ? wasm_v128_load(src_v) : wasm_v128_load64_zero(src_v), 2);
  v128_t uv8 = wasm_u8x16_narrow_i16x8(u16, v16);
  return wasm_i8x16_shuffle(uv8, uv8, 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6,
                            14, 7, 15);
}

// Loads 16 10-bit samples (or 8 when width < 16), shifts right by 2, and
// clamps to [0, 255].
static __inline v128_t Read16_10To8_WASMSIMD(const uint16_t* src, int width) {
  v128_t s0 = wasm_u16x8_shr(wasm_v128_load(src), 2);
  v128_t s1 = (width >= 16) ? wasm_u16x8_shr(wasm_v128_load(src + 8), 2) : s0;
  return wasm_u8x16_narrow_i16x8(s0, s1);
}

static __inline void Yuv210ToRgb16_WASMSIMD(const uint16_t* src_y,
                                            const uint16_t* src_u,
                                            const uint16_t* src_v,
                                            int width,
                                            const struct YuvCoefs_WASMSIMD* c,
                                            v128_t* rgb) {
  v128_t ye, yo;
  ReadY10_WASMSIMD(src_y, width, c, &ye, &yo);
  v128_t uvs = wasm_v128_xor(ReadUV210_WASMSIMD(src_u, src_v, width), c->x80);
  v128_t ui = wasm_i16x8_shr(wasm_i16x8_shl(uvs, 8), 8);
  v128_t vi = wasm_i16x8_shr(uvs, 8);
  v128_t b = wasm_i16x8_mul(ui, c->ub);
  v128_t g =
      wasm_i16x8_add(wasm_i16x8_mul(ui, c->ug), wasm_i16x8_mul(vi, c->vg));
  v128_t r = wasm_i16x8_mul(vi, c->vr);
  rgb[0] = wasm_i16x8_add_sat(ye, b);
  rgb[1] = wasm_i16x8_add_sat(yo, b);
  rgb[2] = wasm_i16x8_sub_sat(ye, g);
  rgb[3] = wasm_i16x8_sub_sat(yo, g);
  rgb[4] = wasm_i16x8_add_sat(ye, r);
  rgb[5] = wasm_i16x8_add_sat(yo, r);
}

static __inline void Yuv410ToRgb16_WASMSIMD(const uint16_t* src_y,
                                            const uint16_t* src_u,
                                            const uint16_t* src_v,
                                            int width,
                                            const struct YuvCoefs_WASMSIMD* c,
                                            v128_t* rgb) {
  v128_t ye, yo;
  ReadY10_WASMSIMD(src_y, width, c, &ye, &yo);
  v128_t us = wasm_v128_xor(Read16_10To8_WASMSIMD(src_u, width), c->x80);
  v128_t vs = wasm_v128_xor(Read16_10To8_WASMSIMD(src_v, width), c->x80);
  v128_t ue = wasm_i16x8_shr(wasm_i16x8_shl(us, 8), 8);
  v128_t uo = wasm_i16x8_shr(us, 8);
  v128_t ve = wasm_i16x8_shr(wasm_i16x8_shl(vs, 8), 8);
  v128_t vo = wasm_i16x8_shr(vs, 8);
  rgb[0] = wasm_i16x8_add_sat(ye, wasm_i16x8_mul(ue, c->ub));
  rgb[1] = wasm_i16x8_add_sat(yo, wasm_i16x8_mul(uo, c->ub));
  rgb[2] = wasm_i16x8_sub_sat(
      ye, wasm_i16x8_add(wasm_i16x8_mul(ue, c->ug), wasm_i16x8_mul(ve, c->vg)));
  rgb[3] = wasm_i16x8_sub_sat(
      yo, wasm_i16x8_add(wasm_i16x8_mul(uo, c->ug), wasm_i16x8_mul(vo, c->vg)));
  rgb[4] = wasm_i16x8_add_sat(ye, wasm_i16x8_mul(ve, c->vr));
  rgb[5] = wasm_i16x8_add_sat(yo, wasm_i16x8_mul(vo, c->vr));
}
#endif

#ifdef HAS_I210TOARGBROW_WASMSIMD
void I210ToARGBRow_WASMSIMD(const uint16_t* src_y,
                            const uint16_t* src_u,
                            const uint16_t* src_v,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t rgb[6];
    Yuv210ToRgb16_WASMSIMD(src_y, src_u, src_v, width, &c, rgb);
    StoreARGB_WASMSIMD(rgb, ff, dst_argb, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I210TOARGBROW_WASMSIMD

#ifdef HAS_I210ALPHATOARGBROW_WASMSIMD
void I210AlphaToARGBRow_WASMSIMD(const uint16_t* src_y,
                                 const uint16_t* src_u,
                                 const uint16_t* src_v,
                                 const uint16_t* src_a,
                                 uint8_t* dst_argb,
                                 const struct YuvConstants* yuvconstants,
                                 int width) {
  struct YuvCoefs_WASMSIMD c;
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t rgb[6];
    Yuv210ToRgb16_WASMSIMD(src_y, src_u, src_v, width, &c, rgb);
    v128_t a = Read16_10To8_WASMSIMD(src_a, width);
    StoreARGB_WASMSIMD(rgb, PackAlphaOddEven_WASMSIMD(a), dst_argb, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    src_a += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I210ALPHATOARGBROW_WASMSIMD

#ifdef HAS_I410TOARGBROW_WASMSIMD
void I410ToARGBRow_WASMSIMD(const uint16_t* src_y,
                            const uint16_t* src_u,
                            const uint16_t* src_v,
                            uint8_t* dst_argb,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t ff = SplatConst8(0xff);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t rgb[6];
    Yuv410ToRgb16_WASMSIMD(src_y, src_u, src_v, width, &c, rgb);
    StoreARGB_WASMSIMD(rgb, ff, dst_argb, width);
    src_y += 16;
    src_u += 16;
    src_v += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I410TOARGBROW_WASMSIMD

#ifdef HAS_I410ALPHATOARGBROW_WASMSIMD
void I410AlphaToARGBRow_WASMSIMD(const uint16_t* src_y,
                                 const uint16_t* src_u,
                                 const uint16_t* src_v,
                                 const uint16_t* src_a,
                                 uint8_t* dst_argb,
                                 const struct YuvConstants* yuvconstants,
                                 int width) {
  struct YuvCoefs_WASMSIMD c;
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t rgb[6];
    Yuv410ToRgb16_WASMSIMD(src_y, src_u, src_v, width, &c, rgb);
    v128_t a = Read16_10To8_WASMSIMD(src_a, width);
    StoreARGB_WASMSIMD(rgb, PackAlphaOddEven_WASMSIMD(a), dst_argb, width);
    src_y += 16;
    src_u += 16;
    src_v += 16;
    src_a += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I410ALPHATOARGBROW_WASMSIMD

#ifdef HAS_I210TOAR30ROW_WASMSIMD
void I210ToAR30Row_WASMSIMD(const uint16_t* src_y,
                            const uint16_t* src_u,
                            const uint16_t* src_v,
                            uint8_t* dst_ar30,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t zero = SplatConst16(0);
  v128_t k3ff0 = SplatConst16(0x3ff0);
  v128_t kc000 = SplatConst16(0xc000);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t rgb[6];
    Yuv210ToRgb16_WASMSIMD(src_y, src_u, src_v, width, &c, rgb);
    StoreAR30_WASMSIMD(rgb, zero, k3ff0, kc000, dst_ar30, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_ar30 += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I210TOAR30ROW_WASMSIMD

#ifdef HAS_I410TOAR30ROW_WASMSIMD
void I410ToAR30Row_WASMSIMD(const uint16_t* src_y,
                            const uint16_t* src_u,
                            const uint16_t* src_v,
                            uint8_t* dst_ar30,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t zero = SplatConst16(0);
  v128_t k3ff0 = SplatConst16(0x3ff0);
  v128_t kc000 = SplatConst16(0xc000);
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t rgb[6];
    Yuv410ToRgb16_WASMSIMD(src_y, src_u, src_v, width, &c, rgb);
    StoreAR30_WASMSIMD(rgb, zero, k3ff0, kc000, dst_ar30, width);
    src_y += 16;
    src_u += 16;
    src_v += 16;
    dst_ar30 += 64;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I410TOAR30ROW_WASMSIMD

#if defined(HAS_YUY2TOYROW_WASMSIMD) || defined(HAS_UYVYTOYROW_WASMSIMD)
void YUY2ToYRow_WASMSIMD(const uint8_t* src_yuy2, uint8_t* dst_y, int width) {
  do {
    asm("" : "+r"(src_yuy2));
    v128_t s0 = wasm_v128_load(src_yuy2);
    v128_t s1 = wasm_v128_load(src_yuy2 + 16);
    wasm_v128_store(dst_y, wasm_i8x16_shuffle(s0, s1, 0, 2, 4, 6, 8, 10, 12, 14,
                                              16, 18, 20, 22, 24, 26, 28, 30));
    src_yuy2 += 32;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}

void UYVYToYRow_WASMSIMD(const uint8_t* src_uyvy, uint8_t* dst_y, int width) {
  do {
    asm("" : "+r"(src_uyvy));
    v128_t s0 = wasm_v128_load(src_uyvy);
    v128_t s1 = wasm_v128_load(src_uyvy + 16);
    wasm_v128_store(dst_y, wasm_i8x16_shuffle(s0, s1, 1, 3, 5, 7, 9, 11, 13, 15,
                                              17, 19, 21, 23, 25, 27, 29, 31));
    src_uyvy += 32;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_YUY2TOYROW_WASMSIMD || HAS_UYVYTOYROW_WASMSIMD

#if defined(HAS_YUY2TOUVROW_WASMSIMD) ||    \
    defined(HAS_YUY2TOUV422ROW_WASMSIMD) || \
    defined(HAS_UYVYTOUVROW_WASMSIMD) || defined(HAS_UYVYTOUV422ROW_WASMSIMD)
void YUY2ToUV422Row_WASMSIMD(const uint8_t* src_yuy2,
                             uint8_t* dst_u,
                             uint8_t* dst_v,
                             int width) {
  do {
    asm("" : "+r"(src_yuy2));
    v128_t s0 = wasm_v128_load(src_yuy2);
    v128_t s1 = wasm_v128_load(src_yuy2 + 16);
    v128_t uv = wasm_i8x16_shuffle(s0, s1, 1, 5, 9, 13, 17, 21, 25, 29, 3, 7,
                                   11, 15, 19, 23, 27, 31);
    wasm_v128_store64_lane(dst_u, uv, 0);
    wasm_v128_store64_lane(dst_v, uv, 1);
    src_yuy2 += 32;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}

void YUY2ToUVRow_WASMSIMD(const uint8_t* src_yuy2,
                          int src_stride_yuy2,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width) {
  const uint8_t* src_yuy2_1 = src_yuy2 + src_stride_yuy2;
  do {
    asm("" : "+r"(src_yuy2), "+r"(src_yuy2_1));
    v128_t s0 =
        wasm_u8x16_avgr(wasm_v128_load(src_yuy2), wasm_v128_load(src_yuy2_1));
    v128_t s1 = wasm_u8x16_avgr(wasm_v128_load(src_yuy2 + 16),
                                wasm_v128_load(src_yuy2_1 + 16));
    v128_t uv = wasm_i8x16_shuffle(s0, s1, 1, 5, 9, 13, 17, 21, 25, 29, 3, 7,
                                   11, 15, 19, 23, 27, 31);
    wasm_v128_store64_lane(dst_u, uv, 0);
    wasm_v128_store64_lane(dst_v, uv, 1);
    src_yuy2 += 32;
    src_yuy2_1 += 32;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}

void UYVYToUV422Row_WASMSIMD(const uint8_t* src_uyvy,
                             uint8_t* dst_u,
                             uint8_t* dst_v,
                             int width) {
  do {
    asm("" : "+r"(src_uyvy));
    v128_t s0 = wasm_v128_load(src_uyvy);
    v128_t s1 = wasm_v128_load(src_uyvy + 16);
    v128_t uv = wasm_i8x16_shuffle(s0, s1, 0, 4, 8, 12, 16, 20, 24, 28, 2, 6,
                                   10, 14, 18, 22, 26, 30);
    wasm_v128_store64_lane(dst_u, uv, 0);
    wasm_v128_store64_lane(dst_v, uv, 1);
    src_uyvy += 32;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}

void UYVYToUVRow_WASMSIMD(const uint8_t* src_uyvy,
                          int src_stride_uyvy,
                          uint8_t* dst_u,
                          uint8_t* dst_v,
                          int width) {
  const uint8_t* src_uyvy_1 = src_uyvy + src_stride_uyvy;
  do {
    asm("" : "+r"(src_uyvy), "+r"(src_uyvy_1));
    v128_t s0 =
        wasm_u8x16_avgr(wasm_v128_load(src_uyvy), wasm_v128_load(src_uyvy_1));
    v128_t s1 = wasm_u8x16_avgr(wasm_v128_load(src_uyvy + 16),
                                wasm_v128_load(src_uyvy_1 + 16));
    v128_t uv = wasm_i8x16_shuffle(s0, s1, 0, 4, 8, 12, 16, 20, 24, 28, 2, 6,
                                   10, 14, 18, 22, 26, 30);
    wasm_v128_store64_lane(dst_u, uv, 0);
    wasm_v128_store64_lane(dst_v, uv, 1);
    src_uyvy += 32;
    src_uyvy_1 += 32;
    dst_u += 8;
    dst_v += 8;
    width -= 16;
  } while (width > 0);
}
#endif

#if defined(HAS_I422TORGB565ROW_WASMSIMD) ||   \
    defined(HAS_I422TOARGB1555ROW_WASMSIMD) || \
    defined(HAS_I422TOARGB4444ROW_WASMSIMD) || \
    defined(HAS_NV12TORGB565ROW_WASMSIMD)
// Clamps x with 6 fractional bits to 8 bits, keeps the top 'bits' and moves
// them to bit 'pos'.
static __inline v128_t PackChannel_WASMSIMD(v128_t x,
                                            v128_t zero,
                                            v128_t k3fff,
                                            int bits,
                                            int pos) {
  x = wasm_i16x8_max(wasm_i16x8_min(x, k3fff), zero);
  return wasm_i16x8_shl(wasm_u16x8_shr(x, 14 - bits), pos);
}

// Packs even/odd B, G, R into 16 bit pixels with the given channel sizes and
// stores 16 pixels, or 8 when width < 16. k = {0, 0x3fff, alpha} in 16-bit
// lanes. Alpha is set if the channels use fewer than 16 bits.
static __inline void Store16Bit_WASMSIMD(const v128_t* rgb,
                                         int bbits,
                                         int gbits,
                                         int rbits,
                                         const v128_t* k,
                                         uint8_t* dst,
                                         int width) {
  v128_t p[2];
  int i;
  for (i = 0; i < 2; ++i) {
    p[i] = wasm_v128_or(
        PackChannel_WASMSIMD(rgb[i], k[0], k[1], bbits, 0),
        wasm_v128_or(PackChannel_WASMSIMD(rgb[2 + i], k[0], k[1], gbits, bbits),
                     PackChannel_WASMSIMD(rgb[4 + i], k[0], k[1], rbits,
                                          bbits + gbits)));
    if (bbits + gbits + rbits < 16) {
      p[i] = wasm_v128_or(p[i], k[2]);
    }
  }
  wasm_v128_store(dst,
                  wasm_i16x8_shuffle(p[0], p[1], 0, 8, 1, 9, 2, 10, 3, 11));
  if (width >= 16) {
    wasm_v128_store(dst + 16,
                    wasm_i16x8_shuffle(p[0], p[1], 4, 12, 5, 13, 6, 14, 7, 15));
  }
}
#endif

#ifdef HAS_I422TORGB565ROW_WASMSIMD
void I422ToRGB565Row_WASMSIMD(const uint8_t* src_y,
                              const uint8_t* src_u,
                              const uint8_t* src_v,
                              uint8_t* dst_rgb565,
                              const struct YuvConstants* yuvconstants,
                              int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t k[2] = {SplatConst16(0), SplatConst16(0x3fff)};
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadI422_WASMSIMD(src_y, src_u, src_v, width, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    Store16Bit_WASMSIMD(rgb, 5, 6, 5, k, dst_rgb565, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_rgb565 += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422TORGB565ROW_WASMSIMD

#ifdef HAS_I422TOARGB1555ROW_WASMSIMD
void I422ToARGB1555Row_WASMSIMD(const uint8_t* src_y,
                                const uint8_t* src_u,
                                const uint8_t* src_v,
                                uint8_t* dst_argb1555,
                                const struct YuvConstants* yuvconstants,
                                int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t k[3] = {SplatConst16(0), SplatConst16(0x3fff), SplatConst16(0x8000)};
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadI422_WASMSIMD(src_y, src_u, src_v, width, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    Store16Bit_WASMSIMD(rgb, 5, 5, 5, k, dst_argb1555, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_argb1555 += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422TOARGB1555ROW_WASMSIMD

#ifdef HAS_I422TOARGB4444ROW_WASMSIMD
void I422ToARGB4444Row_WASMSIMD(const uint8_t* src_y,
                                const uint8_t* src_u,
                                const uint8_t* src_v,
                                uint8_t* dst_argb4444,
                                const struct YuvConstants* yuvconstants,
                                int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t k[3] = {SplatConst16(0), SplatConst16(0x3fff), SplatConst16(0xf000)};
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadI422_WASMSIMD(src_y, src_u, src_v, width, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    Store16Bit_WASMSIMD(rgb, 4, 4, 4, k, dst_argb4444, width);
    src_y += 16;
    src_u += 8;
    src_v += 8;
    dst_argb4444 += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_I422TOARGB4444ROW_WASMSIMD

#ifdef HAS_NV12TORGB565ROW_WASMSIMD
void NV12ToRGB565Row_WASMSIMD(const uint8_t* src_y,
                              const uint8_t* src_uv,
                              uint8_t* dst_rgb565,
                              const struct YuvConstants* yuvconstants,
                              int width) {
  struct YuvCoefs_WASMSIMD c;
  v128_t k[2] = {SplatConst16(0), SplatConst16(0x3fff)};
  LoadYuvCoefs_WASMSIMD(yuvconstants, 0, &c);
  do {
    v128_t y, uv, rgb[6];
    ReadNV12_WASMSIMD(src_y, src_uv, width, &y, &uv);
    YuvToRgb16_WASMSIMD(y, uv, 0, &c, rgb);
    Store16Bit_WASMSIMD(rgb, 5, 6, 5, k, dst_rgb565, width);
    src_y += 16;
    src_uv += 16;
    dst_rgb565 += 32;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_NV12TORGB565ROW_WASMSIMD

#if defined(HAS_ARGBBLENDROW_WASMSIMD) || defined(HAS_ARGBBLENDROW_RELAXEDSIMD)
// Broadcast 255 - alpha (from byte 3 of each pixel) to 16 bit lanes.
static const uint8_t kShuffleAlpha16[16] = {3,  128, 3,  128, 7,  128, 7,  128,
                                            11, 128, 11, 128, 15, 128, 15, 128};

template <bool kRelaxed>
static inline void ARGBBlendRow_Wasm(const uint8_t* src_argb,
                                     const uint8_t* src_argb1,
                                     uint8_t* dst_argb,
                                     int width) {
  v128_t shuf_a = LoadConst(kShuffleAlpha16);
  v128_t one = SplatConst16(1);
  v128_t mask_lo = SplatConst16(0x00ff);
  v128_t mask_alpha = SplatConst32((int)0xff000000);

  while (width >= 4) {
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb1);
    src_argb += 16;
    src_argb1 += 16;

    // a = 256 - alpha.
    v128_t a = wasm_i16x8_add(
        WasmSwizzle<kRelaxed>(wasm_v128_xor(s0, mask_alpha), shuf_a), one);
    v128_t s1_lo =
        wasm_u16x8_shr(wasm_i16x8_mul(wasm_v128_and(s1, mask_lo), a), 8);
    v128_t s1_hi =
        wasm_v128_andnot(wasm_i16x8_mul(wasm_u16x8_shr(s1, 8), a), mask_lo);

    v128_t res = wasm_v128_or(s0, mask_alpha);
    res = wasm_u8x16_add_sat(res, s1_lo);
    res = wasm_u8x16_add_sat(res, s1_hi);

    wasm_v128_store(dst_argb, res);
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

    *(uint32_t*)dst_argb = 0xff000000u | r | g | b;
    dst_argb += 4;
    --width;
  }
}

#ifdef HAS_ARGBBLENDROW_WASMSIMD
void ARGBBlendRow_WASMSIMD(const uint8_t* src_argb,
                           const uint8_t* src_argb1,
                           uint8_t* dst_argb,
                           int width) {
  ARGBBlendRow_Wasm<false>(src_argb, src_argb1, dst_argb, width);
}
#endif  // HAS_ARGBBLENDROW_WASMSIMD

#ifdef HAS_ARGBBLENDROW_RELAXEDSIMD
void ARGBBlendRow_RELAXEDSIMD(const uint8_t* src_argb,
                              const uint8_t* src_argb1,
                              uint8_t* dst_argb,
                              int width) {
  ARGBBlendRow_Wasm<true>(src_argb, src_argb1, dst_argb, width);
}
#endif  // HAS_ARGBBLENDROW_RELAXEDSIMD
#endif  // HAS_ARGBBLENDROW_WASMSIMD || HAS_ARGBBLENDROW_RELAXEDSIMD

#ifdef HAS_BLENDPLANEROW_WASMSIMD
void BlendPlaneRow_WASMSIMD(const uint8_t* src0,
                            const uint8_t* src1,
                            const uint8_t* alpha,
                            uint8_t* dst,
                            int width) {
  // src0 * a + src1 * (255 - a) + 255 fits in 16 bits. 255 - a is a ^ 255.
  // The high bytes are the result (uzp2 on Arm).
  v128_t c255 = SplatConst16(255);
  v128_t ff = SplatConst8(255);
  do {
    v128_t s0 = wasm_v128_load(src0);
    v128_t s1 = wasm_v128_load(src1);
    v128_t a = wasm_v128_load(alpha);
    v128_t ia = wasm_v128_xor(a, ff);
    src0 += 16;
    src1 += 16;
    alpha += 16;
    v128_t lo = wasm_i16x8_add(wasm_u16x8_extmul_low_u8x16(s0, a),
                               wasm_u16x8_extmul_low_u8x16(s1, ia));
    v128_t hi = wasm_i16x8_add(wasm_u16x8_extmul_high_u8x16(s0, a),
                               wasm_u16x8_extmul_high_u8x16(s1, ia));
    lo = wasm_i16x8_add(lo, c255);
    hi = wasm_i16x8_add(hi, c255);
    wasm_v128_store(dst, wasm_i8x16_shuffle(lo, hi, 1, 3, 5, 7, 9, 11, 13, 15,
                                            17, 19, 21, 23, 25, 27, 29, 31));
    dst += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_BLENDPLANEROW_WASMSIMD

#if defined(HAS_ARGBATTENUATEROW_WASMSIMD) || \
    defined(HAS_ARGBATTENUATEROW_RELAXEDSIMD)
static const uint8_t kShuffleAttenuateAlpha[16] = {
    3, 3, 3, 128, 7, 7, 7, 128, 11, 11, 11, 128, 15, 15, 15, 128};

template <bool kRelaxed>
static inline void ARGBAttenuateRow_Wasm(const uint8_t* src_argb,
                                         uint8_t* dst_argb,
                                         int width) {
  v128_t shuf_a = LoadConst(kShuffleAttenuateAlpha);
  v128_t mask_a = SplatConst32((int)0xff000000);
  v128_t c255 = SplatConst16(255);
  do {
    asm("" : "+r"(src_argb), "+r"(dst_argb));
    v128_t s = wasm_v128_load(src_argb);
    v128_t a = WasmSwizzle<kRelaxed>(s, shuf_a);
    v128_t p_lo = wasm_u16x8_shr(
        wasm_i16x8_add(wasm_u16x8_extmul_low_u8x16(s, a), c255), 8);
    v128_t p_hi = wasm_u16x8_shr(
        wasm_i16x8_add(wasm_u16x8_extmul_high_u8x16(s, a), c255), 8);
    v128_t res = wasm_v128_or(wasm_u8x16_narrow_i16x8(p_lo, p_hi),
                              wasm_v128_and(s, mask_a));
    wasm_v128_store(dst_argb, res);
    src_argb += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}

#ifdef HAS_ARGBATTENUATEROW_WASMSIMD
void ARGBAttenuateRow_WASMSIMD(const uint8_t* src_argb,
                               uint8_t* dst_argb,
                               int width) {
  ARGBAttenuateRow_Wasm<false>(src_argb, dst_argb, width);
}
#endif  // HAS_ARGBATTENUATEROW_WASMSIMD

#ifdef HAS_ARGBATTENUATEROW_RELAXEDSIMD
void ARGBAttenuateRow_RELAXEDSIMD(const uint8_t* src_argb,
                                  uint8_t* dst_argb,
                                  int width) {
  ARGBAttenuateRow_Wasm<true>(src_argb, dst_argb, width);
}
#endif  // HAS_ARGBATTENUATEROW_RELAXEDSIMD
#endif  // HAS_ARGBATTENUATEROW_WASMSIMD || HAS_ARGBATTENUATEROW_RELAXEDSIMD

#ifdef HAS_ARGBEXTRACTALPHAROW_WASMSIMD
void ARGBExtractAlphaRow_WASMSIMD(const uint8_t* src_argb,
                                  uint8_t* dst_a,
                                  int width) {
  do {
    asm("" : "+r"(src_argb), "+r"(dst_a));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    v128_t s2 = wasm_v128_load(src_argb + 32);
    v128_t s3 = wasm_v128_load(src_argb + 48);
    s0 = wasm_u32x4_shr(s0, 24);
    s1 = wasm_u32x4_shr(s1, 24);
    s2 = wasm_u32x4_shr(s2, 24);
    s3 = wasm_u32x4_shr(s3, 24);
    v128_t a = wasm_u8x16_narrow_i16x8(wasm_i16x8_narrow_i32x4(s0, s1),
                                       wasm_i16x8_narrow_i32x4(s2, s3));
    wasm_v128_store(dst_a, a);
    src_argb += 64;
    dst_a += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_ARGBEXTRACTALPHAROW_WASMSIMD

#if defined(HAS_ARGBCOPYYTOALPHAROW_WASMSIMD) || \
    defined(HAS_ARGBCOPYYTOALPHAROW_RELAXEDSIMD)
// Swizzle Y[i] to the alpha byte of 32 bit lane i; 128 indices give 0.
static const uint8_t kYToAlphaSwizzleTable[64] = {
    128, 128, 128, 0,  128, 128, 128, 1,  128, 128, 128, 2,  128, 128, 128, 3,
    128, 128, 128, 4,  128, 128, 128, 5,  128, 128, 128, 6,  128, 128, 128, 7,
    128, 128, 128, 8,  128, 128, 128, 9,  128, 128, 128, 10, 128, 128, 128, 11,
    128, 128, 128, 12, 128, 128, 128, 13, 128, 128, 128, 14, 128, 128, 128, 15};

template <bool kRelaxed>
static inline void ARGBCopyYToAlphaRow_Wasm(const uint8_t* src,
                                            uint8_t* dst,
                                            int width) {
  v128_t rgb_mask = SplatConst32(0x00ffffff);
  v128_t idx0 = LoadConst(kYToAlphaSwizzleTable);
  v128_t idx1 = LoadConst(kYToAlphaSwizzleTable + 16);
  v128_t idx2 = LoadConst(kYToAlphaSwizzleTable + 32);
  v128_t idx3 = LoadConst(kYToAlphaSwizzleTable + 48);
  do {
    asm("" : "+r"(src), "+r"(dst));
    v128_t y = wasm_v128_load(src);
    v128_t d0 = wasm_v128_and(wasm_v128_load(dst), rgb_mask);
    v128_t d1 = wasm_v128_and(wasm_v128_load(dst + 16), rgb_mask);
    v128_t d2 = wasm_v128_and(wasm_v128_load(dst + 32), rgb_mask);
    v128_t d3 = wasm_v128_and(wasm_v128_load(dst + 48), rgb_mask);
    wasm_v128_store(dst, wasm_v128_or(d0, WasmSwizzle<kRelaxed>(y, idx0)));
    wasm_v128_store(dst + 16, wasm_v128_or(d1, WasmSwizzle<kRelaxed>(y, idx1)));
    wasm_v128_store(dst + 32, wasm_v128_or(d2, WasmSwizzle<kRelaxed>(y, idx2)));
    wasm_v128_store(dst + 48, wasm_v128_or(d3, WasmSwizzle<kRelaxed>(y, idx3)));
    src += 16;
    dst += 64;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_ARGBCOPYYTOALPHAROW_WASMSIMD
void ARGBCopyYToAlphaRow_WASMSIMD(const uint8_t* src, uint8_t* dst, int width) {
  ARGBCopyYToAlphaRow_Wasm<false>(src, dst, width);
}
#endif  // HAS_ARGBCOPYYTOALPHAROW_WASMSIMD

#ifdef HAS_ARGBCOPYYTOALPHAROW_RELAXEDSIMD
void ARGBCopyYToAlphaRow_RELAXEDSIMD(const uint8_t* src,
                                     uint8_t* dst,
                                     int width) {
  ARGBCopyYToAlphaRow_Wasm<true>(src, dst, width);
}
#endif  // HAS_ARGBCOPYYTOALPHAROW_RELAXEDSIMD
#endif  // HAS_ARGBCOPYYTOALPHAROW_WASMSIMD ||
        // HAS_ARGBCOPYYTOALPHAROW_RELAXEDSIMD

#ifdef HAS_ARGBCOPYALPHAROW_WASMSIMD
void ARGBCopyAlphaRow_WASMSIMD(const uint8_t* src_argb,
                               uint8_t* dst_argb,
                               int width) {
  v128_t mask_alpha = SplatConst32((int)0xff000000);
  do {
    asm("" : "+r"(src_argb), "+r"(dst_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb + 16);
    v128_t d0 = wasm_v128_load(dst_argb);
    v128_t d1 = wasm_v128_load(dst_argb + 16);
    wasm_v128_store(dst_argb, wasm_v128_bitselect(s0, d0, mask_alpha));
    wasm_v128_store(dst_argb + 16, wasm_v128_bitselect(s1, d1, mask_alpha));
    src_argb += 32;
    dst_argb += 32;
    width -= 8;
  } while (width > 0);
}
#endif  // HAS_ARGBCOPYALPHAROW_WASMSIMD

#ifdef HAS_ARGBORROW_WASMSIMD
void ARGBOrRow_WASMSIMD(const uint8_t* src_argb,
                        uint8_t* dst_argb,
                        uint32_t value,
                        int width) {
  v128_t val = wasm_i32x4_splat((int32_t)value);
  do {
    asm("" : "+r"(src_argb), "+r"(dst_argb));
    v128_t s = wasm_v128_load(src_argb);
    wasm_v128_store(dst_argb, wasm_v128_or(s, val));
    src_argb += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBORROW_WASMSIMD

#ifdef HAS_ARGBALPHABLENDROW_WASMSIMD
void ARGBAlphaBlendRow_WASMSIMD(const uint8_t* src_argb0,
                                const uint8_t* src_argb1,
                                uint8_t* dst_argb,
                                int width) {
  v128_t c1 = SplatConst16(1);
  v128_t c00ffffff = SplatConst32(0x00ffffff);
  do {
    asm("" : "+r"(src_argb0), "+r"(src_argb1), "+r"(dst_argb));
    v128_t s = wasm_v128_load(src_argb0);
    v128_t d = wasm_v128_load(src_argb1);
    src_argb0 += 16;
    src_argb1 += 16;
    v128_t sa = wasm_u32x4_shr(s, 24);
    v128_t da = wasm_u32x4_shr(d, 24);
    v128_t prod16 = wasm_i16x8_add(wasm_i16x8_mul(da, sa), c1);
    v128_t da_sa =
        wasm_u16x8_shr(wasm_i16x8_add(prod16, wasm_u16x8_shr(prod16, 8)), 8);
    v128_t da2 = wasm_i32x4_add(da, wasm_i32x4_sub(sa, da_sa));
    v128_t t = wasm_u32x4_shr(wasm_i32x4_eq(da, wasm_i32x4_const_splat(0)), 24);
    v128_t sa_eff = wasm_v128_or(sa, t);
    v128_t den_eff = wasm_v128_or(da2, t);
    v128_t num = wasm_i32x4_sub(wasm_i32x4_shl(sa_eff, 8), sa_eff);
    v128_t ratio = wasm_u32x4_trunc_sat_f32x4(wasm_f32x4_div(
        wasm_f32x4_convert_u32x4(num), wasm_f32x4_convert_u32x4(den_eff)));
    v128_t r8 = wasm_i8x16_shuffle(ratio, ratio, 0, 0, 0, 0, 4, 4, 4, 4, 8, 8,
                                   8, 8, 12, 12, 12, 12);
    v128_t inv_r8 = wasm_v128_not(r8);
    v128_t acc_lo =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extmul_low_u8x16(d, inv_r8),
                                      wasm_u16x8_extmul_low_u8x16(s, r8)),
                       c1);
    v128_t acc_hi =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extmul_high_u8x16(d, inv_r8),
                                      wasm_u16x8_extmul_high_u8x16(s, r8)),
                       c1);
    acc_lo =
        wasm_u16x8_shr(wasm_i16x8_add(acc_lo, wasm_u16x8_shr(acc_lo, 8)), 8);
    acc_hi =
        wasm_u16x8_shr(wasm_i16x8_add(acc_hi, wasm_u16x8_shr(acc_hi, 8)), 8);
    v128_t rgb = wasm_u8x16_narrow_i16x8(acc_lo, acc_hi);
    v128_t out = wasm_v128_or(wasm_v128_and(rgb, c00ffffff),
                              wasm_i32x4_shl(da2, 24));
    wasm_v128_store(dst_argb, out);
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBALPHABLENDROW_WASMSIMD

#ifdef HAS_ARGBMULTIPLYROW_WASMSIMD
void ARGBMultiplyRow_WASMSIMD(const uint8_t* src_argb,
                              const uint8_t* src_argb1,
                              uint8_t* dst_argb,
                              int width) {
  v128_t c128 = SplatConst16(128);
  do {
    asm("" : "+r"(src_argb), "+r"(src_argb1), "+r"(dst_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb1);
    v128_t lo = wasm_u16x8_shr(
        wasm_i16x8_add(wasm_u16x8_extmul_low_u8x16(s0, s1), c128), 8);
    v128_t hi = wasm_u16x8_shr(
        wasm_i16x8_add(wasm_u16x8_extmul_high_u8x16(s0, s1), c128), 8);
    wasm_v128_store(dst_argb, wasm_u8x16_narrow_i16x8(lo, hi));
    src_argb += 16;
    src_argb1 += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBMULTIPLYROW_WASMSIMD

#ifdef HAS_ARGBADDROW_WASMSIMD
void ARGBAddRow_WASMSIMD(const uint8_t* src_argb,
                         const uint8_t* src_argb1,
                         uint8_t* dst_argb,
                         int width) {
  do {
    asm("" : "+r"(src_argb), "+r"(src_argb1), "+r"(dst_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb1);
    wasm_v128_store(dst_argb, wasm_u8x16_add_sat(s0, s1));
    src_argb += 16;
    src_argb1 += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBADDROW_WASMSIMD

#ifdef HAS_ARGBSUBTRACTROW_WASMSIMD
void ARGBSubtractRow_WASMSIMD(const uint8_t* src_argb,
                              const uint8_t* src_argb1,
                              uint8_t* dst_argb,
                              int width) {
  do {
    asm("" : "+r"(src_argb), "+r"(src_argb1), "+r"(dst_argb));
    v128_t s0 = wasm_v128_load(src_argb);
    v128_t s1 = wasm_v128_load(src_argb1);
    wasm_v128_store(dst_argb, wasm_u8x16_sub_sat(s0, s1));
    src_argb += 16;
    src_argb1 += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBSUBTRACTROW_WASMSIMD

#if defined(HAS_ARGBGRAYROW_WASMSIMD) || defined(HAS_ARGBGRAYROW_RELAXEDSIMD)
static const uint16_t kGrayCoeffs[8] = {29, 150, 77, 0, 29, 150, 77, 0};
static const uint8_t kShuffleGray[16] = {0, 0, 0, 128, 1, 1, 1, 128,
                                         2, 2, 2, 128, 3, 3, 3, 128};

template <bool kRelaxed>
static inline void ARGBGrayRow_Wasm(const uint8_t* src_argb,
                                    uint8_t* dst_argb,
                                    int width) {
  v128_t coeffs = LoadConst(kGrayCoeffs);
  v128_t shuf = LoadConst(kShuffleGray);
  v128_t mask_a = SplatConst32((int)0xff000000);
  v128_t c128 = SplatConst32(128);
  do {
    asm("" : "+r"(src_argb), "+r"(dst_argb));
    v128_t s = wasm_v128_load(src_argb);
    v128_t lo = wasm_i32x4_dot_i16x8(wasm_u16x8_extend_low_u8x16(s), coeffs);
    v128_t hi = wasm_i32x4_dot_i16x8(wasm_u16x8_extend_high_u8x16(s), coeffs);
    v128_t y32 = wasm_u32x4_shr(
        wasm_i32x4_add(wasm_i32x4_add(wasm_i32x4_shuffle(lo, hi, 0, 2, 4, 6),
                                      wasm_i32x4_shuffle(lo, hi, 1, 3, 5, 7)),
                       c128),
        8);
    v128_t y16 = wasm_i16x8_narrow_i32x4(y32, y32);
    v128_t y = wasm_u8x16_narrow_i16x8(y16, y16);
    wasm_v128_store(dst_argb, wasm_v128_or(WasmSwizzle<kRelaxed>(y, shuf),
                                           wasm_v128_and(s, mask_a)));
    src_argb += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}

#ifdef HAS_ARGBGRAYROW_WASMSIMD
void ARGBGrayRow_WASMSIMD(const uint8_t* src_argb,
                          uint8_t* dst_argb,
                          int width) {
  ARGBGrayRow_Wasm<false>(src_argb, dst_argb, width);
}
#endif  // HAS_ARGBGRAYROW_WASMSIMD

#ifdef HAS_ARGBGRAYROW_RELAXEDSIMD
void ARGBGrayRow_RELAXEDSIMD(const uint8_t* src_argb,
                             uint8_t* dst_argb,
                             int width) {
  ARGBGrayRow_Wasm<true>(src_argb, dst_argb, width);
}
#endif  // HAS_ARGBGRAYROW_RELAXEDSIMD
#endif  // HAS_ARGBGRAYROW_WASMSIMD || HAS_ARGBGRAYROW_RELAXEDSIMD

#if defined(HAS_ARGBSEPIAROW_WASMSIMD) || defined(HAS_ARGBSEPIAROW_RELAXEDSIMD)
static const uint16_t kSepiaCoeffsB16[8] = {17, 68, 35, 0, 17, 68, 35, 0};
static const uint16_t kSepiaCoeffsG16[8] = {22, 88, 45, 0, 22, 88, 45, 0};
static const uint16_t kSepiaCoeffsR16[8] = {24, 98, 50, 0, 24, 98, 50, 0};

#ifdef HAS_ARGBSEPIAROW_WASMSIMD
void ARGBSepiaRow_WASMSIMD(uint8_t* dst_argb, int width) {
  v128_t cb = LoadConst(kSepiaCoeffsB16);
  v128_t cg = LoadConst(kSepiaCoeffsG16);
  v128_t cr = LoadConst(kSepiaCoeffsR16);
  v128_t mask_a = SplatConst32((int)0xff000000);
  do {
    v128_t s = wasm_v128_load(dst_argb);
    v128_t s_lo = wasm_u16x8_extend_low_u8x16(s);
    v128_t s_hi = wasm_u16x8_extend_high_u8x16(s);
    v128_t b0 = wasm_i32x4_dot_i16x8(s_lo, cb);
    v128_t b1 = wasm_i32x4_dot_i16x8(s_hi, cb);
    v128_t g0 = wasm_i32x4_dot_i16x8(s_lo, cg);
    v128_t g1 = wasm_i32x4_dot_i16x8(s_hi, cg);
    v128_t r0 = wasm_i32x4_dot_i16x8(s_lo, cr);
    v128_t r1 = wasm_i32x4_dot_i16x8(s_hi, cr);
    v128_t b32 =
        wasm_u32x4_shr(wasm_i32x4_add(wasm_i32x4_shuffle(b0, b1, 0, 2, 4, 6),
                                      wasm_i32x4_shuffle(b0, b1, 1, 3, 5, 7)),
                       7);
    v128_t g32 =
        wasm_u32x4_shr(wasm_i32x4_add(wasm_i32x4_shuffle(g0, g1, 0, 2, 4, 6),
                                      wasm_i32x4_shuffle(g0, g1, 1, 3, 5, 7)),
                       7);
    v128_t r32 =
        wasm_u32x4_shr(wasm_i32x4_add(wasm_i32x4_shuffle(r0, r1, 0, 2, 4, 6),
                                      wasm_i32x4_shuffle(r0, r1, 1, 3, 5, 7)),
                       7);
    v128_t bg = wasm_u8x16_narrow_i16x8(wasm_i16x8_narrow_i32x4(b32, b32),
                                        wasm_i16x8_narrow_i32x4(g32, g32));
    v128_t r = wasm_u8x16_narrow_i16x8(wasm_i16x8_narrow_i32x4(r32, r32),
                                       wasm_i16x8_const_splat(0));
    v128_t bgr = wasm_i8x16_shuffle(bg, r, 0, 8, 16, 31, 1, 9, 17, 31, 2, 10,
                                    18, 31, 3, 11, 19, 31);
    wasm_v128_store(dst_argb, wasm_v128_or(bgr, wasm_v128_and(s, mask_a)));
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBSEPIAROW_WASMSIMD

#ifdef HAS_ARGBSEPIAROW_RELAXEDSIMD
static const uint8_t kSepiaCoeffsB8[16] = {17, 68, 35, 0, 17, 68, 35, 0,
                                           17, 68, 35, 0, 17, 68, 35, 0};
static const uint8_t kSepiaCoeffsG8[16] = {22, 88, 45, 0, 22, 88, 45, 0,
                                           22, 88, 45, 0, 22, 88, 45, 0};
static const uint8_t kSepiaCoeffsR8[16] = {24, 98, 50, 0, 24, 98, 50, 0,
                                           24, 98, 50, 0, 24, 98, 50, 0};

void ARGBSepiaRow_RELAXEDSIMD(uint8_t* dst_argb, int width) {
  v128_t cb = LoadConst(kSepiaCoeffsB8);
  v128_t cg = LoadConst(kSepiaCoeffsG8);
  v128_t cr = LoadConst(kSepiaCoeffsR8);
  // All weights are in [0, 98] <= 127. Biasing pixels by -128 makes the
  // first operand signed and the second i7x16, which is deterministic on
  // both x86 and Arm.
  v128_t x80 = SplatConst8(-128);
  v128_t bias_b = SplatConst32(128 * (17 + 68 + 35));
  v128_t bias_g = SplatConst32(128 * (22 + 88 + 45));
  v128_t bias_r = SplatConst32(128 * (24 + 98 + 50));
  v128_t mask_a = SplatConst32((int)0xff000000);
  do {
    v128_t s = wasm_v128_load(dst_argb);
    v128_t ss = wasm_v128_xor(s, x80);
    v128_t b = wasm_u32x4_shr(
        wasm_i32x4_relaxed_dot_i8x16_i7x16_add(ss, cb, bias_b), 7);
    v128_t g = wasm_u32x4_shr(
        wasm_i32x4_relaxed_dot_i8x16_i7x16_add(ss, cg, bias_g), 7);
    v128_t r = wasm_u32x4_shr(
        wasm_i32x4_relaxed_dot_i8x16_i7x16_add(ss, cr, bias_r), 7);
    v128_t bg = wasm_u8x16_narrow_i16x8(wasm_i16x8_narrow_i32x4(b, b),
                                        wasm_i16x8_narrow_i32x4(g, g));
    v128_t r8 = wasm_u8x16_narrow_i16x8(wasm_i16x8_narrow_i32x4(r, r),
                                        wasm_i16x8_const_splat(0));
    v128_t bgr = wasm_i8x16_shuffle(bg, r8, 0, 8, 16, 31, 1, 9, 17, 31, 2, 10,
                                    18, 31, 3, 11, 19, 31);
    wasm_v128_store(dst_argb, wasm_v128_or(bgr, wasm_v128_and(s, mask_a)));
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBSEPIAROW_RELAXEDSIMD
#endif  // HAS_ARGBSEPIAROW_WASMSIMD || HAS_ARGBSEPIAROW_RELAXEDSIMD

#ifdef HAS_ARGBCOLORMATRIXROW_WASMSIMD
void ARGBColorMatrixRow_WASMSIMD(const uint8_t* src_argb,
                                 uint8_t* dst_argb,
                                 const int8_t* matrix_argb,
                                 int width) {
  v128_t m = wasm_v128_load(matrix_argb);
  v128_t m_lo = wasm_i16x8_extend_low_i8x16(m);
  v128_t m_hi = wasm_i16x8_extend_high_i8x16(m);
  v128_t mb = wasm_i64x2_shuffle(m_lo, m_lo, 0, 0);
  v128_t mg = wasm_i64x2_shuffle(m_lo, m_lo, 1, 1);
  v128_t mr = wasm_i64x2_shuffle(m_hi, m_hi, 0, 0);
  v128_t ma = wasm_i64x2_shuffle(m_hi, m_hi, 1, 1);
  do {
    asm("" : "+r"(src_argb), "+r"(dst_argb));
    v128_t s = wasm_v128_load(src_argb);
    v128_t s_lo = wasm_u16x8_extend_low_u8x16(s);
    v128_t s_hi = wasm_u16x8_extend_high_u8x16(s);
    v128_t b0 = wasm_i32x4_dot_i16x8(s_lo, mb);
    v128_t b1 = wasm_i32x4_dot_i16x8(s_hi, mb);
    v128_t g0 = wasm_i32x4_dot_i16x8(s_lo, mg);
    v128_t g1 = wasm_i32x4_dot_i16x8(s_hi, mg);
    v128_t r0 = wasm_i32x4_dot_i16x8(s_lo, mr);
    v128_t r1 = wasm_i32x4_dot_i16x8(s_hi, mr);
    v128_t a0 = wasm_i32x4_dot_i16x8(s_lo, ma);
    v128_t a1 = wasm_i32x4_dot_i16x8(s_hi, ma);
    v128_t b32 =
        wasm_i32x4_shr(wasm_i32x4_add(wasm_i32x4_shuffle(b0, b1, 0, 2, 4, 6),
                                      wasm_i32x4_shuffle(b0, b1, 1, 3, 5, 7)),
                       6);
    v128_t g32 =
        wasm_i32x4_shr(wasm_i32x4_add(wasm_i32x4_shuffle(g0, g1, 0, 2, 4, 6),
                                      wasm_i32x4_shuffle(g0, g1, 1, 3, 5, 7)),
                       6);
    v128_t r32 =
        wasm_i32x4_shr(wasm_i32x4_add(wasm_i32x4_shuffle(r0, r1, 0, 2, 4, 6),
                                      wasm_i32x4_shuffle(r0, r1, 1, 3, 5, 7)),
                       6);
    v128_t a32 =
        wasm_i32x4_shr(wasm_i32x4_add(wasm_i32x4_shuffle(a0, a1, 0, 2, 4, 6),
                                      wasm_i32x4_shuffle(a0, a1, 1, 3, 5, 7)),
                       6);
    v128_t b = wasm_i16x8_narrow_i32x4(b32, b32);
    v128_t g = wasm_i16x8_narrow_i32x4(g32, g32);
    v128_t r = wasm_i16x8_narrow_i32x4(r32, r32);
    v128_t a = wasm_i16x8_narrow_i32x4(a32, a32);
    v128_t bg = wasm_u8x16_narrow_i16x8(b, g);
    v128_t ra = wasm_u8x16_narrow_i16x8(r, a);
    wasm_v128_store(
        dst_argb, wasm_i8x16_shuffle(bg, ra, 0, 8, 16, 24, 1, 9, 17, 25, 2, 10,
                                     18, 26, 3, 11, 19, 27));
    src_argb += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBCOLORMATRIXROW_WASMSIMD

#ifdef HAS_ARGBQUANTIZEROW_WASMSIMD
void ARGBQuantizeRow_WASMSIMD(uint8_t* dst_argb,
                              int scale,
                              int interval_size,
                              int interval_offset,
                              int width) {
  v128_t vscale = wasm_i16x8_splat(scale);
  v128_t vsize = wasm_i16x8_splat(interval_size);
  v128_t voffset = wasm_i8x16_splat(interval_offset);
  v128_t mask_a = SplatConst32((int)0xff000000);
  do {
    v128_t s = wasm_v128_load(dst_argb);
    v128_t s_lo = wasm_u16x8_extend_low_u8x16(s);
    v128_t s_hi = wasm_u16x8_extend_high_u8x16(s);
    v128_t q_lo = wasm_i16x8_mul(
        wasm_i16x8_shuffle(wasm_u32x4_extmul_low_u16x8(s_lo, vscale),
                           wasm_u32x4_extmul_high_u16x8(s_lo, vscale), 1, 3, 5,
                           7, 9, 11, 13, 15),
        vsize);
    v128_t q_hi = wasm_i16x8_mul(
        wasm_i16x8_shuffle(wasm_u32x4_extmul_low_u16x8(s_hi, vscale),
                           wasm_u32x4_extmul_high_u16x8(s_hi, vscale), 1, 3, 5,
                           7, 9, 11, 13, 15),
        vsize);
    v128_t q = wasm_i8x16_add(wasm_u8x16_narrow_i16x8(q_lo, q_hi), voffset);
    wasm_v128_store(dst_argb, wasm_v128_bitselect(s, q, mask_a));
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBQUANTIZEROW_WASMSIMD

#ifdef HAS_ARGBSHADEROW_WASMSIMD
void ARGBShadeRow_WASMSIMD(const uint8_t* src_argb,
                           uint8_t* dst_argb,
                           int width,
                           uint32_t value) {
  v128_t v = wasm_i32x4_splat((int32_t)value);
  v128_t c128 = SplatConst16(128);
  do {
    asm("" : "+r"(src_argb), "+r"(dst_argb));
    v128_t s = wasm_v128_load(src_argb);
    v128_t lo = wasm_i16x8_add(wasm_u16x8_extmul_low_u8x16(s, v), c128);
    v128_t hi = wasm_i16x8_add(wasm_u16x8_extmul_high_u8x16(s, v), c128);
    lo = wasm_u16x8_shr(wasm_i16x8_add(lo, wasm_u16x8_shr(lo, 8)), 8);
    hi = wasm_u16x8_shr(wasm_i16x8_add(hi, wasm_u16x8_shr(hi, 8)), 8);
    wasm_v128_store(dst_argb, wasm_u8x16_narrow_i16x8(lo, hi));
    src_argb += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBSHADEROW_WASMSIMD

#if defined(HAS_SOBELROW_WASMSIMD) || defined(HAS_SOBELROW_RELAXEDSIMD) || \
    defined(HAS_SOBELTOPLANEROW_WASMSIMD) ||                               \
    defined(HAS_SOBELXROW_WASMSIMD) || defined(HAS_SOBELYROW_WASMSIMD) ||  \
    defined(HAS_SOBELXYROW_WASMSIMD)
void SobelXRow_WASMSIMD(const uint8_t* src_y0,
                        const uint8_t* src_y1,
                        const uint8_t* src_y2,
                        uint8_t* dst_sobelx,
                        int width) {
  do {
    v128_t a0 = wasm_v128_load(src_y0);
    v128_t a1 = wasm_v128_load(src_y1);
    v128_t a2 = wasm_v128_load(src_y2);
    v128_t b0 = wasm_v128_load(src_y0 + 2);
    v128_t b1 = wasm_v128_load(src_y1 + 2);
    v128_t b2 = wasm_v128_load(src_y2 + 2);
    v128_t a_lo =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_low_u8x16(a0),
                                      wasm_u16x8_extend_low_u8x16(a2)),
                       wasm_i16x8_shl(wasm_u16x8_extend_low_u8x16(a1), 1));
    v128_t a_hi =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_high_u8x16(a0),
                                      wasm_u16x8_extend_high_u8x16(a2)),
                       wasm_i16x8_shl(wasm_u16x8_extend_high_u8x16(a1), 1));
    v128_t b_lo =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_low_u8x16(b0),
                                      wasm_u16x8_extend_low_u8x16(b2)),
                       wasm_i16x8_shl(wasm_u16x8_extend_low_u8x16(b1), 1));
    v128_t b_hi =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_high_u8x16(b0),
                                      wasm_u16x8_extend_high_u8x16(b2)),
                       wasm_i16x8_shl(wasm_u16x8_extend_high_u8x16(b1), 1));
    v128_t diff_lo = wasm_i16x8_abs(wasm_i16x8_sub(a_lo, b_lo));
    v128_t diff_hi = wasm_i16x8_abs(wasm_i16x8_sub(a_hi, b_hi));
    wasm_v128_store(dst_sobelx, wasm_u8x16_narrow_i16x8(diff_lo, diff_hi));
    src_y0 += 16;
    src_y1 += 16;
    src_y2 += 16;
    dst_sobelx += 16;
    width -= 16;
  } while (width > 0);
}

void SobelYRow_WASMSIMD(const uint8_t* src_y0,
                        const uint8_t* src_y1,
                        uint8_t* dst_sobely,
                        int width) {
  do {
    v128_t a0 = wasm_v128_load(src_y0);
    v128_t a1 = wasm_v128_load(src_y0 + 1);
    v128_t a2 = wasm_v128_load(src_y0 + 2);
    v128_t b0 = wasm_v128_load(src_y1);
    v128_t b1 = wasm_v128_load(src_y1 + 1);
    v128_t b2 = wasm_v128_load(src_y1 + 2);
    v128_t a_lo =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_low_u8x16(a0),
                                      wasm_u16x8_extend_low_u8x16(a2)),
                       wasm_i16x8_shl(wasm_u16x8_extend_low_u8x16(a1), 1));
    v128_t a_hi =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_high_u8x16(a0),
                                      wasm_u16x8_extend_high_u8x16(a2)),
                       wasm_i16x8_shl(wasm_u16x8_extend_high_u8x16(a1), 1));
    v128_t b_lo =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_low_u8x16(b0),
                                      wasm_u16x8_extend_low_u8x16(b2)),
                       wasm_i16x8_shl(wasm_u16x8_extend_low_u8x16(b1), 1));
    v128_t b_hi =
        wasm_i16x8_add(wasm_i16x8_add(wasm_u16x8_extend_high_u8x16(b0),
                                      wasm_u16x8_extend_high_u8x16(b2)),
                       wasm_i16x8_shl(wasm_u16x8_extend_high_u8x16(b1), 1));
    v128_t diff_lo = wasm_i16x8_abs(wasm_i16x8_sub(a_lo, b_lo));
    v128_t diff_hi = wasm_i16x8_abs(wasm_i16x8_sub(a_hi, b_hi));
    wasm_v128_store(dst_sobely, wasm_u8x16_narrow_i16x8(diff_lo, diff_hi));
    src_y0 += 16;
    src_y1 += 16;
    dst_sobely += 16;
    width -= 16;
  } while (width > 0);
}

template <bool kRelaxed>
static inline void SobelRow_Wasm(const uint8_t* src_sobelx,
                                 const uint8_t* src_sobely,
                                 uint8_t* dst_argb,
                                 int width) {
  v128_t shuf0 = LoadConst(kShuffleJ400ToARGB);
  v128_t shuf1 = LoadConst(kShuffleJ400ToARGB + 16);
  v128_t shuf2 = LoadConst(kShuffleJ400ToARGB + 32);
  v128_t shuf3 = LoadConst(kShuffleJ400ToARGB + 48);
  v128_t alpha = SplatConst32((int)0xff000000);
  do {
    asm("" : "+r"(dst_argb));
    v128_t v = wasm_u8x16_add_sat(wasm_v128_load(src_sobelx),
                                  wasm_v128_load(src_sobely));
    wasm_v128_store(dst_argb,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(v, shuf0), alpha));
    wasm_v128_store(dst_argb + 16,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(v, shuf1), alpha));
    wasm_v128_store(dst_argb + 32,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(v, shuf2), alpha));
    wasm_v128_store(dst_argb + 48,
                    wasm_v128_or(WasmSwizzle<kRelaxed>(v, shuf3), alpha));
    src_sobelx += 16;
    src_sobely += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}

#ifdef HAS_SOBELROW_WASMSIMD
void SobelRow_WASMSIMD(const uint8_t* src_sobelx,
                       const uint8_t* src_sobely,
                       uint8_t* dst_argb,
                       int width) {
  SobelRow_Wasm<false>(src_sobelx, src_sobely, dst_argb, width);
}
#endif  // HAS_SOBELROW_WASMSIMD

#ifdef HAS_SOBELROW_RELAXEDSIMD
void SobelRow_RELAXEDSIMD(const uint8_t* src_sobelx,
                          const uint8_t* src_sobely,
                          uint8_t* dst_argb,
                          int width) {
  SobelRow_Wasm<true>(src_sobelx, src_sobely, dst_argb, width);
}
#endif  // HAS_SOBELROW_RELAXEDSIMD

void SobelToPlaneRow_WASMSIMD(const uint8_t* src_sobelx,
                              const uint8_t* src_sobely,
                              uint8_t* dst_y,
                              int width) {
  do {
    wasm_v128_store(dst_y, wasm_u8x16_add_sat(wasm_v128_load(src_sobelx),
                                              wasm_v128_load(src_sobely)));
    src_sobelx += 16;
    src_sobely += 16;
    dst_y += 16;
    width -= 16;
  } while (width > 0);
}

void SobelXYRow_WASMSIMD(const uint8_t* src_sobelx,
                         const uint8_t* src_sobely,
                         uint8_t* dst_argb,
                         int width) {
  v128_t a = SplatConst8(0xff);
  do {
    asm("" : "+r"(dst_argb));
    v128_t r = wasm_v128_load(src_sobelx);
    v128_t b = wasm_v128_load(src_sobely);
    v128_t g = wasm_u8x16_add_sat(r, b);
    v128_t bg0 = wasm_i8x16_shuffle(b, g, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                    21, 6, 22, 7, 23);
    v128_t bg1 = wasm_i8x16_shuffle(b, g, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28,
                                    13, 29, 14, 30, 15, 31);
    v128_t ra0 = wasm_i8x16_shuffle(r, a, 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5,
                                    21, 6, 22, 7, 23);
    v128_t ra1 = wasm_i8x16_shuffle(r, a, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28,
                                    13, 29, 14, 30, 15, 31);
    wasm_v128_store(dst_argb,
                    wasm_i16x8_shuffle(bg0, ra0, 0, 8, 1, 9, 2, 10, 3, 11));
    wasm_v128_store(dst_argb + 16,
                    wasm_i16x8_shuffle(bg0, ra0, 4, 12, 5, 13, 6, 14, 7, 15));
    wasm_v128_store(dst_argb + 32,
                    wasm_i16x8_shuffle(bg1, ra1, 0, 8, 1, 9, 2, 10, 3, 11));
    wasm_v128_store(dst_argb + 48,
                    wasm_i16x8_shuffle(bg1, ra1, 4, 12, 5, 13, 6, 14, 7, 15));
    src_sobelx += 16;
    src_sobely += 16;
    dst_argb += 64;
    width -= 16;
  } while (width > 0);
}
#endif

#ifdef HAS_ARGBPOLYNOMIALROW_WASMSIMD
void ARGBPolynomialRow_WASMSIMD(const uint8_t* src_argb,
                                uint8_t* dst_argb,
                                const float* poly,
                                int width) {
  v128_t c0 = wasm_v128_load(poly);
  v128_t c1 = wasm_v128_load(poly + 4);
  v128_t c2 = wasm_v128_load(poly + 8);
  v128_t c3 = wasm_v128_load(poly + 12);
  do {
    asm("" : "+r"(src_argb), "+r"(dst_argb));
    v128_t s = wasm_v128_load(src_argb);
    v128_t s16_0 = wasm_u16x8_extend_low_u8x16(s);
    v128_t s16_1 = wasm_u16x8_extend_high_u8x16(s);
    v128_t f0 = wasm_f32x4_convert_u32x4(wasm_u32x4_extend_low_u16x8(s16_0));
    v128_t f1 = wasm_f32x4_convert_u32x4(wasm_u32x4_extend_high_u16x8(s16_0));
    v128_t f2 = wasm_f32x4_convert_u32x4(wasm_u32x4_extend_low_u16x8(s16_1));
    v128_t f3 = wasm_f32x4_convert_u32x4(wasm_u32x4_extend_high_u16x8(s16_1));
    v128_t r0 = wasm_f32x4_add(
        c0,
        wasm_f32x4_mul(
            f0, wasm_f32x4_add(
                    c1, wasm_f32x4_mul(
                            f0, wasm_f32x4_add(c2, wasm_f32x4_mul(f0, c3))))));
    v128_t r1 = wasm_f32x4_add(
        c0,
        wasm_f32x4_mul(
            f1, wasm_f32x4_add(
                    c1, wasm_f32x4_mul(
                            f1, wasm_f32x4_add(c2, wasm_f32x4_mul(f1, c3))))));
    v128_t r2 = wasm_f32x4_add(
        c0,
        wasm_f32x4_mul(
            f2, wasm_f32x4_add(
                    c1, wasm_f32x4_mul(
                            f2, wasm_f32x4_add(c2, wasm_f32x4_mul(f2, c3))))));
    v128_t r3 = wasm_f32x4_add(
        c0,
        wasm_f32x4_mul(
            f3, wasm_f32x4_add(
                    c1, wasm_f32x4_mul(
                            f3, wasm_f32x4_add(c2, wasm_f32x4_mul(f3, c3))))));
    v128_t i0 = wasm_i16x8_narrow_i32x4(wasm_i32x4_trunc_sat_f32x4(r0),
                                        wasm_i32x4_trunc_sat_f32x4(r1));
    v128_t i1 = wasm_i16x8_narrow_i32x4(wasm_i32x4_trunc_sat_f32x4(r2),
                                        wasm_i32x4_trunc_sat_f32x4(r3));
    wasm_v128_store(dst_argb, wasm_u8x16_narrow_i16x8(i0, i1));
    src_argb += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBPOLYNOMIALROW_WASMSIMD

#ifdef HAS_ARGBLUMACOLORTABLEROW_WASMSIMD
void ARGBLumaColorTableRow_WASMSIMD(const uint8_t* src_argb,
                                    uint8_t* dst_argb,
                                    int width,
                                    const uint8_t* luma,
                                    uint32_t lumacoeff) {
  v128_t coeffs = wasm_u16x8_extend_low_u8x16(wasm_i32x4_splat(lumacoeff));
  do {
    v128_t s = wasm_v128_load(src_argb);
    v128_t y01 = wasm_i32x4_dot_i16x8(wasm_u16x8_extend_low_u8x16(s), coeffs);
    v128_t y23 = wasm_i32x4_dot_i16x8(wasm_u16x8_extend_high_u8x16(s), coeffs);
    v128_t y =
        wasm_v128_and(wasm_i32x4_add(wasm_i32x4_shuffle(y01, y23, 0, 2, 4, 6),
                                     wasm_i32x4_shuffle(y01, y23, 1, 3, 5, 7)),
                      SplatConst32(0x7f00));
    uint32_t y_lane0 = (uint32_t)wasm_i32x4_extract_lane(y, 0);
    uint32_t y_lane1 = (uint32_t)wasm_i32x4_extract_lane(y, 1);
    uint32_t y_lane2 = (uint32_t)wasm_i32x4_extract_lane(y, 2);
    uint32_t y_lane3 = (uint32_t)wasm_i32x4_extract_lane(y, 3);
    const uint8_t* l0 = luma + y_lane0;
    const uint8_t* l1 = luma + y_lane1;
    const uint8_t* l2 = luma + y_lane2;
    const uint8_t* l3 = luma + y_lane3;
    dst_argb[0] = l0[src_argb[0]];
    dst_argb[1] = l0[src_argb[1]];
    dst_argb[2] = l0[src_argb[2]];
    dst_argb[3] = src_argb[3];
    dst_argb[4] = l1[src_argb[4]];
    dst_argb[5] = l1[src_argb[5]];
    dst_argb[6] = l1[src_argb[6]];
    dst_argb[7] = src_argb[7];
    dst_argb[8] = l2[src_argb[8]];
    dst_argb[9] = l2[src_argb[9]];
    dst_argb[10] = l2[src_argb[10]];
    dst_argb[11] = src_argb[11];
    dst_argb[12] = l3[src_argb[12]];
    dst_argb[13] = l3[src_argb[13]];
    dst_argb[14] = l3[src_argb[14]];
    dst_argb[15] = src_argb[15];
    src_argb += 16;
    dst_argb += 16;
    width -= 4;
  } while (width > 0);
}
#endif  // HAS_ARGBLUMACOLORTABLEROW_WASMSIMD

#ifdef HAS_HALFFLOATROW_WASMSIMD
void HalfFloatRow_WASMSIMD(const uint16_t* src,
                           uint16_t* dst,
                           float scale,
                           int width) {
  v128_t vmult = wasm_f32x4_splat(1.9259299444e-34f * scale);
  do {
    asm("" : "+r"(src), "+r"(dst));
    v128_t s0 = wasm_v128_load(src);
    v128_t s1 = wasm_v128_load(src + 8);
    v128_t f0 = wasm_u32x4_shr(
        wasm_f32x4_mul(
            wasm_f32x4_convert_u32x4(wasm_u32x4_extend_low_u16x8(s0)), vmult),
        13);
    v128_t f1 = wasm_u32x4_shr(
        wasm_f32x4_mul(
            wasm_f32x4_convert_u32x4(wasm_u32x4_extend_high_u16x8(s0)), vmult),
        13);
    v128_t f2 = wasm_u32x4_shr(
        wasm_f32x4_mul(
            wasm_f32x4_convert_u32x4(wasm_u32x4_extend_low_u16x8(s1)), vmult),
        13);
    v128_t f3 = wasm_u32x4_shr(
        wasm_f32x4_mul(
            wasm_f32x4_convert_u32x4(wasm_u32x4_extend_high_u16x8(s1)), vmult),
        13);
    wasm_v128_store(dst, wasm_i16x8_shuffle(f0, f1, 0, 2, 4, 6, 8, 10, 12, 14));
    wasm_v128_store(dst + 8,
                    wasm_i16x8_shuffle(f2, f3, 0, 2, 4, 6, 8, 10, 12, 14));
    src += 16;
    dst += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_HALFFLOATROW_WASMSIMD

#ifdef HAS_BYTETOFLOATROW_WASMSIMD
void ByteToFloatRow_WASMSIMD(const uint8_t* src,
                             float* dst,
                             float scale,
                             int width) {
  v128_t vscale = wasm_f32x4_splat(scale);
  do {
    asm("" : "+r"(src), "+r"(dst));
    v128_t s = wasm_v128_load(src);
    v128_t s0 = wasm_u16x8_extend_low_u8x16(s);
    v128_t s1 = wasm_u16x8_extend_high_u8x16(s);
    wasm_v128_store(
        dst,
        wasm_f32x4_mul(
            wasm_f32x4_convert_u32x4(wasm_u32x4_extend_low_u16x8(s0)), vscale));
    wasm_v128_store(
        dst + 4, wasm_f32x4_mul(
                     wasm_f32x4_convert_u32x4(wasm_u32x4_extend_high_u16x8(s0)),
                     vscale));
    wasm_v128_store(
        dst + 8,
        wasm_f32x4_mul(
            wasm_f32x4_convert_u32x4(wasm_u32x4_extend_low_u16x8(s1)), vscale));
    wasm_v128_store(dst + 12,
                    wasm_f32x4_mul(wasm_f32x4_convert_u32x4(
                                       wasm_u32x4_extend_high_u16x8(s1)),
                                   vscale));
    src += 16;
    dst += 16;
    width -= 16;
  } while (width > 0);
}
#endif  // HAS_BYTETOFLOATROW_WASMSIMD

#ifdef __cplusplus
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_WASMSIMD) && defined(__wasm_simd128__)

/*
 *  Copyright 2024 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef INCLUDE_LIBYUV_CPU_SUPPORT_H_
#define INCLUDE_LIBYUV_CPU_SUPPORT_H_

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if defined(__pnacl__) || defined(__CLR_VER) ||            \
    (defined(__native_client__) && defined(__x86_64__)) || \
    (defined(__i386__) && !defined(__SSE__) && !defined(__clang__))
#define LIBYUV_DISABLE_X86
#endif

#if defined(__native_client__)
#define LIBYUV_DISABLE_NEON
#endif

// MemorySanitizer does not support assembly code yet. http://crbug.com/344505
#if defined(__has_feature)
#if __has_feature(memory_sanitizer)
#if !defined(LIBYUV_MSAN)
#define LIBYUV_MSAN 1
#endif
#if !defined(LIBYUV_DISABLE_NEON)
#define LIBYUV_DISABLE_NEON
#endif
#if !defined(LIBYUV_DISABLE_SME)
#define LIBYUV_DISABLE_SME
#endif
#if !defined(LIBYUV_DISABLE_SVE)
#define LIBYUV_DISABLE_SVE
#endif
#if !defined(LIBYUV_DISABLE_RVV)
#define LIBYUV_DISABLE_RVV
#endif
#if !defined(LIBYUV_DISABLE_LSX)
#define LIBYUV_DISABLE_LSX
#endif
#if !defined(LIBYUV_DISABLE_LASX)
#define LIBYUV_DISABLE_LASX
#endif
#if !defined(LIBYUV_ENABLE_ROWWIN)
#define LIBYUV_ENABLE_ROWWIN
#endif
#endif  // __has_feature(memory_sanitizer)
#endif  // defined(__has_feature)

// Disable all platforms that use inline assembly when LIBYUV_DISABLE_ASM is
// defined at build time.
#if defined(LIBYUV_DISABLE_ASM)
#if !defined(LIBYUV_DISABLE_X86)
#define LIBYUV_DISABLE_X86
#endif
#if !defined(LIBYUV_DISABLE_NEON)
#define LIBYUV_DISABLE_NEON
#endif
#if !defined(LIBYUV_DISABLE_SVE)
#define LIBYUV_DISABLE_SVE
#endif
#if !defined(LIBYUV_DISABLE_SME)
#define LIBYUV_DISABLE_SME
#endif
#if !defined(LIBYUV_DISABLE_RVV)
#define LIBYUV_DISABLE_RVV
#endif
#if !defined(LIBYUV_DISABLE_LSX)
#define LIBYUV_DISABLE_LSX
#endif
#if !defined(LIBYUV_DISABLE_LASX)
#define LIBYUV_DISABLE_LASX
#endif
#endif  // defined(LIBYUV_DISABLE_ASM)

// clang >= 3.5.0 required for Arm64.
#if defined(__clang__) && defined(__aarch64__) && !defined(LIBYUV_DISABLE_NEON)
#if (__clang_major__ < 3) || (__clang_major__ == 3 && (__clang_minor__ < 5))
#define LIBYUV_DISABLE_NEON
#endif  // clang >= 3.5
#endif  // __clang__

// GCC >= 4.7.0 required for AVX2.
#if defined(__GNUC__) && !defined(__clang__) && \
    !defined(LIBYUV_ENABLE_ROWWIN) &&           \
    (defined(__x86_64__) || defined(__i386__))
#if (__GNUC__ > 4) || (__GNUC__ == 4 && (__GNUC_MINOR__ >= 7))
#define GCC_HAS_AVX2 1
#endif  // GNUC >= 4.7
#endif  // __GNUC__

// GCC >= 9.0.0 required for AVX512 (including VNNI and VBMI).
#if defined(__GNUC__) && !defined(__clang__) && \
    !defined(LIBYUV_ENABLE_ROWWIN) && defined(__x86_64__)
#if (__GNUC__ >= 9)
#define GCC_HAS_AVX512 1
#endif  // GNUC >= 9
#if (__GNUC__ >= 11)
#define GCC_HAS_ACEV1 1
#endif  // GNUC >= 11
#endif  // __GNUC__

// Function target attributes so GCC enables zmm16..zmm31 and k1..k7 clobbers
// inside AVX512 functions without emitting AVX512 in non-AVX512 functions.
#if defined(GCC_HAS_AVX512)
#define LIBYUV_TARGET_AVX512BW \
  __attribute__((target("avx512bw,avx512vl,avx512f,bmi2")))
#define LIBYUV_TARGET_AVX512VBMI \
  __attribute__((target("avx512vbmi,avx512bw,avx512vl,avx512f,bmi2")))
#define LIBYUV_TARGET_AVX512VNNI \
  __attribute__((target("avx512vnni,avx512bw,avx512vl,avx512f,bmi2")))
#define LIBYUV_TARGET_AVX512VNNIVBMI \
  __attribute__((                    \
      target("avx512vnni,avx512vbmi,avx512bw,avx512vl,avx512f,bmi2")))
#elif !defined(LIBYUV_ENABLE_ROWWIN) && \
    !(defined(_MSC_VER) && !defined(__clang__))
#define LIBYUV_TARGET_AVX512BW
#define LIBYUV_TARGET_AVX512VBMI
#define LIBYUV_TARGET_AVX512VNNI
#define LIBYUV_TARGET_AVX512VNNIVBMI
#endif

#if defined(GCC_HAS_ACEV1)
#define LIBYUV_TARGET_ACEV1 \
  __attribute__((target("amx-tile,avx512bw,avx512vl,avx512f,bmi2")))
#elif !defined(LIBYUV_ENABLE_ROWWIN) && \
    !(defined(_MSC_VER) && !defined(__clang__))
#define LIBYUV_TARGET_ACEV1
#endif

// clang >= 3.4.0 required for AVX2.
#if defined(__clang__) && !defined(LIBYUV_ENABLE_ROWWIN) && \
    (defined(__x86_64__) || defined(__i386__))
#if (__clang_major__ > 3) || (__clang_major__ == 3 && (__clang_minor__ >= 4))
#define CLANG_HAS_AVX2 1
#endif  // clang >= 3.4
#endif  // __clang__

// clang >= 7.0.0 required for AVX512.
#if defined(__clang__) && !defined(LIBYUV_ENABLE_ROWWIN) && \
    (defined(__x86_64__) || defined(__i386__))
// clang in xcode follows a different versioning scheme.
// TODO(fbarchard): fix xcode 9 ios b/789.
#if (__clang_major__ >= 7) && !defined(__APPLE__)
#define CLANG_HAS_AVX512 1
#endif  // clang >= 7
#endif  // __clang__

// clang >= 11.0.0 required for ACEV1.
#if defined(__clang__) && !defined(LIBYUV_ENABLE_ROWWIN) && defined(__x86_64__)
#if (__clang_major__ >= 11) && !defined(__APPLE__)
#define CLANG_HAS_ACEV1 1
#endif  // clang >= 11
#endif  // __clang__

// Visual C 2012 (or clang >= 3.4 with LIBYUV_ENABLE_ROWWIN) required for AVX2.
#if ((defined(_M_IX86) || defined(_M_X64)) && !defined(__clang__) && \
     defined(_MSC_VER) && _MSC_VER >= 1700) ||                       \
    (defined(LIBYUV_ENABLE_ROWWIN) && defined(__clang__) &&          \
     (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || \
      defined(_M_IX86)) &&                                           \
     ((__clang_major__ > 3) ||                                       \
      (__clang_major__ == 3 && (__clang_minor__ >= 4))))
#define VISUALC_HAS_AVX2 1
#endif  // VisualStudio >= 2012

// Visual C 2019 (or clang >= 7 with LIBYUV_ENABLE_ROWWIN) required for AVX512.
#if (defined(_M_X64) || defined(__x86_64__)) &&                        \
    ((!defined(__clang__) && defined(_MSC_VER) && _MSC_VER >= 1920) || \
     (defined(LIBYUV_ENABLE_ROWWIN) && defined(__clang__) &&           \
      (__clang_major__ >= 7) && !defined(__APPLE__)))
#define VISUALC_HAS_AVX512 1
#endif  // VisualStudio >= 2019

#if defined(LIBYUV_ENABLE_SME) && !defined(LIBYUV_DISABLE_ASM)
#undef LIBYUV_DISABLE_SME
#endif

// clang >= 11.0.0 or gcc >= 10.0.0 required for SVE2.
#if defined(__aarch64__) && !defined(LIBYUV_DISABLE_SVE)
#if (defined(__clang__) && (__clang_major__ < 11)) || \
    (!defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 10))
#define LIBYUV_DISABLE_SVE
#endif
#endif

// Clang 18+ on Linux/Android, Apple Clang 17+ on macOS, or GCC 15+ on Linux
// required for SME (__arm_tpidr2_save from compiler-rt or libgcc).
#if !defined(LIBYUV_DISABLE_SME) && defined(__aarch64__)
#if defined(__clang__) && ((defined(__linux__) && (__clang_major__ >= 18)) || \
                           (defined(__APPLE__) && (__clang_major__ >= 17)))
#define CLANG_HAS_SME 1
#elif defined(__linux__) && !defined(__clang__) && defined(__GNUC__) && \
    (__GNUC__ >= 15)
#define GCC_HAS_SME 1
#endif
#endif

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // INCLUDE_LIBYUV_CPU_SUPPORT_H_

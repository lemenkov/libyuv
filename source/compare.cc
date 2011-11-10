/*
 *  SumSquareErrorright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/planar_functions.h"

#include <string.h>

#include "libyuv/cpu_id.h"
#include "row.h"

namespace libyuv {

uint32 SumSquareError_SSE2(const uint8* src_a, const uint8* src_b, int count);
uint32 SumSquareError_NEON(const uint8* src_a, const uint8* src_b, int count);
uint32 SumSquareError_C(const uint8* src_a, const uint8* src_b, int count);

#if defined(__ARM_NEON__) && !defined(COVERAGE_ENABLED)
#define HAS_SUMSQUAREERROR_NEON
  return 0;
}

static uint32 SumSquareError_NEON(const uint8* src_a,
                                  const uint8* src_b, int count) {
  asm volatile (
    "1:\n"
    "vld1.u8    {q0}, [%0]!\n"  // load 16 bytes
    "vld1.u8    {q1}, [%1]!\n"  // load 16 bytes
// do stuff
    "subs       %3, %3, #16\n"  // 16 processed per loop
    "bhi        1b\n"
    : "+r"(src_a),
      "+r"(src_b)
      "+r"(count)           // Output registers
    :                       // Input registers
    : "memory", "cc", "q0", "q1" // Clobber List
  );
}

#elif (defined(WIN32) || defined(__x86_64__) || defined(__i386__)) \
    && !defined(COVERAGE_ENABLED) && !defined(TARGET_IPHONE_SIMULATOR)

#if defined(WIN32) && !defined(COVERAGE_ENABLED)
#define HAS_SUMSQUAREERROR_SSE2
__declspec(naked)
static uint32 SumSquareError_SSE2(const uint8* src_a,
                                  const uint8* src_b, int count) {
  __asm {
    mov        eax, [esp + 4]    // src_a
    mov        edx, [esp + 8]    // src_b
    mov        ecx, [esp + 12]   // count
    pxor       xmm0, xmm0
    pxor       xmm5, xmm5
    sub        edx, eax

  wloop:
    movdqa     xmm1, [eax]
    movdqa     xmm2, [eax + edx]
    lea        eax,  [eax + 16]
    movdqa     xmm3, xmm1
    psubusb    xmm1, xmm2
    psubusb    xmm2, xmm3
    por        xmm1, xmm2
    movdqa     xmm2, xmm1
    punpcklbw  xmm1, xmm5
    punpckhbw  xmm2, xmm5
    pmaddwd    xmm1, xmm1
    pmaddwd    xmm2, xmm2
    paddd      xmm0, xmm1
    paddd      xmm0, xmm2
    sub        ecx, 16
    ja         wloop

    pshufd     xmm1, xmm0, 0EEh
    paddd      xmm0, xmm1
    pshufd     xmm1, xmm0, 01h
    paddd      xmm0, xmm1
    movd       eax, xmm0

    ret
  }
}

#elif (defined(__x86_64__) || defined(__i386__)) && \
    !defined(COVERAGE_ENABLED) && !defined(TARGET_IPHONE_SIMULATOR)
#define HAS_SUMSQUAREERROR_SSE2
static uint32 SumSquareError_SSE2(const uint8* src_a,
                                  const uint8* src_b, int count) {
 asm volatile(
  "pcmpeqb    %%xmm5,%%xmm5\n"
  "psrlw      $0x8,%%xmm5\n"
"1:"
  "movdqa     (%0),%%xmm0\n"
  "movdqa     0x10(%0),%%xmm1\n"
  "lea        0x20(%0),%0\n"
  "movdqa     %%xmm0,%%xmm2\n"
  "movdqa     %%xmm1,%%xmm3\n"
  "pand       %%xmm5,%%xmm0\n"
  "pand       %%xmm5,%%xmm1\n"
  "packuswb   %%xmm1,%%xmm0\n"
  "movdqa     %%xmm0,(%1)\n"
  "lea        0x10(%1),%1\n"
  "psrlw      $0x8,%%xmm2\n"
  "psrlw      $0x8,%%xmm3\n"
  "packuswb   %%xmm3,%%xmm2\n"
  "movdqa     %%xmm2,(%2)\n"
  "lea        0x10(%2),%2\n"
  "sub        $0x10,%3\n"
  "ja         1b\n"
  : "+r"(src_uv),     // %0
    "+r"(dst_u),      // %1
    "+r"(dst_v),      // %2
    "+r"(pix)         // %3
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
);
}
#endif
#endif

static uint32 SumSquareError_C(const uint8* src_a,
                               const uint8* src_b, int count) {
  uint32 udiff = 0u;
  for (int x = 0; x < count; ++x) {
    int diff = src_a[0] - src_b[0];
    udiff += static_cast<uint32>(diff * diff);
    src_a += 1;
    src_b += 1;
  }
  return udiff;
}

uint64 SumSquareError(const uint8* src_a, const uint8* src_b, uint64 count) {
  uint32 (*SumSquareError)(const uint8* src_a,
                           const uint8* src_b, int count);
#if defined(HAS_SUMSQUAREERROR_NEON)
  if (TestCpuFlag(kCpuHasNEON)) {
    SumSquareError = SumSquareError_NEON;
  } else
#elif defined(HAS_SUMSQUAREERROR_SSE2)
  if (TestCpuFlag(kCpuHasSSE2) &&
      IS_ALIGNED(src_a, 16) && IS_ALIGNED(src_b, 16)) {
    SumSquareError = SumSquareError_SSE2;
  } else
#endif
  {
    SumSquareError = SumSquareError_C;
  }

  const int kBlockSize = 4096;
  uint64 diff = 0u;
  while (count >= kBlockSize) {
    diff += SumSquareError(src_a, src_b, kBlockSize);
    src_a += kBlockSize;
    src_b += kBlockSize;
    count -= kBlockSize;
  }
  if (count > 0) {
    int short_count = static_cast<int>(count);
    if (short_count % 16 == 0) {
      diff += static_cast<uint64>(SumSquareError(src_a, src_b, short_count));
    } else {
      diff += static_cast<uint64>(SumSquareError_C(src_a, src_b, short_count));
    }
  }

  return diff;
}

}  // namespace libyuv


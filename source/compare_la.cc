/*
 *  Copyright 2022 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/compare_row.h"
#include "libyuv/row.h"
#include "libyuv/video_common.h"

// Note: every register written by an inline asm block (vector, general
// purpose, mask/predicate, "cc" for flags and "memory" for stores) must be
// listed in its clobber list, even when the ABI treats it as caller-saved.
// With LTO/LTCG the compiler may keep values live in any register across the
// asm statement, and other OS ABIs (e.g. Windows) differ in which registers
// are callee-saved. List "memory" first, then "cc", then registers.

#if (!defined(LIBYUV_DISABLE_LSX) || !defined(LIBYUV_DISABLE_LASX)) && \
    defined(__loongarch_lp64)

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if defined(HAS_HASHDJB2_LSX) || defined(HAS_HASHDJB2_LASX)
static const ulvec32 kDjb2Multiplicands[] = {
    {0x0c3525e1, 0xa3476dc1, 0x3b4039a1, 0x4f5f0981, 0x30f35d61, 0x855cb541,
     0x040a9121, 0x747c7101},
    {0xec41d4e1, 0x4cfa3cc1, 0x025528a1, 0x00121881, 0x00008c61, 0x00000441,
     0x00000021, 0x00000001},
};
#endif  // defined(HAS_HASHDJB2_LSX) || defined(HAS_HASHDJB2_LASX)

#ifdef HAS_HAMMINGDISTANCE_LSX
uint32_t HammingDistance_LSX(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count) {
  if (count <= 0) {
    return 0;
  }
  uint32_t diff;
  asm volatile(
      "vxor.v           $vr4, $vr4, $vr4               \n"

      "1:                                              \n"
      "vld              $vr0, %[sa], 0                 \n"
      "vld              $vr1, %[sb], 0                 \n"
      "vxor.v           $vr0, $vr0, $vr1               \n"
      "vpcnt.b          $vr0, $vr0                     \n"
      "vhaddw.hu.bu     $vr0, $vr0, $vr0               \n"
      "vhaddw.wu.hu     $vr0, $vr0, $vr0               \n"
      "vadd.w           $vr4, $vr4, $vr0               \n"
      "addi.d           %[sa], %[sa], 16               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bgtz             %[cnt], 1b                     \n"

      "vhaddw.du.wu     $vr4, $vr4, $vr4               \n"
      "vhaddw.qu.du     $vr4, $vr4, $vr4               \n"
      "vpickve2gr.wu    %[diff], $vr4, 0               \n"
      : [sa] "+r"(src_a),   // %[sa]
        [sb] "+r"(src_b),   // %[sb]
        [cnt] "+r"(count),  // %[cnt]
        [diff] "=r"(diff)   // %[diff]
      :
      : "memory", "$vr0", "$vr1", "$vr4");
  return diff;
}
#endif  // HAS_HAMMINGDISTANCE_LSX

#ifdef HAS_SUMSQUAREERROR_LSX
uint32_t SumSquareError_LSX(const uint8_t* src_a,
                            const uint8_t* src_b,
                            int count) {
  if (count <= 0) {
    return 0;
  }
  uint32_t sse;
  asm volatile(
      "vxor.v           $vr4, $vr4, $vr4               \n"

      "1:                                              \n"
      "vld              $vr0, %[sa], 0                 \n"
      "vld              $vr1, %[sb], 0                 \n"
      "vabsd.bu         $vr0, $vr0, $vr1               \n"
      "vmulwev.h.bu     $vr2, $vr0, $vr0               \n"
      "vmulwod.h.bu     $vr3, $vr0, $vr0               \n"
      "vhaddw.wu.hu     $vr2, $vr2, $vr2               \n"
      "vhaddw.wu.hu     $vr3, $vr3, $vr3               \n"
      "vadd.w           $vr4, $vr4, $vr2               \n"
      "vadd.w           $vr4, $vr4, $vr3               \n"
      "addi.d           %[sa], %[sa], 16               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bgtz             %[cnt], 1b                     \n"

      "vhaddw.du.wu     $vr4, $vr4, $vr4               \n"
      "vhaddw.qu.du     $vr4, $vr4, $vr4               \n"
      "vpickve2gr.wu    %[sse], $vr4, 0                \n"
      : [sa] "+r"(src_a),   // %[sa]
        [sb] "+r"(src_b),   // %[sb]
        [cnt] "+r"(count),  // %[cnt]
        [sse] "=r"(sse)     // %[sse]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
  return sse;
}
#endif  // HAS_SUMSQUAREERROR_LSX

#ifdef HAS_HASHDJB2_LSX
uint32_t HashDjb2_LSX(const uint8_t* src, int count, uint32_t seed) {
  if (count <= 0) {
    return seed;
  }
  uint32_t hash = seed;
  uint32_t tmp;
  uint32_t c16 = 0x92d9e201u;
  asm volatile(
      "vld              $vr4, %[muls], 0               \n"
      "vld              $vr5, %[muls], 16              \n"
      "vld              $vr6, %[muls], 32              \n"
      "vld              $vr7, %[muls], 48              \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vsllwil.hu.bu    $vr1, $vr0, 0                  \n"
      "vexth.hu.bu      $vr2, $vr0                     \n"
      "vsllwil.wu.hu    $vr0, $vr1, 0                  \n"
      "vexth.wu.hu      $vr1, $vr1                     \n"
      "vsllwil.wu.hu    $vr3, $vr2, 0                  \n"
      "vexth.wu.hu      $vr2, $vr2                     \n"
      "vmul.w           $vr0, $vr0, $vr4               \n"
      "vmadd.w          $vr0, $vr1, $vr5               \n"
      "vmadd.w          $vr0, $vr3, $vr6               \n"
      "vmadd.w          $vr0, $vr2, $vr7               \n"
      "vhaddw.du.wu     $vr0, $vr0, $vr0               \n"
      "vhaddw.qu.du     $vr0, $vr0, $vr0               \n"
      "vpickve2gr.wu    %[tmp], $vr0, 0                \n"
      "mul.w            %[hash], %[hash], %[c16]       \n"
      "add.w            %[hash], %[hash], %[tmp]       \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bgtz             %[cnt], 1b                     \n"
      : [src] "+r"(src),                 // %[src]
        [cnt] "+r"(count),               // %[cnt]
        [hash] "+r"(hash),               // %[hash]
        [tmp] "=&r"(tmp)                 // %[tmp]
      : [muls] "r"(kDjb2Multiplicands),  // %[muls]
        [c16] "r"(c16)                   // %[c16]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
  return hash;
}
#endif  // HAS_HASHDJB2_LSX

#ifdef HAS_SSIM4X4ROW_LSX
void Ssim4x4Row_LSX(const uint8_t* src_a,
                    int stride_a,
                    const uint8_t* src_b,
                    int stride_b,
                    uint32_t* dst,
                    int width) {
  if (width <= 0) {
    return;
  }
  const uint8_t* pa;
  const uint8_t* pb;
  int r;
  asm volatile(
      "1:                                              \n"
      "move             %[pa], %[sa]                   \n"
      "move             %[pb], %[sb]                   \n"
      "li.d             %[r], 4                        \n"
      "vxor.v           $vr8, $vr8, $vr8               \n"
      "vxor.v           $vr9, $vr9, $vr9               \n"
      "vxor.v           $vr10, $vr10, $vr10            \n"
      "vxor.v           $vr11, $vr11, $vr11            \n"

      "2:                                              \n"
      "vld              $vr0, %[pa], 0                 \n"
      "vld              $vr1, %[pb], 0                 \n"
      "add.d            %[pa], %[pa], %[sta]           \n"
      "add.d            %[pb], %[pb], %[stb]           \n"
      "vhaddw.hu.bu     $vr2, $vr0, $vr0               \n"
      "vhaddw.hu.bu     $vr3, $vr1, $vr1               \n"
      "vadd.h           $vr8, $vr8, $vr2               \n"
      "vadd.h           $vr9, $vr9, $vr3               \n"
      "vmulwev.h.bu     $vr2, $vr0, $vr1               \n"
      "vmulwod.h.bu     $vr3, $vr0, $vr1               \n"
      "vhaddw.wu.hu     $vr2, $vr2, $vr2               \n"
      "vhaddw.wu.hu     $vr3, $vr3, $vr3               \n"
      "vadd.w           $vr11, $vr11, $vr2             \n"
      "vadd.w           $vr11, $vr11, $vr3             \n"
      "vmulwev.h.bu     $vr2, $vr0, $vr0               \n"
      "vmulwod.h.bu     $vr3, $vr0, $vr0               \n"
      "vmulwev.h.bu     $vr4, $vr1, $vr1               \n"
      "vmulwod.h.bu     $vr5, $vr1, $vr1               \n"
      "vhaddw.wu.hu     $vr2, $vr2, $vr2               \n"
      "vhaddw.wu.hu     $vr3, $vr3, $vr3               \n"
      "vhaddw.wu.hu     $vr4, $vr4, $vr4               \n"
      "vhaddw.wu.hu     $vr5, $vr5, $vr5               \n"
      "vadd.w           $vr2, $vr2, $vr3               \n"
      "vadd.w           $vr4, $vr4, $vr5               \n"
      "vadd.w           $vr10, $vr10, $vr2             \n"
      "vadd.w           $vr10, $vr10, $vr4             \n"
      "addi.d           %[r], %[r], -1                 \n"
      "bnez             %[r], 2b                       \n"

      "vhaddw.wu.hu     $vr8, $vr8, $vr8               \n"
      "vhaddw.wu.hu     $vr9, $vr9, $vr9               \n"
      "vilvl.w          $vr0, $vr9, $vr8               \n"
      "vilvh.w          $vr1, $vr9, $vr8               \n"
      "vilvl.w          $vr2, $vr11, $vr10             \n"
      "vilvh.w          $vr3, $vr11, $vr10             \n"
      "vilvl.d          $vr4, $vr2, $vr0               \n"
      "vilvh.d          $vr5, $vr2, $vr0               \n"
      "vilvl.d          $vr6, $vr3, $vr1               \n"
      "vilvh.d          $vr7, $vr3, $vr1               \n"
      "vst              $vr4, %[dst], 0                \n"
      "vst              $vr5, %[dst], 16               \n"
      "vst              $vr6, %[dst], 32               \n"
      "vst              $vr7, %[dst], 48               \n"
      "addi.d           %[sa], %[sa], 16               \n"
      "addi.d           %[sb], %[sb], 16               \n"
      "addi.d           %[dst], %[dst], 64             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bgtz             %[cnt], 1b                     \n"
      : [sa] "+r"(src_a),                // %[sa]
        [sb] "+r"(src_b),                // %[sb]
        [dst] "+r"(dst),                 // %[dst]
        [cnt] "+r"(width),               // %[cnt]
        [pa] "=&r"(pa),                  // %[pa]
        [pb] "=&r"(pb),                  // %[pb]
        [r] "=&r"(r)                     // %[r]
      : [sta] "r"((ptrdiff_t)stride_a),  // %[sta]
        [stb] "r"((ptrdiff_t)stride_b)   // %[stb]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_SSIM4X4ROW_LSX

#ifdef HAS_SSIMWINDOWROW_LSX
float SsimWindowRow_LSX(const uint32_t* row0,
                        const uint32_t* row1,
                        int num_windows) {
  if (num_windows <= 0) {
    return 0.0f;
  }
  const uint32_t* row0_next = row0 + 4;
  const uint32_t* row1_next = row1 + 4;
  float total;
  uint32_t c1 = 26634;
  uint32_t c2 = 239708;
  uint32_t c0 = 0;
  asm volatile(
      "vreplgr2vr.w     $vr6, %[c0]                    \n"
      "vreplgr2vr.w     $vr4, %[c1]                    \n"
      "vreplgr2vr.w     $vr5, %[c2]                    \n"

      "1:                                              \n"
      "vld              $vr0, %[r0], 0                 \n"
      "vld              $vr1, %[r0n], 0                \n"
      "vld              $vr2, %[r1], 0                 \n"
      "vld              $vr3, %[r1n], 0                \n"
      "vadd.w           $vr0, $vr0, $vr1               \n"
      "vadd.w           $vr2, $vr2, $vr3               \n"
      "vadd.w           $vr0, $vr0, $vr2               \n"
      "vshuf4i.w        $vr1, $vr0, 0x00               \n"
      "vshuf4i.w        $vr2, $vr0, 0x55               \n"
      "vshuf4i.w        $vr3, $vr0, 0xaa               \n"
      "vshuf4i.w        $vr0, $vr0, 0xff               \n"
      "vmul.w           $vr7, $vr1, $vr2               \n"
      "vslli.w          $vr7, $vr7, 1                  \n"
      "vmul.w           $vr1, $vr1, $vr1               \n"
      "vmadd.w          $vr1, $vr2, $vr2               \n"
      "vslli.w          $vr0, $vr0, 7                  \n"
      "vsub.w           $vr0, $vr0, $vr7               \n"
      "vadd.w           $vr7, $vr7, $vr4               \n"
      "vadd.w           $vr0, $vr0, $vr5               \n"
      "vslli.w          $vr3, $vr3, 6                  \n"
      "vsub.w           $vr3, $vr3, $vr1               \n"
      "vadd.w           $vr1, $vr1, $vr4               \n"
      "vadd.w           $vr3, $vr3, $vr5               \n"
      "vffint.s.w       $vr7, $vr7                     \n"
      "vffint.s.w       $vr0, $vr0                     \n"
      "vffint.s.w       $vr1, $vr1                     \n"
      "vffint.s.w       $vr3, $vr3                     \n"
      "vfmul.s          $vr7, $vr7, $vr0               \n"
      "vfmul.s          $vr1, $vr1, $vr3               \n"
      "vfdiv.s          $vr7, $vr7, $vr1               \n"
      "vfadd.s          $vr6, $vr6, $vr7               \n"
      "addi.d           %[r0], %[r0], 16               \n"
      "addi.d           %[r0n], %[r0n], 16             \n"
      "addi.d           %[r1], %[r1], 16               \n"
      "addi.d           %[r1n], %[r1n], 16             \n"
      "addi.d           %[cnt], %[cnt], -1             \n"
      "bgtz             %[cnt], 1b                     \n"

      "vstelm.w         $vr6, %[tot], 0, 0             \n"
      : [r0] "+r"(row0),         // %[r0]
        [r0n] "+r"(row0_next),   // %[r0n]
        [r1] "+r"(row1),         // %[r1]
        [r1n] "+r"(row1_next),   // %[r1n]
        [cnt] "+r"(num_windows)  // %[cnt]
      : [c0] "r"(c0),            // %[c0]
        [c1] "r"(c1),            // %[c1]
        [c2] "r"(c2),            // %[c2]
        [tot] "r"(&total)        // %[tot]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
  return total;
}
#endif  // HAS_SSIMWINDOWROW_LSX

#ifdef HAS_ARGBDETECTROW_LSX
uint32_t ARGBDetectRow_LSX(const uint8_t* argb, int width) {
  uint32_t diff = 0;
  uint32_t rgb_mask = 0x00ffff00u;
  asm volatile(
      "vreplgr2vr.w     $vr2, %[rgb_mask]              \n"
      "addi.d           %[w], %[w], -4                 \n"
      "bltz             %[w], 2f                       \n"

      "1:                                              \n"
      "vld              $vr0, %[argb], 0               \n"
      "addi.d           %[argb], %[argb], 16           \n"
      "vor.v            $vr0, $vr0, $vr2               \n"
      "vseqi.b          $vr0, $vr0, -1                 \n"
      "vmskltz.b        $vr0, $vr0                     \n"
      "vpickve2gr.hu    %[diff], $vr0, 0               \n"
      "nor              %[diff], %[diff], $zero        \n"
      "bstrpick.w       %[diff], %[diff], 15, 0        \n"
      "bnez             %[diff], 4f                    \n"
      "addi.d           %[w], %[w], -4                 \n"
      "bgez             %[w], 1b                       \n"

      "2:                                              \n"
      "addi.d           %[w], %[w], 4                  \n"
      "blez             %[w], 4f                       \n"

      "3:                                              \n"
      "vldrepl.w        $vr0, %[argb], 0               \n"
      "addi.d           %[argb], %[argb], 4            \n"
      "vor.v            $vr0, $vr0, $vr2               \n"
      "vseqi.b          $vr0, $vr0, -1                 \n"
      "vmskltz.b        $vr0, $vr0                     \n"
      "vpickve2gr.hu    %[diff], $vr0, 0               \n"
      "nor              %[diff], %[diff], $zero        \n"
      "bstrpick.w       %[diff], %[diff], 15, 0        \n"
      "bnez             %[diff], 4f                    \n"
      "addi.d           %[w], %[w], -1                 \n"
      "bnez             %[w], 3b                       \n"

      "4:                                              \n"
      : [argb] "+r"(argb),        // %[argb]
        [w] "+r"(width),          // %[w]
        [diff] "+r"(diff)         // %[diff]
      : [rgb_mask] "r"(rgb_mask)  // %[rgb_mask]
      : "memory", "$vr0", "$vr2");
  if (diff) {
    return (__builtin_ctz(diff) & 3) == 0 ? FOURCC_BGRA : FOURCC_ARGB;
  }
  return 0;
}
#endif  // HAS_ARGBDETECTROW_LSX

#ifdef HAS_HAMMINGDISTANCE_LASX
uint32_t HammingDistance_LASX(const uint8_t* src_a,
                              const uint8_t* src_b,
                              int count) {
  if (count <= 0) {
    return 0;
  }
  uint32_t diff;
  asm volatile(
      "xvxor.v          $xr4, $xr4, $xr4               \n"

      "1:                                              \n"
      "xvld             $xr0, %[sa], 0                 \n"
      "xvld             $xr1, %[sb], 0                 \n"
      "xvxor.v          $xr0, $xr0, $xr1               \n"
      "xvpcnt.b         $xr0, $xr0                     \n"
      "xvhaddw.hu.bu    $xr0, $xr0, $xr0               \n"
      "xvhaddw.wu.hu    $xr0, $xr0, $xr0               \n"
      "xvadd.w          $xr4, $xr4, $xr0               \n"
      "addi.d           %[sa], %[sa], 32               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bgtz             %[cnt], 1b                     \n"

      "xvhaddw.du.wu    $xr4, $xr4, $xr4               \n"
      "xvhaddw.qu.du    $xr4, $xr4, $xr4               \n"
      "xvpermi.q        $xr0, $xr4, 0x01               \n"
      "vadd.w           $vr4, $vr4, $vr0               \n"
      "vpickve2gr.wu    %[diff], $vr4, 0               \n"
      : [sa] "+r"(src_a),   // %[sa]
        [sb] "+r"(src_b),   // %[sb]
        [cnt] "+r"(count),  // %[cnt]
        [diff] "=r"(diff)   // %[diff]
      :
      : "memory", "$xr0", "$xr1", "$xr4");
  return diff;
}
#endif  // HAS_HAMMINGDISTANCE_LASX

#ifdef HAS_SUMSQUAREERROR_LASX
uint32_t SumSquareError_LASX(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count) {
  if (count <= 0) {
    return 0;
  }
  uint32_t sse;
  asm volatile(
      "xvxor.v          $xr4, $xr4, $xr4               \n"

      "1:                                              \n"
      "xvld             $xr0, %[sa], 0                 \n"
      "xvld             $xr1, %[sb], 0                 \n"
      "xvabsd.bu        $xr0, $xr0, $xr1               \n"
      "xvmulwev.h.bu    $xr2, $xr0, $xr0               \n"
      "xvmulwod.h.bu    $xr3, $xr0, $xr0               \n"
      "xvhaddw.wu.hu    $xr2, $xr2, $xr2               \n"
      "xvhaddw.wu.hu    $xr3, $xr3, $xr3               \n"
      "xvadd.w          $xr4, $xr4, $xr2               \n"
      "xvadd.w          $xr4, $xr4, $xr3               \n"
      "addi.d           %[sa], %[sa], 32               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bgtz             %[cnt], 1b                     \n"

      "xvhaddw.du.wu    $xr4, $xr4, $xr4               \n"
      "xvhaddw.qu.du    $xr4, $xr4, $xr4               \n"
      "xvpermi.q        $xr0, $xr4, 0x01               \n"
      "vadd.w           $vr4, $vr4, $vr0               \n"
      "vpickve2gr.wu    %[sse], $vr4, 0                \n"
      : [sa] "+r"(src_a),   // %[sa]
        [sb] "+r"(src_b),   // %[sb]
        [cnt] "+r"(count),  // %[cnt]
        [sse] "=r"(sse)     // %[sse]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
  return sse;
}
#endif  // HAS_SUMSQUAREERROR_LASX

#ifdef HAS_HASHDJB2_LASX
uint32_t HashDjb2_LASX(const uint8_t* src, int count, uint32_t seed) {
  if (count <= 0) {
    return seed;
  }
  uint32_t hash = seed;
  uint32_t tmp;
  uint32_t c16 = 0x92d9e201u;
  asm volatile(
      "xvld             $xr4, %[muls], 0               \n"
      "xvld             $xr5, %[muls], 32              \n"

      "1:                                              \n"
      "vld              $vr0, %[src], 0                \n"
      "vbsrl.v          $vr1, $vr0, 8                  \n"
      "vext2xv.wu.bu    $xr0, $xr0                     \n"
      "vext2xv.wu.bu    $xr1, $xr1                     \n"
      "xvmul.w          $xr0, $xr0, $xr4               \n"
      "xvmadd.w         $xr0, $xr1, $xr5               \n"
      "xvhaddw.du.wu    $xr0, $xr0, $xr0               \n"
      "xvhaddw.qu.du    $xr0, $xr0, $xr0               \n"
      "xvpermi.q        $xr1, $xr0, 0x01               \n"
      "vadd.w           $vr0, $vr0, $vr1               \n"
      "vpickve2gr.wu    %[tmp], $vr0, 0                \n"
      "mul.w            %[hash], %[hash], %[c16]       \n"
      "add.w            %[hash], %[hash], %[tmp]       \n"
      "addi.d           %[src], %[src], 16             \n"
      "addi.d           %[cnt], %[cnt], -16            \n"
      "bgtz             %[cnt], 1b                     \n"
      : [src] "+r"(src),                 // %[src]
        [cnt] "+r"(count),               // %[cnt]
        [hash] "+r"(hash),               // %[hash]
        [tmp] "=&r"(tmp)                 // %[tmp]
      : [muls] "r"(kDjb2Multiplicands),  // %[muls]
        [c16] "r"(c16)                   // %[c16]
      : "memory", "$xr0", "$xr1", "$xr4", "$xr5");
  return hash;
}
#endif  // HAS_HASHDJB2_LASX

#ifdef HAS_SSIM4X4ROW_LASX
void Ssim4x4Row_LASX(const uint8_t* src_a,
                     int stride_a,
                     const uint8_t* src_b,
                     int stride_b,
                     uint32_t* dst,
                     int width) {
  if (width <= 0) {
    return;
  }
  const uint8_t* pa;
  const uint8_t* pb;
  int r;
  asm volatile(
      "1:                                              \n"
      "move             %[pa], %[sa]                   \n"
      "move             %[pb], %[sb]                   \n"
      "li.d             %[r], 4                        \n"
      "xvxor.v          $xr8, $xr8, $xr8               \n"
      "xvxor.v          $xr9, $xr9, $xr9               \n"
      "xvxor.v          $xr10, $xr10, $xr10            \n"
      "xvxor.v          $xr11, $xr11, $xr11            \n"

      "2:                                              \n"
      "xvld             $xr0, %[pa], 0                 \n"
      "xvld             $xr1, %[pb], 0                 \n"
      "add.d            %[pa], %[pa], %[sta]           \n"
      "add.d            %[pb], %[pb], %[stb]           \n"
      "xvhaddw.hu.bu    $xr2, $xr0, $xr0               \n"
      "xvhaddw.hu.bu    $xr3, $xr1, $xr1               \n"
      "xvadd.h          $xr8, $xr8, $xr2               \n"
      "xvadd.h          $xr9, $xr9, $xr3               \n"
      "xvmulwev.h.bu    $xr2, $xr0, $xr1               \n"
      "xvmulwod.h.bu    $xr3, $xr0, $xr1               \n"
      "xvhaddw.wu.hu    $xr2, $xr2, $xr2               \n"
      "xvhaddw.wu.hu    $xr3, $xr3, $xr3               \n"
      "xvadd.w          $xr11, $xr11, $xr2             \n"
      "xvadd.w          $xr11, $xr11, $xr3             \n"
      "xvmulwev.h.bu    $xr2, $xr0, $xr0               \n"
      "xvmulwod.h.bu    $xr3, $xr0, $xr0               \n"
      "xvmulwev.h.bu    $xr4, $xr1, $xr1               \n"
      "xvmulwod.h.bu    $xr5, $xr1, $xr1               \n"
      "xvhaddw.wu.hu    $xr2, $xr2, $xr2               \n"
      "xvhaddw.wu.hu    $xr3, $xr3, $xr3               \n"
      "xvhaddw.wu.hu    $xr4, $xr4, $xr4               \n"
      "xvhaddw.wu.hu    $xr5, $xr5, $xr5               \n"
      "xvadd.w          $xr2, $xr2, $xr3               \n"
      "xvadd.w          $xr4, $xr4, $xr5               \n"
      "xvadd.w          $xr10, $xr10, $xr2             \n"
      "xvadd.w          $xr10, $xr10, $xr4             \n"
      "addi.d           %[r], %[r], -1                 \n"
      "bnez             %[r], 2b                       \n"

      "xvhaddw.wu.hu    $xr8, $xr8, $xr8               \n"
      "xvhaddw.wu.hu    $xr9, $xr9, $xr9               \n"
      "xvilvl.w         $xr0, $xr9, $xr8               \n"
      "xvilvh.w         $xr1, $xr9, $xr8               \n"
      "xvilvl.w         $xr2, $xr11, $xr10             \n"
      "xvilvh.w         $xr3, $xr11, $xr10             \n"
      "xvilvl.d         $xr4, $xr2, $xr0               \n"
      "xvilvh.d         $xr5, $xr2, $xr0               \n"
      "xvilvl.d         $xr6, $xr3, $xr1               \n"
      "xvilvh.d         $xr7, $xr3, $xr1               \n"
      "xvor.v           $xr0, $xr5, $xr5               \n"
      "xvpermi.q        $xr0, $xr4, 0x20               \n"
      "xvpermi.q        $xr5, $xr4, 0x31               \n"
      "xvor.v           $xr1, $xr7, $xr7               \n"
      "xvpermi.q        $xr1, $xr6, 0x20               \n"
      "xvpermi.q        $xr7, $xr6, 0x31               \n"
      "xvst             $xr0, %[dst], 0                \n"
      "xvst             $xr1, %[dst], 32               \n"
      "xvst             $xr5, %[dst], 64               \n"
      "xvst             $xr7, %[dst], 96               \n"
      "addi.d           %[sa], %[sa], 32               \n"
      "addi.d           %[sb], %[sb], 32               \n"
      "addi.d           %[dst], %[dst], 128            \n"
      "addi.d           %[cnt], %[cnt], -32            \n"
      "bgtz             %[cnt], 1b                     \n"
      : [sa] "+r"(src_a),                // %[sa]
        [sb] "+r"(src_b),                // %[sb]
        [dst] "+r"(dst),                 // %[dst]
        [cnt] "+r"(width),               // %[cnt]
        [pa] "=&r"(pa),                  // %[pa]
        [pb] "=&r"(pb),                  // %[pb]
        [r] "=&r"(r)                     // %[r]
      : [sta] "r"((ptrdiff_t)stride_a),  // %[sta]
        [stb] "r"((ptrdiff_t)stride_b)   // %[stb]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11");
}
#endif  // HAS_SSIM4X4ROW_LASX

#ifdef HAS_SSIMWINDOWROW_LASX
float SsimWindowRow_LASX(const uint32_t* row0,
                         const uint32_t* row1,
                         int num_windows) {
  if (num_windows <= 0) {
    return 0.0f;
  }
  const uint32_t* row0_next = row0 + 4;
  const uint32_t* row1_next = row1 + 4;
  float total;
  uint32_t c1 = 26634;
  uint32_t c2 = 239708;
  uint32_t c0 = 0;
  asm volatile(
      "xvreplgr2vr.w    $xr6, %[c0]                    \n"
      "xvreplgr2vr.w    $xr4, %[c1]                    \n"
      "xvreplgr2vr.w    $xr5, %[c2]                    \n"
      "addi.d           %[cnt], %[cnt], -2             \n"
      "bltz             %[cnt], 2f                     \n"

      "1:                                              \n"
      "xvld             $xr0, %[r0], 0                 \n"
      "xvld             $xr1, %[r0n], 0                \n"
      "xvld             $xr2, %[r1], 0                 \n"
      "xvld             $xr3, %[r1n], 0                \n"
      "xvadd.w          $xr0, $xr0, $xr1               \n"
      "xvadd.w          $xr2, $xr2, $xr3               \n"
      "xvadd.w          $xr0, $xr0, $xr2               \n"
      "xvshuf4i.w       $xr1, $xr0, 0x00               \n"
      "xvshuf4i.w       $xr2, $xr0, 0x55               \n"
      "xvshuf4i.w       $xr3, $xr0, 0xaa               \n"
      "xvshuf4i.w       $xr0, $xr0, 0xff               \n"
      "xvmul.w          $xr7, $xr1, $xr2               \n"
      "xvslli.w         $xr7, $xr7, 1                  \n"
      "xvmul.w          $xr1, $xr1, $xr1               \n"
      "xvmadd.w         $xr1, $xr2, $xr2               \n"
      "xvslli.w         $xr0, $xr0, 7                  \n"
      "xvsub.w          $xr0, $xr0, $xr7               \n"
      "xvadd.w          $xr7, $xr7, $xr4               \n"
      "xvadd.w          $xr0, $xr0, $xr5               \n"
      "xvslli.w         $xr3, $xr3, 6                  \n"
      "xvsub.w          $xr3, $xr3, $xr1               \n"
      "xvadd.w          $xr1, $xr1, $xr4               \n"
      "xvadd.w          $xr3, $xr3, $xr5               \n"
      "xvffint.s.w      $xr7, $xr7                     \n"
      "xvffint.s.w      $xr0, $xr0                     \n"
      "xvffint.s.w      $xr1, $xr1                     \n"
      "xvffint.s.w      $xr3, $xr3                     \n"
      "xvfmul.s         $xr7, $xr7, $xr0               \n"
      "xvfmul.s         $xr1, $xr1, $xr3               \n"
      "xvfdiv.s         $xr7, $xr7, $xr1               \n"
      "xvfadd.s         $xr6, $xr6, $xr7               \n"
      "addi.d           %[r0], %[r0], 32               \n"
      "addi.d           %[r0n], %[r0n], 32             \n"
      "addi.d           %[r1], %[r1], 32               \n"
      "addi.d           %[r1n], %[r1n], 32             \n"
      "addi.d           %[cnt], %[cnt], -2             \n"
      "bgez             %[cnt], 1b                     \n"

      "xvpermi.q        $xr0, $xr6, 0x01               \n"
      "vfadd.s          $vr6, $vr6, $vr0               \n"

      "2:                                              \n"
      "addi.d           %[cnt], %[cnt], 2              \n"
      "beqz             %[cnt], 3f                     \n"
      "vld              $vr0, %[r0], 0                 \n"
      "vld              $vr1, %[r0n], 0                \n"
      "vld              $vr2, %[r1], 0                 \n"
      "vld              $vr3, %[r1n], 0                \n"
      "vadd.w           $vr0, $vr0, $vr1               \n"
      "vadd.w           $vr2, $vr2, $vr3               \n"
      "vadd.w           $vr0, $vr0, $vr2               \n"
      "vshuf4i.w        $vr1, $vr0, 0x00               \n"
      "vshuf4i.w        $vr2, $vr0, 0x55               \n"
      "vshuf4i.w        $vr3, $vr0, 0xaa               \n"
      "vshuf4i.w        $vr0, $vr0, 0xff               \n"
      "vmul.w           $vr7, $vr1, $vr2               \n"
      "vslli.w          $vr7, $vr7, 1                  \n"
      "vmul.w           $vr1, $vr1, $vr1               \n"
      "vmadd.w          $vr1, $vr2, $vr2               \n"
      "vslli.w          $vr0, $vr0, 7                  \n"
      "vsub.w           $vr0, $vr0, $vr7               \n"
      "vadd.w           $vr7, $vr7, $vr4               \n"
      "vadd.w           $vr0, $vr0, $vr5               \n"
      "vslli.w          $vr3, $vr3, 6                  \n"
      "vsub.w           $vr3, $vr3, $vr1               \n"
      "vadd.w           $vr1, $vr1, $vr4               \n"
      "vadd.w           $vr3, $vr3, $vr5               \n"
      "vffint.s.w       $vr7, $vr7                     \n"
      "vffint.s.w       $vr0, $vr0                     \n"
      "vffint.s.w       $vr1, $vr1                     \n"
      "vffint.s.w       $vr3, $vr3                     \n"
      "vfmul.s          $vr7, $vr7, $vr0               \n"
      "vfmul.s          $vr1, $vr1, $vr3               \n"
      "vfdiv.s          $vr7, $vr7, $vr1               \n"
      "vfadd.s          $vr6, $vr6, $vr7               \n"

      "3:                                              \n"
      "vstelm.w         $vr6, %[tot], 0, 0             \n"
      : [r0] "+r"(row0),         // %[r0]
        [r0n] "+r"(row0_next),   // %[r0n]
        [r1] "+r"(row1),         // %[r1]
        [r1n] "+r"(row1_next),   // %[r1n]
        [cnt] "+r"(num_windows)  // %[cnt]
      : [c0] "r"(c0),            // %[c0]
        [c1] "r"(c1),            // %[c1]
        [c2] "r"(c2),            // %[c2]
        [tot] "r"(&total)        // %[tot]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
  return total;
}
#endif  // HAS_SSIMWINDOWROW_LASX

#ifdef HAS_ARGBDETECTROW_LASX
uint32_t ARGBDetectRow_LASX(const uint8_t* argb, int width) {
  uint32_t diff = 0;
  uint32_t tmp;
  uint32_t rgb_mask = 0x00ffff00u;
  asm volatile(
      "xvreplgr2vr.w    $xr2, %[rgb_mask]              \n"
      "addi.d           %[w], %[w], -8                 \n"
      "bltz             %[w], 2f                       \n"

      "1:                                              \n"
      "xvld             $xr0, %[argb], 0               \n"
      "addi.d           %[argb], %[argb], 32           \n"
      "xvor.v           $xr0, $xr0, $xr2               \n"
      "xvseqi.b         $xr0, $xr0, -1                 \n"
      "xvmskltz.b       $xr0, $xr0                     \n"
      "xvpickve2gr.wu   %[diff], $xr0, 0               \n"
      "xvpickve2gr.wu   %[tmp], $xr0, 4                \n"
      "bstrins.w        %[diff], %[tmp], 31, 16        \n"
      "nor              %[diff], %[diff], $zero        \n"
      "bnez             %[diff], 4f                    \n"
      "addi.d           %[w], %[w], -8                 \n"
      "bgez             %[w], 1b                       \n"

      "2:                                              \n"
      "addi.d           %[w], %[w], 8                  \n"
      "blez             %[w], 4f                       \n"

      "3:                                              \n"
      "vldrepl.w        $vr0, %[argb], 0               \n"
      "addi.d           %[argb], %[argb], 4            \n"
      "vor.v            $vr0, $vr0, $vr2               \n"
      "vseqi.b          $vr0, $vr0, -1                 \n"
      "vmskltz.b        $vr0, $vr0                     \n"
      "vpickve2gr.hu    %[diff], $vr0, 0               \n"
      "nor              %[diff], %[diff], $zero        \n"
      "bstrpick.w       %[diff], %[diff], 15, 0        \n"
      "bnez             %[diff], 4f                    \n"
      "addi.d           %[w], %[w], -1                 \n"
      "bnez             %[w], 3b                       \n"

      "4:                                              \n"
      : [argb] "+r"(argb),        // %[argb]
        [w] "+r"(width),          // %[w]
        [diff] "+r"(diff),        // %[diff]
        [tmp] "=&r"(tmp)          // %[tmp]
      : [rgb_mask] "r"(rgb_mask)  // %[rgb_mask]
      : "memory", "$xr0", "$xr2");
  if (diff) {
    return (__builtin_ctz(diff) & 3) == 0 ? FOURCC_BGRA : FOURCC_ARGB;
  }
  return 0;
}
#endif  // HAS_ARGBDETECTROW_LASX

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // (!defined(LIBYUV_DISABLE_LSX) || !defined(LIBYUV_DISABLE_LASX)) &&
        // defined(__loongarch_lp64)

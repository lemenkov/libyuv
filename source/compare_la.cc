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
  uint32_t diff;
  asm volatile(
      "vxor.v $vr4, $vr4, $vr4\n\t"
      "1:\n\t"
      "vld $vr0, %[sa], 0\n\t"
      "vld $vr1, %[sb], 0\n\t"
      "vxor.v $vr0, $vr0, $vr1\n\t"
      "vpcnt.b $vr0, $vr0\n\t"
      "vhaddw.hu.bu $vr0, $vr0, $vr0\n\t"
      "vhaddw.wu.hu $vr0, $vr0, $vr0\n\t"
      "vadd.w $vr4, $vr4, $vr0\n\t"
      "addi.d %[sa], %[sa], 16\n\t"
      "addi.d %[sb], %[sb], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      "vhaddw.du.wu $vr4, $vr4, $vr4\n\t"
      "vhaddw.qu.du $vr4, $vr4, $vr4\n\t"
      "vpickve2gr.wu %[diff], $vr4, 0\n\t"
      : [sa] "+r"(src_a), [sb] "+r"(src_b), [cnt] "+r"(count), [diff] "=r"(diff)
      :
      : "memory", "$vr0", "$vr1", "$vr4");
  return diff;
}
#endif  // HAS_HAMMINGDISTANCE_LSX

#ifdef HAS_SUMSQUAREERROR_LSX
uint32_t SumSquareError_LSX(const uint8_t* src_a,
                            const uint8_t* src_b,
                            int count) {
  uint32_t sse;
  asm volatile(
      "vxor.v $vr4, $vr4, $vr4\n\t"
      "1:\n\t"
      "vld $vr0, %[sa], 0\n\t"
      "vld $vr1, %[sb], 0\n\t"
      "vabsd.bu $vr0, $vr0, $vr1\n\t"
      "vmulwev.h.bu $vr2, $vr0, $vr0\n\t"
      "vmulwod.h.bu $vr3, $vr0, $vr0\n\t"
      "vhaddw.wu.hu $vr2, $vr2, $vr2\n\t"
      "vhaddw.wu.hu $vr3, $vr3, $vr3\n\t"
      "vadd.w $vr4, $vr4, $vr2\n\t"
      "vadd.w $vr4, $vr4, $vr3\n\t"
      "addi.d %[sa], %[sa], 16\n\t"
      "addi.d %[sb], %[sb], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      "vhaddw.du.wu $vr4, $vr4, $vr4\n\t"
      "vhaddw.qu.du $vr4, $vr4, $vr4\n\t"
      "vpickve2gr.wu %[sse], $vr4, 0\n\t"
      : [sa] "+r"(src_a), [sb] "+r"(src_b), [cnt] "+r"(count), [sse] "=r"(sse)
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4");
  return sse;
}
#endif  // HAS_SUMSQUAREERROR_LSX

#ifdef HAS_HASHDJB2_LSX
uint32_t HashDjb2_LSX(const uint8_t* src, int count, uint32_t seed) {
  uint32_t hash = seed;
  uint32_t tmp;
  uint32_t c16 = 0x92d9e201u;
  asm volatile(
      "vld $vr4, %[muls], 0\n\t"
      "vld $vr5, %[muls], 16\n\t"
      "vld $vr6, %[muls], 32\n\t"
      "vld $vr7, %[muls], 48\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vsllwil.hu.bu $vr1, $vr0, 0\n\t"
      "vexth.hu.bu $vr2, $vr0\n\t"
      "vsllwil.wu.hu $vr0, $vr1, 0\n\t"
      "vexth.wu.hu $vr1, $vr1\n\t"
      "vsllwil.wu.hu $vr3, $vr2, 0\n\t"
      "vexth.wu.hu $vr2, $vr2\n\t"
      "vmul.w $vr0, $vr0, $vr4\n\t"
      "vmadd.w $vr0, $vr1, $vr5\n\t"
      "vmadd.w $vr0, $vr3, $vr6\n\t"
      "vmadd.w $vr0, $vr2, $vr7\n\t"
      "vhaddw.du.wu $vr0, $vr0, $vr0\n\t"
      "vhaddw.qu.du $vr0, $vr0, $vr0\n\t"
      "vpickve2gr.wu %[tmp], $vr0, 0\n\t"
      "mul.w %[hash], %[hash], %[c16]\n\t"
      "add.w %[hash], %[hash], %[tmp]\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [cnt] "+r"(count), [hash] "+r"(hash), [tmp] "=&r"(tmp)
      : [muls] "r"(kDjb2Multiplicands), [c16] "r"(c16)
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
  const uint8_t* pa;
  const uint8_t* pb;
  int r;
  asm volatile(
      "1:\n\t"
      "move %[pa], %[sa]\n\t"
      "move %[pb], %[sb]\n\t"
      "li.d %[r], 4\n\t"
      "vxor.v $vr8, $vr8, $vr8\n\t"
      "vxor.v $vr9, $vr9, $vr9\n\t"
      "vxor.v $vr10, $vr10, $vr10\n\t"
      "vxor.v $vr11, $vr11, $vr11\n\t"
      "2:\n\t"
      "vld $vr0, %[pa], 0\n\t"
      "vld $vr1, %[pb], 0\n\t"
      "add.d %[pa], %[pa], %[sta]\n\t"
      "add.d %[pb], %[pb], %[stb]\n\t"
      "vhaddw.hu.bu $vr2, $vr0, $vr0\n\t"
      "vhaddw.hu.bu $vr3, $vr1, $vr1\n\t"
      "vadd.h $vr8, $vr8, $vr2\n\t"
      "vadd.h $vr9, $vr9, $vr3\n\t"
      "vmulwev.h.bu $vr2, $vr0, $vr1\n\t"
      "vmulwod.h.bu $vr3, $vr0, $vr1\n\t"
      "vhaddw.wu.hu $vr2, $vr2, $vr2\n\t"
      "vhaddw.wu.hu $vr3, $vr3, $vr3\n\t"
      "vadd.w $vr11, $vr11, $vr2\n\t"
      "vadd.w $vr11, $vr11, $vr3\n\t"
      "vmulwev.h.bu $vr2, $vr0, $vr0\n\t"
      "vmulwod.h.bu $vr3, $vr0, $vr0\n\t"
      "vmulwev.h.bu $vr4, $vr1, $vr1\n\t"
      "vmulwod.h.bu $vr5, $vr1, $vr1\n\t"
      "vhaddw.wu.hu $vr2, $vr2, $vr2\n\t"
      "vhaddw.wu.hu $vr3, $vr3, $vr3\n\t"
      "vhaddw.wu.hu $vr4, $vr4, $vr4\n\t"
      "vhaddw.wu.hu $vr5, $vr5, $vr5\n\t"
      "vadd.w $vr2, $vr2, $vr3\n\t"
      "vadd.w $vr4, $vr4, $vr5\n\t"
      "vadd.w $vr10, $vr10, $vr2\n\t"
      "vadd.w $vr10, $vr10, $vr4\n\t"
      "addi.d %[r], %[r], -1\n\t"
      "bnez %[r], 2b\n\t"
      "vhaddw.wu.hu $vr8, $vr8, $vr8\n\t"
      "vhaddw.wu.hu $vr9, $vr9, $vr9\n\t"
      "vilvl.w $vr0, $vr9, $vr8\n\t"
      "vilvh.w $vr1, $vr9, $vr8\n\t"
      "vilvl.w $vr2, $vr11, $vr10\n\t"
      "vilvh.w $vr3, $vr11, $vr10\n\t"
      "vilvl.d $vr4, $vr2, $vr0\n\t"
      "vilvh.d $vr5, $vr2, $vr0\n\t"
      "vilvl.d $vr6, $vr3, $vr1\n\t"
      "vilvh.d $vr7, $vr3, $vr1\n\t"
      "vst $vr4, %[dst], 0\n\t"
      "vst $vr5, %[dst], 16\n\t"
      "vst $vr6, %[dst], 32\n\t"
      "vst $vr7, %[dst], 48\n\t"
      "addi.d %[sa], %[sa], 16\n\t"
      "addi.d %[sb], %[sb], 16\n\t"
      "addi.d %[dst], %[dst], 64\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sa] "+r"(src_a), [sb] "+r"(src_b), [dst] "+r"(dst), [cnt] "+r"(width),
        [pa] "=&r"(pa), [pb] "=&r"(pb), [r] "=&r"(r)
      : [sta] "r"((ptrdiff_t)stride_a), [stb] "r"((ptrdiff_t)stride_b)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_SSIM4X4ROW_LSX

#ifdef HAS_SSIMWINDOWROW_LSX
float SsimWindowRow_LSX(const uint32_t* row0,
                        const uint32_t* row1,
                        int num_windows) {
  const uint32_t* row0_next = row0 + 4;
  const uint32_t* row1_next = row1 + 4;
  float total;
  uint32_t c1 = 26634;
  uint32_t c2 = 239708;
  uint32_t c0 = 0;
  asm volatile(
      "vreplgr2vr.w $vr6, %[c0]\n\t"
      "vreplgr2vr.w $vr4, %[c1]\n\t"
      "vreplgr2vr.w $vr5, %[c2]\n\t"
      "1:\n\t"
      "vld $vr0, %[r0], 0\n\t"
      "vld $vr1, %[r0n], 0\n\t"
      "vld $vr2, %[r1], 0\n\t"
      "vld $vr3, %[r1n], 0\n\t"
      "vadd.w $vr0, $vr0, $vr1\n\t"
      "vadd.w $vr2, $vr2, $vr3\n\t"
      "vadd.w $vr0, $vr0, $vr2\n\t"
      "vshuf4i.w $vr1, $vr0, 0x00\n\t"
      "vshuf4i.w $vr2, $vr0, 0x55\n\t"
      "vshuf4i.w $vr3, $vr0, 0xaa\n\t"
      "vshuf4i.w $vr0, $vr0, 0xff\n\t"
      "vmul.w $vr7, $vr1, $vr2\n\t"
      "vslli.w $vr7, $vr7, 1\n\t"
      "vmul.w $vr1, $vr1, $vr1\n\t"
      "vmadd.w $vr1, $vr2, $vr2\n\t"
      "vslli.w $vr0, $vr0, 7\n\t"
      "vsub.w $vr0, $vr0, $vr7\n\t"
      "vadd.w $vr7, $vr7, $vr4\n\t"
      "vadd.w $vr0, $vr0, $vr5\n\t"
      "vslli.w $vr3, $vr3, 6\n\t"
      "vsub.w $vr3, $vr3, $vr1\n\t"
      "vadd.w $vr1, $vr1, $vr4\n\t"
      "vadd.w $vr3, $vr3, $vr5\n\t"
      "vffint.s.w $vr7, $vr7\n\t"
      "vffint.s.w $vr0, $vr0\n\t"
      "vffint.s.w $vr1, $vr1\n\t"
      "vffint.s.w $vr3, $vr3\n\t"
      "vfmul.s $vr7, $vr7, $vr0\n\t"
      "vfmul.s $vr1, $vr1, $vr3\n\t"
      "vfdiv.s $vr7, $vr7, $vr1\n\t"
      "vfadd.s $vr6, $vr6, $vr7\n\t"
      "addi.d %[r0], %[r0], 16\n\t"
      "addi.d %[r0n], %[r0n], 16\n\t"
      "addi.d %[r1], %[r1], 16\n\t"
      "addi.d %[r1n], %[r1n], 16\n\t"
      "addi.d %[cnt], %[cnt], -1\n\t"
      "bnez %[cnt], 1b\n\t"
      "vstelm.w $vr6, %[tot], 0, 0\n\t"
      : [r0] "+r"(row0), [r0n] "+r"(row0_next), [r1] "+r"(row1),
        [r1n] "+r"(row1_next), [cnt] "+r"(num_windows)
      : [c0] "r"(c0), [c1] "r"(c1), [c2] "r"(c2), [tot] "r"(&total)
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
      "vreplgr2vr.w $vr2, %[rgb_mask]\n\t"
      "addi.d %[w], %[w], -4\n\t"
      "bltz %[w], 2f\n\t"
      "1:\n\t"
      "vld $vr0, %[argb], 0\n\t"
      "addi.d %[argb], %[argb], 16\n\t"
      "vor.v $vr0, $vr0, $vr2\n\t"
      "vseqi.b $vr0, $vr0, -1\n\t"
      "vmskltz.b $vr0, $vr0\n\t"
      "vpickve2gr.hu %[diff], $vr0, 0\n\t"
      "nor %[diff], %[diff], $zero\n\t"
      "bstrpick.w %[diff], %[diff], 15, 0\n\t"
      "bnez %[diff], 4f\n\t"
      "addi.d %[w], %[w], -4\n\t"
      "bgez %[w], 1b\n\t"
      "2:\n\t"
      "addi.d %[w], %[w], 4\n\t"
      "beqz %[w], 4f\n\t"
      "3:\n\t"
      "vldrepl.w $vr0, %[argb], 0\n\t"
      "addi.d %[argb], %[argb], 4\n\t"
      "vor.v $vr0, $vr0, $vr2\n\t"
      "vseqi.b $vr0, $vr0, -1\n\t"
      "vmskltz.b $vr0, $vr0\n\t"
      "vpickve2gr.hu %[diff], $vr0, 0\n\t"
      "nor %[diff], %[diff], $zero\n\t"
      "bstrpick.w %[diff], %[diff], 15, 0\n\t"
      "bnez %[diff], 4f\n\t"
      "addi.d %[w], %[w], -1\n\t"
      "bnez %[w], 3b\n\t"
      "4:\n\t"
      : [argb] "+r"(argb), [w] "+r"(width), [diff] "+r"(diff)
      : [rgb_mask] "r"(rgb_mask)
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
  uint32_t diff;
  asm volatile(
      "xvxor.v $xr4, $xr4, $xr4\n\t"
      "1:\n\t"
      "xvld $xr0, %[sa], 0\n\t"
      "xvld $xr1, %[sb], 0\n\t"
      "xvxor.v $xr0, $xr0, $xr1\n\t"
      "xvpcnt.b $xr0, $xr0\n\t"
      "xvhaddw.hu.bu $xr0, $xr0, $xr0\n\t"
      "xvhaddw.wu.hu $xr0, $xr0, $xr0\n\t"
      "xvadd.w $xr4, $xr4, $xr0\n\t"
      "addi.d %[sa], %[sa], 32\n\t"
      "addi.d %[sb], %[sb], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      "xvhaddw.du.wu $xr4, $xr4, $xr4\n\t"
      "xvhaddw.qu.du $xr4, $xr4, $xr4\n\t"
      "xvpermi.q $xr0, $xr4, 0x01\n\t"
      "vadd.w $vr4, $vr4, $vr0\n\t"
      "vpickve2gr.wu %[diff], $vr4, 0\n\t"
      : [sa] "+r"(src_a), [sb] "+r"(src_b), [cnt] "+r"(count), [diff] "=r"(diff)
      :
      : "memory", "$xr0", "$xr1", "$xr4");
  return diff;
}
#endif  // HAS_HAMMINGDISTANCE_LASX

#ifdef HAS_SUMSQUAREERROR_LASX
uint32_t SumSquareError_LASX(const uint8_t* src_a,
                             const uint8_t* src_b,
                             int count) {
  uint32_t sse;
  asm volatile(
      "xvxor.v $xr4, $xr4, $xr4\n\t"
      "1:\n\t"
      "xvld $xr0, %[sa], 0\n\t"
      "xvld $xr1, %[sb], 0\n\t"
      "xvabsd.bu $xr0, $xr0, $xr1\n\t"
      "xvmulwev.h.bu $xr2, $xr0, $xr0\n\t"
      "xvmulwod.h.bu $xr3, $xr0, $xr0\n\t"
      "xvhaddw.wu.hu $xr2, $xr2, $xr2\n\t"
      "xvhaddw.wu.hu $xr3, $xr3, $xr3\n\t"
      "xvadd.w $xr4, $xr4, $xr2\n\t"
      "xvadd.w $xr4, $xr4, $xr3\n\t"
      "addi.d %[sa], %[sa], 32\n\t"
      "addi.d %[sb], %[sb], 32\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      "xvhaddw.du.wu $xr4, $xr4, $xr4\n\t"
      "xvhaddw.qu.du $xr4, $xr4, $xr4\n\t"
      "xvpermi.q $xr0, $xr4, 0x01\n\t"
      "vadd.w $vr4, $vr4, $vr0\n\t"
      "vpickve2gr.wu %[sse], $vr4, 0\n\t"
      : [sa] "+r"(src_a), [sb] "+r"(src_b), [cnt] "+r"(count), [sse] "=r"(sse)
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4");
  return sse;
}
#endif  // HAS_SUMSQUAREERROR_LASX

#ifdef HAS_HASHDJB2_LASX
uint32_t HashDjb2_LASX(const uint8_t* src, int count, uint32_t seed) {
  uint32_t hash = seed;
  uint32_t tmp;
  uint32_t c16 = 0x92d9e201u;
  asm volatile(
      "xvld $xr4, %[muls], 0\n\t"
      "xvld $xr5, %[muls], 32\n\t"
      "1:\n\t"
      "vld $vr0, %[src], 0\n\t"
      "vbsrl.v $vr1, $vr0, 8\n\t"
      "vext2xv.wu.bu $xr0, $xr0\n\t"
      "vext2xv.wu.bu $xr1, $xr1\n\t"
      "xvmul.w $xr0, $xr0, $xr4\n\t"
      "xvmadd.w $xr0, $xr1, $xr5\n\t"
      "xvhaddw.du.wu $xr0, $xr0, $xr0\n\t"
      "xvhaddw.qu.du $xr0, $xr0, $xr0\n\t"
      "xvpermi.q $xr1, $xr0, 0x01\n\t"
      "vadd.w $vr0, $vr0, $vr1\n\t"
      "vpickve2gr.wu %[tmp], $vr0, 0\n\t"
      "mul.w %[hash], %[hash], %[c16]\n\t"
      "add.w %[hash], %[hash], %[tmp]\n\t"
      "addi.d %[src], %[src], 16\n\t"
      "addi.d %[cnt], %[cnt], -16\n\t"
      "bnez %[cnt], 1b\n\t"
      : [src] "+r"(src), [cnt] "+r"(count), [hash] "+r"(hash), [tmp] "=&r"(tmp)
      : [muls] "r"(kDjb2Multiplicands), [c16] "r"(c16)
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
  const uint8_t* pa;
  const uint8_t* pb;
  int r;
  asm volatile(
      "1:\n\t"
      "move %[pa], %[sa]\n\t"
      "move %[pb], %[sb]\n\t"
      "li.d %[r], 4\n\t"
      "xvxor.v $xr8, $xr8, $xr8\n\t"
      "xvxor.v $xr9, $xr9, $xr9\n\t"
      "xvxor.v $xr10, $xr10, $xr10\n\t"
      "xvxor.v $xr11, $xr11, $xr11\n\t"
      "2:\n\t"
      "xvld $xr0, %[pa], 0\n\t"
      "xvld $xr1, %[pb], 0\n\t"
      "add.d %[pa], %[pa], %[sta]\n\t"
      "add.d %[pb], %[pb], %[stb]\n\t"
      "xvhaddw.hu.bu $xr2, $xr0, $xr0\n\t"
      "xvhaddw.hu.bu $xr3, $xr1, $xr1\n\t"
      "xvadd.h $xr8, $xr8, $xr2\n\t"
      "xvadd.h $xr9, $xr9, $xr3\n\t"
      "xvmulwev.h.bu $xr2, $xr0, $xr1\n\t"
      "xvmulwod.h.bu $xr3, $xr0, $xr1\n\t"
      "xvhaddw.wu.hu $xr2, $xr2, $xr2\n\t"
      "xvhaddw.wu.hu $xr3, $xr3, $xr3\n\t"
      "xvadd.w $xr11, $xr11, $xr2\n\t"
      "xvadd.w $xr11, $xr11, $xr3\n\t"
      "xvmulwev.h.bu $xr2, $xr0, $xr0\n\t"
      "xvmulwod.h.bu $xr3, $xr0, $xr0\n\t"
      "xvmulwev.h.bu $xr4, $xr1, $xr1\n\t"
      "xvmulwod.h.bu $xr5, $xr1, $xr1\n\t"
      "xvhaddw.wu.hu $xr2, $xr2, $xr2\n\t"
      "xvhaddw.wu.hu $xr3, $xr3, $xr3\n\t"
      "xvhaddw.wu.hu $xr4, $xr4, $xr4\n\t"
      "xvhaddw.wu.hu $xr5, $xr5, $xr5\n\t"
      "xvadd.w $xr2, $xr2, $xr3\n\t"
      "xvadd.w $xr4, $xr4, $xr5\n\t"
      "xvadd.w $xr10, $xr10, $xr2\n\t"
      "xvadd.w $xr10, $xr10, $xr4\n\t"
      "addi.d %[r], %[r], -1\n\t"
      "bnez %[r], 2b\n\t"
      "xvhaddw.wu.hu $xr8, $xr8, $xr8\n\t"
      "xvhaddw.wu.hu $xr9, $xr9, $xr9\n\t"
      "xvilvl.w $xr0, $xr9, $xr8\n\t"
      "xvilvh.w $xr1, $xr9, $xr8\n\t"
      "xvilvl.w $xr2, $xr11, $xr10\n\t"
      "xvilvh.w $xr3, $xr11, $xr10\n\t"
      "xvilvl.d $xr4, $xr2, $xr0\n\t"
      "xvilvh.d $xr5, $xr2, $xr0\n\t"
      "xvilvl.d $xr6, $xr3, $xr1\n\t"
      "xvilvh.d $xr7, $xr3, $xr1\n\t"
      "xvor.v $xr0, $xr5, $xr5\n\t"
      "xvpermi.q $xr0, $xr4, 0x20\n\t"
      "xvpermi.q $xr5, $xr4, 0x31\n\t"
      "xvor.v $xr1, $xr7, $xr7\n\t"
      "xvpermi.q $xr1, $xr6, 0x20\n\t"
      "xvpermi.q $xr7, $xr6, 0x31\n\t"
      "xvst $xr0, %[dst], 0\n\t"
      "xvst $xr1, %[dst], 32\n\t"
      "xvst $xr5, %[dst], 64\n\t"
      "xvst $xr7, %[dst], 96\n\t"
      "addi.d %[sa], %[sa], 32\n\t"
      "addi.d %[sb], %[sb], 32\n\t"
      "addi.d %[dst], %[dst], 128\n\t"
      "addi.d %[cnt], %[cnt], -32\n\t"
      "bnez %[cnt], 1b\n\t"
      : [sa] "+r"(src_a), [sb] "+r"(src_b), [dst] "+r"(dst), [cnt] "+r"(width),
        [pa] "=&r"(pa), [pb] "=&r"(pb), [r] "=&r"(r)
      : [sta] "r"((ptrdiff_t)stride_a), [stb] "r"((ptrdiff_t)stride_b)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11");
}
#endif  // HAS_SSIM4X4ROW_LASX

#ifdef HAS_SSIMWINDOWROW_LASX
float SsimWindowRow_LASX(const uint32_t* row0,
                         const uint32_t* row1,
                         int num_windows) {
  const uint32_t* row0_next = row0 + 4;
  const uint32_t* row1_next = row1 + 4;
  float total;
  uint32_t c1 = 26634;
  uint32_t c2 = 239708;
  uint32_t c0 = 0;
  asm volatile(
      "xvreplgr2vr.w $xr6, %[c0]\n\t"
      "xvreplgr2vr.w $xr4, %[c1]\n\t"
      "xvreplgr2vr.w $xr5, %[c2]\n\t"
      "addi.d %[cnt], %[cnt], -2\n\t"
      "bltz %[cnt], 2f\n\t"
      "1:\n\t"
      "xvld $xr0, %[r0], 0\n\t"
      "xvld $xr1, %[r0n], 0\n\t"
      "xvld $xr2, %[r1], 0\n\t"
      "xvld $xr3, %[r1n], 0\n\t"
      "xvadd.w $xr0, $xr0, $xr1\n\t"
      "xvadd.w $xr2, $xr2, $xr3\n\t"
      "xvadd.w $xr0, $xr0, $xr2\n\t"
      "xvshuf4i.w $xr1, $xr0, 0x00\n\t"
      "xvshuf4i.w $xr2, $xr0, 0x55\n\t"
      "xvshuf4i.w $xr3, $xr0, 0xaa\n\t"
      "xvshuf4i.w $xr0, $xr0, 0xff\n\t"
      "xvmul.w $xr7, $xr1, $xr2\n\t"
      "xvslli.w $xr7, $xr7, 1\n\t"
      "xvmul.w $xr1, $xr1, $xr1\n\t"
      "xvmadd.w $xr1, $xr2, $xr2\n\t"
      "xvslli.w $xr0, $xr0, 7\n\t"
      "xvsub.w $xr0, $xr0, $xr7\n\t"
      "xvadd.w $xr7, $xr7, $xr4\n\t"
      "xvadd.w $xr0, $xr0, $xr5\n\t"
      "xvslli.w $xr3, $xr3, 6\n\t"
      "xvsub.w $xr3, $xr3, $xr1\n\t"
      "xvadd.w $xr1, $xr1, $xr4\n\t"
      "xvadd.w $xr3, $xr3, $xr5\n\t"
      "xvffint.s.w $xr7, $xr7\n\t"
      "xvffint.s.w $xr0, $xr0\n\t"
      "xvffint.s.w $xr1, $xr1\n\t"
      "xvffint.s.w $xr3, $xr3\n\t"
      "xvfmul.s $xr7, $xr7, $xr0\n\t"
      "xvfmul.s $xr1, $xr1, $xr3\n\t"
      "xvfdiv.s $xr7, $xr7, $xr1\n\t"
      "xvfadd.s $xr6, $xr6, $xr7\n\t"
      "addi.d %[r0], %[r0], 32\n\t"
      "addi.d %[r0n], %[r0n], 32\n\t"
      "addi.d %[r1], %[r1], 32\n\t"
      "addi.d %[r1n], %[r1n], 32\n\t"
      "addi.d %[cnt], %[cnt], -2\n\t"
      "bgez %[cnt], 1b\n\t"
      "xvpermi.q $xr0, $xr6, 0x01\n\t"
      "vfadd.s $vr6, $vr6, $vr0\n\t"
      "2:\n\t"
      "addi.d %[cnt], %[cnt], 2\n\t"
      "beqz %[cnt], 3f\n\t"
      "vld $vr0, %[r0], 0\n\t"
      "vld $vr1, %[r0n], 0\n\t"
      "vld $vr2, %[r1], 0\n\t"
      "vld $vr3, %[r1n], 0\n\t"
      "vadd.w $vr0, $vr0, $vr1\n\t"
      "vadd.w $vr2, $vr2, $vr3\n\t"
      "vadd.w $vr0, $vr0, $vr2\n\t"
      "vshuf4i.w $vr1, $vr0, 0x00\n\t"
      "vshuf4i.w $vr2, $vr0, 0x55\n\t"
      "vshuf4i.w $vr3, $vr0, 0xaa\n\t"
      "vshuf4i.w $vr0, $vr0, 0xff\n\t"
      "vmul.w $vr7, $vr1, $vr2\n\t"
      "vslli.w $vr7, $vr7, 1\n\t"
      "vmul.w $vr1, $vr1, $vr1\n\t"
      "vmadd.w $vr1, $vr2, $vr2\n\t"
      "vslli.w $vr0, $vr0, 7\n\t"
      "vsub.w $vr0, $vr0, $vr7\n\t"
      "vadd.w $vr7, $vr7, $vr4\n\t"
      "vadd.w $vr0, $vr0, $vr5\n\t"
      "vslli.w $vr3, $vr3, 6\n\t"
      "vsub.w $vr3, $vr3, $vr1\n\t"
      "vadd.w $vr1, $vr1, $vr4\n\t"
      "vadd.w $vr3, $vr3, $vr5\n\t"
      "vffint.s.w $vr7, $vr7\n\t"
      "vffint.s.w $vr0, $vr0\n\t"
      "vffint.s.w $vr1, $vr1\n\t"
      "vffint.s.w $vr3, $vr3\n\t"
      "vfmul.s $vr7, $vr7, $vr0\n\t"
      "vfmul.s $vr1, $vr1, $vr3\n\t"
      "vfdiv.s $vr7, $vr7, $vr1\n\t"
      "vfadd.s $vr6, $vr6, $vr7\n\t"
      "3:\n\t"
      "vstelm.w $vr6, %[tot], 0, 0\n\t"
      : [r0] "+r"(row0), [r0n] "+r"(row0_next), [r1] "+r"(row1),
        [r1n] "+r"(row1_next), [cnt] "+r"(num_windows)
      : [c0] "r"(c0), [c1] "r"(c1), [c2] "r"(c2), [tot] "r"(&total)
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
      "xvreplgr2vr.w $xr2, %[rgb_mask]\n\t"
      "addi.d %[w], %[w], -8\n\t"
      "bltz %[w], 2f\n\t"
      "1:\n\t"
      "xvld $xr0, %[argb], 0\n\t"
      "addi.d %[argb], %[argb], 32\n\t"
      "xvor.v $xr0, $xr0, $xr2\n\t"
      "xvseqi.b $xr0, $xr0, -1\n\t"
      "xvmskltz.b $xr0, $xr0\n\t"
      "xvpickve2gr.wu %[diff], $xr0, 0\n\t"
      "xvpickve2gr.wu %[tmp], $xr0, 4\n\t"
      "bstrins.w %[diff], %[tmp], 31, 16\n\t"
      "nor %[diff], %[diff], $zero\n\t"
      "bnez %[diff], 4f\n\t"
      "addi.d %[w], %[w], -8\n\t"
      "bgez %[w], 1b\n\t"
      "2:\n\t"
      "addi.d %[w], %[w], 8\n\t"
      "beqz %[w], 4f\n\t"
      "3:\n\t"
      "vldrepl.w $vr0, %[argb], 0\n\t"
      "addi.d %[argb], %[argb], 4\n\t"
      "vor.v $vr0, $vr0, $vr2\n\t"
      "vseqi.b $vr0, $vr0, -1\n\t"
      "vmskltz.b $vr0, $vr0\n\t"
      "vpickve2gr.hu %[diff], $vr0, 0\n\t"
      "nor %[diff], %[diff], $zero\n\t"
      "bstrpick.w %[diff], %[diff], 15, 0\n\t"
      "bnez %[diff], 4f\n\t"
      "addi.d %[w], %[w], -1\n\t"
      "bnez %[w], 3b\n\t"
      "4:\n\t"
      : [argb] "+r"(argb), [w] "+r"(width), [diff] "+r"(diff), [tmp] "=&r"(tmp)
      : [rgb_mask] "r"(rgb_mask)
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

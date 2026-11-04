/*
 *  Copyright 2022 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"
#include "libyuv/scale_row.h"

#if (!defined(LIBYUV_DISABLE_LSX) || !defined(LIBYUV_DISABLE_LASX)) && \
    defined(__loongarch_lp64)

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#ifdef HAS_SCALEARGBROWDOWN2_LSX
void ScaleARGBRowDown2_LSX(const uint8_t* src_argb,
                           ptrdiff_t src_stride,
                           uint8_t* dst_argb,
                           int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 2                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vld        $vr1, %[src], 16                     \n"
      "vpickod.w  $vr0, $vr1, $vr0                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width)    // %[w]
      :
      : "memory", "$vr0", "$vr1");
}

void ScaleARGBRowDown2Linear_LSX(const uint8_t* src_argb,
                                 ptrdiff_t src_stride,
                                 uint8_t* dst_argb,
                                 int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 2                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vld        $vr1, %[src], 16                     \n"
      "vpickev.w  $vr2, $vr1, $vr0                     \n"
      "vpickod.w  $vr3, $vr1, $vr0                     \n"
      "vavgr.bu   $vr0, $vr3, $vr2                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width)    // %[w]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}

void ScaleARGBRowDown2Box_LSX(const uint8_t* src_argb,
                              ptrdiff_t src_stride,
                              uint8_t* dst_argb,
                              int dst_width) {
  const uint8_t* t = src_argb + src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 2                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[s], 0                        \n"
      "vld        $vr1, %[s], 16                       \n"
      "vld        $vr2, %[t], 0                        \n"
      "vld        $vr3, %[t], 16                       \n"
      "vpickev.w  $vr4, $vr1, $vr0                     \n"
      "vpickod.w  $vr5, $vr1, $vr0                     \n"
      "vpickev.w  $vr6, $vr3, $vr2                     \n"
      "vpickod.w  $vr7, $vr3, $vr2                     \n"
      "vaddwev.h.bu $vr0, $vr4, $vr5                   \n"
      "vaddwod.h.bu $vr1, $vr4, $vr5                   \n"
      "vaddwev.h.bu $vr2, $vr6, $vr7                   \n"
      "vaddwod.h.bu $vr3, $vr6, $vr7                   \n"
      "vadd.h     $vr0, $vr0, $vr2                     \n"
      "vadd.h     $vr1, $vr1, $vr3                     \n"
      "vsrari.h   $vr0, $vr0, 2                        \n"
      "vsrari.h   $vr1, $vr1, 2                        \n"
      "vpackev.b  $vr0, $vr1, $vr0                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[s], %[s], 32                       \n"
      "addi.d     %[t], %[t], 32                       \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [s] "+r"(src_argb),    // %[s]
        [t] "+r"(t),           // %[t]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width)    // %[w]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_SCALEARGBROWDOWN2_LSX

#ifdef HAS_SCALEARGBROWDOWNEVEN_LSX
void ScaleARGBRowDownEven_LSX(const uint8_t* src_argb,
                              ptrdiff_t src_stride,
                              int32_t src_stepx,
                              uint8_t* dst_argb,
                              int dst_width) {
  const ptrdiff_t step = (ptrdiff_t)src_stepx * 4;
  uint32_t r0, r1, r2, r3;
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 2                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "ld.w       %[r0], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r1], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r2], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r3], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "st.w       %[r0], %[dst], 0                     \n"
      "st.w       %[r1], %[dst], 4                     \n"
      "st.w       %[r2], %[dst], 8                     \n"
      "st.w       %[r3], %[dst], 12                    \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width),   // %[w]
        [r0] "=&r"(r0),        // %[r0]
        [r1] "=&r"(r1),        // %[r1]
        [r2] "=&r"(r2),        // %[r2]
        [r3] "=&r"(r3)         // %[r3]
      : [step] "r"(step)       // %[step]
      : "memory");
}

void ScaleARGBRowDownEvenBox_LSX(const uint8_t* src_argb,
                                 ptrdiff_t src_stride,
                                 int src_stepx,
                                 uint8_t* dst_argb,
                                 int dst_width) {
  const ptrdiff_t step = (ptrdiff_t)src_stepx * 4;
  const uint8_t* next_argb = src_argb + src_stride;
  uint64_t r0, r1;
  asm volatile(
      "srli.w     %[w], %[w], 2                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "ld.d       %[r0], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "ld.d       %[r1], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "vinsgr2vr.d $vr0, %[r0], 0                      \n"
      "vinsgr2vr.d $vr0, %[r1], 1                      \n"
      "ld.d       %[r0], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "ld.d       %[r1], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "vinsgr2vr.d $vr1, %[r0], 0                      \n"
      "vinsgr2vr.d $vr1, %[r1], 1                      \n"

      "ld.d       %[r0], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "ld.d       %[r1], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "vinsgr2vr.d $vr2, %[r0], 0                      \n"
      "vinsgr2vr.d $vr2, %[r1], 1                      \n"
      "ld.d       %[r0], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "ld.d       %[r1], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "vinsgr2vr.d $vr3, %[r0], 0                      \n"
      "vinsgr2vr.d $vr3, %[r1], 1                      \n"

      "vpickev.w  $vr4, $vr1, $vr0                     \n"
      "vpickod.w  $vr5, $vr1, $vr0                     \n"
      "vpickev.w  $vr6, $vr3, $vr2                     \n"
      "vpickod.w  $vr7, $vr3, $vr2                     \n"
      "vaddwev.h.bu $vr0, $vr4, $vr5                   \n"
      "vaddwod.h.bu $vr1, $vr4, $vr5                   \n"
      "vaddwev.h.bu $vr2, $vr6, $vr7                   \n"
      "vaddwod.h.bu $vr3, $vr6, $vr7                   \n"
      "vadd.h     $vr0, $vr0, $vr2                     \n"
      "vadd.h     $vr1, $vr1, $vr3                     \n"
      "vsrari.h   $vr0, $vr0, 2                        \n"
      "vsrari.h   $vr1, $vr1, 2                        \n"
      "vpackev.b  $vr0, $vr1, $vr0                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [s] "+r"(src_argb),    // %[s]
        [t] "+r"(next_argb),   // %[t]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width),   // %[w]
        [r0] "=&r"(r0),        // %[r0]
        [r1] "=&r"(r1)         // %[r1]
      : [step] "r"(step)       // %[step]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_SCALEARGBROWDOWNEVEN_LSX

#ifdef HAS_SCALEROWDOWN2_LSX
void ScaleRowDown2_LSX(const uint8_t* src_ptr,
                       ptrdiff_t src_stride,
                       uint8_t* dst,
                       int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 4                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vld        $vr1, %[src], 16                     \n"
      "vpickod.b  $vr0, $vr1, $vr0                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(dst_width)   // %[w]
      :
      : "memory", "$vr0", "$vr1");
}

void ScaleRowDown2Linear_LSX(const uint8_t* src_ptr,
                             ptrdiff_t src_stride,
                             uint8_t* dst,
                             int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 4                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vld        $vr1, %[src], 16                     \n"
      "vpickev.b  $vr2, $vr1, $vr0                     \n"
      "vpickod.b  $vr3, $vr1, $vr0                     \n"
      "vavgr.bu   $vr0, $vr3, $vr2                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(dst_width)   // %[w]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}

void ScaleRowDown2Box_LSX(const uint8_t* src_ptr,
                          ptrdiff_t src_stride,
                          uint8_t* dst,
                          int dst_width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 4                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vld        $vr1, %[src], 16                     \n"
      "vld        $vr2, %[nex], 0                      \n"
      "vld        $vr3, %[nex], 16                     \n"
      "vaddwev.h.bu $vr4, $vr0, $vr2                   \n"
      "vaddwod.h.bu $vr5, $vr0, $vr2                   \n"
      "vaddwev.h.bu $vr6, $vr1, $vr3                   \n"
      "vaddwod.h.bu $vr7, $vr1, $vr3                   \n"
      "vadd.h     $vr4, $vr4, $vr5                     \n"
      "vadd.h     $vr6, $vr6, $vr7                     \n"
      "vsrarni.b.h $vr6, $vr4, 2                       \n"
      "vst        $vr6, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[nex], %[nex], 32                   \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [nex] "+r"(src_nex),  // %[nex]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(dst_width)   // %[w]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_SCALEROWDOWN2_LSX

#ifdef HAS_SCALEROWDOWN4_LSX
void ScaleRowDown4_LSX(const uint8_t* src_ptr,
                       ptrdiff_t src_stride,
                       uint8_t* dst,
                       int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vld        $vr1, %[src], 16                     \n"
      "vpickev.b  $vr0, $vr1, $vr0                     \n"
      "vpickod.b  $vr0, $vr0, $vr0                     \n"
      "vstelm.d   $vr0, %[dst], 0, 0                   \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 8                    \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(dst_width)   // %[w]
      :
      : "memory", "$vr0", "$vr1");
}

void ScaleRowDown4Box_LSX(const uint8_t* src_ptr,
                          ptrdiff_t src_stride,
                          uint8_t* dst,
                          int dst_width) {
  const uint8_t* ptr1 = src_ptr + src_stride;
  const uint8_t* ptr2 = ptr1 + src_stride;
  const uint8_t* ptr3 = ptr2 + src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[p0], 0                       \n"
      "vld        $vr1, %[p0], 16                      \n"
      "vld        $vr2, %[p1], 0                       \n"
      "vld        $vr3, %[p1], 16                      \n"
      "vld        $vr4, %[p2], 0                       \n"
      "vld        $vr5, %[p2], 16                      \n"
      "vld        $vr6, %[p3], 0                       \n"
      "vld        $vr7, %[p3], 16                      \n"
      "vhaddw.hu.bu $vr0, $vr0, $vr0                   \n"
      "vhaddw.hu.bu $vr1, $vr1, $vr1                   \n"
      "vhaddw.hu.bu $vr2, $vr2, $vr2                   \n"
      "vhaddw.hu.bu $vr3, $vr3, $vr3                   \n"
      "vhaddw.hu.bu $vr4, $vr4, $vr4                   \n"
      "vhaddw.hu.bu $vr5, $vr5, $vr5                   \n"
      "vhaddw.hu.bu $vr6, $vr6, $vr6                   \n"
      "vhaddw.hu.bu $vr7, $vr7, $vr7                   \n"
      "vadd.h     $vr0, $vr0, $vr2                     \n"
      "vadd.h     $vr1, $vr1, $vr3                     \n"
      "vadd.h     $vr4, $vr4, $vr6                     \n"
      "vadd.h     $vr5, $vr5, $vr7                     \n"
      "vadd.h     $vr0, $vr0, $vr4                     \n"
      "vadd.h     $vr1, $vr1, $vr5                     \n"
      "vhaddw.wu.hu $vr0, $vr0, $vr0                   \n"
      "vhaddw.wu.hu $vr1, $vr1, $vr1                   \n"
      "vsrarni.h.w $vr1, $vr0, 4                       \n"
      "vsrlni.b.h $vr1, $vr1, 0                        \n"
      "vstelm.d   $vr1, %[dst], 0, 0                   \n"
      "addi.d     %[p0], %[p0], 32                     \n"
      "addi.d     %[p1], %[p1], 32                     \n"
      "addi.d     %[p2], %[p2], 32                     \n"
      "addi.d     %[p3], %[p3], 32                     \n"
      "addi.d     %[dst], %[dst], 8                    \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [p0] "+r"(src_ptr),  // %[p0]
        [p1] "+r"(ptr1),     // %[p1]
        [p2] "+r"(ptr2),     // %[p2]
        [p3] "+r"(ptr3),     // %[p3]
        [dst] "+r"(dst),     // %[dst]
        [w] "+r"(dst_width)  // %[w]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_SCALEROWDOWN4_LSX

#if defined(HAS_SCALEROWDOWN38_LSX) || defined(HAS_SCALEROWDOWN38_LASX)
static const uint64_t kScale38Shuf[2] = {0x13100e0b08060300ULL,
                                         0x000000001e1b1816ULL};
static const uint64_t kScale38BoxShuf[2] = {0x0a08160604120200ULL,
                                            0x000000001e0e0c1aULL};
#endif

#ifdef HAS_SCALEROWDOWN38_LSX

void ScaleRowDown38_LSX(const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        uint8_t* dst,
                        int dst_width) {
  int count = dst_width / 12;
  (void)src_stride;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr2, %[shuf], 0                     \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vld        $vr1, %[src], 16                     \n"
      "vshuf.b    $vr0, $vr1, $vr0, $vr2               \n"
      "vstelm.d   $vr0, %[dst], 0, 0                   \n"
      "vstelm.w   $vr0, %[dst], 8, 2                   \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),      // %[src]
        [dst] "+r"(dst),          // %[dst]
        [cnt] "+r"(count)         // %[cnt]
      : [shuf] "r"(kScale38Shuf)  // %[shuf]
      : "memory", "$vr0", "$vr1", "$vr2");
}

void ScaleRowDown38_2_Box_LSX(const uint8_t* src_ptr,
                              ptrdiff_t src_stride,
                              uint8_t* dst_ptr,
                              int dst_width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  int count = dst_width / 12;
  int c2aaa = 0x2aaa;
  int c4000 = 0x4000;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr8, %[shuf], 0                     \n"
      "vreplgr2vr.h $vr9, %[c2aaa]                     \n"
      "vreplgr2vr.w $vr10, %[c4000]                    \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vld        $vr1, %[src], 16                     \n"
      "vld        $vr2, %[nex], 0                      \n"
      "vld        $vr3, %[nex], 16                     \n"
      "vaddwev.h.bu $vr4, $vr0, $vr2                   \n"
      "vaddwod.h.bu $vr5, $vr0, $vr2                   \n"
      "vaddwev.h.bu $vr6, $vr1, $vr3                   \n"
      "vaddwod.h.bu $vr7, $vr1, $vr3                   \n"
      "vpickev.h  $vr0, $vr6, $vr4                     \n"
      "vpickev.h  $vr1, $vr7, $vr5                     \n"
      "vpackod.h  $vr2, $vr5, $vr4                     \n"
      "vpackod.h  $vr3, $vr7, $vr6                     \n"
      "vpickev.w  $vr4, $vr3, $vr2                     \n"
      "vadd.h     $vr0, $vr0, $vr1                     \n"
      "vadd.h     $vr0, $vr0, $vr4                     \n"
      "vmuh.h     $vr0, $vr0, $vr9                     \n"
      "vpickod.w  $vr2, $vr3, $vr2                     \n"
      "vhaddw.wu.hu $vr2, $vr2, $vr2                   \n"
      "vmul.w     $vr2, $vr2, $vr10                    \n"
      "vshuf.b    $vr0, $vr2, $vr0, $vr8               \n"
      "vstelm.d   $vr0, %[dst], 0, 0                   \n"
      "vstelm.w   $vr0, %[dst], 8, 2                   \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[nex], %[nex], 32                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),          // %[src]
        [nex] "+r"(src_nex),          // %[nex]
        [dst] "+r"(dst_ptr),          // %[dst]
        [cnt] "+r"(count)             // %[cnt]
      : [shuf] "r"(kScale38BoxShuf),  // %[shuf]
        [c2aaa] "r"(c2aaa),           // %[c2aaa]
        [c4000] "r"(c4000)            // %[c4000]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10");
}

void ScaleRowDown38_3_Box_LSX(const uint8_t* src_ptr,
                              ptrdiff_t src_stride,
                              uint8_t* dst_ptr,
                              int dst_width) {
  const uint8_t* ptr1 = src_ptr + src_stride;
  const uint8_t* ptr2 = ptr1 + src_stride;
  int count = dst_width / 12;
  int c1c71 = 0x1c71;
  int c2aaa = 0x2aaa;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vldi       $vr11, 0                             \n"
      "vld        $vr8, %[shuf], 0                     \n"
      "vreplgr2vr.h $vr9, %[c1c71]                     \n"
      "vreplgr2vr.w $vr10, %[c2aaa]                    \n"
      "1:                                              \n"
      "vld        $vr0, %[p0], 0                       \n"
      "vld        $vr1, %[p0], 16                      \n"
      "vld        $vr2, %[p1], 0                       \n"
      "vld        $vr3, %[p1], 16                      \n"
      "vaddwev.h.bu $vr4, $vr0, $vr2                   \n"
      "vaddwod.h.bu $vr5, $vr0, $vr2                   \n"
      "vaddwev.h.bu $vr6, $vr1, $vr3                   \n"
      "vaddwod.h.bu $vr7, $vr1, $vr3                   \n"
      "vld        $vr0, %[p2], 0                       \n"
      "vld        $vr1, %[p2], 16                      \n"
      "vpackev.b  $vr2, $vr11, $vr0                    \n"
      "vpackod.b  $vr3, $vr11, $vr0                    \n"
      "vadd.h     $vr4, $vr4, $vr2                     \n"
      "vadd.h     $vr5, $vr5, $vr3                     \n"
      "vpackev.b  $vr2, $vr11, $vr1                    \n"
      "vpackod.b  $vr3, $vr11, $vr1                    \n"
      "vadd.h     $vr6, $vr6, $vr2                     \n"
      "vadd.h     $vr7, $vr7, $vr3                     \n"
      "vpickev.h  $vr0, $vr6, $vr4                     \n"
      "vpickev.h  $vr1, $vr7, $vr5                     \n"
      "vpackod.h  $vr2, $vr5, $vr4                     \n"
      "vpackod.h  $vr3, $vr7, $vr6                     \n"
      "vpickev.w  $vr4, $vr3, $vr2                     \n"
      "vadd.h     $vr0, $vr0, $vr1                     \n"
      "vadd.h     $vr0, $vr0, $vr4                     \n"
      "vmuh.h     $vr0, $vr0, $vr9                     \n"
      "vpickod.w  $vr2, $vr3, $vr2                     \n"
      "vhaddw.wu.hu $vr2, $vr2, $vr2                   \n"
      "vmul.w     $vr2, $vr2, $vr10                    \n"
      "vshuf.b    $vr0, $vr2, $vr0, $vr8               \n"
      "vstelm.d   $vr0, %[dst], 0, 0                   \n"
      "vstelm.w   $vr0, %[dst], 8, 2                   \n"
      "addi.d     %[p0], %[p0], 32                     \n"
      "addi.d     %[p1], %[p1], 32                     \n"
      "addi.d     %[p2], %[p2], 32                     \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [p0] "+r"(src_ptr),           // %[p0]
        [p1] "+r"(ptr1),              // %[p1]
        [p2] "+r"(ptr2),              // %[p2]
        [dst] "+r"(dst_ptr),          // %[dst]
        [cnt] "+r"(count)             // %[cnt]
      : [shuf] "r"(kScale38BoxShuf),  // %[shuf]
        [c1c71] "r"(c1c71),           // %[c1c71]
        [c2aaa] "r"(c2aaa)            // %[c2aaa]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11");
}
#endif  // HAS_SCALEROWDOWN38_LSX

#ifdef HAS_SCALEADDROW_LSX
void ScaleAddRow_LSX(const uint8_t* src_ptr, uint16_t* dst_ptr, int src_width) {
  asm volatile(
      "srli.w     %[w], %[w], 4                        \n"
      "beqz       %[w], 2f                             \n"
      "vldi       $vr0, 0                              \n"
      "1:                                              \n"
      "vld        $vr1, %[src], 0                      \n"
      "vld        $vr2, %[dst], 0                      \n"
      "vld        $vr3, %[dst], 16                     \n"
      "vilvl.b    $vr4, $vr0, $vr1                     \n"
      "vilvh.b    $vr5, $vr0, $vr1                     \n"
      "vadd.h     $vr2, $vr2, $vr4                     \n"
      "vadd.h     $vr3, $vr3, $vr5                     \n"
      "vst        $vr2, %[dst], 0                      \n"
      "vst        $vr3, %[dst], 16                     \n"
      "addi.d     %[src], %[src], 16                   \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst_ptr),  // %[dst]
        [w] "+r"(src_width)   // %[w]
      :
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5");
}
#endif  // HAS_SCALEADDROW_LSX

#if defined(HAS_SCALEUVADDCOLS3_LSX) || defined(HAS_SCALEUVADDCOLS3_LASX)
static const uint8_t kScaleUVAddCols3Shuf[] = {
    0, 1, 2,  3,  12, 13, 14, 15, 24, 25, 26, 27, 4,  5,  6,  7,
    4, 5, 6,  7,  16, 17, 18, 19, 28, 29, 30, 31, 8,  9,  10, 11,
    8, 9, 10, 11, 20, 21, 22, 23, 0,  1,  2,  3,  12, 13, 14, 15,
    0, 1, 2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 20, 21, 22, 23,
    0, 1, 2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 24, 25, 26, 27,
    0, 1, 2,  3,  4,  5,  6,  7,  16, 17, 18, 19, 28, 29, 30, 31,
};
#endif

#ifdef HAS_SCALEUVADDCOLS3_LSX

void ScaleUVAddCols3_LSX(int dst_width,
                         int scaleval,
                         const uint16_t* src_uv,
                         uint8_t* dst_uv) {
  asm volatile(
      "vld        $vr6, %[shuf], 0                     \n"
      "vld        $vr7, %[shuf], 16                    \n"
      "vld        $vr8, %[shuf], 32                    \n"
      "vld        $vr9, %[shuf], 48                    \n"
      "vld        $vr10, %[shuf], 64                   \n"
      "vld        $vr11, %[shuf], 80                   \n"
      "vreplgr2vr.h $vr12, %[scaleval]                 \n"
      "1:                                              \n"
      "vld        $vr0, %[src_uv], 0                   \n"
      "vld        $vr1, %[src_uv], 16                  \n"
      "vld        $vr2, %[src_uv], 32                  \n"
      "vshuf.b    $vr3, $vr1, $vr0, $vr6               \n"
      "vshuf.b    $vr4, $vr1, $vr0, $vr7               \n"
      "vshuf.b    $vr5, $vr1, $vr0, $vr8               \n"
      "vshuf.b    $vr3, $vr2, $vr3, $vr9               \n"
      "vshuf.b    $vr4, $vr2, $vr4, $vr10              \n"
      "vshuf.b    $vr5, $vr2, $vr5, $vr11              \n"
      "vadd.h     $vr3, $vr3, $vr4                     \n"
      "vadd.h     $vr3, $vr3, $vr5                     \n"
      "vmuh.hu    $vr3, $vr3, $vr12                    \n"
      "vld        $vr0, %[src_uv], 48                  \n"
      "vld        $vr1, %[src_uv], 64                  \n"
      "vld        $vr2, %[src_uv], 80                  \n"
      "addi.d     %[src_uv], %[src_uv], 96             \n"
      "vshuf.b    $vr13, $vr1, $vr0, $vr6              \n"
      "vshuf.b    $vr4, $vr1, $vr0, $vr7               \n"
      "vshuf.b    $vr5, $vr1, $vr0, $vr8               \n"
      "vshuf.b    $vr13, $vr2, $vr13, $vr9             \n"
      "vshuf.b    $vr4, $vr2, $vr4, $vr10              \n"
      "vshuf.b    $vr5, $vr2, $vr5, $vr11              \n"
      "vadd.h     $vr13, $vr13, $vr4                   \n"
      "vadd.h     $vr13, $vr13, $vr5                   \n"
      "vmuh.hu    $vr13, $vr13, $vr12                  \n"
      "vpickev.b  $vr0, $vr13, $vr3                    \n"
      "vst        $vr0, %[dst_uv], 0                   \n"
      "addi.d     %[dst_uv], %[dst_uv], 16             \n"
      "addi.w     %[dst_width], %[dst_width], -8       \n"
      "bgtz       %[dst_width], 1b                     \n"
      : [src_uv] "+r"(src_uv),       // %[src_uv]
        [dst_uv] "+r"(dst_uv),       // %[dst_uv]
        [dst_width] "+r"(dst_width)  // %[dst_width]
      : [scaleval] "r"(scaleval),    // %[scaleval]
        [shuf] "r"(kScaleUVAddCols3Shuf)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9", "$vr10", "$vr11", "$vr12", "$vr13");
}
#endif  // HAS_SCALEUVADDCOLS3_LSX

#if defined(HAS_SCALEFILTERCOLS_LSX) || defined(HAS_SCALEARGBCOLS_LSX) || \
    defined(HAS_SCALEARGBFILTERCOLS_LSX)
static const uint64_t kRamp0123[2] = {0x0000000100000000ULL,
                                      0x0000000300000002ULL};
#endif

#ifdef HAS_SCALEFILTERCOLS_LSX
void ScaleFilterCols_LSX(uint8_t* dst_ptr,
                         const uint8_t* src_ptr,
                         int dst_width,
                         int x,
                         int dx) {
  ptrdiff_t i0, i1, i2, i3;
  uint32_t p0, p1;
  asm volatile(
      "srli.w     %[w], %[w], 2                        \n"
      "beqz       %[w], 2f                             \n"
      "vld        $vr0, %[ramp], 0                     \n"
      "vreplgr2vr.w $vr1, %[dx]                        \n"
      "vreplgr2vr.w $vr2, %[x]                         \n"
      "vmul.w     $vr0, $vr1, $vr0                     \n"
      "vslli.w    $vr1, $vr1, 2                        \n"
      "vadd.w     $vr2, $vr2, $vr0                     \n"
      "vldi       $vr3, 0x7f                           \n"
      "vsrli.w    $vr3, $vr3, 24                       \n"
      "vaddi.wu   $vr4, $vr3, 1                        \n"
      "1:                                              \n"
      "vsrai.w    $vr5, $vr2, 16                       \n"
      "vsrai.w    $vr6, $vr2, 9                        \n"
      "vand.v     $vr6, $vr6, $vr3                     \n"
      "vsub.w     $vr7, $vr4, $vr6                     \n"
      "vslli.w    $vr6, $vr6, 8                        \n"
      "vor.v      $vr6, $vr6, $vr7                     \n"
      "vadd.w     $vr2, $vr2, $vr1                     \n"
      "vpickve2gr.w %[i0], $vr5, 0                     \n"
      "vpickve2gr.w %[i1], $vr5, 1                     \n"
      "vpickve2gr.w %[i2], $vr5, 2                     \n"
      "vpickve2gr.w %[i3], $vr5, 3                     \n"
      "ldx.h      %[p0], %[src], %[i0]                 \n"
      "ldx.h      %[p1], %[src], %[i1]                 \n"
      "vinsgr2vr.h $vr5, %[p0], 0                      \n"
      "vinsgr2vr.h $vr5, %[p1], 2                      \n"
      "ldx.h      %[p0], %[src], %[i2]                 \n"
      "ldx.h      %[p1], %[src], %[i3]                 \n"
      "vinsgr2vr.h $vr5, %[p0], 4                      \n"
      "vinsgr2vr.h $vr5, %[p1], 6                      \n"
      "vmulwev.h.bu $vr7, $vr5, $vr6                  \n"
      "vmaddwod.h.bu $vr7, $vr5, $vr6                 \n"
      "vsrari.h   $vr5, $vr7, 7                        \n"
      "vpickev.b  $vr5, $vr5, $vr5                     \n"
      "vpickev.b  $vr5, $vr5, $vr5                     \n"
      "vstelm.w   $vr5, %[dst], 0, 0                   \n"
      "addi.d     %[dst], %[dst], 4                    \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [dst] "+r"(dst_ptr),   // %[dst]
        [w] "+r"(dst_width),   // %[w]
        [i0] "=&r"(i0),        // %[i0]
        [i1] "=&r"(i1),        // %[i1]
        [i2] "=&r"(i2),        // %[i2]
        [i3] "=&r"(i3),        // %[i3]
        [p0] "=&r"(p0),        // %[p0]
        [p1] "=&r"(p1)         // %[p1]
      : [src] "r"(src_ptr),    // %[src]
        [x] "r"(x),            // %[x]
        [dx] "r"(dx),          // %[dx]
        [ramp] "r"(kRamp0123)  // %[ramp]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7");
}
#endif  // HAS_SCALEFILTERCOLS_LSX

#ifdef HAS_SCALEARGBCOLS_LSX
void ScaleARGBCols_LSX(uint8_t* dst_argb,
                       const uint8_t* src_argb,
                       int dst_width,
                       int x,
                       int dx) {
  ptrdiff_t i0, i1, i2, i3;
  uint32_t p0, p1;
  asm volatile(
      "srli.w     %[w], %[w], 2                        \n"
      "beqz       %[w], 2f                             \n"
      "vld        $vr0, %[ramp], 0                     \n"
      "vreplgr2vr.w $vr1, %[dx]                        \n"
      "vreplgr2vr.w $vr2, %[x]                         \n"
      "vmul.w     $vr0, $vr1, $vr0                     \n"
      "vslli.w    $vr1, $vr1, 2                        \n"
      "vadd.w     $vr2, $vr2, $vr0                     \n"
      "1:                                              \n"
      "vsrai.w    $vr3, $vr2, 16                       \n"
      "vslli.w    $vr3, $vr3, 2                        \n"
      "vadd.w     $vr2, $vr2, $vr1                     \n"
      "vpickve2gr.w %[i0], $vr3, 0                     \n"
      "vpickve2gr.w %[i1], $vr3, 1                     \n"
      "vpickve2gr.w %[i2], $vr3, 2                     \n"
      "vpickve2gr.w %[i3], $vr3, 3                     \n"
      "ldx.w      %[p0], %[src], %[i0]                 \n"
      "ldx.w      %[p1], %[src], %[i1]                 \n"
      "st.w       %[p0], %[dst], 0                     \n"
      "st.w       %[p1], %[dst], 4                     \n"
      "ldx.w      %[p0], %[src], %[i2]                 \n"
      "ldx.w      %[p1], %[src], %[i3]                 \n"
      "st.w       %[p0], %[dst], 8                     \n"
      "st.w       %[p1], %[dst], 12                    \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width),   // %[w]
        [i0] "=&r"(i0),        // %[i0]
        [i1] "=&r"(i1),        // %[i1]
        [i2] "=&r"(i2),        // %[i2]
        [i3] "=&r"(i3),        // %[i3]
        [p0] "=&r"(p0),        // %[p0]
        [p1] "=&r"(p1)         // %[p1]
      : [src] "r"(src_argb),   // %[src]
        [x] "r"(x),            // %[x]
        [dx] "r"(dx),          // %[dx]
        [ramp] "r"(kRamp0123)  // %[ramp]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
}
#endif  // HAS_SCALEARGBCOLS_LSX

#ifdef HAS_SCALEARGBFILTERCOLS_LSX
void ScaleARGBFilterCols_LSX(uint8_t* dst_argb,
                             const uint8_t* src_argb,
                             int dst_width,
                             int x,
                             int dx) {
  ptrdiff_t i0, i1, i2, i3;
  uint64_t p0, p1;
  asm volatile(
      "srli.w     %[w], %[w], 2                        \n"
      "beqz       %[w], 2f                             \n"
      "vld        $vr0, %[ramp], 0                     \n"
      "vreplgr2vr.w $vr1, %[dx]                        \n"
      "vreplgr2vr.w $vr2, %[x]                         \n"
      "vmul.w     $vr0, $vr1, $vr0                     \n"
      "vslli.w    $vr1, $vr1, 2                        \n"
      "vadd.w     $vr2, $vr2, $vr0                     \n"
      "vldi       $vr3, 0x7f                           \n"
      "1:                                              \n"
      "vsrai.w    $vr4, $vr2, 16                       \n"
      "vslli.w    $vr4, $vr4, 2                        \n"
      "vsrai.w    $vr5, $vr2, 9                        \n"
      "vadd.w     $vr2, $vr2, $vr1                     \n"
      "vand.v     $vr5, $vr5, $vr3                     \n"
      "vshuf4i.b  $vr5, $vr5, 0x00                     \n"
      "vxor.v     $vr6, $vr5, $vr3                     \n"
      "vilvl.b    $vr7, $vr5, $vr6                     \n"
      "vilvh.b    $vr8, $vr5, $vr6                     \n"
      "vpickve2gr.w %[i0], $vr4, 0                     \n"
      "vpickve2gr.w %[i1], $vr4, 1                     \n"
      "vpickve2gr.w %[i2], $vr4, 2                     \n"
      "vpickve2gr.w %[i3], $vr4, 3                     \n"
      "ldx.d      %[p0], %[src], %[i0]                 \n"
      "ldx.d      %[p1], %[src], %[i1]                 \n"
      "vinsgr2vr.d $vr4, %[p0], 0                      \n"
      "vinsgr2vr.d $vr4, %[p1], 1                      \n"
      "ldx.d      %[p0], %[src], %[i2]                 \n"
      "ldx.d      %[p1], %[src], %[i3]                 \n"
      "vinsgr2vr.d $vr5, %[p0], 0                      \n"
      "vinsgr2vr.d $vr5, %[p1], 1                      \n"
      "vpickev.w  $vr6, $vr5, $vr4                     \n"
      "vpickod.w  $vr4, $vr5, $vr4                     \n"
      "vilvl.b    $vr5, $vr4, $vr6                     \n"
      "vilvh.b    $vr4, $vr4, $vr6                     \n"
      "vmulwev.h.bu $vr6, $vr5, $vr7                  \n"
      "vmaddwod.h.bu $vr6, $vr5, $vr7                 \n"
      "vmulwev.h.bu $vr7, $vr4, $vr8                  \n"
      "vmaddwod.h.bu $vr7, $vr4, $vr8                 \n"
      "vsrani.b.h $vr7, $vr6, 7                        \n"
      "vst        $vr7, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width),   // %[w]
        [i0] "=&r"(i0),        // %[i0]
        [i1] "=&r"(i1),        // %[i1]
        [i2] "=&r"(i2),        // %[i2]
        [i3] "=&r"(i3),        // %[i3]
        [p0] "=&r"(p0),        // %[p0]
        [p1] "=&r"(p1)         // %[p1]
      : [src] "r"(src_argb),   // %[src]
        [x] "r"(x),            // %[x]
        [dx] "r"(dx),          // %[dx]
        [ramp] "r"(kRamp0123)  // %[ramp]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8");
}
#endif  // HAS_SCALEARGBFILTERCOLS_LSX

#if defined(HAS_SCALEROWDOWN34_LSX) || defined(HAS_SCALEROWDOWN34_LASX)
static const uint64_t kScale34Shuf[2] = {0x0908070504030100ULL,
                                         0x000000000f0d0c0bULL};
static const uint64_t kScale34BoxLo[2] = {0x0504030202010100ULL,
                                          0x0000000007060605ULL};
static const uint64_t kScale34BoxHi[2] = {0x0d0c0b0a0a090908ULL,
                                          0x000000000f0e0e0dULL};
static const uint64_t kScale34BoxWt[2] = {0x0103030102020103ULL,
                                          0x0000000003010202ULL};
static const uint64_t kScale34Pack12[2] = {0x0908050403020100ULL,
                                           0x000000000d0c0b0aULL};
#endif

#ifdef HAS_SCALEROWDOWN34_LSX

void ScaleRowDown34_LSX(const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        uint8_t* dst,
                        int dst_width) {
  int count = dst_width / 12;
  (void)src_stride;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr1, %[shuf], 0                     \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "vshuf.b    $vr0, $vr0, $vr0, $vr1               \n"
      "vstelm.d   $vr0, %[dst], 0, 0                   \n"
      "vstelm.w   $vr0, %[dst], 8, 2                   \n"
      "addi.d     %[src], %[src], 16                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),      // %[src]
        [dst] "+r"(dst),          // %[dst]
        [cnt] "+r"(count)         // %[cnt]
      : [shuf] "r"(kScale34Shuf)  // %[shuf]
      : "memory", "$vr0", "$vr1");
}

void ScaleRowDown34_0_Box_LSX(const uint8_t* src_ptr,
                              ptrdiff_t src_stride,
                              uint8_t* d,
                              int dst_width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  int count = dst_width / 12;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr0, %[lo], 0                       \n"
      "vld        $vr1, %[hi], 0                       \n"
      "vld        $vr2, %[wt], 0                       \n"
      "vld        $vr3, %[pack], 0                     \n"
      "1:                                              \n"
      "vld        $vr4, %[src], 0                      \n"
      "vld        $vr5, %[nex], 0                      \n"
      "vshuf.b    $vr6, $vr4, $vr4, $vr0               \n"
      "vshuf.b    $vr7, $vr4, $vr4, $vr1               \n"
      "vshuf.b    $vr8, $vr5, $vr5, $vr0               \n"
      "vshuf.b    $vr9, $vr5, $vr5, $vr1               \n"
      "vmulwev.h.bu $vr4, $vr6, $vr2                  \n"
      "vmaddwod.h.bu $vr4, $vr6, $vr2                 \n"
      "vmulwev.h.bu $vr5, $vr7, $vr2                  \n"
      "vmaddwod.h.bu $vr5, $vr7, $vr2                 \n"
      "vmulwev.h.bu $vr6, $vr8, $vr2                  \n"
      "vmaddwod.h.bu $vr6, $vr8, $vr2                 \n"
      "vmulwev.h.bu $vr7, $vr9, $vr2                  \n"
      "vmaddwod.h.bu $vr7, $vr9, $vr2                 \n"
      "vsrari.h   $vr4, $vr4, 2                        \n"
      "vsrari.h   $vr5, $vr5, 2                        \n"
      "vsrari.h   $vr6, $vr6, 2                        \n"
      "vsrari.h   $vr7, $vr7, 2                        \n"
      "vslli.h    $vr8, $vr4, 1                        \n"
      "vslli.h    $vr9, $vr5, 1                        \n"
      "vadd.h     $vr4, $vr4, $vr8                     \n"
      "vadd.h     $vr5, $vr5, $vr9                     \n"
      "vadd.h     $vr4, $vr4, $vr6                     \n"
      "vadd.h     $vr5, $vr5, $vr7                     \n"
      "vsrarni.b.h $vr5, $vr4, 2                       \n"
      "vshuf.b    $vr5, $vr5, $vr5, $vr3               \n"
      "vstelm.d   $vr5, %[dst], 0, 0                   \n"
      "vstelm.w   $vr5, %[dst], 8, 2                   \n"
      "addi.d     %[src], %[src], 16                   \n"
      "addi.d     %[nex], %[nex], 16                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),        // %[src]
        [nex] "+r"(src_nex),        // %[nex]
        [dst] "+r"(d),              // %[dst]
        [cnt] "+r"(count)           // %[cnt]
      : [lo] "r"(kScale34BoxLo),    // %[lo]
        [hi] "r"(kScale34BoxHi),    // %[hi]
        [wt] "r"(kScale34BoxWt),    // %[wt]
        [pack] "r"(kScale34Pack12)  // %[pack]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9");
}

void ScaleRowDown34_1_Box_LSX(const uint8_t* src_ptr,
                              ptrdiff_t src_stride,
                              uint8_t* d,
                              int dst_width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  int count = dst_width / 12;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr0, %[lo], 0                       \n"
      "vld        $vr1, %[hi], 0                       \n"
      "vld        $vr2, %[wt], 0                       \n"
      "vld        $vr3, %[pack], 0                     \n"
      "1:                                              \n"
      "vld        $vr4, %[src], 0                      \n"
      "vld        $vr5, %[nex], 0                      \n"
      "vshuf.b    $vr6, $vr4, $vr4, $vr0               \n"
      "vshuf.b    $vr7, $vr4, $vr4, $vr1               \n"
      "vshuf.b    $vr8, $vr5, $vr5, $vr0               \n"
      "vshuf.b    $vr9, $vr5, $vr5, $vr1               \n"
      "vmulwev.h.bu $vr4, $vr6, $vr2                  \n"
      "vmaddwod.h.bu $vr4, $vr6, $vr2                 \n"
      "vmulwev.h.bu $vr5, $vr7, $vr2                  \n"
      "vmaddwod.h.bu $vr5, $vr7, $vr2                 \n"
      "vmulwev.h.bu $vr6, $vr8, $vr2                  \n"
      "vmaddwod.h.bu $vr6, $vr8, $vr2                 \n"
      "vmulwev.h.bu $vr7, $vr9, $vr2                  \n"
      "vmaddwod.h.bu $vr7, $vr9, $vr2                 \n"
      "vsrari.h   $vr4, $vr4, 2                        \n"
      "vsrari.h   $vr5, $vr5, 2                        \n"
      "vsrari.h   $vr6, $vr6, 2                        \n"
      "vsrari.h   $vr7, $vr7, 2                        \n"
      "vadd.h     $vr4, $vr4, $vr6                     \n"
      "vadd.h     $vr5, $vr5, $vr7                     \n"
      "vsrarni.b.h $vr5, $vr4, 1                       \n"
      "vshuf.b    $vr5, $vr5, $vr5, $vr3               \n"
      "vstelm.d   $vr5, %[dst], 0, 0                   \n"
      "vstelm.w   $vr5, %[dst], 8, 2                   \n"
      "addi.d     %[src], %[src], 16                   \n"
      "addi.d     %[nex], %[nex], 16                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),        // %[src]
        [nex] "+r"(src_nex),        // %[nex]
        [dst] "+r"(d),              // %[dst]
        [cnt] "+r"(count)           // %[cnt]
      : [lo] "r"(kScale34BoxLo),    // %[lo]
        [hi] "r"(kScale34BoxHi),    // %[hi]
        [wt] "r"(kScale34BoxWt),    // %[wt]
        [pack] "r"(kScale34Pack12)  // %[pack]
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3", "$vr4", "$vr5", "$vr6",
        "$vr7", "$vr8", "$vr9");
}
#endif  // HAS_SCALEROWDOWN34_LSX

#ifdef HAS_SCALEMAXSAMPLES_LSX
float ScaleMaxSamples_LSX(const float* src,
                          float* dst,
                          float scale,
                          int width) {
  float fmax = 0.f;
  asm volatile(
      "vldi       $vr3, 0                              \n"
      "vldrepl.w  $vr2, %[scale_ptr], 0                \n"
      "addi.w     %[w], %[w], -4                       \n"
      "bltz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "addi.d     %[src], %[src], 16                   \n"
      "vfmax.s    $vr3, $vr3, $vr0                     \n"
      "vfmul.s    $vr0, $vr0, $vr2                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -4                       \n"
      "bgez       %[w], 1b                             \n"
      "vshuf4i.w  $vr1, $vr3, 0x4e                     \n"
      "vfmax.s    $vr3, $vr3, $vr1                     \n"
      "vshuf4i.w  $vr1, $vr3, 0xb1                     \n"
      "vfmax.s    $vr3, $vr3, $vr1                     \n"
      "vstelm.w   $vr3, %[fmax_ptr], 0, 0              \n"
      "2:                                              \n"
      "addi.w     %[w], %[w], 4                        \n"
      : [src] "+r"(src),    // %[src]
        [dst] "+r"(dst),    // %[dst]
        [w] "+r"(width)     // %[w]
      : [scale_ptr] "r"(&scale),
        [fmax_ptr] "r"(&fmax)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
  for (; width > 0; --width) {
    float v = *src++;
    float vs = v * scale;
    fmax = (v > fmax) ? v : fmax;
    *dst++ = vs;
  }
  return fmax;
}
#endif  // HAS_SCALEMAXSAMPLES_LSX

#ifdef HAS_SCALESUMSAMPLES_LSX
float ScaleSumSamples_LSX(const float* src,
                          float* dst,
                          float scale,
                          int width) {
  float fsum = 0.f;
  asm volatile(
      "vldi       $vr3, 0                              \n"
      "vldrepl.w  $vr2, %[scale_ptr], 0                \n"
      "addi.w     %[w], %[w], -4                       \n"
      "bltz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "addi.d     %[src], %[src], 16                   \n"
      "vfmadd.s   $vr3, $vr0, $vr0, $vr3               \n"
      "vfmul.s    $vr0, $vr0, $vr2                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -4                       \n"
      "bgez       %[w], 1b                             \n"
      "vshuf4i.w  $vr1, $vr3, 0x4e                     \n"
      "vfadd.s    $vr3, $vr3, $vr1                     \n"
      "vshuf4i.w  $vr1, $vr3, 0xb1                     \n"
      "vfadd.s    $vr3, $vr3, $vr1                     \n"
      "vstelm.w   $vr3, %[fsum_ptr], 0, 0              \n"
      "2:                                              \n"
      "addi.w     %[w], %[w], 4                        \n"
      : [src] "+r"(src),    // %[src]
        [dst] "+r"(dst),    // %[dst]
        [w] "+r"(width)     // %[w]
      : [scale_ptr] "r"(&scale),
        [fsum_ptr] "r"(&fsum)
      : "memory", "$vr0", "$vr1", "$vr2", "$vr3");
  for (; width > 0; --width) {
    float v = *src++;
    fsum += v * v;
    *dst++ = v * scale;
  }
  return fsum;
}
#endif  // HAS_SCALESUMSAMPLES_LSX

#ifdef HAS_SCALESAMPLES_LSX
void ScaleSamples_LSX(const float* src, float* dst, float scale, int width) {
  asm volatile(
      "vldrepl.w  $vr2, %[scale_ptr], 0                \n"
      "addi.w     %[w], %[w], -4                       \n"
      "bltz       %[w], 2f                             \n"
      "1:                                              \n"
      "vld        $vr0, %[src], 0                      \n"
      "addi.d     %[src], %[src], 16                   \n"
      "vfmul.s    $vr0, $vr0, $vr2                     \n"
      "vst        $vr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -4                       \n"
      "bgez       %[w], 1b                             \n"
      "2:                                              \n"
      "addi.w     %[w], %[w], 4                        \n"
      : [src] "+r"(src),    // %[src]
        [dst] "+r"(dst),    // %[dst]
        [w] "+r"(width)     // %[w]
      : [scale_ptr] "r"(&scale)
      : "memory", "$vr0", "$vr2");
  if (width > 0) {
    ScaleSamples_C(src, dst, scale, width);
  }
}
#endif  // HAS_SCALESAMPLES_LSX

#ifdef HAS_SCALEARGBROWDOWN2_LASX
void ScaleARGBRowDown2_LASX(const uint8_t* src_argb,
                            ptrdiff_t src_stride,
                            uint8_t* dst_argb,
                            int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvld       $xr1, %[src], 32                     \n"
      "xvpickod.w $xr0, $xr1, $xr0                     \n"
      "xvpermi.d  $xr0, $xr0, 0xd8                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 64                   \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width)    // %[w]
      :
      : "memory", "$xr0", "$xr1");
}

void ScaleARGBRowDown2Linear_LASX(const uint8_t* src_argb,
                                  ptrdiff_t src_stride,
                                  uint8_t* dst_argb,
                                  int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvld       $xr1, %[src], 32                     \n"
      "xvpickev.w $xr2, $xr1, $xr0                     \n"
      "xvpickod.w $xr3, $xr1, $xr0                     \n"
      "xvavgr.bu  $xr0, $xr3, $xr2                     \n"
      "xvpermi.d  $xr0, $xr0, 0xd8                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 64                   \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width)    // %[w]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}

void ScaleARGBRowDown2Box_LASX(const uint8_t* src_argb,
                               ptrdiff_t src_stride,
                               uint8_t* dst_argb,
                               int dst_width) {
  const uint8_t* t = src_argb + src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[s], 0                        \n"
      "xvld       $xr1, %[s], 32                       \n"
      "xvld       $xr2, %[t], 0                        \n"
      "xvld       $xr3, %[t], 32                       \n"
      "xvpickev.w $xr4, $xr1, $xr0                     \n"
      "xvpickod.w $xr5, $xr1, $xr0                     \n"
      "xvpickev.w $xr6, $xr3, $xr2                     \n"
      "xvpickod.w $xr7, $xr3, $xr2                     \n"
      "xvaddwev.h.bu $xr0, $xr4, $xr5                  \n"
      "xvaddwod.h.bu $xr1, $xr4, $xr5                  \n"
      "xvaddwev.h.bu $xr2, $xr6, $xr7                  \n"
      "xvaddwod.h.bu $xr3, $xr6, $xr7                  \n"
      "xvadd.h    $xr0, $xr0, $xr2                     \n"
      "xvadd.h    $xr1, $xr1, $xr3                     \n"
      "xvsrari.h  $xr0, $xr0, 2                        \n"
      "xvsrari.h  $xr1, $xr1, 2                        \n"
      "xvpackev.b $xr0, $xr1, $xr0                     \n"
      "xvpermi.d  $xr0, $xr0, 0xd8                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[s], %[s], 64                       \n"
      "addi.d     %[t], %[t], 64                       \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [s] "+r"(src_argb),    // %[s]
        [t] "+r"(t),           // %[t]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width)    // %[w]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_SCALEARGBROWDOWN2_LASX

#ifdef HAS_SCALEARGBROWDOWNEVEN_LASX
void ScaleARGBRowDownEven_LASX(const uint8_t* src_argb,
                               ptrdiff_t src_stride,
                               int32_t src_stepx,
                               uint8_t* dst_argb,
                               int dst_width) {
  const ptrdiff_t step = (ptrdiff_t)src_stepx * 4;
  uint32_t r0, r1, r2, r3;
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "ld.w       %[r0], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r1], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r2], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r3], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "xvinsgr2vr.w $xr0, %[r0], 0                     \n"
      "xvinsgr2vr.w $xr0, %[r1], 1                     \n"
      "xvinsgr2vr.w $xr0, %[r2], 2                     \n"
      "xvinsgr2vr.w $xr0, %[r3], 3                     \n"
      "ld.w       %[r0], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r1], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r2], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "ld.w       %[r3], %[src], 0                     \n"
      "add.d      %[src], %[src], %[step]              \n"
      "xvinsgr2vr.w $xr0, %[r0], 4                     \n"
      "xvinsgr2vr.w $xr0, %[r1], 5                     \n"
      "xvinsgr2vr.w $xr0, %[r2], 6                     \n"
      "xvinsgr2vr.w $xr0, %[r3], 7                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_argb),  // %[src]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width),   // %[w]
        [r0] "=&r"(r0),        // %[r0]
        [r1] "=&r"(r1),        // %[r1]
        [r2] "=&r"(r2),        // %[r2]
        [r3] "=&r"(r3)         // %[r3]
      : [step] "r"(step)       // %[step]
      : "memory", "$xr0");
}

void ScaleARGBRowDownEvenBox_LASX(const uint8_t* src_argb,
                                  ptrdiff_t src_stride,
                                  int src_stepx,
                                  uint8_t* dst_argb,
                                  int dst_width) {
  const ptrdiff_t step = (ptrdiff_t)src_stepx * 4;
  const uint8_t* next_argb = src_argb + src_stride;
  uint64_t r0, r1;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "ld.d       %[r0], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "ld.d       %[r1], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "xvinsgr2vr.d $xr0, %[r0], 0                     \n"
      "xvinsgr2vr.d $xr0, %[r1], 1                     \n"
      "ld.d       %[r0], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "ld.d       %[r1], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "xvinsgr2vr.d $xr1, %[r0], 0                     \n"
      "xvinsgr2vr.d $xr1, %[r1], 1                     \n"
      "ld.d       %[r0], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "ld.d       %[r1], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "xvinsgr2vr.d $xr0, %[r0], 2                     \n"
      "xvinsgr2vr.d $xr0, %[r1], 3                     \n"
      "ld.d       %[r0], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "ld.d       %[r1], %[s], 0                       \n"
      "add.d      %[s], %[s], %[step]                  \n"
      "xvinsgr2vr.d $xr1, %[r0], 2                     \n"
      "xvinsgr2vr.d $xr1, %[r1], 3                     \n"

      "ld.d       %[r0], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "ld.d       %[r1], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "xvinsgr2vr.d $xr2, %[r0], 0                     \n"
      "xvinsgr2vr.d $xr2, %[r1], 1                     \n"
      "ld.d       %[r0], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "ld.d       %[r1], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "xvinsgr2vr.d $xr3, %[r0], 0                     \n"
      "xvinsgr2vr.d $xr3, %[r1], 1                     \n"
      "ld.d       %[r0], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "ld.d       %[r1], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "xvinsgr2vr.d $xr2, %[r0], 2                     \n"
      "xvinsgr2vr.d $xr2, %[r1], 3                     \n"
      "ld.d       %[r0], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "ld.d       %[r1], %[t], 0                       \n"
      "add.d      %[t], %[t], %[step]                  \n"
      "xvinsgr2vr.d $xr3, %[r0], 2                     \n"
      "xvinsgr2vr.d $xr3, %[r1], 3                     \n"

      "xvpickev.w $xr4, $xr1, $xr0                     \n"
      "xvpickod.w $xr5, $xr1, $xr0                     \n"
      "xvpickev.w $xr6, $xr3, $xr2                     \n"
      "xvpickod.w $xr7, $xr3, $xr2                     \n"
      "xvaddwev.h.bu $xr0, $xr4, $xr5                  \n"
      "xvaddwod.h.bu $xr1, $xr4, $xr5                  \n"
      "xvaddwev.h.bu $xr2, $xr6, $xr7                  \n"
      "xvaddwod.h.bu $xr3, $xr6, $xr7                  \n"
      "xvadd.h    $xr0, $xr0, $xr2                     \n"
      "xvadd.h    $xr1, $xr1, $xr3                     \n"
      "xvsrari.h  $xr0, $xr0, 2                        \n"
      "xvsrari.h  $xr1, $xr1, 2                        \n"
      "xvpackev.b $xr0, $xr1, $xr0                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [s] "+r"(src_argb),    // %[s]
        [t] "+r"(next_argb),   // %[t]
        [dst] "+r"(dst_argb),  // %[dst]
        [w] "+r"(dst_width),   // %[w]
        [r0] "=&r"(r0),        // %[r0]
        [r1] "=&r"(r1)         // %[r1]
      : [step] "r"(step)       // %[step]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_SCALEARGBROWDOWNEVEN_LASX

#ifdef HAS_SCALEROWDOWN2_LASX
void ScaleRowDown2_LASX(const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        uint8_t* dst,
                        int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 5                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvld       $xr1, %[src], 32                     \n"
      "xvpickod.b $xr0, $xr1, $xr0                     \n"
      "xvpermi.d  $xr0, $xr0, 0xd8                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 64                   \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(dst_width)   // %[w]
      :
      : "memory", "$xr0", "$xr1");
}

void ScaleRowDown2Linear_LASX(const uint8_t* src_ptr,
                              ptrdiff_t src_stride,
                              uint8_t* dst,
                              int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 5                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvld       $xr1, %[src], 32                     \n"
      "xvpickev.b $xr2, $xr1, $xr0                     \n"
      "xvpickod.b $xr3, $xr1, $xr0                     \n"
      "xvavgr.bu  $xr0, $xr3, $xr2                     \n"
      "xvpermi.d  $xr0, $xr0, 0xd8                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 64                   \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(dst_width)   // %[w]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}

void ScaleRowDown2Box_LASX(const uint8_t* src_ptr,
                           ptrdiff_t src_stride,
                           uint8_t* dst,
                           int dst_width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 5                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvld       $xr1, %[src], 32                     \n"
      "xvld       $xr2, %[nex], 0                      \n"
      "xvld       $xr3, %[nex], 32                     \n"
      "xvaddwev.h.bu $xr4, $xr0, $xr2                  \n"
      "xvaddwod.h.bu $xr5, $xr0, $xr2                  \n"
      "xvaddwev.h.bu $xr6, $xr1, $xr3                  \n"
      "xvaddwod.h.bu $xr7, $xr1, $xr3                  \n"
      "xvadd.h    $xr4, $xr4, $xr5                     \n"
      "xvadd.h    $xr6, $xr6, $xr7                     \n"
      "xvsrarni.b.h $xr6, $xr4, 2                      \n"
      "xvpermi.d  $xr6, $xr6, 0xd8                     \n"
      "xvst       $xr6, %[dst], 0                      \n"
      "addi.d     %[src], %[src], 64                   \n"
      "addi.d     %[nex], %[nex], 64                   \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [nex] "+r"(src_nex),  // %[nex]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(dst_width)   // %[w]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_SCALEROWDOWN2_LASX

#ifdef HAS_SCALEROWDOWN4_LASX
void ScaleRowDown4_LASX(const uint8_t* src_ptr,
                        ptrdiff_t src_stride,
                        uint8_t* dst,
                        int dst_width) {
  (void)src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 4                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvld       $xr1, %[src], 32                     \n"
      "xvpickev.b $xr0, $xr1, $xr0                     \n"
      "xvpermi.d  $xr0, $xr0, 0xd8                     \n"
      "xvpickod.b $xr0, $xr0, $xr0                     \n"
      "xvstelm.d  $xr0, %[dst], 0, 0                   \n"
      "xvstelm.d  $xr0, %[dst], 8, 2                   \n"
      "addi.d     %[src], %[src], 64                   \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst),      // %[dst]
        [w] "+r"(dst_width)   // %[w]
      :
      : "memory", "$xr0", "$xr1");
}

void ScaleRowDown4Box_LASX(const uint8_t* src_ptr,
                           ptrdiff_t src_stride,
                           uint8_t* dst,
                           int dst_width) {
  const uint8_t* ptr1 = src_ptr + src_stride;
  const uint8_t* ptr2 = ptr1 + src_stride;
  const uint8_t* ptr3 = ptr2 + src_stride;
  asm volatile(
      "srli.w     %[w], %[w], 4                        \n"
      "beqz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[p0], 0                       \n"
      "xvld       $xr1, %[p0], 32                      \n"
      "xvld       $xr2, %[p1], 0                       \n"
      "xvld       $xr3, %[p1], 32                      \n"
      "xvld       $xr4, %[p2], 0                       \n"
      "xvld       $xr5, %[p2], 32                      \n"
      "xvld       $xr6, %[p3], 0                       \n"
      "xvld       $xr7, %[p3], 32                      \n"
      "xvhaddw.hu.bu $xr0, $xr0, $xr0                  \n"
      "xvhaddw.hu.bu $xr1, $xr1, $xr1                  \n"
      "xvhaddw.hu.bu $xr2, $xr2, $xr2                  \n"
      "xvhaddw.hu.bu $xr3, $xr3, $xr3                  \n"
      "xvhaddw.hu.bu $xr4, $xr4, $xr4                  \n"
      "xvhaddw.hu.bu $xr5, $xr5, $xr5                  \n"
      "xvhaddw.hu.bu $xr6, $xr6, $xr6                  \n"
      "xvhaddw.hu.bu $xr7, $xr7, $xr7                  \n"
      "xvadd.h    $xr0, $xr0, $xr2                     \n"
      "xvadd.h    $xr1, $xr1, $xr3                     \n"
      "xvadd.h    $xr4, $xr4, $xr6                     \n"
      "xvadd.h    $xr5, $xr5, $xr7                     \n"
      "xvadd.h    $xr0, $xr0, $xr4                     \n"
      "xvadd.h    $xr1, $xr1, $xr5                     \n"
      "xvhaddw.wu.hu $xr0, $xr0, $xr0                  \n"
      "xvhaddw.wu.hu $xr1, $xr1, $xr1                  \n"
      "xvsrarni.h.w $xr1, $xr0, 4                      \n"
      "xvpermi.d  $xr1, $xr1, 0xd8                     \n"
      "xvsrlni.b.h $xr1, $xr1, 0                       \n"
      "xvstelm.d  $xr1, %[dst], 0, 0                   \n"
      "xvstelm.d  $xr1, %[dst], 8, 2                   \n"
      "addi.d     %[p0], %[p0], 64                     \n"
      "addi.d     %[p1], %[p1], 64                     \n"
      "addi.d     %[p2], %[p2], 64                     \n"
      "addi.d     %[p3], %[p3], 64                     \n"
      "addi.d     %[dst], %[dst], 16                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [p0] "+r"(src_ptr),  // %[p0]
        [p1] "+r"(ptr1),     // %[p1]
        [p2] "+r"(ptr2),     // %[p2]
        [p3] "+r"(ptr3),     // %[p3]
        [dst] "+r"(dst),     // %[dst]
        [w] "+r"(dst_width)  // %[w]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_SCALEROWDOWN4_LASX

#ifdef HAS_SCALEROWDOWN38_LASX
void ScaleRowDown38_LASX(const uint8_t* src_ptr,
                         ptrdiff_t src_stride,
                         uint8_t* dst,
                         int dst_width) {
  int count = dst_width / 12;
  (void)src_stride;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr2, %[shuf], 0                     \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvpermi.d  $xr1, $xr0, 0x4e                     \n"
      "vshuf.b    $vr0, $vr1, $vr0, $vr2               \n"
      "vstelm.d   $vr0, %[dst], 0, 0                   \n"
      "vstelm.w   $vr0, %[dst], 8, 2                   \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),      // %[src]
        [dst] "+r"(dst),          // %[dst]
        [cnt] "+r"(count)         // %[cnt]
      : [shuf] "r"(kScale38Shuf)  // %[shuf]
      : "memory", "$xr0", "$xr1", "$xr2");
}

void ScaleRowDown38_2_Box_LASX(const uint8_t* src_ptr,
                               ptrdiff_t src_stride,
                               uint8_t* dst_ptr,
                               int dst_width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  int count = dst_width / 12;
  int c2aaa = 0x2aaa;
  int c4000 = 0x4000;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr8, %[shuf], 0                     \n"
      "vreplgr2vr.h $vr9, %[c2aaa]                     \n"
      "vreplgr2vr.w $vr10, %[c4000]                    \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvld       $xr2, %[nex], 0                      \n"
      "xvaddwev.h.bu $xr4, $xr0, $xr2                  \n"
      "xvaddwod.h.bu $xr5, $xr0, $xr2                  \n"
      "xvpermi.d  $xr6, $xr4, 0x4e                     \n"
      "xvpermi.d  $xr7, $xr5, 0x4e                     \n"
      "vpickev.h  $vr0, $vr6, $vr4                     \n"
      "vpickev.h  $vr1, $vr7, $vr5                     \n"
      "vpackod.h  $vr2, $vr5, $vr4                     \n"
      "vpackod.h  $vr3, $vr7, $vr6                     \n"
      "vpickev.w  $vr4, $vr3, $vr2                     \n"
      "vadd.h     $vr0, $vr0, $vr1                     \n"
      "vadd.h     $vr0, $vr0, $vr4                     \n"
      "vmuh.h     $vr0, $vr0, $vr9                     \n"
      "vpickod.w  $vr2, $vr3, $vr2                     \n"
      "vhaddw.wu.hu $vr2, $vr2, $vr2                   \n"
      "vmul.w     $vr2, $vr2, $vr10                    \n"
      "vshuf.b    $vr0, $vr2, $vr0, $vr8               \n"
      "vstelm.d   $vr0, %[dst], 0, 0                   \n"
      "vstelm.w   $vr0, %[dst], 8, 2                   \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[nex], %[nex], 32                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),          // %[src]
        [nex] "+r"(src_nex),          // %[nex]
        [dst] "+r"(dst_ptr),          // %[dst]
        [cnt] "+r"(count)             // %[cnt]
      : [shuf] "r"(kScale38BoxShuf),  // %[shuf]
        [c2aaa] "r"(c2aaa),           // %[c2aaa]
        [c4000] "r"(c4000)            // %[c4000]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10");
}

void ScaleRowDown38_3_Box_LASX(const uint8_t* src_ptr,
                               ptrdiff_t src_stride,
                               uint8_t* dst_ptr,
                               int dst_width) {
  const uint8_t* ptr1 = src_ptr + src_stride;
  const uint8_t* ptr2 = ptr1 + src_stride;
  int count = dst_width / 12;
  int c1c71 = 0x1c71;
  int c2aaa = 0x2aaa;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "xvldi      $xr11, 0                             \n"
      "vld        $vr8, %[shuf], 0                     \n"
      "vreplgr2vr.h $vr9, %[c1c71]                     \n"
      "vreplgr2vr.w $vr10, %[c2aaa]                    \n"
      "1:                                              \n"
      "xvld       $xr0, %[p0], 0                       \n"
      "xvld       $xr2, %[p1], 0                       \n"
      "xvaddwev.h.bu $xr4, $xr0, $xr2                  \n"
      "xvaddwod.h.bu $xr5, $xr0, $xr2                  \n"
      "xvld       $xr0, %[p2], 0                       \n"
      "xvpackev.b $xr2, $xr11, $xr0                    \n"
      "xvpackod.b $xr3, $xr11, $xr0                    \n"
      "xvadd.h    $xr4, $xr4, $xr2                     \n"
      "xvadd.h    $xr5, $xr5, $xr3                     \n"
      "xvpermi.d  $xr6, $xr4, 0x4e                     \n"
      "xvpermi.d  $xr7, $xr5, 0x4e                     \n"
      "vpickev.h  $vr0, $vr6, $vr4                     \n"
      "vpickev.h  $vr1, $vr7, $vr5                     \n"
      "vpackod.h  $vr2, $vr5, $vr4                     \n"
      "vpackod.h  $vr3, $vr7, $vr6                     \n"
      "vpickev.w  $vr4, $vr3, $vr2                     \n"
      "vadd.h     $vr0, $vr0, $vr1                     \n"
      "vadd.h     $vr0, $vr0, $vr4                     \n"
      "vmuh.h     $vr0, $vr0, $vr9                     \n"
      "vpickod.w  $vr2, $vr3, $vr2                     \n"
      "vhaddw.wu.hu $vr2, $vr2, $vr2                   \n"
      "vmul.w     $vr2, $vr2, $vr10                    \n"
      "vshuf.b    $vr0, $vr2, $vr0, $vr8               \n"
      "vstelm.d   $vr0, %[dst], 0, 0                   \n"
      "vstelm.w   $vr0, %[dst], 8, 2                   \n"
      "addi.d     %[p0], %[p0], 32                     \n"
      "addi.d     %[p1], %[p1], 32                     \n"
      "addi.d     %[p2], %[p2], 32                     \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [p0] "+r"(src_ptr),           // %[p0]
        [p1] "+r"(ptr1),              // %[p1]
        [p2] "+r"(ptr2),              // %[p2]
        [dst] "+r"(dst_ptr),          // %[dst]
        [cnt] "+r"(count)             // %[cnt]
      : [shuf] "r"(kScale38BoxShuf),  // %[shuf]
        [c1c71] "r"(c1c71),           // %[c1c71]
        [c2aaa] "r"(c2aaa)            // %[c2aaa]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11");
}
#endif  // HAS_SCALEROWDOWN38_LASX

#ifdef HAS_SCALEADDROW_LASX
void ScaleAddRow_LASX(const uint8_t* src_ptr,
                      uint16_t* dst_ptr,
                      int src_width) {
  asm volatile(
      "srli.w     %[w], %[w], 5                        \n"
      "beqz       %[w], 2f                             \n"
      "xvldi      $xr0, 0                              \n"
      "1:                                              \n"
      "xvld       $xr1, %[src], 0                      \n"
      "xvld       $xr2, %[dst], 0                      \n"
      "xvld       $xr3, %[dst], 32                     \n"
      "xvpermi.d  $xr1, $xr1, 0xd8                     \n"
      "xvilvl.b   $xr4, $xr0, $xr1                     \n"
      "xvilvh.b   $xr5, $xr0, $xr1                     \n"
      "xvadd.h    $xr2, $xr2, $xr4                     \n"
      "xvadd.h    $xr3, $xr3, $xr5                     \n"
      "xvst       $xr2, %[dst], 0                      \n"
      "xvst       $xr3, %[dst], 32                     \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 64                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),  // %[src]
        [dst] "+r"(dst_ptr),  // %[dst]
        [w] "+r"(src_width)   // %[w]
      :
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5");
}
#endif  // HAS_SCALEADDROW_LASX

#ifdef HAS_SCALEUVADDCOLS3_LASX
void ScaleUVAddCols3_LASX(int dst_width,
                          int scaleval,
                          const uint16_t* src_uv,
                          uint8_t* dst_uv) {
  asm volatile(
      "vld        $vr6, %[shuf], 0                     \n"
      "vld        $vr7, %[shuf], 16                    \n"
      "vld        $vr8, %[shuf], 32                    \n"
      "vld        $vr9, %[shuf], 48                    \n"
      "vld        $vr10, %[shuf], 64                   \n"
      "vld        $vr11, %[shuf], 80                   \n"
      "xvpermi.d  $xr6, $xr6, 0x44                     \n"
      "xvpermi.d  $xr7, $xr7, 0x44                     \n"
      "xvpermi.d  $xr8, $xr8, 0x44                     \n"
      "xvpermi.d  $xr9, $xr9, 0x44                     \n"
      "xvpermi.d  $xr10, $xr10, 0x44                   \n"
      "xvpermi.d  $xr11, $xr11, 0x44                   \n"
      "xvreplgr2vr.h $xr12, %[scaleval]                \n"
      "1:                                              \n"
      "vld        $vr0, %[src_uv], 0                   \n"
      "vld        $vr1, %[src_uv], 16                  \n"
      "vld        $vr2, %[src_uv], 32                  \n"
      "vld        $vr3, %[src_uv], 48                  \n"
      "vld        $vr4, %[src_uv], 64                  \n"
      "vld        $vr5, %[src_uv], 80                  \n"
      "addi.d     %[src_uv], %[src_uv], 96             \n"
      "xvpermi.q  $xr0, $xr3, 0x02                     \n"
      "xvpermi.q  $xr1, $xr4, 0x02                     \n"
      "xvpermi.q  $xr2, $xr5, 0x02                     \n"
      "xvshuf.b   $xr3, $xr1, $xr0, $xr6               \n"
      "xvshuf.b   $xr4, $xr1, $xr0, $xr7               \n"
      "xvshuf.b   $xr5, $xr1, $xr0, $xr8               \n"
      "xvshuf.b   $xr3, $xr2, $xr3, $xr9               \n"
      "xvshuf.b   $xr4, $xr2, $xr4, $xr10              \n"
      "xvshuf.b   $xr5, $xr2, $xr5, $xr11              \n"
      "xvadd.h    $xr3, $xr3, $xr4                     \n"
      "xvadd.h    $xr3, $xr3, $xr5                     \n"
      "xvmuh.hu   $xr3, $xr3, $xr12                    \n"
      "xvpermi.d  $xr4, $xr3, 0x4e                     \n"
      "vpickev.b  $vr0, $vr4, $vr3                     \n"
      "vst        $vr0, %[dst_uv], 0                   \n"
      "addi.d     %[dst_uv], %[dst_uv], 16             \n"
      "addi.w     %[dst_width], %[dst_width], -8       \n"
      "bgtz       %[dst_width], 1b                     \n"
      : [src_uv] "+r"(src_uv),       // %[src_uv]
        [dst_uv] "+r"(dst_uv),       // %[dst_uv]
        [dst_width] "+r"(dst_width)  // %[dst_width]
      : [scaleval] "r"(scaleval),    // %[scaleval]
        [shuf] "r"(kScaleUVAddCols3Shuf)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9", "$xr10", "$xr11", "$xr12");
}
#endif  // HAS_SCALEUVADDCOLS3_LASX

#if defined(HAS_SCALEFILTERCOLS_LASX) || defined(HAS_SCALEARGBCOLS_LASX) || \
    defined(HAS_SCALEARGBFILTERCOLS_LASX)
static const uint64_t kRamp01234567[4] = {
    0x0000000100000000ULL, 0x0000000300000002ULL, 0x0000000500000004ULL,
    0x0000000700000006ULL};
#endif

#ifdef HAS_SCALEFILTERCOLS_LASX
void ScaleFilterCols_LASX(uint8_t* dst_ptr,
                          const uint8_t* src_ptr,
                          int dst_width,
                          int x,
                          int dx) {
  ptrdiff_t i0, i1, i2, i3;
  uint32_t p0, p1;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "xvld       $xr0, %[ramp], 0                     \n"
      "xvreplgr2vr.w $xr1, %[dx]                       \n"
      "xvreplgr2vr.w $xr2, %[x]                        \n"
      "xvmul.w    $xr0, $xr1, $xr0                     \n"
      "xvslli.w   $xr1, $xr1, 3                        \n"
      "xvadd.w    $xr2, $xr2, $xr0                     \n"
      "xvldi      $xr3, 0x7f                           \n"
      "xvsrli.w   $xr3, $xr3, 24                       \n"
      "xvaddi.wu  $xr4, $xr3, 1                        \n"
      "1:                                              \n"
      "xvsrai.w   $xr5, $xr2, 16                       \n"
      "xvsrai.w   $xr6, $xr2, 9                        \n"
      "xvand.v    $xr6, $xr6, $xr3                     \n"
      "xvsub.w    $xr7, $xr4, $xr6                     \n"
      "xvslli.w   $xr6, $xr6, 8                        \n"
      "xvor.v     $xr6, $xr6, $xr7                     \n"
      "xvadd.w    $xr2, $xr2, $xr1                     \n"
      "xvpickve2gr.w %[i0], $xr5, 0                    \n"
      "xvpickve2gr.w %[i1], $xr5, 1                    \n"
      "xvpickve2gr.w %[i2], $xr5, 2                    \n"
      "xvpickve2gr.w %[i3], $xr5, 3                    \n"
      "ldx.h      %[p0], %[src], %[i0]                 \n"
      "ldx.h      %[p1], %[src], %[i1]                 \n"
      "xvinsgr2vr.w $xr0, %[p0], 0                     \n"
      "xvinsgr2vr.w $xr0, %[p1], 1                     \n"
      "ldx.h      %[p0], %[src], %[i2]                 \n"
      "ldx.h      %[p1], %[src], %[i3]                 \n"
      "xvinsgr2vr.w $xr0, %[p0], 2                     \n"
      "xvinsgr2vr.w $xr0, %[p1], 3                     \n"
      "xvpickve2gr.w %[i0], $xr5, 4                    \n"
      "xvpickve2gr.w %[i1], $xr5, 5                    \n"
      "xvpickve2gr.w %[i2], $xr5, 6                    \n"
      "xvpickve2gr.w %[i3], $xr5, 7                    \n"
      "ldx.h      %[p0], %[src], %[i0]                 \n"
      "ldx.h      %[p1], %[src], %[i1]                 \n"
      "xvinsgr2vr.w $xr0, %[p0], 4                     \n"
      "xvinsgr2vr.w $xr0, %[p1], 5                     \n"
      "ldx.h      %[p0], %[src], %[i2]                 \n"
      "ldx.h      %[p1], %[src], %[i3]                 \n"
      "xvinsgr2vr.w $xr0, %[p0], 6                     \n"
      "xvinsgr2vr.w $xr0, %[p1], 7                     \n"
      "xvmulwev.h.bu $xr7, $xr0, $xr6                  \n"
      "xvmaddwod.h.bu $xr7, $xr0, $xr6                 \n"
      "xvsrari.h  $xr5, $xr7, 7                        \n"
      "xvpickev.b $xr5, $xr5, $xr5                     \n"
      "xvpickev.b $xr5, $xr5, $xr5                     \n"
      "xvstelm.w  $xr5, %[dst], 0, 0                   \n"
      "xvstelm.w  $xr5, %[dst], 4, 4                   \n"
      "addi.d     %[dst], %[dst], 8                    \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [dst] "+r"(dst_ptr),       // %[dst]
        [w] "+r"(dst_width),       // %[w]
        [i0] "=&r"(i0),            // %[i0]
        [i1] "=&r"(i1),            // %[i1]
        [i2] "=&r"(i2),            // %[i2]
        [i3] "=&r"(i3),            // %[i3]
        [p0] "=&r"(p0),            // %[p0]
        [p1] "=&r"(p1)             // %[p1]
      : [src] "r"(src_ptr),        // %[src]
        [x] "r"(x),                // %[x]
        [dx] "r"(dx),              // %[dx]
        [ramp] "r"(kRamp01234567)  // %[ramp]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7");
}
#endif  // HAS_SCALEFILTERCOLS_LASX

#ifdef HAS_SCALEARGBCOLS_LASX
void ScaleARGBCols_LASX(uint8_t* dst_argb,
                        const uint8_t* src_argb,
                        int dst_width,
                        int x,
                        int dx) {
  ptrdiff_t i0, i1, i2, i3;
  uint32_t p0, p1;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "xvld       $xr0, %[ramp], 0                     \n"
      "xvreplgr2vr.w $xr1, %[dx]                       \n"
      "xvreplgr2vr.w $xr2, %[x]                        \n"
      "xvmul.w    $xr0, $xr1, $xr0                     \n"
      "xvslli.w   $xr1, $xr1, 3                        \n"
      "xvadd.w    $xr2, $xr2, $xr0                     \n"
      "1:                                              \n"
      "xvsrai.w   $xr3, $xr2, 16                       \n"
      "xvslli.w   $xr3, $xr3, 2                        \n"
      "xvadd.w    $xr2, $xr2, $xr1                     \n"
      "xvpickve2gr.w %[i0], $xr3, 0                    \n"
      "xvpickve2gr.w %[i1], $xr3, 1                    \n"
      "xvpickve2gr.w %[i2], $xr3, 2                    \n"
      "xvpickve2gr.w %[i3], $xr3, 3                    \n"
      "ldx.w      %[p0], %[src], %[i0]                 \n"
      "ldx.w      %[p1], %[src], %[i1]                 \n"
      "xvinsgr2vr.w $xr0, %[p0], 0                     \n"
      "xvinsgr2vr.w $xr0, %[p1], 1                     \n"
      "ldx.w      %[p0], %[src], %[i2]                 \n"
      "ldx.w      %[p1], %[src], %[i3]                 \n"
      "xvinsgr2vr.w $xr0, %[p0], 2                     \n"
      "xvinsgr2vr.w $xr0, %[p1], 3                     \n"
      "xvpickve2gr.w %[i0], $xr3, 4                    \n"
      "xvpickve2gr.w %[i1], $xr3, 5                    \n"
      "xvpickve2gr.w %[i2], $xr3, 6                    \n"
      "xvpickve2gr.w %[i3], $xr3, 7                    \n"
      "ldx.w      %[p0], %[src], %[i0]                 \n"
      "ldx.w      %[p1], %[src], %[i1]                 \n"
      "xvinsgr2vr.w $xr0, %[p0], 4                     \n"
      "xvinsgr2vr.w $xr0, %[p1], 5                     \n"
      "ldx.w      %[p0], %[src], %[i2]                 \n"
      "ldx.w      %[p1], %[src], %[i3]                 \n"
      "xvinsgr2vr.w $xr0, %[p0], 6                     \n"
      "xvinsgr2vr.w $xr0, %[p1], 7                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [dst] "+r"(dst_argb),      // %[dst]
        [w] "+r"(dst_width),       // %[w]
        [i0] "=&r"(i0),            // %[i0]
        [i1] "=&r"(i1),            // %[i1]
        [i2] "=&r"(i2),            // %[i2]
        [i3] "=&r"(i3),            // %[i3]
        [p0] "=&r"(p0),            // %[p0]
        [p1] "=&r"(p1)             // %[p1]
      : [src] "r"(src_argb),       // %[src]
        [x] "r"(x),                // %[x]
        [dx] "r"(dx),              // %[dx]
        [ramp] "r"(kRamp01234567)  // %[ramp]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
}
#endif  // HAS_SCALEARGBCOLS_LASX

#ifdef HAS_SCALEARGBFILTERCOLS_LASX
void ScaleARGBFilterCols_LASX(uint8_t* dst_argb,
                              const uint8_t* src_argb,
                              int dst_width,
                              int x,
                              int dx) {
  ptrdiff_t i0, i1, i2, i3;
  uint64_t p0, p1;
  asm volatile(
      "srli.w     %[w], %[w], 3                        \n"
      "beqz       %[w], 2f                             \n"
      "xvld       $xr0, %[ramp], 0                     \n"
      "xvreplgr2vr.w $xr1, %[dx]                       \n"
      "xvreplgr2vr.w $xr2, %[x]                        \n"
      "xvmul.w    $xr0, $xr1, $xr0                     \n"
      "xvslli.w   $xr1, $xr1, 3                        \n"
      "xvadd.w    $xr2, $xr2, $xr0                     \n"
      "xvldi      $xr3, 0x7f                           \n"
      "1:                                              \n"
      "xvsrai.w   $xr0, $xr2, 16                       \n"
      "xvslli.w   $xr0, $xr0, 2                        \n"
      "xvsrai.w   $xr5, $xr2, 9                        \n"
      "xvadd.w    $xr2, $xr2, $xr1                     \n"
      "xvand.v    $xr5, $xr5, $xr3                     \n"
      "xvshuf4i.b $xr5, $xr5, 0x00                     \n"
      "xvxor.v    $xr6, $xr5, $xr3                     \n"
      "xvilvl.b   $xr7, $xr5, $xr6                     \n"
      "xvilvh.b   $xr8, $xr5, $xr6                     \n"
      "xvpickve2gr.w %[i0], $xr0, 0                    \n"
      "xvpickve2gr.w %[i1], $xr0, 1                    \n"
      "xvpickve2gr.w %[i2], $xr0, 2                    \n"
      "xvpickve2gr.w %[i3], $xr0, 3                    \n"
      "ldx.d      %[p0], %[src], %[i0]                 \n"
      "ldx.d      %[p1], %[src], %[i1]                 \n"
      "xvinsgr2vr.d $xr4, %[p0], 0                     \n"
      "xvinsgr2vr.d $xr4, %[p1], 1                     \n"
      "ldx.d      %[p0], %[src], %[i2]                 \n"
      "ldx.d      %[p1], %[src], %[i3]                 \n"
      "xvinsgr2vr.d $xr5, %[p0], 0                     \n"
      "xvinsgr2vr.d $xr5, %[p1], 1                     \n"
      "xvpickve2gr.w %[i0], $xr0, 4                    \n"
      "xvpickve2gr.w %[i1], $xr0, 5                    \n"
      "xvpickve2gr.w %[i2], $xr0, 6                    \n"
      "xvpickve2gr.w %[i3], $xr0, 7                    \n"
      "ldx.d      %[p0], %[src], %[i0]                 \n"
      "ldx.d      %[p1], %[src], %[i1]                 \n"
      "xvinsgr2vr.d $xr4, %[p0], 2                     \n"
      "xvinsgr2vr.d $xr4, %[p1], 3                     \n"
      "ldx.d      %[p0], %[src], %[i2]                 \n"
      "ldx.d      %[p1], %[src], %[i3]                 \n"
      "xvinsgr2vr.d $xr5, %[p0], 2                     \n"
      "xvinsgr2vr.d $xr5, %[p1], 3                     \n"
      "xvpickev.w $xr6, $xr5, $xr4                     \n"
      "xvpickod.w $xr4, $xr5, $xr4                     \n"
      "xvilvl.b   $xr5, $xr4, $xr6                     \n"
      "xvilvh.b   $xr4, $xr4, $xr6                     \n"
      "xvmulwev.h.bu $xr6, $xr5, $xr7                  \n"
      "xvmaddwod.h.bu $xr6, $xr5, $xr7                 \n"
      "xvmulwev.h.bu $xr7, $xr4, $xr8                  \n"
      "xvmaddwod.h.bu $xr7, $xr4, $xr8                 \n"
      "xvsrani.b.h $xr7, $xr6, 7                       \n"
      "xvst       $xr7, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -1                       \n"
      "bnez       %[w], 1b                             \n"
      "2:                                              \n"
      : [dst] "+r"(dst_argb),      // %[dst]
        [w] "+r"(dst_width),       // %[w]
        [i0] "=&r"(i0),            // %[i0]
        [i1] "=&r"(i1),            // %[i1]
        [i2] "=&r"(i2),            // %[i2]
        [i3] "=&r"(i3),            // %[i3]
        [p0] "=&r"(p0),            // %[p0]
        [p1] "=&r"(p1)             // %[p1]
      : [src] "r"(src_argb),       // %[src]
        [x] "r"(x),                // %[x]
        [dx] "r"(dx),              // %[dx]
        [ramp] "r"(kRamp01234567)  // %[ramp]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8");
}
#endif  // HAS_SCALEARGBFILTERCOLS_LASX

#ifdef HAS_SCALEROWDOWN34_LASX
void ScaleRowDown34_LASX(const uint8_t* src_ptr,
                         ptrdiff_t src_stride,
                         uint8_t* dst,
                         int dst_width) {
  int count = dst_width / 24;
  (void)src_stride;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr1, %[shuf], 0                     \n"
      "xvpermi.d  $xr1, $xr1, 0x44                     \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "xvshuf.b   $xr0, $xr0, $xr0, $xr1               \n"
      "xvstelm.d  $xr0, %[dst], 0, 0                   \n"
      "xvstelm.w  $xr0, %[dst], 8, 2                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "xvstelm.d  $xr0, %[dst], 0, 2                   \n"
      "xvstelm.w  $xr0, %[dst], 8, 6                   \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),      // %[src]
        [dst] "+r"(dst),          // %[dst]
        [cnt] "+r"(count)         // %[cnt]
      : [shuf] "r"(kScale34Shuf)  // %[shuf]
      : "memory", "$xr0", "$xr1");
}

void ScaleRowDown34_0_Box_LASX(const uint8_t* src_ptr,
                               ptrdiff_t src_stride,
                               uint8_t* d,
                               int dst_width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  int count = dst_width / 24;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr0, %[lo], 0                       \n"
      "vld        $vr1, %[hi], 0                       \n"
      "vld        $vr2, %[wt], 0                       \n"
      "vld        $vr3, %[pack], 0                     \n"
      "xvpermi.d  $xr0, $xr0, 0x44                     \n"
      "xvpermi.d  $xr1, $xr1, 0x44                     \n"
      "xvpermi.d  $xr2, $xr2, 0x44                     \n"
      "xvpermi.d  $xr3, $xr3, 0x44                     \n"
      "1:                                              \n"
      "xvld       $xr4, %[src], 0                      \n"
      "xvld       $xr5, %[nex], 0                      \n"
      "xvshuf.b   $xr6, $xr4, $xr4, $xr0               \n"
      "xvshuf.b   $xr7, $xr4, $xr4, $xr1               \n"
      "xvshuf.b   $xr8, $xr5, $xr5, $xr0               \n"
      "xvshuf.b   $xr9, $xr5, $xr5, $xr1               \n"
      "xvmulwev.h.bu $xr4, $xr6, $xr2                  \n"
      "xvmaddwod.h.bu $xr4, $xr6, $xr2                 \n"
      "xvmulwev.h.bu $xr5, $xr7, $xr2                  \n"
      "xvmaddwod.h.bu $xr5, $xr7, $xr2                 \n"
      "xvmulwev.h.bu $xr6, $xr8, $xr2                  \n"
      "xvmaddwod.h.bu $xr6, $xr8, $xr2                 \n"
      "xvmulwev.h.bu $xr7, $xr9, $xr2                  \n"
      "xvmaddwod.h.bu $xr7, $xr9, $xr2                 \n"
      "xvsrari.h  $xr4, $xr4, 2                        \n"
      "xvsrari.h  $xr5, $xr5, 2                        \n"
      "xvsrari.h  $xr6, $xr6, 2                        \n"
      "xvsrari.h  $xr7, $xr7, 2                        \n"
      "xvslli.h   $xr8, $xr4, 1                        \n"
      "xvslli.h   $xr9, $xr5, 1                        \n"
      "xvadd.h    $xr4, $xr4, $xr8                     \n"
      "xvadd.h    $xr5, $xr5, $xr9                     \n"
      "xvadd.h    $xr4, $xr4, $xr6                     \n"
      "xvadd.h    $xr5, $xr5, $xr7                     \n"
      "xvsrarni.b.h $xr5, $xr4, 2                      \n"
      "xvshuf.b   $xr5, $xr5, $xr5, $xr3               \n"
      "xvstelm.d  $xr5, %[dst], 0, 0                   \n"
      "xvstelm.w  $xr5, %[dst], 8, 2                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "xvstelm.d  $xr5, %[dst], 0, 2                   \n"
      "xvstelm.w  $xr5, %[dst], 8, 6                   \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[nex], %[nex], 32                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),        // %[src]
        [nex] "+r"(src_nex),        // %[nex]
        [dst] "+r"(d),              // %[dst]
        [cnt] "+r"(count)           // %[cnt]
      : [lo] "r"(kScale34BoxLo),    // %[lo]
        [hi] "r"(kScale34BoxHi),    // %[hi]
        [wt] "r"(kScale34BoxWt),    // %[wt]
        [pack] "r"(kScale34Pack12)  // %[pack]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9");
}

void ScaleRowDown34_1_Box_LASX(const uint8_t* src_ptr,
                               ptrdiff_t src_stride,
                               uint8_t* d,
                               int dst_width) {
  const uint8_t* src_nex = src_ptr + src_stride;
  int count = dst_width / 24;
  asm volatile(
      "beqz       %[cnt], 2f                           \n"
      "vld        $vr0, %[lo], 0                       \n"
      "vld        $vr1, %[hi], 0                       \n"
      "vld        $vr2, %[wt], 0                       \n"
      "vld        $vr3, %[pack], 0                     \n"
      "xvpermi.d  $xr0, $xr0, 0x44                     \n"
      "xvpermi.d  $xr1, $xr1, 0x44                     \n"
      "xvpermi.d  $xr2, $xr2, 0x44                     \n"
      "xvpermi.d  $xr3, $xr3, 0x44                     \n"
      "1:                                              \n"
      "xvld       $xr4, %[src], 0                      \n"
      "xvld       $xr5, %[nex], 0                      \n"
      "xvshuf.b   $xr6, $xr4, $xr4, $xr0               \n"
      "xvshuf.b   $xr7, $xr4, $xr4, $xr1               \n"
      "xvshuf.b   $xr8, $xr5, $xr5, $xr0               \n"
      "xvshuf.b   $xr9, $xr5, $xr5, $xr1               \n"
      "xvmulwev.h.bu $xr4, $xr6, $xr2                  \n"
      "xvmaddwod.h.bu $xr4, $xr6, $xr2                 \n"
      "xvmulwev.h.bu $xr5, $xr7, $xr2                  \n"
      "xvmaddwod.h.bu $xr5, $xr7, $xr2                 \n"
      "xvmulwev.h.bu $xr6, $xr8, $xr2                  \n"
      "xvmaddwod.h.bu $xr6, $xr8, $xr2                 \n"
      "xvmulwev.h.bu $xr7, $xr9, $xr2                  \n"
      "xvmaddwod.h.bu $xr7, $xr9, $xr2                 \n"
      "xvsrari.h  $xr4, $xr4, 2                        \n"
      "xvsrari.h  $xr5, $xr5, 2                        \n"
      "xvsrari.h  $xr6, $xr6, 2                        \n"
      "xvsrari.h  $xr7, $xr7, 2                        \n"
      "xvadd.h    $xr4, $xr4, $xr6                     \n"
      "xvadd.h    $xr5, $xr5, $xr7                     \n"
      "xvsrarni.b.h $xr5, $xr4, 1                      \n"
      "xvshuf.b   $xr5, $xr5, $xr5, $xr3               \n"
      "xvstelm.d  $xr5, %[dst], 0, 0                   \n"
      "xvstelm.w  $xr5, %[dst], 8, 2                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "xvstelm.d  $xr5, %[dst], 0, 2                   \n"
      "xvstelm.w  $xr5, %[dst], 8, 6                   \n"
      "addi.d     %[src], %[src], 32                   \n"
      "addi.d     %[nex], %[nex], 32                   \n"
      "addi.d     %[dst], %[dst], 12                   \n"
      "addi.w     %[cnt], %[cnt], -1                   \n"
      "bnez       %[cnt], 1b                           \n"
      "2:                                              \n"
      : [src] "+r"(src_ptr),        // %[src]
        [nex] "+r"(src_nex),        // %[nex]
        [dst] "+r"(d),              // %[dst]
        [cnt] "+r"(count)           // %[cnt]
      : [lo] "r"(kScale34BoxLo),    // %[lo]
        [hi] "r"(kScale34BoxHi),    // %[hi]
        [wt] "r"(kScale34BoxWt),    // %[wt]
        [pack] "r"(kScale34Pack12)  // %[pack]
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3", "$xr4", "$xr5", "$xr6",
        "$xr7", "$xr8", "$xr9");
}
#endif  // HAS_SCALEROWDOWN34_LASX

#ifdef HAS_SCALEMAXSAMPLES_LASX
float ScaleMaxSamples_LASX(const float* src,
                           float* dst,
                           float scale,
                           int width) {
  float fmax = 0.f;
  asm volatile(
      "xvldi      $xr3, 0                              \n"
      "xvldrepl.w $xr2, %[scale_ptr], 0                \n"
      "addi.w     %[w], %[w], -8                       \n"
      "bltz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "addi.d     %[src], %[src], 32                   \n"
      "xvfmax.s   $xr3, $xr3, $xr0                     \n"
      "xvfmul.s   $xr0, $xr0, $xr2                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -8                       \n"
      "bgez       %[w], 1b                             \n"
      "xvpermi.d  $xr1, $xr3, 0x4e                     \n"
      "vfmax.s    $vr3, $vr3, $vr1                     \n"
      "vshuf4i.w  $vr1, $vr3, 0x4e                     \n"
      "vfmax.s    $vr3, $vr3, $vr1                     \n"
      "vshuf4i.w  $vr1, $vr3, 0xb1                     \n"
      "vfmax.s    $vr3, $vr3, $vr1                     \n"
      "vstelm.w   $vr3, %[fmax_ptr], 0, 0              \n"
      "2:                                              \n"
      "addi.w     %[w], %[w], 8                        \n"
      : [src] "+r"(src),    // %[src]
        [dst] "+r"(dst),    // %[dst]
        [w] "+r"(width)     // %[w]
      : [scale_ptr] "r"(&scale),
        [fmax_ptr] "r"(&fmax)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
  for (; width > 0; --width) {
    float v = *src++;
    float vs = v * scale;
    fmax = (v > fmax) ? v : fmax;
    *dst++ = vs;
  }
  return fmax;
}
#endif  // HAS_SCALEMAXSAMPLES_LASX

#ifdef HAS_SCALESUMSAMPLES_LASX
float ScaleSumSamples_LASX(const float* src,
                           float* dst,
                           float scale,
                           int width) {
  float fsum = 0.f;
  asm volatile(
      "xvldi      $xr3, 0                              \n"
      "xvldrepl.w $xr2, %[scale_ptr], 0                \n"
      "addi.w     %[w], %[w], -8                       \n"
      "bltz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "addi.d     %[src], %[src], 32                   \n"
      "xvfmadd.s  $xr3, $xr0, $xr0, $xr3               \n"
      "xvfmul.s   $xr0, $xr0, $xr2                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -8                       \n"
      "bgez       %[w], 1b                             \n"
      "xvpermi.d  $xr1, $xr3, 0x4e                     \n"
      "vfadd.s    $vr3, $vr3, $vr1                     \n"
      "vshuf4i.w  $vr1, $vr3, 0x4e                     \n"
      "vfadd.s    $vr3, $vr3, $vr1                     \n"
      "vshuf4i.w  $vr1, $vr3, 0xb1                     \n"
      "vfadd.s    $vr3, $vr3, $vr1                     \n"
      "vstelm.w   $vr3, %[fsum_ptr], 0, 0              \n"
      "2:                                              \n"
      "addi.w     %[w], %[w], 8                        \n"
      : [src] "+r"(src),    // %[src]
        [dst] "+r"(dst),    // %[dst]
        [w] "+r"(width)     // %[w]
      : [scale_ptr] "r"(&scale),
        [fsum_ptr] "r"(&fsum)
      : "memory", "$xr0", "$xr1", "$xr2", "$xr3");
  for (; width > 0; --width) {
    float v = *src++;
    fsum += v * v;
    *dst++ = v * scale;
  }
  return fsum;
}
#endif  // HAS_SCALESUMSAMPLES_LASX

#ifdef HAS_SCALESAMPLES_LASX
void ScaleSamples_LASX(const float* src, float* dst, float scale, int width) {
  asm volatile(
      "xvldrepl.w $xr2, %[scale_ptr], 0                \n"
      "addi.w     %[w], %[w], -8                       \n"
      "bltz       %[w], 2f                             \n"
      "1:                                              \n"
      "xvld       $xr0, %[src], 0                      \n"
      "addi.d     %[src], %[src], 32                   \n"
      "xvfmul.s   $xr0, $xr0, $xr2                     \n"
      "xvst       $xr0, %[dst], 0                      \n"
      "addi.d     %[dst], %[dst], 32                   \n"
      "addi.w     %[w], %[w], -8                       \n"
      "bgez       %[w], 1b                             \n"
      "2:                                              \n"
      "addi.w     %[w], %[w], 8                        \n"
      : [src] "+r"(src),    // %[src]
        [dst] "+r"(dst),    // %[dst]
        [w] "+r"(width)     // %[w]
      : [scale_ptr] "r"(&scale)
      : "memory", "$xr0", "$xr2");
  if (width > 0) {
    ScaleSamples_C(src, dst, scale, width);
  }
}
#endif  // HAS_SCALESAMPLES_LASX

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // (!defined(LIBYUV_DISABLE_LSX) || !defined(LIBYUV_DISABLE_LASX)) &&
        // defined(__loongarch_lp64)

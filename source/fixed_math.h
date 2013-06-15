/*
 *  Copyright 2013 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef INCLUDE_LIBYUV_FIXED_MATH_H_  // NOLINT
#define INCLUDE_LIBYUV_FIXED_MATH_H_

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

extern const float kRecipTable[4097];

// Divide num by div and return value as 16.16 fixed point.
static __inline int FixedDiv(int num, int div) {
  if (static_cast<unsigned int>(div) <= 4096u) {
    return static_cast<int>(num * kRecipTable[div]);
  }
  return static_cast<int>((static_cast<int64>(num) << 16) / div);
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // INCLUDE_LIBYUV_FIXED_MATH_H_  NOLINT

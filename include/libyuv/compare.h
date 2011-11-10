/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef INCLUDE_LIBYUV_COMPARE_H_
#define INCLUDE_LIBYUV_COMPARE_H_

#include "libyuv/basic_types.h"

namespace libyuv {

uint64 SumSquareError(const uint8* src_a, const uint8* src_b, uint64 count);

}  // namespace libyuv

#endif  // INCLUDE_LIBYUV_COMPARE_H_

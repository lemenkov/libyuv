/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <string.h>

#include "libyuv/video_common.h"
#include "../unit_test/unit_test.h"

namespace libyuv {

// Tests SVN version against include/libyuv/version.h
// SVN version is bumped by documentation changes as well as code.
// Although the versions should match, once checked in, a tolerance is allowed.
TEST_F(libyuvTest, TestCanonicalFourCC) {
  EXPECT_EQ(FOURCC_YUY2, CanonicalFourCC(FOURCC_YUYV));
  EXPECT_EQ(FOURCC_YUY2, CanonicalFourCC(FOURCC_YUVS));
}

}  // namespace libyuv

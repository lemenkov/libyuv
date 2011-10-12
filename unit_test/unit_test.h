/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef UINIT_TEST_H_
#define UINIT_TEST_H_

#include "basic_types.h"
#include <gtest/gtest.h>

class libyuvTest : public ::testing::Test {
 protected:
  libyuvTest();
  virtual void SetUp();
  virtual void TearDown();

  const uint32 _rotate_max_w;
  const uint32 _rotate_max_h;

};

#endif // UNIT_TEST_H_

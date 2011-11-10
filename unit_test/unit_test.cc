/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cstring>
#include "unit_test.h"

class libyuvEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

libyuvTest::libyuvTest() :
  _rotate_max_w(128),
  _rotate_max_h(128),
  _benchmark_iterations(1000),
  _benchmark_width(1280),
  _benchmark_height(720) {
}

void libyuvTest::SetUp() {
}

void libyuvTest::TearDown() {
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  libyuvEnvironment* env = new libyuvEnvironment;
  ::testing::AddGlobalTestEnvironment(env);

  return RUN_ALL_TESTS();
}
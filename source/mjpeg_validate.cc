/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/mjpeg_decoder.h"

namespace libyuv {

// Helper function to validate the jpeg appears intact.
// TODO(fbarchard): Optimize case where SOI is found but EOI is not.
bool ValidateJpeg(const uint8* sample, size_t sample_size) {
  if (sample_size < 64) {
    // ERROR: Invalid jpeg size: sample_size
    return false;
  }
  if (sample[0] != 0xff || sample[1] != 0xd8) {  // Start Of Image
    // ERROR: Invalid jpeg initial start code
    return false;
  }
  for (int i = static_cast<int>(sample_size) - 2; i > 1;) {
    if (sample[i] != 0xd9) {
      if (sample[i] == 0xff && sample[i + 1] == 0xd9) {  // End Of Image
        return true;
      }
      --i;
    }
    --i;
  }
  // ERROR: Invalid jpeg end code not found. Size sample_size
  return false;
}

}  // namespace libyuv



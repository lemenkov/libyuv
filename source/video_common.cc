/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "video_common.h"

#include <sstream>

#include "common.h"

namespace libyuv {

#define ARRAY_SIZE(x) (static_cast<int>((sizeof(x)/sizeof(x[0]))))

struct FourCCAliasEntry {
  uint32 alias;
  uint32 canonical;
};

static const FourCCAliasEntry kFourCCAliases[] = {
  {FOURCC_IYUV, FOURCC_I420},
  {FOURCC_YU12, FOURCC_I420},
  {FOURCC_YUYV, FOURCC_YUY2},
  {FOURCC_YUVS, FOURCC_YUY2},
  {FOURCC_HDYC, FOURCC_UYVY},
  {FOURCC_2VUY, FOURCC_UYVY},
  {FOURCC_BA81, FOURCC_BGGR},
  {FOURCC_JPEG, FOURCC_MJPG},  // Note: JPEG has DHT while MJPG does not.
  {FOURCC_RGB3, FOURCC_RAW},
  {FOURCC_BGR3, FOURCC_24BG},
};

uint32 CanonicalFourCC(uint32 fourcc) {
  for (int i = 0; i < ARRAY_SIZE(kFourCCAliases); ++i) {
    if (kFourCCAliases[i].alias == fourcc) {
      return kFourCCAliases[i].canonical;
    }
  }
  // Not an alias, so return it as-is.
  return fourcc;
}

std::string VideoFormat::ToString() const {
  std::string fourcc_name = GetFourccName(fourcc) + " ";
  for (std::string::const_iterator i = fourcc_name.begin();
      i < fourcc_name.end(); ++i) {
    // Test character is printable; Avoid isprint() which asserts on negatives
    if (*i < 32 || *i >= 127) {
      fourcc_name = "";
      break;
    }
  }

  std::ostringstream ss;
  ss << fourcc_name << width << "x" << height << "x" << IntervalToFps(interval);
  return ss.str();
}

}  // namespace libyuv

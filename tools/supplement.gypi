# Copyright 2011 The LibYuv Project Authors. All rights reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS. All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

# Supplement file for libyuv. To be processed, this file needs to be located in
# the first level of directories below the gyp_libyuv file.

# This is needed to workaround the otherwise failing include of base.gyp in
# Chromium's common.gypi when a sanitizer tool is used.
{
  'variables': {
    'use_sanitizer_options': 0,
  },
  'target_defaults': {
    'conditions': [
      # Add default sanitizer options similar to Chromium. This is needed to get
      # the sanitizer options that has LeakSanitizer disabled by default.
      # Otherwise yasm will throw leak errors during compile when
      # GYP_DEFINES="asan=1".
      ['OS=="linux" and (chromeos==0 or target_arch!="ia32")', {
        'dependencies': [
          '<(DEPTH)/tools/sanitizer_options.gyp:sanitizer_options',
        ],
      }],
    ],
  },
}

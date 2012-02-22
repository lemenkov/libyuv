# Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'targets': [
    {
      'target_name': 'libyuv',
      'type': 'static_library',
      'defines': [
        #'HAVE_JPEG',
      ],
      'include_dirs': [
        'include',
        '.',
	#'third_party/libjpeg_turbo',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include',
          '.',
        ],
      },
      'sources': [
        # includes
        'include/libyuv/basic_types.h',
        'include/libyuv/convert.h',
        'include/libyuv/convert_from.h',
        'include/libyuv/scale.h',
        'include/libyuv/planar_functions.h',
        'include/libyuv/video_common.h',
        'include/libyuv/version.h',

        # headers
        'source/rotate_priv.h',
        'source/row.h',

        # sources
        'source/compare.cc',
        'source/convert.cc',
        'source/convert_from.cc',
        'source/cpu_id.cc',
        'source/format_conversion.cc',
        'source/planar_functions.cc',
        #'source/mjpeg_decoder.cc',
        'source/rotate.cc',
        'source/rotate_neon.cc',
        'source/row_common.cc',
        'source/row_posix.cc',
        'source/row_neon.cc',
        'source/row_win.cc',
        'source/scale.cc',
        'source/video_common.cc',
      ],
    },
  ], # targets
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:

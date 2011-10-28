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
      'include_dirs': [
        'include',
        '.',
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
        'include/libyuv/general.h',
        'include/libyuv/scale.h',
        'include/libyuv/planar_functions.h',
        

        # headers
         'source/conversion_tables.h',
        'source/cpu_id.h',
        'source/rotate.h',
        'source/rotate_priv.h',
        'source/row.h',
        'source/video_common.h',

        # sources
        'source/convert.cc',
        'source/cpu_id.cc',
        'source/format_conversion.cc',
        'source/general.cc',
        'source/planar_functions.cc',
        'source/rotate.cc',
        'source/row_table.cc',
        'source/scale.cc',
        'source/video_common.cc',
      ],
      'conditions': [
        ['OS=="win"', {
         'sources': [
           'source/row_win.cc',
         ],
        },{ # else
         'sources': [
            'source/row_posix.cc',
          ],
        }],
      ]
    },
  ], # targets
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:

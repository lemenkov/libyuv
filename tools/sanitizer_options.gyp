# Copyright 2014 The LibYuv Project Authors. All rights reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS. All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

# This is a similar target to the one in Chromium's base.gyp. It's needed to get
# the same sanitizer settings as Chromium uses (i.e. ASan, LSan, TSan...).
{
  'targets': [
    {
      'target_name': 'sanitizer_options',
      'type': 'static_library',
      'toolsets': ['host', 'target'],
      'variables': {
         # Every target is going to depend on sanitizer_options, so allow
         # this one to depend on itself.
         'prune_self_dependency': 1,
         # Do not let 'none' targets depend on this one, they don't need to.
         'link_dependency': 1,
       },
      'sources': [
        'sanitizer_options/sanitizer_options.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      # Some targets may want to opt-out from ASan, TSan and MSan and link
      # without the corresponding runtime libraries. We drop the libc++
      # dependency and omit the compiler flags to avoid bringing instrumented
      # code to those targets.
      'conditions': [
        ['use_custom_libcxx==1', {
          'dependencies!': [
            '<(DEPTH)/third_party/libc++/libc++.gyp:libcxx_proxy',
          ],
        }],
        ['tsan==1', {
          'sources': [
            'tsan_suppressions/tsan_suppressions.cc',
          ],
        }],
      ],
      'cflags!': [
        '-fsanitize=address',
        '-fsanitize=thread',
        '-fsanitize=memory',
        '-fsanitize-memory-track-origins',
      ],
      'direct_dependent_settings': {
        'ldflags': [
          '-Wl,-u_sanitizer_options_link_helper',
        ],
      },
    },
  ],  # targets
}

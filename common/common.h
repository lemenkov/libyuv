/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef LIBYUV_SOURCE_COMMON_H_
#define LIBYUV_SOURCE_COMMON_H_

#if defined(_MSC_VER)
// warning C4355: 'this' : used in base member initializer list
#pragma warning(disable:4355)
#endif

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG _DEBUG
#endif  // !defined(ENABLE_DEBUG)

#if ENABLE_DEBUG

#if defined(_MSC_VER) && _MSC_VER < 1300
#define __FUNCTION__ ""
#endif
#else // !ENABLE_DEBUG

#endif // !ENABLE_DEBUG

// Forces compiler to inline, even against its better judgement. Use wisely.
#if defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline))
#elif defined(WIN32)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE
#endif

#endif // LIBYUV_SOURCE_COMMON_H_

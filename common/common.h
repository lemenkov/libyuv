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

#include "constructor_magic.h"

#if defined(_MSC_VER)
// warning C4355: 'this' : used in base member initializer list
#pragma warning(disable:4355)
#endif

//////////////////////////////////////////////////////////////////////
// General Utilities
//////////////////////////////////////////////////////////////////////

#ifndef UNUSED
#define UNUSED(x) Unused(static_cast<const void *>(&x))
#define UNUSED2(x,y) Unused(static_cast<const void *>(&x)); Unused(static_cast<const void *>(&y))
#define UNUSED3(x,y,z) Unused(static_cast<const void *>(&x)); Unused(static_cast<const void *>(&y)); Unused(static_cast<const void *>(&z))
#define UNUSED4(x,y,z,a) Unused(static_cast<const void *>(&x)); Unused(static_cast<const void *>(&y)); Unused(static_cast<const void *>(&z)); Unused(static_cast<const void *>(&a))
#define UNUSED5(x,y,z,a,b) Unused(static_cast<const void *>(&x)); Unused(static_cast<const void *>(&y)); Unused(static_cast<const void *>(&z)); Unused(static_cast<const void *>(&a)); Unused(static_cast<const void *>(&b))
inline void Unused(const void *) { }
#endif // UNUSED

#ifndef WIN32
#define strnicmp(x,y,n) strncasecmp(x,y,n)
#define stricmp(x,y) strcasecmp(x,y)

// TODO(sergeyu): Remove this. std::max should be used everywhere in the code.
// NOMINMAX must be defined where we include <windows.h>.
#define stdmax(x,y) std::max(x,y)
#else
#define stdmax(x,y) libyuv::_max(x,y)
#endif


#define ARRAY_SIZE(x) (static_cast<int>((sizeof(x)/sizeof(x[0]))))

/////////////////////////////////////////////////////////////////////////////
// Assertions
/////////////////////////////////////////////////////////////////////////////

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG _DEBUG
#endif  // !defined(ENABLE_DEBUG)

#if ENABLE_DEBUG

namespace libyuv {

// Break causes the debugger to stop executing, or the program to abort
void Break();

// LogAssert writes information about an assertion to the log
void LogAssert(const char * function, const char * file, int line,
               const char * expression);

inline bool Assert(bool result, const char * function, const char * file,
                   int line, const char * expression) {
  if (!result) {
    LogAssert(function, file, line, expression);
    Break();
    return false;
  }
  return true;
}

}  // namespace libyuv

#if defined(_MSC_VER) && _MSC_VER < 1300
#define __FUNCTION__ ""
#endif

#ifndef ASSERT
#define ASSERT(x) (void)libyuv::Assert((x),__FUNCTION__,__FILE__,__LINE__,#x)
#endif

#ifndef VERIFY
#define VERIFY(x) libyuv::Assert((x),__FUNCTION__,__FILE__,__LINE__,#x)
#endif

#else // !ENABLE_DEBUG

namespace libyuv {

inline bool libyuv(bool result) { return result; }

}  // namespace libyuv

#ifndef ASSERT
#define ASSERT(x) (void)0
#endif

#ifndef VERIFY
#define VERIFY(x) libyuv::ImplicitCastToBool(x)
#endif

#endif // !ENABLE_DEBUG

#define COMPILE_TIME_ASSERT(expr)       char CTA_UNIQUE_NAME[expr]
#define CTA_UNIQUE_NAME                 CTA_MAKE_NAME(__LINE__)
#define CTA_MAKE_NAME(line)             CTA_MAKE_NAME2(line)
#define CTA_MAKE_NAME2(line)            constraint_ ## line

#ifdef __GNUC__
// Forces compiler to inline, even against its better judgement. Use wisely.
#define FORCE_INLINE __attribute__((always_inline))
#else
#define FORCE_INLINE
#endif

#endif // LIBYUV_SOURCE_COMMON_H_

/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef INCLUDE_LIBYUV_CPU_ID_H_
#define INCLUDE_LIBYUV_CPU_ID_H_

namespace libyuv {

// These flags are only valid on x86 processors
static const int kCpuHasSSE2 = 1;
static const int kCpuHasSSSE3 = 2;

// These flags are only valid on ARM processors
static const int kCpuHasNEON = 4;

// Internal flag to indicate cpuid is initialized.
static const int kCpuInitialized = 8;

// Detect CPU has SSE2 etc.
bool TestCpuFlag(int flag);

// For testing, allow CPU flags to be disabled.
// ie MaskCpuFlags(~kCpuHasSSSE3) to disable SSSE3.  -1 to enable all.
void MaskCpuFlags(int enable_flags);

}  // namespace libyuv

#endif  // INCLUDE_LIBYUV_CPU_ID_H_

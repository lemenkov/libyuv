/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "row.h"

extern "C" {

#define MAKETABLE(NAME) \
SIMD_ALIGNED(const int16 NAME[256 * 3][4]) = {\
  RGBY(0x00), RGBY(0x01), RGBY(0x02), RGBY(0x03), \
  RGBY(0x04), RGBY(0x05), RGBY(0x06), RGBY(0x07), \
  RGBY(0x08), RGBY(0x09), RGBY(0x0A), RGBY(0x0B), \
  RGBY(0x0C), RGBY(0x0D), RGBY(0x0E), RGBY(0x0F), \
  RGBY(0x10), RGBY(0x11), RGBY(0x12), RGBY(0x13), \
  RGBY(0x14), RGBY(0x15), RGBY(0x16), RGBY(0x17), \
  RGBY(0x18), RGBY(0x19), RGBY(0x1A), RGBY(0x1B), \
  RGBY(0x1C), RGBY(0x1D), RGBY(0x1E), RGBY(0x1F), \
  RGBY(0x20), RGBY(0x21), RGBY(0x22), RGBY(0x23), \
  RGBY(0x24), RGBY(0x25), RGBY(0x26), RGBY(0x27), \
  RGBY(0x28), RGBY(0x29), RGBY(0x2A), RGBY(0x2B), \
  RGBY(0x2C), RGBY(0x2D), RGBY(0x2E), RGBY(0x2F), \
  RGBY(0x30), RGBY(0x31), RGBY(0x32), RGBY(0x33), \
  RGBY(0x34), RGBY(0x35), RGBY(0x36), RGBY(0x37), \
  RGBY(0x38), RGBY(0x39), RGBY(0x3A), RGBY(0x3B), \
  RGBY(0x3C), RGBY(0x3D), RGBY(0x3E), RGBY(0x3F), \
  RGBY(0x40), RGBY(0x41), RGBY(0x42), RGBY(0x43), \
  RGBY(0x44), RGBY(0x45), RGBY(0x46), RGBY(0x47), \
  RGBY(0x48), RGBY(0x49), RGBY(0x4A), RGBY(0x4B), \
  RGBY(0x4C), RGBY(0x4D), RGBY(0x4E), RGBY(0x4F), \
  RGBY(0x50), RGBY(0x51), RGBY(0x52), RGBY(0x53), \
  RGBY(0x54), RGBY(0x55), RGBY(0x56), RGBY(0x57), \
  RGBY(0x58), RGBY(0x59), RGBY(0x5A), RGBY(0x5B), \
  RGBY(0x5C), RGBY(0x5D), RGBY(0x5E), RGBY(0x5F), \
  RGBY(0x60), RGBY(0x61), RGBY(0x62), RGBY(0x63), \
  RGBY(0x64), RGBY(0x65), RGBY(0x66), RGBY(0x67), \
  RGBY(0x68), RGBY(0x69), RGBY(0x6A), RGBY(0x6B), \
  RGBY(0x6C), RGBY(0x6D), RGBY(0x6E), RGBY(0x6F), \
  RGBY(0x70), RGBY(0x71), RGBY(0x72), RGBY(0x73), \
  RGBY(0x74), RGBY(0x75), RGBY(0x76), RGBY(0x77), \
  RGBY(0x78), RGBY(0x79), RGBY(0x7A), RGBY(0x7B), \
  RGBY(0x7C), RGBY(0x7D), RGBY(0x7E), RGBY(0x7F), \
  RGBY(0x80), RGBY(0x81), RGBY(0x82), RGBY(0x83), \
  RGBY(0x84), RGBY(0x85), RGBY(0x86), RGBY(0x87), \
  RGBY(0x88), RGBY(0x89), RGBY(0x8A), RGBY(0x8B), \
  RGBY(0x8C), RGBY(0x8D), RGBY(0x8E), RGBY(0x8F), \
  RGBY(0x90), RGBY(0x91), RGBY(0x92), RGBY(0x93), \
  RGBY(0x94), RGBY(0x95), RGBY(0x96), RGBY(0x97), \
  RGBY(0x98), RGBY(0x99), RGBY(0x9A), RGBY(0x9B), \
  RGBY(0x9C), RGBY(0x9D), RGBY(0x9E), RGBY(0x9F), \
  RGBY(0xA0), RGBY(0xA1), RGBY(0xA2), RGBY(0xA3), \
  RGBY(0xA4), RGBY(0xA5), RGBY(0xA6), RGBY(0xA7), \
  RGBY(0xA8), RGBY(0xA9), RGBY(0xAA), RGBY(0xAB), \
  RGBY(0xAC), RGBY(0xAD), RGBY(0xAE), RGBY(0xAF), \
  RGBY(0xB0), RGBY(0xB1), RGBY(0xB2), RGBY(0xB3), \
  RGBY(0xB4), RGBY(0xB5), RGBY(0xB6), RGBY(0xB7), \
  RGBY(0xB8), RGBY(0xB9), RGBY(0xBA), RGBY(0xBB), \
  RGBY(0xBC), RGBY(0xBD), RGBY(0xBE), RGBY(0xBF), \
  RGBY(0xC0), RGBY(0xC1), RGBY(0xC2), RGBY(0xC3), \
  RGBY(0xC4), RGBY(0xC5), RGBY(0xC6), RGBY(0xC7), \
  RGBY(0xC8), RGBY(0xC9), RGBY(0xCA), RGBY(0xCB), \
  RGBY(0xCC), RGBY(0xCD), RGBY(0xCE), RGBY(0xCF), \
  RGBY(0xD0), RGBY(0xD1), RGBY(0xD2), RGBY(0xD3), \
  RGBY(0xD4), RGBY(0xD5), RGBY(0xD6), RGBY(0xD7), \
  RGBY(0xD8), RGBY(0xD9), RGBY(0xDA), RGBY(0xDB), \
  RGBY(0xDC), RGBY(0xDD), RGBY(0xDE), RGBY(0xDF), \
  RGBY(0xE0), RGBY(0xE1), RGBY(0xE2), RGBY(0xE3), \
  RGBY(0xE4), RGBY(0xE5), RGBY(0xE6), RGBY(0xE7), \
  RGBY(0xE8), RGBY(0xE9), RGBY(0xEA), RGBY(0xEB), \
  RGBY(0xEC), RGBY(0xED), RGBY(0xEE), RGBY(0xEF), \
  RGBY(0xF0), RGBY(0xF1), RGBY(0xF2), RGBY(0xF3), \
  RGBY(0xF4), RGBY(0xF5), RGBY(0xF6), RGBY(0xF7), \
  RGBY(0xF8), RGBY(0xF9), RGBY(0xFA), RGBY(0xFB), \
  RGBY(0xFC), RGBY(0xFD), RGBY(0xFE), RGBY(0xFF), \
  RGBU(0x00), RGBU(0x01), RGBU(0x02), RGBU(0x03), \
  RGBU(0x04), RGBU(0x05), RGBU(0x06), RGBU(0x07), \
  RGBU(0x08), RGBU(0x09), RGBU(0x0A), RGBU(0x0B), \
  RGBU(0x0C), RGBU(0x0D), RGBU(0x0E), RGBU(0x0F), \
  RGBU(0x10), RGBU(0x11), RGBU(0x12), RGBU(0x13), \
  RGBU(0x14), RGBU(0x15), RGBU(0x16), RGBU(0x17), \
  RGBU(0x18), RGBU(0x19), RGBU(0x1A), RGBU(0x1B), \
  RGBU(0x1C), RGBU(0x1D), RGBU(0x1E), RGBU(0x1F), \
  RGBU(0x20), RGBU(0x21), RGBU(0x22), RGBU(0x23), \
  RGBU(0x24), RGBU(0x25), RGBU(0x26), RGBU(0x27), \
  RGBU(0x28), RGBU(0x29), RGBU(0x2A), RGBU(0x2B), \
  RGBU(0x2C), RGBU(0x2D), RGBU(0x2E), RGBU(0x2F), \
  RGBU(0x30), RGBU(0x31), RGBU(0x32), RGBU(0x33), \
  RGBU(0x34), RGBU(0x35), RGBU(0x36), RGBU(0x37), \
  RGBU(0x38), RGBU(0x39), RGBU(0x3A), RGBU(0x3B), \
  RGBU(0x3C), RGBU(0x3D), RGBU(0x3E), RGBU(0x3F), \
  RGBU(0x40), RGBU(0x41), RGBU(0x42), RGBU(0x43), \
  RGBU(0x44), RGBU(0x45), RGBU(0x46), RGBU(0x47), \
  RGBU(0x48), RGBU(0x49), RGBU(0x4A), RGBU(0x4B), \
  RGBU(0x4C), RGBU(0x4D), RGBU(0x4E), RGBU(0x4F), \
  RGBU(0x50), RGBU(0x51), RGBU(0x52), RGBU(0x53), \
  RGBU(0x54), RGBU(0x55), RGBU(0x56), RGBU(0x57), \
  RGBU(0x58), RGBU(0x59), RGBU(0x5A), RGBU(0x5B), \
  RGBU(0x5C), RGBU(0x5D), RGBU(0x5E), RGBU(0x5F), \
  RGBU(0x60), RGBU(0x61), RGBU(0x62), RGBU(0x63), \
  RGBU(0x64), RGBU(0x65), RGBU(0x66), RGBU(0x67), \
  RGBU(0x68), RGBU(0x69), RGBU(0x6A), RGBU(0x6B), \
  RGBU(0x6C), RGBU(0x6D), RGBU(0x6E), RGBU(0x6F), \
  RGBU(0x70), RGBU(0x71), RGBU(0x72), RGBU(0x73), \
  RGBU(0x74), RGBU(0x75), RGBU(0x76), RGBU(0x77), \
  RGBU(0x78), RGBU(0x79), RGBU(0x7A), RGBU(0x7B), \
  RGBU(0x7C), RGBU(0x7D), RGBU(0x7E), RGBU(0x7F), \
  RGBU(0x80), RGBU(0x81), RGBU(0x82), RGBU(0x83), \
  RGBU(0x84), RGBU(0x85), RGBU(0x86), RGBU(0x87), \
  RGBU(0x88), RGBU(0x89), RGBU(0x8A), RGBU(0x8B), \
  RGBU(0x8C), RGBU(0x8D), RGBU(0x8E), RGBU(0x8F), \
  RGBU(0x90), RGBU(0x91), RGBU(0x92), RGBU(0x93), \
  RGBU(0x94), RGBU(0x95), RGBU(0x96), RGBU(0x97), \
  RGBU(0x98), RGBU(0x99), RGBU(0x9A), RGBU(0x9B), \
  RGBU(0x9C), RGBU(0x9D), RGBU(0x9E), RGBU(0x9F), \
  RGBU(0xA0), RGBU(0xA1), RGBU(0xA2), RGBU(0xA3), \
  RGBU(0xA4), RGBU(0xA5), RGBU(0xA6), RGBU(0xA7), \
  RGBU(0xA8), RGBU(0xA9), RGBU(0xAA), RGBU(0xAB), \
  RGBU(0xAC), RGBU(0xAD), RGBU(0xAE), RGBU(0xAF), \
  RGBU(0xB0), RGBU(0xB1), RGBU(0xB2), RGBU(0xB3), \
  RGBU(0xB4), RGBU(0xB5), RGBU(0xB6), RGBU(0xB7), \
  RGBU(0xB8), RGBU(0xB9), RGBU(0xBA), RGBU(0xBB), \
  RGBU(0xBC), RGBU(0xBD), RGBU(0xBE), RGBU(0xBF), \
  RGBU(0xC0), RGBU(0xC1), RGBU(0xC2), RGBU(0xC3), \
  RGBU(0xC4), RGBU(0xC5), RGBU(0xC6), RGBU(0xC7), \
  RGBU(0xC8), RGBU(0xC9), RGBU(0xCA), RGBU(0xCB), \
  RGBU(0xCC), RGBU(0xCD), RGBU(0xCE), RGBU(0xCF), \
  RGBU(0xD0), RGBU(0xD1), RGBU(0xD2), RGBU(0xD3), \
  RGBU(0xD4), RGBU(0xD5), RGBU(0xD6), RGBU(0xD7), \
  RGBU(0xD8), RGBU(0xD9), RGBU(0xDA), RGBU(0xDB), \
  RGBU(0xDC), RGBU(0xDD), RGBU(0xDE), RGBU(0xDF), \
  RGBU(0xE0), RGBU(0xE1), RGBU(0xE2), RGBU(0xE3), \
  RGBU(0xE4), RGBU(0xE5), RGBU(0xE6), RGBU(0xE7), \
  RGBU(0xE8), RGBU(0xE9), RGBU(0xEA), RGBU(0xEB), \
  RGBU(0xEC), RGBU(0xED), RGBU(0xEE), RGBU(0xEF), \
  RGBU(0xF0), RGBU(0xF1), RGBU(0xF2), RGBU(0xF3), \
  RGBU(0xF4), RGBU(0xF5), RGBU(0xF6), RGBU(0xF7), \
  RGBU(0xF8), RGBU(0xF9), RGBU(0xFA), RGBU(0xFB), \
  RGBU(0xFC), RGBU(0xFD), RGBU(0xFE), RGBU(0xFF), \
  RGBV(0x00), RGBV(0x01), RGBV(0x02), RGBV(0x03), \
  RGBV(0x04), RGBV(0x05), RGBV(0x06), RGBV(0x07), \
  RGBV(0x08), RGBV(0x09), RGBV(0x0A), RGBV(0x0B), \
  RGBV(0x0C), RGBV(0x0D), RGBV(0x0E), RGBV(0x0F), \
  RGBV(0x10), RGBV(0x11), RGBV(0x12), RGBV(0x13), \
  RGBV(0x14), RGBV(0x15), RGBV(0x16), RGBV(0x17), \
  RGBV(0x18), RGBV(0x19), RGBV(0x1A), RGBV(0x1B), \
  RGBV(0x1C), RGBV(0x1D), RGBV(0x1E), RGBV(0x1F), \
  RGBV(0x20), RGBV(0x21), RGBV(0x22), RGBV(0x23), \
  RGBV(0x24), RGBV(0x25), RGBV(0x26), RGBV(0x27), \
  RGBV(0x28), RGBV(0x29), RGBV(0x2A), RGBV(0x2B), \
  RGBV(0x2C), RGBV(0x2D), RGBV(0x2E), RGBV(0x2F), \
  RGBV(0x30), RGBV(0x31), RGBV(0x32), RGBV(0x33), \
  RGBV(0x34), RGBV(0x35), RGBV(0x36), RGBV(0x37), \
  RGBV(0x38), RGBV(0x39), RGBV(0x3A), RGBV(0x3B), \
  RGBV(0x3C), RGBV(0x3D), RGBV(0x3E), RGBV(0x3F), \
  RGBV(0x40), RGBV(0x41), RGBV(0x42), RGBV(0x43), \
  RGBV(0x44), RGBV(0x45), RGBV(0x46), RGBV(0x47), \
  RGBV(0x48), RGBV(0x49), RGBV(0x4A), RGBV(0x4B), \
  RGBV(0x4C), RGBV(0x4D), RGBV(0x4E), RGBV(0x4F), \
  RGBV(0x50), RGBV(0x51), RGBV(0x52), RGBV(0x53), \
  RGBV(0x54), RGBV(0x55), RGBV(0x56), RGBV(0x57), \
  RGBV(0x58), RGBV(0x59), RGBV(0x5A), RGBV(0x5B), \
  RGBV(0x5C), RGBV(0x5D), RGBV(0x5E), RGBV(0x5F), \
  RGBV(0x60), RGBV(0x61), RGBV(0x62), RGBV(0x63), \
  RGBV(0x64), RGBV(0x65), RGBV(0x66), RGBV(0x67), \
  RGBV(0x68), RGBV(0x69), RGBV(0x6A), RGBV(0x6B), \
  RGBV(0x6C), RGBV(0x6D), RGBV(0x6E), RGBV(0x6F), \
  RGBV(0x70), RGBV(0x71), RGBV(0x72), RGBV(0x73), \
  RGBV(0x74), RGBV(0x75), RGBV(0x76), RGBV(0x77), \
  RGBV(0x78), RGBV(0x79), RGBV(0x7A), RGBV(0x7B), \
  RGBV(0x7C), RGBV(0x7D), RGBV(0x7E), RGBV(0x7F), \
  RGBV(0x80), RGBV(0x81), RGBV(0x82), RGBV(0x83), \
  RGBV(0x84), RGBV(0x85), RGBV(0x86), RGBV(0x87), \
  RGBV(0x88), RGBV(0x89), RGBV(0x8A), RGBV(0x8B), \
  RGBV(0x8C), RGBV(0x8D), RGBV(0x8E), RGBV(0x8F), \
  RGBV(0x90), RGBV(0x91), RGBV(0x92), RGBV(0x93), \
  RGBV(0x94), RGBV(0x95), RGBV(0x96), RGBV(0x97), \
  RGBV(0x98), RGBV(0x99), RGBV(0x9A), RGBV(0x9B), \
  RGBV(0x9C), RGBV(0x9D), RGBV(0x9E), RGBV(0x9F), \
  RGBV(0xA0), RGBV(0xA1), RGBV(0xA2), RGBV(0xA3), \
  RGBV(0xA4), RGBV(0xA5), RGBV(0xA6), RGBV(0xA7), \
  RGBV(0xA8), RGBV(0xA9), RGBV(0xAA), RGBV(0xAB), \
  RGBV(0xAC), RGBV(0xAD), RGBV(0xAE), RGBV(0xAF), \
  RGBV(0xB0), RGBV(0xB1), RGBV(0xB2), RGBV(0xB3), \
  RGBV(0xB4), RGBV(0xB5), RGBV(0xB6), RGBV(0xB7), \
  RGBV(0xB8), RGBV(0xB9), RGBV(0xBA), RGBV(0xBB), \
  RGBV(0xBC), RGBV(0xBD), RGBV(0xBE), RGBV(0xBF), \
  RGBV(0xC0), RGBV(0xC1), RGBV(0xC2), RGBV(0xC3), \
  RGBV(0xC4), RGBV(0xC5), RGBV(0xC6), RGBV(0xC7), \
  RGBV(0xC8), RGBV(0xC9), RGBV(0xCA), RGBV(0xCB), \
  RGBV(0xCC), RGBV(0xCD), RGBV(0xCE), RGBV(0xCF), \
  RGBV(0xD0), RGBV(0xD1), RGBV(0xD2), RGBV(0xD3), \
  RGBV(0xD4), RGBV(0xD5), RGBV(0xD6), RGBV(0xD7), \
  RGBV(0xD8), RGBV(0xD9), RGBV(0xDA), RGBV(0xDB), \
  RGBV(0xDC), RGBV(0xDD), RGBV(0xDE), RGBV(0xDF), \
  RGBV(0xE0), RGBV(0xE1), RGBV(0xE2), RGBV(0xE3), \
  RGBV(0xE4), RGBV(0xE5), RGBV(0xE6), RGBV(0xE7), \
  RGBV(0xE8), RGBV(0xE9), RGBV(0xEA), RGBV(0xEB), \
  RGBV(0xEC), RGBV(0xED), RGBV(0xEE), RGBV(0xEF), \
  RGBV(0xF0), RGBV(0xF1), RGBV(0xF2), RGBV(0xF3), \
  RGBV(0xF4), RGBV(0xF5), RGBV(0xF6), RGBV(0xF7), \
  RGBV(0xF8), RGBV(0xF9), RGBV(0xFA), RGBV(0xFB), \
  RGBV(0xFC), RGBV(0xFD), RGBV(0xFE), RGBV(0xFF), \
};

#define CS(v) static_cast<int16>(v)

// ARGB table
#define RGBY(i) { \
  CS(1.164 * 64 * (i - 16) + 0.5), \
  CS(1.164 * 64 * (i - 16) + 0.5), \
  CS(1.164 * 64 * (i - 16) + 0.5), \
  CS(256 * 64 - 1) \
}

#define RGBU(i) { \
  CS(2.018 * 64 * (i - 128) + 0.5), \
  CS(-0.391 * 64 * (i - 128) - 0.5), \
  0, \
  0 \
}

#define RGBV(i) { \
  0, \
  CS(-0.813 * 64 * (i - 128) - 0.5), \
  CS(1.596 * 64 * (i - 128) + 0.5), \
  0 \
}

MAKETABLE(kCoefficientsRgbY)

#undef RGBY
#undef RGBU
#undef RGBV

// BGRA table
#define RGBY(i) { \
  CS(256 * 64 - 1), \
  CS(1.164 * 64 * (i - 16) + 0.5), \
  CS(1.164 * 64 * (i - 16) + 0.5), \
  CS(1.164 * 64 * (i - 16) + 0.5) \
}

#define RGBU(i) { \
  0, \
  0, \
  CS(-0.391 * 64 * (i - 128) - 0.5), \
  CS(2.018 * 64 * (i - 128) + 0.5) \
}

#define RGBV(i) { \
  0, \
  CS(1.596 * 64 * (i - 128) + 0.5), \
  CS(-0.813 * 64 * (i - 128) - 0.5), \
  0 \
}

MAKETABLE(kCoefficientsBgraY)

#undef RGBY
#undef RGBU
#undef RGBV

// ABGR table
#define RGBY(i) { \
  CS(1.164 * 64 * (i - 16) + 0.5), \
  CS(1.164 * 64 * (i - 16) + 0.5), \
  CS(1.164 * 64 * (i - 16) + 0.5), \
  CS(256 * 64 - 1) \
}

#define RGBU(i) { \
  0, \
  CS(-0.391 * 64 * (i - 128) - 0.5), \
  CS(2.018 * 64 * (i - 128) + 0.5), \
  0 \
}

#define RGBV(i) { \
  CS(1.596 * 64 * (i - 128) + 0.5), \
  CS(-0.813 * 64 * (i - 128) - 0.5), \
  0, \
  0 \
}

MAKETABLE(kCoefficientsAbgrY)

void ABGRToARGBRow_C(const uint8* src_abgr, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    // To support in-place conversion.
    uint8 r = src_abgr[0];
    uint8 g = src_abgr[1];
    uint8 b = src_abgr[2];
    uint8 a = src_abgr[3];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = a;
    dst_argb += 4;
    src_abgr += 4;
  }
}

void BGRAToARGBRow_C(const uint8* src_bgra, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    // To support in-place conversion.
    uint8 a = src_bgra[0];
    uint8 r = src_bgra[1];
    uint8 g = src_bgra[2];
    uint8 b = src_bgra[3];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = a;
    dst_argb += 4;
    src_bgra += 4;
  }
}

void RAWToARGBRow_C(const uint8* src_raw, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 r = src_raw[0];
    uint8 g = src_raw[1];
    uint8 b = src_raw[2];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_raw += 3;
  }
}

void BG24ToARGBRow_C(const uint8* src_bg24, uint8* dst_argb, int pix) {
  for (int x = 0; x < pix; ++x) {
    uint8 b = src_bg24[0];
    uint8 g = src_bg24[1];
    uint8 r = src_bg24[2];
    dst_argb[0] = b;
    dst_argb[1] = g;
    dst_argb[2] = r;
    dst_argb[3] = 255u;
    dst_argb[3] = 255u;
    dst_argb += 4;
    src_bg24 += 3;
  }
}

// C versions do the same
void RGB24ToYRow_C(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  BG24ToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_C(row, dst_y, pix);
}

void RAWToYRow_C(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  RAWToARGBRow_C(src_argb, row, pix);
  ARGBToYRow_C(row, dst_y, pix);
}

void RGB24ToUVRow_C(const uint8* src_argb, int src_stride_argb,
                    uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  BG24ToARGBRow_C(src_argb, row, pix);
  BG24ToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

void RAWToUVRow_C(const uint8* src_argb, int src_stride_argb,
                  uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RAWToARGBRow_C(src_argb, row, pix);
  RAWToARGBRow_C(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

static inline int RGBToY(uint8 r, uint8 g, uint8 b) {
  return (( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
}

static inline int RGBToU(uint8 r, uint8 g, uint8 b) {
  return ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
}
static inline int RGBToV(uint8 r, uint8 g, uint8 b) {
  return ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
}

#define MAKEROWY(NAME,R,G,B) \
void NAME ## ToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {       \
  for (int x = 0; x < width; ++x) {                                            \
    dst_y[0] = RGBToY(src_argb0[R], src_argb0[G], src_argb0[B]);               \
    src_argb0 += 4;                                                            \
    dst_y += 1;                                                                \
  }                                                                            \
}                                                                              \
void NAME ## ToUVRow_C(const uint8* src_rgb0, int src_stride_rgb,              \
                       uint8* dst_u, uint8* dst_v, int width) {                \
  const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           \
  for (int x = 0; x < width - 1; x += 2) {                                     \
    uint8 ab = (src_rgb0[B] + src_rgb0[B + 4] +                                \
               src_rgb1[B] + src_rgb1[B + 4]) >> 2;                            \
    uint8 ag = (src_rgb0[G] + src_rgb0[G + 4] +                                \
               src_rgb1[G] + src_rgb1[G + 4]) >> 2;                            \
    uint8 ar = (src_rgb0[R] + src_rgb0[R + 4] +                                \
               src_rgb1[R] + src_rgb1[R + 4]) >> 2;                            \
    dst_u[0] = RGBToU(ar, ag, ab);                                             \
    dst_v[0] = RGBToV(ar, ag, ab);                                             \
    src_rgb0 += 8;                                                             \
    src_rgb1 += 8;                                                             \
    dst_u += 1;                                                                \
    dst_v += 1;                                                                \
  }                                                                            \
  if (width & 1) {                                                             \
    uint8 ab = (src_rgb0[B] + src_rgb1[B]) >> 1;                               \
    uint8 ag = (src_rgb0[G] + src_rgb1[G]) >> 1;                               \
    uint8 ar = (src_rgb0[R] + src_rgb1[R]) >> 1;                               \
    dst_u[0] = RGBToU(ar, ag, ab);                                             \
    dst_v[0] = RGBToV(ar, ag, ab);                                             \
  }                                                                            \
}

MAKEROWY(ARGB,2,1,0)
MAKEROWY(BGRA,1,2,3)
MAKEROWY(ABGR,0,1,2)

#if defined(HAS_RAWTOYROW_SSSE3)

void RGB24ToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  BG24ToARGBRow_SSSE3(src_argb, row, pix);
  ARGBToYRow_SSSE3(row, dst_y, pix);
}

void RAWToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride]);
  RAWToARGBRow_SSSE3(src_argb, row, pix);
  ARGBToYRow_SSSE3(row, dst_y, pix);
}

#endif

#if defined(HAS_RAWTOUVROW_SSSE3)
#if defined(HAS_ARGBTOUVROW_SSSE3)
void RGB24ToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                        uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  BG24ToARGBRow_SSSE3(src_argb, row, pix);
  BG24ToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}

void RAWToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RAWToARGBRow_SSSE3(src_argb, row, pix);
  RAWToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_SSSE3(row, kMaxStride, dst_u, dst_v, pix);
}

#else

void RGB24ToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                        uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  BG24ToARGBRow_SSSE3(src_argb, row, pix);
  BG24ToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

void RAWToUVRow_SSSE3(const uint8* src_argb, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int pix) {
  SIMD_ALIGNED(uint8 row[kMaxStride * 2]);
  RAWToARGBRow_SSSE3(src_argb, row, pix);
  RAWToARGBRow_SSSE3(src_argb + src_stride_argb, row + kMaxStride, pix);
  ARGBToUVRow_C(row, kMaxStride, dst_u, dst_v, pix);
}

#endif
#endif

void I400ToARGBRow_C(const uint8* src_y, uint8* dst_argb, int pix) {
  // Copy a Y to RGB.
  for (int x = 0; x < pix; ++x) {
    uint8 y = src_y[0];
    dst_argb[2] = dst_argb[1] = dst_argb[0] = y;
    dst_argb[3] = 255u;
    dst_argb += 4;
    ++src_y;
  }
}

// C reference code that mimic the YUV assembly.
#define packuswb(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))
#define paddsw(x, y) (((x) + (y)) < -32768 ? -32768 : \
    (((x) + (y)) > 32767 ? 32767 : ((x) + (y))))

static inline void YuvPixel(uint8 y,
                            uint8 u,
                            uint8 v,
                            uint8* rgb_buf,
                            int ashift,
                            int rshift,
                            int gshift,
                            int bshift) {

  int b = kCoefficientsRgbY[256+u][0];
  int g = kCoefficientsRgbY[256+u][1];
  int r = kCoefficientsRgbY[256+u][2];
  int a = kCoefficientsRgbY[256+u][3];

  b = paddsw(b, kCoefficientsRgbY[512+v][0]);
  g = paddsw(g, kCoefficientsRgbY[512+v][1]);
  r = paddsw(r, kCoefficientsRgbY[512+v][2]);
  a = paddsw(a, kCoefficientsRgbY[512+v][3]);

  b = paddsw(b, kCoefficientsRgbY[y][0]);
  g = paddsw(g, kCoefficientsRgbY[y][1]);
  r = paddsw(r, kCoefficientsRgbY[y][2]);
  a = paddsw(a, kCoefficientsRgbY[y][3]);

  b >>= 6;
  g >>= 6;
  r >>= 6;
  a >>= 6;

  *reinterpret_cast<uint32*>(rgb_buf) = (packuswb(b) << bshift) |
                                        (packuswb(g) << gshift) |
                                        (packuswb(r) << rshift) |
                                        (packuswb(a) << ashift);
}

void FastConvertYUVToARGBRow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 24, 16, 8, 0);
    YuvPixel(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 4, 24, 16, 8, 0);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 24, 16, 8, 0);
  }
}

void FastConvertYUVToBGRARow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 0, 8, 16, 24);
    YuvPixel(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 4, 0, 8, 16, 24);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf, 0, 8, 16, 24);
  }
}

void FastConvertYUVToABGRRow_C(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* rgb_buf,
                               int width) {
  for (int x = 0; x < width - 1; x += 2) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 24, 0, 8, 16);
    YuvPixel(y_buf[1], u_buf[0], v_buf[0], rgb_buf + 4, 24, 0, 8, 16);
    y_buf += 2;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 8;  // Advance 2 pixels.
  }
  if (width & 1) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf + 0, 24, 0, 8, 16);
  }
}

void FastConvertYUV444ToARGBRow_C(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgb_buf,
                                   int width) {
  for (int x = 0; x < width; ++x) {
    YuvPixel(y_buf[0], u_buf[0], v_buf[0], rgb_buf, 24, 16, 8, 0);
    y_buf += 1;
    u_buf += 1;
    v_buf += 1;
    rgb_buf += 4;  // Advance 1 pixel.
  }
}

void FastConvertYToARGBRow_C(const uint8* y_buf,
                             uint8* rgb_buf,
                             int width) {
  for (int x = 0; x < width; ++x) {
    YuvPixel(y_buf[0], 128, 128, rgb_buf, 24, 16, 8, 0);
    y_buf += 1;
    rgb_buf += 4;  // Advance 1 pixel.
  }
}

}  // extern "C"

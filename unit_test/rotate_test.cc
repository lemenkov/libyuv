/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "unit_test.h"
#include "rotate.h"
#include <stdlib.h>

using namespace libyuv;

void print_array(uint8 *array, int w, int h) {
  int i, j;

  for (i = 0; i < h; ++i) {
    for (j = 0; j < w; ++j)
      printf("%4d", array[i*w + j]);

    printf("\n");
  }
}

TEST_F(libyuvTest, Transpose) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 8; iw < _rotate_max_w && !err; ++iw)
    for (ih = 8; ih < _rotate_max_h && !err; ++ih) {
      int i;
      uint8 *input;
      uint8 *output_1;
      uint8 *output_2;

      ow = ih;
      oh = iw;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_1 = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_2 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));

      for (i = 0; i < iw*ih; ++i) {
        input[i] = i;
        output_1[i] = 0;
        output_2[i] = 0;
      }

      Transpose(input,    iw, output_1, ow, iw, ih);
      Transpose(output_1, ow, output_2, oh, ow, oh);

      for (i = 0; i < iw*ih; ++i) {
        if (input[i] != output_2[i])
          err++;
      }

      if (err) {
        printf("input %dx%d \n", iw, ih);
        print_array(input, iw, ih);

        printf("transpose 1\n");
        print_array(output_1, ow, oh);

        printf("transpose 2\n");
        print_array(output_2, iw, ih);
      }

      free(input);
      free(output_1);
      free(output_2);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate90) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 8; iw < _rotate_max_w && !err; ++iw)
    for (ih = 8; ih < _rotate_max_h && !err; ++ih) {
      int i;
      uint8 *input;
      uint8 *output_0;
      uint8 *output_90;
      uint8 *output_180;
      uint8 *output_270;

      ow = ih;
      oh = iw;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_90 = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_180 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_270 = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));

      for (i = 0; i < iw*ih; ++i) {
        input[i] = i;
        output_0[i] = 0;
        output_90[i] = 0;
        output_180[i] = 0;
        output_270[i] = 0;
      }

      Rotate90(input,      iw, output_90,  ow, iw, ih);
      Rotate90(output_90,  ow, output_180, oh, ow, oh);
      Rotate90(output_180, oh, output_270, ow, oh, ow);
      Rotate90(output_270, ow, output_0,   iw, ow, oh);

      for (i = 0; i < iw*ih; ++i) {
        if (input[i] != output_0[i])
          err++;
      }

      if (err) {
        printf("input %dx%d \n", iw, ih);
        print_array(input, iw, ih);

        printf("output 90\n");
        print_array(output_90, ow, oh);

        printf("output 180\n");
        print_array(output_180, iw, ih);

        printf("output 270\n");
        print_array(output_270, ow, oh);

        printf("output 0\n");
        print_array(output_0, iw, ih);
      }

      free(input);
      free(output_0);
      free(output_90);
      free(output_180);
      free(output_270);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate90Deinterleave) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 16; iw < _rotate_max_w && !err; iw += 2)
    for (ih = 8; ih < _rotate_max_h && !err; ++ih) {
      int i;
      uint8 *input;
      uint8 *output_0_u;
      uint8 *output_0_v;
      uint8 *output_90_u;
      uint8 *output_90_v;
      uint8 *output_180_u;
      uint8 *output_180_v;

      ow = ih;
      oh = iw>>1;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_0_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_90_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_90_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_180_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_180_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));

      for (i = 0; i < iw*ih; i +=2) {
        input[i] = i>>1;
        input[i+1] = -(i>>1);
      }

      for (i = 0; i < ow*oh; ++i) {
        output_0_u[i] = 0;
        output_0_v[i] = 0;
        output_90_u[i] = 0;
        output_90_v[i] = 0;
        output_180_u[i] = 0;
        output_180_v[i] = 0;
      }

      Rotate90_deinterleave(input, iw,
                            output_90_u, ow,
                            output_90_v, ow,
                            iw, ih);

      Rotate90(output_90_u, ow, output_180_u, oh, ow, oh);
      Rotate90(output_90_v, ow, output_180_v, oh, ow, oh);

      Rotate180(output_180_u, ow, output_0_u, ow, ow, oh);
      Rotate180(output_180_v, ow, output_0_v, ow, ow, oh);

      for (i = 0; i < ow*oh; ++i) {
        if (output_0_u[i] != (uint8)i)
          err++;
        if (output_0_v[i] != (uint8)(-i))
          err++;
      }

      if (err) {
        printf("input %dx%d \n", iw, ih);
        print_array(input, iw, ih);

        printf("output 90_u\n");
        print_array(output_90_u, ow, oh);

        printf("output 90_v\n");
        print_array(output_90_v, ow, oh);

        printf("output 180_u\n");
        print_array(output_180_u, oh, ow);

        printf("output 180_v\n");
        print_array(output_180_v, oh, ow);

        printf("output 0_u\n");
        print_array(output_0_u, oh, ow);

        printf("output 0_v\n");
        print_array(output_0_v, oh, ow);
      }

      free(input);
      free(output_0_u);
      free(output_0_v);
      free(output_90_u);
      free(output_90_v);
      free(output_180_u);
      free(output_180_v);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate180Deinterleave) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 16; iw < _rotate_max_w && !err; iw += 2)
    for (ih = 8; ih < _rotate_max_h && !err; ++ih) {
      int i;
      uint8 *input;
      uint8 *output_0_u;
      uint8 *output_0_v;
      uint8 *output_90_u;
      uint8 *output_90_v;
      uint8 *output_180_u;
      uint8 *output_180_v;

      ow = iw>>1;
      oh = ih;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_0_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_90_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_90_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_180_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_180_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));

      for (i = 0; i < iw*ih; i +=2) {
        input[i] = i>>1;
        input[i+1] = -(i>>1);
      }

      for (i = 0; i < ow*oh; ++i) {
        output_0_u[i] = 0;
        output_0_v[i] = 0;
        output_90_u[i] = 0;
        output_90_v[i] = 0;
        output_180_u[i] = 0;
        output_180_v[i] = 0;
      }

      Rotate180_deinterleave(input, iw,
                             output_180_u, ow,
                             output_180_v, ow,
                             iw, ih);

      Rotate90(output_180_u, ow, output_90_u, oh, ow, oh);
      Rotate90(output_180_v, ow, output_90_v, oh, ow, oh);

      Rotate90(output_90_u, oh, output_0_u, ow, oh, ow);
      Rotate90(output_90_v, oh, output_0_v, ow, oh, ow);

      for (i = 0; i < ow*oh; ++i) {
        if (output_0_u[i] != (uint8)i)
          err++;
        if (output_0_v[i] != (uint8)(-i))
          err++;
      }

      if (err) {
        printf("input %dx%d \n", iw, ih);
        print_array(input, iw, ih);

        printf("output 180_u\n");
        print_array(output_180_u, oh, ow);

        printf("output 180_v\n");
        print_array(output_180_v, oh, ow);

        printf("output 90_u\n");
        print_array(output_90_u, oh, ow);

        printf("output 90_v\n");
        print_array(output_90_v, oh, ow);

        printf("output 0_u\n");
        print_array(output_0_u, ow, oh);

        printf("output 0_v\n");
        print_array(output_0_v, ow, oh);
      }

      free(input);
      free(output_0_u);
      free(output_0_v);
      free(output_90_u);
      free(output_90_v);
      free(output_180_u);
      free(output_180_v);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate270Deinterleave) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 16; iw < _rotate_max_w && !err; iw += 2)
    for (ih = 8; ih < _rotate_max_h && !err; ++ih) {
      int i;
      uint8 *input;
      uint8 *output_0_u;
      uint8 *output_0_v;
      uint8 *output_270_u;
      uint8 *output_270_v;
      uint8 *output_180_u;
      uint8 *output_180_v;

      ow = ih;
      oh = iw>>1;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_0_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_270_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_270_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_180_u = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_180_v = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));

      for (i = 0; i < iw*ih; i +=2) {
        input[i] = i>>1;
        input[i+1] = -(i>>1);
      }

      for (i = 0; i < ow*oh; ++i) {
        output_0_u[i] = 0;
        output_0_v[i] = 0;
        output_270_u[i] = 0;
        output_270_v[i] = 0;
        output_180_u[i] = 0;
        output_180_v[i] = 0;
      }

      Rotate270_deinterleave(input, iw,
                             output_270_u, ow,
                             output_270_v, ow,
                             iw, ih);

      Rotate270(output_270_u, ow, output_180_u, oh, ow, oh);
      Rotate270(output_270_v, ow, output_180_v, oh, ow, oh);

      Rotate180(output_180_u, ow, output_0_u, ow, ow, oh);
      Rotate180(output_180_v, ow, output_0_v, ow, ow, oh);

      for (i = 0; i < ow*oh; ++i) {
        if (output_0_u[i] != (uint8)i)
          err++;
        if (output_0_v[i] != (uint8)(-i))
          err++;
      }

      if (err) {
        printf("input %dx%d \n", iw, ih);
        print_array(input, iw, ih);

        printf("output 270_u\n");
        print_array(output_270_u, ow, oh);

        printf("output 270_v\n");
        print_array(output_270_v, ow, oh);

        printf("output 180_u\n");
        print_array(output_180_u, oh, ow);

        printf("output 180_v\n");
        print_array(output_180_v, oh, ow);

        printf("output 0_u\n");
        print_array(output_0_u, oh, ow);

        printf("output 0_v\n");
        print_array(output_0_v, oh, ow);
      }

      free(input);
      free(output_0_u);
      free(output_0_v);
      free(output_270_u);
      free(output_270_v);
      free(output_180_u);
      free(output_180_v);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate180) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 8; iw < _rotate_max_w && !err; ++iw)
    for (ih = 8; ih < _rotate_max_h && !err; ++ih) {
      int i;
      uint8 *input;
      uint8 *output_0;
      uint8 *output_180;

      ow = iw;
      oh = ih;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_180 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));

      for (i = 0; i < iw*ih; ++i) {
        input[i] = i;
        output_0[i] = 0;
        output_180[i] = 0;
      }

      Rotate180(input,      iw, output_180, ow, iw, ih);
      Rotate180(output_180, ow, output_0,   iw, ow, oh);

      for (i = 0; i < iw*ih; ++i) {
        if (input[i] != output_0[i])
          err++;
      }

      if (err) {
        printf("input %dx%d \n", iw, ih);
        print_array(input, iw, ih);

        printf("output 180\n");
        print_array(output_180, iw, ih);

        printf("output 0\n");
        print_array(output_0, iw, ih);
      }

      free(input);
      free(output_0);
      free(output_180);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate270) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 8; iw < _rotate_max_w && !err; ++iw)
    for (ih = 8; ih < _rotate_max_h && !err; ++ih) {
      int i;
      uint8 *input;
      uint8 *output_0;
      uint8 *output_90;
      uint8 *output_180;
      uint8 *output_270;

      ow = ih;
      oh = iw;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_90 = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));
      output_180 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_270 = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));

      for (i = 0; i < iw*ih; ++i) {
        input[i] = i;
        output_0[i] = 0;
        output_90[i] = 0;
        output_180[i] = 0;
        output_270[i] = 0;
      }

      Rotate270(input,      iw, output_270, ow, iw, ih);
      Rotate270(output_270, ow, output_180, oh, ow, oh);
      Rotate270(output_180, oh, output_90,  ow, oh, ow);
      Rotate270(output_90,  ow, output_0,   iw, ow, oh);

      for (i = 0; i < iw*ih; ++i) {
        if (input[i] != output_0[i])
          err++;
      }

      if (err) {
        printf("input %dx%d \n", iw, ih);
        print_array(input, iw, ih);

        printf("output 270\n");
        print_array(output_270, ow, oh);

        printf("output 180\n");
        print_array(output_180, iw, ih);

        printf("output 90\n");
        print_array(output_90, ow, oh);

        printf("output 0\n");
        print_array(output_0, iw, ih);
      }

      free(input);
      free(output_0);
      free(output_90);
      free(output_180);
      free(output_270);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate90and270) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 16; iw < _rotate_max_w && !err; iw += 4)
    for (ih = 16; ih < _rotate_max_h && !err; ih += 4) {
      int i;
      uint8 *input;
      uint8 *output_0;
      uint8 *output_90;
      ow = ih;
      oh = iw;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_90 = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));

      for (i = 0; i < iw*ih; ++i) {
        input[i] = i;
        output_0[i] = 0;
        output_90[i] = 0;
      }

      Rotate90(input,     iw, output_90,  ow, iw, ih);
      Rotate270(output_90, ow, output_0,   iw, ow, oh);

      for (i = 0; i < iw*ih; ++i) {
        if (input[i] != output_0[i])
          err++;
      }

      if (err) {
        printf("intput %dx%d\n", iw, ih);
        print_array(input, iw, ih);

        printf("output \n");
        print_array(output_90, ow, oh);

        printf("output \n");
        print_array(output_0, iw, ih);
      }

      free(input);
      free(output_0);
      free(output_90);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate90Pitch) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 16; iw < _rotate_max_w && !err; iw += 4)
    for (ih = 16; ih < _rotate_max_h && !err; ih += 4) {
      int i;
      uint8 *input;
      uint8 *output_0;
      uint8 *output_90;
      ow = ih;
      oh = iw;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_90 = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));

      for (i = 0; i < iw*ih; ++i) {
        input[i] = i;
        output_0[i] = 0;
        output_90[i] = 0;
      }

      Rotate90(input,                        iw,
               output_90 + (ow>>1),              ow, iw>>1, ih>>1);
      Rotate90(input + (iw>>1),              iw,
               output_90 + (ow>>1) + ow*(oh>>1), ow, iw>>1, ih>>1);
      Rotate90(input + iw*(ih>>1),           iw,
               output_90,                        ow, iw>>1, ih>>1);
      Rotate90(input + (iw>>1) + iw*(ih>>1), iw,
               output_90 + ow*(oh>>1),           ow, iw>>1, ih>>1);

      Rotate270(output_90, ih, output_0,   iw, ow, oh);

      for (i = 0; i < iw*ih; ++i) {
        if (input[i] != output_0[i])
          err++;
      }

      if (err) {
        printf("intput %dx%d\n", iw, ih);
        print_array(input, iw, ih);

        printf("output \n");
        print_array(output_90, ow, oh);

        printf("output \n");
        print_array(output_0, iw, ih);
      }

      free(input);
      free(output_0);
      free(output_90);
    }

  EXPECT_EQ(0, err);
}

TEST_F(libyuvTest, Rotate270Pitch) {
  int iw, ih, ow, oh;
  int err = 0;

  for (iw = 16; iw < _rotate_max_w && !err; iw += 4)
    for (ih = 16; ih < _rotate_max_h && !err; ih += 4) {
      int i;
      uint8 *input;
      uint8 *output_0;
      uint8 *output_270;

      ow = ih;
      oh = iw;

      input = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_0 = static_cast<uint8*>(malloc(sizeof(uint8)*iw*ih));
      output_270 = static_cast<uint8*>(malloc(sizeof(uint8)*ow*oh));

      for (i = 0; i < iw*ih; ++i) {
        input[i] = i;
        output_270[i] = 0;
      }

      Rotate270(input,                        iw,
                output_270 + ow*(oh>>1),           ow, iw>>1, ih>>1);
      Rotate270(input + (iw>>1),              iw,
                output_270,                        ow, iw>>1, ih>>1);
      Rotate270(input + iw*(ih>>1),           iw,
                output_270 + (ow>>1) + ow*(oh>>1), ow, iw>>1, ih>>1);
      Rotate270(input + (iw>>1) + iw*(ih>>1), iw,
                output_270 + (ow>>1),              ow, iw>>1, ih>>1);

      Rotate90(output_270, ih, output_0,   iw, ow, oh);

      for (i = 0; i < iw*ih; ++i) {
        if (input[i] != output_0[i])
          err++;
      }

      if (err) {
        printf("intput %dx%d\n", iw, ih);
        print_array(input, iw, ih);

        printf("output \n");
        print_array(output_270, ow, oh);

        printf("output \n");
        print_array(output_0, iw, ih);
      }

      free(input);
      free(output_0);
      free(output_270);
    }

  EXPECT_EQ(0, err);
}

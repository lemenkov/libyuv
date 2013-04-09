/*
 *  Copyright 2013 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Convert an ARGB image to YUV.
// Usage: convert src_argb.raw dst_yuv.raw

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libyuv/convert.h"

// options
bool verbose = false;
int image_width = 0, image_height = 0;
int fileindex_org = 0;  // argv argument contains the original file name.
int fileindex_rec = 0;  // argv argument contains the reconstructed file name.
int num_rec = 0;  // Number of reconstructed images.
int num_skip_org = 0;  // Number of frames to skip in original.
int num_frames = 0;  // Number of frames to convert.

// Parse PYUV format. ie name.1920x800_24Hz_P420.yuv
bool ExtractResolutionFromFilename(const char* name,
                                   int* width_ptr,
                                   int* height_ptr) {
  // Isolate the .width_height. section of the filename by searching for a
  // dot or underscore followed by a digit.
  for (int i = 0; name[i]; ++i) {
    if ((name[i] == '.' || name[i] == '_') &&
        name[i + 1] >= '0' && name[i + 1] <= '9') {
      int n = sscanf(name + i + 1, "%dx%d", width_ptr, height_ptr);  // NOLINT
      if (2 == n) {
        return true;
      }
    }
  }
  return false;
}

void PrintHelp(const char * program) {
  printf("%s [-options] src_argb.raw dst_yuv.raw\n", program);
  printf(" -s <width> <height> .... specify resolution.  "
         "Optional if name contains\n"
         "                          resolution (ie. "
         "name.1920x800_24Hz_P420.yuv)\n");
  printf(" -skip <src_argb> ....... Number of frame to skip of src_argb\n");
  printf(" -frames <num> .......... Number of frames to convert\n");
  printf(" -v ..................... verbose\n");
  printf(" -h ..................... this help\n");
  exit(0);
}

void ParseOptions(int argc, const char* argv[]) {
  if (argc <= 1) PrintHelp(argv[0]);
  for (int c = 1; c < argc; ++c) {
    if (!strcmp(argv[c], "-v")) {
      verbose = true;
    } else if (!strcmp(argv[c], "-h") || !strcmp(argv[c], "-help")) {
      PrintHelp(argv[0]);
    } else if (!strcmp(argv[c], "-s") && c + 2 < argc) {
      image_width = atoi(argv[++c]);    // NOLINT
      image_height = atoi(argv[++c]);   // NOLINT
    } else if (!strcmp(argv[c], "-skip") && c + 1 < argc) {
      num_skip_org = atoi(argv[++c]);   // NOLINT
    } else if (!strcmp(argv[c], "-frames") && c + 1 < argc) {
      num_frames = atoi(argv[++c]);     // NOLINT
    } else if (argv[c][0] == '-') {
      fprintf(stderr, "Unknown option. %s\n", argv[c]);
    } else if (fileindex_org == 0) {
      fileindex_org = c;
    } else if (fileindex_rec == 0) {
      fileindex_rec = c;
      num_rec = 1;
    } else {
      ++num_rec;
    }
  }
  if (fileindex_org == 0 || fileindex_rec == 0) {
    fprintf(stderr, "Missing filenames\n");
    PrintHelp(argv[0]);
  }
  if (num_skip_org < 0) {
    fprintf(stderr, "Skipped frames incorrect\n");
    PrintHelp(argv[0]);
  }
  if (num_frames < 0) {
    fprintf(stderr, "Number of frames incorrect\n");
    PrintHelp(argv[0]);
  }
  if (image_width <= 0 || image_height <=0) {
    int org_width, org_height;
    int rec_width, rec_height;
    bool org_res_avail = ExtractResolutionFromFilename(argv[fileindex_org],
                                                       &org_width,
                                                       &org_height);
    bool rec_res_avail = ExtractResolutionFromFilename(argv[fileindex_rec],
                                                       &rec_width,
                                                       &rec_height);
    if (org_res_avail) {
      if (rec_res_avail) {
        if ((org_width == rec_width) && (org_height == rec_height)) {
          image_width = org_width;
          image_height = org_height;
        } else {
          // TODO(fbarchard): Apply scaling.
          fprintf(stderr, "Sequences have different resolutions.\n");
          PrintHelp(argv[0]);
        }
      } else {
        image_width = org_width;
        image_height = org_height;
      }
    } else if (rec_res_avail) {
      image_width = rec_width;
      image_height = rec_height;
    } else {
      fprintf(stderr, "Missing dimensions.\n");
      PrintHelp(argv[0]);
    }
  }
}

int main(int argc, const char* argv[]) {
  ParseOptions(argc, argv);

  // Open original file (first file argument)
  FILE* const file_org = fopen(argv[fileindex_org], "rb");
  if (file_org == NULL) {
    fprintf(stderr, "Cannot open %s\n", argv[fileindex_org]);
    exit(1);
  }

  // Open all files to convert to
  FILE** file_rec = new FILE* [num_rec];
  memset(file_rec, 0, num_rec * sizeof(FILE*)); // NOLINT
  for (int cur_rec = 0; cur_rec < num_rec; ++cur_rec) {
    file_rec[cur_rec] = fopen(argv[fileindex_rec + cur_rec], "wb");
    if (file_rec[cur_rec] == NULL) {
      fprintf(stderr, "Cannot open %s\n", argv[fileindex_rec + cur_rec]);
      fclose(file_org);
      for (int i = 0; i < cur_rec; ++i) {
        fclose(file_rec[i]);
      }
      delete[] file_rec;
      exit(1);
    }
  }

  const int org_size = image_width * image_height * 4;  // ARGB
  const int y_size = image_width * image_height;
  const int uv_size = (image_width >> 1) * (image_height >> 1);
  const size_t total_size = y_size + 2 * uv_size;
#if defined(_MSC_VER)
  _fseeki64(file_org,
            static_cast<__int64>(num_skip_org) *
            static_cast<__int64>(org_size), SEEK_SET);
#else
  fseek(file_org, num_skip_org * total_size, SEEK_SET);
#endif

  uint8* const ch_org = new uint8[org_size];
  uint8* const ch_rec = new uint8[total_size];
  if (ch_org == NULL || ch_rec == NULL) {
    fprintf(stderr, "No memory available\n");
    fclose(file_org);
    for (int i = 0; i < num_rec; ++i) {
      fclose(file_rec[i]);
    }
    delete[] ch_org;
    delete[] ch_rec;
    delete[] file_rec;
    exit(1);
  }

  if (verbose) {
    printf("Size: %dx%d\n", image_width, image_height);
  }

  int number_of_frames;
  for (number_of_frames = 0; ; ++number_of_frames) {
    if (num_frames && number_of_frames >= num_frames)
      break;

    size_t bytes_org = fread(ch_org, sizeof(uint8), org_size, file_org);
    if (bytes_org < total_size)
      break;

    for (int cur_rec = 0; cur_rec < num_rec; ++cur_rec) {
      int half_width = (image_width + 1) / 2;
      int half_height = (image_height + 1) / 2;
      libyuv::ARGBToI420(ch_org, image_width * 4,
                         ch_rec, image_width,
                         ch_rec + image_width * image_height, half_width,
                         ch_rec + image_width * image_height +
                             half_width * half_height,  half_width,
                         image_width, image_height);
      size_t bytes_rec = fwrite(ch_rec, sizeof(uint8),
                                total_size, file_rec[cur_rec]);
      if (bytes_rec < total_size)
        break;

      if (verbose) {
        printf("%5d", number_of_frames);
      }
      if (verbose) {
        printf("\t%s", argv[fileindex_rec + cur_rec]);
        printf("\n");
      }
    }
  }

  fclose(file_org);
  for (int cur_rec = 0; cur_rec < num_rec; ++cur_rec) {
    fclose(file_rec[cur_rec]);
  }
  delete[] ch_org;
  delete[] ch_rec;
  delete[] file_rec;
  return 0;
}

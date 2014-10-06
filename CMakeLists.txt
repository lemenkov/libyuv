cmake_minimum_required(VERSION 2.8)

# Basic CMakeLists for libyuv, compiles w/o the jpeg library
# created for "roxlu build system" to compile libyuv on windows

set(ly_base_dir ${CMAKE_CURRENT_LIST_DIR})
set(ly_src_dir ${ly_base_dir}/source/)
set(ly_inc_dir ${ly_base_dir}/include)
set(ly_lib_name "yuv")

set(ly_source_files
  ${ly_src_dir}/compare.cc
  ${ly_src_dir}/compare_common.cc
  ${ly_src_dir}/compare_neon.cc
  ${ly_src_dir}/compare_posix.cc
  ${ly_src_dir}/compare_win.cc
  ${ly_src_dir}/convert.cc
  ${ly_src_dir}/convert_argb.cc
  ${ly_src_dir}/convert_from.cc
  ${ly_src_dir}/convert_from_argb.cc
  ${ly_src_dir}/convert_to_argb.cc
  ${ly_src_dir}/convert_to_i420.cc
  ${ly_src_dir}/cpu_id.cc
  ${ly_src_dir}/format_conversion.cc
  ${ly_src_dir}/mjpeg_decoder.cc
  ${ly_src_dir}/mjpeg_validate.cc
  ${ly_src_dir}/planar_functions.cc
  ${ly_src_dir}/rotate.cc
  ${ly_src_dir}/rotate_argb.cc
  ${ly_src_dir}/rotate_mips.cc
  ${ly_src_dir}/rotate_neon.cc
  ${ly_src_dir}/row_any.cc
  ${ly_src_dir}/row_common.cc
  ${ly_src_dir}/row_mips.cc
  ${ly_src_dir}/row_neon.cc
  ${ly_src_dir}/row_posix.cc
  ${ly_src_dir}/row_win.cc
  ${ly_src_dir}/scale.cc
  ${ly_src_dir}/scale_argb.cc
  ${ly_src_dir}/scale_common.cc
  ${ly_src_dir}/scale_mips.cc
  ${ly_src_dir}/scale_neon.cc
  ${ly_src_dir}/scale_win.cc
  ${ly_src_dir}/video_common.cc
#  ${ly_src_dir}/convert_jpeg.cc
)

if (WIN32)
  list(APPEND ly_source_files
    ${ly_src_dir}/scale_win.cc
    )
endif()

set(ly_header_files
  ${ly_inc_dir}/libyuv/basic_types.h
  ${ly_inc_dir}/libyuv/compare.h
  ${ly_inc_dir}/libyuv/convert.h
  ${ly_inc_dir}/libyuv/convert_argb.h
  ${ly_inc_dir}/libyuv/convert_from.h
  ${ly_inc_dir}/libyuv/convert_from_argb.h
  ${ly_inc_dir}/libyuv/cpu_id.h
  ${ly_inc_dir}/libyuv/format_conversion.h
  ${ly_inc_dir}/libyuv/planar_functions.h
  ${ly_inc_dir}/libyuv/rotate.h
  ${ly_inc_dir}/libyuv/rotate_argb.h
  ${ly_inc_dir}/libyuv/row.h
  ${ly_inc_dir}/libyuv/scale.h
  ${ly_inc_dir}/libyuv/scale_argb.h
  ${ly_inc_dir}/libyuv/scale_row.h
  ${ly_inc_dir}/libyuv/version.h
  ${ly_inc_dir}/libyuv/video_common.h
  ${ly_inc_dir}/libyuv/mjpeg_decoder.h
)

include_directories(${ly_inc_dir})

add_library(${ly_lib_name} STATIC ${ly_source_files})

install(TARGETS ${ly_lib_name} DESTINATION lib)
install(FILES ${ly_header_files} DESTINATION include/libyuv)
install(FILES ${ly_inc_dir}/libyuv.h DESTINATION include/)


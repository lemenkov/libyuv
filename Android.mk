# This is the Android makefile for libyuv so that we can
# build it with the Android NDK.
ifneq ($(TARGET_ARCH),x86)

LOCAL_PATH := $(call my-dir)

common_SRC_FILES := \
    source/compare.cc \
    source/convert.cc \
    source/convert_from.cc \
    source/cpu_id.cc \
    source/format_conversion.cc \
    source/planar_functions.cc \
    source/rotate.cc \
    source/row_common.cc \
    source/row_posix.cc \
    source/scale.cc \
    source/scale_argb.cc \
    source/video_common.cc
# For Neon support, add .neon to all filenames and the following
#    source/rotate_neon.cc
#    source/row_neon.cc

common_CFLAGS := -Wall -fexceptions

common_C_INCLUDES = $(LOCAL_PATH)/include

# For the device
# =====================================================
# Device static library

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_NDK_VERSION := 5
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := stlport_static

LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_C_INCLUDES += $(common_C_INCLUDES)

LOCAL_MODULE:= libyuv_static
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

endif

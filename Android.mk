# This is the Android makefile for libyuv for both platform and NDK.
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
    source/compare.cc \
    source/convert.cc \
    source/convert_from.cc \
    source/convert_from_argb.cc \
    source/cpu_id.cc \
    source/format_conversion.cc \
    source/planar_functions.cc \
    source/rotate.cc \
    source/row_common.cc \
    source/row_posix.cc \
    source/scale.cc \
    source/scale_argb.cc \
    source/video_common.cc

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_CFLAGS += -DLIBYUV_NEON
    LOCAL_SRC_FILES += \
        source/compare_neon.cc.neon \
        source/rotate_neon.cc.neon \
        source/row_neon.cc.neon \
        source/scale_neon.cc.neon
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_MODULE := libyuv_static
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)


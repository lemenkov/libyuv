# This is a generic makefile for libyuv for gcc.
# make -f linux.mk CXX=clang++

CXX?=g++
CXXFLAGS?=-O2 -fomit-frame-pointer
CXXFLAGS+=-Iinclude/

LOCAL_OBJ_FILES := \
    source/compare.o           \
    source/compare_common.o    \
    source/compare_posix.o     \
    source/convert.o           \
    source/convert_argb.o      \
    source/convert_from.o      \
    source/convert_from_argb.o \
    source/convert_to_argb.o   \
    source/convert_to_i420.o   \
    source/cpu_id.o            \
    source/planar_functions.o  \
    source/rotate.o            \
    source/rotate_argb.o       \
    source/rotate_mips.o       \
    source/row_any.o           \
    source/row_common.o        \
    source/row_mips.o          \
    source/row_posix.o         \
    source/scale.o             \
    source/scale_argb.o        \
    source/scale_common.o      \
    source/scale_mips.o        \
    source/scale_posix.o       \
    source/video_common.o

.cc.o:
	$(CXX) -c $(CXXFLAGS) $*.cc -o $*.o

all: libyuv.a convert

libyuv.a: $(LOCAL_OBJ_FILES)
	$(AR) $(ARFLAGS) $@ $(LOCAL_OBJ_FILES)

# A test utility that uses libyuv conversion.
convert: util/convert.cc libyuv.a
	$(CXX) $(CXXFLAGS) -Iutil/ -o $@ util/convert.cc libyuv.a

clean:
	/bin/rm -f source/*.o *.ii *.s libyuv.a convert


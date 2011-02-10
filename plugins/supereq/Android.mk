LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := supereq

LOCAL_SRC_FILES += supereq.c supereq.h Equ.cpp Fftsg_fl.cpp

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../.. -O2 -std=c99 -I$(LOCAL_PATH)/../../plugins/supereq/ffmpeg_fft

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
#	LOCAL_SRC_FILES += ff_rdft.c\
#		ffmpeg_fft/libavutil/mem.c\
#		ffmpeg_fft/libavutil/mathematics.c\
#		ffmpeg_fft/libavutil/rational.c\
#		ffmpeg_fft/libavutil/intfloat_readwrite.c\
#		ffmpeg_fft/libavcodec/dct.c\
#		ffmpeg_fft/libavcodec/arm/fft_init_arm.c\
#		ffmpeg_fft/libavcodec/avfft.c\
#		ffmpeg_fft/libavcodec/fft.c\
#		ffmpeg_fft/libavcodec/dct32.c\
#		ffmpeg_fft/libavcodec/rdft.c\
#		ffmpeg_fft/libavcodec/arm/asm.S\
#		ffmpeg_fft/libavcodec/arm/fft_neon.S\
#		ffmpeg_fft/libavcodec/arm/simple_idct_neon.S\
#		ffmpeg_fft/libavcodec/arm/rdft_neon.S

	LOCAL_CFLAGS += -mfpu=neon -mfloat-abi=softfp -fpic -fno-signed-zeros -DUSE_OOURA
else
	LOCAL_CFLAGS += -DUSE_OOURA
endif

LOCAL_ARM_MODE := arm

LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)

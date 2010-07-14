LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libsamplerate

LOCAL_SRC_FILES +=\
	src/samplerate.c\
	src/src_linear.c\
	src/src_sinc.c\
	src/src_zoh.c

LOCAL_CFLAGS += -std=c99

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)



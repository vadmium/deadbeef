LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := supereq

LOCAL_SRC_FILES += supereq.c supereq.h Equ.cpp Fftsg_fl.cpp

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../.. -O2 -std=c99

LOCAL_ARM_MODE := arm

LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)

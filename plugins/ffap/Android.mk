LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := ffap

LOCAL_SRC_FILES += ffap.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../.. -std=c99 -O2

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)


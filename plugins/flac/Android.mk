LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := flac

LOCAL_SRC_FILES += flac.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../android/jni/flaclib/include -std=c99

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)


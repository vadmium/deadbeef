LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mpgmad

LOCAL_SRC_FILES += mpgmad.c

LOCAL_STATIC_LIBRARIES := mad

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../android/jni/libmad -std=c99 -O2

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)



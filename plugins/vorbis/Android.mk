LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := vorbis

LOCAL_SRC_FILES += vorbis.c vcedit.c

LOCAL_STATIC_LIBRARIES := tremorlib ogglib

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../android/jni/libogg/include -I$(LOCAL_PATH)/../../android/jni/libvorbis/include -std=c99 -O2

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)


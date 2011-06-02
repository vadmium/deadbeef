LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := sndfile

LOCAL_SRC_FILES += sndfile.c

LOCAL_STATIC_LIBRARIES := sf

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../android.freeplugins/jni/libsndfile/src -O2 --std=c99 -DHAVE_CONFIG_H

LOCAL_ARM_MODE := arm
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)


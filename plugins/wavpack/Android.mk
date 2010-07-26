LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := wavpack

LOCAL_SRC_FILES += wavpack.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../android/jni/libwv/include -std=c99
#LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../android/jni/tinywv -std=c99 -DTINYWV

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)



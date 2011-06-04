LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := ogglib

LOCAL_SRC_FILES += src/framing.c src/bitwise.c

LOCAL_CFLAGS += -O2 -I$(LOCAL_PATH)/include -I$(LOCAL_PATH)/../include

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)




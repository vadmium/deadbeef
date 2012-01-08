LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := m3u

LOCAL_SRC_FILES += m3u.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../.. -std=c99 -O2

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)


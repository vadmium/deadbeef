LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_STATIC_LIBRARIES := curl
LOCAL_MODULE := vfs_curl

LOCAL_SRC_FILES += vfs_curl.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../.. -O2 -I$(LOCAL_PATH)/../../.. -I$(LOCAL_PATH)/../../android/jni/curl/include/

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)


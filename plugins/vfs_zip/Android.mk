LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := vfs_zip

LOCAL_SRC_FILES += vfs_zip.c

LOCAL_STATIC_LIBRARIES := zip z

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../.. -I$(LOCAL_PATH)/../../android/jni/libzip/lib -O2 -std=c99

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)



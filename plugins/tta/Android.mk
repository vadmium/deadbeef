LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := tta

LOCAL_SRC_FILES += ttadec.c ttaplug.c

LOCAL_CFLAGS += -std=c99

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)



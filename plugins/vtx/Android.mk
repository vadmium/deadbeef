LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := vtx

LOCAL_SRC_FILES += vtx.c ay8912.c lh5dec.c vtxfile.c

LOCAL_CFLAGS += -std=c99 -O2

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)


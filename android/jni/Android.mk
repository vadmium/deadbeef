TOP_LOCAL_PATH := $(call my-dir)
include $(call all-subdir-makefiles)
LOCAL_PATH      := $(TOP_LOCAL_PATH)

include $(CLEAR_VARS)

CORE_PATH=../..

LOCAL_STATIC_LIBRARIES := core gme dumb vtx adplug sid musepack flac flaclib ffap tta mpgmad mad wavpack wv

LOCAL_CFLAGS    += -O2 -finline-functions -I$(LOCAL_PATH) -I$(LOCAL_PATH)/../.. -std=c99

LOCAL_MODULE    := deadbeef
LOCAL_SRC_FILES := deadbeef-jni.c

LOCAL_ARM_MODE := arm
LOCAL_LDLIBS    := -llog -lz

include $(BUILD_SHARED_LIBRARY)

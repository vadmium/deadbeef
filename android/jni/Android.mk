TOP_LOCAL_PATH := $(call my-dir)
include $(call all-subdir-makefiles)
LOCAL_PATH      := $(TOP_LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_STATIC_LIBRARIES := core

LOCAL_CFLAGS    += -O2 -finline-functions -I$(LOCAL_PATH) -I$(LOCAL_PATH)/../.. -std=c99
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_NEON := true
	LOCAL_CFLAGS += -mfpu=neon -DUSE_NEON
endif

LOCAL_MODULE    := deadbeef
LOCAL_SRC_FILES := deadbeef-jni.c equalizer.c

LOCAL_ARM_MODE := arm
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)

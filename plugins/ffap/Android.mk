LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_STATIC_LIBRARIES := cpufeatures

LOCAL_MODULE := ffap

LOCAL_SRC_FILES += ffap.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../.. -std=c99 -O2
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_NEON := true
	LOCAL_SRC_FILES += int_neon.S
	LOCAL_CFLAGS += -mfpu=neon -DARCH_ARM=1
endif

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)
$(call import-module,cpufeatures)


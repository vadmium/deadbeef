LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mpgmad

LOCAL_SRC_FILES += mpgmad.c

LOCAL_STATIC_LIBRARIES := mad

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../android.freeplugins/jni/libmad -std=c99 -O2
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_CFLAGS += -mfpu=neon -mfloat-abi=softfp -fpic -fno-signed-zeros
endif

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)



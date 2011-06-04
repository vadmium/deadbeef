LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := wv

LOCAL_SRC_FILES += \
	src/bits.c\
	src/extra1.c\
	src/extra2.c\
	src/float.c\
	src/metadata.c\
	src/pack.c\
	src/tags.c\
	src/unpack3.c\
	src/unpack.c\
	src/words.c\
	src/wputils.c\
	src/arml.S\
	src/arm.S

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../android/jni/libwv/include -DCPU_ARM -finline-functions -std=c99 -O2

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)




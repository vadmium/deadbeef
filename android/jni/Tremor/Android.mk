LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := tremorlib

LOCAL_SRC_FILES +=\
	block.c\
	codebook.c\
	floor0.c\
	floor1.c\
	info.c\
	iseeking_example.c\
	ivorbisfile_example.c\
	mapping0.c\
	mdct.c\
	registry.c\
	res012.c\
	sharedbook.c\
	synthesis.c\
	vorbisfile.c\
	window.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../libogg/include -O2 -D_ARM_ASSEM_

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)





LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := vorbislib

LOCAL_SRC_FILES +=\
lib/analysis.c\
lib/floor1.c\
lib/psy.c\
lib/tone.c\
lib/barkmel.c\
lib/synthesis.c\
lib/res0.c\
lib/floor0.c\
lib/window.c\
lib/smallft.c\
lib/vorbisfile.c\
lib/mdct.c\
lib/info.c\
lib/mapping0.c\
lib/bitrate.c\
lib/envelope.c\
lib/lookup.c\
lib/sharedbook.c\
lib/lsp.c\
lib/codebook.c\
lib/registry.c\
lib/block.c\
lib/lpc.c

LOCAL_CFLAGS += -O2 -I$(LOCAL_PATH)/include -I$(LOCAL_PATH)/../include -I$(LOCAL_PATH)/../libogg/include -I$(LOCAL_PATH)/lib

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)




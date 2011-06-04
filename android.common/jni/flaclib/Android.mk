LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := flaclib

LOCAL_SRC_FILES +=\
	libFLAC/bitmath.c\
	libFLAC/bitreader.c\
	libFLAC/bitwriter.c\
	libFLAC/cpu.c\
	libFLAC/crc.c\
	libFLAC/fixed.c\
	libFLAC/float.c\
	libFLAC/format.c\
	libFLAC/lpc.c\
	libFLAC/md5.c\
	libFLAC/memory.c\
	libFLAC/metadata_iterators.c\
	libFLAC/metadata_object.c\
	libFLAC/ogg_decoder_aspect.c\
	libFLAC/ogg_encoder_aspect.c\
	libFLAC/ogg_helper.c\
	libFLAC/ogg_mapping.c\
	libFLAC/stream_decoder.c\
	libFLAC/stream_encoder.c\
	libFLAC/stream_encoder_framing.c\
	libFLAC/window.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/libFLAC/include -I$(LOCAL_PATH)/include -I$(LOCAL_PATH)/../libogg/include -DVERSION=\"1.2.1\" -O2

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)




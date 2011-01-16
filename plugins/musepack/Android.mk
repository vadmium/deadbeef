LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := musepack

LOCAL_SRC_FILES +=\
	musepack.c\
	huffman.c\
	mpc_bits_reader.c\
	mpc_decoder.c\
	mpc_demux.c\
	mpc_reader.c\
	requant.c\
	streaminfo.c\
	synth_filter.c\
	crc32.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../.. -O2

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)



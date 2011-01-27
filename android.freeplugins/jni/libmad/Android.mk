LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mad

LOCAL_SRC_FILES += bit.c decoder.c fixed.c frame.c huffman.c layer12.c layer3.c minimad.c stream.c synth.c timer.c version.c imdct_l_arm.S

LOCAL_CFLAGS += -DFPM_ARM -DASO_IMDCT -O2

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)




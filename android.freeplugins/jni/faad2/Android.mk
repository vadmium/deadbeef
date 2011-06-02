LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := faad2

LOCAL_SRC_FILES += \
	libfaad/bits.c\
	libfaad/cfft.c\
	libfaad/common.c\
	libfaad/decoder.c\
	libfaad/drc.c\
	libfaad/drm_dec.c\
	libfaad/error.c\
	libfaad/filtbank.c\
	libfaad/hcr.c\
	libfaad/huffman.c\
	libfaad/ic_predict.c\
	libfaad/is.c\
	libfaad/lt_predict.c\
	libfaad/mdct.c\
	libfaad/mp4.c\
	libfaad/ms.c\
	libfaad/output.c\
	libfaad/pns.c\
	libfaad/ps_dec.c\
	libfaad/ps_syntax.c\
	libfaad/pulse.c\
	libfaad/rvlc.c\
	libfaad/sbr_dct.c\
	libfaad/sbr_dec.c\
	libfaad/sbr_e_nf.c\
	libfaad/sbr_fbt.c\
	libfaad/sbr_hfadj.c\
	libfaad/sbr_hfgen.c\
	libfaad/sbr_huff.c\
	libfaad/sbr_qmf.c\
	libfaad/sbr_syntax.c\
	libfaad/sbr_tf_grid.c\
	libfaad/specrec.c\
	libfaad/ssr.c\
	libfaad/ssr_fb.c\
	libfaad/ssr_ipqf.c\
	libfaad/syntax.c\
	libfaad/tns.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/include -I$(LOCAL_PATH)/libfaad -std=c99 -O2 -DHAVE_CONFIG_H

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)




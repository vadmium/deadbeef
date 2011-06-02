LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := sf

LOCAL_SRC_FILES += \
	src/svx.c\
	src/gsm610.c\
	src/mat4.c\
	src/test_audio_detect.c\
	src/nist.c\
	src/wav_w64.c\
	src/mpc2k.c\
	src/pcm.c\
	src/broadcast.c\
	src/test_file_io.c\
	src/mat5.c\
	src/test_broadcast_var.c\
	src/aiff.c\
	src/dither.c\
	src/dwvw.c\
	src/test_strncpy_crlf.c\
	src/test_float.c\
	src/audio_detect.c\
	src/test_conversions.c\
	src/command.c\
	src/htk.c\
	src/avr.c\
	src/test_log_printf.c\
	src/test_main.c\
	src/windows.c\
	src/ircam.c\
	src/wve.c\
	src/w64.c\
	src/sndfile.c\
	src/G72x/g723_16.c\
	src/G72x/g723_24.c\
	src/G72x/g723_40.c\
	src/G72x/g721.c\
	src/G72x/g72x.c\
	src/G72x/g72x_test.c\
	src/sds.c\
	src/wav.c\
	src/float32.c\
	src/GSM610/add.c\
	src/GSM610/gsm_create.c\
	src/GSM610/decode.c\
	src/GSM610/preprocess.c\
	src/GSM610/gsm_option.c\
	src/GSM610/code.c\
	src/GSM610/gsm_encode.c\
	src/GSM610/gsm_decode.c\
	src/GSM610/gsm_destroy.c\
	src/GSM610/table.c\
	src/GSM610/rpe.c\
	src/GSM610/short_term.c\
	src/GSM610/long_term.c\
	src/GSM610/lpc.c\
	src/au.c\
	src/test_ima_oki_adpcm.c\
	src/ulaw.c\
	src/id3.c\
	src/rx2.c\
	src/chunk.c\
	src/txw.c\
	src/ima_adpcm.c\
	src/ima_oki_adpcm.c\
	src/file_io.c\
	src/caf.c\
	src/raw.c\
	src/ogg.c\
	src/strings.c\
	src/xi.c\
	src/interleave.c\
	src/test_endswap.c\
	src/ms_adpcm.c\
	src/macos.c\
	src/flac.c\
	src/vox_adpcm.c\
	src/sd2.c\
	src/paf.c\
	src/voc.c\
	src/double64.c\
	src/g72x.c\
	src/rf64.c\
	src/common.c\
	src/chanmap.c\
	src/pvf.c\
	src/macbinary3.c\
	src/alaw.c\
	src/dwd.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/src -std=c99 -O0 -g -DHAVE_CONFIG_H

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)




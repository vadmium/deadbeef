LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := sid

LOCAL_SRC_FILES +=\
	plugin.c csid.cpp csid.h\
	sidplay-libs/libsidplay/src/mixer.cpp\
	sidplay-libs/libsidplay/src/player.cpp\
	sidplay-libs/libsidplay/src/mos656x/mos656x.cpp\
	sidplay-libs/libsidplay/src/mos6510/mos6510.cpp\
	sidplay-libs/libsidplay/src/sid6526/sid6526.cpp\
	sidplay-libs/libsidplay/src/sidtune/MUS.cpp\
	sidplay-libs/libsidplay/src/sidtune/SidTuneTools.cpp\
	sidplay-libs/libsidplay/src/sidtune/InfoFile.cpp\
	sidplay-libs/libsidplay/src/sidtune/SidTune.cpp\
	sidplay-libs/libsidplay/src/sidtune/PSID.cpp\
	sidplay-libs/libsidplay/src/sidtune/PP20.cpp\
	sidplay-libs/libsidplay/src/sidtune/IconInfo.cpp\
	sidplay-libs/libsidplay/src/event.cpp\
	sidplay-libs/libsidplay/src/psiddrv.cpp\
	sidplay-libs/libsidplay/src/config.cpp\
	sidplay-libs/libsidplay/src/sidplay2.cpp\
	sidplay-libs/libsidplay/src/xsid/xsid.cpp\
	sidplay-libs/libsidplay/src/mos6526/mos6526.cpp\
	sidplay-libs/builders/resid-builder/src/resid.cpp\
	sidplay-libs/builders/resid-builder/src/resid-builder.cpp\
	sidplay-libs/resid/wave8580_PS_.cpp\
	sidplay-libs/resid/filter.cpp\
	sidplay-libs/resid/pot.cpp\
	sidplay-libs/resid/wave.cpp\
	sidplay-libs/resid/version.cpp\
	sidplay-libs/resid/wave6581__ST.cpp\
	sidplay-libs/resid/extfilt.cpp\
	sidplay-libs/resid/wave8580_PST.cpp\
	sidplay-libs/resid/wave6581_PST.cpp\
	sidplay-libs/resid/wave6581_P_T.cpp\
	sidplay-libs/resid/wave6581_PS_.cpp\
	sidplay-libs/resid/envelope.cpp\
	sidplay-libs/resid/voice.cpp\
	sidplay-libs/resid/sid.cpp\
	sidplay-libs/resid/wave8580__ST.cpp\
	sidplay-libs/resid/wave8580_P_T.cpp\
	sidplay-libs/libsidplay/src/reloc65.c

SID_PATH=$(LOCAL_PATH)/sidplay-libs

LOCAL_CFLAGS += -DHAVE_CONFIG_H -I$(LOCAL_PATH)/../.. -DHAVE_UNIX -I$(SID_PATH) -I$(SID_PATH)/unix -I$(SID_PATH)/libsidplay -I$(SID_PATH)/libsidplay/include -I$(SID_PATH)/libsidplay/include/sidplay -I$(SID_PATH)/libsidutils/include/sidplay/utils -I$(SID_PATH)/builders/resid-builder/include/sidplay/builders -I$(SID_PATH)/builders/resid-builder/include -DHAVE_STRCASECMP -DHAVE_STRNCASECMP -DVERSION=\"deadbeef\" -DPACKAGE=\"libsidplay2\"

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)



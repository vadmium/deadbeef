LOCAL_PATH      := $(call my-dir)

include $(CLEAR_VARS)

CORE_PATH=../..

#include $(LOCAL_PATH)/../../Android.mk
#include $(LOCAL_PATH)/loaders/Makefile
#include $(LOCAL_PATH)/loaders/prowizard/Makefile
#include $(LOCAL_PATH)/player/Makefile

#MISC_SOURCES    := $(addprefix misc/,$(MISC_OBJS))
#LOADERS_SOURCES := $(addprefix loaders/,$(LOADERS_OBJS))
#PROWIZ_SOURCES  := $(addprefix loaders/prowizard/,$(PROWIZ_OBJS))
#PLAYER_SOURCES  := $(addprefix player/,$(PLAYER_OBJS))

LOCAL_MODULE    := deadbeef
LOCAL_CFLAGS    := -O2 -DHAVE_CONFIG_H -I$(LOCAL_PATH) -I$(LOCAL_PATH)/libsamplerate/src -I$(LOCAL_PATH)/../../plugins/gme/Game_Music_Emu-0.5.2 -I$(LOCAL_PATH)/../.. -std=c99
LOCAL_LDLIBS    := -Lbuild/platforms/android-3/arch-arm/usr/lib -llog
LOCAL_SRC_FILES := deadbeef-jni.c\
	$(CORE_PATH)/conf.c\
	$(CORE_PATH)/junklib.c\
	$(CORE_PATH)/messagepump.c\
	$(CORE_PATH)/metacache.c\
	$(CORE_PATH)/playlist.c\
	$(CORE_PATH)/plugins.c\
	$(CORE_PATH)/streamer.c\
	$(CORE_PATH)/threading_pthread.c\
	$(CORE_PATH)/utf8.c\
	$(CORE_PATH)/vfs.c\
	$(CORE_PATH)/vfs_stdio.c\
	$(CORE_PATH)/volume.c\
	$(CORE_PATH)/md5/md5.c\
	libsamplerate/src/samplerate.c\
	libsamplerate/src/src_linear.c\
	libsamplerate/src/src_sinc.c\
	libsamplerate/src/src_zoh.c\
	$(CORE_PATH)/plugins/gme/cgme.c\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ay_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gb_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Hes_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Fme7_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sms_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ay_Cpu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gb_Cpu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Kss_Cpu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Namco_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Snes_Spc.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ay_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gb_Oscs.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Kss_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Oscs.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Spc_Cpu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Blip_Buffer.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gbs_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Kss_Scc_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Vrc6_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Spc_Dsp.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Classic_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/gme.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/M3u_Playlist.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nsfe_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Spc_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Data_Reader.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gme_File.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Multi_Buffer.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nsf_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Vgm_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Dual_Resampler.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gym_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Music_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sap_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Vgm_Emu_Impl.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Effects_Buffer.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Hes_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Apu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sap_Cpu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ym2413_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Fir_Resampler.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Hes_Cpu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Cpu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sap_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ym2612_Emu.cpp\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ay_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ay_Cpu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ay_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/blargg_common.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/blargg_config.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/blargg_endian.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/blargg_source.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Blip_Buffer.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Classic_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Data_Reader.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Dual_Resampler.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Effects_Buffer.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Fir_Resampler.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gb_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gb_Cpu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/gb_cpu_io.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gb_Oscs.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gbs_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gme_File.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/gme.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Gym_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Hes_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Hes_Cpu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/hes_cpu_io.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Hes_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Kss_Cpu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Kss_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Kss_Scc_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/M3u_Playlist.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Multi_Buffer.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Music_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Cpu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/nes_cpu_io.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Fme7_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Namco_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Oscs.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nes_Vrc6_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nsfe_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Nsf_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sap_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sap_Cpu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/sap_cpu_io.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sap_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sms_Apu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Sms_Oscs.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Snes_Spc.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Spc_Cpu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Spc_Dsp.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Spc_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Vgm_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Vgm_Emu_Impl.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ym2413_Emu.h\
	$(CORE_PATH)/plugins/gme/Game_Music_Emu-0.5.2/gme/Ym2612_Emu.h

include $(BUILD_SHARED_LIBRARY)

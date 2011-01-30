LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := gme

LOCAL_SRC_FILES +=\
	cgme.c\
	Game_Music_Emu-0.5.2/gme/Ay_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Gb_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Hes_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Nes_Fme7_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Sms_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Ay_Cpu.cpp\
	Game_Music_Emu-0.5.2/gme/Gb_Cpu.cpp\
	Game_Music_Emu-0.5.2/gme/Kss_Cpu.cpp\
	Game_Music_Emu-0.5.2/gme/Nes_Namco_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Snes_Spc.cpp\
	Game_Music_Emu-0.5.2/gme/Ay_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Gb_Oscs.cpp\
	Game_Music_Emu-0.5.2/gme/Kss_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Nes_Oscs.cpp\
	Game_Music_Emu-0.5.2/gme/Spc_Cpu.cpp\
	Game_Music_Emu-0.5.2/gme/Blip_Buffer.cpp\
	Game_Music_Emu-0.5.2/gme/Gbs_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Kss_Scc_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Nes_Vrc6_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Spc_Dsp.cpp\
	Game_Music_Emu-0.5.2/gme/Classic_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/gme.cpp\
	Game_Music_Emu-0.5.2/gme/M3u_Playlist.cpp\
	Game_Music_Emu-0.5.2/gme/Nsfe_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Spc_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Data_Reader.cpp\
	Game_Music_Emu-0.5.2/gme/Gme_File.cpp\
	Game_Music_Emu-0.5.2/gme/Multi_Buffer.cpp\
	Game_Music_Emu-0.5.2/gme/Nsf_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Vgm_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Dual_Resampler.cpp\
	Game_Music_Emu-0.5.2/gme/Gym_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Music_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Sap_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Vgm_Emu_Impl.cpp\
	Game_Music_Emu-0.5.2/gme/Effects_Buffer.cpp\
	Game_Music_Emu-0.5.2/gme/Hes_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Nes_Apu.cpp\
	Game_Music_Emu-0.5.2/gme/Sap_Cpu.cpp\
	Game_Music_Emu-0.5.2/gme/Ym2413_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Fir_Resampler.cpp\
	Game_Music_Emu-0.5.2/gme/Hes_Cpu.cpp\
	Game_Music_Emu-0.5.2/gme/Nes_Cpu.cpp\
	Game_Music_Emu-0.5.2/gme/Sap_Emu.cpp\
	Game_Music_Emu-0.5.2/gme/Ym2612_Emu.cpp

LOCAL_CFLAGS += -I$(LOCAL_PATH)/Game_Music_Emu-0.5.2 -std=c99 -O2

LOCAL_ARM_MODE := arm
LOCAL_LDLIBS    := -lz

include $(BUILD_SHARED_LIBRARY)

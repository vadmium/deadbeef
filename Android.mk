LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := core

LOCAL_SRC_FILES +=\
	conf.c\
	junklib.c\
	messagepump.c\
	metacache.c\
	playlist.c\
	plugins.c\
	streamer.c\
	threading_pthread.c\
	utf8.c\
	vfs.c\
	vfs_stdio.c\
	volume.c\
	md5/md5.c\
	ringbuf.c\
	dsppreset.c\
	premix.c\
	ConvertUTF/ConvertUTF.c


LOCAL_CFLAGS += -O2 -I$(LOCAL_PATH) -std=c99 -DVERSION=\"0.4.4\"

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)


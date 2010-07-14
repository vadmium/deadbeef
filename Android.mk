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
	moduleconf-android.h

LOCAL_CFLAGS += -O2 -DHAVE_CONFIG_H -I$(LOCAL_PATH) -std=c99 -I$(LOCAL_PATH)/android/jni/libsamplerate/src

LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)


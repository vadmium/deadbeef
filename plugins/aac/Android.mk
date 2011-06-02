LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := aac

LOCAL_SRC_FILES += aac.c aac_parser.c\
	mp4ff/mp4atom.c\
	mp4ff/mp4ff.c\
	mp4ff/mp4meta.c\
	mp4ff/mp4sample.c\
	mp4ff/mp4tagupdate.c\
	mp4ff/mp4util.c

LOCAL_STATIC_LIBRARIES := faad2

LOCAL_CFLAGS += -I$(LOCAL_PATH)/mp4ff -I$(LOCAL_PATH)/../../android.freeplugins/jni/faad2/include -O2 -DUSE_MP4FF -std=c99 -I$(LOCAL_PATH)/../.. -DHAVE_CONFIG_H -DUSE_TAGGING

LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)


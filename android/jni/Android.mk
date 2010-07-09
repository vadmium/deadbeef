LOCAL_PATH      := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../plugins/Android.mk
#include $(LOCAL_PATH)/loaders/Makefile
#include $(LOCAL_PATH)/loaders/prowizard/Makefile
#include $(LOCAL_PATH)/player/Makefile

#MISC_SOURCES    := $(addprefix misc/,$(MISC_OBJS))
#LOADERS_SOURCES := $(addprefix loaders/,$(LOADERS_OBJS))
#PROWIZ_SOURCES  := $(addprefix loaders/prowizard/,$(PROWIZ_OBJS))
#PLAYER_SOURCES  := $(addprefix player/,$(PLAYER_OBJS))

#DEADBEEF_VERSION     := `grep ^\#define VERSION $(LOCAL_PATH)/../../config.h|sed 's/#define VERSION //'`
DEADBEEF_VERSION := "0.4.2"
LOCAL_MODULE    := deadbeef
LOCAL_CFLAGS    := -DVERSION=\"$(DEADBEEF_VERSION)\" -O2 -DHAVE_CONFIG_H -I$(LOCAL_PATH)
LOCAL_LDLIBS    := -Lbuild/platforms/android-3/arch-arm/usr/lib -llog
LOCAL_SRC_FILES := deadbeef-jni.c
#        $(MISC_SOURCES:.o=.c) \
#        $(LOADERS_SOURCES:.o=.c) \
#        $(PROWIZ_SOURCES:.o=.c) \
#        $(PLAYER_SOURCES:.o=.c.arm)

include $(BUILD_SHARED_LIBRARY)

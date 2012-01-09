OUTPUT_APK_FILE=../bin/deadbeeffreeplugins.apk
OUTPUT_APK_FILE_TEMP=../bin/deadbeeffreeplugins.temp.apk
DEX_FILE=../bin/classes.dex
RES_FILE=../bin/resources.ap_

all:
	rm ../../config.h
	ln -s android/config-android.h ../../config.h
	$(NDK)/ndk-build

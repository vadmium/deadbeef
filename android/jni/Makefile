OUTPUT_APK_FILE=../bin/deadbeef.apk
OUTPUT_APK_FILE_TEMP=../bin/deadbeef.temp.apk
DEX_FILE=../bin/classes.dex
RES_FILE=../bin/resources.ap_

all:
	rm ../../config.h
	ln -s android/config-android.h ../../config.h
	javah -classpath ../../android/bin/classes -o deadbeef-jni.h org.deadbeef.android.DeadbeefAPI
	$(NDK)/ndk-build


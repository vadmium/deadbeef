OUTPUT_APK_FILE=../bin/deadbeeffreeplugins.apk
OUTPUT_APK_FILE_TEMP=../bin/deadbeeffreeplugins.temp.apk
DEX_FILE=../bin/classes.dex
RES_FILE=../bin/resources.ap_

all:
	$(NDK)/ndk-build

install:
	cd .. && touch src/org/deadbeef/android.freeplugins/AndroidManifest.xml && ant debug
	#apkbuilder  ${OUTPUT_APK_FILE_TEMP} -u -z  ${RES_FILE} -f  ${DEX_FILE}
	#jarsigner -keystore $(HOME)/.android/debug.keystore -storepass android -keypass android -signedjar $(OUTPUT_APK_FILE) $(OUTPUT_APK_FILE_TEMP) androiddebugkey
	adb install -r $(OUTPUT_APK_FILE)
	

	

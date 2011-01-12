OUTPUT_APK_FILE=../bin/deadbeef.apk
OUTPUT_APK_FILE_TEMP=../bin/deadbeef.temp.apk
DEX_FILE=../bin/classes.dex
RES_FILE=../bin/resources.ap_

all:
	~/android/android-ndk-r5/ndk-build
	#cd .. && touch src/org/deadbeef/android/Deadbeef.java && /usr/share/java/apache-ant/bin/ant -v
	#apkbuilder  ${OUTPUT_APK_FILE_TEMP} -u -z  ${RES_FILE} -f  ${DEX_FILE}
	#jarsigner -keystore $(HOME)/.android/debug.keystore -storepass android -keypass android -signedjar $(OUTPUT_APK_FILE) $(OUTPUT_APK_FILE_TEMP) androiddebugkey
	#adb install -r $(OUTPUT_APK_FILE)

	

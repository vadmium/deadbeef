/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_deadbeef_android_DeadbeefAPI */

#ifndef _Included_org_deadbeef_android_DeadbeefAPI
#define _Included_org_deadbeef_android_DeadbeefAPI
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    start
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_start
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    stop
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_stop
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    getBuffer
 * Signature: (I[S)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_getBuffer
  (JNIEnv *, jclass, jint, jshortArray);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    getSamplerate
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_getSamplerate
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_count
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1count
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_item_text
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1item_1text
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_add_folder
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1add_1folder
  (JNIEnv *, jclass, jstring);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_clear
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1clear
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_current_idx
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1current_1idx
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_metadata
 * Signature: (ILjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1metadata
  (JNIEnv *, jclass, jint, jstring);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_duration_formatted
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1duration_1formatted
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_prev
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1prev
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_next
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1next
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_idx
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1idx
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_get_pos_normalized
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1pos_1normalized
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_get_pos_seconds
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1pos_1seconds
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_get_pos_formatted
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1pos_1formatted
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_get_duration_seconds
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1duration_1seconds
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_get_duration_formatted
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1duration_1formatted
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_seek
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1seek
  (JNIEnv *, jclass, jfloat);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_toggle_pause
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1toggle_1pause
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_play
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1play
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_pause
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1pause
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1stop
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_is_paused
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1paused
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_is_playing
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1playing
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif

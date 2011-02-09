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
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_start
  (JNIEnv *, jclass, jstring, jstring);

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
 * Method:    getChannels
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_getChannels
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_getcount
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1getcount
  (JNIEnv *, jclass, jint);

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
 * Method:    pl_add_file
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1add_1file
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
 * Method:    pl_get_for_idx
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1for_1idx
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_meta
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1meta
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    meta_get_key
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_meta_1get_1key
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    meta_get_value
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_meta_1get_1value
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    meta_get_next
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_meta_1get_1next
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_item_unref
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1item_1unref
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_totaltime
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1totaltime
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_item_duration
 * Signature: (I)F
 */
JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1item_1duration
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_track_filetype
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1track_1filetype
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_insert_dir
 * Signature: (IILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1insert_1dir
  (JNIEnv *, jclass, jint, jint, jstring);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    pl_get_track_path
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1track_1path
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_get_count
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1count
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_get_sel_count
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1sel_1count
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_add
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1add
  (JNIEnv *, jclass, jint, jstring);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_remove
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1remove
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_set_curr
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1set_1curr
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_get_curr
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1curr
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_get_title
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1title
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_set_title
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1set_1title
  (JNIEnv *, jclass, jint, jstring);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    plt_move
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_plt_1move
  (JNIEnv *, jclass, jint, jint);

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
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1paused
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_is_playing
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1playing
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    play_is_stopped
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1stopped
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    is_streamer_active
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_deadbeef_android_DeadbeefAPI_is_1streamer_1active
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    set_play_mode
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_set_1play_1mode
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    get_play_mode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_get_1play_1mode
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    set_play_order
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_set_1play_1order
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    get_play_order
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_get_1play_1order
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    streamer_get_playing_track
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1playing_1track
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    streamer_get_streaming_track
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1streaming_1track
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    streamer_get_current_fileinfo
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1current_1fileinfo
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    streamer_get_current_fileinfo_format
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1current_1fileinfo_1format
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    streamer_get_playpos
 * Signature: ()F
 */
JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1playpos
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    streamer_get_apx_bitrate
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1apx_1bitrate
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    fmt_get_channels
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_fmt_1get_1channels
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    fmt_get_bps
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_fmt_1get_1bps
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    fmt_get_samplerate
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_fmt_1get_1samplerate
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    event_is_pending
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_deadbeef_android_DeadbeefAPI_event_1is_1pending
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    event_dispatch
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_event_1dispatch
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    event_get_type
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_event_1get_1type
  (JNIEnv *, jclass);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    event_get_string
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_event_1get_1string
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    event_get_int
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_event_1get_1int
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    dsp_find
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_dsp_1find
  (JNIEnv *, jclass, jstring);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    dsp_enable
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_dsp_1enable
  (JNIEnv *, jclass, jint, jboolean);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    dsp_is_enabled
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_deadbeef_android_DeadbeefAPI_dsp_1is_1enabled
  (JNIEnv *, jclass, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    dsp_get_param
 * Signature: (II)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_dsp_1get_1param
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    dsp_set_param
 * Signature: (IILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_dsp_1set_1param
  (JNIEnv *, jclass, jint, jint, jstring);

/*
 * Class:     org_deadbeef_android_DeadbeefAPI
 * Method:    dsp_save_config
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_dsp_1save_1config
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif

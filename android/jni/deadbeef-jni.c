/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdlib.h>
#include <jni.h>
#include "deadbeef-jni.h"
#include <android/log.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <locale.h>
#ifndef __linux__
#define _POSIX_C_SOURCE
#endif
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/errno.h>
#include <signal.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <math.h>
#include "playlist.h"
#include "unistd.h"
#include "threading.h"
#include "messagepump.h"
#include "streamer.h"
#include "conf.h"
#include "volume.h"
#include "plugins.h"

#define trace(...) { android_trace(__VA_ARGS__); }
//#define trace(fmt,...)

void
android_trace (const char *fmt, ...) {
    char p[300];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(p, sizeof(p), fmt, ap);
    va_end(ap);
    __android_log_write(ANDROID_LOG_INFO,"DDB",p);
}

// some common global variables
char sys_install_path[PATH_MAX]; // see deadbeef->get_prefix
char dbconfdir[PATH_MAX]; // $HOME/.config/deadbeef
char dbinstalldir[PATH_MAX]; // see deadbeef->get_prefix
char dbdocdir[PATH_MAX]; // see deadbeef->get_doc_dir
char dbplugindir[PATH_MAX]; // see deadbeef->get_plugin_dir
char dbpixmapdir[PATH_MAX]; // see deadbeef->get_pixmap_dir

// fake output plugin
static int jni_out_state = OUTPUT_STATE_STOPPED;

static int jni_out_init(void)
{
    jni_out_state = OUTPUT_STATE_STOPPED;
    return 0;
}

static int jni_out_free(void)
{
    jni_out_state = OUTPUT_STATE_STOPPED;

    return 0;
}

static int jni_out_play(void)
{
    jni_out_state = OUTPUT_STATE_PLAYING;
    return 0;
}

static int jni_out_stop(void)
{
    jni_out_state = OUTPUT_STATE_STOPPED;
    return 0;
}

static int jni_out_pause(void)
{
    jni_out_state = OUTPUT_STATE_PAUSED;
    return 0;
}

static int jni_out_unpause(void)
{
    jni_out_state = OUTPUT_STATE_PLAYING;

    return 0;
}

static int jni_out_get_state(void)
{
    return jni_out_state;
}

static int
jni_setformat(ddb_waveformat_t *fmt);

static DB_output_t jni_out = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.name = "Dummy output plugin",
    .plugin.descr = "Allows to run without real output plugin",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = jni_out_init,
    .free = jni_out_free,
    .setformat = jni_setformat,
    .play = jni_out_play,
    .stop = jni_out_stop,
    .pause = jni_out_pause,
    .unpause = jni_out_unpause,
    .state = jni_out_get_state,
};


static int
jni_setformat(ddb_waveformat_t *fmt)
{
    jni_out.fmt.samplerate = fmt->samplerate;
    if (jni_out.fmt.samplerate > 96000) {
        jni_out.fmt.samplerate = 96000;
    }
    else if (jni_out.fmt.samplerate < 8000) {
        jni_out.fmt.samplerate = 8000;
    }
    jni_out.fmt.channels = fmt->channels;
    jni_out.fmt.channelmask = fmt->channelmask;
    if (jni_out.fmt.channels > 2) {
        jni_out.fmt.channels = 2;
        jni_out.fmt.channelmask = 3;
    }
    jni_out.fmt.bps = 16;
    jni_out.fmt.is_float = 0;
    trace ("jni_setformat %d %d\n", jni_out.fmt.samplerate, jni_out.fmt.channels);
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_start (JNIEnv *env, jclass cls, jstring android_config_dir, jstring plugins_path) {
    trace("ddb_start");
      // initialize ddb
    setlocale (LC_ALL, "");
    setlocale (LC_NUMERIC, "C");
    srand (time (NULL));
     const jbyte *str;
     str = (*env)->GetStringUTFChars(env, android_config_dir, NULL);
     if (str == NULL) {
         return -1;
     }
    strcpy (dbconfdir, str);
    trace ("dbconfdir: %s\n", dbconfdir);
     (*env)->ReleaseStringUTFChars(env, android_config_dir, str);

    strcpy (dbinstalldir, "");
    strcpy (dbplugindir, "/data/data/org.deadbeef.android/lib");

     str = (*env)->GetStringUTFChars(env, plugins_path, NULL);
     conf_set_str ("android.plugin_path", str);

     (*env)->ReleaseStringUTFChars(env, android_config_dir, str);


    pl_init ();
    conf_load (); // required by some plugins at startup
    volume_set_db (conf_get_float ("playback.volume", 0)); // volume need to be initialized before plugins start
    messagepump_init ();
    plug_load_all ();
    pl_load_all ();
    plt_set_curr (conf_get_int ("playlist.current", 0));
    plug_trigger_event_playlistchanged ();

    // setup fake output plugin
    extern DB_output_t *output_plugin;
    output_plugin = &jni_out;

    streamer_init ();

    // start song #0 in playlist
    //streamer_set_nextsong (0, 1);

    trace ("ddb_start done");
    return 0;
}

JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_stop
  (JNIEnv *env, jclass cls)
{
    pl_save_all ();
    conf_save ();
    streamer_free ();
    plug_unload_all ();
    plt_free (); // plt_free may access conf_*
    pl_free ();
    conf_free ();
    messagepump_free ();
    plug_cleanup ();
    return 0;
}

JNIEXPORT int JNICALL
Java_org_deadbeef_android_DeadbeefAPI_getBuffer (JNIEnv *env, jclass cls, jint size, jshortArray buffer) {
    int bytesread = 0;
    char b[size*2];
    if (jni_out_state == OUTPUT_STATE_PLAYING && streamer_ok_to_read (-1)) {
        bytesread = streamer_read (b, size*2);
        if (bytesread != size*2) {
            memset (b+bytesread, 0, size*2-bytesread);
        }
    }
    else {
        memset (b, 0, sizeof (b));
    }
    (*env)->SetShortArrayRegion(env, buffer, 0, size, (short *)b);
    return bytesread;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_getSamplerate (JNIEnv *env, jclass cls) {
    return jni_out.fmt.samplerate;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_getChannels (JNIEnv *env, jclass cls) {
    return jni_out.fmt.channels;
}

JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1count
  (JNIEnv *env, jclass cls)
{
    int cnt = pl_getcount (PL_MAIN);
//    trace("pl_getcount = %d\n", cnt);
    return cnt;
}

JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1item_1text
  (JNIEnv *env, jclass cls, jint idx)
{
    char s[200] = "n/a";
    playItem_t *it = pl_get_for_idx_and_iter (idx, PL_MAIN);
    if (it) {
        pl_format_title (it, idx, s, sizeof (s), -1, "%n. %f");
        pl_item_unref (it);
    }
    return (*env)->NewStringUTF(env, s);
}

JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1add_1folder
  (JNIEnv *env, jclass cls, jstring path)
{
     const jbyte *str;
     str = (*env)->GetStringUTFChars(env, path, NULL);
     if (str == NULL) {
         return -1;
     }
     int res = pl_add_dir (str, NULL, NULL);
     pl_save_current ();

     trace ("added %s; new pl_count: %d\n", str, pl_getcount (PL_MAIN));

     (*env)->ReleaseStringUTFChars(env, path, str);
     return res;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1add_1file (JNIEnv *env, jclass cls, jstring path) {
     const jbyte *str;
     str = (*env)->GetStringUTFChars(env, path, NULL);
     if (str == NULL) {
         return -1;
     }
     trace ("adding %s...\n", str);
     // replace %20 with spaces
     char outname[PATH_MAX];

     const char *p = str;
     if (!strncmp (str, "file://", 7)) {
         p += 7;
     }
     char *out = outname;
     while (*p) {
         if (!strncmp (p, "%20", 3)) {
             *out++ = 0x20;
             p += 3;
         }
         else {
             *out++ = *p++;
         }
     }
     *out = 0;
     (*env)->ReleaseStringUTFChars(env, path, str);

     int idx = pl_getcount (PL_MAIN);
     trace ("converted to %s...\n", outname);
     int res = pl_add_file (outname, NULL, NULL);
     pl_save_current ();

     trace ("added %s; new pl_count: %d\n", outname, pl_getcount (PL_MAIN));

     if (res != 0) {
         idx = -1;
     }
     return idx;
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1clear
  (JNIEnv *env, jclass cls)
{
    pl_clear ();
    pl_save_current ();
}


JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1current_1idx
  (JNIEnv *env, jclass cls)
{
    playItem_t *it = streamer_get_playing_track ();
    if (it) {
        int idx = pl_get_idx_of (it);
        pl_item_unref (it);
        return idx;
    }
    return -1;
}

JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1metadata
  (JNIEnv *env, jclass cls, jint idx, jstring key)
{
     const jbyte *str;
     str = (*env)->GetStringUTFChars(env, key, NULL);
     if (str == NULL) {
         return NULL;
     }

     playItem_t *it = pl_get_for_idx_and_iter (idx, PL_MAIN);
     jstring res = NULL;

     if (it) {
         const char *val = pl_find_meta (it, str);
         if (val) {
             res = (*env)->NewStringUTF(env, val);
         }
         pl_item_unref (it);
         (*env)->ReleaseStringUTFChars(env, key, str);
         return res;
     }
     (*env)->ReleaseStringUTFChars(env, key, str);
     return NULL;
}

JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1duration_1formatted
  (JNIEnv *env, jclass cls, jint idx)
{
    playItem_t *it = pl_get_for_idx_and_iter (idx, PL_MAIN);
    if (it) {
        float dur = pl_get_item_duration (it);
        pl_item_unref (it);
        char s[50];
        pl_format_time (dur, s, sizeof (s));
        return (*env)->NewStringUTF(env, s);
    }
    return (*env)->NewStringUTF(env, "-:--");
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1prev
  (JNIEnv *env, jclass cls)
{
    jni_out_stop ();
    streamer_move_to_prevsong ();
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1next
  (JNIEnv *env, jclass cls)
{
    jni_out_stop ();
    streamer_move_to_nextsong (1);
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1idx
  (JNIEnv *env, jclass cls, jint idx)
{
    trace ("play_idx: %d\n", idx);
    jni_out_stop ();
    streamer_set_nextsong (idx, 1);
}


JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1pos_1normalized
  (JNIEnv *env, jclass cls)
{
    playItem_t *it = streamer_get_playing_track ();
    if (it) {
        float pos = streamer_get_playpos () / pl_get_item_duration (it);
        pl_item_unref (it);
        return pos;
    }
    return 0;
}

JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1pos_1seconds
  (JNIEnv *env, jclass cls)
{
    playItem_t *it = streamer_get_playing_track ();
    if (it) {
        float pos = streamer_get_playpos ();
        pl_item_unref (it);
        return pos;
    }
    return -1;
}

JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1pos_1formatted
  (JNIEnv *env, jclass cls)
{
    playItem_t *it = streamer_get_playing_track ();
    if (it) {
        float pos = streamer_get_playpos ();
        pl_item_unref (it);
        char s[50];
        pl_format_time (pos, s, sizeof (s));
        return (*env)->NewStringUTF(env, s);
    }
    return (*env)->NewStringUTF(env, "-:--");
}

JNIEXPORT jfloat JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1duration_1seconds
  (JNIEnv *env, jclass cls)
{
    playItem_t *it = streamer_get_playing_track ();
    if (it) {
        float dur= pl_get_item_duration (it);
        pl_item_unref (it);
        return dur;
    }
    return -1;
}

JNIEXPORT jstring JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1get_1duration_1formatted
  (JNIEnv *env, jclass cls)
{
    playItem_t *it = streamer_get_playing_track ();
    if (it) {
        float dur = pl_get_item_duration (it);
        pl_item_unref (it);
        char s[50];
        pl_format_time (dur, s, sizeof (s));
        return (*env)->NewStringUTF(env, s);
    }
    return (*env)->NewStringUTF(env, "-:--");
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1seek
  (JNIEnv *env, jclass cls, jfloat pos)
{
    playItem_t *trk = streamer_get_playing_track ();
    if (trk) {
        float time = pos * pl_get_item_duration (trk);
        if (time < 0) {
            time = 0;
        }
        pl_item_unref (trk);
        streamer_set_seek (time);
    }
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1toggle_1pause
  (JNIEnv *env, jclass cls)
{
    if (jni_out_get_state () == OUTPUT_STATE_PLAYING) {
        jni_out_pause ();
    }
    else {
        jni_out_play ();
    }
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1play
  (JNIEnv *env, jclass cls)
{
    trace ("out_play\n");
    jni_out_play ();
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1pause
  (JNIEnv *env, jclass cls)
{
    jni_out_pause ();
}

JNIEXPORT void JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1stop
  (JNIEnv *env, jclass cls)
{
    jni_out_stop ();
}

JNIEXPORT int JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1paused
  (JNIEnv *env, jclass cls)
{
    return jni_out_get_state () == OUTPUT_STATE_PAUSED;
}

JNIEXPORT int JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1playing
  (JNIEnv *env, jclass cls)
{
    return (jni_out_get_state () == OUTPUT_STATE_PLAYING);
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_is_1streamer_1active (JNIEnv *env, jclass cls) {
    playItem_t *it = streamer_get_playing_track ();
    if (it) {
        pl_item_unref (it);
    }
    int res = (it ? 1 : 0) || !streamer_ok_to_read (-1);
    trace ("streamer_active: %d\n", res);
    return res;
}

JNIEXPORT void
JNICALL Java_org_deadbeef_android_DeadbeefAPI_set_1play_1mode (JNIEnv *env, jclass cls, jint mode) {
    conf_set_int ("playback.loop", mode);
    conf_save ();
}

JNIEXPORT jint
JNICALL Java_org_deadbeef_android_DeadbeefAPI_get_1play_1mode (JNIEnv *env, jclass cls) {
    return conf_get_int ("playback.loop", 0);
}

JNIEXPORT void
JNICALL Java_org_deadbeef_android_DeadbeefAPI_set_1play_1order (JNIEnv *env, jclass cls, jint order) {
    conf_set_int ("playback.order", order);
    streamer_configchanged ();
    conf_save ();
}

JNIEXPORT jint
JNICALL Java_org_deadbeef_android_DeadbeefAPI_get_1play_1order (JNIEnv *env, jclass cls) {
    return pl_get_order ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1for_1idx (JNIEnv *env, jclass cls, jint idx) {
    return (jint)pl_get_for_idx_and_iter (idx, PL_MAIN);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1meta (JNIEnv *env, jclass cls, jint trk) {
    return (jint)pl_get_metadata ((playItem_t *)trk);
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_meta_1get_1key (JNIEnv *env, jclass cls, jint meta) {
    return (*env)->NewStringUTF(env, ((DB_metaInfo_t*)meta)->key);

}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_meta_1get_1value (JNIEnv *env, jclass cls, jint meta) {
    return (*env)->NewStringUTF(env, ((DB_metaInfo_t*)meta)->value);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_meta_1get_1next (JNIEnv *env, jclass cls, jint meta) {
    return (jint)(((DB_metaInfo_t *)meta)->next);
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1item_1unref (JNIEnv *env, jclass cls, jint trk) {
    pl_item_unref ((playItem_t *)trk);
}

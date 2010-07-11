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
#include "org_deadbeef_android_DeadbeefAPI.h"
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
#include "playback.h"
#include "unistd.h"
#include "threading.h"
#include "messagepump.h"
#include "streamer.h"
#include "conf.h"
#include "volume.h"
#include "plugins.h"

// some common global variables
char confdir[1024]; // $HOME/.config
char dbconfdir[1024]; // $HOME/.config/deadbeef

// fake output plugin
int jni_out_state = OUTPUT_STATE_STOPPED;

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
    if (jni_out_state == OUTPUT_STATE_PAUSED) {
        jni_out_state = OUTPUT_STATE_PLAYING;
    }

    return 0;
}

static int jni_out_change_rate(int rate)
{
    return 44100;
}

static int jni_out_get_rate(void)
{
    return 44100;
}

static int jni_out_get_bps(void)
{
    return 16;
}

static int jni_out_get_channels(void)
{
    return 2;
}

static int jni_out_get_endianness(void)
{
#if WORDS_BIGENDIAN
    return 1;
#else
    return 0;
#endif
}

static int jni_out_get_state(void)
{
    return jni_out_state;
}

static DB_output_t jni_out = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.nostop = 1,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.name = "Dummy output plugin",
    .plugin.descr = "Allows to run without real output plugin",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = jni_out_init,
    .free = jni_out_free,
    .change_rate = jni_out_change_rate,
    .play = jni_out_play,
    .stop = jni_out_stop,
    .pause = jni_out_pause,
    .unpause = jni_out_unpause,
    .state = jni_out_get_state,
    .samplerate = jni_out_get_rate,
    .bitspersample = jni_out_get_bps,
    .channels = jni_out_get_channels,
    .endianness = jni_out_get_endianness,
};

JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_start
  (JNIEnv *env, jobject obj) {
    __android_log_write(ANDROID_LOG_INFO,"DDB","ddb_start");
      // initialize ddb
    setlocale (LC_ALL, "");
    setlocale (LC_NUMERIC, "C");
    srand (time (NULL));
    strcpy (confdir, "/sdcard/deadbeef");
    mkdir (confdir, 0755);
    strcpy (dbconfdir, "/sdcard/deadbeef");
    mkdir (dbconfdir, 0755);

    pl_init ();
    conf_load (); // required by some plugins at startup
    volume_set_db (conf_get_float ("playback.volume", 0)); // volume need to be initialized before plugins start
    messagepump_init ();
    plug_load_all ();
    pl_load_all ();
    plt_set_curr (conf_get_int ("playlist.current", 0));
    plug_trigger_event_playlistchanged ();
    streamer_init ();

    // setup fake output plugin
    extern DB_output_t *output_plugin;
    output_plugin = &jni_out;

    // add test file to playlist
    pl_add_file ("/sdcard/deadbeef/test.nsf", NULL, NULL);
    // start song #0 in playlist
    streamer_set_nextsong (0, 1);

    __android_log_write(ANDROID_LOG_INFO,"DDB","ddb_start done");
    return 0;
}

JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_stop
  (JNIEnv *env, jobject obj) {
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

JNIEXPORT jshortArray JNICALL Java_org_deadbeef_android_DeadbeefAPI_getBuffer
  (JNIEnv *env, jobject obj, jint size, jshortArray buffer) {
    short b[size];
    memset (b, 0, sizeof (b));
    if (streamer_ok_to_read (size*2)) {
        int bytesread = streamer_read ((char *)b, size*2);
    }
    else
    {
        char out[100];
        snprintf (out, sizeof (out), "stream failed, buffer fill: %d bytes, requested: %d bytes\n", streamer_get_fill (), size*2);
        __android_log_write(ANDROID_LOG_INFO,"DDB",out);
    }
    (*env)->SetShortArrayRegion(env, buffer, 0, size, b);

    return buffer;
}

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
#include <cpu-features.h>

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

// cpu features
int neon_supported = 0;
int armv7a_supported = 0;
int vfp_supported = 0;

#ifdef HAVE_EQ
// dsp params
extern float eq_bands[11]; // nr.0 is preamp
extern int eq_on;
extern float eq_changed;
#endif

static int
jni_setformat(ddb_waveformat_t *fmt);

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

static DB_output_t jni_out = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.name = "Dummy output plugin",
    .plugin.descr = "Allows to run without real output plugin",
    .plugin.copyright = "(C) 2011 Alexey Yakovenko, All Rights Reserved",
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


#define EV_MAX_ARGS 5
intptr_t jnievent_mutex;

typedef struct jni_event_s {
    const char *id;
    char *str_args[EV_MAX_ARGS];
    int int_args[EV_MAX_ARGS];
    struct jni_event_s *next;
} jni_event_t;

static jni_event_t *events;
static jni_event_t *events_tail;

void
jnievent_dispatch (void) {
    if (events) {
        jni_event_t *next = events->next;
        for (int i = 0; i < EV_MAX_ARGS; i++) {
            if (events->str_args[i]) {
                free (events->str_args[i]);
            }
        }
        free (events);
        if (events_tail == events) {
            events_tail = NULL;
        }
        events = next;
    }
}
void
jnievent_init (void) {
    jnievent_mutex = mutex_create ();
}

void
jnievent_free (void) {
    if (jnievent_mutex) {
        mutex_lock (jnievent_mutex);
        while (events) {
            jnievent_dispatch ();
        }
        mutex_free (jnievent_mutex);
        jnievent_mutex = 0;
    }
    events = NULL;
    events_tail = NULL;
}

JavaVM android_jvm;

#define ATTACH_JVM(jni_env)  \
        JNIEnv g_env;\
        int env_status = android_jvm->GetEnv(&android_jvm, (void **)&g_env, JNI_VERSION_1_6); \
        jint attachResult = android_jvm->AttachCurrentThread(&android_jvm, &jni_env,NULL);

#define DETACH_JVM(jni_env)   if( env_status == JNI_EDETACHED ){ android_jvm->DetachCurrentThread(&android_jvm); }

#define THREAD_PRIORITY_AUDIO -16
#define THREAD_PRIORITY_URGENT_AUDIO -19

static int
set_android_thread_priority(int priority){
    jclass process_class;
    jmethodID set_prio_method;
    JNIEnv *jni_env = 0;
    ATTACH_JVM(jni_env);
    int result = 0;

    //Get pointer to the java class
    process_class = (jclass)(*jni_env)->NewGlobalRef(jni_env, (*jni_env)->FindClass(jni_env, "android/os/Process"));
    if (process_class == 0) {
        result = -1;
        goto on_finish;
    }

    //Get the set priority function
    set_prio_method = (*jni_env)->GetStaticMethodID(jni_env, process_class, "setThreadPriority", "(I)V");
    if (set_prio_method == 0) {
        result = -1;
        goto on_finish;
    }

    //Call it
    (*jni_env)->CallStaticIntMethod(jni_env, process_class, set_prio_method, priority);
    // TODO : catch exceptions

on_finish:
    DETACH_JVM(jni_env);
    return result;
}

static int
lastfm_songstarted (ddb_event_track_t *e) {
    jni_event_t *ev = malloc (sizeof (jni_event_t));
    memset (ev, 0, sizeof (jni_event_t));
    ev->id = "songstarted";
    const char *artist = pl_find_meta ((playItem_t *)e->track, "artist");
    const char *album = pl_find_meta ((playItem_t *)e->track, "album");
    const char *title = pl_find_meta ((playItem_t *)e->track, "title");

    ev->str_args[0] = artist ? strdup (artist) : NULL;
    ev->str_args[1]= album ? strdup (album) : NULL;
    ev->str_args[2]= title ? strdup (title) : NULL;
    ev->int_args[0] = pl_get_item_duration ((playItem_t *)e->track) * 1000;
    mutex_lock (jnievent_mutex);
    if (events_tail) {
        events_tail->next = ev;
        events_tail = ev;
    }
    else {
        events = events_tail = ev;
    }
    mutex_unlock (jnievent_mutex);
    return 0;
}

static int
lastfm_songfinished (ddb_event_track_t *e) {
    jni_event_t *ev = malloc (sizeof (jni_event_t));
    memset (ev, 0, sizeof (jni_event_t));
    ev->id = "songfinished";
    mutex_lock (jnievent_mutex);
    if (events_tail) {
        events_tail->next = ev;
        events_tail = ev;
    }
    else {
        events = events_tail = ev;
    }
    mutex_unlock (jnievent_mutex);
    return 0;
}


static int
lastfm_paused (ddb_event_state_t *e) {
    jni_event_t *ev = NULL;

    if (e->state) { // paused
        ev = malloc (sizeof (jni_event_t));
        memset (ev, 0, sizeof (jni_event_t));
        ev->id = "paused";
    }
    else { // resumed
        ev->id = "resumed";
        playItem_t *trk = streamer_get_playing_track ();
        if (trk) {
            ev = malloc (sizeof (jni_event_t));
            memset (ev, 0, sizeof (jni_event_t));
            const char *artist = pl_find_meta (trk, "artist");
            const char *album = pl_find_meta (trk, "album");
            const char *title = pl_find_meta (trk, "title");
            ev->str_args[0] = artist ? strdup (artist) : NULL;
            ev->str_args[1]= album ? strdup (album) : NULL;
            ev->str_args[2]= title ? strdup (title) : NULL;
            ev->int_args[0] = pl_get_item_duration (trk) * 1000;
            ev->int_args[1] = streamer_get_playpos () * 1000;
            pl_item_unref (trk);
        }
    }

    if (ev) {
        mutex_lock (jnievent_mutex);
        if (events_tail) {
            events_tail->next = ev;
            events_tail = ev;
        }
        else {
            events = events_tail = ev;
        }
        mutex_unlock (jnievent_mutex);
    }
    return 0;
}

int
scan_dsp_presets (void);

intptr_t mainloop_tid;

static int
lastfm_songstarted (ddb_event_track_t *e);

static int
lastfm_songfinished (ddb_event_track_t *e);

static int
lastfm_paused (ddb_event_state_t *e);

void
mainloop_thread (void *ctx) {
    for (;;) {
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        int term = 0;
        while (messagepump_pop(&msg, &ctx, &p1, &p2) != -1) {
            // broadcast to all plugins
            DB_plugin_t **plugs = plug_get_list ();
            for (int n = 0; plugs[n]; n++) {
                if (plugs[n]->message) {
                    plugs[n]->message (msg, ctx, p1, p2);
                }
            }
            DB_output_t *output = plug_get_output ();
            switch (msg) {
            case DB_EV_SONGSTARTED:
                lastfm_songstarted ((ddb_event_track_t *)ctx);
                break;
            case DB_EV_SONGFINISHED:
                lastfm_songfinished ((ddb_event_track_t *)ctx);
                break;
            case DB_EV_PAUSED:
                lastfm_paused ((ddb_event_state_t *)ctx);
                break;
            case DB_EV_REINIT_SOUND:
                plug_reinit_sound ();
                streamer_reset (1);
                conf_save ();
                break;
            case DB_EV_TERMINATE:
                term = 1;
                break;
            case DB_EV_PLAY_CURRENT:
                if (p1) {
                    output->stop ();
                    pl_playqueue_clear ();
                    streamer_set_nextsong (0, 1);
                }
                else {
                    streamer_play_current_track ();
                }
                break;
            case DB_EV_PLAY_NUM:
                output->stop ();
                pl_playqueue_clear ();
                streamer_set_nextsong (p1, 1);
                if (pl_get_order () == PLAYBACK_ORDER_SHUFFLE_ALBUMS) {
                    int pl = streamer_get_current_playlist ();
                    playlist_t *plt = plt_get_for_idx (pl);
                    plt_init_shuffle_albums (plt, p1);
                    plt_unref (plt);
                }
                break;
            case DB_EV_STOP:
                streamer_set_nextsong (-2, 0);
                break;
            case DB_EV_NEXT:
                output->stop ();
                streamer_move_to_nextsong (1);
                break;
            case DB_EV_PREV:
                output->stop ();
                streamer_move_to_prevsong ();
                break;
            case DB_EV_PAUSE:
                if (output->state () != OUTPUT_STATE_PAUSED) {
                    output->pause ();
                    messagepump_push (DB_EV_PAUSED, 0, 1, 0);
                }
                break;
            case DB_EV_TOGGLE_PAUSE:
                if (output->state () == OUTPUT_STATE_PAUSED) {
                    output->unpause ();
                    messagepump_push (DB_EV_PAUSED, 0, 0, 0);
                }
                else {
                    output->pause ();
                    messagepump_push (DB_EV_PAUSED, 0, 1, 0);
                }
                break;
            case DB_EV_PLAY_RANDOM:
                output->stop ();
                streamer_move_to_randomsong ();
                break;
            case DB_EV_PLAYLIST_REFRESH:
                pl_save_current ();
                messagepump_push (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
                break;
            case DB_EV_CONFIGCHANGED:
                conf_save ();
                streamer_configchanged ();
                break;
            case DB_EV_SEEK:
                streamer_set_seek (p1 / 1000.f);
                break;
            }
            if (msg >= DB_EV_FIRST && ctx) {
                messagepump_event_free ((ddb_event_t *)ctx);
            }
        }
        if (term) {
            return;
        }
        messagepump_wait ();
        //usleep(50000);
        //plug_trigger_event (DB_EV_FRAMEUPDATE, 0);
    }
}

JNIEXPORT jint
JNI_OnLoad(JavaVM *vm, void *reserved) {
    android_jvm = *vm;
    return JNI_VERSION_1_4;
}

void
restore_resume_state (void) {
    DB_output_t *output = plug_get_output ();
    if (output && output->state () == OUTPUT_STATE_STOPPED) {
        int plt = conf_get_int ("resume.playlist", plt_get_curr_idx ());
        int track = conf_get_int ("resume.track", 0);
        float pos = conf_get_float ("resume.position", 0);
        int paused = conf_get_int ("resume.paused", 1);
        trace ("resume: track %d pos %f playlist %d\n", track, pos, plt);
        if (plt >= 0 && track >= 0 && pos >= 0) {
            streamer_lock (); // need to hold streamer thread to make the resume operation atomic
            streamer_set_current_playlist (plt);
            streamer_set_nextsong (track, paused ? 2 : 3);
            streamer_set_seek (pos);
            streamer_unlock ();
        }
    }
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_restore_1resume_1state (JNIEnv *env, jclass cls) {
    restore_resume_state ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_start (JNIEnv *env, jclass cls, jstring android_config_dir, jstring plugins_path) {
    trace("ddb_start");

    trace ("off_t %d\n", sizeof (off_t));
    trace ("loff_t %d\n", sizeof (loff_t));
    trace ("long long %d\n", sizeof (long long));
    trace ("off64_t %d\n", sizeof (off64_t));
    trace ("size_t %d\n", sizeof (size_t));
    trace ("ssize_t %d\n", sizeof (ssize_t));
    trace ("wchar_t %d\n", sizeof (wchar_t));

#ifndef USE_NEON
    trace ("neon disabled!\n");
#else
    trace ("neon enabled!\n");
#endif

    uint64_t features = android_getCpuFeatures ();
//    trace ("cpu features: 0x%llx\n", features);
    if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM) {
        if (features & ANDROID_CPU_ARM_FEATURE_NEON) {
            neon_supported = 1;
        }
        if (features & ANDROID_CPU_ARM_FEATURE_ARMv7) {
            armv7a_supported = 1;
        }
        if (features & ANDROID_CPU_ARM_FEATURE_VFPv3) {
            vfp_supported = 1;
        }
    }
    trace ("neon: %d\n", neon_supported);
    trace ("vfp: %d\n", vfp_supported);
    trace ("armv7a: %d\n", armv7a_supported);

    // initialize ddb
    trace ("setlocale\n");
    setlocale (LC_ALL, "");
    setlocale (LC_NUMERIC, "C");
    srand (time (NULL));
    trace ("set dbconfdir\n");
    const jbyte *str;
    str = (*env)->GetStringUTFChars(env, android_config_dir, NULL);
    if (str == NULL) {
        return -1;
    }
    strncpy (dbconfdir, str, sizeof (dbconfdir));
    dbconfdir[sizeof(dbconfdir)-1] = 0;
    int err = mkdir (dbconfdir, 0);
    trace ("dbconfdir: %s\n", dbconfdir);
    (*env)->ReleaseStringUTFChars(env, android_config_dir, str);

    strcpy (dbinstalldir, "");
    strcpy (dbplugindir, "/data/data/org.deadbeef.android/lib");

    trace ("pl_init\n");
    pl_init ();

    trace ("conf_load\n");
    conf_load (); // required by some plugins at startup

    str = (*env)->GetStringUTFChars(env, plugins_path, NULL);
    conf_set_str ("android.plugin_path", str);
    (*env)->ReleaseStringUTFChars(env, android_config_dir, str);

//    volume_set_db (conf_get_float ("playback.volume", 0)); // volume need to be initialized before plugins start

    trace ("messagepump_init\n");
    messagepump_init ();
    trace ("plug_load_all\n");
    plug_load_all ();

    trace ("pl_free\n");
    pl_free ();

    trace ("pl_load_all\n");
    pl_load_all ();
    plt_set_curr_idx (conf_get_int ("playlist.current", 0));

    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, 0, 0);

    // setup fake output plugin
    extern DB_output_t *output_plugin;
    output_plugin = &jni_out;

#ifdef HAVE_EQ
    // prepare dsp presets
    scan_dsp_presets ();

    // load eq settings
    eq_on = conf_get_int ("android.eq_enabled", 0);
    char f[PATH_MAX];
    snprintf (f, sizeof (f), "%s/eqconfig", dbconfdir);
    FILE *fp = fopen (f, "rt");
    if (fp) {
        int n = 0;
        float p;
        while (1 == fscanf (fp, "%f\n", &p) && n < 11) {
            eq_bands[n] = p;
            n++;
        }
        fclose (fp);
    }
    eq_changed = 1;
#endif

    trace ("streamer_init\n");
    streamer_init ();
    plug_connect_all ();

    restore_resume_state ();

    jnievent_init ();

    mainloop_tid = thread_start (mainloop_thread, NULL);
    trace ("ddb_start done");
    return 0;
}

JNIEXPORT jint JNICALL Java_org_deadbeef_android_DeadbeefAPI_stop
  (JNIEnv *env, jclass cls)
{
    trace ("DeadbeefAPI.stop called\n");
    if (!mainloop_tid) {
        return -1;
    }
    trace ("order mainloop to terminate\n");
    messagepump_push (DB_EV_TERMINATE, 0, 0, 0);
    trace ("wait for mainloop to finish\n");
    thread_join (mainloop_tid);
    trace ("mainloop finished\n");
    mainloop_tid = 0;
    trace ("jnievent_free\n");
    jnievent_free ();
    trace ("pl_save_all\n");
    pl_save_all ();
    trace ("conf_save\n");
    conf_save ();
    trace ("streamer_free\n");
    streamer_free ();
    trace ("plug_unload_all\n");
    plug_unload_all ();
    trace ("pl_free\n");
    pl_free ();
    trace ("conf_free\n");
    conf_free ();
    trace ("messagepump_free\n");
    messagepump_free ();
    trace ("plug_cleanup\n");
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
     playlist_t *plt = plt_get_curr ();
     int res = -1;
     if (plt) {
         res = plt_add_dir (plt, str, NULL, NULL);
         pl_save_current ();
         plt_unref (plt);
     }

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

     playlist_t *plt = plt_get_curr ();
     int res = -1;
     if (plt) {
         res = plt_add_file (plt, outname, NULL, NULL);
         pl_save_current ();
         plt_unref (plt);
     }

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
  (JNIEnv *env, jclass cls, jint trk)
{
    float dur = pl_get_item_duration ((playItem_t *)trk);
    char s[50];
    pl_format_time (dur, s, sizeof (s));
    return (*env)->NewStringUTF(env, s);
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

JNIEXPORT jboolean JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1paused
  (JNIEnv *env, jclass cls)
{
    return jni_out_get_state () == OUTPUT_STATE_PAUSED;
}

JNIEXPORT jboolean JNICALL Java_org_deadbeef_android_DeadbeefAPI_play_1is_1playing
  (JNIEnv *env, jclass cls)
{
    return (jni_out_get_state () == OUTPUT_STATE_PLAYING);
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_play_1is_1stopped (JNIEnv *env, jclass cls) {
    return (jni_out_get_state () == OUTPUT_STATE_STOPPED);
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_is_1streamer_1active (JNIEnv *env, jclass cls) {
    playItem_t *it = streamer_get_playing_track ();
    if (it) {
        pl_item_unref (it);
    }
    int res = (it ? 1 : 0) || !streamer_ok_to_read (-1);
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
    return (jint)pl_get_metadata_head ((playItem_t *)trk);
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

JNIEXPORT jfloat JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1totaltime (JNIEnv *env, jclass cls) {
    return pl_get_totaltime ();
}

JNIEXPORT jfloat JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1item_1duration (JNIEnv *env, jclass cls, jint trk) {
    return pl_get_item_duration ((playItem_t *)trk);
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1track_1filetype (JNIEnv *env, jclass cls, jint trk) {
    pl_lock ();
    const char *ft = pl_find_meta (((playItem_t *)trk), ":FILETYPE");
    jstring res = ft ? (*env)->NewStringUTF(env, ft) : NULL;;
    pl_unlock ();
    return res;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1insert_1dir (JNIEnv *env, jclass cls, jint plt, jint after, jstring path) {

    const char *str = (*env)->GetStringUTFChars(env, path, NULL);
    if (str == NULL) {
        return -1;
    }
    playItem_t *it = pl_get_for_idx_and_iter (after, PL_MAIN);
    int abort = 0;
    playlist_t *p = plt_get_for_idx (plt);
    playItem_t *ret = NULL;
    if (p) {
        playItem_t *ret = plt_insert_dir (p, it, str, &abort, NULL, NULL);
        plt_unref (p);
    }
    (*env)->ReleaseStringUTFChars(env, path, str);
    if (it) {
        pl_item_unref (it);
    }
    return ret ? 0 : -1;
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1get_1track_1path (JNIEnv *env, jclass cls, jint trk) {
    pl_lock ();
    const char *fname = pl_find_meta ((playItem_t *)trk, ":URI");
    jstring res = fname ? (*env)->NewStringUTF(env, fname) : NULL;
    pl_unlock ();
    return res;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1getcount (JNIEnv *env, jclass cls, jint iter) {
    return pl_getcount (iter);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1count (JNIEnv *env, jclass cls) {
    return plt_get_count ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1sel_1count (JNIEnv *env, jclass cls, jint idx) {
    return plt_get_sel_count (idx);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1add (JNIEnv *env, jclass cls, jint before, jstring title) {
     const char *str = (*env)->GetStringUTFChars(env, title, NULL);
     if (str == NULL) {
         return -1;
     }
     int res = plt_add (before, str);
     (*env)->ReleaseStringUTFChars(env, title, str);
     return res;
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1remove (JNIEnv *env, jclass cls, jint idx) {
    plt_remove (idx);
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1set_1curr_1idx (JNIEnv *env, jclass cls, jint idx) {
    plt_set_curr_idx (idx);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1curr (JNIEnv *env, jclass cls) {
    return plt_get_curr_idx ();
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1title (JNIEnv *env, jclass cls, jint idx) {
    char buf[1000];
    playlist_t *p = plt_get_for_idx (idx);
    if (p) {
        plt_get_title (p, buf, sizeof (buf));
        jstring s = (*env)->NewStringUTF(env, buf);
        plt_unref (p);
        return s;
    }
    return NULL;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1set_1title (JNIEnv *env, jclass cls, jint idx, jstring title) {
     const char *str = (*env)->GetStringUTFChars(env, title, NULL);
     if (str == NULL) {
         return -1;
     }
     playlist_t *p = plt_get_for_idx (idx);
     if (p) {
         plt_set_title (p, str);
         plt_unref (p);
     }
     (*env)->ReleaseStringUTFChars(env, title, str);
     return 0;
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1move (JNIEnv *env, jclass cls, jint from, jint to) {
    plt_move (from, to);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1playing_1track (JNIEnv *env, jclass cls) {
    return (jint)streamer_get_playing_track ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1streaming_1track (JNIEnv *env, jclass cls) {
    return (jint)streamer_get_streaming_track ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1current_1fileinfo (JNIEnv *env, jclass cls) {
    return (jint)streamer_get_current_fileinfo ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1current_1fileinfo_1format (JNIEnv *env, jclass cls) {
    DB_fileinfo_t *inf = streamer_get_current_fileinfo ();
    if (inf) {
        return (jint)&inf->fmt;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_fmt_1get_1channels
(JNIEnv *env, jclass cls, jint fmt) {
    return ((ddb_waveformat_t *)fmt)->channels;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_fmt_1get_1bps
(JNIEnv *env, jclass cls, jint fmt) {
    return ((ddb_waveformat_t *)fmt)->bps;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_fmt_1get_1samplerate
(JNIEnv *env, jclass cls, jint fmt) {
    return ((ddb_waveformat_t *)fmt)->samplerate;
}

JNIEXPORT jfloat JNICALL
Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1playpos (JNIEnv *env, jclass cls) {
    return streamer_get_playpos ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_streamer_1get_1apx_1bitrate (JNIEnv *env, jclass cls) {
    return streamer_get_apx_bitrate ();
}
JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_event_1is_1pending (JNIEnv *env, jclass cls) {
    jboolean res = 0;
    mutex_lock (jnievent_mutex);
    if (events) {
        res = 1;
    }
    mutex_unlock (jnievent_mutex);
    return res;
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_event_1dispatch (JNIEnv *env, jclass cls) {
    mutex_lock (jnievent_mutex);
    jnievent_dispatch ();
    mutex_unlock (jnievent_mutex);
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_event_1get_1type (JNIEnv *env, jclass cls) {
    mutex_lock (jnievent_mutex);
    if (!events) {
        mutex_unlock (jnievent_mutex);
        return NULL;
    }
    jstring s = (*env)->NewStringUTF(env, events->id);
    mutex_unlock (jnievent_mutex);
    return s;
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_event_1get_1string (JNIEnv *env, jclass cls, jint idx) {
    mutex_lock (jnievent_mutex);
    if (!events) {
        mutex_unlock (jnievent_mutex);
        return NULL;
    }
    jstring s = events->str_args[idx] ? (*env)->NewStringUTF(env, events->str_args[idx]) : NULL;
    mutex_unlock (jnievent_mutex);
    return s;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_event_1get_1int (JNIEnv *env, jclass cls, jint idx) {
    mutex_lock (jnievent_mutex);
    if (!events) {
        mutex_unlock (jnievent_mutex);
        return -1;
    }
    jint res = events->int_args[idx];
    mutex_unlock (jnievent_mutex);
    return res;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1find (JNIEnv *env, jclass cls, jstring name) {
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    ddb_dsp_context_t *ctx = streamer_get_dsp_chain ();
    while (ctx) {
        if (!strcmp (ctx->plugin->plugin.id, str)) {
            break;
        }
    }
    (*env)->ReleaseStringUTFChars(env, name, str);
    return (jint)ctx;
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1enable (JNIEnv *env, jclass cls, jint dsp, jboolean enable) {
    ddb_dsp_context_t *ctx = ((ddb_dsp_context_t *)dsp);
    ctx->enabled = enable;
    streamer_dsp_postinit ();
    if (enable) {
        streamer_reset (1);
    }
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1is_1enabled (JNIEnv *env, jclass cls, jint dsp) {
    ddb_dsp_context_t *ctx = ((ddb_dsp_context_t *)dsp);
    return ctx->enabled;
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1get_1param (JNIEnv *env, jclass cls, jint dsp, jint p) {
    ddb_dsp_context_t *ctx = ((ddb_dsp_context_t *)dsp);
    char param[1000];
    ctx->plugin->get_param (ctx, p, param, sizeof (param));
    return (*env)->NewStringUTF(env, param);
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1set_1param (JNIEnv *env, jclass cls, jint dsp, jint p, jstring v) {
    ddb_dsp_context_t *ctx = ((ddb_dsp_context_t *)dsp);
    const char *str = (*env)->GetStringUTFChars(env, v, NULL);
    ctx->plugin->set_param (ctx, p, str);
    (*env)->ReleaseStringUTFChars(env, v, str);
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1save_1config (JNIEnv *env, jclass cls) {
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/dspconfig", plug_get_config_dir ());
    struct ddb_dsp_context_s *dsp_chain = streamer_get_dsp_chain ();
    streamer_dsp_chain_save (fname, dsp_chain);
}

#ifdef HAVE_EQ
#define MAX_DSP_PRESETS 20
#define MAX_DSP_FNAME 100
char dsp_preset_names[MAX_DSP_PRESETS][100];
int num_dsp_presets = 0;

void
make_dsp_presets_path () {
    char dir[PATH_MAX];
    snprintf (dir, sizeof (dir), "%s/presets", dbconfdir);
    mkdir (dir, 0);
    snprintf (dir, sizeof (dir), "%s/presets/dsp", dbconfdir);
    mkdir (dir, 0);
    snprintf (dir, sizeof (dir), "%s/presets/dsp/eq", dbconfdir);
    mkdir (dir, 0);
}
#endif

static int dirent_alphasort (const struct dirent **a, const struct dirent **b) {
    return strcmp ((*a)->d_name, (*b)->d_name);
}

#ifdef HAVE_EQ
int
scan_dsp_presets (void) {
    make_dsp_presets_path ();
    num_dsp_presets = 0;

    char dir[PATH_MAX];
    snprintf (dir, sizeof (dir), "%s/presets/dsp/eq", dbconfdir);
    struct dirent **namelist = NULL;
    int n = scandir (dir, &namelist, NULL, dirent_alphasort);
    for (int i = 0; i < n; i++) {
        const char *ext = strrchr (namelist[i]->d_name, '.');
        if (ext && !strcmp (ext, ".txt")) {
            strncpy (dsp_preset_names[num_dsp_presets++], namelist[i]->d_name, MAX_DSP_FNAME);
            char *dot = strrchr (dsp_preset_names[num_dsp_presets-1], '.');
            if (dot) {
                *dot = 0;
            }
        }
        free (namelist[i]);
    }
    if (namelist) {
        free (namelist);
    }
}
#endif

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1num_1presets (JNIEnv *env, jclass cls, jint dsp) {
#ifdef HAVE_EQ
    return num_dsp_presets;
#else
    return 0;
#endif
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1preset_1name (JNIEnv *env, jclass cls, jint dsp, jint idx) {
#ifdef HAVE_EQ
    return (*env)->NewStringUTF(env, dsp_preset_names[idx]);
#else
    return NULL;
#endif
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1delete_1preset (JNIEnv *env, jclass cls, jint dsp, jint idx) {
#ifdef HAVE_EQ
    if (idx >= num_dsp_presets) {
        return -1;
    }
    char f[PATH_MAX];
    snprintf (f, sizeof (f), "%s/presets/dsp/eq/%s", dbconfdir, dsp_preset_names[idx]);
    unlink (f);
    if (idx < num_dsp_presets-1) {
        memmove (&dsp_preset_names[idx], &dsp_preset_names[idx+1], MAX_DSP_FNAME * (num_dsp_presets-idx-1));
    }
    num_dsp_presets--;
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1load_1preset (JNIEnv *env, jclass cls, jint dsp, jint idx) {
#ifdef HAVE_EQ
    char f[PATH_MAX];
    snprintf (f, sizeof (f), "%s/presets/dsp/eq/%s.txt", dbconfdir, dsp_preset_names[idx]);
    FILE *fp = fopen (f, "rt");
    if (!fp) {
        return -1;
    }

    ddb_dsp_context_t *ctx = (ddb_dsp_context_t *)dsp;
    int n = 0;
    char s[1000];
    while (1 == fscanf (fp, "%1000s\n", s) && n < 19) {
        ctx->plugin->set_param (ctx, n, s);
        n++;
    }

    fclose (fp);
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1save_1preset (JNIEnv *env, jclass cls, jint dsp, jstring name) {
#ifdef HAVE_EQ
    char f[PATH_MAX];
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    if (str == NULL) {
        return -1;
    }
    snprintf (f, sizeof (f), "%s/presets/dsp/eq/%s.txt", dbconfdir, str);
    (*env)->ReleaseStringUTFChars(env, name, str);

    FILE *fp = fopen (f, "w+t");
    if (!fp) {
        return -1;
    }

    ddb_dsp_context_t *ctx = (ddb_dsp_context_t *)dsp;

//    int n = ctx->plugin->num_params ();
    int n = 11;
    for (int i = 0; i < n; i++) {
        char s[1000];
//        ctx->plugin->get_param (ctx, i, s, sizeof (s));
//        fprintf (fp, "%s\n", s);
        fprintf (fp, "%f\n", eq_bands[i]);
    }

    fclose (fp);
    scan_dsp_presets ();
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_dsp_1rename_1preset (JNIEnv *env, jclass cls, jint dsp, jint idx, jstring name) {
#ifdef HAVE_EQ
    char newname[PATH_MAX];
    char oldname[PATH_MAX];
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    if (str == NULL) {
        return -1;
    }
    snprintf (newname, sizeof (newname), "%s/presets/dsp/eq/%s.txt", dbconfdir, str);
    (*env)->ReleaseStringUTFChars(env, name, str);

    snprintf (oldname, sizeof (oldname), "%s/presets/dsp/eq/%s.txt", dbconfdir, dsp_preset_names[idx]);

    rename (oldname, newname);
    scan_dsp_presets ();
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_conf_1save (JNIEnv *env, jclass cls) {
    return conf_save ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1item_1bitrate (JNIEnv *env, jclass cls, jint trk) {
    return pl_find_meta_int ((playItem_t *)trk, ":BITRATE", -1);
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plugin_1exists (JNIEnv *env, jclass cls, jstring id) {
    const jbyte *str;
    str = (*env)->GetStringUTFChars(env, id, NULL);
    if (str == NULL) {
        return 0;
    }
    if (!plug_get_for_id (str)) {
        (*env)->ReleaseStringUTFChars(env, id, str);
        return 0;
    }
    (*env)->ReleaseStringUTFChars(env, id, str);
    return 1;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_conf_1get_1int (JNIEnv *env, jclass cls, jstring key, jint def) {
    const jbyte *str;
    str = (*env)->GetStringUTFChars(env, key, NULL);
    int ret = conf_get_int (str, def);
    (*env)->ReleaseStringUTFChars(env, key, str);
    return ret;
}

JNIEXPORT void
JNICALL Java_org_deadbeef_android_DeadbeefAPI_conf_1set_1int (JNIEnv *env, jclass cls, jstring key, jint val) {
    const jbyte *str;
    str = (*env)->GetStringUTFChars(env, key, NULL);
    conf_set_int (str, val);
    (*env)->ReleaseStringUTFChars(env, key, str);
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_conf_1get_1str (JNIEnv *env, jclass cls, jstring key, jstring def) {
    jstring retstr = NULL;
    const jbyte *str, *defstr;
    str = (*env)->GetStringUTFChars(env, key, NULL);
    defstr = (*env)->GetStringUTFChars(env, def, NULL);
    conf_lock ();
    const char *ret = conf_get_str_fast (str, defstr);
    if (ret) {
        retstr = (*env)->NewStringUTF(env, ret);
    }
    conf_unlock ();
    (*env)->ReleaseStringUTFChars(env, key, str);
    (*env)->ReleaseStringUTFChars(env, def, defstr);
    return retstr;
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_conf_1set_1str (JNIEnv *env, jclass cls, jstring key, jstring val) {
    const jbyte *str, *valstr;
    str = (*env)->GetStringUTFChars(env, key, NULL);
    valstr = (*env)->GetStringUTFChars(env, val, NULL);
    conf_set_str (str, valstr);
    (*env)->ReleaseStringUTFChars(env, key, str);
    (*env)->ReleaseStringUTFChars(env, val, valstr);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_conf_1load (JNIEnv *env, jclass cls) {
    return conf_load ();
}


JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_reinit (JNIEnv *env, jclass cls) {
    int res = conf_load ();
    if (res) {
        trace ("conf_load failed\n");
        return -1;
    }
    pl_free ();
    res = pl_load_all ();
    if (res) {
        trace ("pl_load_all failed\n");
        return -1;
    }
    plt_set_curr_idx (conf_get_int ("playlist.current", 0));
    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1save_1current (JNIEnv *env, jclass cls) {
    return pl_save_current ();
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plug_1load_1all (JNIEnv *env, jclass cls) {
    return plug_load_all ();
}

// eq

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1enable (JNIEnv *env, jclass cls, jboolean enable) {
#ifdef HAVE_EQ
    eq_on = enable;
    eq_changed = 1;
    conf_set_int ("android.eq_enabled", eq_on);
    conf_save ();
#endif
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1is_1enabled (JNIEnv *env, jclass cls) {
#ifdef HAVE_EQ
    return eq_on;
#else
    return 0;
#endif
}

JNIEXPORT jfloat JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1get_1param (JNIEnv *env, jclass cls, jint p) {
#ifdef HAVE_EQ
    return (eq_bands[p] + 20) / 40.f * 100.f;
#else
    return 0;
#endif
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1set_1param (JNIEnv *env, jclass cls, jint p, jfloat v) {
#ifdef HAVE_EQ
    // convert from 0..100 range
    eq_bands[p] = v / 100.f * 40.f - 20.f;
    eq_changed = 1;
#endif
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1save_1preset (JNIEnv *env, jclass cls, jstring name) {
#ifdef HAVE_EQ
    char f[PATH_MAX];
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    if (str == NULL) {
        return -1;
    }
    snprintf (f, sizeof (f), "%s/presets/dsp/eq/%s.txt", dbconfdir, str);
    (*env)->ReleaseStringUTFChars(env, name, str);

    FILE *fp = fopen (f, "w+t");
    if (!fp) {
        return -1;
    }

    int n = 11;
    for (int i = 0; i < n; i++) {
        fprintf (fp, "%f\n", eq_bands[i]);
    }

    fclose (fp);
    scan_dsp_presets ();
#endif
    return 0;
}


JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1load_1preset (JNIEnv *env, jclass cls, jint idx) {
#ifdef HAVE_EQ
    char f[PATH_MAX];
    snprintf (f, sizeof (f), "%s/presets/dsp/eq/%s.txt", dbconfdir, dsp_preset_names[idx]);
    FILE *fp = fopen (f, "rt");
    if (!fp) {
        return -1;
    }

    int n = 0;
    float p;
    while (1 == fscanf (fp, "%f\n", &p) && n < 11) {
        eq_bands[n] = p;
        n++;
    }

    fclose (fp);
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1rename_1preset (JNIEnv *env, jclass cls, jint idx, jstring name) {
#ifdef HAVE_EQ
    char newname[PATH_MAX];
    char oldname[PATH_MAX];
    const char *str = (*env)->GetStringUTFChars(env, name, NULL);
    if (str == NULL) {
        return -1;
    }
    snprintf (newname, sizeof (newname), "%s/presets/dsp/eq/%s.txt", dbconfdir, str);
    (*env)->ReleaseStringUTFChars(env, name, str);

    snprintf (oldname, sizeof (oldname), "%s/presets/dsp/eq/%s.txt", dbconfdir, dsp_preset_names[idx]);

    rename (oldname, newname);
    scan_dsp_presets ();
#endif
    return 0;
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1preset_1name (JNIEnv *env, jclass cls, jint idx) {
#ifdef HAVE_EQ
    return (*env)->NewStringUTF(env, dsp_preset_names[idx]);
#else
    return NULL;
#endif
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1num_1presets (JNIEnv *env, jclass cls) {
#ifdef HAVE_EQ
    return num_dsp_presets;
#else
    return 0;
#endif
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1delete_1preset (JNIEnv *env, jclass cls, jint idx) {
#ifdef HAVE_EQ
    if (idx >= num_dsp_presets) {
        return -1;
    }
    char f[PATH_MAX];
    snprintf (f, sizeof (f), "%s/presets/dsp/eq/%s", dbconfdir, dsp_preset_names[idx]);
    unlink (f);
    if (idx < num_dsp_presets-1) {
        memmove (&dsp_preset_names[idx], &dsp_preset_names[idx+1], MAX_DSP_FNAME * (num_dsp_presets-idx-1));
    }
    num_dsp_presets--;
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_eq_1save_1config (JNIEnv *env, jclass cls) {
#ifdef HAVE_EQ
    char f[PATH_MAX];
    snprintf (f, sizeof (f), "%s/eqconfig", dbconfdir);
    unlink (f);
    FILE *fp = fopen (f, "w+t");
    if (!fp) {
        return -1;
    }

    int n = 11;
    for (int i = 0; i < n; i++) {
        fprintf (fp, "%f\n", eq_bands[i]);
    }

    fclose (fp);
#endif
    return 0;
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1remove_1item (JNIEnv *env, jclass cls, jint plt, jint it) {
    playlist_t *p = plt_get_for_idx (plt);
    if (p) {
        playItem_t *i = plt_get_item_for_idx (p, it, PL_MAIN);
        if (i) {
            plt_remove_item (p, i);
        }
        plt_unref (p);
    }
}

JNIEXPORT jstring JNICALL
Java_org_deadbeef_android_DeadbeefAPI_pl_1format_1title (JNIEnv *env, jclass cls, jint trk, jint idx, jint id, jstring fmt) {
    char buffer[1000];
    const char *str = (*env)->GetStringUTFChars(env, fmt, NULL);
    pl_format_title ((playItem_t *)trk, idx, buffer, sizeof (buffer), id, str);
    (*env)->ReleaseStringUTFChars(env, fmt, str);
    return (*env)->NewStringUTF(env, buffer);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_str_1get_1idx_1of (JNIEnv *env, jclass cls, jint trk) {
    return str_get_idx_of ((playItem_t *)trk);
}

JNIEXPORT jint JNICALL
Java_org_deadbeef_android_DeadbeefAPI_plt_1get_1item_1count (JNIEnv *env, jclass cls, jint idx) {
    playlist_t *plt = plt_get_for_idx (idx);
    if (plt) {
        int cnt = plt_get_item_count (plt, PL_MAIN);
        plt_unref (plt);
        return cnt;
    }
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_neon_1supported (JNIEnv *env, jclass cls) {
    return neon_supported;
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_vfp_1supported (JNIEnv *env, jclass cls) {
    return vfp_supported;
}

JNIEXPORT jboolean JNICALL
Java_org_deadbeef_android_DeadbeefAPI_armv7a_1supported (JNIEnv *env, jclass cls) {
    return armv7a_supported;
}

JNIEXPORT void JNICALL
Java_org_deadbeef_android_DeadbeefAPI_save_1resume_1state (JNIEnv *env, jclass cls) {
    playItem_t *trk = streamer_get_playing_track ();
    DB_output_t *output = plug_get_output ();
    float playpos = -1;
    int playtrack = -1;
    int playlist = streamer_get_current_playlist ();
    int paused = (output->state () == OUTPUT_STATE_PAUSED);
    if (trk && playlist >= 0) {
        playtrack = str_get_idx_of (trk);
        playpos = streamer_get_playpos ();
        pl_item_unref (trk);
    }

    conf_set_float ("resume.position", playpos);
    conf_set_int ("resume.track", playtrack);
    conf_set_int ("resume.playlist", playlist);
    conf_set_int ("resume.paused", 1);
    trace ("saved session: pos=%f track=%d plt=%d\n", playpos, playtrack, playlist);
}

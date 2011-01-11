/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/prctl.h>
#include "../../deadbeef.h"
#include "../../config.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

#define LOCK {deadbeef->mutex_lock (mutex); /*fprintf (stderr, "alsa lock %s:%d\n", __FILE__, __LINE__);*/}
#define UNLOCK {deadbeef->mutex_unlock (mutex); /*fprintf (stderr, "alsa unlock %s:%d\n", __FILE__, __LINE__);*/}

#define DEFAULT_BUFFER_SIZE 8192
#define DEFAULT_PERIOD_SIZE 1024
#define DEFAULT_BUFFER_SIZE_STR "8192"
#define DEFAULT_PERIOD_SIZE_STR "1024"

static DB_output_t plugin;
DB_functions_t *deadbeef;

static snd_pcm_t *audio;
static int alsa_terminate;
static ddb_waveformat_t requested_fmt;
static int state; // one of output_state_t
static uintptr_t mutex;
static intptr_t alsa_tid;

static snd_pcm_uframes_t buffer_size;
static snd_pcm_uframes_t period_size;

static snd_pcm_uframes_t req_buffer_size;
static snd_pcm_uframes_t req_period_size;

static char conf_alsa_soundcard[100] = "default";

//static snd_async_handler_t *pcm_callback;

static int
palsa_callback (char *stream, int len);

#if 0
static void
alsa_callback (snd_async_handler_t *pcm_callback) {
    snd_pcm_t *pcm_handle = snd_async_handler_get_pcm(pcm_callback);
    snd_pcm_sframes_t avail;
    int err;
    printf ("alsa_callback\n");

    avail = snd_pcm_avail_update(pcm_handle);
    while (avail >= period_size) {
        char buf[avail * 4];
        palsa_callback (buf, avail * 4);
        if ((err = snd_pcm_writei (pcm_handle, buf, period_size)) < 0) {
            perror ("snd_pcm_writei");
        }
        avail = snd_pcm_avail_update(pcm_handle);
    }
}
#endif

static void
palsa_thread (void *context);

static int
palsa_init (void);

static int
palsa_free (void);

static int
palsa_setformat (ddb_waveformat_t *fmt);

static int
palsa_play (void);

static int
palsa_stop (void);

static int
palsa_pause (void);

static int
palsa_unpause (void);

static int
palsa_get_channels (void);

static int
palsa_get_endianness (void);

static void
palsa_enum_soundcards (void (*callback)(const char *name, const char *desc, void*), void *userdata);

static int
palsa_set_hw_params (ddb_waveformat_t *fmt) {
    snd_pcm_hw_params_t *hw_params = NULL;
    int err = 0;

    memcpy (&plugin.fmt, fmt, sizeof (ddb_waveformat_t));
    if (!plugin.fmt.channels) {
        // generic format
        plugin.fmt.bps = 16;
        plugin.fmt.is_float = 0;
        plugin.fmt.channels = 2;
        plugin.fmt.samplerate = 44100;
        plugin.fmt.channelmask = 3;
    }
retry:

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_any (audio, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_set_access (audio, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
                snd_strerror (err));
        goto error;
    }

    snd_pcm_format_t sample_fmt;

    switch (plugin.fmt.bps) {
    case 8:
        sample_fmt = SND_PCM_FORMAT_S8;
        break;
    case 16:
#if WORDS_BIGENDIAN
        sample_fmt = SND_PCM_FORMAT_S16_BE;
#else
        sample_fmt = SND_PCM_FORMAT_S16_LE;
#endif
        break;
    case 24:
#if WORDS_BIGENDIAN
        sample_fmt = SND_PCM_FORMAT_S24_3BE;
#else
        sample_fmt = SND_PCM_FORMAT_S24_3LE;
#endif
        break;
    case 32:
        if (plugin.fmt.is_float) {
#if WORDS_BIGENDIAN
            sample_fmt = SND_PCM_FORMAT_FLOAT_BE;
#else
            sample_fmt = SND_PCM_FORMAT_FLOAT_LE;
#endif
        }
        else {
#if WORDS_BIGENDIAN
            sample_fmt = SND_PCM_FORMAT_S32_BE;
#else
            sample_fmt = SND_PCM_FORMAT_S32_LE;
#endif
        }
        break;
    };

    //sample_fmt = SND_PCM_FORMAT_S16_LE;
    if ((err = snd_pcm_hw_params_set_format (audio, hw_params, sample_fmt)) < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n",
                snd_strerror (err));
        goto error;
    }

    snd_pcm_hw_params_get_format (hw_params, &sample_fmt);
    trace ("chosen sample format: %04Xh\n", (int)sample_fmt);

    int val = plugin.fmt.samplerate;
    int ret = 0;

    if ((err = snd_pcm_hw_params_set_rate_resample (audio, hw_params, 1)) < 0) {
        fprintf (stderr, "cannot setup resampling (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_set_rate_near (audio, hw_params, &val, &ret)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
                snd_strerror (err));
        goto error;
    }
    plugin.fmt.samplerate = val;
    trace ("chosen samplerate: %d Hz\n", val);

    if ((err = snd_pcm_hw_params_set_channels (audio, hw_params, plugin.fmt.channels)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
                snd_strerror (err));
        goto error;
    }

    int nchan;
    snd_pcm_hw_params_get_channels (hw_params, &nchan);
    trace ("alsa channels: %d\n", nchan);

    req_buffer_size = deadbeef->conf_get_int ("alsa.buffer", DEFAULT_BUFFER_SIZE);
    req_period_size = deadbeef->conf_get_int ("alsa.period", DEFAULT_PERIOD_SIZE);
    buffer_size = req_buffer_size;
    period_size = req_period_size;
    trace ("trying buffer size: %d frames\n", (int)buffer_size);
    trace ("trying period size: %d frames\n", (int)period_size);
    snd_pcm_hw_params_set_buffer_size_near (audio, hw_params, &buffer_size);
    snd_pcm_hw_params_set_period_size_near (audio, hw_params, &period_size, NULL);
    trace ("alsa buffer size: %d frames\n", (int)buffer_size);
    trace ("alsa period size: %d frames\n", (int)period_size);

    if ((err = snd_pcm_hw_params (audio, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
                snd_strerror (err));
        goto error;

//        if (plugin.fmt.channels > 2 && plugin.fmt.samplerate >= 96000) {
//            plugin.fmt.samplerate = 48000;
//            fprintf (stderr, "falling back to 48000KHz\n");
//            goto retry;
//        }
    }

    plugin.fmt.is_float = 0;
    switch (sample_fmt) {
    case SND_PCM_FORMAT_S8:
        plugin.fmt.bps = 8;
        break;
    case SND_PCM_FORMAT_S16_BE:
    case SND_PCM_FORMAT_S16_LE:
        plugin.fmt.bps = 16;
        break;
    case SND_PCM_FORMAT_S24_3BE:
    case SND_PCM_FORMAT_S24_3LE:
        plugin.fmt.bps = 24;
        break;
    case SND_PCM_FORMAT_S32_BE:
    case SND_PCM_FORMAT_S32_LE:
        plugin.fmt.bps = 32;
        break;
    case SND_PCM_FORMAT_FLOAT_LE:
    case SND_PCM_FORMAT_FLOAT_BE:
        plugin.fmt.bps = 32;
        plugin.fmt.is_float = 1;
        break;
    }

    trace ("chosen bps: %d (%s)\n", plugin.fmt.bps, plugin.fmt.is_float ? "float" : "int");

    plugin.fmt.channels = nchan;
    plugin.fmt.channelmask = 0;
    if (nchan == 1) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT;
    }
    if (nchan == 2) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT;
    }
    if (nchan == 3) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_LOW_FREQUENCY;
    }
    if (nchan == 4) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT;
    }
    if (nchan == 5) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER;
    }
    if (nchan == 6) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER | DDB_SPEAKER_LOW_FREQUENCY;
    }
    if (nchan == 7) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER | DDB_SPEAKER_SIDE_LEFT | DDB_SPEAKER_SIDE_RIGHT;
    }
    if (nchan == 8) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER | DDB_SPEAKER_SIDE_LEFT | DDB_SPEAKER_SIDE_RIGHT | DDB_SPEAKER_LOW_FREQUENCY;
    }
error:
    if (err < 0) {
        memset (&plugin.fmt, 0, sizeof (ddb_waveformat_t));
    }
    if (hw_params) {
        snd_pcm_hw_params_free (hw_params);
    }
    return err;
}

int
palsa_init (void) {
    int err;
    alsa_tid = 0;
    mutex = 0;

    // get and cache conf variables
    strcpy (conf_alsa_soundcard, deadbeef->conf_get_str ("alsa_soundcard", "default"));
    trace ("alsa_soundcard: %s\n", conf_alsa_soundcard);

    snd_pcm_sw_params_t *sw_params = NULL;
    state = OUTPUT_STATE_STOPPED;
    //const char *conf_alsa_soundcard = conf_get_str ("alsa_soundcard", "default");
    if ((err = snd_pcm_open (&audio, conf_alsa_soundcard, SND_PCM_STREAM_PLAYBACK, 0))) {
        fprintf (stderr, "could not open audio device (%s)\n",
                snd_strerror (err));
        return -1;
    }

    mutex = deadbeef->mutex_create ();

    if (requested_fmt.samplerate != 0) {
        memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));
    }

    if (palsa_set_hw_params (&plugin.fmt) < 0) {
        goto open_error;
    }

    if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) {
        fprintf (stderr, "cannot allocate software parameters structure (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    if ((err = snd_pcm_sw_params_current (audio, sw_params)) < 0) {
        fprintf (stderr, "cannot initialize software parameters structure (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    snd_pcm_sw_params_set_start_threshold (audio, sw_params, buffer_size - period_size);

    if ((err = snd_pcm_sw_params_set_avail_min (audio, sw_params, period_size)) < 0) {
        fprintf (stderr, "cannot set minimum available count (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    snd_pcm_uframes_t av;
    if ((err = snd_pcm_sw_params_get_avail_min (sw_params, &av)) < 0) {
        fprintf (stderr, "snd_pcm_sw_params_get_avail_min failed (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    fprintf (stderr, "alsa avail_min: %d frames\n", (int)av);


//    if ((err = snd_pcm_sw_params_set_start_threshold (audio, sw_params, 0U)) < 0) {
//        trace ("cannot set start mode (%s)\n",
//                snd_strerror (err));
//        goto open_error;
//    }

    if ((err = snd_pcm_sw_params (audio, sw_params)) < 0) {
        fprintf (stderr, "cannot set software parameters (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    snd_pcm_sw_params_free (sw_params);
    sw_params = NULL;

    /* the interface will interrupt the kernel every N frames, and ALSA
       will wake up this program very soon after that.
       */

    if ((err = snd_pcm_prepare (audio)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    alsa_terminate = 0;
    alsa_tid = deadbeef->thread_start (palsa_thread, NULL);

    return 0;

open_error:
    if (sw_params) {
        snd_pcm_sw_params_free (sw_params);
    }
    if (audio != NULL) {
        palsa_free ();
    }

    return -1;
}

int
palsa_setformat (ddb_waveformat_t *fmt) {
    memcpy (&requested_fmt, fmt, sizeof (ddb_waveformat_t));
    trace ("palsa_setformat %dbit %s %dch %dHz channelmask=%X\n", fmt->bps, fmt->is_float ? "float" : "int", fmt->channels, fmt->samplerate, fmt->channelmask);
    if (!audio) {
        return -1;
    }
    if (!memcmp (fmt, &plugin.fmt, sizeof (ddb_waveformat_t))) {
        trace ("palsa_setformat ignored\n");
        return 0;
    }
#if 0
    else {
        trace ("switching format:\n"
        "bps %d -> %d\n"
        "is_float %d -> %d\n"
        "is_multichannel %d -> %d\n"
        "channels %d -> %d\n"
        "samplerate %d -> %d\n"
        "channelmask %d -> %d\n"
        , fmt->bps, plugin.fmt.bps
        , fmt->is_float, plugin.fmt.is_float
        , fmt->is_multichannel, plugin.fmt.is_multichannel
        , fmt->channels, plugin.fmt.channels
        , fmt->samplerate, plugin.fmt.samplerate
        , fmt->channelmask, plugin.fmt.channelmask
        );
    }
#endif
    LOCK;
    int s = state;
    state = OUTPUT_STATE_STOPPED;
    snd_pcm_drop (audio);
    int ret = palsa_set_hw_params (fmt);
    UNLOCK;
    if (ret < 0) {
        trace ("palsa_change_rate: impossible to set requested format\n");
        return -1;
    }
    trace ("new format %dbit %s %dch %dHz channelmask=%X\n", plugin.fmt.bps, plugin.fmt.is_float ? "float" : "int", plugin.fmt.channels, plugin.fmt.samplerate, plugin.fmt.channelmask);

    switch (s) {
    case OUTPUT_STATE_STOPPED:
        return palsa_stop ();
    case OUTPUT_STATE_PLAYING:
        return palsa_play ();
    case OUTPUT_STATE_PAUSED:
        if (0 != palsa_play ()) {
            return -1;
        }
        if (0 != palsa_pause ()) {
            return -1;
        }
        break;
    }
    return 0;
}

int
palsa_free (void) {
    trace ("palsa_free\n");
    if (audio && !alsa_terminate) {
        LOCK;
        alsa_terminate = 1;
        UNLOCK;
        trace ("waiting for alsa thread to finish\n");
        if (alsa_tid) {
            deadbeef->thread_join (alsa_tid);
            alsa_tid = 0;
        }
        snd_pcm_close(audio);
        audio = NULL;
        if (mutex) {
            deadbeef->mutex_free (mutex);
            mutex = 0;
        }
        state = OUTPUT_STATE_STOPPED;
        alsa_terminate = 0;
    }
    return 0;
}

static void
palsa_hw_pause (int pause) {
    if (!audio) {
        return;
    }
    if (state == OUTPUT_STATE_STOPPED) {
        return;
    }
    if (pause == 1) {
        snd_pcm_drop (audio);
    }
    else {
        snd_pcm_prepare (audio);
        snd_pcm_start (audio);
    }
}

int
palsa_play (void) {
    int err;
    if (state == OUTPUT_STATE_STOPPED) {
        if (!audio) {
            if (palsa_init () < 0) {
                state = OUTPUT_STATE_STOPPED;
                return -1;
            }
        }
        else {
            if ((err = snd_pcm_prepare (audio)) < 0) {
                fprintf (stderr, "cannot prepare audio interface for use (%d, %s)\n",
                        err, snd_strerror (err));
                return -1;
            }
        }
    }
    if (state != OUTPUT_STATE_PLAYING) {
        LOCK;
//        trace ("alsa: installing async handler\n");
//        if (snd_async_add_pcm_handler (&pcm_callback, audio, alsa_callback, NULL) < 0) {
//            perror ("snd_async_add_pcm_handler");
//        }
//        trace ("pcm_callback=%p\n", pcm_callback);
        snd_pcm_start (audio);
        UNLOCK;
        state = OUTPUT_STATE_PLAYING;
    }
    return 0;
}


int
palsa_stop (void) {
    if (!audio) {
        return 0;
    }
    state = OUTPUT_STATE_STOPPED;
    LOCK;
    snd_pcm_drop (audio);
#if 0
    if (pcm_callback) {
        snd_async_del_handler (pcm_callback);
        pcm_callback = NULL;
    }
#endif
    UNLOCK;
    deadbeef->streamer_reset (1);
    if (deadbeef->conf_get_int ("alsa.freeonstop", 0))  {
        palsa_free ();
    }
    return 0;
}

int
palsa_pause (void) {
    if (state == OUTPUT_STATE_STOPPED || !audio) {
        return -1;
    }
    // set pause state
    LOCK;
    palsa_hw_pause (1);
    UNLOCK;
    state = OUTPUT_STATE_PAUSED;
    return 0;
}

int
palsa_unpause (void) {
    // unset pause state
    if (state == OUTPUT_STATE_PAUSED) {
        state = OUTPUT_STATE_PLAYING;
        LOCK;
        palsa_hw_pause (0);
        UNLOCK;
    }
    return 0;
}

static void
palsa_thread (void *context) {
    prctl (PR_SET_NAME, "deadbeef-alsa", 0, 0, 0, 0);
    int err;
    for (;;) {
        if (alsa_terminate) {
            break;
        }
        if (state != OUTPUT_STATE_PLAYING || !deadbeef->streamer_ok_to_read (-1)) {
            usleep (10000);
            continue;
        }
        LOCK;
        /* find out how much space is available for playback data */
        snd_pcm_sframes_t frames_to_deliver = snd_pcm_avail_update (audio);
        while (frames_to_deliver >= period_size) {
            if (alsa_terminate) {
                break;
            }
            err = 0;
            char buf[period_size * (plugin.fmt.bps>>3) * plugin.fmt.channels];
            int bytes_to_write = palsa_callback (buf, period_size * (plugin.fmt.bps>>3) * plugin.fmt.channels);

            if (bytes_to_write >= (plugin.fmt.bps>>3) * plugin.fmt.channels) {
               err = snd_pcm_writei (audio, buf, snd_pcm_bytes_to_frames(audio, bytes_to_write));
            }
            else {
                UNLOCK;
                usleep (10000);
                LOCK;
                continue;
            }

            if (err < 0) {
                if (err == -ESTRPIPE) {
                    fprintf (stderr, "alsa: trying to recover from suspend... (error=%d, %s)\n", err,  snd_strerror (err));
                    while ((err = snd_pcm_resume(audio)) == -EAGAIN) {
                        sleep(1); /* wait until the suspend flag is released */
                    }
                    if (err < 0) {
                        err = snd_pcm_prepare(audio);
                        if (err < 0) {
                            fprintf (stderr, "Can't recovery from suspend, prepare failed: %s", snd_strerror(err));
                            exit (-1);
                        }
                    }
            //        deadbeef->sendmessage (M_REINIT_SOUND, 0, 0, 0);
            //        break;
                }
                else {
                    if (err != -EPIPE) {
                        fprintf (stderr, "alsa: snd_pcm_writei error=%d, %s\n", err, snd_strerror (err));
                    }
                    snd_pcm_prepare (audio);
                    snd_pcm_start (audio);
                    continue;
                }
            }
            frames_to_deliver = snd_pcm_avail_update (audio);
        }
        UNLOCK;
        usleep (period_size * 1000000 / plugin.fmt.samplerate / 2);
    }
}

static int
palsa_callback (char *stream, int len) {
    return deadbeef->streamer_read (stream, len);
}

static int
palsa_configchanged (DB_event_t *ev, uintptr_t data) {
    const char *alsa_soundcard = deadbeef->conf_get_str ("alsa_soundcard", "default");
    int buffer = deadbeef->conf_get_int ("alsa.buffer", DEFAULT_BUFFER_SIZE);
    int period = deadbeef->conf_get_int ("alsa.period", DEFAULT_PERIOD_SIZE);
    if (audio &&
            (strcmp (alsa_soundcard, conf_alsa_soundcard)
            || buffer != req_buffer_size
            || period != req_period_size)) {
        trace ("alsa: config option changed, restarting\n");
        deadbeef->sendmessage (M_REINIT_SOUND, 0, 0, 0);
    }
    return 0;
}

// derived from alsa-utils/aplay.c
static void
palsa_enum_soundcards (void (*callback)(const char *name, const char *desc, void *), void *userdata) {
    void **hints, **n;
    char *name, *descr, *io;
    const char *filter = "Output";
    if (snd_device_name_hint(-1, "pcm", &hints) < 0)
        return;
    n = hints;
    while (*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        descr = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if (io == NULL || !strcmp(io, filter)) {
            if (name && descr && callback) {
                callback (name, descr, userdata);
            }
        }
        if (name != NULL)
            free(name);
        if (descr != NULL)
            free(descr);
        if (io != NULL)
            free(io);
        n++;
    }
    snd_device_name_free_hint(hints);
}

static int
palsa_get_state (void) {
    return state;
}

static int
alsa_start (void) {
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (palsa_configchanged), 0);
    return 0;
}

static int
alsa_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (palsa_configchanged), 0);
    return 0;
}

DB_plugin_t *
alsa_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static const char settings_dlg[] =
    "property \"Release device while stopped\" checkbox alsa.freeonstop 0;\n"
    "property \"Preferred buffer size\" entry alsa.buffer " DEFAULT_BUFFER_SIZE_STR ";\n"
    "property \"Preferred period size\" entry alsa.period " DEFAULT_PERIOD_SIZE_STR ";\n"
;

// define plugin interface
static DB_output_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.name = "ALSA output plugin",
    .plugin.descr = "plays sound through linux standard alsa library",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = alsa_start,
    .plugin.stop = alsa_stop,
    .plugin.configdialog = settings_dlg,
    .init = palsa_init,
    .free = palsa_free,
    .setformat = palsa_setformat,
    .play = palsa_play,
    .stop = palsa_stop,
    .pause = palsa_pause,
    .unpause = palsa_unpause,
    .state = palsa_get_state,
    .enum_soundcards = palsa_enum_soundcards,
};

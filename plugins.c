/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <dirent.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
//#include <alloca.h>
#include <string.h>
#ifndef __linux__
#define _POSIX_C_SOURCE
#endif
#include <limits.h>
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "gettext.h"
#include "plugins.h"
#include "md5/md5.h"
#include "messagepump.h"
#include "threading.h"
#include "playlist.h"
#include "volume.h"
#include "streamer.h"
#include "common.h"
#include "conf.h"
#include "junklib.h"
#include "vfs.h"
#include "premix.h"
#include "dsppreset.h"

extern void android_trace (const char *fmt, ...);
#define trace(...) { android_trace(__VA_ARGS__); }
//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#ifndef PATH_MAX
#define PATH_MAX    1024    /* max # of characters in a path name */
#endif

//#define DISABLE_VERSIONCHECK 1

static uintptr_t mutex;

// deadbeef api
static DB_functions_t deadbeef_api = {
    // FIXME: set to 1.0 after api freeze
    .vmajor = 0,
    .vminor = 0,
    .ev_subscribe = plug_ev_subscribe,
    .ev_unsubscribe = plug_ev_unsubscribe,
    .md5 = plug_md5,
    .md5_to_str = plug_md5_to_str,
    .md5_init = (void (*)(DB_md5_t *s))md5_init,
    .md5_append = (void (*)(DB_md5_t *s, const uint8_t *daya, int nbytes))md5_append,
    .md5_finish = (void (*)(DB_md5_t *s, uint8_t digest[16]))md5_finish,
    .get_output = plug_get_output,
    .playback_get_pos = plug_playback_get_pos,
    .playback_set_pos = plug_playback_set_pos,
    // streamer access
    .streamer_get_playing_track = (DB_playItem_t *(*) (void))streamer_get_playing_track,
    .streamer_get_streaming_track = (DB_playItem_t *(*) (void))streamer_get_streaming_track,
    .streamer_get_playpos = streamer_get_playpos,
    .streamer_seek = streamer_set_seek,
    .streamer_ok_to_read = streamer_ok_to_read,
    .streamer_reset = streamer_reset,
    .streamer_read = streamer_read,
    .streamer_set_bitrate = streamer_set_bitrate,
    .streamer_get_apx_bitrate = streamer_get_apx_bitrate,
    .streamer_get_current_fileinfo = streamer_get_current_fileinfo,
    .streamer_get_current_playlist = streamer_get_current_playlist,
    .streamer_get_dsp_chain = streamer_get_dsp_chain,
    .streamer_set_dsp_chain = streamer_set_dsp_chain,
    // folders
    .get_config_dir = plug_get_config_dir,
    .get_prefix = plug_get_prefix,
    .get_doc_dir = plug_get_doc_dir,
    .get_plugin_dir = plug_get_plugin_dir,
    .get_pixmap_dir = plug_get_pixmap_dir,
    // threading
    .thread_start = thread_start,
    .thread_start_low_priority = thread_start_low_priority,
    .thread_join = thread_join,
    .thread_detach = thread_detach,
    .thread_exit = thread_exit,
    .mutex_create = mutex_create,
    .mutex_create_nonrecursive = mutex_create_nonrecursive,
    .mutex_free = mutex_free,
    .mutex_lock = mutex_lock,
    .mutex_unlock = mutex_unlock,
    .cond_create = cond_create,
    .cond_free = cond_free,
    .cond_wait = cond_wait,
    .cond_signal = cond_signal,
    .cond_broadcast = cond_broadcast,
    // playlist management
    .plt_get_count = plt_get_count,
    .plt_get_head = (DB_playItem_t * (*) (int plt))plt_get_head,
    .plt_get_sel_count = plt_get_sel_count,
    .plt_add = plt_add,
    .plt_remove = plt_remove,
    .plt_free = plt_free,
    .plt_set_curr = plt_set_curr,
    .plt_get_curr = plt_get_curr,
    .plt_get_title = plt_get_title,
    .plt_set_title = plt_set_title,
    .plt_move = plt_move,
    // playlist access
    .pl_lock = pl_lock,
    .pl_unlock = pl_unlock,
    .plt_lock = plt_lock,
    .plt_unlock = plt_unlock,
    .pl_item_alloc = (DB_playItem_t* (*)(void))pl_item_alloc,
    .pl_item_ref = (void (*)(DB_playItem_t *))pl_item_ref,
    .pl_item_unref = (void (*)(DB_playItem_t *))pl_item_unref,
    .pl_item_copy = (void (*)(DB_playItem_t *, DB_playItem_t *))pl_item_copy,
    .pl_add_file = (int (*) (const char *, int (*cb)(DB_playItem_t *it, void *data), void *))pl_add_file,
    .pl_add_dir = (int (*) (const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data))pl_add_dir,
    .pl_add_files_begin = pl_add_files_begin,
    .pl_add_files_end = pl_add_files_end,
    .pl_insert_item = (DB_playItem_t *(*) (DB_playItem_t *after, DB_playItem_t *it))pl_insert_item,
    .pl_insert_dir = (DB_playItem_t *(*) (DB_playItem_t *after, const char *dirname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data))pl_insert_dir,
    .pl_insert_file = (DB_playItem_t *(*) (DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data))pl_insert_file,
    .pl_get_idx_of = (int (*) (DB_playItem_t *it))pl_get_idx_of,
    .pl_get_idx_of_iter = (int (*) (DB_playItem_t *it, int iter))pl_get_idx_of_iter,
    .pl_get_for_idx = (DB_playItem_t * (*)(int))pl_get_for_idx,
    .pl_get_for_idx_and_iter = (DB_playItem_t * (*) (int idx, int iter))pl_get_for_idx_and_iter,
    .pl_set_item_duration = (void (*) (DB_playItem_t *it, float duration))pl_set_item_duration,
    .pl_get_item_duration = (float (*) (DB_playItem_t *it))pl_get_item_duration,
    .pl_get_item_flags = (uint32_t (*) (DB_playItem_t *it))pl_get_item_flags,
    .pl_set_item_flags = (void (*) (DB_playItem_t *it, uint32_t flags))pl_set_item_flags,
    .pl_sort = pl_sort,
    .pl_items_copy_junk = (void (*)(DB_playItem_t *from, DB_playItem_t *first, DB_playItem_t *last))pl_items_copy_junk,
    .pl_get_totaltime = pl_get_totaltime,
    .pl_getcount = pl_getcount,
    .pl_delete_selected = pl_delete_selected,
    .pl_set_cursor = pl_set_cursor,
    .pl_get_cursor = pl_get_cursor,
    .pl_set_selected = (void (*) (DB_playItem_t *, int))pl_set_selected,
    .pl_is_selected = (int (*) (DB_playItem_t *))pl_is_selected,
    .pl_clear = pl_clear,
    .pl_load = pl_load,
    .pl_save = pl_save,
    .pl_save_current = pl_save_current,
    .pl_save_all = pl_save_all,
    .pl_select_all = pl_select_all,
    .pl_crop_selected = pl_crop_selected,
    .pl_getselcount = pl_getselcount,
    .pl_get_first = (DB_playItem_t *(*) (int))pl_get_first,
    .pl_get_last = (DB_playItem_t *(*) (int))pl_get_last,
    .pl_get_next = (DB_playItem_t *(*) (DB_playItem_t *, int))pl_get_next,
    .pl_get_prev = (DB_playItem_t *(*) (DB_playItem_t *, int))pl_get_prev,
    .pl_format_title = (int (*) (DB_playItem_t *it, int idx, char *s, int size, int id, const char *fmt))pl_format_title,
    .pl_format_title_escaped = (int (*) (DB_playItem_t *it, int idx, char *s, int size, int id, const char *fmt))pl_format_title_escaped,
    .pl_format_time = pl_format_time,
    .pl_move_items = (void (*) (int iter, int plt_from, DB_playItem_t *drop_before, uint32_t *indexes, int count))pl_move_items,
    .pl_copy_items = (void (*) (int iter, int plt_from, DB_playItem_t *before, uint32_t *indices, int cnt))pl_copy_items,
    .pl_search_reset = pl_search_reset,
    .pl_search_process = pl_search_process,
    // metainfo
    .pl_add_meta = (void (*) (DB_playItem_t *, const char *, const char *))pl_add_meta,
    .pl_append_meta = (void (*) (DB_playItem_t *, const char *, const char *))pl_append_meta,
    .pl_find_meta = (const char *(*) (DB_playItem_t *, const char *))pl_find_meta,
    .pl_replace_meta = (void (*) (DB_playItem_t *, const char *, const char *))pl_replace_meta,
    .pl_delete_all_meta = (void (*) (DB_playItem_t *it))pl_delete_all_meta,
    .pl_get_metadata = (DB_metaInfo_t *(*)(DB_playItem_t *it))pl_get_metadata,
    // cuesheet support
    .pl_insert_cue_from_buffer = (DB_playItem_t *(*) (DB_playItem_t *after, DB_playItem_t *origin, const uint8_t *buffer, int buffersize, int numsamples, int samplerate))pl_insert_cue_from_buffer,
    .pl_insert_cue = (DB_playItem_t *(*)(DB_playItem_t *after, DB_playItem_t *origin, int numsamples, int samplerate))pl_insert_cue,
    // playqueue support
    .pl_playqueue_push = (int (*) (DB_playItem_t *))pl_playqueue_push,
    .pl_playqueue_clear = pl_playqueue_clear,
    .pl_playqueue_pop = pl_playqueue_pop,
    .pl_playqueue_remove = (void (*) (DB_playItem_t *))pl_playqueue_remove,
    .pl_playqueue_test = (int (*) (DB_playItem_t *))pl_playqueue_test,
    // volume control
    .volume_set_db = plug_volume_set_db,
    .volume_get_db = volume_get_db,
    .volume_set_amp = plug_volume_set_amp,
    .volume_get_amp = volume_get_amp,
    .volume_get_min_db = volume_get_min_db,
    // junk reading
    .junk_id3v1_read = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_id3v1_read,
    .junk_id3v1_find = junk_id3v1_find,
    .junk_id3v1_write = (int (*) (FILE *, DB_playItem_t *))junk_id3v1_write,
    .junk_id3v2_find = junk_id3v2_find,
    .junk_id3v2_read = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_id3v2_read,
    .junk_id3v2_read_full = (int (*)(DB_playItem_t *, DB_id3v2_tag_t *tag, DB_FILE *fp))junk_id3v2_read_full,
    .junk_id3v2_convert_24_to_23 = junk_id3v2_convert_24_to_23,
    .junk_id3v2_convert_23_to_24 = junk_id3v2_convert_23_to_24,
    .junk_id3v2_convert_22_to_24 = junk_id3v2_convert_22_to_24,
    .junk_id3v2_free = junk_id3v2_free,
    .junk_id3v2_write = junk_id3v2_write,
    .junk_id3v2_remove_frames = junk_id3v2_remove_frames,
    .junk_id3v2_add_text_frame = junk_id3v2_add_text_frame,
    .junk_apev2_read = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_apev2_read,
    .junk_apev2_read_mem = (int (*) (DB_playItem_t *it, char *mem, int size))junk_apev2_read_mem,
    .junk_apev2_read_full = (int (*) (DB_playItem_t *it, DB_apev2_tag_t *tag_store, DB_FILE *fp))junk_apev2_read_full,
    .junk_apev2_read_full_mem = (int (*) (DB_playItem_t *it, DB_apev2_tag_t *tag_store, char *mem, int memsize))junk_apev2_read_full_mem,
    .junk_apev2_find = junk_apev2_find,
    .junk_apev2_remove_frames = junk_apev2_remove_frames,
    .junk_apev2_add_text_frame = junk_apev2_add_text_frame,
    .junk_apev2_free = junk_apev2_free,
    .junk_apev2_write = junk_apev2_write,
    .junk_get_leading_size = junk_get_leading_size,
    .junk_get_leading_size_stdio = junk_get_leading_size_stdio,
    .junk_detect_charset = junk_detect_charset,
    .junk_recode = junk_recode,
    .junk_iconv = junk_iconv,
    .junk_rewrite_tags = (int (*) (DB_playItem_t *it, uint32_t flags, int id3v2_version, const char *id3v1_encoding))junk_rewrite_tags,
    // vfs
    .fopen = vfs_fopen,
    .fclose = vfs_fclose,
    .fread = vfs_fread,
    .fseek = vfs_fseek,
    .ftell = vfs_ftell,
    .rewind = vfs_rewind,
    .fgetlength = vfs_fgetlength,
    .fget_content_type = vfs_get_content_type,
    .fset_track = (void (*) (DB_FILE *stream, DB_playItem_t *it))vfs_set_track,
    .fabort = vfs_fabort,
    // message passing
    .sendmessage = messagepump_push,
    // configuration access
    .conf_get_str = conf_get_str,
    .conf_get_float = conf_get_float,
    .conf_get_int = conf_get_int,
    .conf_get_int64 = conf_get_int64,
    .conf_set_str = conf_set_str,
    .conf_set_int = conf_set_int,
    .conf_set_int64 = conf_set_int64,
    .conf_set_float = conf_set_float,
    .conf_find = conf_find,
    .conf_remove_items = conf_remove_items,
    .conf_save = conf_save,
    // plugin communication
    .plug_get_decoder_list = plug_get_decoder_list,
    .plug_get_vfs_list = plug_get_vfs_list,
    .plug_get_output_list = plug_get_output_list,
    .plug_get_dsp_list = plug_get_dsp_list,
    .plug_get_playlist_list = plug_get_playlist_list,
    .plug_get_list = plug_get_list,
    .plug_get_decoder_id = plug_get_decoder_id,
    .plug_remove_decoder_id = plug_remove_decoder_id,
    .plug_get_for_id = plug_get_for_id,
    // plugin events
    .plug_trigger_event_trackchange = (void (*) (DB_playItem_t *from, DB_playItem_t *to))plug_trigger_event_trackchange,
    .plug_trigger_event_trackinfochanged = (void (*) (DB_playItem_t *track))plug_trigger_event_trackinfochanged,
    .plug_trigger_event_playlistchanged = plug_trigger_event_playlistchanged,
    // misc utilities
    .is_local_file = plug_is_local_file,
    // pcm utilities
    .pcm_convert = pcm_convert,
    // dsp preset management
    .dsp_preset_load = dsp_preset_load,
    .dsp_preset_save = dsp_preset_save,
    .dsp_preset_free = dsp_preset_free
};

DB_functions_t *deadbeef = &deadbeef_api;

const char *
plug_get_config_dir (void) {
    return dbconfdir;
}

const char *
plug_get_prefix (void) {
    return dbinstalldir;
}

const char *
plug_get_doc_dir (void) {
    return dbdocdir;
}

const char *
plug_get_plugin_dir (void) {
    return dbplugindir;
}

const char *
plug_get_pixmap_dir (void) {
    return dbpixmapdir;
}

void
plug_volume_set_db (float db) {
    volume_set_db (db);
    plug_trigger_event_volumechanged ();
}

void
plug_volume_set_amp (float amp) {
    volume_set_amp (amp);
    plug_trigger_event_volumechanged ();
}

#define MAX_PLUGINS 100
DB_plugin_t *g_plugins[MAX_PLUGINS+1];

DB_decoder_t *g_decoder_plugins[MAX_DECODER_PLUGINS+1];

#define MAX_VFS_PLUGINS 10
DB_vfs_t *g_vfs_plugins[MAX_VFS_PLUGINS+1];

#define MAX_DSP_PLUGINS 10
DB_dsp_t *g_dsp_plugins[MAX_DSP_PLUGINS+1];

#define MAX_OUTPUT_PLUGINS 10
DB_output_t *g_output_plugins[MAX_OUTPUT_PLUGINS+1];
DB_output_t *output_plugin = NULL;

#define MAX_PLAYLIST_PLUGINS 10
DB_playlist_t *g_playlist_plugins[MAX_PLAYLIST_PLUGINS+1];

void
plug_md5 (uint8_t sig[16], const char *in, int len) {
    md5_state_t st;
    md5_init (&st);
    md5_append (&st, in, len);
    md5_finish (&st, sig);
}

void
plug_md5_to_str (char *str, const uint8_t sig[16]) {
    int i = 0;
    static const char hex[] = "0123456789abcdef";
    for (i = 0; i < 16; i++) {
        *str++ = hex[(sig[i]&0xf0)>>4];
        *str++ = hex[sig[i]&0xf];
    }
    *str = 0;
}

// event handlers
typedef struct {
    DB_plugin_t *plugin;
    DB_callback_t callback;
    uintptr_t data;
} evhandler_t;
#define MAX_HANDLERS 100
static evhandler_t handlers[DB_EV_MAX][MAX_HANDLERS];

// plugin control structures
typedef struct plugin_s {
    void *handle;
    DB_plugin_t *plugin;
    struct plugin_s *next;
} plugin_t;

plugin_t *plugins;
plugin_t *plugins_tail;

void
plug_ev_subscribe (DB_plugin_t *plugin, int ev, DB_callback_t callback, uintptr_t data) {
    assert (ev < DB_EV_MAX && ev >= 0);
    int i;
    mutex_lock (mutex);
    for (i = 0; i < MAX_HANDLERS; i++) {
        if (!handlers[ev][i].plugin) {
            handlers[ev][i].plugin = plugin;
            handlers[ev][i].callback = callback;
            handlers[ev][i].data = data;
            break;
        }
    }
    mutex_unlock (mutex);
    if (i == MAX_HANDLERS) {
        trace ("failed to subscribe plugin %s to event %d (too many event handlers)\n", plugin->name, ev);
    }
}

void
plug_ev_unsubscribe (DB_plugin_t *plugin, int ev, DB_callback_t callback, uintptr_t data) {
    assert (ev < DB_EV_MAX && ev >= 0);
    mutex_lock (mutex);
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handlers[ev][i].plugin == plugin) {
            handlers[ev][i].plugin = NULL;
            handlers[ev][i].callback = NULL;
            handlers[ev][i].data = 0;
            break;
        }
    }
    mutex_unlock (mutex);
}

float
plug_playback_get_pos (void) {
    playItem_t *trk = streamer_get_playing_track ();
    if (!trk || trk->_duration <= 0) {
        if (trk) {
            pl_item_unref (trk);
        }
        return 0;
    }
    if (trk) {
        pl_item_unref (trk);
    }
    return streamer_get_playpos () * 100 / trk->_duration;
}

void
plug_playback_set_pos (float pos) {
    playItem_t *trk = streamer_get_playing_track ();
    if (!trk || trk->_duration <= 0) {
        if (trk) {
            pl_item_unref (trk);
        }
        return;
    }
    float t = pos * trk->_duration / 100.f;
    if (trk) {
        pl_item_unref (trk);
    }
    streamer_set_seek (t);
}

/////// non-api functions (plugin support)
void
plug_event_call (DB_event_t *ev) {
    if (!mutex) {
        trace ("plug: event passed before plugin initialization\n");
    }
    ev->time = time (NULL);
//    printf ("plug_event_call enter %d\n", ev->event);
    mutex_lock (mutex);

    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handlers[ev->event][i].plugin) {
            handlers[ev->event][i].callback (ev, handlers[ev->event][i].data);
        }
    }
//    if (ev->event == DB_EV_PLAYLISTSWITCH) {
//        printf ("DB_EV_PLAYLISTSWITCH %d %d\n", plt_get_curr (), conf_get_int ("playlist.current", 0));
//        pl_save_current ();
//    }

    mutex_unlock (mutex);
//    printf ("plug_event_call leave %d\n", ev->event);
}

void
plug_trigger_event (int ev, uintptr_t param) {
    DB_event_t *event;
    switch (ev) {
    case DB_EV_SONGSTARTED:
    case DB_EV_SONGFINISHED:
        {
        DB_event_track_t *pev = alloca (sizeof (DB_event_track_t));
        playItem_t *pltrack = streamer_get_playing_track ();
        pev->track = DB_PLAYITEM (pltrack);
        if (pltrack) {
            pl_item_unref (pltrack);
        }
        event = DB_EVENT (pev);
        }
        break;
    default:
        event = alloca (sizeof (DB_event_t));
    }
    event->event = ev;
    plug_event_call (event);
}

void
plug_trigger_event_trackchange (playItem_t *from, playItem_t *to) {
    DB_event_trackchange_t event;
    event.ev.event = DB_EV_SONGCHANGED;
    //printf ("plug_trigger_event_trackchange %p %d %p %d\n", from, from ? from->_refc : -1, to, to ? to->_refc : -1);
    event.from = (DB_playItem_t *)from;
    event.to = (DB_playItem_t *)to;
    plug_event_call (DB_EVENT (&event));
}
void
plug_trigger_event_trackinfochanged (playItem_t *track) {
    DB_event_track_t event;
    event.ev.event = DB_EV_TRACKINFOCHANGED;
    event.track = DB_PLAYITEM (track);
    //printf ("\033[0;31mtrackinfochanged %p(%s), playing: %p, streaming: %p\033[37;0m\n", track, track->fname, streamer_get_playing_track (), streamer_get_streaming_track ());
    plug_event_call (DB_EVENT (&event));
}

void
plug_trigger_event_paused (int paused) {
    DB_event_state_t event;
    event.ev.event = DB_EV_PAUSED;
    event.state = paused;
    plug_event_call (DB_EVENT (&event));
}

void
plug_trigger_event_playlistchanged (void) {
    DB_event_t event;
    event.event = DB_EV_PLAYLISTCHANGED;
    plug_event_call (DB_EVENT (&event));
}

void
plug_trigger_event_volumechanged (void) {
    DB_event_t event;
    event.event = DB_EV_VOLUMECHANGED;
    plug_event_call (DB_EVENT (&event));
}

int
plug_init_plugin (DB_plugin_t* (*loadfunc)(DB_functions_t *), void *handle) {
    DB_plugin_t *plugin_api = loadfunc (&deadbeef_api);
    if (!plugin_api) {
        return -1;
    }
#if !DISABLE_VERSIONCHECK
    if (plugin_api->api_vmajor != 0 || plugin_api->api_vminor != 0) {
        // version check enabled
        if (plugin_api->api_vmajor != DB_API_VERSION_MAJOR || plugin_api->api_vminor != DB_API_VERSION_MINOR) {
            trace ("\033[0;31mWARNING: plugin \"%s\" wants API v%d.%d (got %d.%d), will not be loaded\033[0;m\n", plugin_api->name, plugin_api->api_vmajor, plugin_api->api_vminor, DB_API_VERSION_MAJOR, DB_API_VERSION_MINOR);
            return -1;
        }
    }
    else {
            trace ("\033[0;31mWARNING: plugin \"%s\" has disabled version check. do not distribute!\033[0;m\n", plugin_api->name);
    }
#endif
    plugin_t *plug = malloc (sizeof (plugin_t));
    memset (plug, 0, sizeof (plugin_t));
    plug->plugin = plugin_api;
    plug->handle = handle;
    if (plugins_tail) {
        plugins_tail->next = plug;
        plugins_tail = plug;
    }
    else {
        plugins = plugins_tail = plug;
    }
    return 0;
}

static int dirent_alphasort (const struct dirent **a, const struct dirent **b) {
    return strcmp ((*a)->d_name, (*b)->d_name);
}

void
plug_remove_plugin (void *p) {
    int i;
    for (i = 0; g_plugins[i]; i++) {
        if (g_plugins[i] == p) {
            memmove (&g_plugins[i], &g_plugins[i+1], (MAX_PLUGINS+1-i-1) * sizeof (void*));
        }
    }
    for (i = 0; g_decoder_plugins[i]; i++) {
        if (g_decoder_plugins[i] == p) {
            memmove (&g_decoder_plugins[i], &g_decoder_plugins[i+1], (MAX_DECODER_PLUGINS+1-i-1) * sizeof (void*));
        }
    }
    for (i = 0; g_vfs_plugins[i]; i++) {
        if (g_vfs_plugins[i] == p) {
            memmove (&g_vfs_plugins[i], &g_vfs_plugins[i+1], (MAX_VFS_PLUGINS+1-i-1) * sizeof (void*));
        }
    }
    for (i = 0; g_dsp_plugins[i]; i++) {
        if (g_dsp_plugins[i] == p) {
            memmove (&g_dsp_plugins[i], &g_dsp_plugins[i+1], (MAX_DSP_PLUGINS+1-i-1) * sizeof (void*));
        }
    }
    for (i = 0; g_output_plugins[i]; i++) {
        if (g_output_plugins[i] == p) {
            memmove (&g_output_plugins[i], &g_output_plugins[i+1], (MAX_OUTPUT_PLUGINS+1-i-1) * sizeof (void*));
        }
    }
    for (i = 0; g_playlist_plugins[i]; i++) {
        if (g_playlist_plugins[i] == p) {
            memmove (&g_playlist_plugins[i], &g_playlist_plugins[i+1], (MAX_PLAYLIST_PLUGINS+1-i-1) * sizeof (void*));
        }
    }
}

int
load_plugin_dir (const char *plugdir) {
    int n = 0;
    const char *conf_blacklist_plugins = conf_get_str ("blacklist_plugins", "");
    trace ("loading plugins from %s\n", plugdir);
    struct dirent **namelist = NULL;
    n = scandir (plugdir, &namelist, NULL, dirent_alphasort);
    if (n < 0)
    {
        if (namelist) {
            free (namelist);
        }
        return 0;
    }
    else
    {
        trace ("plug_load_all: scandir found %d files\n", n);
        int i;
        for (i = 0; i < n; i++)
        {
            // skip hidden files and fallback plugins
            while (namelist[i]->d_name[0] != '.'
#ifndef ANDROID
                    && !strstr (namelist[i]->d_name, ".fallback.")
#else
                    && !strstr (namelist[i]->d_name, "libdeadbeef")
#endif
                  )
            {
                int l = strlen (namelist[i]->d_name);
                if (l < 3) {
                    break;
                }
                if (strcasecmp (&namelist[i]->d_name[l-3], ".so")) {
                    break;
                }
                char d_name[256];
                memcpy (d_name, namelist[i]->d_name, l+1);
#ifndef ANDROID
                // no blacklisted
                const uint8_t *p = conf_blacklist_plugins;
                while (*p) {
                    const uint8_t *e = p;
                    while (*e && *e > 0x20) {
                        e++;
                    }
                    if (l-3 == e-p) {
                        if (!strncmp (p, d_name, e-p)) {
                            p = NULL;
                            break;
                        }
                    }
                    p = e;
                    while (*p && *p <= 0x20) {
                        p++;
                    }
                }
                if (!p) {
                    trace ("plugin %s is blacklisted in config file\n", d_name);
                    break;
                }
#endif
                char fullname[PATH_MAX];
                snprintf (fullname, PATH_MAX, "%s/%s", plugdir, d_name);
                trace ("loading plugin %s\n", d_name);
                void *handle = dlopen (fullname, RTLD_NOW);
                if (!handle) {
                    trace ("dlopen error: %s\n", dlerror ());
#ifdef ANDROID
                    break;
#else
                    strcpy (fullname + strlen(fullname) - 3, ".fallback.so");
                    trace ("trying %s...\n", fullname);
                    handle = dlopen (fullname, RTLD_NOW);
                    if (!handle) {
                        trace ("dlopen error: %s\n", dlerror ());
                        break;
                    }
                    else {
                        fprintf (stderr, "successfully started fallback plugin %s\n", fullname);
                    }
#endif
                }
                d_name[l-3] = 0;
                strcat (d_name, "_load");
#ifndef ANDROID
                DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name);
#else
                DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name+3);
#endif
                if (!plug_load) {
                    trace ("dlsym error: %s\n", dlerror ());
                    dlclose (handle);
                    break;
                }
                if (plug_init_plugin (plug_load, handle) < 0) {
                    d_name[l-3] = 0;
                    trace ("plugin %s is incompatible with current version of deadbeef, please upgrade the plugin\n", d_name);
                    dlclose (handle);
                    break;
                }
                break;
            }
            free (namelist[i]);
        }
        free (namelist);
    }
    return 0;
}

int
plug_load_all (void) {
#if DISABLE_VERSIONCHECK
    trace ("\033[0;31mDISABLE_VERSIONCHECK=1! do not distribute!\033[0;m\n");
#endif

    trace ("plug: mutex_create\n");
    mutex = mutex_create ();
    const char *dirname = deadbeef->get_plugin_dir ();

#ifndef ANDROID
    char *xdg_local_home = getenv ("XDG_LOCAL_HOME");
    char xdg_plugin_dir[1024];

    if (xdg_local_home) {
        strcpy (xdg_plugin_dir, xdg_local_home);
    } else {
        char *homedir = getenv ("HOME");

        if (!homedir) {
            trace ("plug_load_all: warning: unable to find home directory\n");
            xdg_plugin_dir[0] = 0;
        }
        else {
            int written = snprintf (xdg_plugin_dir, sizeof (xdg_plugin_dir), "%s/.local/lib/deadbeef", homedir);
            if (written > sizeof (xdg_plugin_dir)) {
                trace ("warning: XDG_LOCAL_HOME value is too long: %s. Ignoring.", xdg_local_home);
                xdg_plugin_dir[0] = 0;
            }
        }
    }

    const char *plugins_dirs[] = { dirname, xdg_plugin_dir, NULL };
#else
    const char *plugins_dirs[] = { dirname, NULL };
#endif

    int k = 0;

    while (plugins_dirs[k]) {
        const char *plugdir = plugins_dirs[k++];
        if (!(*plugdir)) {
            continue;
        }
        load_plugin_dir (plugdir);
    }

#ifdef ANDROID
    char plugin_path[1000];
    strncpy (plugin_path, conf_get_str ("android.plugin_path", ""), sizeof (plugin_path)-1);
    plugin_path[sizeof(plugin_path)-1] = 0;
    char *p = plugin_path;
    while (*p) {
        while (*p == ':') {
            p++;
        }
        if (!(*p)) {
            break;
        }
        char *e = strchr (p, ':');
        if (e) {
            *e = 0;
        }

        char path[PATH_MAX];
        snprintf (path, sizeof (path), "/data/data/%s/lib", p);
        load_plugin_dir (path);
        if (!e) {
            break;
        }
        p = e+1;
    }
#endif

// load all compiled-in modules
#define PLUG(n) extern DB_plugin_t * n##_load (DB_functions_t *api);
#include "moduleconf.h"
#undef PLUG
#define PLUG(n) plug_init_plugin (n##_load, NULL);
#include "moduleconf.h"
#undef PLUG
#ifdef ANDROID
#define PLUG(n) extern DB_plugin_t * n##_load (DB_functions_t *api);
#include "moduleconf-android.h"
#undef PLUG
#define PLUG(n) plug_init_plugin (n##_load, NULL);
#include "moduleconf-android.h"
#undef PLUG
#endif

    plugin_t *plug;
    // categorize plugins
    int numplugins = 0;
    int numdecoders = 0;
    int numvfs = 0;
    int numoutput = 0;
    int numdsp = 0;
    int numplaylist = 0;
    for (plug = plugins; plug; plug = plug->next) {
        g_plugins[numplugins++] = plug->plugin;
        if (plug->plugin->type == DB_PLUGIN_DECODER) {
//            trace ("found decoder plugin %s\n", plug->plugin->name);
            if (numdecoders >= MAX_DECODER_PLUGINS) {
                break;
            }
            g_decoder_plugins[numdecoders++] = (DB_decoder_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_VFS) {
//            trace ("found vfs plugin %s\n", plug->plugin->name);
            if (numvfs >= MAX_VFS_PLUGINS) {
                break;
            }
            g_vfs_plugins[numvfs++] = (DB_vfs_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_OUTPUT) {
//            trace ("found output plugin %s\n", plug->plugin->name);
            if (numoutput >= MAX_OUTPUT_PLUGINS) {
                break;
            }
            g_output_plugins[numoutput++] = (DB_output_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_DSP) {
//            trace ("found dsp plugin %s\n", plug->plugin->name);
            if (numdsp >= MAX_DSP_PLUGINS) {
                break;
            }
            g_dsp_plugins[numdsp++] = (DB_dsp_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_PLAYLIST) {
            if (numplaylist >= MAX_PLAYLIST_PLUGINS) {
                break;
            }
            g_playlist_plugins[numplaylist++] = (DB_playlist_t *)plug->plugin;
        }
    }
    // start plugins
    plugin_t *prev = NULL;
    for (plug = plugins; plug;) {
        if (plug->plugin->start) {
            if (plug->plugin->start () < 0) {
                fprintf (stderr, "plugin %s failed to start, deactivated.\n", plug->plugin->name);
                if (plug->plugin->stop) {
                    plug->plugin->stop ();
                }
                if (plug->handle) {
                    dlclose (plug->handle);
                }
                plug_remove_plugin (plug->plugin);
                if (prev) {
                    prev->next = plug->next;
                }
                else {
                    plugins = plug->next;
                }
                plugin_t *next = plug->next;
                free (plug);
                plug = next;
                continue;
            }
        }
        prev = plug;
        plug = plug->next;
    }

//    trace ("numplugins: %d, numdecoders: %d, numvfs: %d\n", numplugins, numdecoders, numvfs);
    g_plugins[numplugins] = NULL;
    g_decoder_plugins[numdecoders] = NULL;
    g_vfs_plugins[numvfs] = NULL;
    g_output_plugins[numoutput] = NULL;
    g_dsp_plugins[numdsp] = NULL;
    g_playlist_plugins[numplaylist] = NULL;

    // select output plugin
    if (plug_select_output () < 0) {
        trace ("failed to find output plugin!\n");
        return -1;
    }
    return 0;
}

void
plug_connect_all (void) {
    plugin_t *plug;
    plugin_t *prev = NULL;
    for (plug = plugins; plug;) {
        if (plug->plugin->connect) {
            if (plug->plugin->connect () < 0) {
                fprintf (stderr, "plugin %s failed to connect to dependencies, deactivated.\n", plug->plugin->name);

                if (plug->plugin->stop) {
                    plug->plugin->stop ();
                }
                if (plug->handle) {
                    dlclose (plug->handle);
                }
                plug_remove_plugin (plug->plugin);

                if (prev) {
                    prev->next = plug->next;
                }
                else {
                    plugins = plug->next;
                }
                plugin_t *next = plug->next;
                free (plug);
                plug = next;
                continue;
            }
        }
        prev = plug;
        plug = plug->next;
    }

}

void
plug_unload_all (void) {
    trace ("plug_unload_all\n");
    plugin_t *p;
    for (p = plugins; p; p = p->next) {
        if (p->plugin->stop) {
            trace ("stopping %s...\n", p->plugin->name);
            fflush (stderr);
            p->plugin->stop ();
        }
    }
    trace ("stopped all plugins\n");
    while (plugins) {
        plugin_t *next = plugins->next;
#ifndef ANDROID
        if (plugins->handle) {
            dlclose (plugins->handle);
        }
#endif
        free (plugins);
        plugins = next;
    }
    plugins_tail = NULL;
    trace ("all plugins had been unloaded\n");
}

void
plug_cleanup (void) {
    if (mutex) {
        mutex_free (mutex);
        mutex = 0;
    }
    plug_free_decoder_ids ();
}

struct DB_decoder_s **
plug_get_decoder_list (void) {
    return g_decoder_plugins;
}

struct DB_vfs_s **
plug_get_vfs_list (void) {
    return g_vfs_plugins;
}

struct DB_output_s **
plug_get_output_list (void) {
    return g_output_plugins;
}

struct DB_dsp_s **
plug_get_dsp_list (void) {
    return g_dsp_plugins;
}

struct DB_playlist_s **
plug_get_playlist_list (void) {
    return g_playlist_plugins;
}

struct DB_plugin_s **
plug_get_list (void) {
    return g_plugins;
}

DB_output_t *
plug_get_output (void) {
    return output_plugin;
}

int
plug_select_output (void) {
#ifdef ANDROID
    return 0;
#else
    const char *outplugname = conf_get_str ("output_plugin", _("ALSA output plugin"));
    for (int i = 0; g_output_plugins[i]; i++) {
        DB_output_t *p = g_output_plugins[i];
        if (!strcmp (p->plugin.name, outplugname)) {
            trace ("selected output plugin: %s\n", outplugname);
            output_plugin = p;
            break;
        }
    }
    if (!output_plugin) {
        output_plugin = g_output_plugins[0];
        if (output_plugin) {
            trace ("selected output plugin: %s\n", output_plugin->plugin.name);
            conf_set_str ("output_plugin", output_plugin->plugin.name);
        }
    }
    if (!output_plugin) {
        return -1;
    }
    plug_trigger_event (DB_EV_OUTPUTCHANGED, 0);
    return 0;
#endif
}

void
plug_reinit_sound (void) {
    DB_output_t *prev = plug_get_output ();
    int state = OUTPUT_STATE_STOPPED;

    if (prev) {
        state = prev->state ();
        prev->free ();
    }

    if (plug_select_output () < 0) {
        const char *outplugname = conf_get_str ("output_plugin", "ALSA output plugin");
        trace ("failed to select output plugin %s\nreverted to %s\n", outplugname, prev->plugin.name);
        output_plugin = prev;
    }
    DB_output_t *output = plug_get_output ();
    if (output->init () < 0) {
        streamer_reset (1);
        streamer_set_nextsong (-2, 0);
        return;
    }

    if (state != OUTPUT_STATE_PAUSED && state != OUTPUT_STATE_STOPPED) {
        if (output->play () < 0) {
            trace ("failed to reinit sound output\n");
            streamer_set_nextsong (-2, 0);
        }
    }
}

// list of all unique decoder ids used in current session
static char *decoder_ids[MAX_DECODER_PLUGINS];

const char *
plug_get_decoder_id (const char *id) {
    int i;
    char **lastnull = NULL;
    for (i = 0; i < MAX_DECODER_PLUGINS; i++) {
        if (decoder_ids[i] && !strcmp (id, decoder_ids[i])) {
            return decoder_ids[i];
        }
        else if (!lastnull && !decoder_ids[i]) {
            lastnull = &decoder_ids[i];
        }
    }
    if (!lastnull) {
        return NULL;
    }
    char *newid = strdup (id);
    *lastnull = newid;
    return newid;
}

void
plug_remove_decoder_id (const char *id) {
    int i;
    for (i = 0; i < MAX_DECODER_PLUGINS; i++) {
        if (decoder_ids[i] && !strcmp (decoder_ids[i], id)) {
            free (decoder_ids[i]);
            decoder_ids[i] = NULL;
        }
    }
}

void
plug_free_decoder_ids (void) {
    int i;
    for (i = 0; i < MAX_DECODER_PLUGINS; i++) {
        if (decoder_ids[i]) {
            free (decoder_ids[i]);
            decoder_ids[i] = NULL;
        }
    }
}

DB_decoder_t *
plug_get_decoder_for_id (const char *id) {
    DB_decoder_t **plugins = plug_get_decoder_list ();
    for (int c = 0; plugins[c]; c++) {
        if (!strcmp (id, plugins[c]->plugin.id)) {
            return plugins[c];
        }
    }
    return NULL;
}

DB_plugin_t *
plug_get_for_id (const char *id) {
    DB_plugin_t **plugins = plug_get_list ();
    for (int c = 0; plugins[c]; c++) {
        if (plugins[c]->id && !strcmp (id, plugins[c]->id)) {
            return plugins[c];
        }
    }
    return NULL;
}

int
plug_is_local_file (const char *fname) {
    if (!strncasecmp (fname, "file://", 7)) {
        return 1;
    }

    for (; *fname; fname++) {
        if (!strncmp (fname, "://", 3)) {
            return 0;
        }
    }

    return 1;
}

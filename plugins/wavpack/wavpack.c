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

#include <string.h>
#ifdef TINYWV
#include <wavpack.h>
#else
#include <wavpack/wavpack.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    DB_FILE *file;
#ifndef TINYWV
    DB_FILE *c_file;
#endif
    WavpackContext *ctx;
    int startsample;
    int endsample;
} wvctx_t;

#ifdef TINYWV
int32_t wv_read_stream(void *buf, int32_t sz, void *file_handle) {
    return deadbeef->fread (buf, 1, sz, (DB_FILE *)file_handle);
}
#else
int32_t wv_read_bytes(void *id, void *data, int32_t bcount) {
//    trace ("wv_read_bytes\n");
    return deadbeef->fread (data, 1, bcount, id);
}

uint32_t wv_get_pos(void *id) {
//    trace ("wv_get_pos\n");
    return deadbeef->ftell (id);
}

int wv_set_pos_abs(void *id, uint32_t pos) {
//    trace ("wv_set_pos_abs\n");
    return deadbeef->fseek (id, pos, SEEK_SET);
}
int wv_set_pos_rel(void *id, int32_t delta, int mode) {
//    trace ("wv_set_pos_rel\n");
    return deadbeef->fseek (id, delta, SEEK_CUR);
}
int wv_push_back_byte(void *id, int c) {
//    trace ("wv_push_back_byte\n");
    deadbeef->fseek (id, -1, SEEK_CUR);
    return deadbeef->ftell (id);
}
uint32_t wv_get_length(void *id) {
//    trace ("wv_get_length\n");
    size_t pos = deadbeef->ftell (id);
    deadbeef->fseek (id, 0, SEEK_END);
    size_t sz = deadbeef->ftell (id);
    deadbeef->fseek (id, pos, SEEK_SET);
    return sz;
}
int wv_can_seek(void *id) {
//    trace ("wv_can_seek\n");
    return 1;
}

int32_t wv_write_bytes (void *id, void *data, int32_t bcount) {
    return 0;
}

static WavpackStreamReader wsr = {
    .read_bytes = wv_read_bytes,
    .get_pos = wv_get_pos,
    .set_pos_abs = wv_set_pos_abs,
    .set_pos_rel = wv_set_pos_rel,
    .push_back_byte = wv_push_back_byte,
    .get_length = wv_get_length,
    .can_seek = wv_can_seek,
    .write_bytes = wv_write_bytes
};
#endif

static DB_fileinfo_t *
wv_open (void) {
    DB_fileinfo_t *_info = malloc (sizeof (wvctx_t));
    memset (_info, 0, sizeof (wvctx_t));
    return _info;
}

static int
wv_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    wvctx_t *info = (wvctx_t *)_info;
    info->file = deadbeef->fopen (it->fname);
    if (!info->file) {
        return -1;
    }

#ifndef TINYWV
    char *c_fname = alloca (strlen (it->fname) + 2);
    if (c_fname) {
        strcpy (c_fname, it->fname);
        strcat (c_fname, "c");
        info->c_file = deadbeef->fopen (c_fname);
    }
    else {
        fprintf (stderr, "wavpack warning: failed to alloc memory for correction file name\n");
    }
#endif

    char error[80];
#ifdef TINYWV
    info->ctx = WavpackOpenFileInput (wv_read_stream, info->file, error);
#else
    info->ctx = WavpackOpenFileInputEx (&wsr, info->file, info->c_file, error, OPEN_2CH_MAX | OPEN_NORMALIZE, 0);
#endif
    if (!info->ctx) {
        fprintf (stderr, "wavpack error: %s\n", error);
        return -1;
    }
    _info->plugin = &plugin;
    _info->bps = WavpackGetBytesPerSample (info->ctx) * 8;
    _info->channels = WavpackGetReducedChannels (info->ctx);
    _info->samplerate = WavpackGetSampleRate (info->ctx);
    _info->readpos = 0;
    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        if (plugin.seek_sample (_info, 0) < 0) {
            return -1;
        }
    }
    else {
        info->startsample = 0;
        info->endsample = WavpackGetNumSamples (info->ctx)-1;
    }
    return 0;
}

static void
wv_free (DB_fileinfo_t *_info) {
    if (_info) {
        wvctx_t *info = (wvctx_t *)_info;
        if (info->file) {
            deadbeef->fclose (info->file);
            info->file = NULL;
        }
#ifndef TINYWV
        if (info->c_file) {
            deadbeef->fclose (info->c_file);
            info->c_file = NULL;
        }
#endif
        if (info->ctx) {
            WavpackCloseFile (info->ctx);
            info->ctx = NULL;
        }
        free (_info);
    }
}

static int
wv_read_int16 (DB_fileinfo_t *_info, char *bytes, int size) {
    wvctx_t *info = (wvctx_t *)_info;
    int currentsample = WavpackGetSampleIndex (info->ctx);
    if (size / (2 * _info->channels) + currentsample > info->endsample) {
        size = (info->endsample - currentsample + 1) * 2 * _info->channels;
        trace ("wv: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int32_t buffer[size/2];
    int nchannels = WavpackGetReducedChannels (info->ctx);
    int n = WavpackUnpackSamples (info->ctx, buffer, size/(2*nchannels));
    size = n * 2 * nchannels;
    // convert to int16
    int32_t *p = buffer;
    n *= nchannels;

    if (WavpackGetMode (info->ctx) & MODE_FLOAT) {
        while (n > 0) {
            float val = *(float*)p;
            if (val >= 1.0)
                *((int16_t *)bytes) = 32767;
            else if (val <= -1.0)
                *((int16_t *)bytes) = -32768;
            else
                *((int16_t *)bytes) = floor (val * 32768.f);
            bytes += sizeof (int16_t);
            p++;
            n--;
        }
    }
    else {
        while (n > 0) {
            if (_info->bps >= 16) {
                *((int16_t *)bytes) = (int16_t)((*p) >> (_info->bps-16));
            }
            else {
                *((int16_t *)bytes) = (int16_t)((*p) << (16-_info->bps));
            }
            bytes += sizeof (int16_t);
            p++;
            n--;
        }
    }
    _info->readpos = (float)(WavpackGetSampleIndex (info->ctx)-info->startsample)/WavpackGetSampleRate (info->ctx);

#ifndef TINYWV
    deadbeef->streamer_set_bitrate (WavpackGetInstantBitrate (info->ctx) / 1000);
#endif

    return size;
}

static int
wv_read_float32 (DB_fileinfo_t *_info, char *bytes, int size) {
    wvctx_t *info = (wvctx_t *)_info;
    int currentsample = WavpackGetSampleIndex (info->ctx);
    if (size / (4 * _info->channels) + currentsample > info->endsample) {
        size = (info->endsample - currentsample + 1) * 4 * _info->channels;
        trace ("wv: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int32_t buffer[size/4];
    int nchannels = WavpackGetReducedChannels (info->ctx);
    int n = WavpackUnpackSamples (info->ctx, buffer, size/(4*nchannels));
    size = n * 4 * nchannels;
    // convert to float32
    n *= nchannels;

    if (WavpackGetMode (info->ctx) & MODE_FLOAT) {
        memcpy (bytes, buffer, size);
    }
    else {
        float mul = 1.f/ (1UL << (_info->bps-1));
        int32_t *p = buffer;
        while (n > 0) {
            *((float *)bytes) = (*p) * mul;
            bytes += sizeof (float);
            p++;
            n--;
        }
    }
    _info->readpos = (float)(WavpackGetSampleIndex (info->ctx)-info->startsample)/WavpackGetSampleRate (info->ctx);
#ifndef TINYWV
    deadbeef->streamer_set_bitrate (WavpackGetInstantBitrate (info->ctx) / 1000);
#endif
    return size;
}

static int
wv_seek_sample (DB_fileinfo_t *_info, int sample) {
#ifndef TINYWV
    wvctx_t *info = (wvctx_t *)_info;
    WavpackSeekSample (info->ctx, sample + info->startsample);
    _info->readpos = (float)(WavpackGetSampleIndex (info->ctx) - info->startsample) / WavpackGetSampleRate (info->ctx);
#endif
    return 0;
}

static int
wv_seek (DB_fileinfo_t *_info, float sec) {
    wvctx_t *info = (wvctx_t *)_info;
    return wv_seek_sample (_info, sec * WavpackGetSampleRate (info->ctx));
}

static DB_playItem_t *
wv_insert (DB_playItem_t *after, const char *fname) {

    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }
    char error[80];
#ifdef TINYWV
    WavpackContext *ctx = WavpackOpenFileInput (wv_read_stream, fp, error);
#else
    WavpackContext *ctx = WavpackOpenFileInputEx (&wsr, fp, NULL, error, 0, 0);
#endif
    if (!ctx) {
        fprintf (stderr, "wavpack error: %s\n", error);
        deadbeef->fclose (fp);
        return NULL;
    }
    int totalsamples = WavpackGetNumSamples (ctx);
    int samplerate = WavpackGetSampleRate (ctx);
    float duration = (float)totalsamples / samplerate;

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = "wv";
    deadbeef->pl_set_item_duration (it, duration);
    trace ("wv: totalsamples=%d, samplerate=%d, duration=%f\n", totalsamples, samplerate, duration);

#if 0
    int num = WavpackGetNumTagItems (ctx);
    trace ("num tag items: %d\n", num);

    for (int i = 0; i < num; i++) {
        char str[1024];
        WavpackGetTagItemIndexed (ctx, i, str, sizeof (str));
        trace ("tag item: %s\n", str);
    }

#endif
    int apeerr = deadbeef->junk_apev2_read (it, fp);
    if (!apeerr) {
        trace ("wv: ape tag found\n");
    }
    int v1err = deadbeef->junk_id3v1_read (it, fp);
    if (!v1err) {
        trace ("wv: id3v1 tag found\n");
    }

#if 0
    // embedded cue
    char *emb_cuesheet;
    int len = WavpackGetTagItem (ctx, "cuesheet", NULL, 0);
    if (len) {
        emb_cuesheet = malloc (len);
        if (emb_cuesheet) {
            WavpackGetTagItem (ctx, "Cuesheet", emb_cuesheet, len);
            trace ("got cuesheet\n%s\n", emb_cuesheet);
            DB_playItem_t *last = deadbeef->pl_insert_cue_from_buffer (after, it, emb_cuesheet, strlen (emb_cuesheet), totalsamples, samplerate);
            free (emb_cuesheet);
            if (last) {
                deadbeef->pl_item_unref (it);
                deadbeef->fclose (fp);
                WavpackCloseFile (ctx);
                return last;
            }
            trace ("pl_insert_cue_from_buffer failed!\n");
        }
    }
#endif

    // embedded cue
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    if (cuesheet) {
        trace ("found cuesheet: %s\n", cuesheet);
        DB_playItem_t *last = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), totalsamples, samplerate);
        if (last) {
            deadbeef->fclose (fp);
            WavpackCloseFile (ctx);
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (last);
            return last;
        }
    }
    // cue file on disc
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, it, totalsamples, samplerate);
    if (cue_after) {
        deadbeef->fclose (fp);
        WavpackCloseFile (ctx);
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue_after);
        return cue_after;
    }

    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);

    deadbeef->fclose (fp);
    WavpackCloseFile (ctx);
    return after;
}

int
wv_read_metadata (DB_playItem_t *it) {
    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        return -1;
    }
    int apeerr = deadbeef->junk_apev2_read (it, fp);
    if (!apeerr) {
        trace ("wv: ape tag found\n");
    }
    int v1err = deadbeef->junk_id3v1_read (it, fp);
    if (!v1err) {
        trace ("wv: id3v1 tag found\n");
    }
    deadbeef->fclose (fp);
    return 0;
}

int
wv_write_metadata (DB_playItem_t *it) {
    int strip_apev2 = deadbeef->conf_get_int ("wv.strip_apev2", 0);
    int strip_id3v1 = deadbeef->conf_get_int ("wv.strip_id3v1", 0);
    int write_apev2 = deadbeef->conf_get_int ("wv.write_apev2", 1);
    int write_id3v1 = deadbeef->conf_get_int ("wv.write_id3v1", 0);

    uint32_t junk_flags = 0;
    if (strip_id3v1) {
        junk_flags |= JUNK_STRIP_ID3V1;
    }
    if (strip_apev2) {
        junk_flags |= JUNK_STRIP_APEV2;
    }
    if (write_id3v1) {
        junk_flags |= JUNK_WRITE_ID3V1;
    }
    if (write_apev2) {
        junk_flags |= JUNK_WRITE_APEV2;
    }

    return deadbeef->junk_rewrite_tags (it, junk_flags, 0, NULL);
}

static const char *exts[] = { "wv", NULL };
static const char *filetypes[] = { "wv", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "wv",
    .plugin.name = "WavPack decoder",
    .plugin.descr = ".wv player using libwavpack",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = wv_open,
    .init = wv_init,
    .free = wv_free,
    .read_int16 = wv_read_int16,
    .read_float32 = wv_read_float32,
    .seek = wv_seek,
    .seek_sample = wv_seek_sample,
    .insert = wv_insert,
    .read_metadata = wv_read_metadata,
    .write_metadata = wv_write_metadata,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
wavpack_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include "ttadec.h"
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
DB_functions_t *deadbeef;

#define MAX_BSIZE (MAX_BPS>>3)

typedef struct {
    DB_fileinfo_t info;
    tta_info tta;
    int currentsample;
    int startsample;
    int endsample;
    char buffer[PCM_BUFFER_LENGTH * MAX_BSIZE * MAX_NCH];
    int remaining;
    int samples_to_skip;
} tta_info_t;

static DB_fileinfo_t *
tta_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (tta_info_t));
    tta_info_t *info = (tta_info_t *)_info;
    memset (info, 0, sizeof (tta_info_t));
    return _info;
}

static int
tta_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    tta_info_t *info = (tta_info_t *)_info;

    trace ("open_tta_file %s\n", it->fname);
    if (open_tta_file (it->fname, &info->tta, 0) != 0) {
        fprintf (stderr, "tta: failed to open %s\n", it->fname);
        return -1;
    }

    if (player_init (&info->tta) != 0) {
        fprintf (stderr, "tta: failed to init player for %s\n", it->fname);
        return -1;
    }

    _info->fmt.bps = info->tta.BPS;
    _info->fmt.channels = info->tta.NCH;
    _info->fmt.samplerate = info->tta.SAMPLERATE;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->readpos = 0;
    _info->plugin = &plugin;

    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = (info->tta.DATALENGTH)-1;
    }
    trace ("open_tta_file %s success!\n", it->fname);
    return 0;
}

static void
tta_free (DB_fileinfo_t *_info) {
    tta_info_t *info = (tta_info_t *)_info;
    if (info) {
        player_stop (&info->tta);
        close_tta_file (&info->tta);
        free (info);
    }
}

static int
tta_read (DB_fileinfo_t *_info, char *bytes, int size) {
    tta_info_t *info = (tta_info_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (info->currentsample + size / samplesize > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * samplesize;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;

    while (size > 0) {
        if (info->samples_to_skip > 0 && info->remaining > 0) {
            int skip = min (info->remaining, info->samples_to_skip);
            if (skip < info->remaining) {
                memmove (info->buffer, info->buffer + skip * samplesize, (info->remaining - skip) * samplesize);
            }
            info->remaining -= skip;
            info->samples_to_skip -= skip;
        }
        if (info->remaining > 0) {
            int n = size / samplesize;
            n = min (n, info->remaining);
            int nn = n;
            char *p = info->buffer;

            memcpy (bytes, p, n * samplesize);
            bytes += n * samplesize;
            size -= n * samplesize;
            p += n * samplesize;

            if (info->remaining > nn) {
                memmove (info->buffer, p, (info->remaining - nn) * samplesize);
            }
            info->remaining -= nn;
        }

        if (size > 0 && !info->remaining) {
            info->remaining = get_samples (&info->tta, info->buffer);
            if (info->remaining <= 0) {
                break;
            }
        }
    }
    info->currentsample += (initsize-size) / samplesize;
    return initsize-size;
}

static int
tta_seek_sample (DB_fileinfo_t *_info, int sample) {
    tta_info_t *info = (tta_info_t *)_info;

    info->samples_to_skip = set_position (&info->tta, sample + info->startsample);
    if (info->samples_to_skip < 0) {
        fprintf (stderr, "tta: seek failed\n");
        return -1;
    }

    info->currentsample = sample + info->startsample;
    info->remaining = 0;
    _info->readpos = sample / _info->fmt.samplerate;
    return 0;
}

static int
tta_seek (DB_fileinfo_t *_info, float time) {
    tta_info_t *info = (tta_info_t *)_info;
    return tta_seek_sample (_info, time * _info->fmt.samplerate);
}

static DB_playItem_t *
tta_insert (DB_playItem_t *after, const char *fname) {
    tta_info tta;
    if (open_tta_file (fname, &tta, 0) != 0) {
        fprintf (stderr, "tta: failed to open %s\n", fname);
        return NULL;
    }

//    if (tta.BPS != 16) {
//        fprintf (stderr, "tta: only 16 bit is supported yet, skipped %s\n", fname);
//        return NULL;
//    }

    int totalsamples = tta.DATALENGTH;
    double dur = tta.LENGTH;

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = "TTA";
    deadbeef->pl_set_item_duration (it, dur);

    close_tta_file (&tta);
    DB_FILE *fp = deadbeef->fopen (fname);
    if (fp) {
        /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
        /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
        deadbeef->fclose (fp);
    }

    // embedded cue
    deadbeef->pl_lock ();
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    DB_playItem_t *cue = NULL;
    if (cuesheet) {
        cue = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), totalsamples, tta.SAMPLERATE);
        if (cue) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            deadbeef->pl_unlock ();
            return cue;
        }
    }
    deadbeef->pl_unlock ();

    cue  = deadbeef->pl_insert_cue (after, it, totalsamples, tta.SAMPLERATE);
    if (cue) {
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue);
        return cue;
    }

    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);

    return after;
}

static int tta_read_metadata (DB_playItem_t *it) {
    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        return -1;
    }
    deadbeef->pl_delete_all_meta (it);
    /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
    /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->fclose (fp);
    return 0;
}

static int tta_write_metadata (DB_playItem_t *it) {
    // get options

    int strip_id3v2 = 0;
    int strip_id3v1 = 0;
    int write_id3v2 = 1;
    int write_id3v1 = 1;

    uint32_t junk_flags = 0;
    if (strip_id3v2) {
        junk_flags |= JUNK_STRIP_ID3V2;
    }
    if (strip_id3v1) {
        junk_flags |= JUNK_STRIP_ID3V1;
    }
    if (write_id3v2) {
        junk_flags |= JUNK_WRITE_ID3V2;
    }
    if (write_id3v1) {
        junk_flags |= JUNK_WRITE_ID3V1;
    }

    int id3v2_version = 4;
    const char *id3v1_encoding = deadbeef->conf_get_str ("mp3.id3v1_encoding", "iso8859-1");
    return deadbeef->junk_rewrite_tags (it, junk_flags, id3v2_version, id3v1_encoding);
}


static int
tta_start (void) {
    return 0;
}

static int
tta_stop (void) {
    return 0;
}

static const char * exts[] = { "tta", NULL };
static const char *filetypes[] = { "TTA", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "tta",
    .plugin.name = "tta decoder",
    .plugin.descr = "tta decoder based on TTA Hardware Players Library Version 1.2",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = tta_start,
    .plugin.stop = tta_stop,
    .open = tta_open,
    .init = tta_init,
    .free = tta_free,
    .read = tta_read,
    .seek = tta_seek,
    .seek_sample = tta_seek_sample,
    .insert = tta_insert,
    .read_metadata = tta_read_metadata,
    .write_metadata = tta_write_metadata,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
tta_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

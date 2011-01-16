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
#include <math.h>
#include "mpc/mpcdec.h"
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    mpc_streaminfo si;
    mpc_demux *demux;
//    mpc_decoder *mpcdec;
    mpc_reader reader;
    int currentsample;
    int startsample;
    int endsample;
    mpc_uint32_t vbr_update_acc;
    mpc_uint32_t vbr_update_bits;
    MPC_SAMPLE_FORMAT buffer[MPC_DECODER_BUFFER_LENGTH];
    int remaining;
} musepack_info_t;

mpc_int32_t musepack_vfs_read (mpc_reader *r, void *ptr, mpc_int32_t size) {
    return deadbeef->fread(ptr, 1, size, (DB_FILE *)r->data);
}

/// Seeks to byte position offset.
mpc_bool_t musepack_vfs_seek (mpc_reader *r, mpc_int32_t offset) {
    int res = deadbeef->fseek ((DB_FILE *)r->data, offset, SEEK_SET);
    if (res == 0) {
        return 1;
    }
    return 0;
}

/// Returns the current byte offset in the stream.
mpc_int32_t musepack_vfs_tell (mpc_reader *r) {
    return deadbeef->ftell ((DB_FILE *)r->data);
}

/// Returns the total length of the source stream, in bytes.
mpc_int32_t musepack_vfs_get_size (mpc_reader *r) {
    return deadbeef->fgetlength ((DB_FILE *)r->data);
}

/// True if the stream is a seekable stream.
mpc_bool_t musepack_vfs_canseek (mpc_reader *r) {
    return 1;
}

static DB_fileinfo_t *
musepack_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (musepack_info_t));
    musepack_info_t *info = (musepack_info_t *)_info;
    memset (info, 0, sizeof (musepack_info_t));
    return _info;
}

static int
musepack_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    musepack_info_t *info = (musepack_info_t *)_info;

    info->reader.read = musepack_vfs_read;
    info->reader.seek = musepack_vfs_seek;
    info->reader.tell = musepack_vfs_tell;
    info->reader.get_size = musepack_vfs_get_size;
    info->reader.canseek = musepack_vfs_canseek;

    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        return -1;
    }
    info->reader.data = fp;

    info->demux = mpc_demux_init (&info->reader);
    if (!info->demux) {
        fprintf (stderr, "mpc: mpc_demux_init failed\n");
        deadbeef->fclose (fp);
        info->reader.data = NULL;
        return -1;
    }
    mpc_demux_get_info (info->demux, &info->si);

//    info->mpcdec = mpc_decoder_init (&info->si);
//    if (!info->mpcdec) {
//        deadbeef->fclose ((DB_FILE *)info->reader.data);
//        info->reader.data = NULL;
//        return -1;
//    }
    info->vbr_update_acc = 0;
    info->vbr_update_bits = 0;
    info->remaining = 0;

    _info->fmt.is_float = 1;
    _info->fmt.bps = 32;
    _info->fmt.channels = info->si.channels;
    _info->fmt.samplerate = info->si.sample_freq;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    _info->readpos = 0;
    _info->plugin = &plugin;

    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = mpc_streaminfo_get_length_samples (&info->si)-1;
    }

    return 0;
}

static void
musepack_free (DB_fileinfo_t *_info) {
    musepack_info_t *info = (musepack_info_t *)_info;
    if (info) {
        if (info->demux) {
            mpc_demux_exit (info->demux);
            info->demux = NULL;
        }
        if (info->reader.data) {
            deadbeef->fclose ((DB_FILE *)info->reader.data);
            info->reader.data = NULL;
        }
        free (info);
    }
}

#if 0
static int
musepack_read (DB_fileinfo_t *_info, char *bytes, int size) {
    musepack_info_t *info = (musepack_info_t *)_info;

    int samplesize = _info->fmt.bps / 8 * _info->fmt.channels;
    if (info->currentsample + size / samplesize > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * samplesize;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;

    while (size > 0) {
        if (info->remaining > 0) {
            int n = size / samplesize;
            n = min (n, info->remaining);
            int nn = n;
            float *p = info->buffer;
            while (n > 0) {
                int sample = (int)(*p * 32767.0f);
                if (sample > 32767) {
                    sample = 32767;
                }
                else if (sample < -32768) {
                    sample = -32768;
                }
                *((int16_t *)bytes) = (int16_t)sample;
                bytes += 2;
                if (_info->fmt.channels == 2) {
                    sample = (int)(*(p+1) * 32767.0f);
                    if (sample > 32767) {
                        sample = 32767;
                    }
                    else if (sample < -32768) {
                        sample = -32768;
                    }
                    *((int16_t *)bytes) = (int16_t)sample;
                    bytes += 2;
                }
                n--;
                size -= samplesize;
                p += info->si.channels;
            }
            if (info->remaining > nn) {
                memmove (info->buffer, p, (info->remaining - nn) * sizeof (float) * _info->fmt.channels);
            }
            info->remaining -= nn;
        }

        if (size > 0 && !info->remaining) {
            mpc_frame_info frame;
            frame.buffer = info->buffer;
            mpc_status err = mpc_demux_decode (info->demux, &frame);
            if (err != 0 || frame.bits == -1) {
                break;
            }

            info->remaining = frame.samples;
        }
    }
    info->currentsample += (initsize-size) / samplesize;
    return initsize-size;
}
#endif

static int
musepack_read (DB_fileinfo_t *_info, char *bytes, int size) {
    musepack_info_t *info = (musepack_info_t *)_info;
    int samplesize = _info->fmt.bps / 8 * _info->fmt.channels;

    if (info->currentsample + size / samplesize > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * samplesize;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;

    while (size > 0) {
        if (info->remaining > 0) {
            int n = size / samplesize;
            n = min (n, info->remaining);

            memcpy (bytes, info->buffer, n * samplesize);

            size -= n * samplesize;
            bytes += n * samplesize;

            if (info->remaining > n) {
                memmove (info->buffer, ((char *)info->buffer) + n * samplesize, (info->remaining - n) * samplesize);
            }
            info->remaining -= n;
        }

        if (size > 0 && !info->remaining) {
            mpc_frame_info frame;
            frame.buffer = info->buffer;
            mpc_status err = mpc_demux_decode (info->demux, &frame);
            if (err != 0 || frame.bits == -1) {
                break;
            }

            info->remaining = frame.samples;
        }
    }
    info->currentsample += (initsize-size) / samplesize;
    return initsize-size;
}

static int
musepack_seek_sample (DB_fileinfo_t *_info, int sample) {
    musepack_info_t *info = (musepack_info_t *)_info;
    mpc_status err = mpc_demux_seek_sample (info->demux, sample + info->startsample);
    if (err != 0) {
        fprintf (stderr, "musepack: seek failed\n");
        return -1;
    }
    info->currentsample = sample + info->startsample;
    _info->readpos = (float)sample / _info->fmt.samplerate;
    info->remaining = 0;
    return 0;
}

static int
musepack_seek (DB_fileinfo_t *_info, float time) {
    musepack_info_t *info = (musepack_info_t *)_info;
    return musepack_seek_sample (_info, time * _info->fmt.samplerate);
}

static DB_playItem_t *
musepack_insert (DB_playItem_t *after, const char *fname) {
    trace ("mpc: inserting %s\n", fname);
    mpc_reader reader = {
        .read = musepack_vfs_read,
        .seek = musepack_vfs_seek,
        .tell = musepack_vfs_tell,
        .get_size = musepack_vfs_get_size,
        .canseek = musepack_vfs_canseek,
    };

    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("mpc: insert failed to open %s\n", fname);
        return NULL;
    }
    reader.data = fp;

    mpc_demux *demux = mpc_demux_init (&reader);
    if (!demux) {
        trace ("mpc: mpc_demux_init failed\n");
        deadbeef->fclose (fp);
        return NULL;
    }

    mpc_streaminfo si;
    //mpc_streaminfo_init (&si);
    mpc_demux_get_info (demux, &si);

    int totalsamples = mpc_streaminfo_get_length_samples (&si);
    double dur = mpc_streaminfo_get_length (&si);

    // replay gain
    float gain_title = 0.f;
    float gain_album = 0.f;
    float peak_title = 1.f;
    float peak_album = 1.f;
    if (si.gain_title != 0) {
        gain_title = 64.82-si.gain_title/256.0;
    }
    if (si.gain_album != 0) {
        gain_album = 64.82-si.gain_album/256.0;
    }
    if (si.peak_title != 0) {
        peak_title = pow (10, si.peak_title / (20.0 * 256.0)) / (1<<15);
    }
    if (si.peak_album != 0) {
        peak_album = pow (10, si.peak_album / (20.0 * 256.0)) / (1<<15);
    }

    // chapters
    int nchapters = mpc_demux_chap_nb (demux);
    DB_playItem_t *prev = NULL;
    DB_playItem_t *meta = NULL;
    if (nchapters > 1) {
        int i;
        for (i = 0; i < nchapters; i++) {
            const mpc_chap_info *ch = mpc_demux_chap (demux, i);
            DB_playItem_t *it = deadbeef->pl_item_alloc ();
            it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
            it->fname = strdup (fname);
            it->filetype = "MusePack";
            it->tracknum = i;
            it->startsample = ch->sample;
            it->endsample = totalsamples-1;
            float gain = gain_title, peak = peak_title;
            if (ch->gain != 0) {
                gain = 64.82-ch->gain/256.0;
            }
            if (ch->peak != 0) {
                peak = pow (10, ch->peak / (20.0 * 256.0)) / (1<<15);
            }
            it->replaygain_album_gain = gain_album;
            it->replaygain_album_peak = peak_album;
            it->replaygain_track_gain = gain_title;
            it->replaygain_track_peak = peak_title;
            deadbeef->pl_set_item_flags (it, DDB_IS_SUBTRACK);
            if (!prev) {
                meta = deadbeef->pl_item_alloc ();
                /*int apeerr = */deadbeef->junk_apev2_read (meta, fp);
            }
            else {
                prev->endsample = it->startsample-1;
                float dur = (prev->endsample - prev->startsample) / (float)si.sample_freq;
                deadbeef->pl_set_item_duration (prev, dur);
            }
            if (i == nchapters - 1) {
                float dur = (it->endsample - it->startsample) / (float)si.sample_freq;
                deadbeef->pl_set_item_duration (it, dur);
            }
            if (ch->tag_size > 0) {
                uint8_t *tag = ch->tag;
                deadbeef->junk_apev2_read_mem (it, ch->tag, ch->tag_size);
                if (meta) {
                    deadbeef->pl_items_copy_junk (meta, it, it);
                }
            }
            after = deadbeef->pl_insert_item (after, it);
            prev = it;
            deadbeef->pl_item_unref (it);
        }
        mpc_demux_exit (demux);
        demux = NULL;
        deadbeef->fclose (fp);
        if (meta) {
            deadbeef->pl_item_unref (meta);
        }
        return after;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = "MusePack";
    deadbeef->pl_set_item_duration (it, dur);

    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    it->replaygain_album_gain = gain_album;
    it->replaygain_album_peak = peak_album;
    it->replaygain_track_gain = gain_title;
    it->replaygain_track_peak = peak_title;

    deadbeef->fclose (fp);

    deadbeef->pl_lock ();

    // embedded cue
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    DB_playItem_t *cue = NULL;
    if (cuesheet) {
        cue = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), totalsamples, si.sample_freq);
        if (cue) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            deadbeef->pl_unlock ();
            mpc_demux_exit (demux);
            demux = NULL;
            return cue;
        }
    }
    deadbeef->pl_unlock ();

    cue  = deadbeef->pl_insert_cue (after, it, totalsamples, si.sample_freq);
    if (cue) {
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue);
        mpc_demux_exit (demux);
        demux = NULL;

        return cue;
    }

    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);

    mpc_demux_exit (demux);
    demux = NULL;

    return after;
}

static int musepack_read_metadata (DB_playItem_t *it) {
    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        return -1;
    }
    deadbeef->pl_delete_all_meta (it);
    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->fclose (fp);
    return 0;
}

static int musepack_write_metadata (DB_playItem_t *it) {
    // get options
    int strip_apev2 = deadbeef->conf_get_int ("ape.strip_apev2", 0);
    int write_apev2 = deadbeef->conf_get_int ("ape.write_apev2", 1);

    uint32_t junk_flags = 0;
    if (strip_apev2) {
        junk_flags |= JUNK_STRIP_APEV2;
    }
    if (write_apev2) {
        junk_flags |= JUNK_WRITE_APEV2;
    }

    return deadbeef->junk_rewrite_tags (it, junk_flags, 4, NULL);
}


static int
musepack_start (void) {
    return 0;
}

static int
musepack_stop (void) {
    return 0;
}

static const char * exts[] = { "mpc", "mpp", "mp+", NULL };
static const char *filetypes[] = { "MusePack", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "musepack",
    .plugin.name = "MusePack decoder",
    .plugin.descr = "Musepack decoder using libmppdec",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = musepack_start,
    .plugin.stop = musepack_stop,
    .open = musepack_open,
    .init = musepack_init,
    .free = musepack_free,
    .read = musepack_read,
    .seek = musepack_seek,
    .seek_sample = musepack_seek_sample,
    .insert = musepack_insert,
    .read_metadata = musepack_read_metadata,
    .write_metadata = musepack_write_metadata,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
musepack_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

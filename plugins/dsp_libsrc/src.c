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
#include <samplerate.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "conf.h"
#include "threading.h"
#include "deadbeef.h"
#include "src.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_functions_t *deadbeef;

#define SRC_BUFFER 16000
#define SRC_MAX_CHANNELS 8

static DB_dsp_t plugin;

typedef struct {
    ddb_dsp_context_t ctx;

    int channels;
    int quality;
    float samplerate;
    SRC_STATE *src;
    SRC_DATA srcdata;
    int remaining; // number of input samples in SRC buffer
    __attribute__((__aligned__(16))) char in_fbuffer[sizeof(float)*SRC_BUFFER*SRC_MAX_CHANNELS];
    unsigned quality_changed : 1;
    unsigned need_reset : 1;
} ddb_src_libsamplerate_t;

ddb_dsp_context_t*
ddb_src_open (void) {
    ddb_src_libsamplerate_t *src = malloc (sizeof (ddb_src_libsamplerate_t));
    DDB_INIT_DSP_CONTEXT (src,ddb_src_libsamplerate_t,&plugin);

    src->samplerate = 44100;
    src->quality = 2;
    src->channels = -1;
    return (ddb_dsp_context_t *)src;
}

void
ddb_src_close (ddb_dsp_context_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    if (src->src) {
        src_delete (src->src);
        src->src = NULL;
    }
    free (src);
}

void
ddb_src_reset (ddb_dsp_context_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    src->need_reset = 1;
}


void
ddb_src_set_ratio (ddb_dsp_context_t *_src, float ratio) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    if (src->srcdata.src_ratio != ratio) {
        src->srcdata.src_ratio = ratio;
        src_set_ratio (src->src, ratio);
    }
}

int
ddb_src_process (ddb_dsp_context_t *_src, float *samples, int nframes, int maxframes, ddb_waveformat_t *fmt, float *r) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;

    if (fmt->samplerate == src->samplerate) {
        return nframes;
    }

    if (src->need_reset || src->channels != fmt->channels || src->quality_changed || !src->src) {
        src->quality_changed = 0;
        src->remaining = 0;
        if (src->src) {
            src_delete (src->src);
            src->src = NULL;
        }
        src->channels = fmt->channels;
        src->src = src_new (src->quality, src->channels, NULL);
        src->need_reset = 0;
    }

    float ratio = src->samplerate / fmt->samplerate;
    ddb_src_set_ratio (_src, ratio);
    fmt->samplerate = src->samplerate;

    int numoutframes = 0;
    int outsize = nframes*24;
    float outbuf[outsize*fmt->channels];
    memset (outbuf, 0, sizeof (outbuf));
    char *output = (char *)outbuf;
    float *input = samples;
    int inputsize = nframes;

    int samplesize = fmt->channels * sizeof (float);

    do {
        // add more frames to input SRC buffer
        int n = inputsize;
        if (n >= SRC_BUFFER - src->remaining) {
            n = SRC_BUFFER - src->remaining;
        }

        if (n > 0) {
            memcpy (&src->in_fbuffer[src->remaining*samplesize], samples, n * samplesize);

            src->remaining += n;
            samples += n * fmt->channels;
        }
        if (!src->remaining) {
            trace ("WARNING: SRC input buffer starved\n");
            break;
        }

        // call libsamplerate
        src->srcdata.data_in = (float *)src->in_fbuffer;
        src->srcdata.data_out = (float *)output;
        src->srcdata.input_frames = src->remaining;
        src->srcdata.output_frames = outsize;
        src->srcdata.end_of_input = 0;
        trace ("src input: %d, ratio %f, buffersize: %d\n", src->srcdata.input_frames, src->srcdata.src_ratio, sizeof (outbuf));
        int src_err = src_process (src->src, &src->srcdata);
        trace ("src output: %d, used: %d\n", src->srcdata.output_frames_gen, src->srcdata.input_frames_used);

        if (src_err) {
            const char *err = src_strerror (src_err) ;
            fprintf (stderr, "src_process error %s\n"
                    "srcdata.data_in=%p, srcdata.data_out=%p, srcdata.input_frames=%d, srcdata.output_frames=%d, srcdata.src_ratio=%f", err, src->srcdata.data_in, src->srcdata.data_out, (int)src->srcdata.input_frames, (int)src->srcdata.output_frames, src->srcdata.src_ratio);
            return nframes;
        }

        inputsize -= n;
        output += src->srcdata.output_frames_gen * samplesize;
        numoutframes += src->srcdata.output_frames_gen;
        outsize -= src->srcdata.output_frames_gen;

        // calculate how many unused input samples left
        src->remaining -= src->srcdata.input_frames_used;
        // copy spare samples for next update
        if (src->remaining > 0 && src->srcdata.input_frames_used > 0) {
            memmove (src->in_fbuffer, &src->in_fbuffer[src->srcdata.input_frames_used*samplesize], src->remaining * samplesize);
        }
        if (src->srcdata.output_frames_gen == 0) {
            trace ("src: output_frames_gen=0, interrupt\n");
            break;
        }
    } while (inputsize > 0 && outsize > 0);

    memcpy (input, outbuf, numoutframes * fmt->channels * sizeof (float));
    //static FILE *out = NULL;
    //if (!out) {
    //    out = fopen ("out.raw", "w+b");
    //}
    //fwrite (input, 1,  numoutframes*sizeof(float)*(*nchannels), out);

    fmt->samplerate = src->samplerate;
    trace ("src: ratio=%f, in=%d, out=%d\n", ratio, nframes, numoutframes);
    return numoutframes;
}

int
ddb_src_num_params (void) {
    return SRC_PARAM_COUNT;
}

const char *
ddb_src_get_param_name (int p) {
    switch (p) {
    case SRC_PARAM_QUALITY:
        return "Quality";
    case SRC_PARAM_SAMPLERATE:
        return "Samplerate";
    default:
        fprintf (stderr, "ddb_src_get_param_name: invalid param index (%d)\n", p);
    }
}

void
ddb_src_set_param (ddb_dsp_context_t *ctx, int p, const char *val) {
    switch (p) {
    case SRC_PARAM_SAMPLERATE:
        ((ddb_src_libsamplerate_t*)ctx)->samplerate = atof (val);
        if (((ddb_src_libsamplerate_t*)ctx)->samplerate < 8000) {
            ((ddb_src_libsamplerate_t*)ctx)->samplerate = 8000;
        }
        if (((ddb_src_libsamplerate_t*)ctx)->samplerate > 192000) {
            ((ddb_src_libsamplerate_t*)ctx)->samplerate = 192000;
        }
        break;
    case SRC_PARAM_QUALITY:
        ((ddb_src_libsamplerate_t*)ctx)->quality = atoi (val);
        ((ddb_src_libsamplerate_t*)ctx)->quality_changed = 1;
        break;
    default:
        fprintf (stderr, "ddb_src_set_param: invalid param index (%d)\n", p);
    }
}

void
ddb_src_get_param (ddb_dsp_context_t *ctx, int p, char *val, int sz) {
    switch (p) {
    case SRC_PARAM_SAMPLERATE:
        snprintf (val, sz, "%f", ((ddb_src_libsamplerate_t*)ctx)->samplerate);
        break;
    case SRC_PARAM_QUALITY:
        snprintf (val, sz, "%d", ((ddb_src_libsamplerate_t*)ctx)->quality);
        break;
    default:
        fprintf (stderr, "ddb_src_get_param: invalid param index (%d)\n", p);
    }
}

static const char settings_dlg[] =
    "property \"Target Samplerate\" spinbtn[8192,192000,1] 0 48000;\n"
    "property \"Quality / Algorythm\" select[5] 1 2 SINC_BEST_QUALITY SINC_MEDIUM_QUALITY SINC_FASTEST ZERO_ORDER_HOLD LINEAR;\n"
;

static DB_dsp_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .open = ddb_src_open,
    .close = ddb_src_close,
    .process = ddb_src_process,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "SRC",
    .plugin.name = "Resampler (Secret Rabbit Code)",
    .plugin.descr = "Samplerate converter using libsamplerate",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sf.net",
    .plugin.website = "http://deadbeef.sf.net",
    .num_params = ddb_src_num_params,
    .get_param_name = ddb_src_get_param_name,
    .set_param = ddb_src_set_param,
    .get_param = ddb_src_get_param,
    .reset = ddb_src_reset,
    .configdialog = settings_dlg,
};

DB_plugin_t *
dsp_libsrc_load (DB_functions_t *f) {
    deadbeef = f;
    return &plugin.plugin;
}

/*
 * Copyright (c) 2005, 2006 - The Musepack Development Team
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *     * Neither the name of the The Musepack Development Team nor the
 *       names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior
 *       written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *   Thanks to Felipe Rivera for letting us use some code from his
 *   EQU-Plugin (http://equ.sourceforge.net/) for our Equalizer.
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "equalizer.h"

// Fixed Point Fractional bits
#define FP_FRBITS 28

// Conversions
#define EQ_REAL(x) ((int)((x) * (1 << FP_FRBITS)))

/* Floating point */
typedef struct {
    float beta;
    float alpha;
    float gamma;
} sIIRCoefficients;

/* Coefficient history for the IIR filter */
typedef struct {
    float x[3];                 /* x[n], x[n-1], x[n-2] */
    float y[3];                 /* y[n], y[n-1], y[n-2] */
} sXYData;

/* BETA, ALPHA, GAMMA */
static sIIRCoefficients iir_cforiginal10[] = {
    {(9.9421504945e-01), (2.8924752745e-03), (1.9941421835e+00)},   /*    60.0 Hz */
    {(9.8335039428e-01), (8.3248028618e-03), (1.9827686547e+00)},   /*   170.0 Hz */
    {(9.6958094144e-01), (1.5209529281e-02), (1.9676601546e+00)},   /*   310.0 Hz */
    {(9.4163923306e-01), (2.9180383468e-02), (1.9345490229e+00)},   /*   600.0 Hz */
    {(9.0450844499e-01), (4.7745777504e-02), (1.8852109613e+00)},   /*  1000.0 Hz */
    {(7.3940088234e-01), (1.3029955883e-01), (1.5829158753e+00)},   /*  3000.0 Hz */
    {(5.4697667908e-01), (2.2651166046e-01), (1.0153238114e+00)},   /*  6000.0 Hz */
    {(3.1023210589e-01), (3.4488394706e-01), (-1.8142472036e-01)},  /* 12000.0 Hz */
    {(2.6718639778e-01), (3.6640680111e-01), (-5.2117742267e-01)},  /* 14000.0 Hz */
    {(2.4201241845e-01), (3.7899379077e-01), (-8.0847117831e-01)},  /* 16000.0 Hz */
};

/* History for two filters */
static sXYData data_history[EQ_MAX_BANDS][EQ_CHANNELS];
static sXYData data_history2[EQ_MAX_BANDS][EQ_CHANNELS];

/* Coefficients */
static sIIRCoefficients *iir_cf;

/* Gain for each band
 * values should be between -0.2 and 1.0 */
static float gain[10];
static float preamp;

int round_trick(float floatvalue_to_round);


static void
output_set_eq(int active, float pre, float * bands)
{
    int i;

    preamp = 1.0 + 0.0932471 * pre + 0.00279033 * pre * pre;
    for (i = 0; i < 10; ++i)
        gain[i] = 0.03 * bands[i] + 0.000999999 * bands[i] * bands[i];
}

/* Init the filter */
void
init_iir(int on, float preamp_ctrl, float *eq_ctrl)
{
    iir_cf = iir_cforiginal10;

    /* Zero the history arrays */
    memset(data_history, 0, sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);
    memset(data_history2, 0, sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);

    output_set_eq(on, preamp_ctrl, eq_ctrl);
}

int
iir(char *d, int length)
{
    int16_t *data = (int16_t *) d;

    /* Indexes for the history arrays
     * These have to be kept between calls to this function
     * hence they are static */
    static int i = 0, j = 2, k = 1;

    int index, band, channel;
    int tempint, halflength;
    float out[EQ_CHANNELS], pcm[EQ_CHANNELS];

    /**
	 * IIR filter equation is
	 * y[n] = 2 * (alpha*(x[n]-x[n-2]) + gamma*y[n-1] - beta*y[n-2])
	 *
	 * NOTE: The 2 factor was introduced in the coefficients to save
	 * 			a multiplication
	 *
	 * This algorithm cascades two filters to get nice filtering
	 * at the expense of extra CPU cycles
	 */
    /* 16bit, 2 bytes per sample, so divide by two the length of
     * the buffer (length is in bytes)
     */
    halflength = (length >> 1);
    for (index = 0; index < halflength; index += 2) {
        /* For each channel */
        for (channel = 0; channel < EQ_CHANNELS; channel++) {
            /* No need to scale when processing the PCM with the filter */
            pcm[channel] = data[index + channel];
            /* Preamp gain */
            pcm[channel] *= preamp;

            out[channel] = 0;
            /* For each band */
            for (band = 0; band < 10; band++) {
                /* Store Xi(n) */
                data_history[band][channel].x[i] = pcm[channel];
                /* Calculate and store Yi(n) */
                data_history[band][channel].y[i] =
                    (iir_cf[band].alpha * (data_history[band][channel].x[i]
                                           - data_history[band][channel].x[k])
                     + iir_cf[band].gamma * data_history[band][channel].y[j]
                     - iir_cf[band].beta * data_history[band][channel].y[k]
                    );
                /*
                 * The multiplication by 2.0 was 'moved' into the coefficients to save
                 * CPU cycles here */
                /* Apply the gain  */
                out[channel] += data_history[band][channel].y[i] * gain[band];  // * 2.0;
            }                   /* For each band */

            if (0) {
                /* Filter the sample again */
                for (band = 0; band < 10; band++) {
                    /* Store Xi(n) */
                    data_history2[band][channel].x[i] = out[channel];
                    /* Calculate and store Yi(n) */
                    data_history2[band][channel].y[i] =
                        (iir_cf[band].alpha *
                         (data_history2[band][channel].x[i]
                          - data_history2[band][channel].x[k])
                         +
                         iir_cf[band].gamma *
                         data_history2[band][channel].y[j]
                         -
                         iir_cf[band].beta * data_history2[band][channel].y[k]
                        );
                    /* Apply the gain */
                    out[channel] +=
                        data_history2[band][channel].y[i] * gain[band];
                }               /* For each band */
            }

            /* Volume stuff
               Scale down original PCM sample and add it to the filters
               output. This substitutes the multiplication by 0.25
             */

            out[channel] += (data[index + channel] >> 2);

            //printf("out[channel] = %f\n", out[channel]);
            /* Round and convert to integer */
#if 0
#ifdef PPC
            tempint = round_ppc(out[channel]);
#else
# ifdef X86
            tempint = round_trick(out[channel]);
# else
            tempint = (int) lroundf(out[channel]);
# endif
#endif
#endif
            //tempint = (int) lroundf(out[channel]);
            tempint = (int) out[channel];

            //printf("iir: old=%d new=%d\n", data[index+channel], tempint);
            /* Limit the output */
            if (tempint < -32768)
                data[index + channel] = -32768;
            else if (tempint > 32767)
                data[index + channel] = 32767;
            else
                data[index + channel] = tempint;
        }                       /* For each channel */

        i++;
        j++;
        k++;

        /* Wrap around the indexes */
        if (i == 3)
            i = 0;
        else if (j == 3)
            j = 0;
        else
            k = 0;


    }                           /* For each pair of samples */

    return length;
}

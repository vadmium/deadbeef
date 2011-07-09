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
#include <stdint.h>
#include "equalizer.h"
#ifdef USE_NEON
#include <arm_neon.h>
#endif

#define trace(...) { android_trace(__VA_ARGS__); }
//#define trace(...)
void
android_trace (const char *fmt, ...);

//#define USE_FIXEDPOINT

#ifdef USE_FIXEDPOINT
typedef int32_t REAL;
#define BP 10
#define STEP (1.0 / (1<<BP))
#define FIXED(f) ((REAL)((f) / STEP))
#define REAL(f) ((float)(f) * STEP)
#define MUL(a,b) ((REAL)(((long long)(a)*(b))>>BP))
//#define DIV(a,b) ((((long long)(a)<<BP))/(b))
#else
typedef float REAL;
#define FIXED(f) (f)
#define REAL(f) (f)
#define MUL(a,b) ((a)*(b))
//#define DIV(a,b) ((a)/(b))
#endif

/* BETA, ALPHA, GAMMA */
#define BETA 0
#define ALPHA 1
#define GAMMA 2
static const REAL iir_cf[3][12] __attribute__ ((aligned (16))) = {
    { FIXED(9.9421504945e-01), FIXED(9.8335039428e-01), FIXED(9.6958094144e-01), FIXED(9.4163923306e-01), FIXED(9.0450844499e-01), FIXED(7.3940088234e-01), FIXED(5.4697667908e-01), FIXED(3.1023210589e-01), FIXED(2.6718639778e-01), FIXED(2.4201241845e-01), 0, 0 },

    { FIXED(2.8924752745e-03), FIXED(8.3248028618e-03), FIXED(1.5209529281e-02), FIXED(2.9180383468e-02), FIXED(4.7745777504e-02), FIXED(1.3029955883e-01), FIXED(2.2651166046e-01), FIXED(3.4488394706e-01), FIXED(3.6640680111e-01), FIXED(3.7899379077e-01), 0, 0 },

    { FIXED(1.9941421835e+00), FIXED(1.9827686547e+00), FIXED(1.9676601546e+00), FIXED(1.9345490229e+00), FIXED(1.8852109613e+00), FIXED(1.5829158753e+00), FIXED(1.0153238114e+00), FIXED(-1.8142472036e-01), FIXED(-5.2117742267e-01), FIXED(-8.0847117831e-01), 0, 0 }
};

/* History for two filters */
static REAL data_history_x[EQ_CHANNELS][3][EQ_MAX_BANDS] __attribute__ ((aligned (16)));
static REAL data_history_y[EQ_CHANNELS][3][EQ_MAX_BANDS] __attribute__ ((aligned (16)));

/* Gain for each band
 * values should be between -0.2 and 1.0 */
static REAL gain[EQ_MAX_BANDS] __attribute__ ((aligned (16)));
static REAL preamp;

static void
output_set_eq(int active, float pre, float * bands)
{
    int i;

    preamp = FIXED(1.0 + 0.0932471 * pre + 0.00279033 * pre * pre);

    for (i = 0; i < 10; ++i) {
        float g = bands[i];
        gain[i] = FIXED(0.03 * g + 0.000999999 * g * g);
    }
}

/* Init the filter */
void
init_iir(int on, float preamp_ctrl, float *eq_ctrl)
{
    /* Zero the history arrays */
    memset(data_history_x, 0, sizeof(data_history_x));
    memset(data_history_y, 0, sizeof(data_history_y));

    output_set_eq(on, preamp_ctrl, eq_ctrl);
}

#ifdef USE_ASM
void
EXTERN_ASMeq_apply_neon(float32_t *dhxi, float32_t *dhxk, float32_t *dhyi, float32_t *dhyj, float32_t *dhyk, float32_t pcm, float32_t *cfa, float32_t *cfb, float32_t *cfg, float32_t *gain, float32_t *out);
#endif

int
iir(int16_t * restrict data, int length)
{
    /* Indexes for the history arrays
     * These have to be kept between calls to this function
     * hence they are static */
    static int i = 0, j = 2, k = 1;

    int index, band, channel;
    int tempint, halflength;

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
            REAL out;
#ifdef USE_FIXEDPOINT
            REAL pcm = (REAL)data[index + channel] * preamp;
#else
            REAL pcm = MUL((float)data[index + channel],preamp);
#endif
#ifdef USE_NEON
            char *dhxi = (char *)data_history_x[channel][i];
            char *dhxk = (char *)data_history_x[channel][k];
            char *dhyi = (char *)data_history_y[channel][i];
            char *dhyj = (char *)data_history_y[channel][j];
            char *dhyk = (char *)data_history_y[channel][k];
            char *cfa = (char *)iir_cf[ALPHA];
            char *cfb = (char *)iir_cf[BETA];
            char *cfg = (char *)iir_cf[GAMMA];
            char *gains = (char *)gain;
#ifdef USE_ASM
            EXTERN_ASMeq_apply_neon((float32_t*)dhxi, (float32_t*)dhxk, (float32_t*)dhyi, (float32_t*)dhyj, (float32_t*)dhyk, pcm, (float32_t*)cfa, (float32_t*)cfb, (float32_t*)cfg, (float32_t*)gains, (float32_t*)&out);
#else

            float32x4_t q0 = vdupq_n_f32 (pcm);
            float s31 = 0;
            /* For each band */
            for (band = 0; band < 48; band += 16) {
                /* Store Xi(n) */
                vst1q_f32 ((float32_t*)(dhxi+band), q0);
                float32x4_t q1 = vld1q_f32((float32_t*)(cfa+band));
                float32x4_t q2 = vld1q_f32((float32_t*)(cfb+band));
                float32x4_t q3 = vld1q_f32((float32_t*)(cfg+band));

                /* Calculate and store Yi(n) */
                float32x4_t q4 = vld1q_f32((float32_t*)(dhxk+band));
                float32x4_t q5 = vld1q_f32((float32_t*)(dhyj+band));
                float32x4_t q6 = vld1q_f32((float32_t*)(dhyk+band));

                // alpha * (xi - xk) + gamma * yj - beta * yk
                q4 = vsubq_f32 (q0, q4);
                q4 = vmulq_f32 (q1, q4);
                q3 = vmulq_f32 (q3, q5);
                q2 = vmulq_f32 (q2, q6);
                q4 = vaddq_f32 (q4, q3);
                q4 = vsubq_f32 (q4, q2);
                vst1q_f32 ((float32_t*)(dhyi+band), q4);

                // yi * gain
                q1 = vld1q_f32((float32_t*)(gains+band));
                q4 = vmulq_f32 (q4, q1);

                // add to result
                s31 += vgetq_lane_f32 (q4, 0);
                s31 += vgetq_lane_f32 (q4, 1);
                s31 += vgetq_lane_f32 (q4, 2);
                s31 += vgetq_lane_f32 (q4, 3);
            }
            out = s31;
#endif
#else
            /* For each band */
            for (band = 0; band < 10; band++) {
                /* Store Xi(n) */
                data_history_x[channel][i][band] = pcm;
                /* Calculate and store Yi(n) */
                data_history_y[channel][i][band] =
                    (MUL(iir_cf[ALPHA][band], (data_history_x[channel][i][band] - data_history_x[channel][k][band]))
                     + MUL(iir_cf[GAMMA][band], data_history_y[channel][j][band])
                     - MUL(iir_cf[BETA][band], data_history_y[channel][k][band])
                    );
                /*
                 * The multiplication by 2.0 was 'moved' into the coefficients to save
                 * CPU cycles here */
                /* Apply the gain  */
                out += MUL(data_history_y[channel][i][band], gain[band]); // * 2.0;
            }                   /* For each band */
#endif

            /* Volume stuff
               Scale down original PCM sample and add it to the filters
               output. This substitutes the multiplication by 0.25
             */

            out += (data[index + channel] >> 2);
            
#ifdef USE_FIXEDPOINT
            tempint = out >> (BP-2);
#else
            tempint = (int) out;
            tempint *= 4;
#endif
//            trace ("data %d, preamp %lld, pcm %lld, out %d\n", (int)data[index + channel], preamp, pcm[channel], tempint);

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

/*
This file contains functions that acually handle the creation of the
raw audio samples. They return a buffer's worth of audio samples,
as defined by constants.BUFFER_SAMPLES.

Copyright (C) 2025

This file is part of StrataSonic

StrataSonic is a free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <math.h>

#include "synthesis.h"
#include "constants.h"


// samples is array of zeroes, of size BUFFER_SAMPLES
// When program starts release, it also must set env_vals.envAt to 0
void envelope(Envelope *env_vals, double *samples) {
    float samples_per_ms = (SAMPLE_RATE / 1000);
    int attack_samples = *(env_vals->attack_ms) * samples_per_ms;
    int decay_samples = *(env_vals->decay_ms) * samples_per_ms;
    int release_samples = *(env_vals->release_ms) * samples_per_ms;

    if (*(env_vals->sustain_percent) == 0 ||
        (env_vals->releasing && release_samples == 0)) {
        return;
    }

    double expo, num, percent;
    int s, sample;
    for (sample = 0; sample < BUFFER_SAMPLES; sample++) {
        s = sample + env_vals->envAt;
        percent = 0.0;
        if (!(env_vals->releasing)) {
            if (attack_samples != 0 && s < attack_samples) {
                expo = 2.56 * ((double)s / attack_samples);
                percent = (pow(2.0, expo) - 1) / 4.8971;
            } else if (decay_samples != 0  && s < (attack_samples + decay_samples)) {
                percent = pow(0.01, ((double)s / decay_samples));
                percent *= (1 - *(env_vals->sustain_percent));
                percent += *(env_vals->sustain_percent);
            } else {
                percent = *(env_vals->sustain_percent);
            }
        } else if (s < release_samples) {
            percent = pow(0.01, ((double)s / release_samples));
            percent *= *(env_vals->sustain_percent);
        } else {
            samples[sample] = 0.0;
            continue;
        }
        samples[sample] = percent;
        samples[sample] /= *(env_vals->sustain_percent);
        samples[sample] *= *(env_vals->sustain_value);
    }
    env_vals->envAt += BUFFER_SAMPLES;
}


//samples is array of ones, of size BUFFER_SAMPLES
void lfosc(LFO *lfo_vals, double *samples) {
    if (*(lfo_vals->speed_hz) == 0) { return; }
    else if (*(lfo_vals->percent_effect) == 0) { return; }

    double change_per_step = *(lfo_vals->speed_hz) / (double)SAMPLE_RATE;
    double s = lfo_vals->lfoAt;
    double wave;
    int sample;
    for (sample = 0; sample < BUFFER_SAMPLES; sample++) {
        s = fmod((s + change_per_step), 1.0);
        wave = cos(2 * M_PI * s);
        samples[sample] = (wave + 1) / 2;
        samples[sample] *= *(lfo_vals->percent_effect);
        samples[sample] += 1 - *(lfo_vals->percent_effect);
    }
    lfo_vals->lfoAt = s + change_per_step;
}


//samples is array of samples to filter, of size BUFFER_SAMPLES
void lpfilter(LPF *lpf_vals, double *samples, double *cutoff_freq_vals) {
    const double Q = 0.707106781186548; // = 1 / sqrt(2)
    double a_coeffs[3];
    double b_coeffs[3];
    double _sin, _cos, alpha;
    double result;
    int sample;
    for (int sample = 0; sample < BUFFER_SAMPLES; sample++) {
        _sin = sin(2.0 * M_PI * cutoff_freq_vals[sample] / SAMPLE_RATE);
        _cos = cos(2.0 * M_PI * cutoff_freq_vals[sample] / SAMPLE_RATE);
        alpha = _sin / (2 * Q);

        b_coeffs[0] = (1 - _cos) / 2;
        b_coeffs[1] = 1 - _cos;
        b_coeffs[2] = (1 - _cos) / 2;

        a_coeffs[0] = 1 + alpha;
        a_coeffs[1] = -2 * _cos;
        a_coeffs[2] = 1 - alpha;

        // Start at index 1 and do index 0 at the end.
        result = 0.0;
        for (int i = 1; i < 3; i++) {
            result += b_coeffs[i] * lpf_vals->in_history[i - 1];
            result -= a_coeffs[i] * lpf_vals->out_history[i - 1];
        }
        result += b_coeffs[0] * (samples[sample] * 2 - 1);
        result /= a_coeffs[0];

        // Re-assign arrays
        lpf_vals->in_history[2] = lpf_vals->in_history[1];
        lpf_vals->out_history[2] = lpf_vals->out_history[1];

        lpf_vals->in_history[1] = lpf_vals->in_history[0];
        lpf_vals->out_history[1] = lpf_vals->out_history[0];

        lpf_vals->in_history[0] = (samples[sample] * 2.0 - 1);
        lpf_vals->out_history[0] = result;

        samples[sample] = (result + 1) / 2;
    }
}


//samples is just any initialized array of size BUFFER_SAMPLES
void oscillator(Osc *osc_vals, double *samples,
                double *amp_values, double *pitch_shift_values,
                float unison) {
    double s;
    int sample;
    double note_step, hertz, change_per_step;
    for (sample = 0; sample < BUFFER_SAMPLES; sample++) {
        note_step = pitch_shift_values[sample] + osc_vals->note + unison;
        hertz = 440 * pow(2, (note_step / 12));
        change_per_step = hertz / SAMPLE_RATE;

        s = fmod((sample * change_per_step + osc_vals->oscAt), 1.0);
        switch (osc_vals->type) {
            case 0:
                samples[sample] = sin(2 * M_PI * s) * amp_values[sample];
                break;
            case 1:
                samples[sample] = (sin(2 * M_PI * s) > 0) ? 1 : -1;
                samples[sample] *= amp_values[sample];
                break;
            case 2:
                samples[sample] = (2 * s - 1) * amp_values[sample];
                break;
            case 3:
                samples[sample] = (s < 0.5) ? (4 * s - 1)  : (3 - 4 * s);
                samples[sample] *= amp_values[sample];
                break;
        }
    }
    osc_vals->oscAt = s + change_per_step;
}

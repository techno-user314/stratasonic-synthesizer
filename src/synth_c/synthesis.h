#ifndef SYNTHESIS_H
#define SYNTHESIS_H

typedef struct Envelope {
    int envAt;
    int releasing;
    float *sustain_value;

    int *attack_ms;
    int *decay_ms;
    float *sustain_percent;
    int *release_ms;
} Envelope;

typedef struct LFO {
    double lfoAt;
    int *speed_hz;
    double *percent_effect;
} LFO;

typedef struct LPF {
    double in_history[3];
    double out_history[3];
} LPF;

typedef struct Osc {
    int type;
    double oscAt;
    int note;
} Osc;

void envelope(Envelope *env_vals, double *samples);

void lfosc(LFO *lfo_vals, double *samples);

void lpfilter(LPF *lpf_vals, double *samples, double *cutoff_freq_vals);

void oscillator(Osc *osc_vals, double *samples,
                double *amp_values, double *pitch_shift_values,
                float unison);

#endif

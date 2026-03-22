/*
This file contains all the functions that control the process of
taking the input and turning it into an audible note.

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
#include <stdlib.h>
#include <math.h>

#include "constants.h"
#include "synthesis.h"
#include "recorder.h"

/*
Struct typedefs
*/
typedef struct Note {
    int note; // 0
    float *unison;

    Envelope *ampEnv;
    LFO *ampLFO;
    Envelope *pitchEnv;
    LFO *pitchLFO;

    LPF *lpfilter;
    Envelope *filterEnv;

    Osc *osc1;
    Osc *osc2;

    int releasing; // false
    int isdone; // false
} Note;

typedef struct Oscillator {
    int noteCount; // 0
    Note *notes[VOICES]; // {NULL}
    int input_map[VOICES]; // {-1}

    float unison_cents; // 0
    int oscType; // 0
    int selectedMod; // 0

    float amp; // 1
    Envelope *ampEnv;
    LFO *ampLFO;

    int pitchOctave; // 0
    int pitchSteps; // 0
    float pitchCents; // 0
    float pitch; // 0
    Envelope *pitchEnv;
    LFO *pitchLFO;

    float filter; // MAX_CUTOFF
    Envelope *filterEnv;
} Oscillator;

typedef struct Page {
    float amp; // 1
    int selectedOsc; // 0
    int recording; // false
    int playing; // false

    Oscillator *oscs[4];
    int unmuted[4]; // [true, true, true, true]

    Recorder *recorder;
} Page;

typedef struct Synth {
    Page *layers[4]; // {Page}
    int selectedLayer; // 0
    float amp; // 1
} Synth;


/*
Note functions
*/
void initialize_note(Note *note_ptr, Oscillator *parent_osc, int note_num) {
    // Initialize Note's members
    note_ptr->note = 0;
    note_ptr->unison = &(parent_osc->unison_cents);

    note_ptr->ampEnv = malloc(sizeof(Envelope));
    note_ptr->ampLFO = malloc(sizeof(LFO));
    note_ptr->pitchEnv = malloc(sizeof(Envelope));
    note_ptr->pitchLFO = malloc(sizeof(LFO));

    note_ptr->lpfilter = malloc(sizeof(LPF));
    note_ptr->filterEnv = malloc(sizeof(Envelope));

    note_ptr->osc1 = malloc(sizeof(Osc));
    note_ptr->osc2 = malloc(sizeof(Osc));

    note_ptr->releasing = 0;
    note_ptr->isdone = 0;

    // Shallow copy parent_osc's modulators into note
    *(note_ptr->ampEnv) = *(parent_osc->ampEnv);
    *(note_ptr->ampLFO) = *(parent_osc->ampLFO);
    *(note_ptr->pitchEnv) = *(parent_osc->pitchEnv);
    *(note_ptr->pitchLFO) = *(parent_osc->pitchLFO);
    *(note_ptr->filterEnv) = *(parent_osc->filterEnv);

    // Initialize lpfilter's members
    for (int i=0; i < 3; i++) {
        note_ptr->lpfilter->in_history[i] = 0.0;
        note_ptr->lpfilter->out_history[i] = 0.0;
    }

    // Initialize oscs' members
    note_ptr->osc1->type = parent_osc->oscType;
    note_ptr->osc1->oscAt = 0.0;
    note_ptr->osc1->note = note_num;

    note_ptr->osc2->type = parent_osc->oscType;
    note_ptr->osc2->oscAt = 0.0;
    note_ptr->osc2->note = note_num;
}

void destroy_note(Note *note_ptr) {
    free(note_ptr->ampEnv);
    free(note_ptr->ampLFO);
    free(note_ptr->pitchEnv);
    free(note_ptr->pitchLFO);

    free(note_ptr->lpfilter);
    free(note_ptr->filterEnv);

    free(note_ptr->osc1);
    free(note_ptr->osc2);
}

void note_start_release(Note *note_obj) {
    note_obj->releasing = 1;
    note_obj->ampEnv->releasing = 1;
    note_obj->pitchEnv->releasing = 1;
    note_obj->filterEnv->releasing = 1;
    note_obj->ampEnv->envAt = 0;
    note_obj->pitchEnv->envAt = 0;
    note_obj->filterEnv->envAt = 0;
}

void note_next_buffer(Note *note_ptr, double *samples) {
    double amp_env[BUFFER_SAMPLES] = {0.0};
    double pitch_env[BUFFER_SAMPLES] = {0.0};
    envelope(note_ptr->ampEnv, amp_env);
    envelope(note_ptr->pitchEnv, pitch_env);

    double amp_lfo[BUFFER_SAMPLES] = {0.0};
    double pitch_lfo[BUFFER_SAMPLES] = {0.0};
    for (int i=0; i < BUFFER_SAMPLES; i++) {
        amp_lfo[i] = 1.0;
        pitch_lfo[i] = 1.0;
    }
    lfosc(note_ptr->ampLFO, amp_lfo);
    lfosc(note_ptr->pitchLFO, pitch_lfo);

    double filter_env[BUFFER_SAMPLES] = {0.0};
    envelope(note_ptr->filterEnv, filter_env);

    for (int i=0; i < BUFFER_SAMPLES; i++) {
        filter_env[i] = MAX_CUTOFF - filter_env[i] + MIN_CUTOFF;
        amp_env[i] *= amp_lfo[i];
        pitch_env[i] *= pitch_lfo[i];
    }

    double temp_samples[BUFFER_SAMPLES];
    if (note_ptr->unison != 0) {
        oscillator(note_ptr->osc1,
                   samples, amp_env, pitch_env,  *(note_ptr->unison));
        oscillator(note_ptr->osc2,
                   temp_samples, amp_env, pitch_env, -*(note_ptr->unison));
        for (int i=0; i < BUFFER_SAMPLES; i++) {
            samples[i] += temp_samples[i];
            samples[i] /= 2;
        }
    } else {
        oscillator(note_ptr->osc1, samples, amp_env, pitch_env, 0.0);
    }
    lpfilter(note_ptr->lpfilter, samples, filter_env);
    if (note_ptr->releasing && amp_env[BUFFER_SAMPLES-1] == 0) {
        note_ptr->isdone = 1;
    }
}


/*
Oscillator functions
*/
void initialize_osc(Oscillator **osc) {
    *osc = malloc(sizeof(Oscillator));

    (*osc)->noteCount = 0;
    for (int i=0; i < VOICES; i++) {
        (*osc)->notes[i] = NULL;
        (*osc)->input_map[i] = -1;
    }

    (*osc)->unison_cents = 0.0;
    (*osc)->oscType = 0;
    (*osc)->selectedMod = 0;

    // Amplitude modulators
    (*osc)->amp = 1;

    (*osc)->ampEnv = malloc(sizeof(Envelope));
    (*osc)->ampEnv->envAt = 0;
    (*osc)->ampEnv->releasing = 0;
    (*osc)->ampEnv->sustain_value = &((*osc)->amp);
    (*osc)->ampEnv->attack_ms = malloc(sizeof(int));
    *((*osc)->ampEnv->attack_ms) = 0;
    (*osc)->ampEnv->decay_ms = malloc(sizeof(int));
    *((*osc)->ampEnv->decay_ms) = 0;
    (*osc)->ampEnv->sustain_percent = malloc(sizeof(float));
    *((*osc)->ampEnv->sustain_percent) = 1.0;
    (*osc)->ampEnv->release_ms = malloc(sizeof(int));
    *((*osc)->ampEnv->release_ms) = 0;

    (*osc)->ampLFO = malloc(sizeof(LFO));
    (*osc)->ampLFO->lfoAt = 0;
    (*osc)->ampLFO->speed_hz = malloc(sizeof(int));
    *((*osc)->ampLFO->speed_hz) = 0;
    (*osc)->ampLFO->percent_effect = malloc(sizeof(double));
    *((*osc)->ampLFO->percent_effect) = 0.0;

    // Pitch modulators
    (*osc)->pitchOctave = 0;
    (*osc)->pitchSteps = 0;
    (*osc)->pitchCents = 0;
    (*osc)->pitch = 0;

    (*osc)->pitchEnv = malloc(sizeof(Envelope));
    (*osc)->pitchEnv->envAt = 0;
    (*osc)->pitchEnv->releasing = 0;
    (*osc)->pitchEnv->sustain_value = &((*osc)->pitch);
    (*osc)->pitchEnv->attack_ms = malloc(sizeof(int));
    *((*osc)->pitchEnv->attack_ms) = 0;
    (*osc)->pitchEnv->decay_ms = malloc(sizeof(int));
    *((*osc)->pitchEnv->decay_ms) = 0;
    (*osc)->pitchEnv->sustain_percent = malloc(sizeof(float));
    *((*osc)->pitchEnv->sustain_percent) = 1.0;
    (*osc)->pitchEnv->release_ms = malloc(sizeof(int));
    *((*osc)->pitchEnv->release_ms) = 0;

    (*osc)->pitchLFO = malloc(sizeof(LFO));
    (*osc)->pitchLFO->lfoAt = 0;
    (*osc)->pitchLFO->speed_hz = malloc(sizeof(int));
    *((*osc)->pitchLFO->speed_hz) = 0;
    (*osc)->pitchLFO->percent_effect = malloc(sizeof(double));
    *((*osc)->pitchLFO->percent_effect) = 0.0;

    // Low-Pass Filter
    (*osc)->filter = 0;

    (*osc)->filterEnv = malloc(sizeof(Envelope));
    (*osc)->filterEnv->envAt = 0;
    (*osc)->filterEnv->releasing = 0;
    (*osc)->filterEnv->sustain_value = &((*osc)->filter);
    (*osc)->filterEnv->attack_ms = malloc(sizeof(int));
    *((*osc)->filterEnv->attack_ms) = 0;
    (*osc)->filterEnv->decay_ms = malloc(sizeof(int));
    *((*osc)->filterEnv->decay_ms) = 0;
    (*osc)->filterEnv->sustain_percent = malloc(sizeof(float));
    *((*osc)->filterEnv->sustain_percent) = 1.0;
    (*osc)->filterEnv->release_ms = malloc(sizeof(int));
    *((*osc)->filterEnv->release_ms) = 0;
}

void destroy_osc(Oscillator *osc) {
    free(osc->ampEnv->attack_ms);
    free(osc->ampEnv->decay_ms);
    free(osc->ampEnv->sustain_percent);
    free(osc->ampEnv->release_ms);
    free(osc->ampEnv);

    free(osc->ampLFO->speed_hz);
    free(osc->ampLFO->percent_effect);
    free(osc->ampLFO);

    free(osc->pitchEnv->attack_ms);
    free(osc->pitchEnv->decay_ms);
    free(osc->pitchEnv->sustain_percent);
    free(osc->pitchEnv->release_ms);
    free(osc->pitchEnv);

    free(osc->pitchLFO->speed_hz);
    free(osc->pitchLFO->percent_effect);
    free(osc->pitchLFO);

    free(osc->filterEnv->attack_ms);
    free(osc->filterEnv->decay_ms);
    free(osc->filterEnv->sustain_percent);
    free(osc->filterEnv->release_ms);
    free(osc->filterEnv);

    // Check for and destroy unreleased notes
    for (int i=0; i < VOICES; i++) {
        if (osc->notes[i] != NULL) {
            destroy_note(osc->notes[i]);
            free(osc->notes[i]);
        }
    }
    free(osc);
}

void osc_add_note(Oscillator *osc, int note) {
    for (int i = 0; i < VOICES; i++) {
        if (osc->notes[i] == NULL) {
            osc->notes[i] = malloc(sizeof(Note));
            initialize_note(osc->notes[i], osc, note);
            osc->noteCount += 1;
            osc->input_map[i] = note + MIDI_TO_A4;
            break;
        }
    }
}

void osc_rm_note(Oscillator *osc, int in_note) {
    for (int i = 0; i < VOICES; i++) {
        if (osc->input_map[i] == in_note) {
            note_start_release(osc->notes[i]);
            osc->input_map[i] = -1;
            break;
        }
    }
}

void osc_end_notes(Oscillator *osc) {
    int fake_input;
    for (int i = 0; i < VOICES; i++) {
        fake_input = osc->input_map[i];
        if (fake_input != -1) {
            note_start_release(osc->notes[i]);
            osc->input_map[i] = -1;
        }
    }
}

void osc_modify_envelope(Oscillator *osc, int parameter, float new_value) {
    Envelope *to_modify;
    switch (osc->selectedMod) {
        case 0:
            to_modify = osc->ampEnv;
            break;
        case 1:
            to_modify = osc->pitchEnv;
            break;
        case 2:
            if (parameter == ENV_DECAY || parameter == ENV_SUSTAIN) {
                return;
            }
            to_modify = osc->filterEnv;
            break;
    }

    switch (parameter) {
        case ENV_ATTACK:
            *(to_modify->attack_ms) = (exp(new_value * 2.85) - 1) / 2 * 500;
            break;
        case ENV_DECAY:
            *(to_modify->decay_ms) = (exp(new_value * 2.85) - 1) / 2 * 500;
            break;
        case ENV_SUSTAIN:
            *(to_modify->sustain_percent) = new_value;
            break;
        case ENV_RELEASE:
            *(to_modify->release_ms) = (exp(new_value * 2.85) - 1) / 2 * 500;
            *(osc->ampEnv->release_ms) = (exp(new_value * 2.85) - 1) / 2 * 500;
            break;
    }
}

void osc_modify_lfo(Oscillator *osc, int parameter, float new_value) {
    LFO *to_modify;
    switch (osc->selectedMod) {
        case 0: to_modify = osc->ampLFO; break;
        case 1: to_modify = osc->pitchLFO; break;
        case 2: return;
    }

    switch (parameter) {
        case LFO_AMP:
            *(to_modify->percent_effect) = new_value;
            break;
        case LFO_SPEED:
            *(to_modify->speed_hz) = new_value;
            break;
    }
}

void osc_process_input(Oscillator *osc, int action, int button, float value) {
    float num;
    switch (action) {
        case ADD_NOTE:
            osc_add_note(osc, button);
            break;
        case RM_NOTE:
            osc_rm_note(osc, button);
            break;
        case OSC_TYPE:
            osc->oscType = (osc->oscType + 1) % 4;
            break;
        case OSC_AMP:
            osc->amp = value;
            break;
        case OSC_OCTAVE:
            osc->pitchOctave += button * 12;
            osc->pitch = osc->pitchOctave;
            osc->pitch += osc->pitchSteps + osc->pitchCents;
            break;
        case OSC_PITCH:
            if (button == 4) {
                osc->pitchSteps = value;
            } else {
                osc->pitchCents = value / 100;
            }
            osc->pitch = osc->pitchOctave;
            osc->pitch += osc->pitchSteps + osc->pitchCents;
            break;
        case MODULATOR_SELECT:
            osc->selectedMod = button;
            break;
        case ENV_ATTACK:
            osc_modify_envelope(osc, ENV_ATTACK, value);
            break;
        case ENV_DECAY:
            osc_modify_envelope(osc, ENV_DECAY, value);
            break;
        case ENV_SUSTAIN:
            osc_modify_envelope(osc, ENV_SUSTAIN, value);
            break;
        case ENV_RELEASE:
            osc_modify_envelope(osc, ENV_RELEASE, value);
            break;
        case LFO_SPEED:
            osc_modify_lfo(osc, LFO_SPEED, value);
            break;
        case LFO_AMP:
            osc_modify_lfo(osc, LFO_AMP, value);
            break;
        case FILTER_FREQ:
            osc->filter = MAX_CUTOFF - value * MAX_CUTOFF;
            break;
        case UNISON:
            osc->unison_cents = value / 100;
            break;
    }
}

void osc_next_buffer(Oscillator *osc, double *samples) {
    for (int i=0; i < VOICES; i++) {
        if (osc->notes[i] != NULL && osc->notes[i]->isdone) {
            destroy_note(osc->notes[i]);
            free(osc->notes[i]);
            osc->notes[i] = NULL;
            osc->noteCount -= 1;
        }
    }

    if (osc->noteCount == 0 || *(osc->ampEnv->sustain_percent) == 0){ return; }

    double temp_samples[BUFFER_SAMPLES] = {0};
    for (int i=0; i < VOICES; i++) {
        if (osc->notes[i] != NULL) {
            note_next_buffer(osc->notes[i], temp_samples);
            for (int j=0; j < BUFFER_SAMPLES; j++) {
                samples[j] += temp_samples[j];
                temp_samples[j] = 0.0;
            }
        }
    }
    if (osc->noteCount > 0) {
        float note_weight = 1 / *(osc->ampEnv->sustain_percent);
        note_weight *= (4 > osc->noteCount) ? 4 : osc->noteCount;
        note_weight = 1 / note_weight;
        for (int i=0; i < BUFFER_SAMPLES; i++) {
            samples[i] *= note_weight;
        }
    }
}


/*
Page functions
*/
void initialize_page(Page **page) {
    *page = malloc(sizeof(Page));

    (*page)->amp = 1.0;
    for (int i=0; i < 4; i++) {
        (*page)->unmuted[i] = 1;
    }
    (*page)->selectedOsc = 0;
    (*page)->recording = 0;
    (*page)->playing = 0;

    Oscillator *osc1, *osc2, *osc3, *osc4;
    initialize_osc(&osc1);
    initialize_osc(&osc2);
    initialize_osc(&osc3);
    initialize_osc(&osc4);
    (*page)->oscs[0] = osc1;
    (*page)->oscs[1] = osc2;
    (*page)->oscs[2] = osc3;
    (*page)->oscs[3] = osc4;

    (*page)->recorder = malloc(sizeof(Recorder));
    initialize_recorder((*page)->recorder);
}

void destroy_page(Page *page) {
    // Free oscillators
    for (int i=0; i < 4; i++) {
        destroy_osc(page->oscs[i]);
    }
    // Free the recorder
    clear_recorder(page->recorder);
    free(page->recorder);
    // Free page variables
    free(page);
}

void page_end_notes(Page *layer) {
    for (int i = 0; i < 4; i++) {
        osc_end_notes(layer->oscs[i]);
    }
}

void page_process_input(Page *layer, int action, int button, float value) {
    Oscillator *osc;
    switch (action) {
        case ADD_NOTE:
            recorder_record(layer->recorder, action, button);
            for (int i = 0; i < 4; i++) {
                if (layer->unmuted[i]) {
                    osc = layer->oscs[i];
                    osc_process_input(osc, action, button, value);
                }
            }
            break;
        case RM_NOTE:
            recorder_record(layer->recorder, action, button);
            for (int i = 0; i < 4; i++) {
                osc = layer->oscs[i];
                osc_process_input(osc, action, button, value);
            }
            break;
        case LAYER_AMP:
            layer->amp = value;
            break;
        case LAYER_REC:
            if (button == 1 && !layer->recording) {
                layer->playing = !layer->playing;
            } else if (button == 2) {
                layer->recording = !layer->recording;
                layer->playing = !layer->recording;
                if (layer->recording) {
                    clear_recorder(layer->recorder);
                }
            }
            break;
        case OSC_SELECT:
            layer->selectedOsc = button;
            break;
        case OSC_MUTE:
            if (button == 4) {
                layer->unmuted[layer->selectedOsc] = !layer->unmuted[layer->selectedOsc];
            } else {
                layer->unmuted[button] = !layer->unmuted[button];
            }
            break;
        case OSC_AMP:
            osc = layer->oscs[button];
            osc_process_input(osc, action, button, value);
            break;
        case OSC_PITCH:
            if (button != 4) {
                osc = layer->oscs[button];
                osc_process_input(osc, action, button, value);
            } else {
                osc = layer->oscs[layer->selectedOsc];
                osc_process_input(osc, action, button, value);
            }
            break;
        case FILTER_FREQ:
            osc = layer->oscs[button];
            osc_process_input(osc, action, button, value);
            break;
        default:
            osc = layer->oscs[layer->selectedOsc];
            osc_process_input(osc, action, button, value);
            break;
    }
}

void page_next_buffer(Page *page, double *samples) {
    int num_unmuted = 0;
    for (int i=0; i < 4; i++) {
        num_unmuted += page->unmuted[i];
    }
    if (num_unmuted == 0 ) { return; }

    double temp_samples[BUFFER_SAMPLES] = {0};
    for (int i=0; i < 4; i++) {
        osc_next_buffer(page->oscs[i], temp_samples);
        for (int j=0; j < BUFFER_SAMPLES; j++) {
            samples[j] += temp_samples[j];
            temp_samples[j] = 0.0;
        }
    }
    for (int i=0; i < BUFFER_SAMPLES; i++) {
        samples[i] *= page->amp;
        samples[i] /= 4;
    }

    // Tick recorder
    if (page->recording && page->recorder->input_count > 0) {
        recorder_tick(page->recorder, 1);
    } else if (page->playing) {
        Recorder *rec_ptr = page->recorder;
        recorder_tick(rec_ptr, 0);
        for (int i=0; i < rec_ptr->input_count; i++) {
            if (rec_ptr->inputs[i]->tick_played == rec_ptr->play_tick) {
                page_process_input(page,
                                   rec_ptr->inputs[i]->value[0],
                                   rec_ptr->inputs[i]->value[1], -1);
            }
        }
    }
}


/*
Synth functions
*/
Synth *init() {
    Synth *synth1 = malloc(sizeof(Synth));
    synth1->amp = 1.0;
    synth1->selectedLayer = 0;

    Page *page1, *page2, *page3, *page4;
    initialize_page(&page1);
    initialize_page(&page2);
    initialize_page(&page3);
    initialize_page(&page4);
    synth1->layers[0] = page1;
    synth1->layers[1] = page2;
    synth1->layers[2] = page3;
    synth1->layers[3] = page4;

    return synth1;
}

void terminate(Synth *synth) {
    for (int i=0; i < 4; i++) {
        destroy_page(synth->layers[i]);
    }
    free(synth);
}

void process_input(Synth *synth, int action, int button, float value) {
    Page *layer;
    if (action == -1) { return; }
    switch (action) {
        case SET_VOLUME:
            synth->amp = value;
            break;
        case LAYER_SELECT:
            page_end_notes(synth->layers[synth->selectedLayer]);
            synth->selectedLayer = button;
            break;
        case LAYER_AMP:
            layer = synth->layers[button];
            page_process_input(layer, action, button, value);
            break;
        default:
            layer = synth->layers[synth->selectedLayer];
            page_process_input(layer, action, button, value);
            break;
    }
}

void next_buffer(Synth *synth, double *samples) {
    double temp_samples[BUFFER_SAMPLES] = {0};
    for (int i=0; i < 4; i++) {
        page_next_buffer(synth->layers[i], temp_samples);
        for (int j=0; j < BUFFER_SAMPLES; j++) {
            samples[j] += temp_samples[j];
            temp_samples[j] = 0.0;
        }
    }
    for (int i=0; i < BUFFER_SAMPLES; i++) {
        samples[i] *= synth->amp;
        samples[i] /= 4;
    }
}

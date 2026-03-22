/*
Define program-wide constants.

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
#ifndef CONSTANTS_H
#define CONSTANTS_H

#define BUFFER_SAMPLES 256 // Number of samples in one buffer of audio
#define SAMPLE_RATE 44100 // Sampling rate of the audio
#define VOICES 12 // Max number of voices that a layer can have
#define REC_MAX_INPUTS 100 // Max number of inputs that can be recorded
#define MIDI_TO_A4 57 // MIDI input that corrosponds to A4
#define MAX_CUTOFF 10000 // Maximum filter cutoff frequancy
#define MIN_CUTOFF 250 // Minimun filter cutoff frequancy

#define SET_VOLUME 1
#define ADD_NOTE 2
#define RM_NOTE 3
#define LAYER_SELECT 4
#define LAYER_AMP 5
#define LAYER_REC 6
#define OSC_SELECT 7
#define OSC_MUTE 8
#define OSC_AMP 9
#define OSC_TYPE 10
#define OSC_OCTAVE 11
#define OSC_PITCH 12
#define ENV_ATTACK 13
#define ENV_DECAY 14
#define ENV_SUSTAIN 15
#define ENV_RELEASE 16
#define LFO_SPEED 17
#define LFO_AMP 18
#define FILTER_TYPE 19
#define FILTER_FREQ 20
#define FILTER_AMP 21
#define FILTER_PARAM 22
#define MODULATOR_SELECT 23
#define UNISON 24

#endif

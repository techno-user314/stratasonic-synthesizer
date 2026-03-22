"""
This file handles converting MIDI input codes into program universal constants
for control of the synth. The 'inpt' parameter is a MIDI code, e.g. [7, 15, 127].

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
"""
from math import e

from constants import *

# Function to turn raw MIDI codes into action codes
def parse_input(msg):
    match msg:
        case [153, 59, _]: 
            return POWER, -1, -1

        case [176, 1, value]: 
            return SET_VOLUME, -1, value / 127

        case [153, button, _] if button in [40, 41, 42, 43]:
            return LAYER_SELECT, button - 40, -1

        case [slider, 7, value] if slider in [184, 185, 186, 187]:
            return LAYER_AMP, slider - 184, value / 127
        
        case [176, button, 127] if button in [18, 19, 20]:
            return LAYER_REC, button - 18, -1
            
        case [176, 15, 127]:
            return OSC_TYPE, -1, -1

        case [176, button, 127] if button in [16, 17]:
            return OSC_OCTAVE, (button - 16) * 2 - 1, -1

        case [153, button, _] if button in [36, 37, 38, 39]:
            return OSC_MUTE, button - 36, -1

        case [153, 48, _]:
            return OSC_MUTE, 4, -1

        case [153, button, _] if button in [44, 45, 46, 47]:
            return OSC_SELECT, button - 44, -1

        case [slider, 7, value] if slider in [176, 177, 178, 179]:
            return OSC_AMP, slider - 176, value / 127

        case [224, 0, value] if value != 64:
            return OSC_PITCH, 4, int((value / 127 - 0.5) * 24)

        case [176, knob, value] if knob in [30, 31, 32, 33]:
            return OSC_PITCH, knob - 30, value

        case [153, button, _] if button in [49, 50, 51]:
            return MODULATOR_SELECT, button - 49, -1

        case [180, 7, value]:
            return ENV_ATTACK, -1, value / 127

        case [181, 7, value]:
            return ENV_DECAY, -1, value / 127

        case [182, 7, value]:
            return ENV_SUSTAIN, -1, value / 127

        case [183, 7, value]:
            return ENV_RELEASE, -1, value / 127

        case [176, 34, value]:
            return LFO_AMP, -1, value / 127

        case [176, 35, value]:
            return LFO_SPEED, -1, value * 30 / 127

        case [176, 37, value]:
            return UNISON, -1, value

        case [176, knob, value] if knob in [38, 39, 40, 41]:
            return FILTER_FREQ, knob - 38, (value / 127) ** e

        case [144, button, velocity]:
            return ADD_NOTE, button - MIDI_TO_A4, velocity

        case [128, button, _]:
            return RM_NOTE, button, -1

        case _:
            #print(f"No known action for {msg}")
            return -1, -1, -1

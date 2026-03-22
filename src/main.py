"""
This is StrataSonic: A digital subtractive synthesizer program
that is semi-polyphonic, and has built-in audio recording and layering.

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
import sys
import time
import math
import struct
import ctypes

import pyaudio
from rtmidi.midiutil import open_midiinput

from midi_input import parse_input
from constants import BUFFER_SAMPLES, SAMPLE_RATE

# Declare the C library interface
synth = ctypes.CDLL("./libcsynth.so")

synth.init.restype = ctypes.c_void_p
synth.terminate.argtypes = [ctypes.c_void_p]
synth.process_input.argtypes = [ctypes.c_void_p, ctypes.c_int,
                                ctypes.c_int, ctypes.c_float]
synth.next_buffer.argtypes = [ctypes.c_void_p,
                              ctypes.POINTER(ctypes.c_double)]

# Define globals
CHANNELS = 1
soundforge = synth.init()
streaming = True


# Callback functions
class MidiInputHandler(object):
    def __init__(self, port):
        self.port = port
        self._wallclock = time.time()

    def __call__(self, event, data=None):
        global streaming
        message, deltatime = event
        self._wallclock += deltatime
        
        code, value1, value2 = parse_input(message)
        if code == 0:
           streaming = False
        elif code != -1:
            synth.process_input(soundforge, code, value1, value2)

def audio_callback(in_data, frame_count, time_info, status):
    datarray = [0 for _ in range(BUFFER_SAMPLES)]
    datarray = (ctypes.c_double * BUFFER_SAMPLES)(*datarray)
    synth.next_buffer(soundforge, datarray)

    # Convert audio samples from unsigned double to 16-bit signed integer
    buffer = [int(max(-1, min(s, 1)) * 32767) for s in datarray]
    format_str = '<' + str(BUFFER_SAMPLES) + 'h'
    data = bytearray(struct.pack(format_str, *buffer))

    # Detect stream complete
    code = pyaudio.paContinue if streaming else pyaudio.paComplete
    return bytes(data), code


# Main loop
print("Setting up StrataSonic - ")
print("Opening MIDI device...")
midiin, port_name = open_midiinput(1)
midiin.set_callback(MidiInputHandler(port_name))

print("Creating stream...")
audio = pyaudio.PyAudio()
stream = audio.open(format=pyaudio.paInt16,
                    channels=CHANNELS,
                    rate=SAMPLE_RATE,
                    output=True,
                    stream_callback=audio_callback,
                    frames_per_buffer=BUFFER_SAMPLES)

try:
    print("Streaming active - ")
    while stream.is_active():
        time.sleep(1)
    streaming = False

except Exception as e:
    streaming = False
    print("\nError:", e)

finally:
    print("\nExiting", end="")

    # Disengage RtMidi
    midiin.close_port()
    del midiin
    print(".", end="")

    # Disengage PyAudio
    stream.close()
    audio.terminate()
    print(".", end="")

    # Free system resources
    synth.terminate(soundforge)
    print(".\nShutdown successful. Goodbye!")

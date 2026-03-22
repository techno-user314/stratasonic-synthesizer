# StrataSonic - A Digital Subtractive Synthesizer
<sup>**Version 1.2.0**  |  Iteration 1.9.10.21 </sup>  
  
StrataSonic is a digital audio synthesizer designed to allow simple music creation without the need for a full DAW. It can run on a headless Raspberry Pi, and is controlled via MIDI input.

## Features
- StrataSonic creates sound via subtractive synthesis.
- StrataSonic currently supports up to 25 voices per layer. In the future, this will be user defined so that it can be customized to the user's computer's architechture capabilities.
- There are four distinct, toggleable layers. Each layer has it's own customizable oscillators, effectively acting like four seperate synthesizers. Each layer has a record feature, and allows you to record, and than loop, a background harmony. This feature can be used as a toggle between four settings banks.
- Every one of the 16 oscillators has it's own filter and waveform selection, along with it's own independant envelopes and modulators for amplitude, pitch shifting, and filter cutoff frequancy.

## Installation
Here are the steps to install and begin using StrataSonic:
1. Ensure that both Python and a C compiler are installed on your system.
2. Install the necessary Python libraries with `pip install python-rtmidi pyaudio`
3. Clone this repo.
4. Compile the files in "src/c_synth" into a shared library called "libcsynth".
    - Using GCC on a Linux system, this command would be: `gcc -shared -o libcsynth.so -fPIC control.c synthesis.c recorder.c`
5. Run main.py with a Python interperator running Python 3.11+
> [!NOTE]
> StrataSonic uses PortAudio19 as the audio interface. If Python is throwing exceptions related to pyaudio, it may be fixed by installing PortAudio19.
> On Linux systems: `sudo apt install portaudio19-dev`

## Usage
Run main.py with a Python interperater to launch the synthesizer. If you are on a headless Rasperry Pi, it is recommended to have main.py run on startup. StrataSonic should automatically interface with any connected MIDI device. StrataSonic is setup for usage with a Donner MIDI MK-25 by default, however, all the mappings are adjustable by modifying midi_input.py.
Possible bindings include, but are not limited to:
- Adjust volume
- Play note
- Change selected layer
- Change selected layer's amplitude
- Start recording[^1]
- Playback recording in loop[^2]
- Change selected oscillator
- Mute oscillator
- Change selected oscillator's amplitude
- Change selected oscillators waveform
- Change selected oscillators detune or pitch offset[^3]
- Select a modulator to adjust for the currently selected oscillator
- Change modulator settings[^4]

[^1]: Recording starts when the first note is played, and ends when the recording button is pushed again.  
[^2]: It is also possible to pause the recording playback.  
[^3]: It is possible to adjust the pitch offset in units of cents, steps and octaves.
[^4]: There are several bindings for this that vary by modulator. For example, the low frequancy oscillator two settings: speed and amplitude, but an envelope has four: attack, decay, sustain, release.  

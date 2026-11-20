# Upright

Upright is a physically modelled piano for the Teensy Audio Library: a felt hammer strikes a stiff
string, heard through a soundboard or a pickup. A grand, an upright, a Rhodes and a Wurlitzer are
the same model with different numbers, played as one voice or as a polyphonic pool.

Play it at https://dangelo.engineering (the piano under Upright there).

## Install

- Arduino IDE: download this repository as a ZIP, then Sketch > Include Library > Add .ZIP Library.
- PlatformIO: `lib_deps = https://github.com/dangelo729/upright.git`

It needs a Teensy 4.x and the Teensy Audio Library (in Teensyduino); then `#include <Upright.h>`.

## Use

```cpp
#include <Audio.h>
#include <Upright.h>

AudioSynthUpright    piano;
AudioOutputI2S       i2s;
AudioConnection      cordLeft(piano, 0, i2s, 0), cordRight(piano, 0, i2s, 1);
AudioControlSGTL5000 codec;

void setup() {
    AudioMemory(8);
    codec.enable();
    codec.volume(0.5f);
    up::Tuning t;
    t.outputGain = 4.0f;            // the piano to full scale: this note peaks near 0.44
    piano.setTuning(t);
    piano.setShape(up::grand());    // the instrument
}

void loop() {
    piano.noteOn(60.0f, 0.8f);      // middle c, velocity 0.8
    delay(1000);
    piano.noteOff();                // the key let go: its damper lands
    delay(1000);
}
```

`AudioSynthUpright` is one voice with a soundboard of its own:

- `noteOn(midi, velocity)` strikes a note, velocity 0 to 1; `noteOff()` lets the key go.
- `setPedal(amount)` is the sustain pedal, 0 up to 1 down.
- `setShape(up::rhodes())` chooses an instrument: `grand`, `concertGrand`, `upright`, `spinet`,
  `honkyTonk`, `fortepiano`, `rhodes`, `rhodesBark` or `wurlitzer` (by name: `up::presetByName`).
- `set(up::Channel::hardness, 300.0f)` sets one of the 24 parameters that make the instrument (the
  strings, the hammer's felt, the soundboard, the pickup and more); a sounding note eases to it.
- `setTuning(t)` sets the output gain and the engine's rates from an `up::Tuning`.

`AudioSynthUprightPool<M>` is M voices on one shared soundboard, for a keyboard: `noteOn(midi,
velocity)`, `noteOff(midi)`, `pedal(amount)`, `allNotesOff()`, and `setShape`, `set` and
`setTuning` for every voice at once. A key struck while all M voices sound takes the quietest one.
Every call lands at the start of the next audio block (128 samples, 2.9 ms). Declare the objects
globally, as every Teensy Audio object.

## Examples

- `01_Grand`: one piano, a note every two seconds (the sketch above).
- `02_Channels`: one voice set from the serial monitor; a pot on A0 is the hammer's hardness.
- `03_Keyboard`: eight voices on USB MIDI, the sustain pedal, presets by program change; it needs
  a USB Type with MIDI and audio (Tools > USB Type > Serial + MIDI + Audio).
- `99_Benchmark`: the processor usage of pools of eight and four voices at their worst case.

## On a computer

The core (`src/core`) is plain C++17: `make test` runs its tests with clang, and `make tool` builds
`extras/host/build/uptool`, which renders notes to WAV files and spectrograms and fits the model.

## More

- [How it works](extras/docs/design.md)
- [The public surface](extras/docs/reference.md)
- [The host rig](extras/docs/host.md)
- [Known limits](extras/docs/limits.md)

## Credits and license

Upright is MIT (`LICENSE`) and contains no third-party code. Its physics and its numbers come from
published work on piano acoustics, by Bank, Chaigne and Askenfelt, Weinreich and many others;
`NOTICE` names every source.

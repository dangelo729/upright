# upright

A piano synthesizer for the Teensy Audio Library (Teensy 4.1).

A struck string is a mode table, driven by a hammer in a loop, heard
through a body or a pickup. You do not pick a piano; you set its scale, its
hammers, where they strike, how many strings a note has, its body, what the
hammer hits and how it is heard, and how it is damped, and the sound is the
consequence. Upright, grand, Rhodes and Wurlitzer differ by numbers, not by
mechanism.

- **Twenty-four channels, every point a piano.** The instrument's
  dimensions, in physical units where they have them (the strings' lengths in metres and their
  tension in newtons, the felt's stiffness in N/mm^p, the board's area in
  m², the pickup's gap in millimetres, the tine's t60 in seconds), every
  value inside a channel's range legal, and a pure law from them to every
  partial. A preset is a table of the twenty-four; nine are named, from a
  grand and an upright to a Rhodes and a Wurlitzer.
- **The mode table is live and addressable.** The law writes every partial
  of the string as a channel under the instrument's sequence; a later write
  to one partial wins; a measured string plays directly.
- **The string at every key.** A rank turns the string at middle C into the
  string at every note: its length, its mass, its wire (plain, or wrapped on
  a core that thickens toward the bass), its stiffness, its hammer (the
  felt harder up the keyboard) and where the hammer lands.
- **The hammer is in the loop.** A mass on a felt spring meets the string,
  compresses by a power law, loses through the felt, and is thrown off by
  the string's own reply; while it touches, the strings turn at twice the
  sample rate. A key struck again meets its moving string. Its attack is
  more than the string: the blow reaching the board through the action,
  the key's thud on its bed and the string's longitudinal clank, which grows
  faster than the blow.
- **The unison is coupled.** Up to three strings a note on one bridge, as
  Weinreich's coupled piano strings, struck by one hammer: tuned, the low
  partials lock and decay in two stages, the prompt sound and the
  aftersound, while the upper ones beat slowly and die; detuned wide, they
  beat.
- **One output stage, two kinds.** A soundboard that is a plate's mode table
  heard as its velocity, with the plate's diffuse field above its modes, or
  a pickup: one bounded curve of the tine's tip, a magnet's or a plate's,
  differentiated in time. The generator channel chooses, and crossfades.
- **Polyphonic, on one soundboard.** A pool of M voices with a note table: a
  key re-struck meets its own moving string, the quietest voice is stolen
  by a fade, the sustain pedal holds every damper off, and every key drives
  one shared board where its string crosses the bridge.
- **The picture is the engine's own state.** Every partial's amplitude, the
  hammer, the levels; the string's shape along its length and the board's
  over its area from the same phasors.
- **Host-first.** Everything compiles on a Mac with clang and nothing else
  (the two Teensy wrappers to nothing), renders WAVs and spectrograms, and
  passes tests; the board is a compile.

Library `Upright`, namespace `up`, file prefix `up_`, classes
`AudioSynthUpright` and `AudioSynthUprightPool<M>`, tool `uptool`.

## Status

| stage | what | |
|---|---|---|
| 0 — the laws and the chart | the channels, the knobs, the string, tine and reed laws, the rank, the hammer's contact simulated offline, the pickup's curves, the plate's table; `uptool` partials, string, hammer, pickup, board, print, random, fit, chart; the reference tables; the tests | **done** |
| 1 — the engine | stage 0's decisions in the laws; the bank, the hammer's loop, the strings and the unison, the pitch axis, the dampers and the pedal, the pickup and the board as stages, the telemetry, the voice; `uptool` play, corpus, size; the WAV, the spectrogram, the analysis | **done** |
| 2 — the instrument | report-1's eight decisions (the rank's felt split at C4, the hammer knob the stiffness alone, kScale 0.03, the outs' polarity, the asymmetric unison, the board's velocity and shelf, the bounded electrostatic curve); the level table; the audibility floors held in renders; the nine presets; `uptool` level, chart --audio, random's and fit's hearing, --preset, play --undo-level, board --sweep; the corpus grown; the listening WAVs | **done** |
| 3 — the pool, the wrappers, the board, the docs | report-2's decisions (the tip's weight held to ±1, the level table by four anchors over an octave, the tests re-read where the measure missed); `up_pool` with its shared board; `AudioSynthUpright`, `AudioSynthUprightPool<M>`; the four examples; `make board`; the corpus baseline | **done** |
| 3b — three fixes | report-3's decisions: a partial that moves grows in neither output (up, its velocity kept; down, its displacement); the plate's mode pairs found once; kBoardOut set so grand's octave reads 0.0897 rms, the level table written again, the examples' gains measured again; the corpus baseline and `make board` again | **done** |
| 4 — the coupled unison | the unison's strings coupled through one resistive bridge (Weinreich: the rank's bridge loss, the strings equal on the string side, the tonebar's gain and aftersound the tine side's only); the tuned detune 0.5 cents; the loss knee at 1 kHz; kBoardOut set again, the level table written again, the examples' gains measured again; the unison's tests rewritten; the corpus baseline and `make board` again | **done** |
| 4b — the bridge's reactive part, the loss law's form | report-4's answers: a reactive part on the bridge's admittance (no null between the prompt sound and the aftersound); the loss law's frequency term no longer scaled by the rank, the knee at 800 Hz (C8 rings 0.41 s); a secondary struck by its presence; rhodes's and rhodesBark's tonebar back at 1.5 cents; kBoardOut and the level table again; the corpus baseline and `make board` again | **done** |
| 5 — the attack | the owner's "plucked, not struck": the hammer's attack (`up_attack.h`), forces on the bridge on the string side: the knock of the impact, the key's thud on its bed and the string's free longitudinal modes (the clank, driven by the force squared, kLongOut set at grand's C2); `Tuning.attack` scales the three (0: stage 4b's bytes); the rank's longitudinal fundamental; the level table again; the corpus baseline and `make board` again | **done** |
| 5b — the attack made audible | report-5's findings answered: the knock is the blow's share of the hammer's force and the action's noise from the voice's rng, calibrated at grand's C4 (10 and 8 dB under the string over the first 10 ms); the thud calibrated on the band under 150 Hz; the clank's per-note physics (E A_core, μ, c_L in, E A_core / L and the bearing angle out, kLongShare set at grand's A0); `Tuning.attack` 0 .. 2; kBoardOut and the level table again; the corpus baseline and `make board` again | **done** |
| 6 — the knock without its noise, the board's field | the owner heard the attack as clicky and plucky, not struck: the action's noise removed, the knock the blow's share alone (the voice's rng kept, drawn by nothing); the field above the plate's modes, a feedback delay network of sixteen fixed lines beside a direct share of the shelf's mobility, its loss the plate's decay law (a one-pole a line, met at the top mode's t60 and at 4 kHz), its level the shelf's power; kLongShare and kBoardOut set again; the level table and the corpus baseline again; `make board` again | **done** |
| 7 — the channels | the owner's: the eight knobs and their level table removed (`up_knobs`, `pianoOf`, `knobsOf`, `levelDb`, `setKnobs`, `uptool level`), the twenty-four instrument channels the API and the presets tables of them, bit for bit the shapes they were; two channels for the tine, `tineT60` and `tineLoad`, read by the law in place of its constants; `nanShape()`; the tool's fit, random, print and chart over the channels; `02_Channels`; the examples' gains measured again; the corpus baseline and `make board` again | **done** |

- **The host build**: `make clean && make test` from the root green under
  `clang++ -std=c++17 -Wall -Wextra -Werror -O2 -fno-math-errno`:
  `148 tests, 1142 checks, 0 failures, 11 known`, about 42 s from clean
  (the run itself about 34). The eleven known rows are findings, not bugs
  (each under "Known limits"). `make tool` builds
  `extras/host/build/uptool`; `make corpus` writes
  `extras/host/build/corpus` (82 renders, a WAV and a PNG each, and
  `index.txt`), the same bytes on every run: the library's byte baseline.
- **The board compile** (`make board`: PlatformIO 6.2, teensy41, nothing
  flashed), every example with zero warnings in the library's files and the
  sketches' (the framework's own are not ours):

  | example | flash: code, data | RAM1: variables, code (free for locals) | the largest object in RAM |
  |---|---|---|---|
  | `01_Grand` | 66,248, 11,448 | 58,176, 62,216 (400,576) | `piano`, 43,944 |
  | `02_Channels` | 100,440, 17,592 | 73,472, 96,408 (352,512) | `piano`, 43,944 |
  | `03_Keyboard` (`-DUSB_MIDI_AUDIO_SERIAL`) | 76,880, 12,972 | 213,728, 73,560 (212,256) | `keyboard` (`AudioSynthUprightPool<8>`), 188,252 |
  | `99_Benchmark` (`-DUP_MAX_MODES=64`) | 76,496, 12,472 | 312,224, 73,176 (113,760) | `pool8`, 188,252; `pool4`, 105,180 |

  The benchmark built by hand at `-DUP_MAX_MODES=48` (the recipe reads one
  flag line a sketch): RAM1 variables 266,144 (`pool8` 158,556, `pool4`
  89,820). The plate's mode pairs, found once, are 256 bytes of RAM1 in
  every build. Each preset's shape a program reaches is a static of 1,132
  bytes in RAM1, built on first use: `02_Channels` and `03_Keyboard` reach
  all nine through the preset table, about 10 KB.
- **Hardware go / no-go: not yet run — needs a board.** `99_Benchmark`
  prints the processor usage of a pool of eight and of four playing grand
  and of the eight playing rhodes at the worst case (every partial, three
  strings, a chord of eight re-struck every two seconds, the strings gliding);
  the plan's estimate is about 55 % of the 600 MHz for eight acoustic voices
  of 64 partials and their board. The coupled unison adds to it: two sums
  and three multiply-adds a sample on each of the first 16 partials, 1.5 µs
  of a voice's block on the host (19 % of the 8.0 µs a ringing grand C4's
  block takes there; 228 bytes of RAM a voice). The attack adds its four
  longitudinal modes' turn and sum a sample and, for 12 ms after a strike,
  the blow's mean and the thud's phasor: 0.43 .. 0.49 µs of a ringing grand
  C4's block on the host (5.4 .. 6.4 % of its 7.7 .. 8.1 µs at attack 0;
  the attack's own block 0.72 .. 0.75 µs; 428 bytes of RAM a voice). The
  board's field adds its sixteen lines' reads, mix, one-poles and writes a
  sample: the board's block where nothing moves takes 5.4 µs on the host
  against the shelf's 3.6 (a voice's block 8.3 .. 8.7), once a pool and once a
  voice wrapper, and 10,668 bytes a board. A body move costs the board a
  rebuild: kBoardModes frequencies, decays, radiations and output weights
  from the mode pairs found once and the field's sixteen losses, and the
  moved modes' turns at its next block, 0.3 .. 0.4 µs on the host against
  8.3 .. 8.7 µs for a voice's block (the grid search it replaced, 39 µs,
  now runs once, at the first board's init). The voice wrapper rebuilds
  every block while its eased body moves after a write; the pool wrapper
  once per body write, inside its drain's mask.
- **By-ear passes: not yet done.** The listening WAVs of stages 1 and 2 are
  under `plans/upright-engine/out1/listen/` and `out2/listen/`, stage 4's
  before and after the coupling under `out4/listen/`, stage 5's grand at
  C2, C4 and C6 with and without the attack under `out5/listen/` (stage
  5's) and `out5b/listen/` (stage 5b's, and C4 at attack 2); stage 6's are
  the corpus's grand at 36, 60 and 84 at velocity 0.5 and 1
  (`extras/host/build/corpus`), and the owner heard the prototype of its two
  changes as scratch renders (C3, C4 and C5, outside the repository) and
  said go; the ear is the judge, the tests are the tripwire.
- **The chart** (`uptool chart`, 1637 cells in about 1.2 s): every
  partial table at a0, c4 and c7 finite, ordered, decaying, inside −1 .. 1
  and off over the ceiling; every hammer leaving; the engine's contact
  scheme within 10 % of the truth (the simulator at 32 fs) in every cell but
  twelve, all at the far ends of the hardness and felt spans on the
  electric side (rhodes at hardness 2512 and 10000, wurlitzer at 3981 and
  10000, rhodes at felt 0.00354, ...); the largest peak force 1578 N
  (kForceCap 2000). `uptool chart --audio` renders the same 1-d sweeps (1512
  cells, 2 s each, about 55 s) and holds the table's measures beside the
  render's.

## On a Teensy

The library installs as any Arduino library (`library.properties`,
`library.json`; `#include <Upright.h>`). One piano, a note every two
seconds, grand's shape with its felt's stiffness on a pot:

```cpp
#include <Audio.h>
#include <Upright.h>

AudioSynthUpright    piano(1);                  // one voice and its board; its seed
AudioOutputI2S       i2s;
AudioConnection      left(piano, 0, i2s, 0), right(piano, 0, i2s, 1);
AudioControlSGTL5000 codec;

float hardness = 142.0f;                        // N/mm^p at c4: grand's felt

void setup() {
    AudioMemory(8);
    codec.enable();
    codec.volume(0.5f);
    up::Tuning t;
    t.outputGain = 4.0f;
    piano.setTuning(t);
    piano.setShape(up::grand());                // the preset's channels, written whole
}

void loop() {
    static uint32_t last = 0;
    static bool down = false;
    if (millis() - last >= 2000) {
        last = millis();
        piano.noteOn(48.0f, 0.8f);              // a hammer at the next block
        down = true;
    }
    if (down && millis() - last >= 1000) {
        piano.noteOff();                        // the damper lands
        down = false;
    }
    const float h = 25.0f * powf(32.0f, (float)analogRead(A0) / 1023.0f);   // 25 .. 800 N/mm^p, log
    if (fabsf(h - hardness) > 0.01f * hardness) {
        hardness = h;
        piano.set(up::Channel::hardness, hardness);   // the sounding hammer eases there
    }
}
```

A keyboard is `AudioSynthUprightPool<M>`: `noteOn(midi, vel)`,
`noteOff(midi)`, `pedal(p)`, `allNotesOff()` and the pool-wide writes
(`setShape`, `set`, `setRate`, `setPartial`, `setPartials`, `setTuning`).
The examples:

| example | what |
|---|---|
| `01_Grand` | one voice, grand's shape, a note every two seconds alternating C3 and C5 at velocity 0.8, released after a second; a pot on A0 writes the `hardness` channel with `set`, log from 25 to 800 N/mm^p (Chaigne & Askenfelt's C2 hammer to their C6); the output gain 4 (its largest peak 0.134 at a gain of 1: 0.54) |
| `02_Channels` | one voice set by its channels from the serial monitor: `preset NAME` (written whole), `set NAME VALUE` (one channel, its range printed), `tine T60 LOAD` (a `nanShape()` with the generator at 0 and the tine's two channels: `tine 2 1` pings, `tine 30 1.6` rings long, stretched), `n MIDI VEL`, `o`, `p AMOUNT`, `print` (the channels as `--set` lines), `ladder`; the rank's line (L, d, B, the hammer's mass, K and p) on each change; the output gain 1.2 (every preset at C2 .. C6 at most 0.71 at a gain of 1: 0.85) |
| `03_Keyboard` | `AudioSynthUprightPool<8>` on usbMIDI: notes, CC 64 the sustain pedal, a program change a preset written with `setShape`, CC 123 all notes off; the output gain 0.4 (rhodesBark's eight keys in one block 2.27 at a gain of 1) |
| `99_Benchmark` | a pool of eight and of four at the worst case, grand and rhodes, the strings (length and bass) gliding from the spinet's to the concert grand's and back over 20 s, written fifty times a second with `setShape` of a `nanShape()` holding the two; the processor usage every second |

Every call lands at the top of the next audio block (128 samples, 2.9 ms),
a strike too. The wrappers set flush-to-zero for the audio interrupt at
construction; allocate them statically and never destroy them, as every
`AudioStream`.

## The whole public surface

One voice is one key's strings (`up_voice.h`). Every call writes channel
targets (or the tuning, or a strike, or the key's state) and lands at the
top of the next block; the last writer wins, per channel, in the caller's
order.

```cpp
// the player
void noteOn(float midi, float vel01);     // the note, and a hammer at the next block
void noteOff();                           // the key released: the damper lands (unless the pedal holds it, or the key has none)
void setPitch(float midi);  void setGlide(float perSec);
void setPedal(float p01);                 // the sustain pedal, eased at ratePedal
// the instrument: every channel, by name
void  set(Channel c, float target);  void setRate(Channel c, float perSec);
void  setShape(const Shape& s);           // every target at once; a nan entry is no write
Shape shape() const;                      // the targets as held
// conveniences that write channels
void setStrings(float length, float bass, float tension, float gauge);
void setUnison(float strings, float detune);
void setHammer(float hardness, float exponent, float felt, float massG);
void setStrike(float share);
void setBody(float area, float loss, float open);
void setGenerator(float g, float gap, float offset);
void setDamper(float rate, float top);
void setPartial(int k, float ratio, float t60, float gain, float out);   // one partial by hand, under one sequence
void setPartials(const PartialTable& t);  // a measured string, whole (a nan skips), under one sequence
// who is playing
void setTuning(const Tuning& t);  Tuning tuning() const;
// readback
State state() const;                      // the eased instrument, f0, the rank, every partial's amplitude (the attack's too), the hammer, the levels, asleep
int   partials(PartialTable& out) const;  // the eased table
int   stringShape(float* y, int n) const; // the string's displacement (mm) at n points from the agraffe to the bridge
float stringAt(float share) const;        // the same at one share of its length
RankState rank() const;
const Strings& strings() const;           // the strings as they stand: the banks, the hammer (the host's window)
const Attack&  attack() const;            // the attack as it stands: its longitudinal modes, its last sample's parts (the host's window)
// the engine: the pair a pool mixes, each at its generator weight, x outputGain, not clamped:
// the bridge force (N, the attack's in it) and the pickup's voltage; either may be null
void render(float* bridge, float* pickup, int n);
// the handoff (the wrapper's): attach(Pending*), latch()
// for a pool: reseed(seed), stop(), asleep(), energy(), released(), bridgePoint()
```

The channels and the presets (`up_channels.h`, `up_presets.h`; The
channels, below): `nanShape()`, every channel nan, a shape to fill in by
hand in which only the channels written move; `defaultShape()`, grand's
with its partials at middle C, every voice's start; `channelRange(c, lo,
hi)`, `channelName(c)`, `channelByName(name, c)`, `groupOf(c)`;
`grand()` .. `wurlitzer()`, each preset's shape built once on first use
from its table, and `presetCount()`, `presetAt(i)`, `presetByName(name)`
over the table of `PresetInfo { name, shape, lo, hi }` (`shape` the
preset's accessor, called as `p.shape()`).

A partial written by hand stands until the instrument is written again:
the law writes every partial under the sequence of the newest instrument
write, so an instrument write after a hand's (or a table's) rewrites it.
Two `noteOn`s in one block are the second. A strike while the hammer still
touches places a fresh one; a key struck while its string sounds meets the
moving string.

The board (`up_board.h`): `Board::init(fs)`, `setPlate(area, loss,
open)` (the eased body channels, recomputed where they moved),
`inputWeights(midi, w)` (the plate's shape where the note's string crosses
the bridge), `render(forces, w, voices, out, n)` (each voice's bridge force
in at its weights, the plate's modes turned, their radiated velocity and the
direct share and the field above them written), `setShelf(gain)` (the
shelf's mobility, scaling the direct share and the field: the tool's
sweep), `fieldStep(force)` (one sample of the field alone, no highpass: the
tool's and the tests' measure), `fieldLine(i, g, a)` (a line's loss),
`amplitude(k)`, `modeInfo(k)`, `shape(x, y)`, `table()`. A
single voice's player (the tool, the voice wrapper) runs a board of its own
on the voice's bridge force and adds the pickup: the output, not clamped
(the wrapper clamps what it sends). A pool runs one board on all its
voices.

A keyboard (`up_pool.h`): `Pool<M>`, the voices, their note table and one
board inside it.

```cpp
explicit Pool<M>(uint32_t seed = 1, float fs = kDefaultFs);
// the player
void noteOn(float midi, float vel01);     // the key's own voice struck again; else a free one; else the quietest, stolen
void noteOff(float midi);                 // the key released: its damper lands (unless the pedal holds it)
void pedal(float p01);                    // the sustain pedal on every voice, and on every note that lands
void allNotesOff();
// every voice now, and later notes
void set(Channel c, float target);  void setRate(Channel c, float perSec);
void setShape(const Shape& s);            // a nan entry is no write; the body sets the board
void setPartial(int k, float ratio, float t60, float gain, float out);  void setPartials(const PartialTable& t);
void setTuning(const Tuning& t);
// readback
int voices() const;  int sounding() const;  const Voice& voice(int i) const;  const PoolSlot& slot(int i) const;
Tuning tuning() const;  const Board& board() const;  static uint32_t seedOf(uint32_t seed, int i);
// the engine: the board on every voice's bridge force, the pickups added, each at its level, not clamped
void render(float* out, int n);
```

On a Teensy (`AudioSynthUpright.h`, `AudioSynthUprightPool.h`, each whole
inside `#if defined(ARDUINO)`): `AudioSynthUpright` is the voice's surface
one for one (the player, the channels, the conveniences, the tuning, and
`state()`, `shape()`, `partials()`, `stringShape()`, `rank()` read back),
an `AudioStream` with no inputs and one output, the voice's bridge force
through a board of its own with its pickup added. `AudioSynthUprightPool<M>`
is the pool's player and pool-wide writes, each returning false only when
its command was dropped, and `voices()`, `state(i)`, `slot(i)`,
`sounding()`, `dropped()`.

`State` (`up_telemetry.h`): the eased channels, f0, the rank, the main
string's partial amplitudes |z_k| (mm) and the secondaries', the hammer's
height, velocity, the block's peak force, touching, the strike's contact
so far; the block's bridge rms (N), pickup rms and the pair's rms; the
loudest partial, the partials on; released, damped, the eased pedal;
asleep.

## How it works

One block, in order (`up_voice.h` carries it whole):

1. the tuning, when one was set;
2. the law: when an instrument channel was written since its last run, or
   the note's target moved, every partial channel's target under the
   newest instrument write's sequence, from the targets at the target note;
3. wake: a sleeping voice with a strike queued wakes, every channel on its
   target, the banks zeroed, the note jumping; asleep with none, nothing;
4. the channels tick (awake);
5. the pitch axis (`up_pitch.h`): the eased note at the glide → f0;
6. the rank at the eased note (`rankAt`, when it or a channel it reads
   moved);
7. the hammer: the eased felt channels at the rank's hardness scale and
   power, the tine's tip by the generator, the rank's mass (the felt's
   table rebuilt only where they moved);
8. per slot: hz = ratio × f0; on under `ceiling × fs` with a gain or an out
   and a T60; its own decay σ_n = 6.907755 / (t60 × t60Scale), less the
   bridge's σ_b on the first kUnisonModes slots (held at a tenth of σ_n at
   least), and, released on a key with a damper, the damper's rate
   (`dampRateOf`, × (1 − the eased pedal)); the drive scale gain × 2 dtSub
   / (μ L ω) × 1000; the readback weight the gain; the output weight the
   out. Then the secondaries at the detune, the coupling at σ_b; the
   hammer's point mass (μ L / 2) / Σ gain²; every bank's `beginBlock`;
9. a strike due: the hammer placed at the string at `hammerScale ×
   vel^velCurve`;
10. the pair's weights, ramped across the block: the bridge's clamp(−g, 0,
    1) × the rank's bridge scale / 1000 × outputGain, the pickup's clamp(1
    + g, 0, 1) × outputGain; a weight at 0 at both ends compiles its sums
    out;
11. the attack (`up_attack.h`) where the bridge is heard: its longitudinal
    modes set at the rank's string, a strike due arming its thud at the
    rank's hammer mass × the throw (the blow's share follows the
    hammer's force), its weight clamp(−g, 0, 1) × outputGain ramped beside
    the bridge's; with nothing to add it is compiled out;
12. 128 samples: while the hammer touches, kContactRate substeps a sample
    of the hammer's loop; from the sample it leaves, every bank's turn;
    the bridge force with the attack's added, and the pickup's curve and
    pole on the tip's sums;
13. the telemetry; the silence gate: every string bank's energy under
    `silence` (mm²) at the end of a block with no hammer touching and no
    strike queued zeroes the banks and the attack and sleeps.

Control runs every 128 samples whatever size `render()` is called with, so
the same calls give the same bytes at any render size. No heap, no libm per
sample (`config_core_guard` reads the sources and names every function
allowed it). One seeded xorshift32 a voice (the constructor's seed,
`reseed()`), drawn by nothing in the engine: any seed renders the same
bytes.

**Units.** A string's partial is a phasor in millimetres of modal
displacement (`re`), its quadrature `im` (ω im its velocity in mm/s): a
fortissimo string swings O(1) mm, and the silence gate's 1e-9 mm² is −90
dB of a millimetre. The hammer's step is SI; the voice turns the readback
mm → m going in. The bridge force is the rank's π T / L × Σ out re / 1000:
newtons, and the attack's force, already newtons, is added at clamp(−g, 0,
1) × outputGain. The pickup's tip is Σ out re (mm) and Σ out ω im (mm/s) over the
first kTineModes slots of every bank. The board's modes are micrometres and
it is heard as their velocity, µm/s.

**The bank** (`up_bank.h`): N damped rotation oscillators (a template:
the main string's kMaxModes, a secondary's kUnisonModes, the board's
kBoardModes), in the increment form z += z (ca − i cb), ca = r cos ω − 1
from expm1 and sin²(ω/2), cb = r sin ω, turning clockwise so a drive into
im pushes re positive; a second coefficient pair per partial at dt /
kContactRate for the hammer's substeps; no doublets. Per sample: the
readback Σ gain re and its velocity Σ gain ω im, the injection im += F
drive, the turn, the sums (the bridge's Σ out re over every partial, the
tip's Σ tip re and Σ tip ω im over the first kTineModes slots in the same
pass, tip the out held to ±1: a pickup reads the tip's displacement, whose
modal weights are bounded by the shape's 1, where the bridge's slope grows
with n); the output weights ramp across the block, the tip's beside them.
A partial that moves grows in neither output: one ringing since the last
block whose frequency moved has its phasor scaled by min(1, hz_old /
hz_new) (re and im, one division). Moved up it keeps its velocity ω |z|
(the pickup's voltage and the board's modes read velocity), moved down its
displacement |z| (the bridge force reads displacement), so no output grows
through a glide or a key jump either way, as no piano gets louder when a
string is retuned; one moved from 220 to 880 Hz reads |z| 0.25 and its
velocity unchanged to 1e-7, one from 220 to 110 Hz keeps |z| and its
velocity halves, and the board's modes take the same rule as the body
channels move. One partial at 440 Hz, 27.5 Hz and 15 kHz rings within
0.003 cent of its frequency and 0.1 % of its T60.

**The hammer's loop** (`up_string.h`, `up_hammer.h`): while the hammer
touches, every sample is kContactRate (2) substeps of { the main string's
readback (y and its velocity, as the phasors stand); the force; every
string's partials take it; every bank turns by the substep's pair }, then
the sums. The force is K c^p by the felt's table, its loss solved against
the string's point mass under the hammer: F = a (1 + λ u) / (1 + λ a dt /
(2 m_point)), u the hammer's velocity less the string point's own, a the
table's K c^p: one division, stable at any step and second order. Against
the simulator at 32 fs (`up_contact.h`, double, explicit, powf; 32 fs
against 64 fs within 0.08 %), grand through the voice at velocity 1: C4's
force within 0.74 % of its peak at every substep, its contact 0.38 samples
short, its first ten partials within 0.19 dB; C2, C6, C8 within 8.9 % on
the peak (C8's, under the treble's stiffer felt) and 0.5 samples on the
contact. At C4 the ladder 0.1 .. 1 (0.07 .. 7 m/s) reads contacts 4.22 ..
2.10 ms and peaks 0.20 .. 29.7 N (Askenfelt & Jansson's C4: 3.0 ms at 0.6
m/s, 1.84 at 7). A re-strike meets the moving
string: the hammer placed at the rest, or where the string stands when it
is nearer.

**The attack** (`up_attack.h`): the sounds a hammer makes that are not the
string's partials, each a force in newtons on the bridge on the string
side at its own weight (clamp(−g, 0, 1) × outputGain, ramped beside the
bridge's), so it reaches the board as the string's force does (its modes
through the note's bridge point, the direct share and the field), and the
pickup never hears it. Each is calibrated at grand's C4, velocity 1, as its
own output through the board against the string's own over the same
window. The knock is the blow's share alone, kKnockForce (0.983) × the
hammer's force while it touches (the reaction through the shank, the rail
and the frame: the hammer's own pulse at the felt's own velocity
dependence): 10 dB under the string over the first 10 ms (−10.13 dB
measured; −6.96 at C2, −8.86 at C6). With the thud and the clank the
attack's own reads −9.99 dB there (held within 1.5 dB of it; −6.30 at C2,
−8.89 at C6), and from velocity 1 to 0.5 the knock falls 12.63 dB at C4
where the string's own falls 12.44 (C2 12.16 against 11.75; C6 12.44
against 14.37).
The thud, the key reaching its bed (Askenfelt & Jansson): a raised cosine
10 ms wide from 2 ms, its peak kThud (6.75) × m_h v0 over its width (m_h
the rank's hammer mass, v0 the throw); the board's band under 150 Hz over
5 .. 40 ms reads 4.7 dB over attack 0 at C4 (the thud alone 8.9 dB
over the string's own there, the blow alone 7.0: two pushes 2 ms apart,
partly cancelling in the plate's lowest modes). The clank: a string's
longitudinal waves travel at c_L = sqrt(E A_core / μ) (5048 m/s on a plain
wire; a winding adds mass, not stiffness), so its free longitudinal modes
sit at k c_L / (2 L), k times the rank's `longHz` (685 Hz at A0, 1208 at
C2, 4110 at C4, 14.6 kHz at C6, over the ceiling from C7), ringing 0.4 s
(kLongT60), kLongModes (4) of them. The string's stretching, (E A / 2) ∂/∂x
(∂y/∂x)² with the slope F / (2 T) under the hammer over a share kLongShare
of L about it, projected on mode k (modal mass μ L / 2, ω_k = k π c_L / L),
puts F² cos(k π β) E A_core kLongShare dtSub / (4 T² μ c_L) into its im a
substep (the k and the L cancel), and the bridge reads the longitudinal
force through the string's bearing angle, kLongAngle (0.08) × E A_core (π
/ L) Σ k (−1)^k re_k; kLongShare (0.0912, set on the field's board: 141)
sets grand's A0 at velocity 1 to read its clank 10 dB under the note over
the first 50 ms. Against the note
there it reads −10.0 dB at A0, −41.5 at C2, −46.5 at C4, −32.6 at C6
(−25.6, −58.7, −63.9, −56.9 at velocity 0.5), each at the rank's frequency
to 0.001 %: a blow twice as hard drives it four times as hard, and its
modes over the contact's bandwidth make it steeper still (from velocity
0.5 to 1 it rises 30.0 dB at C2 where the strongest partial rises 12.0).
`Tuning.attack` scales the three, 0 .. 2, 1 the calibration; at 0 nothing
of it runs and a voice renders stage 4b's bridge force. The phantom
partials (the forced longitudinal response at the transverse partials'
sums and differences) are not built.

**The unison** (`up_string.h`, after Weinreich's "Coupled piano strings",
JASA 62, 1977): the second and third strings carry the main's first
kUnisonModes slots, the second at 2^(+detune / 1200) and the third at
2^(−kThirdDetune × detune / 1200) (kThirdDetune 0.7: it never shares the
second's frequency), their own decay the main's and the damper's alike. On
the string side the three are equals at their presence (the strings
channel's fractions): each struck, coupled and heard by it, so a string
fading in is the same fraction of a string throughout. They end on one
bridge, an admittance Y (`bridgeMobility`, 3.5e-4 m/s per N) with a reactive
part `bridgeReactive` (0.5) of its resistive one: each string pushes it with
its impedance times its velocity, the bridge moves at Y times the sum, and
each string feels that motion. Per partial under kUnisonModes, before every
turn, S = im₁ + w₂ im₂ + w₃ im₃ (the velocities' sum), R = re₁ + w₂ re₂ +
w₃ re₃ (the displacements') and im_j −= w_j (c S − c_r R): c = 2 σ_b dt, σ_b
the rank's bridge loss T Y / L (0.12 /s at A0 and C2, 0.32 at C4, 1.12 at
C6, 4.0 at C8, the same on every partial), a symmetric damping matrix that
only dissipates; c_r = 2 δ dt, δ = σ_b × bridgeReactive, which lowers the
in-phase motion's pitch (a mass-like bridge: the strings' common motion
carries its mass). The voice hands the bank σ_i = σ_n − σ_b, so one string
alone decays at the law's σ_n; three in phase, as the hammer leaves them, at
σ_n + 2 σ_b, the prompt sound; the strings against each other at σ_i
alone, the aftersound, which barely moves the bridge and keeps the string's
own pitch. Two equal strings detuned by Δf (ε = π Δf), with κ = σ_b (1 + i
bridgeReactive), have their modes at σ_n ∓ sqrt(κ² − ε²): while ε is under
σ_b (551 σ_b / f0 cents: 0.66 at C4, 1.0 at C2, 2.4 at A0, 0.59 at C6, 0.52
at C8) they lock, two decays (its real part) and no beat; past it they beat
at |Im sqrt(κ² − ε²)| / π, under the free rate. On a purely resistive
bridge the hammer's symmetric strike put the locked slow mode at the bridge
in opposite phase to the fast one, so the sum fell through one null where
they crossed (85 dB down at 5.09 s for two strings at 0.3 cents at C4); the
reactive part shifts the in-phase mode's pitch and not the others' (n
strings in phase about n δ under the aftersound), so the two turn against
each other and never cancel. The detune in hertz grows with the partial and
the coupling does not: at the tuned 0.5 cents C4's fundamental locks and
its upper partials beat, slowly. Measured (`test_unison.cpp`, every
expected number from the constants): one string at C6 decays at −18.56 dB/s
against −18.56; three in phase at −38.03 against σ_n + 2 σ_b's −38.03; two
at 0.3 cents at C4 have no dip (the largest, each 10 ms frame against the
lower of the frames a second either side, −0.67 dB), −7.20 dB/s over the
first second against the fast decay's −6.84 and −1.89 from 10 s against the
slow's −1.81; two at 2 cents at C6 beat 1.1747 Hz against |Im sqrt(κ² −
ε²)| / π's 1.1725, 3.2 % under the free 1.2113 (a resistive bridge's
1.1576); grand's three at C4 pass from the prompt sound into the
aftersound at about 2.8 s without a dip, the prompt sound at −10.48 dB/s
(σ_n + 2 σ_b's −9.80) and 0.63 cents under the aftersound's pitch (3 δ / 2π:
0.50), the aftersound from 10 s at −1.54 dB/s against σ_i's −1.59, its own
two modes beating once, 6 dB deep at 7.7 s; C4's 4th partial on two strings
beats 0.2919 Hz against the formula's 0.2909; C4's 8th partial is 60 dB
down by 1.9 s (its tracked t60 1.60 s). On the tine side (u = clamp(1 + g,
0, 1)) the secondaries' outs are × lerp(1, kUnisonGain, u) and their decays
over kAftersound^u, and nothing couples (σ_b and δ × clamp(−g, 0, 1)): the
tonebar, heard at half the tine and ringing 2.5× longer, beats with it at
the free rate (rhodes at E4, its tonebar at 1.51 cents: 0.28832 Hz apart
against the free 0.28833).

**The pickup** (`up_pickup.h`): the `Pickup` stage on the tip's sums (the
outs held to ±1), one Lorentzian whose centre is set per block (the magnet's offset at the tine, slid to the gap
by clamp(g, 0, 1) toward the reed), its derivative × x' with one division a
sample, the amp's pole, × kPickupGain (1.36e-4: rhodes at E4, velocity 1,
peaks 0.29). Rhodes at E4 through the voice is the simulator's tip through
the same curve within 0.06 % rms.

**The board** (`up_board.h`): a Bank of the plate's 32 modes (which (m, n)
are the lowest does not depend on the area at the fixed aspect, every
frequency scaling by 1 / area, so the 40 × 40 search runs once and a body
move evaluates the 32 frequencies from the pairs it found), each voice's
bridge force in at its bridge point, drive dt / (M ω) × 10⁶ (µm; M = ρ h a
b / 4); heard as the modes' velocity, Σ outW ω im (µm/s; outW the radiation
× the listening shape). Above the top mode (f_top, 764 Hz on grand's board) the
voices' summed force goes through two one-pole highpasses at f_top into
two paths beside the modes. The direct share is that force × kFieldDirect
(0.316, −10 dB) of the shelf's mobility kBoardShelf (3133 µm/s per N; the
infinite plate's mobility 1 / (8 sqrt(H ρ h)) is 3604): the spreading
wave's first arrival. The field is the plate's diffuse response where its
modes above the top one overlap, each ringing t60 = 6.907755 / (π f η): a
feedback delay network of kFieldLines (16) lines, 59 .. 281 samples long
(1.3 .. 6.4 ms at 44.1 kHz), 2602 in all end to end in one buffer: one
network mode per 17 Hz, near the plate's own density at 1 m² ((a b / 2)
sqrt(ρ h / H): one per 19 Hz). Each sample the lines' outputs d_i are mixed by
Householder, y_i = d_i − (2 / N) Σ d_j (orthogonal: lossless), each y_i
goes through its line's loss and is written back with the force added at
1; the field is the d_i summed at +1, −1, +1, ... × its gain. The loss is a
one-pole in each line's loop, g_i (1 − a_i) / (1 − a_i z⁻¹), set per plate
rebuild in closed form from η and f_top: g_i, its gain at dc, the pass's
loss at the top mode's t60, 10^(−3 L_i / (t60(f_top) fs)) (under 1: stable), and
a_i so its loss at kFieldFitHz (4 kHz) is the pass's at the plate's t60
there. The gain is kFieldGain (715.8 µm/s per N) × the shelf's mobility /
kBoardShelf, the same on every board: kFieldGain is measured so the
field's impulse energy (a unit impulse in, no highpass, 0.5 s) on grand's
board is (1 − kFieldDirect²) kBoardShelf², 8.836e6 (µm/s per N)², and the
field and the direct share together carry the power the shelf carried (a
flat gain G has Σ h² = G²); on the spinet's board and the concert grand's
the energy reads −0.25 and +0.64 dB from it. The output is the sum × kBoardOut (1.235e-6: grand's octave C4 .. B4 at
velocity 1, each note 0.6 s, its first half second's rms, the mean of the
twelve powers, is 0.0897 rms of full scale, −20.95 dB; grand's C4 alone
peaks 0.18). Measured
(`test_board.cpp`): 1 N for one sample at C4's bridge point on grand's board puts
7.9 % of the first 300 ms's energy in the first sample and 75.1 % within
5 ms (a flat shelf there: 81.5 % and 96.4 %); the board's power over f_top
.. 2 f_top, 2 .. 4 and 4 .. 8 f_top against a flat shelf's at kBoardShelf
reads +0.88, −0.72, +0.03 dB on the spinet's board, +0.18, +1.13, +0.28
on grand's and +3.61, +3.21, +0.70 on the concert grand's (the tilt, two
known rows), the dB's standard
deviation 4 .. 7 dB in each band (the grain); every line's loss meets the
plate's per-pass loss at dc and at 4 kHz within 1.1e-6, and between them
the lines' mean t60 at 1.5 f_top is 0.69, 0.76 and 0.97 of the plate's on
the spinet's, grand's and the concert grand's boards (a one-pole falls
faster than 1 / f); 100 N rms of white noise for half a second, on every
board and at 22.05, 44.1 and 96 kHz,
leaves every sample finite and the output under −100 dBFS within 0.29 s
of its end. The field holds 10,668 bytes (the lines' 2602 floats, each
line's read point, loss and state, the gain): a Board is 14,184 bytes
(3,516 with the shelf alone, stage 5b), `Pool<8>` 182,160; the board's
block where nothing moves takes 5.4 µs on the host against the shelf's
3.6. Through it grand at velocity 1 peaks 0.33 at C2, 0.18 at C4, 0.19 at
C6, and its C4 shows every partial to 4 kHz as a clean line, those over 2
kHz gone within about two seconds, the lowest through the 4 s. A mode
alone (the shelf's mobility 0, so neither the direct share nor the field)
rings within 0.0001 cent and 0.2 % of its table; two voices are the sum of
each; a closed back is 15.8 dB under an open one at 60 Hz.

**The pool** (`up_pool.h`, below) runs once per 128 samples too: its own
block (the stolen voices, the frees, the levels), then every voice's pair
for every sample into scratch at its level, each bridge force into the one
board at its bridge point's weights, the board once, the pickups summed.

**On a Teensy** each wrapper's `update()` runs in the audio interrupt: the
voice wrapper latches its pending table with interrupts masked and renders
unmasked, its board on the bridge force (the plate from the voice's eased
body when it moved, the weights at its bridge point when the note moved),
the pickup added: the plate is rebuilt every block while the eased body
moves after a write (seconds at `rateBody`), each rebuild the 32 frequencies
from the mode pairs found once, the field's sixteen losses and the moved
modes' turns, 0.03 .. 0.04 of a voice's block on the host. The pool wrapper
drains its command queue masked (the plate rebuilt once per body write
there) and renders unmasked. Each clamps what it sends, scales it to int16 and
transmits it on output 0, and renders the block even when `allocate()`
returns NULL, so the voices and the board keep time.

## The modules

| | |
|---|---|
| `src/Upright.h` | the umbrella: the core, and on a Teensy the two wrappers |
| `src/AudioSynthUpright.{h,cpp}` | one voice and its board, one output: the pending table under an interrupt mask |
| `src/AudioSynthUprightPool.h` | the pool, a template on M, header-only: the command queue and its payload rings |
| `src/core/up_config.h` | the build's sizes, clamp, the one smoothing law, `Ramp`, `Arc`, xorshift32, `midiToHz` |
| `src/core/up_tuning.{h,cpp}` | `up::Tuning` and `clamped()` |
| `src/core/up_channels.{h,cpp}` + `up_modes.inc` | the channel table, its groups and ranges, `Shape`, `PartialTable`, the partial blocks, `nanShape`, `defaultShape` |
| `src/core/up_law.{h,cpp}` | the law: the string, the tine, the reed, the path between, the damper's rate, the hammer's tip, the constants |
| `src/core/up_rank.{h,cpp}` | the rank: the string at a note, `RankState` |
| `src/core/up_pitch.{h,cpp}` | the pitch axis: the note and its glide |
| `src/core/up_bank.{h,cpp}` | the rotation oscillators: the drive, the readback, the bridge's and the tip's sums |
| `src/core/up_hammer.{h,cpp}` | the felt's law and its table, the contact step |
| `src/core/up_string.{h,cpp}` | a note's main and secondary strings, their coupling through the bridge, and the hammer's loop over them |
| `src/core/up_attack.{h,cpp}` | the hammer's attack: the knock, the thud and the longitudinal clank |
| `src/core/up_pickup.{h,cpp}` | the curves, the derivative, the amp's pole |
| `src/core/up_board.{h,cpp}` | the plate's law and its bank, the direct share and the field |
| `src/core/up_telemetry.{h,cpp}` | `State`, the string's shape for the picture |
| `src/core/up_voice.{h,cpp}` | the voice: the block, the handoff, the pool's hooks |
| `src/core/up_presets.{h,cpp}` | the nine presets, each a table of the twenty-four instrument channels |
| `src/core/up_pool.{h,cpp}` | the pool: the note table, the steal, pool-wide writes, the shared board |
| `examples/` | `01_Grand`, `02_Channels`, `03_Keyboard`, `99_Benchmark` |
| `extras/host/` | the Makefile, `tests/`, `uptool.cpp`, `up_wav`, `up_png`, `up_analysis.h`, `up_play.h`, `up_fit.h`, `up_contact.h` |
| `extras/reference/` | the reference tables, plain text, with `README.txt` |
| `README.md`, `LICENSE` (MIT), `NOTICE`, `library.properties`, `library.json`, `keywords.txt` | |

`src/core/` includes nothing but the C++ standard library and its own
files, holds nothing on the heap, and calls no libm per sample (the test
`config_core_guard` reads its sources). The wrappers are the only files
that include anything of Arduino's.

## The channels

A piano is its twenty-four instrument channels (`up_channels.h`), in five
groups, each a target, in physical units where it has them, that the
voice eases toward at its group's rate; the law writes every partial from them at the note (The
law). Each channel's range is `channelRange`'s: what a control can
sensibly ask for, and what the tests drive it between. The tool draws,
sweeps and fits each along it, on a log scale where it spans a decade or
more (`channelSpan`, `up_fit.h`).

| channel | group | what | unit | range | grand |
|---|---|---|---|---|---|
| `length` | strings | the speaking length at C4 | m | 0.1 .. 2 | 0.614 |
| `bass` | strings | the longest string the case holds | m | 0.3 .. 4 | 1.61375 |
| `tension` | strings | the design tension | N | 100 .. 3000 | 553 |
| `gauge` | strings | the wire's gauge, 1 as designed | | 0.25 .. 4 | 1 |
| `strings` | strings | how many strings a note has | | 1 .. 3 | 3 |
| `detune` | strings | the unison's spread | cents | 0 .. 100 | 0.5 |
| `strikeShare` | strings | where the hammer lands at C4 | a share of the length | 0 .. 0.5 | 0.12 |
| `lossRate` | strings | the string's own loss at C4 | 1/s | 0.01 .. 50 | 0.45 |
| `lossSlope` | strings | the loss's power in frequency | | 0 .. 4 | 2 |
| `hardness` | hammer | the felt's stiffness at C4: the force at 1 mm | N/mm^p | 1 .. 10000 | 142 |
| `exponent` | hammer | the felt's power p | | 2 .. 3.5 | 2.5 |
| `felt` | hammer | the felt's velocity loss | s/mm | 0 .. 0.005 | 0.0005 |
| `hammerMass` | hammer | the hammer's mass at C4 | g | 0.5 .. 50 | 2.97 |
| `boardArea` | body | the soundboard's area | m² | 0.1 .. 4 | 1 |
| `boardLoss` | body | the board's loss factor | | 0.001 .. 0.3 | 0.03 |
| `open` | body | the back, closed 0 to open 1 | | 0 .. 1 | 0.5 |
| `generator` | generator | string −1, tine 0, reed +1 | | −1 .. 1 | −1 |
| `gap` | generator | the pickup's gap | mm | 0.1 .. 10 | 1.5 |
| `offset` | generator | the tip's offset off the pickup's axis | mm | 0 .. 5 | 0.9 |
| `tonebar` | generator | the tonebar's weight | | 0 .. 1 | 0 |
| `tineT60` | generator | the tine's fundamental t60 at C4 | s | 0.05 .. 100 | 10.4 |
| `tineLoad` | generator | the beam's overtone ratios (n ≥ 2) × this: the tuning spring's mass at the tip | | 0.5 .. 4 | 1.18 |
| `dampRate` | damper | the decay rate a released key's damper adds at C4 | 1/s | 0 .. 500 | 20 |
| `damperTop` | damper | the highest key with a damper | midi | 0 .. 127 | 89 |

**The rates** (`up::Tuning`, `groupOf`, `groupRate`): the strings group
eases at `rateStrings` (8 /s), the hammer at `rateHammer` (8), the body,
the generator and the damper at `rateBody` (4: the instrument's slow
things), every partial channel at `rateModes` (8), the note at `ratePitch`
(0: a jump, the glide channel easing it) and the pedal at `ratePedal` (30).
A rate at or under zero jumps. `setRate(channel, perSec)` sets one
channel's rate, and the tuning's group rates stand on every channel whose
rate nobody set.

**Setting a piano**, the same calls on `up::Voice`, `AudioSynthUpright`,
`up::Pool<M>` and `AudioSynthUprightPool<M>`:

```cpp
piano.setShape(up::grand());                          // a preset's shape, written whole under one sequence number
piano.setShape(up::presetByName("rhodes")->shape());
piano.set(up::Channel::hardness, 300.0f);             // one target
up::Shape s = up::nanShape();                         // every channel nan: no write
s[up::Channel::tineT60] = 30.0f;
s[up::Channel::tineLoad] = 1.6f;
piano.setShape(s);                                    // only the channels written move
piano.setRate(up::Channel::boardArea, 0.5f);          // one channel's ease, per second
piano.setTuning(t);                                   // the group rates, and who is playing (Tuning)
```

A voice and its wrapper also take the conveniences that write a group's
channels together: `setStrings(length, bass, tension, gauge)`,
`setUnison(strings, detune)`, `setHammer(hardness, exponent, felt, massG)`,
`setStrike(share)`, `setBody(area, loss, open)`, `setGenerator(g, gap,
offset)`, `setDamper(rate, top)`.

**The rules.** A nan is no write: a shape's nan channel stands where it was
(a preset's partial and player channels are nan, so `setShape` of a preset
leaves the note, the glide and the pedal alone). The last writer wins per
channel by its sequence number: every write takes the next one, a shape
lands whole under one, and between the caller's table and the audio side's
the newer write wins channel by channel. A partial written by hand stands
until the instrument is written again: the law writes every partial under
the sequence of the newest instrument write. Every value inside
`channelRange` is legal.

**The tine's two channels.** `tineT60` is the tine's fundamental t60 at C4
(s): the overtones' t60s are tineT60 × ratio^−tineSlope (tineSlope 1.2),
the reed's decays kReedT60 (0.6) times the tine's, and every t60 scales
along the keyboard by the rank's t60Scale. `tineLoad` multiplies the beam's
overtone ratios, n ≥ 2: the tuning spring's mass at the tip, 1 the bare
beam (its 2nd partial at 6.27), 1.18 putting it at 7.39 (Shear's measured
7.4). Every preset, and so every fresh voice (`defaultShape`), carries
10.4 s (the fit against Shear's Q for a 1974 Mark I) and 1.18. A small tine
that pings is rhodes with tineT60 2 and tineLoad 1; a big one that rings
long with its overtones stretched, tineT60 30 and tineLoad 1.6
(`02_Channels`' `tine 2 1` and `tine 30 1.6`). Both ease at `rateBody` with
the generator group and are inert on the string (generator −1). A nan
tineT60 in a shape handed straight to `partialsOf` holds at 1 ms, and a nan
or non-positive tineLoad reads as the bare beam's 1.

**The floors, measured** (`tests/test_floors.cpp`): each group of channels
from one end of its reach to the other alone at grand (rhodes for the
pickup), every other channel at the preset's, rendered 4 s at velocity 1
as `play` renders it, the partials tracked at the law's frequencies:

| channels | from → to | the measure | the plan's floor | measured | |
|---|---|---|---|---|---|
| strings | the spinet's length and bass (0.601, 1.22 m) → a concert grand's (0.627, 2.13 m) | the 10th partial of C2, cents off 10 f0 | 30 cents (held at 15) | 16.8 cents (+20.9 → +4.1) | held at 15: the tenor's span is the data's, ±2 %, and C2 is wound on every one |
| hammer | hardness 25.1 N/mm^p, felt 1e-3 s/mm → 803, 2.5e-4 (Chaigne & Askenfelt's C2 hammer's stiffness to their C6's) | the force's band (partials 8 .. 16 against 1 .. 3, the render's partial levels) at velocity 1, at attack 0 | 6 dB (decided 3) | 4.3 dB (−23.1 → −18.8; 4.6 at attack 1) | held at 3 |
| hammer | the same | the same at velocity 0.5 | 12 dB (decided 10) | 12.2 dB (−33.0 → −20.8; 12.3 at attack 1) | held at 10, at attack 0: the string's own partials, without the attack's force in the tracker's first frame |
| strike | strikeShare 0.04 → 0.5 | the 8th partial, its band's power over 0.1 .. 1 s | 20 dB | 80.3 dB (−10.3 → −90.6: the node at the middle) | held |
| unison | one string → three 20 cents apart | the fundamental's beat depth (16 s) | 6 dB | 29.9 dB (0 → 29.9; the tuned three 4.2) | held |
| unison | one string → three tuned (0.5 cents) | the aftersound's ratio, early slope over late (16 s) | 2× | 5.60× (1.00 → 5.62; the wide three 0.24) | held to the tuned three: on the wide three the beats swamp the two-slope fit; read on 16 s, the aftersound's own slope from 10 s |
| board | the spinet's (0.62 m², loss 0.049, closed) → a concert grand's (1.62 m², 0.018, open) | the octave about 100 Hz through the board, at A0 | 8 dB | 11.4 dB (−37.6 → −26.2; at C2 16.2) | held |
| board | the same | the board's first mode (the table) | an octave | 1.40 octaves (81.2 → 30.8 Hz) | held |
| damper | dampRate 60.6 → 6.6 /s | the release's T60 (released at 1 s), read on one string | 8× | 8.70× (0.100 → 0.868 s; three strings 7.91×) | held on one string: the three strings' two decays tilt their fit |
| pickup | gap 3 mm, offset 1.5 → rhodesBark's 0.75, 0.3 | rhodes's E3: the second harmonic against the first | 20 dB | 24.2 dB (−29.4 → −5.2) | held |
| pickup | the same | rhodes's E3: the thd | under 3 %, over 15 % | 3.78 % → 61.15 % | held under 5 % at the far end (the tine and its tonebar swing the tip into the curve), over 15 % close |
| generator | grand → rhodes | the partials on at E3 | over 30 → 6 | 64 → 6 | held |

The chart's `floors.txt` reads the same floors from the law's tables and
the simulator across the same spans: the strings' 16.9 cents at C2 against
30 not met, the rest met.

## The law

`partialsOf(shape, rank)` (pure) and the writer `Law::update(channels,
rank, noteMoved)` (when an instrument channel is newer than its last call,
or the note moved, every partial channel under the sequence of the newest
instrument write; a partial written by hand later stands until the
instrument is written again). Once per instrument write and once per note,
never per sample.

**The slots** are the partials in order: slot k is partial k + 1 in every
build (`UP_MAX_MODES`, 8 .. 96, default 64). A build of 8 keeps the first
eight partials: a piano with no brightness.

**The string** (generator −1): ratios n sqrt(1 + B n²) with the rank's B;
decays sigma_n = lossRate / t60Scale + lossRate (f_n / lossKnee)^lossSlope
at the note's partial frequencies: the floor (the bridge and the air) is
what the rank scales, and the frequency term, the wire's own bending and
viscoelastic loss (Chabassier's b3 ω²), is nearly the same at every note
(his b3 moves 2.4× over seven octaves where t60Scale moves 23×), so it is
not scaled (the table holds 6.907755 / (lossRate (1 + t60Scale (f_n /
lossKnee)^lossSlope)) and the voice's × t60Scale lands sigma_n; the tine's
t60s and a table by hand keep the rank's scaling whole); gains sin(n pi share) at the rank's strike share, signed;
outs n (−1)^(n+1), the slope at the bridge read with the tip's sign,
dimensionless (the rank's bridgeScale, pi T / L, makes them newtons): a
string's out and the beam's share a sign in each of the first six slots.
Grand's c4: B 3.51e-4, the fundamental's t60 13.9 s (one string), its 2nd
partial's 10.7, 4th 5.6, 8th 1.92, 16th 0.50; the fundamental's at c2 37.7
s, c6 3.23, c7 1.26, c8 0.41, key 127's 52 ms.

**The tine** (generator 0): the clamped-free beam, ratios (beta_n / beta_1)²
(`tineRatio(n, load)`), the overtones (n ≥ 2) × the `tineLoad` channel (the
tuning spring's mass at the tip; at the presets' 1.18 the one measured
Rhodes overtone at 7.4, the bare beam's 6.27: 1 : 7.395 : 20.71 : 40.58 :
67.07 : 100.2) over the first kTineModes (6) slots, the slots above off (the
beam's series continued, (2n − 1) pi / 2, so the table stays ordered); a
nan or non-positive load the bare beam's 1; decays the `tineT60` channel ×
ratio^−tineSlope (held over 1 ms, a nan too); gains the beam's shape at the
strike share and outs its shape at the tip, both over the tip's 2.

**The reed** (generator +1): the tine's ratios and shapes, the overtones'
gains at kReedOvertone (0.05) of the beam's, decays kReedT60 (0.6) of the
tine's.

**Between the anchors**: from −1 to 0 every slot's ratio and t60 along the
geometric mean's path, the first six slots' gains and outs linearly, the
string's slots above six faded by clamp(−g, 0, 1); from 0 to +1 the
overtones' gains toward the reed's and the t60s toward the reed's. No gain
or out of the first six crosses zero on the way (along −1 .. 0 by 0.05 at
a0, c4 and c7). `stringsOf` caps the strings at 3 to g −0.5, falling to 2
at 0.

**The ceiling**: a partial over `tuning.ceiling × kDefaultFs` at the note
gets gain 0 and out 0 (off).

**The rank** (`rankAt(shape, midi)`): L = min(length (f_c4 / f0)^rankLength,
bass); mu = T / (4 L² f0²), Z = T / (2 L f0); the plain wire d = sqrt(4 mu /
(pi rho)) × gauge, wrapped past dMax on a core of `core` × (mu / muBreak)^
coreGrow (muBreak = pi rho dMax² / 4: the core thickens as the winding
grows); B = pi³ E dCore⁴ / (64 T L²); the hammer's mass hammerMass (f_c4 /
f0)^rankMass (4.2 g at c2, 2.97 at c4, 2.1 at c6), its felt's stiffness ×
(f0 / f_c4)^rankHardLo below c4 and ^rankHardHi from c4 up (Chaigne &
Askenfelt's three hammers bend at c4: 0.75 below, 1.41 above; K 26.2,
50.2, 142, 1003, 7081 N/mm^p at a0, c2, c4, c6, c8) and its power + rankP
log2(f0 / f_c4) (2.16, 2.5, 2.84), held 1.5 .. 4; the strike share
strikeShare (f_c4 / f0)^rankStrike (0.128 at a0, 0.120 at c4, 0.110 at c8);
every t60 × (f_c4 / f0)^rankLoss held under t60ScaleMax (2.5: a0's
fundamental 37 s); the string's loss into the bridge tension ×
bridgeMobility / L (0.12 /s at a0, 0.32 at c4, 1.12 at c6, 4.0 at c8: the
unison's coupling); no damper over damperTop; the free longitudinal
fundamental c_L / (2 L), c_L = sqrt(E π dCore² / 4 / mu) (a winding adds
mass, not stiffness: 685 Hz at a0, 1208 at c2, 4110 at c4, 14.6 kHz at
c6). At a fixed pitch and tension
the plain wire's B goes as 1 / L⁶ and a wound string's as 1 / L^(2 + 8
coreGrow) (a0: L^−3.38). The bass break on the spinet's strings, grand's
and a concert grand's: A#2, F2, C2 (a0's core 1.63, 1.48, 1.34 mm, its B
8.2e-4, 3.2e-4, 1.25e-4).

**The hammer** (`up_hammer.h`): F = K c^p (1 + lambda dc/dt) while the felt
is compressed, never negative, by the table c^p = sqrt(c)^w × g(c), w =
floor(2p), g(c) = c^(p − w / 2) from a 64-point table over 0 .. 4 mm
spaced in sqrt(c) (exact at the half powers, 0.26 % at the quarter);
symplectic Euler; the loss solved against the string's point (above);
the contact ends when the felt is free and the hammer moves back, or 20
ms after the strike; one step's force held at kForceCap 2000 N (the chart's
largest 1578 N). The tine's tip by the generator: hardness ×
2^(kTipHard (1 − |g|)), felt × 2^(−kTipLoss (1 − |g|)), kTipHard 2,
kTipLoss 1. `stepExact` is the explicit form by powf, the simulator's.

**The attack** (`up_attack.h`, above): the blow's share kKnockForce 0.983
of the hammer's force (calibrated at grand's C4); the thud kThud 6.75 over
kThudMs 10 ms from kThudDelayMs 2 ms (the key on its bed after the hammer
meets the string: Askenfelt & Jansson; the share calibrated on the band
under 150 Hz); kLongModes 4 (`up_config.h`) longitudinal modes ringing
kLongT60 0.4 s (Bank & Sujbert: a few hundred milliseconds; an estimate),
read through kLongAngle 0.08 (a typical bearing angle, 4.6°; physical),
driven over kLongShare 0.0912 of the length (calibrated at grand's A0
through the field).

**The pickup** (`up_pickup.h`): v = C'(x) x', x and x' the tip's
displacement and velocity (Σ tip re and Σ tip ω im over the first six
slots of every string, tip the partial's out held to ±1), C(x) = 1 / (1 +
((x − c) / gap)²), one Lorentzian whose centre c is the magnet's offset at the tine
and slides to the gap toward the reed, c = offset + clamp(g, 0, 1) (gap −
offset): at the reed the plate's curve, centred at the gap, where the
reed's rest a gap from the plate's plane puts the capacitance's peak.
Bounded everywhere, |C'| ≤ 3 sqrt(3) / (8 gap) = 0.65 / gap; a sine at gap
1.5 mm through the plate's curve reads 3.3, 6.6, 9.8, 15.7 % thd at 0.1,
0.2, 0.3, 0.5 mm. The amp's one pole with its −3 dB point at `ampHz`
exactly.

**The plate** (`up_board.h`): the orthotropic simply supported plate's
lowest 32 modes, kBoardH 9 mm, kBoardRho 400, kBoardEx 11 GPa, kBoardEy
2.75 GPa, H = sqrt(Dx Dy), kBoardAspect 0.477; decays pi f eta; radiation
(f / fc)² / (1 + (f / fc)²), fc = kFc0 2^(kFcSpan (1 − open)), kFc0 80 Hz,
kFcSpan 1.5. At 1 m² the first modes are 50.0, 65.3, 90.8 Hz (Suzuki's
Steinway L: 49.7, 76.5, 85.3), the 32nd 764 Hz. Above the top mode the
board is the direct share and the field (How it works): `kBoardShelf` 3133
µm/s per N, their scale (a flat mobility through the two highpasses that
averages over 2 .. 4 f_top what the swept transfer does over the top mode's
octave on grand's board; the infinite plate's 1 / (8 sqrt(H ρ h)), 3604, for
comparison); `kFieldDirect` 0.316 (−10 dB, set by ear); `kFieldLines` 16
of `kFieldLengths` 59 .. 281 samples, `kFieldSamples` 2602 in all;
`kFieldFitHz` 4 kHz (the one-pole's second point); `kFieldGain` 715.8
µm/s per N (measured by `uptool board` on grand's board).

**The damper's rate** (§9): `dampRateOf` = dampRate (1 + f / kDampKnee) (1 −
pedal), kDampKnee 2 kHz, 0 for a note over damperTop.

**The constants** (`LawConstants`, the ones a fit may move; `up_law.cpp`).
tension, loss, lossSlope, l0, bass0 and massC4 are also the law's and the
rank's fallbacks where a shape's channel (`tension`, `lossRate`,
`lossSlope`, `length`, `bass`, `hammerMass`) is nan; grand's channels hold
the same values.

| constant | value | from |
|---|---|---|
| tension, rankLength, dMax, core, coreGrow | 553 N, 0.915, 1.089 mm, 1.0 mm, 0.1725 | the measured B tables' fit |
| l0, bass0 | 0.614 m, 1.61375 m | the same: a grand's C4 speaking length and its longest string |
| loss, lossKnee, lossSlope, rankLoss, t60ScaleMax | 0.45 /s, 800 Hz, 2, 0.8, 2.5 | the plan's loss and slope (C4's fundamental 13.9 s, one string); the knee where the wire's own loss, not scaled by the rank, equals the floor: against Chaigne 2016's three measured strings (D#1 8.29 s, C4 4.84 s at 1 kHz, E6 1.38 s at its fundamental) the law reads 7.82, 5.99, 2.41 s (rms log2 0.50; stage 4's 0.87, stage 3b's 1.70); the cap a0's 37 s |
| bridgeMobility | 3.5e-4 m/s per N | the bridge's driving-point mobility, an estimate: Giordano's soundboard impedance 1 .. 3 × 10³ kg/s puts it 3e-4 .. 1e-3, the bridge stiffer than the board; grand's C4 string loses 0.315 /s into it, the plan's 0.45 floor less its internal loss. No table measures it: `fit --constants` names it and never moves it |
| bridgeReactive | 0.5 | the reactive part of the bridge's admittance over its resistive part, an estimate: enough that the prompt sound and the aftersound have turned 1.3 rad against each other where they cross at C4, so no null (a 45° bridge would be 1; a finite plate's driving point swings between spring-like and mass-like through its modes); its sign lowers the in-phase mode's pitch, checked by measurement. Named by the fit, never moved |
| massC4, rankMass | 2.97 g, 0.25 | Chaigne & Askenfelt's hammers |
| rankHardLo, rankHardHi, rankP, rankStrike | 0.75, 1.41, 0.17, 0.03 | Chaigne & Askenfelt's K below and above c4 and their p up the keyboard; Chabassier's and Chaigne's strike shares |
| tineSlope | 1.2 | the plan's |

## Tuning

`up::Tuning` (`up_tuning.h`) says who is playing; the channels are the piano.

| field | default | what |
|---|---|---|
| `rateStrings`, `rateHammer`, `rateBody`, `rateModes`, `ratePitch`, `ratePedal` | 8, 8, 4, 8, 0, 30 /s | each channel group's ease (0 a jump): the strings, the hammer, the body with the generator and the damper, the partials, the note, the pedal (The channels); `setRate` sets one channel's |
| `hammerScale`, `velCurve` | 7 m/s, 2 | the hammer's speed hammerScale × vel^velCurve: velocity 0.3 is 0.63 m/s (a pianissimo), 0.5 1.75, 0.7 3.4, 1 7 (Askenfelt & Jansson's p 1, mf 2.6, f 5.2, ff 8.6 bracket it) |
| `outputGain` | 1 | the voice's pair to full scale, before the clamp |
| `silence` | 1e-9 mm² | every bank's summed re² + im² under which a voice with nothing pending sleeps (−90 dB of a mm): grand at C4 sleeps 49 s after one strike, C2 151 s, C6 8.6 s, C7 2.4 s |
| `stealRate` | 60 /s | the pool's fade of a stolen voice, by the one law: under 1 % in 26 blocks, to zero in one more, the new note on it 78 ms after its key |
| `ceiling` | 0.45 | a partial over this share of fs is off; held 0.01 .. 0.5 |
| `ampHz` | 6000 Hz | the pickup amp's pole |
| `attack` | 1 | the hammer's attack, the knock, the thud and the clank together (`up_attack.h`), held 0 .. 2: 0 is none (stage 4b's bridge force), 1 the calibration (grand's C4: the attack's own 10 dB under the string over its first 10 ms, the thud 4 dB over the band under 150 Hz; grand's A0: the clank 10 dB under the note), 2 twice it |

`clamped()` holds what would end the voice: a nan falls back to the
default, the ceiling inside its span, the hammer's throw and the pole over
0, the attack inside 0 .. 2. The bank holds a decay slower than a T60 of 1000 s (`kMaxT60`) at 1000
s, so no rounding lets a partial grow.

## The presets

Nine tables of the twenty-four instrument channels, each with a playing
range (`up_presets.h`; `up_presets.cpp` carries each table's provenance in
channel terms): a preset's shape is its table, built once on first use
(the partial and player channels nan, so the law writes the partials at
the note and `setShape` leaves the note, the glide and the pedal alone),
its accessor's (`grand()` ..) the same object. `grand` is the reference
and every voice's start (`defaultShape`). The two fitted tables' strings
come from `uptool fit --table` against the measured B tables (the
descent's own values kept); the rest is set by hand, rendered and heard
(`plans/upright-engine/out2/listen`). Every preset carries the tine's
`tineT60` 10.4 s and `tineLoad` 1.18, inert on the string. Every preset
sounds at every semitone of its range (2 s at velocity 1, at the gain that
brings its octave from C4 to grand's, since at the plain gain wurlitzer's
low notes sit under the floor: every sample finite and the peak over
0.022, a quarter of grand's octave rms; 723 of 723 notes); through
the board every note's last 100 ms of 2 s sits under its first
(`concertGrand`'s B1, whose lively board bloomed past 2 s while the unison
rang uncoupled, now falls with its strings). `honkyTonk` beats 3.94 Hz on
C4's fundamental against `upright`'s 0 (its tuned unison locks);
`rhodesBark`'s thd at E4 is 50.0 % against `rhodes`'s 3.3.

The octave from C4 against grand's is each of the twelve notes C4 .. B4
struck at velocity 1 and rendered 0.6 s as `play` renders it, its rms over
the first half second, the octave the mean of the twelve powers (grand's
0.0897 rms, −20.95 dB full scale). Nothing evens the presets out: a
consumer sets its own gain (`Tuning.outputGain`; each example's gain is
measured on what it plays).

| preset | the channels that differ from grand's | range | where it came from | the octave from C4 against grand's |
|---|---|---|---|---|
| `grand` | (The channels' table) | 21 .. 108 | the reference: the measured B tables' strings, Chaigne & Askenfelt's C4 hammer, a 1 m² spruce board half open | +0.00 dB |
| `concertGrand` | length 0.6265, bass 2.1145, detune 1.28, boardArea 1.6245, boardLoss 0.018467, open 1 | 21 .. 108 | `fit --table inharmonicity_grand.txt` on the strings: rms ln 0.191 over the Steinway D's keys (0.194 from C1 to C7, the worst B4 71 %); the board and the detune by hand | +1.77 |
| `upright` | length 0.6082, bass 1.4221, hardness 100.41, felt 5.743e-4, boardArea 0.7120, boardLoss 0.042133, open 0.15 | 21 .. 108 | `fit --table inharmonicity_upright.txt` on the strings: rms ln 0.301 (0.259 from C1 to C7, the worst B6 123 %); a small damped board and a softer hammer by hand | −5.25 |
| `spinet` | length 0.6014, bass 1.2209, hardness 238.81, felt 4.061e-4, boardArea 0.6156, boardLoss 0.048735, open 0 | 21 .. 108 | by hand: the shortest strings, the smallest board, closed, a harder hammer | −3.88 |
| `honkyTonk` | `upright`'s strings and board; detune 20, hardness 59.70, felt 7.071e-4 | 21 .. 108 | `upright` with its unison 20 cents wide and its hammer soft (Bank's recipe) | −8.59 |
| `fortepiano` | length 0.6102, bass 1.4842, hardness 42.22, felt 8.123e-4, boardArea 0.7846, boardLoss 0.038237, open 0.25 | 29 .. 101 | by hand: strings a little shorter, soft hammers, a small board; the gauge 1 (below) | −4.27 |
| `rhodes` | generator 0, tonebar 1, detune 1.5137 | 28 .. 100 | by hand: the tine, the pickup set (gap 1.5 mm, offset 0.9 mm), the tonebar beside it at 1.51 cents (the beat heard at 1.5; the strings capped at 2) | −5.01 |
| `rhodesBark` | `rhodes`'s, with hardness 337.73, felt 3.536e-4, gap 0.75, offset 0.3 | 28 .. 100 | by hand: the pickup close, a harder tip; the tonebar at rhodes's 1.51 cents | +0.15 |
| `wurlitzer` | generator 1, hardness 84.43, felt 6.156e-4, gap 1.2184, offset 0.72 | 33 .. 96 | by hand: the reed, the pickup a little close, a softer hammer | −16.34 |

The fit run today moves length and bass apart and lands nearer the
tables: `inharmonicity_grand.txt` at length 0.6176, bass 2.1369 (rms ln
0.176), `inharmonicity_upright.txt` at 0.6359, 1.3795 (0.202); the two
fitted presets keep the strings of the earlier fit, which moved them
together along one line (55).

The plan's fortepiano asks for a thin gauge as well; `fortepiano` holds
the gauge at 1 because through this rank the gauge moves only the wire's
stiffness (the mass is the tension's), so a gauge of 0.8 would lower a
plain wire's B to 0.41 of its value (a wound string's core does not
follow it), the opposite of Bank's fortepiano (lighter strings, more
inharmonicity); the shorter strings already raise the bass's B.
`uptool print --preset NAME` writes a preset's channels as `--set` lines
(its name and range above them); `--preset NAME` names the piano on every
command, `--set` over it, and the tool's three anchors are the presets
`grand`, `rhodes` and `wurlitzer`.

## The pool

`Pool<M>` (`up_pool.h`) holds M voices, a note table and one board in
itself, nothing on the heap: `PoolStorage<M>` the voices, their slots and
their lanes (each voice's bridge force for the block, its weights on the
board and the note they are at), `PoolBase` the logic over them (a host tool
can size one at run time). Its output is the board on every voice's bridge
force with every voice's pickup added, each at its level, not clamped; the
voices' seeds follow from the pool's (`seedOf`).

**The note table.** A voice is free (asleep, no note), held (its key
down), released (its key up: its damper on unless the pedal holds it, or
ringing out over `damperTop`) or stolen (fading for a note that waits for
it). `noteOn(midi, vel)` takes, in order:

1. the key's own voice if it still sounds (held or released): struck again
   (`Voice::noteOn`), a fresh hammer on the moving string, nothing
   restarted;
2. a stolen voice already waiting for the key: the newer velocity;
3. a free voice, the one longest asleep: the note lands whole (the pool's
   pedal, then the note; the voice wakes with every channel on its target);
4. else the quietest sounding voice, held or released, by `Voice::energy()`
   (the last block's summed string energy: a cut is least heard where the
   string is quietest), a voice whose strike is still queued counting as
   loud while another sounds and ties going to the oldest note; it is
   stolen;
5. else (every voice already stolen) the oldest stolen one's waiting note
   is replaced.

**The steal.** A stolen voice rings on while the pool fades its share (its
bridge force into the board and its pickup) by the one law at
`tuning.stealRate`, block by block, to under 1 %, then linearly to exactly
zero across one block; at the next block it is stopped (`Voice::stop`: its
share is already zero, so nothing is cut) and the waiting note lands on it
whole. The board it fed rings on, as a soundboard does. At the default 60
/s that is 27 blocks: the new note sounds 78 ms after its key. Measured
(`pool_steals_the_quietest_without_a_cut`): grand on four voices, C4, E4
(softly), G4 and C5, the fifth key (E5) steals E4's voice (its energy
0.00055 mm² against 1.21 .. 2.33), not the oldest; the stolen voice's share
through a board of its own reads 0.174 on the click measure through the
steal, against 0.092 ringing on unstolen and the stage 1 bound of 1.76 for
the top partial it still rings (6.0 kHz: the loss has taken those above it
to zero by 1.25 s).

**The key's release and the pedal.** `noteOff(midi)` releases the key's
voice: its damper lands unless the pedal holds it (a note still waiting for
a voice lands released). `pedal(p)` writes every voice's pedal channel and
is the pool's pedal: a note that lands takes it (03_Keyboard's CC 64).
`allNotesOff()` releases every key. Measured: grand's C4 released at 1 s,
its voice's force 60 dB under its peak 0.210 s later (the damper's T60
0.345 s); under the pedal it rings on; the pedal's fall at 2 s lands it
0.190 s later.

**Pool-wide writes** (`set`, `setRate`, `setShape`, `setPartial`,
`setPartials`, `setTuning`) go to every voice now, in the
caller's order, and stand over later notes: a voice keeps its channel table
across notes, so a note lands on the piano the pool last wrote, the last
writer winning per channel by the sequence exactly as on one voice (a
partial written by hand pool-wide stands on a later note until the
instrument is written again). The board's plate is the written body:
`setShape` sets it from the body channels (`boardArea`,
`boardLoss`, `open`) where they are written, `set` from the one channel;
the plate lands at once (its table rebuilt only when it moved, every
voice's weights again when its sides did); the voices ease their own body
channels, which nothing in the pool reads.

**The board, shared.** Each block every voice renders its pair into
scratch; each bridge force goes into the board at the voice's bridge
point's weights (`Board::inputWeights` at `Voice::bridgePoint()`, the eased
note, recomputed when it moved); the board renders once on all M forces
(the plan's V × N a sample, a voice asleep putting in zeros); the pickups
are summed in voice order and added. Measured
(`pool_is_its_voices_through_one_board`): four voices over 1.67 s, through a
release, the pedal, a shape that moves the plate and a re-strike, each
voice driven alone the same way, their forces through one board of the
test's and their pickups added by hand: the pool's output to the bit.

**The same key twice** meets its moving string on one voice: grand's C4 at
0.8 struck again a second later reads its onset 0.15 dB over a fresh
strike's (the strings' energy 15.9 mm² after the first strike, 1.9 before
the second, 18.3 after it: a strike on a moving string, not the sum of
two).

Control runs once per 128 samples whatever size `render()` is called with:
the same calls give the same bytes at render sizes 1, 128 and 1000
(`pool_same_bytes_at_any_render_size`). Two pools share nothing.

**The handoff on a Teensy** (`AudioSynthUprightPool<M>`): each setter
pushes one command onto a queue of 64 inside an interrupt mask (the
caller's mask put back); a command is an op, a 16-bit index (a channel or a
partial's slot) and four floats (`setPartial`'s); `pedal` is a small
command; a shape, a partial table or a tuning waits in a small ring
beside the queue (two each), and a second of the
same kind straight after the first merges into it. A setter that finds no
room waits unmasked for `update()` to drain the queue (a block, 2.9 ms),
and gives up only after 20 ms, counting it in `dropped()`. `update()`
drains the queue in order with interrupts masked (the board's table runs
there when the body moved: the 32 frequencies
from the mode pairs found once and the field's sixteen losses, no search;
the moved modes' turns follow in the render), then renders unmasked. Sizes
at 64 partials: a voice 20,080 bytes (its attack 428), a slot 48, a lane
644, the board 14,184 (its field 10,668), `Pool<4>` 99,008, `Pool<8>`
182,160; on the board `AudioSynthUprightPool<8>` is 188,252 bytes and
`AudioSynthUprightPool<4>` 105,180 (at 48 partials 158,556 and 89,820).

## The host rig

`make test`, `make tool`, `make corpus`, `make clean` from the root or
`extras/host`: `make test` builds and runs every test (the wrappers'
sources compile too, to nothing off the board, and `test_pool.cpp` includes
`Upright.h`); `make corpus` writes `extras/host/build/corpus`, the same
bytes on every run, the library's byte baseline. The tests read
`src/core`'s sources for libm and the heap (`config_core_guard`) and the
reference tables. `make board` (the root's) compiles every example for
teensy41 by PlatformIO (`$HOME/.platformio/penv/bin/pio`), nothing
flashed: the library staged under `extras/host/build/pio/lib` as a consumer
gets it, the house's flags (`-Wall -Wextra -fno-math-errno`) and a sketch's
own from its `// make board: FLAGS` line, any warning in the library's
files or the sketch a failure (the framework's own are not ours, so not
`-Werror`); it prints each example's flash and RAM lines and its largest
objects in RAM.

```
uptool partials [--midi M] [--set CHANNEL=V ...]   the law's table at the note: slot, ratio, hz, cents off n f0, t60 at
                                         the note, gain, out, on; the rank's line (with its longitudinal fundamental); the
                                         slots the pickup reads. --set lands on the instrument channels before the law and
                                         on a partial channel after it
uptool partials --table FILE             a reference table as read, beside the law at the shape its fit finds
uptool string [--from M --to M]          the rank per semitone: f0, L, d, wound, core, mu, Z, B, the hammer's mass,
                                         its felt's K and p at the note, share, damper; B against each B table; the break
uptool hammer [--midi M --vel V]         the contact by the truth (32 fs): the force every 0.1 ms, the contact, the peak,
                                         the impulse against the momentum, the force's spectrum, the band; the ladder
                                         (with each velocity's m/s) through the engine's scheme and the truth, kth beside;
                                         the felt table against powf
uptool pickup [--amp A] [--from-hammer --midi M --vel V]   the curves; a sine through the voltage; a ladder
uptool board [--sweep --out FILE]        the plate's modes, the bridge-line weights per octave, the listening
                                         weights; the shelf: the top mode, kBoardShelf, the infinite plate's mobility, the
                                         swept transfer's means over the top mode's octave and over 2 .. 4 f_top, the shelf
                                         that makes them equal; the field: each line's length, g and a, its gain, its
                                         impulse energy against the shelf's share and the kFieldGain that matches, the
                                         energy's arrival for 1 N at c4's bridge point, the swept means over f_top / 2 ..
                                         f_top, 2 .. 4 and 4 .. 8 f_top with the field and with the modes alone; --sweep:
                                         the transfer at 1/12 octave, 20 hz .. 10 khz, at the spinet's, grand's and the
                                         concert grand's boards into FILE
uptool print                             the piano's twenty-four instrument channels as --set lines, a line a group, each
                                         value read back to the same bits (a preset's name and range above them)
uptool random [--seed S --count N --midi M]   N pianos, every instrument channel drawn inside its span (channelSpan),
                                         each as print prints it, with its B, first t60, 8th partial's gain and strings;
                                         each heard 2 s at M: the peak, the first half second's rms against grand's
                                         octave, sounds or not; beside it the octave from M up against grand's; both
                                         levels' spread
uptool fit --table FILE                  the shape nearest a table, by its kind: length and bass (B, decay), hardness
                                         (hammer), generator and tineT60 (tine), each along its span as a coordinate
                                         0 .. 1, a grid in fifths, then descent from 0.25 to 0.001; a channel --set names
                                         held there; the channels found, each entry's residual, the --set lines; heard
uptool fit --constants --tables DIR [--kind KIND] [--out FILE] [--fix NAME,...]   the LawConstants and the channels
                                         every table shares (tension, lossRate, lossSlope, hardness, exponent, hammerMass,
                                         from grand's) against every table at once, each table its own shape at its holds;
                                         each table's residual before and after; the proposed values into FILE
uptool chart [DIR]                       each instrument channel alone at 21 points along its span at grand, rhodes,
                                         wurlitzer; the pairs bass x hardness, strikeShare x detune, boardArea x bass at
                                         grand, generator x gap, hardness x gap at rhodes, in fifths of their spans; the
                                         engine's strikes and the truth's; summary.txt, floors.txt
uptool chart --audio [DIR]               the 1-d sweeps rendered 2 s (c4; e3 on the electric anchors): the table's 10th
                                         partial's cents, fundamental's t60, contact, peak and thd beside the render's;
                                         audio_CHANNEL.txt, audio_summary.txt (the worst disagreements)
uptool play [--preset NAME | --table FILE] [--midi M --vel V --sec S --off-at S --restrike-at S --pedal
            --pedal-off-at S --glide-to M2 [--glide-at S --glide-sec S] -o x.wav --png p.png]
                                         one note through a voice and a board of its own: the law's table beside what the
                                         pair shows (each partial's hz, cents, level, t60), the peak, the first half second's
                                         rms, the first three t60s, the beat and the two slopes on the fundamental, the
                                         hammer's contact and peak (the telemetry), the thd (the electric side), the
                                         release's t60, the sleep; --table: a tine table set whole over the piano (rhodes's
                                         when no preset is named)
uptool corpus DIR [--tune NAME=V ...]    the byte baseline: grand, rhodes, wurlitzer at 36, 60, 84 at velocity 0.5 and 1;
                                         grand at 60 re-struck, released, released under the pedal, gliding to 67; rhodes
                                         at 52 and 64; wurlitzer at 57; the measured tine whole at 64; every preset at the
                                         bottom, middle and top of its range at velocity 0.5 and 1 (preset_NAME_MIDI_vVEL);
                                         a Pool<4> of grand: a chord (c3 e3 g3 c4, the pedal from 2 s, the keys up at 3 s)
                                         and five keys on four voices (the fifth steals); a wav and a png each, index.txt
                                         with what play read (the pool's: the calls, the peak, the steal); --tune lands on
                                         every render (`--tune attack=0`: no attack)
uptool size                              sizeof(up::Voice) by part; the board; the pending table; Pool<4>, Pool<8>
   any command: the piano is grand, or --preset NAME (any of the nine), with --set CHANNEL=V ... over it (any channel;
   repeatable, each taking every pair up to the next option, by spaces or commas); --tune NAME=V (any Tuning field);
   --midi M
```

The fits as they run: `inharmonicity_grand.txt` rms ln 0.176 (length
0.6176, bass 2.1369), `inharmonicity_upright.txt` 0.202 (length 0.6359, bass
1.3795), `inharmonicity_grand_model.txt` 0.0919, `hammer_scale_grand.txt`
at hardness 68.5, `tine_rhodes.txt` at the generator 0.025 and tineT60
10.71; `decay_grand.txt` runs to the spans' ends (length 2, bass 4).
Against the law's own B at length 0.6191 m and bass 1.8043 m the fit finds
both within 0.05 % (`fit_finds_its_own_strings`). The
constants' fit reads the tables' `# hold:` lines as channel holds (the B
tables at the concert grand's strings, length 0.6269 and bass 2.1330, and
the upright's at 0.6076 and 1.4036) and holds a tine table's generator at
0, fitting its tineT60 per table.

The host's own instruments (`extras/host`): `up_wav` (16-bit PCM),
`up_png` (a spectrogram, 0 .. 8 kHz, 2048-point Hann frames, 80 dB, a tick
every 100 ms), `up_analysis.h` (a radix-2 FFT; peaks parabola-interpolated;
a narrow band's envelope from one FFT of the record; T60 by least squares
from −6 to −40 dB; the two-slope fit; the beat reader; the tracker; THD;
the click measure: the worst 10 ms frame's largest first difference over
that frame's rms, a sampled sine at f reading 2√2 sin(π f / fs), the bound
for a sound whose top partial is f 1.5 times that), `up_play.h` (a note
rendered and read as `play` does it; how loud a piano sounds over an
octave, and one note; a shape heard as `random`, `fit` and the presets'
test hear it; the board's swept transfer), `up_contact.h` (the contact
simulator, the truth), `up_fit.h` (the three anchors, the presets of their
names; a shape's `--set` lines and their parser; each channel's span,
`channelSpan`; random's draw; the reference tables read; each table's error
at a shape; the channel fit and the constants' fit). The tests' pianos
(`tests/up_shapes.h`): `with(shape, {{channel, v} ...})`, `withGenerator`
(the tonebar 1 − |g| beside it), and five steps each of the strings (the
spinet's to a concert grand's), the hammer (hardness 25.1 .. 803 N/mm^p
with the felt 1e-3 .. 2.5e-4 s/mm) and the board (the spinet's to a
concert grand's), grand's in the middle. A render that peaks over full
scale is written to its WAV at the gain that brings it to 0.99, and the
gain printed.

`extras/reference/` holds the reference tables (`README.txt` names each
file's source and what is unverified): a Steinway D's and an upright's
measured B per key, a Steinway-D model's B, hammers and damping per key,
Chaigne & Askenfelt's hammers, Askenfelt & Jansson's velocities and contact
times, measured decays at 1 kHz, a Rhodes tine's one overtone and its
fundamental's Q per note, the cantilever's roots, and a grand's and an
upright's soundboard modes.

## Where this departs from the plan, and why

1. **B against L is the rank's own law, not L⁴.** At a fixed pitch and
   tension the plain wire's diameter goes as 1 / L, so B goes as 1 / L⁶; a
   wound string's core is fixed, so B goes as 1 / L². The plan's "1 / L⁴"
   and its gate (B falling as L⁴ from the shortest strings to the longest
   within 1 %) are a fixed-gauge string's. The test measures −6.000 at c4
   over the part of the strings' span where c4 stays plain and −2.000 at a0
   end to end; c4 end to end reads −3.73, wound at the short end. The brief
   named it.
2. **The strike share falls up the keyboard**: share = strikeShare (f_c4 /
   f0)^rankStrike. The plan's (f0 / f_c4)^0.12 rises, against its own words
   ("the treble's hammers land at a smaller share") and the rank test.
3. **The 8th partial at the strike share 0.12 is −18 db**, sin(8 pi 0.12), held under
   −15; the plan's −30 belongs to a share of 0.125 exactly. The brief named
   it.
4. **The beam's gains and outs are its shape over the tip's 2** (the tip at
   ±1). The clamped-free shape reaches about 1.43 inside the beam and 2 at
   the tip, and a gain must stay inside −1 .. 1; the drive and the tip's
   read-out stay in one normalisation.
5. **Every slot's ratio and t60 log-interpolate from the string to the
   tine**, not only the first six: the string's 7th partial would sit under
   the interpolated 6th, and the table must stay ordered. The string's
   slots above six fade by clamp(−g, 0, 1) in gain and out.
6. *Withdrawn (stage 7): what it recorded is gone from the library.*
7. **The felt table holds c^(p − w / 2), w = floor(2p)**, the rest by
   multiplying sqrt(c) (which also indexes the table). g(c) = c^(p − 2)
   linear in sqrt(c) misses the 0.2 % bound at 0.01 mm by 2.5 % at p 3 and
   more at 3.5: impossible as written. This form is exact at p 2, 2.5, 3,
   3.5 and 0.26 % at worst at the quarter powers.
8. **kLambda is 5e-4 s/mm (0.5 s/m).** The plan's "0.5 s/mm" would multiply
   the force 3000-fold at 6 m/s; Hunt & Crossley's loss for a felt is of
   order 0.1 .. 1 s/m.
9. **kK was 169 N/mm^p**, the fit's; the plan's 5e5 N/mm^p is a
   conversion slip (500 kN at 1 mm). Stage 1 sets Chaigne & Askenfelt's
   c4 hammer whole: k 142, p 2.5, 2.97 g (report-0's decision 1).
10. **The contact ends when the felt is free and the hammer itself moves
    back** (its velocity under 0), or at kMaxContact. With "moving away in
    the string's frame" a string that runs ahead of a hammer still coming
    ends the strike: at c4, velocity 1, the medium hammer left after 1 ms
    with a sixth of its momentum unspent.
11. **The stage 0 hammer step was unstable at the sample rate**: the
    felt's loss read explicitly against the string's light point diverged
    where lambda F dt passed its mass. Stage 1 solves the loss against the
    point (27) and runs the contact at substeps (28); the explicit form
    stays as `stepExact`, the simulator's truth at 32 fs.
12. **kForceCap was 40 kN** against the unstable step's peaks; stage 1
    holds it at 2000 N (report-0's decision 7), six times the chart's
    largest (348 N).
13. **The tine's tip lives in the law** (`hammerHardnessAt`,
    `hammerFeltAt`, kTipHard 2, kTipLoss 1), applied where the hammer is
    set, so the channels hold the felt's own figures.
    kTipLoss's value is an estimate the plan left open.
14. **No kTipGain.** Rhodes's c4 tip swings 0.60 mm at velocity 1 and its
    thd far, set, close reads 2.6, 2.7, 39 %: near the plan's bands, so no
    gain was added.
15. **The rank carries f0, d and the tension**, so the law applies the
    ceiling at the note (at kDefaultFs: the law knows the note, not the
    rate) and the tool prints the plain wire's diameter.
16. **`dampRateOf` reads the note from the shape's midi channel** (c4 when
    nan), keeping the brief's signature.
17. **The law's grid in tenths runs over the strings, the strike and the
    generator** at a0, c4 and c7, and each channel the law's table does not
    read (the unison's, the hammer's, the body's, the pickup's, the tonebar,
    the damper's) is shown to leave the table bit for bit: every channel in
    tenths together is out of reach.
18. **The second harmonic's growth is tested at the far and close
    settings.** At the set setting (gap 1.5 mm, offset 0.9: offset / gap 0.6) the offset
    sits at the Lorentzian's inflection (1 / sqrt 3), where the second
    harmonic's term vanishes: it falls 5.4 db from 0.2 to 0.4 mm.
19. **The amp's pole is solved so its −3 db point is ampHz exactly**: 1 −
    e^(−2 pi f / fs) puts it at 6.4 kHz, 6.7 % off.
20. **The board's constants**: kBoardEy 2.75 GPa (spruce's 0.65 GPa with the
    ribs folded in), kBoardAspect 0.477, H = sqrt(Dx Dy), kFc0 80 Hz,
    kFcSpan 1.5 (1 gives 4.7 db at 60 Hz, under the 6 asked), the bridge
    line and the listening point named above.
21. **The hammer brightness test reads the 10th partial's force against the
    1st's.** The force's absolute level at the 10th rises with velocity on
    every hammer; the plan's "under 6 db on the softest" only reads as a
    tone change.
22. **The tool's piano starts at grand**: a channel `--set` does not name
    is grand's, or the named preset's.
23. **The reference tables carry a `# kind:` line and may have several `#
    columns:` blocks**; `# hold:` holds a channel in the constants' fit only.
    Three tables more than the brief's (a Steinway-D model's B and hammers,
    an upright's soundboard).
24. **The constants' fit holds** the concert grand's B tables at a concert
    grand's strings (length 0.6269, bass 2.1330 m), the upright's at the
    plan's upright's (0.6076, 1.4036) and the tine at generator 0, since
    each kind's constants trade with those channels; the hammer tables move
    the hardness, exponent and hammerMass every table shares, beside the
    constants, from grand's; `--fix` holds named constants where they
    stand.
25. **Of stage 0's fit, the hammer's and the tine's moves were taken** (k
    169, p 2.6, massC4 5.4 g, rankMass 0.14, tineT60 10.4 s; stage 1 keeps
    tineT60 and sets the hammer by Chaigne & Askenfelt, 9). **The B and
    decay moves were not**: every sensible B fit needs the span c4's length
    then moved across the pianos (kScale) at ±0.1 or 0 (the planner's
    decision), and held at ±0.4 the tension goes to 1025 N or l0 to 0.49 m;
    the decay fit's knee goes
    under 1 kHz, driven by a model's damping settings against three measured
    decays that pull the other way.
26. **The Tuning's clamps**: hammerScale held over 0.001 m/s and ampHz over
    1 Hz.
27. **The felt's loss reads the string point's own velocity, and solves by
    the trapezoid.** Report-0's decision 2 reads ydot from the last two
    readbacks and puts the string's velocity a whole step ahead, F = a (1 +
    lambda u) / (1 + lambda a dt / m_point). It is stable but first order:
    the felt's damper on the light string point has a time constant near
    1 µs, under a substep, and at 8 substeps the worst cell of A0 .. C8 over
    the ladder is 15× decision 3's bar (5 % contact, 10 % peak), at 16 still
    4×: no rate inside 1 .. 16 met it. The bank holds the point's velocity
    for free (Σ gain ω im, the pickup's quadrature), and the trapezoid
    (the velocity across the step on the mean, ydot + F dt / (2 m_point))
    is second order: F = a (1 + lambda u) / (1 + lambda a dt / (2
    m_point)), u = v_hammer − the point's velocity. The worst cell falls to
    0.97 of the bar at 2 substeps, 0.40 at 4, 0.13 at 8. One division, no
    libm; with lambda 0 the explicit force bit for bit (a test). The five
    forms are in `plans/upright-engine/out1/scripts/loss_probe.txt`.
28. **kContactRate is 2**, decision 3's rule: the fewest substeps within 5
    % on the contact and 10 % on the peak of the truth at every octave A0 ..
    C8 over the velocity ladder, through the voice (the worst 3.95 % and
    2.27 %, both at C8). The loop test compares the force records each
    substep against the truth's at the same instant (the truth's 32 fs a
    whole multiple); a per-sample mean of two substeps against one of 32
    differs by the quadrature, not the dynamics (7.7 % of the peak at C4,
    against 0.8 % at the same instants).
29. **kScale held at 0.1 in the fit.** The fit from the new block (every
    table, and the two measured B tables alone) moves every B constant the
    same way in both runs, and both drive kScale to 0 (the floor of its
    span), where c4's length no longer moves across the pianos. Held at the
    decided 0.1 the two runs pull to
    their bounds (tension 675 and 700 N) and the all-tables run worsens the
    upright (0.62 rms ln). The adopted block is the free runs' agreeing
    moves at their means, kScale 0.1: tension 503 N, rankLength 0.91, dMax
    1.12 mm, core 1.0 mm, coreGrow 0.18, l0 0.614 m, bass0 1.62 m,
    scaleBass 0.415 (the grand's rms ln 0.48 → 0.37, the upright's 0.23 →
    0.24; at kScale 0 they would read 0.19 and 0.16). Every move is inside
    the brief's spans, which the fit's search now holds for tension,
    rankLength, coreGrow and l0. `fit --constants` gains `--kind KIND`.
    Stage 2 sets kScale 0.03 and the planner's block at it (49).
30. **The rank's felt at C2 and C6 misses Chaigne & Askenfelt's K**: one
    power through c4's 142 N/mm^p cannot reach both their 50 and their 1000
    (their points bend: 0.75 below c4, 1.41 above; the best single power,
    1.08, misses both by 37 %). rankHard 1.1 stands: K 30.9 at c2 (−39 %)
    and 652 at c6 (−35 %), two known rows; their p within 0.2 at both.
    C6's contact at 6 m/s reads 1.08 ms (under 1.0 asked: known). Stage 2
    splits the felt's power at C4 (47).
31. **The hammer's band at velocity 0.5 is 5.5 dB** (12 asked as a
    check): kPspan raises the power with the stiffness, and under 1 mm of
    compression a higher power is a softer felt (at kPspan 0 the band moves
    9.5 dB, at −0.5 12.7 dB). Known, with the number; kPspan stands.
    Stage 2 drops the power's span: the hammer's span is the stiffness
    alone (48).
32. **The bank turns clockwise**: z += z (ca − i cb), so an injection into
    im pushes re (the displacement) positive and ω im is the velocity
    (less sigma re). The bank's (re, im) are the simulator's (q, p).
33. **The bank's surface**: `step` and `sums` are templates on <kBridge,
    kTip> writing the bridge's sum (every partial on) and the tip's two
    (the first kTineModes slots) in one pass, each compiled out by its
    flag; `readback(y, ydot)` gives the point's velocity beside its
    displacement; `injectWeighted` (the board's voices, a weight a slot);
    `init(fs, subRate)` (a substep rate for the tests); `on(k)`,
    `drive(k)`, `omega(k)` read back. The coefficients' libm is one
    function, `rotationOf`, so the template needs no instantiation.
34. **`Strings::step<kTouching, kBridge, kTip>`** and `setPartial` with the
    damper's rate apart from a partial's own decay: the secondaries decay
    at their own (the main's over kAftersound) plus the damper's, so the
    damper lands on every string of a note alike; their drive is the
    main's × the main's ω over theirs; a secondary under 1e-4 of the main's
    weight is off (since 95 their own decay is the main's on the string
    side, over kAftersound^u toward the tine).
35. **The pickup hears every bank's first six slots**: on the electric side
    the second string is the tonebar, and the pickup sums it with the tine
    (decision 11 confines the tip to slots 0 .. 5; the simulator has no
    secondaries). The second string's weight × the tonebar from the
    generator 0 up (the `tonebar` channel; the tests sweep it as 1 − |g|
    beside the generator, continuous).
36. **The hammer is placed at the rest, or where the string stands when it
    is nearer the hammer**: a re-strike on a string displaced toward the
    hammer never starts inside the felt.
37. **The law runs at the midi channel's target**, before the tick and
    before the wake's snap, from the targets; the rank for
    the block's coefficients is the eased note's. A glide's partial
    targets are the destination's from its first block, eased at
    rateModes.
38. **The voice's pair**: the bridge force's weight carries the rank's π T
    / L / 1000 (mm to N) and outputGain, the pickup's outputGain; both
    ramp across the block and land exactly at its end. `State` gains
    `hammerContact` (the strike's contact so far); the voice gains
    `stringAt(share)` and `strings()` (a const window on the banks and the
    hammer, the tests' and the tool's); `Strings` publishes the last
    sample's substep forces, the strike's peak and impulse; `Hammer` gains
    `setPointMass`, `stop` and a placing height; `Law` gains `pending`.
39. **kBoardOut 0.0097 and kPickupGain 1.36e-4**, each set so its reference
    (grand at C4 through the board, rhodes at E4 through the pickup,
    velocity 1) peaks 0.3: the levels until stage 2's table. The board's
    modes are in micrometres; `Board::render` writes its output. Stage 2
    re-measures kBoardOut for the board's velocity and shelf (52).
40. **The board is the modal sum the brief gives.** The plan's "above the
    plate's modes the response is the radiation shelf alone" is not built:
    the 32 modes reach 764 Hz at body 0, so through the board the string's
    partials above about 1 kHz ring only in the modes' tails (C6 at
    velocity 1 peaks 15 dB under C4, C2 16 dB over). Stage 2 hears the
    modes' velocity and adds the shelf above them (52).
41. **The silence gate sleeps pianissimo treble strikes at once**: A7 at
    velocity 0.1 (0.07 m/s) leaves 1.2e-9 mm², about −100 dBFS; the voice
    sleeps at the end of its first block. The hammer's comparisons run at
    silence 0 (the gate is not under test there).
42. **The thrash's |x| ≤ 1 is a known row**: the voice is not clamped and its
    level is stage 2's; on the electric side a close pickup on the reed's
    curve (the tip swinging into the held slope near the gap) reads up to
    36× full scale at the corners of what the thrash drew, while the board's side stays
    under 0.28 over the whole 30 s. Finiteness is the check. Stage 2's
    bounded curve brings the pickup's peak to 4.0 (53).
43. **Float's own floors in the bank's tests**: kContactRate substeps land
    on one sample's turn within 4e-6 plus the frequency's rounding over a
    block (2e-6 at 27.5 Hz, 1.5e-5 at 19 kHz; the brief's 1e-6 is under
    the rounding of 256 turns against 128); the velocity readback is held
    to the phasors' own rate, not to a finite difference; the board's two
    voices sum within 1e-6 of full scale.
44. **`play` and the corpus**: `--set`'s instrument channels fold into the
    instrument before a table (an instrument write after a table rewrites
    its partials); a render over full scale is written at the gain that
    brings it to 0.99, said in the index; the corpus is 4 s a render, the
    three anchors at 36, 60, 84 at velocities 0.5 and 1 and the brief's
    eight more (26).
45. **The tests' readings**: the unison's three-string slopes run at
    detune 0 (the beat leaves the slopes alone; 1.5 cents printed); the
    beat reader takes one trough a dip under a moving mean, and no beat
    under a 0.1 dB ripple; the generator's sweep writes the generator and
    the tonebar each block (rateBody 0), its click bound the top partial's
    (19.8 kHz: a loose bound, the measure printed); rhodes's sleep is held
    on the tine alone (1.34 × its t60) and printed with its tonebar (3.35
    ×, known); the band test stands down under 16 slots.
46. **sqrt per sample stays allowed** (stage 0's guard): the felt table's
    index is sqrtf, a single instruction; everything else per sample is
    multiplies, adds and the pickup's and the hammer's divisions.

47. **The rank's felt splits at C4** (report-1's decision 2):
    `rankHardLo` 0.75 below C4 and `rankHardHi` 1.41 from C4 up, both
    fit-movable (kind hammer, 0 .. 2.5). K 50.2 at C2 and 1003 at C6
    against Chaigne & Askenfelt's 50.4 and 1000 (checks). C6's contact at 6
    m/s by the truth reads 1.04 ms (under 1.0 asked: known): with their own
    C6 hammer (2.2 g, K 1000, p 3, no loss) on this string it reads 1.06
    ms, so the felt is not what misses; the string's tension (553 N, the B
    tables') and strike share set it (700 N reads 0.97 ms, a share of 0.08
    at C4 0.79).
48. **The hammer's span is the stiffness alone** (decision 3): `kPspan`
    gone, the power held at 2.5, the hardness 25 .. 803 N/mm^p at C4
    (Chaigne & Askenfelt's C2 hammer's stiffness to their C6's) with the
    felt's loss 1e-3 .. 2.5e-4 s/mm beside it. The band soft to hard by the
    truth and through the voice: 11.7
    and 11.8 dB at velocity 0.5 (at least 10, the plan's 12), 3.8 and 3.7
    at velocity 1 (at least 3, the plan's 6); the contacts soft 2.78 and
    2.40 ms, hard 2.22 and 2.09 ms at 0.5 and 1.
49. **kScale 0.03 and the planner's B block** (decision 4): tension 553 N,
    rankLength 0.915, dMax 1.08875 mm, core 1.0 mm, coreGrow 0.1725, l0
    0.614 m, bass0 1.61375 m, and the strings' span across the pianos 2^±0.03
    on c4's length and 2^±0.4025 on the longest string (scaleBass): the
    spinet's strings to a concert grand's, 0.601 .. 0.627 m and 1.22 .. 2.13
    m, the span the tests and the B tables' holds still use. `fit
    --constants --kind B --fix kScale` from it converged in one pass and
    moved nothing, so the block was adopted as given. The B tables' rms ln
    0.194 (grand) and 0.259 (upright) were checks (0.35, 0.5), the worst
    notes known (B4 70.7 %, B6 123.4 %); since stage 7 the fit moves length
    and bass apart and reads 0.177 and 0.182 from C1 to C7 (B4 68.1 %, B6
    71.0 %).
50. **The string's outs are n (−1)^(n+1)** (decision 5): no gain or out of
    the first six crosses zero along the generator from −1 to 0 (320
    readings at a0, c4, c7). At −0.5 grand's C4 fundamental reads 6.0 dB
    under the tine's on the pickup (a check) and 10.4 dB under the string's
    on the bridge (10 asked: known): the bridge's weight is half there (6
    dB) and the fundamental's gain is half way to the tine's, which is
    struck near its clamp where its first mode barely moves.
51. **The unison is asymmetric** (decision 6): `kThirdDetune` 0.7 and
    `kThirdGain` 0.5, in `up_string.h` beside `kUnisonFloor` (the brief
    names that file; `kAftersound` and `kUnisonGain` stay in `up_law.h`;
    `kThirdGain` gone and the strings equal on the string side since 95).
    Grand's C4 fundamental's ripple over 2 .. 6 s reads 14.9 dB by the beat
    reader's measure (12.9 less its straight line; 46 before; 12 asked:
    known). The planner's probe ran the main string at 15.2 / 2.5 s and the
    secondaries at 15.2 s; the engine's main rings 15.2 s and the
    secondaries 38 s, so over 2 .. 6 s the main stands near the pair's
    difference, where three uncoupled phasors cancel deepest. Two strings
    at 2 cents still beat at the detune's rate within 1 % (−0.26 %); three
    at detune 0 read early / late 1.57.
52. **The board's output is velocity and gains the shelf** (decision 7).
    The bank's `step` gains a third flag, `kVelocity` (the simpler of the
    brief's two ways: the board calls `step<true, false, true>`; with it
    off the strings' sums are the same expressions as before). The shelf is
    two one-pole highpasses (the force less its one-pole lowpass, twice:
    two floats of state) at the top mode's frequency, the coefficient 1 −
    exp(−2π f_top / fs) per `setPlate`. The output is (Σ outW ω im +
    kBoardShelf × the shelf) × kBoardOut, so kBoardShelf (3133 µm/s per N,
    measured by `uptool board` at body 0) is in the modal sum's own units,
    beside the infinite plate's 3604, and kBoardOut (4.259e-6, grand's C4
    peaking 0.300; 1.883e-6 from stage 3b, 90; 1.304e-6 from stage 4,
    104; 1.311e-6 from stage 4b, 116) scales both.
    `Board::setShelf(gain)` lets the tool's sweep vary the shelf.
53. **The electrostatic curve is the Lorentzian centred at the gap**
    (decision 8, report-1's finding 9): `electroC(x, gap)` = 1 / (1 + ((x −
    gap) / gap)²); the stage runs one Lorentzian whose centre,
    `pickupOffset(g, gap, offset)` = offset + clamp(g, 0, 1) (gap − offset),
    is set per block: one division a sample, the hold and the second
    denominator gone. |C'| ≤ 0.6495 / gap over x −3 .. 3 mm, gap 0.5 .. 4,
    offset 0 .. 2, g 0, 0.5, 1 (a check). The old curve's derivative grew
    without bound toward the plate (100 / gap at its hold): a hard reed
    near the plate read 40 dB over its rest slope. The thrash's pickup peak
    falls from 36 to 4.0 (under 1 asked: known): the generator's middle in
    the treble (C#7 at g −0.17), where the tip still reads the string's
    partials, whose outs grow with n; the electric anchors peak 0.06 ..
    0.59 across 28 .. 100 at velocity 1. `kPickupGain` stands (rhodes at E4
    peaks 0.288).
54. *Withdrawn (stage 7): what it recorded is gone from the library.*
55. **The presets**: a `PresetInfo` and accessors; the two fitted
    presets keep the descent's strings (concertGrand's length 0.6265 m and
    bass 2.1145, upright's 0.6082 and 1.4221: the fit then moved the two
    together along one line); `fortepiano` holds the gauge at 1 (The
    presets); the tool's three
    anchors are the presets `grand`, `rhodes`, `wurlitzer`, so the chart's
    and the corpus's anchor renders are the presets'.
56. **The floors as measured**: the hammer's band from the render's partial
    levels (the tracker's first frame); the strike's 8th partial as its
    band's power over 0.1 .. 1 s, since at a node the first 50 ms frame
    holds the onset's leakage from the loud odd neighbours (−8.6 dB there,
    20 .. 30 dB under them); the unison at one string and three wide as the
    brief has it, the tuned three printed; the body's 100 Hz band as the octave 71 ..
    141 Hz through the board, read at A0 where three partials fall in it
    (11.4 dB; C1 11.3, C2 15.6, G2 7.4); the damper with one string printed
    (8.74×); the pickup's second harmonic against the first; the
    generator's floor stands down under 31 slots.
57. **The range test's "decaying" through the board is a known row**,
    naming the notes: at eleven of 723 (bass notes of `grand`,
    `concertGrand`, `honkyTonk`) the board's low modes ring up over the
    first 0.3 s (concertGrand's lively board by 7 dB) or a wide unison
    beats, lifting the last 100 ms of 1 s over the first; the strings
    themselves (the voice's pair) fall at every one of them (a check).
58. **`random` hears** each piano 2 s at `--midi` (60) and calls it
    sounding as the presets' range test does; `fit --table` hears its shape
    the same way; `--preset NAME` takes any of the nine (a `--set` over it
    wins) and `print --preset` writes the preset's own name and range.
59. **The corpus's presets** are named `preset_NAME_MIDI_vVEL`, at the
    bottom, the middle ((lo + hi) / 2 rounded down) and the top of each
    range at velocities 0.5 and 1, 4 s, at the tuning's gain;
    stage 1's 52 renders are the same bytes as the base after section 0;
    in `index.txt` their rhodes and wurlitzer source lines now name the
    presets' ranges (28 .. 100, 33 .. 96).
60. **Stage 2's listening WAVs carry 3 dB of common headroom** (outputGain
    0.7071 over the gain that then levelled each piano to grand's octave);
    none passes full scale (the loudest 0.52).
61. **`chart --audio`** reads the table's measures at the rendered note
    (E3 on the electric anchors, where the brief puts the render) beside
    the render's: the tracker's 10th partial and fundamental's T60, the
    telemetry's contact and peak, `play`'s thd; `audio_CHANNEL.txt` each and
    `audio_summary.txt`.
62. **`uptool board` measures the shelf**: the swept transfer (a unit force
    sine at C4's bridge point, 1/12 octave, each point run to steady state
    and its rms read), the two bands' mean dB, and the shelf that equalises
    them by bisection on its log over runs kept at the shelf 0 and 1 (the
    board is linear in it); `--sweep --out FILE` writes the table at body
    −1, 0 and +1.

63. **The tip's weight is the out held to ±1** (report-2's decision 1):
    each bank's first kTineModes slots carry a second output weight, `tip =
    clamp(out, −1, 1)`, ramped across the block beside the out; the tip's
    two sums read it, the bridge's the out; nothing else in the signal path
    moved. A partial at out 3 reads 3 on the bridge and 1 on the tip (a
    check). Every partial of every bank carries the ramp (16 bytes: a voice
    is 19,320 bytes at 64 slots, 17,792 before). Each product of the tip's
    loop is a statement of its own, so clang's contraction leaves the
    bridge's sum the bits it was (fused into one multiply-add, grand's C4
    moved 1 LSB in 158 samples); the 52 stage 1 renders are the same bytes.
    The simulator's tip (`up_contact.h`) holds the same ±1, so the chart's
    thd columns read what the engine plays. Measured: the tine's and the
    reed's levels unchanged (the largest difference 0.00 dB; the
    generator's string-to-tine half moved up to 1.35 dB);
    `kPickupGain`'s reference (rhodes at E4) peaks 0.288. The thrash's raw
    pickup peak did not move (4.02): it is the thrash's key jumps on one
    ringing voice, not the weights (Known limits).
64. **A piano's level is an octave's** (decision 2): the mean power of the
    twelve notes C4 .. B4, each at velocity 1, against the reference,
    `grand`'s octave. One note's level is the board's coincidence of
    partials, plate modes and the field's grain; an octave's mean is the
    piano's.
65. **The octave from a note is the twelve notes from it up** (mid .. mid
    + 11), as the reference's is from C4.
66. *Withdrawn (stage 7): what it recorded is gone from the library.*
67. *Withdrawn (stage 7): what it recorded is gone from the library.*
68. **The tests re-read where the measure missed** (decision 3), each a
    check: the C4 ripple under 16 dB (14.90); the generator's middle on the
    bridge within 12 dB (10.4); the strings at C2 at least 15 cents (16.9);
    the aftersound's ratio from one string to the tuned three at least 2×
    (12.8×), the beat depth from one string to three wide at least 6 dB
    (25.5); the damper on one string at least 8×
    (8.74×; three strings 7.52× printed); the far pickup's thd at E3 under
    5 % (3.78). The plan's number is printed beside each.
69. **The range test reads 2 s** and its "decaying" through the board is a
    known row at one note: concertGrand's B1, whose lively board rings up
    7.5 dB over its first 0.4 s and falls back under its first 100 ms only
    at 1.95 s (the board +0.2 dB at 1.9 .. 2.0 s; the strings −7.5 dB, the
    strings' check held). Report-2 had the bloom over by 1 s; at that note
    it is not. The suite runs about 35 s.
70. **`random` hears the octave** beside the note (`hearOctave`, the
    octave from the note up against the reference); `fit`'s heard line
    names grand's octave.
71. **The pool's storage holds the board's lanes**: `PoolStorage<M>` holds
    each voice's lane (its bridge force for the block at its level, its
    weights on the board, the note they are at) and the board's M force and
    weight pointers, so `PoolBase` runs over storage it is given; the board
    is `PoolBase`'s own.
72. **The pool's plate is the written body, at once**: `setShape` and
    `set` write the board's plate from the body channels'
    targets, not eased (the voices ease theirs, which nothing in the pool
    reads); a plate whose sides moved recomputes every voice's weights at
    its next render. The tool and the voice wrapper ease the plate with
    their one voice's eased body.
73. **Every voice renders every sample and puts its force into the board**
    (zeros from a voice asleep): the plan's V × N a sample, and the pool's
    sum by hand is the same bytes. A stolen voice's fade scales its force
    into the board and its pickup; the board rings on from what it was
    given. The steal's continuity is measured on the voice's share through
    a board of its own.
74. **The same key struck twice is held by its onset** (within 2 dB of a
    fresh strike's: −1.21), its energies printed (15.9 after the first,
    9.5 before the second, 17.7 after), as the brief asks; a strike on a
    moving string is not the sum of two.
75. **The key's own voice is the one whose slot is held or released**:
    a voice asleep whose slot is not yet freed (between its
    last block and the next block's top) is struck the same way and wakes
    whole, as a landing note does.
76. **The pool's readback adds `PoolBase::seedOf(seed, i)`** and
    `board()`; the tests' window `PoolTestAccess` (`tests/up_pool_access.h`,
    a friend) renders one voice's bridge force and pickup at its
    level beside the output. The fade's step a block is 1 −
    `approachCoef(stealRate, dt)`: no libm in `up_pool.cpp`.
77. **The voice wrapper rebuilds the plate's table as the eased body
    moves** (the brief's), and `boardOf` is about 7 voice-blocks of work
    (56 µs on the host against a voice's block at 7.6), every block while
    the body eases after a write; the pool wrapper rebuilds it once per body
    write, under the drain's mask. Stage 3b finds the mode pairs once (88):
    a rebuild is now about a fifth of a voice's block.
78. **The pool wrapper's commands**: `pedal` and `setPartial` (a 16-bit slot
    and four floats) are small commands; the rings hold two shapes, two
    partial tables and two tunings; its readback is
    `voices()`, `state(i)`, `slot(i)`, `sounding()`, `dropped()`.
79. **The host build compiles the wrappers to nothing**: `src/*.cpp` are
    built into the test binary and `test_pool.cpp` includes `Upright.h`, so
    a broken guard fails the host build; a slip inside the guard shows only
    on `make board`.
80. **The examples' gains are measured on the host** (`plans/upright-engine/out3/probes`):
    01_Grand at 2 (eleven hammers, C3 and C5 at 0.8, the largest peak 0.215
    at a gain of 1), 03_Keyboard at 0.2 (eight keys of rhodesBark struck in
    one block, each levelled to grand's octave, 4.54 at 1;
    grand's 1.11), 99_Benchmark at 0.15 a voice and the mixer halving each
    pool. The tuning's `outputGain` lands at the next block unramped. 03_Keyboard's CC 123 is all notes off.
    Stage 3b measures them again at the new
    reference (93).
81. **`99_Benchmark` runs four phases** (the pool of eight at grand, the
    same pool at rhodes, the pool of four at grand, the four ringing out:
    the denormal check) on the notes E1 .. G3, under which every partial of
    every scale is below the ceiling; its `// make board:` line names the
    default, `-DUP_MAX_MODES=64`, and the 48-partial build was compiled by
    hand with the recipe's own command (RAM1 variables 232,160). It
    instantiates `AudioSynthUprightPool<8>` whole (`template class`), so
    `make board` compiles every member of the template.
82. **`make corpus` gains the pool's two renders** (`pool4_chord`: C3 E3 G3
    C4 at velocity 1, the pedal down at 2 s and the keys up at 3 s, so they
    ring on under it; `pool4_steal`: C4, E4 softly, G4, C5, then E5 steals
    the quietest; 6 s each, `grand` on a `Pool<4>`), each written as `play`
    writes (a sum over full scale at the gain that brings it to 0.99, said
    in the index); the 160 renders before them are the same bytes as the
    base after section 0 (`plans/upright-engine/out3/corpus0`). `uptool
    size` prints `Pool<4>` and `Pool<8>`.
83. **The README's sections and their order are the house's.**
84. **A partial that moves keeps its velocity** (stage 3b, report-3's
    decision 1, correcting report-2's decision 1 by report-3's finding 1):
    in `Bank::setPartial` a partial ringing since the last block (the
    bank's own flag that it was live, so a partial coming on from off
    starts from rest as before) whose hz moved has its phasor scaled by
    hz_old / hz_new, re and im, one division; the strings, the secondaries
    and the board's modes all take it (bounded both ways since 94). The
    thrash's row that levelled each block by its shape is removed
    (report-3's finding 2: levelling under a moving shape lifts the ring of
    the shape before, the app's doing, not the engine's); its raw pickup peak is held under 1.5 (1.35
    as first built, 1.08 since 94; it was 4.02), the raw peaks printed.
85. **The key jump is read over four periods of C3's fundamental** each
    side of the jump (1348 samples, 30.6 ms), not over one block: a block
    holds 0.38 of C3's period, and the rms of one block of the tip's
    velocity sum swings 7 dB from block to block before the jump (2843 ..
    6465 mm/s over four blocks), so one block reads the phase, not the
    level. Over four periods the tip moves −0.54 dB and the board +0.67
    (1 and 2 asked); the one-block numbers (−3.42, +5.76) are printed
    beside. The bridge force falls 1.66 dB, not the 12 of the displacement:
    the rank's bridge scale (π T / L) rises with the note.
86. **The tip's velocity sum in the jump test is the test's own sum** over
    the voice's banks (`Voice::strings()`): every bank's first kTineModes
    slots, w im at the weights the voice gives the tip (the out held to ±1,
    the secondaries' × kUnisonGain × their weights; × lerp(1, kUnisonGain,
    clamp(1 + g, 0, 1)) since 95). Grand's pickup weight
    is 0, so the voice compiles its tip's sums out.
87. **The glide's click measure is a new check** (`voice_key_jump_grows_in_neither_output`):
    the corpus's glide render (grand C4 to G4 from 1 s over 2 s), its click
    measure from 0.25 s, 0.683 against the bound 4.18 for its top partial at
    G4 (0.687 without the rule, by a probe). No test held it before.
88. **The plate's mode pairs are found once, at 1 m²** (report-3's
    decision 2), not at the first caller's area, so the table does not
    depend on who calls first; `boardSearch` (the full 40 × 40 search) is
    public for the test that holds boardOf to it, to the bit, at five
    bodies. A probe over 20,001
    areas from 0.05 to 10 m² found the 1 m² order everywhere: the nearest
    two of the 32 sit 0.9 % apart and the 33rd mode 2.1 % over the 32nd, so
    no float tie can swap them. The table is two function-local static
    arrays and a flag, zero-initialised (no constructor, no guard): 256
    bytes of RAM1. No rendered byte moved.
89. **The README has no "Memory and time budget" section** (the house's
    shape has none): the rebuild's cost is in the Status's hardware bullet, "On a
    Teensy" under How it works, the pool's handoff paragraph and the
    wrappers' header comments.
90. **kBoardOut 1.883e-6** (from 4.259e-6, × 0.442, −7.09 dB; report-3's
    decision 3): grand's octave reads 0.08970 rms, −20.94 dB full scale;
    grand's C4 alone 0.0397 rms, peaking 0.133. kPickupGain is kept, so the tine's and the
    reed's own levels against the reference rise 7.08 and 7.09 dB.
    1.304e-6 since 104, 1.311e-6 since 116.
91. **The "sounds" floor follows the reference** (`kSoundPeak` 0.05 →
    0.022, up_play.h): at 0.05, wurlitzer's A1 and A#1 levelled (0.045 and
    0.049, the reference now 7.09 dB lower) failed the range test, the
    measure moving and not the sound; 0.022 is where 0.05 sat against the
    octave reference of stage 3, about a quarter of the reference's rms.
92. **The headroom test holds grand's octave** (0.0897 within 0.1 dB) and
    prints grand's C4 peak (0.133) and the octave's largest (0.335); the
    random pianos' peaks were rendered at C4 from silence, 2 s, as `play`
    renders, each levelled to grand's octave: the median 0.43, the worst
    0.90 (0.5 and 1.2 asked, held).
93. **The examples' gains measured again** (`plans/upright-engine/out3b/probes/gains_probe`):
    01_Grand at 4.5 (the largest peak 0.095 at 1: 0.43), 03_Keyboard at 0.45 (rhodesBark's eight keys 2.01 at 1: 0.90; grand's
    0.49), 99_Benchmark at 0.35 (the probe now plays its phases: the pool of
    eight 0.55 at grand and 0.54 at rhodes, the four 0.44, at 1; the
    mixer's halves keep the mix under 0.1). Each sketch's comment says what
    its gain holds. Measured again in 104.
94. **The move rule is bounded both ways** (the planner's follow-up to
    stage 3b): a moved partial's phasor is scaled by min(1, hz_old /
    hz_new), so moved up it keeps its velocity and moved down its
    displacement, and neither output grows through a glide or a key change
    in either direction (no piano gets louder when a string is retuned; the
    pickup and the board's modes read velocity, the bridge force
    displacement). Keeping velocity both ways (84 as first built) made a
    downward jump push the bridge by hz_old / hz_new: C5 to C3 +3.9 dB on
    the board, the thrash's raw board peak 1.24. Now C3 to C5 reads −0.54 /
    +0.67 dB on the tip and the board (checks, 1 and 2), C5 to C3 −12.61 /
    −7.76 (the board at most +1, a check; the tip's fall printed), the
    thrash's raw peaks 1.08 on the pickup (under 1.5) and 0.12 on the board
    (under 1.0, a new check). `test_bank` holds a move down: 220 to 110 Hz
    keeps |z| and the velocity halves. No corpus byte moved (no render moves
    a ringing partial down); `make board` moved 64 bytes of the serial
    example's code.
95. **The unison couples through one resistive bridge** (stage 4,
    Weinreich's coupled strings): `LawConstants::bridgeMobility` 3.5e-4 m/s
    per N (an estimate); the rank's `bridgeLoss` = tension ×
    bridgeMobility / L (one division a note); the voice hands the main's
    first kUnisonModes slots σ_n − min(σ_b, 0.9 σ_n) and calls
    `Strings::setCoupling(σ_b)` once a block, σ_b × clamp(−g, 0, 1);
    `Strings::step` couples before every turn (the substep's c in the
    hammer's loop, the sample's after it) over a list `beginBlock` makes
    of the main's slots on under kUnisonModes, a secondary's presence 0
    there when it is off, so the sample loop has no branch a partial;
    `Bank::imAt(k)` gives the im by reference. c acts on im alone, 2 σ_b
    dt, so a lone string loses σ_b in amplitude (one string at C6 reads its
    law's slope to 0.01 %). The secondaries are the main's equals on the
    string side: `kThirdGain` is gone, `kUnisonGain` and `kAftersound` are
    the tine side's along u = clamp(1 + g, 0, 1) (`setUnison` takes the
    generator and runs one pow a block); their drive is unchanged (100).
    The hammer reads the main alone. `Strings::coupled()` reads the list's
    length (the tine-side test holds it at 0). 220 bytes a voice; 1.1 µs a
    voice's block on the host, 15 %.
96. **The locked pair falls through a null, and its test reads it so.**
    With a real Y and a symmetric strike a locked pair's slow mode reaches
    the bridge in opposite phase to the fast one, −(u² + q²) / (2 r) of the
    strike's 2 (q = ε / σ_b, r = sqrt(1 − q²), u = 1 − r), so the sum
    crosses zero at ln(o_f / |o_s|) / (2 s): 5.09 s for two strings at 0.3
    cents at C4, read at 5.09 s, 85 dB down. The brief's ripple under 1 dB
    over 0.5 .. 6 s and late slope from 2 s could not hold; the test holds
    the null at the formula's time (within 10 %), no ripple past it (0.38
    dB against a 1.5 s mean), the fast decay over the first second (−7.05
    dB/s against −6.62) and the slow from 10 to 16 s (−1.66 against
    −1.73), on 16 s. Since 108 the bridge has a reactive part and there is
    no null (109).
97. **The tuned three are held at C4 on 16 s, C6 printed.** Grand's three
    at C4 cross through their null at 2.7 s, so the brief's ripple over 2
    .. 6 s reads the null (16.1 dB) and an 8 s render's late slope its
    recovery (−3.27 dB/s against σ_i's −1.44). Held instead: the prompt
    sound's ripple (0.09 dB from 0.2 s to 20 dB down, against a 0.5 s
    mean; under 1) and the two slopes over 16 s (late −1.42 against −1.44,
    early −10.35: 7.3×). At C6 the tuned detune sits near the coupling (ε
    0.95 against σ_b 1.12), so no mode is the aftersound alone: early
    −49.5, late −25.1 against σ_i's −15.1, printed.
98. **The beating partial is read on two strings**: three at (0, +d, −0.7
    d) beat three ways at once (C4's 4th: 0.52 Hz by the reader), which no
    closed form names; two at the tuned detune beat at the formula's rate
    to four digits (0.2860 Hz), held within 5 %; three printed.
99. **The comb's 8th partial is held by its 60 dB time**: its band within
    60 dB of its peak until 2.39 s (under 3), read before the record's last
    0.25 s, where the cut splashes into every band. The tracker's t60 reads
    3.72 s (its fit from 6 to 40 dB down catches a beat's swell 40 dB
    down), printed with the 2nd .. 8th partials' (12.3, 12.7, 14.8, 9.5,
    6.9, 5.3, 3.7 s; 74, 22, 26, 22, 25, 21, 21 before) and their ripples
    (10.9 .. 16.4 dB; 9.5 .. 26.5 before).
100. **A fractional string holds more energy than either neighbour.** The
    third at presence 0.5 is struck whole (the drive unchanged, as the
    brief has it), heard and coupled at half, so it loses less into the
    bridge and keeps its strike: every bank's energy at 1 s 8.82 mm²
    against 5.44 for two strings and 4.90 for three. Held: finite, the
    energy never rising from block to block once the hammer has left, and
    falling from 1 to 2 s; the bridge force's rms within 1 dB of the span
    the two and the three make (at 1 s 7.46 against 6.81 .. 6.92, 0.65 dB
    over it; at 2 s 2.07, between). Closed by 113: the drive by presence.
101. **The tine side is read at 6 cents.** At 3 cents rhodes's tonebar
    reads 1.18 % over the free rate on three troughs, and so does the
    build before the coupling, to the digit: the reader's bias as the tine
    dies under the tonebar. At 6 cents, seven troughs, +0.06 %. Nothing
    couples there (`coupled()` 0, a check). The preset's own detune is the
    tuned 0.5 cents now (1.51 since 114): its tonebar beats 0.095 Hz at E4,
    a 10.5 s period, three times slower than at 1.5.
102. **The unison's floors are read on 16 s** (the beat's depth, the
    aftersound's ratio): on 4 s the tuned three's ratio read 1.28, the
    null at 2.7 s ending the late fit; on 16 s 7.25× from −1 to 0 (the
    floor 2×), the depth 29.2 dB end to end (the floor 6).
103. **The knee's pins.** `law_decays` holds C4's fundamental at 14.4 s
    within 0.1 and C7's at 0.54 within 0.01 (near 15 and under 2.5 at the
    old knee) and prints C6's 2.41 and C8's 0.088. F#6 (no damper) rang
    past 3 s at the old knee and is 60 dB down at 0.88 s on its own now, so
    `voice_release_lands_the_damper` holds what it meant: F#6 released
    renders the bytes of the key held.
104. **kBoardOut 1.304e-6** (from 1.883e-6, × 0.693, −3.19 dB): three
    equal strings push the bridge harder than the old weighted sum (3
    against 1.75 at the strike), and grand's octave read 0.1295 rms at the
    old kBoardOut; now 0.08970, −20.94 dB full scale; grand's C4 peaks
    0.156, the octave's largest 0.369, C2 0.363, C6 0.137. kPickupGain
    kept. The tine's own octave −4.19 dB against grand's (−5.01 before: the
    tonebar at 0.5 cents adds more coherently over the first half second),
    the reed's −17.83 unchanged (no tonebar), the generator's middle's
    +4.15; the unison's span 8.0 dB from one string to three wide (4.6
    before: one string under three coupled). The examples' gains measured
    again (stage 3b's probe built on this core,
    `plans/upright-engine/out4/probes/gains_probe`): 01_Grand 4 (the
    largest peak 0.106 at 1: 0.42), 03_Keyboard 0.5
    (rhodesBark's eight keys 1.83 at 1: 0.92; grand's 0.27), 99_Benchmark
    0.35 kept (the pool of eight 0.50 at grand and 0.53 at rhodes, the four
    0.33, at 1: the mix under 0.1).
105. **`kBridgeMobility` is among the fit's names with the kind "none"**:
    no reference table measures it, so `fit --constants` prints it and
    `--fix` can name it, and no fit moves it. `uptool partials` prints the
    rank's bridge loss on its rank line.
106. **The decay table's residuals** (`uptool partials --table
    decay_grand.txt`, log2 of the law over the table at the fit's
    shape): Chaigne's three measured strings +2.06, +1.51, +1.46 → +1.21,
    +0.67, +0.61 (rms 1.70 → 0.87); Chabassier & Durufle's model block
    (settings, not measurements) rms 1.42 → 2.97, its b3 keeping the upper
    partials longer than the measured strings do. The fit's single number
    follows the model block (840 rows against 3).
107. **The README keeps no list of tests**: the unison's numbers sit in How
    it works' unison paragraph; `test_unison.cpp`'s four tests became ten.
108. **The bridge has a reactive part** (stage 4b, brief-4b's decision 1):
    `LawConstants::bridgeReactive` 0.5; the voice hands the strings δ = σ_b
    × bridgeReactive (σ_b already × clamp(−g, 0, 1)) through
    `setCoupling(σ_b, δ)`; the coupling sums the displacements beside the
    velocities, R = Σ w re (`Bank::reAt`), and im_j −= w_j (c S − c_r R),
    c_r = 2 δ dt. The sign lowers the in-phase mode's pitch, checked by
    measurement: grand's three at C4 put the prompt sound 0.63 cents under
    the aftersound (3 δ / 2π: 0.50), two at 0.3 cents 0.30 (2 δ: 0.33). A
    lone string's pitch falls by δ too (0.17 cents at C4). The coupling now
    costs 1.5 µs a voice's block on the host (19 %; 1.1 µs in stage 4).
109. **The lock is held by "no dip"** (brief-4b's test): each 10 ms frame
    against the lower of the frames 1 s either side, the largest over 0.5
    .. 15 s under 6 dB. Two at 0.3 cents read −0.67 dB (the null read 85);
    grand's three at C4 read 5.96 dB at 7.65 s, the aftersound's own two
    modes beating (where the prompt sound and the aftersound cross, near
    2.8 s, −0.09 dB). The decays within 15 % of the complex formula: the
    pair −7.20 dB/s over the first second against σ_n + Re sqrt(κ² − ε²)'s
    −6.84 and −1.89 from 10 s against −1.81 (the reactive part moves Re
    sqrt from 0.281 to 0.289); the three −10.48 against −9.80 and −1.54
    against σ_i's −1.59. `bridgeReactive` swept by a probe: the three's
    dip 4.3, 6.0, 7.4, 6.2 dB at 0.3, 0.5, 0.7, 1.0; the brief's 0.5 kept.
    The floors' unison depth at 0 reads 4.2 dB (16.1 with the null). The
    glide's tail (2.9 .. 3 s) sits 23 dB under 0.9 .. 1 s (24 in stage 4,
    9.5 in stage 3b): the coupled prompt sound's own faster decay, not the
    null.
110. **The prompt sound's pitch** is the fundamental's spectral peak (a
    Hann window, 2^21 points, parabola-interpolated) over 0.2 .. 1 s against
    8 .. 12 s: −0.633 cents at C4, held in −1 .. −0.2. At C2 the same
    measure reads +0.69 (the prompt sound still holds the 8 .. 12 s window
    there; not held).
111. **The beat past the lock by the complex formula**, |Im sqrt(κ² − ε²)|
    / π: two at 2 cents at C6 beat 1.1747 Hz against 1.1725 (+0.18 %; a
    resistive bridge's 1.1576, the free 1.2113); C4's 4th partial on two
    strings 0.2919 against 0.2909 (+0.33 %).
112. **The loss law's form** (decision 2): `stringT60` takes the rank's
    t60Scale (its call in `up_fit.h` too) and the knee moves to 800 Hz:
    σ_n = lossRate / t60Scale + lossRate (f_n / 800)². `law_decays` pins
    the fundamental at C2 37.7 s, C4 13.9, C6 3.23, C7 1.26, C8 0.413 and
    C4's 8th and 16th partials 1.92 and 0.498, each within 1 %; key 127
    reads 52 ms (the brief's 57). The decay table (`uptool partials --table
    decay_grand.txt`): Chaigne's three −0.08, +0.31, +1.09 log2 at the
    tool's literal 1 kHz (rms 0.66; with E6 at its fundamental, as the
    brief reads it, 0.50), Chabassier & Durufle's model block rms 2.44 (2.97
    in stage 4). C8's known-limit row goes (0.41 s, over 0.3). F#6 (no
    damper) is 60 dB down at 1.99 s on its own.
113. **A secondary is struck by its presence** (decision 3, closing 100):
    its drive × w_j beside its out and its coupling; strings 2.5's energy
    at 1 s reads 5.16 between 4.66 (three) and 5.20 (two), a check, and
    never rises from block to block.
114. **rhodes's and rhodesBark's detune 1.51 cents** (decision 4:
    1.513688), the tonebar's beat stage 2 heard at 1.5; the strings capped
    at 2 as before. Rhodes's octave from C4 read −5.00 dB against grand's
    (−4.19 at stage 4's 0.5 cents).
115. **The tine side's beat is the two strings' frequency difference**: the
    tonebar's peak (the pair less the tine alone) less the tine's (strings
    1), each over 0.2 .. 8 s: 0.28832 Hz against the free 0.28833. The beat
    reader's trough spacing reads 5.3 % over it at the preset's 1.51 cents
    and 4.9 % over on stage 3b's build at 1.5 (three troughs, the tine
    dying under the tonebar), printed.
116. **kBoardOut 1.311e-6** (+0.05 dB): grand's octave 0.08971 rms; the
    generator's middle's own octave +4.12 dB, the tine's −4.19, the reed's
    −17.83. The examples' gains measured again
    (`plans/upright-engine/out4b/probes/gains_probe`): 01_Grand 4 (0.105 at
    1: 0.42), 03_Keyboard back
    to 0.45 (rhodesBark's eight keys 2.01 at 1, its tonebar's level back:
    0.90), 99_Benchmark 0.35 (0.50, 0.54, 0.33 at 1).
117. **`kBridgeReactive` joins the fit's names** with the kind "none";
    NOTICE's Weinreich line names the coupled unison the library now builds
    (it said "does not build"), and Giordano's impedance, behind
    bridgeMobility, is added.
118. **The attack is a module of its own** (stage 5): `up_attack.{h,cpp}`
    (`Attack`: the longitudinal modes in a `Bank<kLongModes>` at
    kContactRate substeps, and the two pulses), `kLongModes` 4 in
    `up_config.h` (no flag), `Tuning.attack` (held 0 .. 1), the rank's
    `longHz` (a sqrt per note), `State::ampLong`, `Voice::attack()` (the
    host's window, as `strings()`). Each pulse is a phasor turned by 2π /
    its width a sample (a raised cosine is 1 − cos), its width whole
    samples (66 and 441 at 44.1 kHz, the thud from sample 88). The voice
    compiles the attack out off the string side and at attack 0.
119. **The clank's drive carries dtSub / ω_k**, as every bank's does (the
    string's and the board's): re is the mode's displacement, and the
    clank's level holds at another rate or substep count. The brief's
    inject(F² k cos(k π β)) is the force's projection on the mode; put into
    im as it stands it would move with the substeps and give a mode k times
    its own displacement.
120. **The attack's weight is its own ramp**, clamp(−g, 0, 1) × outputGain
    beside the bridge's, not its newtons put into the bridge's sum at 1000
    / the bridge scale ahead of the bridge's weight (the brief's "inside
    emit before bridgeW_"): across a block where the note moved, the
    bridge's weight ramps between two notes' bridge scales and carried the
    attack with it, so a key jump from the treble to the bass on one voice
    (the thrash) put 871 N on the bridge and 3.1 on the board. The
    generator's fade and outputGain apply once, as the brief asks.
121. **kLongOut 3.353e5**, read on the attack's own force (the bridge force
    at attack 1 less at attack 0: the attack feeds nothing back, the
    strings the same bits either way) in f_L,1 ± 2 % over the first 50
    ms, against the strongest partial tracked on the bridge force at
    attack 0: grand's C2 at velocity 1, −12.00 dB. Beside it C2 at 0.5
    −30.10, C4 at 1 −45.41 and at 0.5 −67.23, A0 at 1 +11.79, C6 at 1
    −61.56. The whole bridge force's band at C2 reads 10.05 dB against the
    attack's own 10.15.
122. **The square law reads steeper than the square**: from velocity 0.5
    to 1 the clank moves 2.51 × the strongest partial at C2 and 2.81 × at
    C4 (1.7 asked). The longitudinal modes sit over the contact's
    bandwidth (C2's contact 3.25 ms against a 0.83 ms period), so the
    clank reads the force squared's spectral tail, which a harder blow's
    shorter contact raises. The same steepness puts A0 at +11.8 dB over its
    strongest partial, the thrash's raw board peak at 0.59 (0.10 at stage
    4b; its row, under 1.0, holds), `honkyTonk` levelled at 1.07 at A#0 and
    `spinet`'s eight keys at 1.16 at a gain of 1 (03_Keyboard's 0.45 holds
    them at 0.52). Not limited: the calibration point is the brief's.
123. **The knock and the clank are read on the attack's own force.** At C4
    the string's own force reaches the bridge before 1.5 ms (0.13 N over 0
    .. 0.5 ms, the modal sum; 0.83 over 0.5 .. 1 and 4.85 over 1 .. 1.5,
    the stiff string's upper partials ahead of the fundamental's 1.7 ms),
    so at attack 0 the first 1.5 ms is not zero and the whole force at
    attack 1 peaks 5.05 N there; the attack's own force peaks 3.70 N
    against kKnock m_h v0 / kKnockMs's 3.47 (+6.7 %, the clank adding;
    within 10 %, a check). In the suite "attack 0 is stage 4b" is: at 0 the
    attack never runs and its bank stays zero, and the strings are the same
    bits at 0 and 1 every block (the corpus at attack 0 is the byte check,
    126). A transverse partial sits within 2 % of f_L,1 at C4 (the 15th at
    4077 Hz) and at C6, so the whole bridge force's peak there is the
    partial's (−0.82 %, +1.63 %): the pitch is held on the attack's own
    force (C2 −0.001 %, C4 0.000 %), the whole printed beside.
124. **The thud is a known row**, kThud 0.5 and kKnock 0.25 kept: at C4 the
    output's band under 150 Hz over 5 .. 40 ms reads −0.01 dB against
    attack 0 (6 asked), the string's first arrival ringing the plate's low
    modes 19 dB over the attack's share. By the board's linearity (a
    probe, `out5/probes/thud_probe`) kThud 6 reads +4.1 dB and 8 +7.6 at C4
    (1 .. 3 first lower the band: the thud against the string's onset); at
    C2, whose first two partials sit in the band, 8 reads +2.6. kKnock 4
    would add 4.8 dB to C4's first 3 ms.
125. **A0's longitudinal fundamental is 685 Hz**, not the brief's "about
    1.3 kHz at C2 and A0": A0's 1.48 mm core on 70.2 g/m gives c_L 2210
    m/s on the case's 1.614 m; C2's 1.10 mm core on 12.4 g/m 3899 m/s,
    1208 Hz.
126. **`uptool corpus` takes `--tune`** (each render's tuning, a preset's
    gain over it, the pool's), `attack` is among `--tune`'s names,
    `partials`'s rank line names the longitudinal fundamental and `size`
    the attack. The corpus at attack 0 on stage 4b's levelling renders
    stage 4b's 164 files' bytes, its index.txt the same once its 80
    "attack=0" annotations are out; on this stage's levelling 23 files of
    upright's and honkyTonk's presets move with their gains (by up to 0.01
    dB).
    NOTICE adds Bank & Sujbert (the longitudinal modes) and the key's bed
    to Askenfelt & Jansson's line.
127. **kBoardOut stays 1.311e-6**: grand's octave reads 0.08970 rms
    (0.08970131; stage 4b's 0.08970784, −0.0006 dB; 1.31098e-6 would land
    it exactly). The examples keep their gains (`out5/probes/gains_probe`):
    01_Grand 4 (0.104 at 1: 0.42), 03_Keyboard 0.45 (rhodesBark's eight keys 2.01: 0.90; grand's
    0.66), 99_Benchmark 0.35 (0.50, 0.54, 0.33 at 1).
128. **The attack's cost**: 0.43 µs of a ringing grand C4's voice block on
    the host (5.4 %; the attack alone, setBlock and 128 samples, 0.75 µs)
    and 440 bytes a voice; `config_core_guard` names `Attack::init` (the
    pulses' turns, per rate) and `Attack::setBlock` (a cos a mode, per
    block).
129. **The knock is the blow and the action's noise** (stage 5b, brief-5b's
    decision 2): the raised cosine goes. The blow's share is kKnockForce ×
    the mean of the sample's substeps' forces (the sample's impulse at the
    sample rate). The noise is a burst from the voice's xorshift32, drawn
    by nothing else (one draw a sample for the burst's 1058 samples, so the
    corpus stays byte-stable and a voice of another seed differs in the
    burst alone), its envelope a per-sample multiply and its two one-poles
    from per-rate coefficients (`Attack::init`). `Attack::parts()` holds the
    last sample's four parts apart (the host's window): the tests and the
    probes read each through a board of its own.
130. **The knock's calibration** (`out5b/probes/parts_probe`): at grand's C4,
    velocity 1, over the first 10 ms against the string's own, the blow
    −10.00 dB (kKnockForce 0.983: near the hammer's whole force through the
    action), the noise −8.00 (kNoise 9.84), the two −6.14, the attack's own
    −6.10 (the thud −27 and the clank −37 there). C2: −8.3, −5.7, −3.9; C6:
    −12.2, −13.8, −10.0. At velocity 0.5 the knock falls 12.00 dB at C4
    against the string's 12.42, a known row (the noise goes as m_h v0).
131. **The thud's 4 dB is the band's nearest**: the blow's share carries the
    momentum's low end into the plate's lowest modes, so the band under 150
    Hz over 5 .. 40 ms at C4 reads +10.2 dB with no thud; the thud, 2 ms
    later, first lowers it, to its least, +4.72 dB at kThud 6.75 (the
    probe's sweep: +9.8 at 0.5, +6.2 at 4, +5.1 at 8, +6.8 at 10), and the
    check, 4 ± 1, holds there. The thud alone reads +8.9 dB over the
    string's own band, the blow alone +7.0.
132. **The clank's per-note scale** (decision 3): the drive and the output as
    the brief derives them, the rank supplying A_core, μ, L, T and c_L = 2 L
    longHz; kLongAngle 0.08; kLongShare 0.1158 at grand's A0 (−10.00 dB
    against the note over the first 50 ms). Against the note at velocity 1
    A0 −10.0, C2 −35.8, C4 −37.4, C6 −31.9, at 0.5 −26.5, −52.9, −57.4,
    −54.3; against the strongest partial A0 −5.1, C2 −24.2, C4 −39.2, C6
    −35.3. From velocity 0.5 to 1: 2.34 × the partial at A0, 2.51 × at C2,
    2.81 × at C4, 3.10 × at C6. The modes' weights against each other are
    stage 5's (one k left in the output); what moved is each note's scale,
    as A_core² / (μ L²): −4.7 dB at A0 and +18.5 at C4 against C2.
133. **The clank's pitch is read from the burst's end**: the attack's own
    force over 24 .. 84 ms, where the blow, the noise and the thud are done,
    peaks at the rank's frequency to 0.001 % at A0, C2, C4 and C6.
134. **`Tuning.attack` is held 0 .. 2** (decision 4); 1 is the calibration.
135. **The hammer's floor is read at attack 0**: the noise, the same for
    either hammer, sits in the tracker's first frame over C4's upper
    partials, and at velocity 0.5 the soft-to-hard move reads 5.8 dB at
    attack 1 (10 asked); the floor is the hammer's on the string (12.2 at
    attack 0), the attack-1 moves printed (3.5 and 5.8). The strike's floor
    reads 81.1 dB (84.3), the body's 11.4 (11.3).
136. **kBoardOut 1.310e-6** (from 1.311e-6): grand's octave read 0.08976
    rms at the old (the knock's first 10 ms), now 0.08969 (−0.0008 dB). The
    examples (`out5b/probes/gains_probe`): 01_Grand 4 (0.106 at 1: 0.42);
    03_Keyboard 0.45 (rhodesBark's eight keys 2.01: 0.90;
    grand's 0.82); 99_Benchmark 0.35 (0.51, 0.54, 0.33 at 1).
137. **The attack-0 check**: `uptool corpus --tune attack=0` on stage 4b's
    levelling and kBoardOut 1.311e-6 renders stage 4b's 164 files'
    bytes, index.txt once its 80 annotations are out; with this build's
    levelling and kBoardOut 75 of the 165 match (every string-side render
    passes through the board at the new kBoardOut). The bottom notes
    levelled, at velocity 1: grand 0.217, concertGrand 0.500, upright 0.466,
    spinet 0.449, honkyTonk 0.656, fortepiano 0.393 (honkyTonk's loudest
    note now F1's 0.85, not A#0's 1.07); the thrash's raw board peak 0.25
    (0.59 in stage 5), the blow of a hard hammer in the treble.
138. **The attack's cost**: 0.46 µs of a ringing grand C4's voice block on
    the host (5.7 %; the attack alone, setBlock and 128 samples, 0.72 µs),
    464 bytes a voice.
139. **The knock is the blow's share alone** (stage 6): the action's noise
    is gone. The owner heard the attack as clicky and plucky, not struck.
    Measured on stage 5b's build by a host probe (grand at C3, C4 and C5,
    velocity 0.5 and 1): the noise, a white burst with a one-sample onset
    through its one-poles at 300 Hz and 4 kHz decaying over 4 ms, added
    about 30 dB to the band from 5 to 22 kHz over the first 10 ms against
    the string alone and doubled the onset's spectral centroid; it sounded
    alone in the 1.7 ms (C4) to 3.4 ms (C3) before the string's wave
    reaches the bridge; and the click measure (`clickRatio`) rose from 0.35
    to 1.77 at C4 and from 0.21 to 2.13 at C3 with the attack on. With the
    noise alone taken out (the blow, the thud and the clank kept) every
    onset measure came back to the string's own: the click 0.32 and 0.22,
    the 5 .. 22 kHz band within 1 dB of attack 0, the sustain's level and
    peak unchanged. It was the one part of the attack with no physical
    source (5b added it to make the attack audible, calibrated to a level
    target: 129, 130); the knock in the literature is body-borne, through
    the action and the frame into the board, which the blow's share through
    the plate's low modes already is. Gone: kNoise, kNoiseMs, kNoiseSpan,
    kNoiseLowHz, kNoiseHighHz, `AttackParts::noise`, the noise's state and
    poles, the generator argument of `touching`, `step` and `rest`, and
    `attack_noise_deterministic`. The voice keeps its xorshift32, its seed
    and `reseed()` (the voice's and the pool's constructors and the
    wrappers take a seed), drawn by nothing: any seed renders the same
    bytes. Grand's C4 at velocity 1 over the first 10 ms: the blow's share
    −10.13 dB against the string's own (held −10 ± 1.5), the attack's own
    −9.99 (−6.10 with the noise; held at −9.99 ± 1.5); the knock's fall
    from velocity 1 to 0.5 a check at C4, 12.63 dB against the string's
    12.44 (with the noise 12.00 against 12.42, a known row); the thud +4.70
    dB on the band under 150 Hz (4 ± 1). The hammer's floor at attack 1
    reads 4.6 and 12.3 dB (3.5 and 5.8 with the noise, which swamped the
    upper partials' band in the tracker's first frame); the floor stays
    read at attack 0, the string's own (135). The clank's pitch window, 24
    .. 84 ms, starts after the thud's end at 12 ms. A voice is 20,016 bytes
    (its attack 428; 464 with the noise); the attack costs 0.43 .. 0.49 µs
    of a ringing grand C4's block on the host, its own block 0.72 .. 0.75
    µs (Status).
140. **The board's field** (stage 6). Above its 32 modes the board was a
    flat shelf, the summed force through two one-poles × a constant
    mobility: for 1 N for one sample at C4's bridge point 81.5 % of the
    output's energy arrived in the first sample and 96.4 % within 5 ms, a
    wire above 764 Hz. A plate's modes over its top one overlap (the modal
    overlap n η f passes 1 from about 640 Hz on this plate, n one mode per
    19 Hz at 1 m²) and each rings t60 = 6.907755 / (π f η): 96 ms at 764
    Hz, 73 at 1 kHz, 18 at 4 kHz. That diffuse, decaying response is what a
    struck string sounds through; through a wire it reads as a pluck (stage
    5 answered the owner's "plucked, not struck" with the attack, 118, and
    did not reach this). The direct share and the field (How it works) take
    the shelf's place: 7.9 % in the first sample, 75.1 % within 5 ms. The
    owner heard the prototype of both changes (the network this builds, at
    grand's body) and said go. Inside it, each decision measured: the
    lines' lengths are fixed sample counts whatever the body and the rate,
    since the body's size shows in the modes and the crossover and a
    network whose lengths moved under a ringing chord would click (its
    density is the plate's at 1 m² alone: Known limits). kFieldDirect 0.316
    is an estimate set by ear, the spreading wave's first arrival: the
    infinite plate's drive-point mobility is real and instantaneous, but
    the sound of the spreading bending wave is not (at 1 kHz it crosses 0.7
    m in about 3 ms). The gain has no t60 factor: the plan's kFieldGain ×
    sqrt(kFieldT60Ref / t60(f_top)) took a reverberation's energy to go as
    its decay time, but the network's impulse energy at unit gain hardly
    moves with the loss, 16.26 at body −1, 17.25 at 0, 20.0 at +1 (most of
    it the first pass, sixteen arrivals of energy 1 each), so the law read
    +3.96 and −3.57 dB from the shelf's share at body −1 and +1; one gain at
    every body reads −0.25 and +0.64, held within 1 dB, and kFieldT60Ref is
    gone. The loss is a one-pole a line fitted at two points, dc (the top
    mode's t60) and kFieldFitHz (the plate's at 4 kHz), exact there (held
    within 1e-4; 1.1e-6 at worst) and falling faster than 1 / f between:
    the lines' mean t60 at 1.5 f_top is 0.69, 0.76 and 0.97 of the plate's
    at body −1, 0, +1 (Known limits). The decay test asserts that exact
    claim, 96 checks, and prints the one-pole's t60 and a late decay beside
    the plate's. The first build fitted each band's envelope against the
    plate's t60 (within 25 % at 1 kHz, 35 % at 4 kHz), but the first pass of
    sixteen arrivals carries 85 to 98 % of a band's energy and hides a
    decay fit: at body −1 it read 35 % short at 1 kHz and 66 % long at 4
    kHz, the 4 kHz miss the build-up (Schroeder's backward integration read
    −6 % there) and the 1 kHz one the one-pole's own (−37 %). The late decay
    printed is Schroeder's from 10 ms over the octave about 1.5 f_top: 16.7
    and 160.8 ms at body −1 and +1, the one-pole's 16.7 and 164.3; at body
    0 it swings with the band and the start, 23.7 .. 41.0 ms (26.8
    printed). The mean test reads power: the first build held the mean of
    twelve swept points' dB, 1/12 octave apart, over the top mode's octave
    against 2 .. 4 f_top within 5 dB, and on a grainy response (±10 dB from
    point to point) twelve points' mean has a standard deviation near 1.6
    dB and the mean of the dB sits about 2.5 dB under the power's, so it
    read +6.70 dB at body 0 where the power from the impulse response's FFT
    put 2 .. 4 f_top within 0.7 dB of the flat shelf.
    `board_field_mean_power_is_the_shelfs` holds the power mean of |H|²
    over f_top .. 2 f_top, 2 .. 4 and 4 .. 8 f_top (a 16384-point FFT of the
    impulse response) within 3 dB of a flat shelf's (the modes alone plus
    kBoardShelf through the two highpasses), the mean of the dB and its
    standard deviation printed; body +1's two lower bands are known rows
    (the tilt). `board_mode_rings_true` sets the shelf's mobility to 0,
    since the field's reply to its 1000 N impulse buried the mode's zero
    crossings (mode 0 read 96.9 Hz against 49.97);
    `board_shelf_meets_the_modes` is gone. The design runs in
    `Board::setPlate`, already on `config_core_guard`'s list (two pows and a
    sqrt a line and a cos, a rebuild); per sample, multiplies, adds and the
    lines' reads and writes. `Board::fieldStep(force)` (the field alone, the
    tool's and the tests' measure) and `fieldLine(i, g, a)` are public, and
    `uptool board` prints the lines, the gain and its energy, the arrival
    and the band means with the field and with the modes alone.
141. **kLongShare and kBoardOut set again on the field's board.** kLongShare
    0.0912 (from 0.1158): through the field grand's A0 read its clank −7.93
    dB against the note, and the clank is linear in it: 0.1158 ×
    10^(−2.07 / 20) is 0.0912, and A0 reads −10.00. The other notes against
    the note at velocity 1: C2 −41.50, C4 −46.51, C6 −32.63 (−35.78, −37.35,
    −31.88 on the shelf). kBoardOut 1.235e-6 (from 1.310e-6, −0.51 dB):
    grand's octave read 0.09511 rms at the old, +0.509 dB; now 0.08967,
    −20.95 dB full scale (−0.003 dB from 0.0897; 1.236e-6 reads +0.004);
    grand's C4 alone 0.0484 rms, peaking 0.178, the octave's largest 0.381.
    The examples keep their gains (5b's `gains_probe` built on this core):
    01_Grand 4 (0.128 at 1: 0.51), 03_Keyboard 0.45 (rhodesBark's eight keys 2.01:
    0.90; grand's 0.50), 99_Benchmark 0.35 (0.466, 0.535, 0.345 at 1: the
    mix under 0.1). The corpus baseline written again: its 34 tine-side
    renders read the same in `index.txt` (the tine side never reaches the
    board), the 48 string-side renders moved. `make board` again (Status).
## Known limits

- **The presets are not levelled.** Their octaves from C4 run from
  wurlitzer's −16.34 dB to concertGrand's +1.77 against grand's (The
  presets), and a single note is the board's coincidence of partials,
  plate modes and the field's grain; a consumer sets its own gain
  (`Tuning.outputGain`), one gain for all in 03_Keyboard.
- **The board blooms in the bass**: its low modes ring up over the first
  0.3 .. 0.4 s, so a bass note's output can sit over its first 100 ms for
  a while; since the unison couples, no note of a preset's range does so
  at 2 s (concertGrand's B1 did until 1.95 s while its strings rang
  uncoupled); the board is linear and stable.
- **The field tilts on a large board**, two known rows: its power follows
  its t60, so on the concert grand's board, where the top mode (470 Hz)
  rings 253 ms and 4 kHz 30 ms, the board's power over f_top .. 2 f_top
  (470 .. 940 Hz) sits +3.61 dB above a flat shelf's and over 2 .. 4 f_top
  +3.21 (3 asked), over 4 .. 8 f_top +0.70; on the spinet's and grand's
  every band is within −0.72 .. +1.13 dB.
- **The field rings shorter than the plate between the crossover and 4
  kHz**: each line's one-pole meets the plate's law at dc (the top mode's
  t60) and at 4 kHz and falls faster than 1 / f between, so the lines' mean
  t60 at 1.5 f_top is 0.69, 0.76 and 0.97 of the plate's on the spinet's,
  grand's and the concert grand's boards (0.57, 0.65 and 0.80 at the top
  mode). Its late decay there, by Schroeder's integration from 10 ms, lands
  on the one-pole's on the spinet's and the concert grand's (16.7 and 160.8
  ms) and swings with the analysis on grand's.
- **The field is grainy**: its transfer swings with the frequency, as a
  diffuse field's does (the dB's standard deviation 4 .. 7 dB in each
  octave band above the modes, its mean 1.5 .. 3.5 dB under the power's),
  so a single note moves with where its partials land: with the field in,
  the presets' single C4 notes against grand's octave moved −1.1 to +1.4
  dB (grand's from −6.8 to −5.4 dB), and grand's A0 read its clank 2.07 dB
  louder until kLongShare was set again (141).
- **The field's lengths are fixed whatever the body and the rate**: one
  network mode per 16.9 Hz at 44.1 kHz on every board, the plate's own
  density (one per 19.3 Hz) at 1 m² alone; the spinet's plate (0.62 m²)
  has one per 31.3 Hz, the concert grand's (1.62 m²) one per 11.9, and at 96 kHz the
  same lines are one per 36.9 Hz, 0.6 .. 2.9 ms. The body's size shows in
  the modes and the crossover; lengths that moved under a ringing chord
  would click.
- **The field's first pass carries most of an impulse's energy**: sixteen
  arrivals of energy 1 each against the network's 16.26 .. 20.0 at unit
  gain across the boards, so its level is its first pass's and moves 0.89
  dB from the spinet's board to the concert grand's whatever the loss, and
  a decay fitted from a band's peak reads the build-up (the first pass 85
  to 98 % of a band's energy on the spinet's and grand's: 140).
- **The field costs memory and time**: 10,668 bytes a board (a Board
  14,184 bytes, `Pool<8>` 182,160; each voice wrapper carries a board of
  its own, 01_Grand's `piano` 43,944 bytes of RAM1) and about 1.7 µs a
  block on the host (the board's block 5.4 µs where nothing moves, the
  shelf's 3.6; a voice's 8.3), once a pool and once a voice wrapper.
- **The strings' reach at C2 is 16.8 cents** on the 10th partial from the
  spinet's strings to a concert grand's (the plan's 30): the tenor moves ±2
  % (the measured B tables' fit) and C2 is wound on every one, so the bass
  break and the core carry it there.
- **The hammer's span reaches little at a fortissimo**: from Chaigne &
  Askenfelt's C2 hammer to their C6 (hardness 25.1 .. 803 N/mm^p, the felt
  1e-3 .. 2.5e-4 s/mm) 3.8 dB on the force's band at velocity 1 (the plan's
  6), 11.7 at 0.5 (its 12); at a fortissimo every felt is stiff. The
  physics' reach.
- **The pianissimo contact** at C4 is 21 % short of KTH's at 0.28 m/s (the
  modal string's point response, or the felt's law at small
  compressions); from mezzo-forte up within 5 %.
- **C6's contact at 6 m/s is 1.04 ms** by the truth (1.0 asked; Askenfelt &
  Jansson's ff 0.82): the string's tension and strike share set it (47).
- **The measured B tables**, four known rows: fitted on length and bass
  (`fit --table`) the grand's worst note B4 is 68.1 % off (the source's own
  outlier), the upright's B6 71.0 %, their rms ln 0.177 and 0.182 from C1
  to C7 within the gates; the presets' own strings (`concertGrand`,
  `upright`, the earlier fit) read B4 70.7 % and B6 123.4 %.
- **The engine's step at the audio rate** (one substep) misses C6's peak by
  15 %: the contact runs at 2.
- **The phantom partials are not built.** The attack's clank is the
  string's free longitudinal modes driven while the hammer touches; the
  forced longitudinal response, at the sums and differences of the
  transverse partials for as long as they ring (Bank & Sujbert), is the
  later list's, as are the damper's noise and the key's release.
- **The knock is relatively louder at a soft touch at C6**: the blow's
  share is the hammer's force, and from velocity 1 to 0.5 at grand's C6 it
  falls 12.44 dB over the first 10 ms where the string's own falls 14.37. At C4 it falls as far as the string (12.63 against 12.44, a
  check with 0.19 dB to spare) and at C2 (12.16 against 11.75); C6 is
  printed, not held.
- **The thud reads as a dip.** The blow's share, the hammer's own force,
  carries the momentum's low end into the plate's lowest modes and lifts
  the band under 150 Hz at C4 10.2 dB on its own; the thud, a push 2 ms
  later, partly cancels it there, and the band reads +4.7 dB at its
  calibration (kThud 6.75, the least the sweep reaches; the thud alone +8.9
  dB over the string's own band).
- **The clank is the bass's.** Its modes sit over the contact's bandwidth,
  so it grows faster than the square of the blow (from velocity 0.5 to 1
  30.0 dB at C2 where the strongest partial moves 12.0; 3.10 × at C6). Set
  10 dB under the note at grand's A0, it reads 41.5 dB under at C2, 46.5 at
  C4 and 32.6 at C6: a metallic edge in the bottom octave, a trace above.
- **The attack in the generator's middle**: the attack keeps the bridge's
  weight, clamp(−g, 0, 1), while the string's partials fade toward the
  tine's, so between the string and the tine the knock stands over a
  quieter string.
- **The tuned three's aftersound beats once, slowly.** The bridge's
  reactive part keeps the prompt sound and the aftersound from cancelling
  (no dip where they cross at 2.8 s), but the aftersound is two modes, the
  second's and the third's against the main, and they beat against each
  other: grand's C4 fundamental dips 6.0 dB at 7.7 s, 45 dB under the
  strike (the check holds under 6; at bridgeReactive 0.3 it read 4.3, at
  0.7 7.4, at 1.0 6.2). The three strings' release T60 reads 7.9× from
  dampRate 60.6 to 6.6 /s (one string 8.7×), their two decays tilting the
  fit; a wide unison's beats swamp the two-slope fit (three strings 20
  cents apart: the aftersound's ratio 0.24, 5.6 for the tuned three read
  on 16 s).
- **The upper partials still beat.** The detune in hertz grows with the
  partial and the coupling does not, so at the tuned 0.5 cents C4's
  fundamental locks and its partials from the 2nd up beat, the main and
  the second at 0.13 Hz on the 2nd, 0.29 on the 4th and 0.61 on the 8th
  (each a little under its free rate); each of the 3rd .. 8th ripples 11 ..
  13 dB over 0.3 .. 3 s against a 1 s mean (9 .. 27 before the coupling).
  The loss keeps the higher ones short: the 8th is 60 dB down by 1.9 s.
- **The decays against the historic strings**: the law against Chaigne's
  three measured strings reads D#1 7.82 s (8.29 measured), C4 5.99 (4.84)
  at 1 kHz and E6's fundamental 2.41 (1.38): rms log2 0.50, E6 the
  furthest (the tool's literal 1 kHz reading at E6, below its f1, is 2.95
  s: rms 0.66); Chabassier & Durufle's model block, settings and not
  measurements, reads rms log2 2.44 (its b3 rings the upper partials
  longer than the measured strings do); A0's fundamental 38 s (the rank's
  T60 scale capped); the coupled three's aftersound rings at the internal
  loss alone (C4's fundamental 38 s by its rate). The top octave rings
  short but speaks: C8's fundamental 0.41 s, C7's 1.26, key 127's 52 ms.
- **The modal truncation's bass ceiling**: 64 partials cap the bottom
  octave and a half's brightness (A0's 64th partial sits at 2.68 kHz on
  grand, 2.17 on concertGrand); `UP_MAX_MODES` 96 is the lever, at a
  voice's cost.
- **The pickup's far end** reads 2.85 % thd at E4 (2 asked) and 3.78 % at
  E3 (3 asked) through the voice, the tine and its tonebar swinging the tip
  into the curve.
- **The tonebar rings kAftersound longer than the tine**, so rhodes at E3
  sleeps at 3.32 × its tine's t60 (the tine alone at 1.33 ×).
- **One voice re-struck at another note retunes what it rings, and grows
  in neither output.** A ringing partial moved up keeps its velocity ω |z|
  (its displacement × hz_old / hz_new), one moved down keeps its
  displacement (its velocity falls), so a key jump is bounded both ways:
  grand's C3 jumped to C5 on one voice, the tip's velocity sum moves −0.45
  dB and the board's output +0.86 dB (they rose 11.5 and 9.2 dB with the
  partials left as they were, stage 3b); C5 jumped to C3, the board falls
  5.4 dB and the tip's velocity 12.6 (each over four periods of C3). Keeping velocity
  both ways would have carried a treble string's speed into the bass as
  millimetres, the board rising 3.9 dB from C5 to C3 and the thrash's raw
  board peak 1.24. The thrash (every 100 ms on one voice a random shape,
  each instrument channel drawn between the nine presets' lowest and
  highest values, the tonebar 1 − |g|, and a random note) peaks 1.71 on
  the pickup, over the 1.5 it is held under (a known row), and 0.085 on
  the board. A glide up keeps its velocity: grand's C4 gliding a fifth to G4
  ended 3.5 dB quieter through the board than with the partials left as
  they were (the displacement × 2/3, stage 3b), its click measure 0.24
  against the bound 4.18; a glide down keeps
  its displacement: the bridge force holds and the pickup's voltage falls
  by the pitch's ratio. A
  pool re-strikes a voice only at its own key; the single wrapper
  (01_Grand, 02_Channels) moves its one voice between keys. The wrappers
  clamp.
- **The generator's middle**: at −0.5 the bridge's fundamental is 11.8 dB
  under the string's (half the weight by design, the tine's gain near its
  clamp); in the string-to-tine half the pickup reads a string's partials,
  thd at velocity 1 of 98 .. 298 % through the chart's simulator.
- **The silence gate is absolute** (41): a pianissimo treble strike under
  1e-9 mm² sleeps within its first block (velocity 0.1 is 0.07 m/s, under
  any pianissimo: the key does not speak).
- **The bass sleeps late**: grand at C2 sleeps 151 s after a strike (the
  aftersound rings at the internal loss, the bridge's share taken out of
  the capped 37 s), C4 49 s.
- **`fortepiano` holds the gauge at 1**: through this rank the plan's thin
  gauge would lower a plain wire's B (the gauge moves only the wire's
  stiffness), the opposite of Bank's fortepiano.
- **A strike lands one block late** (2.9 ms), as every write; two strikes
  in one block are the second.
- **The steal's latency is the fade**: a stolen voice's new note sounds 78
  ms after its key at the default `stealRate`.
- **Keys struck in one block add their onsets**: eight keys evenly across
  a preset's range at velocity 1 peak 0.22 (wurlitzer) to 2.27
  (rhodesBark) at a gain of 1 (grand 0.59, rhodes 0.97); the pool's sum is
  unclamped and its wrapper clamps (03_Keyboard takes 0.4: at most
  0.91).
- **A glide flickers through the board**: a partial sweeping the plate's
  Q-33 resonances rises and falls with each, and above them with the
  field's grain.
- **The plate's rebuild runs in the audio interrupt**: the 32 frequencies,
  decays, radiations and output weights from the mode pairs found once, the
  field's sixteen losses (two pows and a sqrt a line), and the moved modes'
  turns, 0.3 .. 0.4 µs on the host (0.03 .. 0.04 of a voice's block); the
  voice wrapper every block while its eased body moves after a
  write, the pool wrapper once per body write under its drain's mask. Its
  cost on a board is unmeasured.
- **Nothing has run on a board.** The wrappers' interrupt masks, the
  queue's wait and drop, flush-to-zero in the interrupt's context and the
  cost are compiled, not measured (`99_Benchmark`).

## Credits

Upright is MIT (`LICENSE`) and contains no third-party code; `NOTICE` names
every source in full. The physics and the numbers: Bank, Zambon and Fontana
(the modal string, the hammer's discretisation, the secondary resonators,
the soundboard as parallel sections); Chaigne and Askenfelt (the hammer's
power law, its masses, stiffnesses and exponents, the loss parameters);
Chaigne (measured decays); Chabassier, Duruflé and Joly (a grand's scale,
hammers and damping, spruce's constants); Jaatinen and Pätynen, and
Hinrichsen (measured inharmonicity per key); Askenfelt and Jansson (hammer
velocities and contact times, the key's bed); Conklin (scale design, the bass break);
Fletcher and Rossing, Fletcher, Blackham and Stratton, and Young (the stiff
string, the strike point, soundboards); Suzuki, Ege and Boutillon (a
soundboard's modes); Weinreich (coupled strings, the two-stage decay);
Giordano (a soundboard's impedance, the bridge's mobility); Bank and
Sujbert (the string's longitudinal modes);
Pfeifle, Münster and Pfeifle, Falaize and Hélie, Shear and Wright (the
Rhodes tine, its hammer, its pickups, the Wurlitzer reed); Irvine (the
cantilever's roots). The digital forms: Mutable Instruments' Rings (the
ceiling), read, not copied; Jot and Chaigne ("Digital delay networks for
designing artificial reverberators", AES Convention, 1991: the feedback
delay network and its loss in each line) and Jot (the Householder feedback
matrix), the field's; a modal board below and a delay network above it is
this library's own joining of the two. The house: the author's earlier
libraries, whose rotation form, channel table, pool and wrappers this
follows; and PJRC's Teensy Audio Library, whose `AudioStream` the wrappers
implement.

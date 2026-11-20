# The public surface

Everything is in namespace `up` but the two Teensy wrappers. `#include <Upright.h>` brings in the
whole core and, on a Teensy, the wrappers. The words used here (channel, shape, tuning, the rank,
the law) are defined at the top of [design.md](design.md).

## One voice: `up::Voice`

One voice is one key's strings (`up_voice.h`). Every call writes channel targets (or the tuning,
or a strike, or the key's state) and lands at the top of the next block; the last writer wins, per
channel, in the caller's order.

```cpp
explicit Voice(uint32_t seed = 1, float fs = kDefaultFs);
void init(float fs);                      // a new rate: asleep, every phasor zero, the channels kept
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

A partial written by hand stands until the instrument is written again: the law writes every
partial under the sequence of the newest instrument write, so an instrument write after a hand's
(or a table's) rewrites it. Two `noteOn`s in one block are the second. A strike while the hammer
still touches places a fresh one; a key struck while its string sounds meets the moving string.

## The channels and the presets

`up_channels.h` and `up_presets.h`; the channels themselves are tabled in
[design.md](design.md#the-channels).

- `nanShape()`: every channel nan, a shape to fill in by hand, in which only the channels written
  move.
- `defaultShape()`: grand's, with its partials at middle C, every voice's start.
- `channelRange(c, lo, hi)`, `channelName(c)`, `channelByName(name, c)`, `groupOf(c)`,
  `isInstrumentChannel(c)`.
- `grand()` .. `wurlitzer()`: each preset's shape, built once on first use from its table.
- `presetCount()`, `presetAt(i)`, `presetByName(name)`: the table of `PresetInfo { name, shape, lo,
  hi }`, `shape` the preset's accessor, called as `p.shape()`; `presetByName` returns null for a
  name it does not know.

## The board: `up::Board`

`up_board.h`. `init(fs)`; `reset()`; `setPlate(area, loss, open)` (the eased body channels,
recomputed where they moved); `inputWeights(midi, w)` (the plate's shape where the note's string
crosses the bridge); `render(forces, w, voices, out, n)` (each voice's bridge force in at its
weights, the plate's modes turned, their radiated velocity, the direct share and the field above
them written); `setShelf(gain)` (the shelf's mobility, scaling the direct share and the field: the
tool's sweep); `fieldStep(force)` (one sample of the field alone, no highpass: the tool's and the
tests' measure); `fieldLine(i, g, a)` (a line's loss); `amplitude(k)`, `modeInfo(k)`, `shape(x,
y)` (the plate's displacement at a point), `table()`.

A single voice's player (the tool, the voice wrapper) runs a board of its own on the voice's bridge
force and adds the pickup: the output, not clamped (the wrapper clamps what it sends). A pool runs
one board on all its voices.

## A keyboard: `up::Pool<M>`

`up_pool.h`: the voices, their note table and one board inside it.

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

`up::PoolQueue` (`up_pool.h`) holds a pool's calls in order for the thread that renders it: the
pool's player and pool-wide writes, each returning false when there is no room (64 commands, and
two each of shapes, partial tables and tunings), `drain(pool)`, which plays them into the pool in
order, and `count()`. It masks and waits for nothing: the caller keeps the writer and `drain()`
apart, as the pool wrapper does with an interrupt mask.

## On a Teensy

`AudioSynthUpright.h` and `AudioSynthUprightPool.h`, each whole inside `#if defined(ARDUINO)`, so
on the host they are empty.

- `AudioSynthUpright` (`explicit AudioSynthUpright(uint32_t seed = 1)`) is the voice's surface one
  for one: the player, the channels, the conveniences, the tuning (`setTuning`, `tuning()`), and
  `state()`, `shape()`, `partials()`, `stringShape()`, `rank()` read back. An `AudioStream` with no
  inputs and one output: the voice's bridge force through a board of its own, its pickup added.
- `AudioSynthUprightPool<M>` (`explicit AudioSynthUprightPool(uint32_t seed = 1)`) is the pool's
  player (`noteOn`, `noteOff`, `pedal`, `allNotesOff`) and its pool-wide writes (`set`, `setRate`,
  `setShape`, `setPartial`, `setPartials`, `setTuning`), each returning false only when its
  command was dropped, and `voices()`, `state(i)`, `slot(i)`, `sounding()`, `dropped()` read back.
  One output: the pool's board with its pickups added.

Every call lands at the top of the next audio block (128 samples, 2.9 ms), a strike too. Each
wrapper clamps what it sends. The wrappers set flush-to-zero for the audio interrupt at
construction; allocate them statically and never destroy them, as every `AudioStream`. The seed
starts each voice's xorshift32, which nothing in the engine draws: any seed renders the same
bytes.

## `up::State`

`up_telemetry.h`: the eased channels, f0, the rank, the main string's partial amplitudes |z_k|
(mm) and the secondaries', the attack's longitudinal modes; the hammer's height, velocity, the
block's peak force, touching, the strike's contact so far; the block's bridge rms (N), pickup rms
and the pair's rms; the loudest partial, the partials on; released, damped, the eased pedal;
asleep.

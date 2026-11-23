# How Upright works

A struck string is a mode table, driven by a hammer in a loop, heard through a body or a pickup.
You do not pick a piano; you set its scale, its hammers, where they strike, how many strings a note
has, its body, what the hammer hits and how it is heard, and how it is damped, and the sound
follows. An upright, a grand, a Rhodes and a Wurlitzer differ by numbers, not by mechanism.

## Words used here

- **Voice** (`up::Voice`): one key's strings, their hammer and their pickup. **Pool**
  (`up::Pool<M>`): M voices, a note table and one soundboard they share.
- **Channel**: one number a voice holds, as a target it eases toward at a rate. The **24
  instrument channels** are the instrument's parameters (the strings, the hammer, the body, the
  generator, the damper: [the table](#the-channels)). After them come four blocks of partial
  channels (each partial's ratio, t60, gain and output weight) and three player channels (the note,
  the glide, the pedal). `up::Channel` names them all.
- **Shape** (`up::Shape`): a full set of channel targets, one value per channel; a nan entry is no
  write. A preset is a shape. Not to be confused with `Board::shape(x, y)`, the plate's
  displacement at a point, or `stringShape()`, the string's displacement along its length.
- **Tuning** (`up::Tuning`): the engine's rates and settings (each channel group's ease, the
  hammer's throw, the output gain, the silence gate, the steal's fade, the ceiling, the pickup
  amp's pole, the attack). It holds no pitch: the pitch is the note, equal-tempered at A4 = 440 Hz.
- **The rank**: the string at a given note, derived from the instrument channels, which are given
  at C4.
- **The law**: the pure function from the instrument channels and the rank to every partial's
  ratio, t60, gain and output weight.
- **Slot**: slot k holds partial k + 1, in every build.
- **The generator**: −1 a string, 0 a Rhodes tine, +1 a Wurlitzer reed, and every value between.
- **Block**: 128 samples (2.9 ms at 44.1 kHz), the control rate: every call lands at the top of the
  next one.

## One block

`up_voice.h` carries it whole. In order:

1. the tuning, when one was set;
2. the law: when an instrument channel was written since its last run, or the note's target moved,
   every partial channel's target under the newest instrument write's sequence, from the targets at
   the target note;
3. wake: a sleeping voice with a strike queued wakes, every channel on its target, the banks
   zeroed, the note jumping; asleep with none, nothing;
4. the channels tick (awake);
5. the pitch axis (`up_pitch.h`): the eased note at the glide → f0;
6. the rank at the eased note (`rankAt`, when it or a channel it reads moved);
7. the hammer: the eased felt channels at the rank's hardness scale and power, the tine's tip by
   the generator, the rank's mass (the felt's table rebuilt only where they moved);
8. per slot: hz = ratio × f0; on under `ceiling × fs` with a gain or an out and a T60; its own
   decay σ_n = 6.907755 / (t60 × t60Scale), less the bridge's σ_b on the first kUnisonModes slots
   (held at a tenth of σ_n at least), and, released on a key with a damper, the damper's rate
   (`dampRateOf`, × (1 − the eased pedal)); the drive scale gain × 2 dtSub / (μ L ω) × 1000; the
   readback weight the gain; the output weight the out. Then the secondaries at the detune, the
   coupling at σ_b; the hammer's point mass (μ L / 2) / Σ gain²; every bank's `beginBlock`;
9. a strike due: the hammer placed at the string at `hammerScale × vel^velCurve`;
10. the pair's weights, ramped across the block: the bridge's clamp(−g, 0, 1) × the rank's bridge
    scale / 1000 × outputGain, the pickup's clamp(1 + g, 0, 1) × outputGain; a weight at 0 at both
    ends compiles its sums out;
11. the attack (`up_attack.h`) where the bridge is heard: its longitudinal modes set at the rank's
    string, a strike due arming its thud at the rank's hammer mass × the throw (the blow's share
    follows the hammer's force), its weight clamp(−g, 0, 1) × outputGain ramped beside the
    bridge's; with nothing to add it is compiled out;
12. 128 samples: while the hammer touches, kContactRate substeps a sample of the hammer's loop;
    from the sample it leaves, every bank's turn; the bridge force with the attack's added, and the
    pickup's curve and pole on the tip's sums;
13. the telemetry; the silence gate: every string bank's energy under `silence` (mm²) at the end of
    a block with no hammer touching and no strike queued zeroes the banks and the attack and
    sleeps.

Control runs every 128 samples whatever size `render()` is called with, so the same calls give the
same bytes at any render size. No heap, no libm per sample (the test `config_core_guard` reads the
sources and names every function allowed it).

**Units.** A string's partial is a phasor in millimetres of modal displacement (`re`), its
quadrature `im` (ω im its velocity in mm/s): a fortissimo string swings O(1) mm, and the silence
gate's 1e-9 mm² is −90 dB of a millimetre. The hammer's step is SI; the voice turns the readback
mm → m going in. The bridge force is the rank's π T / L × Σ out re / 1000: newtons, and the
attack's force, already newtons, is added at clamp(−g, 0, 1) × outputGain. The pickup's tip is Σ
out re (mm) and Σ out ω im (mm/s) over the first kTineModes slots of every bank. The board's modes
are micrometres and it is heard as their velocity, µm/s.

**The bank** (`up_bank.h`): N damped rotation oscillators (a template: the main string's
kMaxModes, a secondary's kUnisonModes, the board's kBoardModes), in the increment form z += z (ca −
i cb), ca = r cos ω − 1 from expm1 and sin²(ω/2), cb = r sin ω, turning clockwise so a drive into
im pushes re positive; a second coefficient pair per partial at dt / kContactRate for the hammer's
substeps; no doublets. Per sample: the readback Σ gain re and its velocity Σ gain ω im, the
injection im += F drive, the turn, the sums (the bridge's Σ out re over every partial, the tip's Σ
tip re and Σ tip ω im over the first kTineModes slots in the same pass, tip the out held to ±1: a
pickup reads the tip's displacement, whose modal weights are bounded by the shape's 1, where the
bridge's slope grows with n); the output weights ramp across the block, the tip's beside them.

A partial that moves grows in neither output: one ringing since the last block whose frequency
moved has its phasor scaled by min(1, hz_old / hz_new) (re and im, one division). Moved up it keeps
its velocity ω |z| (the pickup's voltage and the board's modes read velocity), moved down its
displacement |z| (the bridge force reads displacement), so no output grows through a glide or a
key jump either way, as no piano gets louder when a string is retuned. One moved from 220 to
880 Hz reads |z| 0.25 and its velocity unchanged; one from 220 to 110 Hz keeps |z| and its velocity
halves; the board's modes take the same rule as the body channels move. One partial at 440 Hz,
27.5 Hz and 15 kHz rings within 0.003 cent of its frequency and 0.1 % of its T60.

**The hammer's loop** (`up_string.h`, `up_hammer.h`): while the hammer touches, every sample is
kContactRate (2) substeps of { the main string's readback (y and its velocity, as the phasors
stand); the force; every string's partials take it; every bank turns by the substep's pair }, then
the sums. The force is K c^p by the felt's table, its loss solved against the string's point mass
under the hammer: F = a (1 + λ u) / (1 + λ a dt / (2 m_point)), u the hammer's velocity less the
string point's own, a the table's K c^p: one division, stable at any step and second order. Against
the simulator at 32 fs (`up_contact.h`, double, explicit, powf; 32 fs against 64 fs within 0.11 %),
grand through the voice at velocity 1: C4's force within 0.70 % of its peak at every substep, its
contact 0.38 samples short, its first ten partials within 0.18 dB; C2, C6 and C8 within 8.6 % on
the peak (C8's, under the treble's stiffer felt) and 0.5 samples on the contact. At C4 the ladder
0.1 .. 1 (0.07 .. 7 m/s) reads contacts 4.22 .. 2.10 ms and peaks 0.20 .. 29.6 N (Askenfelt &
Jansson's C4: 3.0 ms at 0.6 m/s, 1.84 at 7). A re-strike meets the moving string: the hammer placed
at the rest, or where the string stands when it is nearer.

**The attack** (`up_attack.h`): the sounds a hammer makes that are not the string's partials, each
a force in newtons on the bridge on the string side at its own weight (clamp(−g, 0, 1) ×
outputGain, ramped beside the bridge's), so it reaches the board as the string's force does (its
modes through the note's bridge point, the direct share and the field), and the pickup never hears
it. Each is calibrated at grand's C4, velocity 1, as its own output through the board against the
string's own over the same window.

- The knock is the blow's share, kKnockForce (0.983) × the hammer's force while it touches (the
  reaction through the shank, the rail and the frame: the hammer's own pulse at the felt's own
  velocity dependence): 10 dB under the string over the first 10 ms (−10.13 dB measured; −6.96 at
  C2, −8.86 at C6). With the thud and the clank the attack's own reads −9.99 dB there (held within
  1.5 dB of it; −6.30 at C2, −8.89 at C6), and from velocity 1 to 0.5 the knock falls 12.63 dB at
  C4 where the string's own falls 12.44 (C2 12.16 against 11.75; C6 12.44 against 14.37).
- The thud, the key reaching its bed (Askenfelt & Jansson): a raised cosine 10 ms wide from 2 ms,
  its peak kThud (6.75) × m_h v0 over its width (m_h the rank's hammer mass, v0 the throw); the
  board's band under 150 Hz over 5 .. 40 ms reads 4.7 dB over attack 0 at C4 (the thud alone 8.9
  dB over the string's own there, the blow alone 7.0: two pushes 2 ms apart, partly cancelling in
  the plate's lowest modes).
- The clank: a string's longitudinal waves travel at c_L = sqrt(E A_core / μ) (5048 m/s on a plain
  wire; a winding adds mass, not stiffness), so its free longitudinal modes sit at k c_L / (2 L), k
  times the rank's `longHz` (685 Hz at A0, 1208 at C2, 4110 at C4, 14.6 kHz at C6, over the
  ceiling from C7), ringing 0.4 s (kLongT60), kLongModes (4) of them. The string's stretching, (E
  A / 2) ∂/∂x (∂y/∂x)² with the slope F / (2 T) under the hammer over a share kLongShare of L about
  it, projected on mode k (modal mass μ L / 2, ω_k = k π c_L / L), puts F² cos(k π β) E A_core
  kLongShare dtSub / (4 T² μ c_L) into its im a substep (the k and the L cancel), and the bridge
  reads the longitudinal force through the string's bearing angle, kLongAngle (0.08) × E A_core (π
  / L) Σ k (−1)^k re_k. kLongShare (0.0912) sets grand's A0 at velocity 1 to read its clank 10 dB
  under the note over the first 50 ms. Against the note there it reads −10.0 dB at A0, −41.5 at C2,
  −46.5 at C4, −32.6 at C6 (−25.6, −58.7, −63.9, −56.9 at velocity 0.5), each at the rank's
  frequency to 0.001 %: a blow twice as hard drives it four times as hard, and its modes over the
  contact's bandwidth make it steeper still (from velocity 0.5 to 1 it rises 30.0 dB at C2 where
  the strongest partial rises 12.0).

`Tuning.attack` scales the three, 0 .. 2, 1 the calibration; at 0 nothing of it runs, and the
strings render the same bits as at 1. The phantom partials (the forced longitudinal response at the
transverse partials' sums and differences) are not built.

**The unison** (`up_string.h`, after Weinreich's "Coupled piano strings", JASA 62, 1977): the
second and third strings carry the main's first kUnisonModes slots, the second at 2^(+detune /
1200) and the third at 2^(−kThirdDetune × detune / 1200) (kThirdDetune 0.7: it never shares the
second's frequency), their own decay the main's and the damper's alike. On the string side the
three are equals at their presence (the strings channel's fractions): each struck, coupled and
heard by it, so a string fading in is the same fraction of a string throughout. They end on one
bridge, an admittance Y (`bridgeMobility`, 3.5e-4 m/s per N) with a reactive part
`bridgeReactive` (0.5) of its resistive one: each string pushes it with its impedance times its
velocity, the bridge moves at Y times the sum, and each string feels that motion.

Per partial under kUnisonModes, before every turn, S = im₁ + w₂ im₂ + w₃ im₃ (the velocities' sum),
R = re₁ + w₂ re₂ + w₃ re₃ (the displacements') and im_j −= w_j (c S − c_r R): c = 2 σ_b dt, σ_b
the rank's bridge loss T Y / L (0.12 /s at A0 and C2, 0.32 at C4, 1.12 at C6, 4.0 at C8, the same
on every partial), a symmetric damping matrix that only dissipates; c_r = 2 δ dt, δ = σ_b ×
bridgeReactive, which lowers the in-phase motion's pitch (a mass-like bridge: the strings' common
motion carries its mass). The voice hands the bank σ_i = σ_n − σ_b, so one string alone decays at
the law's σ_n; three in phase, as the hammer leaves them, at σ_n + 2 σ_b, the prompt sound; the
strings against each other at σ_i alone, the aftersound, which barely moves the bridge and keeps the
string's own pitch.

Two equal strings detuned by Δf (ε = π Δf), with κ = σ_b (1 + i bridgeReactive), have their modes
at σ_n ∓ sqrt(κ² − ε²): while ε is under σ_b (551 σ_b / f0 cents: 0.66 at C4, 1.0 at C2, 2.4 at
A0, 0.59 at C6, 0.52 at C8) they lock, two decays (its real part) and no beat; past it they beat
at |Im sqrt(κ² − ε²)| / π, under the free rate. On a purely resistive bridge the hammer's symmetric
strike would put the locked slow mode at the bridge in opposite phase to the fast one, so the sum
would fall through a null where they cross; the reactive part shifts the in-phase mode's pitch and
not the others' (n strings in phase about n δ under the aftersound), so the two turn against each
other and never cancel. The detune in hertz grows with the partial and the coupling does not: at
the tuned 0.5 cents C4's fundamental locks and its upper partials beat, slowly.

Measured (`test_unison.cpp`, every expected number from the constants): one string at C6 decays at
−18.56 dB/s against −18.56; three in phase at −38.03 against σ_n + 2 σ_b's −38.03; two at 0.3 cents
at C4 have no dip (the largest, each 10 ms frame against the lower of the frames a second either
side, −0.67 dB), −7.20 dB/s over the first second against the fast decay's −6.84 and −1.89 from
10 s against the slow's −1.81; two at 2 cents at C6 beat 1.1747 Hz against |Im sqrt(κ² − ε²)| / π's
1.1725, 3.2 % under the free 1.2113 (a resistive bridge's 1.1576); grand's three at C4 pass from
the prompt sound into the aftersound at about 2.8 s without a dip, the prompt sound at −10.48 dB/s
(σ_n + 2 σ_b's −9.80) and 0.63 cents under the aftersound's pitch (3 δ / 2π: 0.50), the aftersound
from 10 s at −1.54 dB/s against σ_i's −1.59, its own two modes beating once, 6 dB deep at 7.7 s;
C4's 4th partial on two strings beats 0.2919 Hz against the formula's 0.2909; C4's 8th partial is
60 dB down by 1.9 s (its tracked t60 1.60 s).

On the tine side (u = clamp(1 + g, 0, 1)) the secondaries' outs are × lerp(1, kUnisonGain, u) and
their decays over kAftersound^u, and nothing couples (σ_b and δ × clamp(−g, 0, 1)): the tonebar,
heard at half the tine and ringing 2.5 times longer, beats with it at the free rate (rhodes at E4,
its tonebar at 1.51 cents: 0.28832 Hz apart against the free 0.28833).

**The pickup** (`up_pickup.h`): the `Pickup` stage on the tip's sums (the outs held to ±1), one
Lorentzian whose centre is set per block (the magnet's offset at the tine, slid to the gap by
clamp(g, 0, 1) toward the reed), its derivative × x' with one division a sample, the amp's pole, ×
kPickupGain (1.36e-4: rhodes at E4, velocity 1, peaks 0.29). Rhodes at E4 through the voice is the
simulator's tip through the same curve within 0.06 % rms.

**The board** (`up_board.h`): a Bank of the plate's 32 modes (which (m, n) are the lowest does not
depend on the area at the fixed aspect, every frequency scaling by 1 / area, so the 40 × 40 search
runs once and a body move evaluates the 32 frequencies from the pairs it found), each voice's
bridge force in at its bridge point, drive dt / (M ω) × 10⁶ (µm; M = ρ h a b / 4); heard as the
modes' velocity, Σ outW ω im (µm/s; outW the radiation × the listening shape).

Above the top mode (f_top, 764 Hz on grand's board) the voices' summed force goes through two
one-pole highpasses at f_top into two paths beside the modes. The direct share is that force ×
kFieldDirect (0.316, −10 dB) of the shelf's mobility kBoardShelf (3133 µm/s per N; the infinite
plate's mobility 1 / (8 sqrt(H ρ h)) is 3604): the spreading wave's first arrival. The field is the
plate's diffuse response where its modes above the top one overlap, each ringing t60 = 6.907755 /
(π f η): a feedback delay network of kFieldLines (16) lines, 59 .. 281 samples long (1.3 .. 6.4 ms
at 44.1 kHz), 2602 in all end to end in one buffer: one network mode per 17 Hz, near the plate's
own density at 1 m² ((a b / 2) sqrt(ρ h / H): one per 19 Hz). Each sample the lines' outputs d_i
are mixed by Householder, y_i = d_i − (2 / N) Σ d_j (orthogonal: lossless), each y_i goes through
its line's loss and is written back with the force added at 1; the field is the d_i summed at +1,
−1, +1, ... × its gain. The loss is a one-pole in each line's loop, g_i (1 − a_i) / (1 − a_i z⁻¹),
set per plate rebuild in closed form from η and f_top: g_i, its gain at dc, the pass's loss at the
top mode's t60, 10^(−3 L_i / (t60(f_top) fs)) (under 1: stable), and a_i so its loss at
kFieldFitHz (4 kHz) is the pass's at the plate's t60 there. The gain is kFieldGain (715.8 µm/s per
N) × the shelf's mobility / kBoardShelf, the same on every board: kFieldGain is measured so the
field's impulse energy (a unit impulse in, no highpass, 0.5 s) on grand's board is (1 −
kFieldDirect²) kBoardShelf², 8.836e6 (µm/s per N)², and the field and the direct share together
carry the power the shelf carried (a flat gain G has Σ h² = G²); on the spinet's board and the
concert grand's the energy reads −0.25 and +0.64 dB from it.

The output is the sum × kBoardOut (1.235e-6: grand's octave C4 .. B4 at velocity 1, each note
0.6 s, its first half second's rms, the mean of the twelve powers, is 0.0897 rms of full scale,
−20.95 dB; grand's C4 alone peaks 0.18).

Measured (`test_board.cpp`): 1 N for one sample at C4's bridge point on grand's board puts 7.9 % of
the first 300 ms's energy in the first sample and 75.1 % within 5 ms (a flat shelf there: 81.5 % and
96.4 %); the board's power over f_top .. 2 f_top, 2 .. 4 and 4 .. 8 f_top against a flat shelf's at
kBoardShelf reads +0.88, −0.72, +0.03 dB on the spinet's board, +0.18, +1.13, +0.28 on grand's and
+3.61, +3.21, +0.70 on the concert grand's (the tilt: [limits.md](limits.md)), the dB's standard
deviation 4 .. 7 dB in each band (the grain); every line's loss meets the plate's per-pass loss at
dc and at 4 kHz within 1.1e-6, and between them the lines' mean t60 at 1.5 f_top is 0.69, 0.76 and
0.97 of the plate's on the spinet's, grand's and the concert grand's boards (a one-pole falls
faster than 1 / f); 100 N rms of white noise for half a second, on every board and at 22.05, 44.1
and 96 kHz, leaves every sample finite and the output under −100 dBFS within 0.29 s of its end.
The field holds 10,668 bytes (the lines' 2602 floats, each line's read point, loss and state, the
gain): a Board is 14,184 bytes, `Pool<8>` 182,096. Through it grand at velocity 1 peaks 0.33 at C2,
0.18 at C4, 0.19 at C6. A mode alone (the shelf's mobility 0, so neither the direct share nor the
field) rings within 0.0001 cent and 0.2 % of its table; two voices are the sum of each; a closed
back is 15.8 dB under an open one at 60 Hz.

**The pool** (`up_pool.h`, [below](#the-pool)) runs once per 128 samples too: its own block (the
stolen voices, the frees, the levels), then every voice's pair for every sample into scratch at its
level, each bridge force into the one board at its bridge point's weights, the board once, the
pickups summed.

**On a Teensy** each wrapper's `update()` runs in the audio interrupt. The voice wrapper latches
its pending table with interrupts masked and renders unmasked, its board on the bridge force (the
plate from the voice's eased body when it moved, the weights at its bridge point when the note
moved), the pickup added: the plate is rebuilt every block while the eased body moves after a write
(seconds at `rateBody`), each rebuild the 32 frequencies from the mode pairs found once, the
field's sixteen losses and the moved modes' turns, about 0.02 of a voice's block on the host. The
pool wrapper turns its setters to its other command queue masked, then drains the one they wrote
(the plate rebuilt once per body write there) and renders, unmasked. Each clamps what it sends,
scales it to int16 and transmits it on output 0, and renders the block even when `allocate()`
returns NULL, so the voices and the board keep time.

## The modules

| | |
|---|---|
| `src/Upright.h` | the umbrella: the core, and on a Teensy the two wrappers |
| `src/AudioSynthUpright.{h,cpp}` | one voice and its board, one output: the pending table under an interrupt mask |
| `src/AudioSynthUprightPool.h` | the pool, a template on M, header-only: two `up::PoolQueue`s, turned under an interrupt mask |
| `src/core/up_config.h` | the build's sizes and their link check, clamp, `finiteOr`, the one smoothing law, `Ramp`, xorshift32, `midiToHz` |
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
| `src/core/up_pool.{h,cpp}` | the pool: the note table, the steal, pool-wide writes, the shared board; `PoolQueue`, its calls held for another thread |
| `examples/` | `01_Grand`, `02_Channels`, `03_Keyboard`, `99_Benchmark` |
| `extras/host/` | the Makefile, `tests/`, `uptool.cpp`, `up_wav`, `up_png`, `up_analysis.h`, `up_play.h`, `up_fit.h`, `up_contact.h` ([host.md](host.md)) |
| `extras/reference/` | the reference tables, plain text, with `README.txt` |
| `extras/docs/` | this file, [reference.md](reference.md), [host.md](host.md), [limits.md](limits.md) |
| `README.md`, `LICENSE` (MIT), `NOTICE`, `library.properties`, `library.json`, `keywords.txt` | |

`src/core/` includes nothing but the C++ standard library and its own files, holds nothing on the
heap, and calls no libm per sample (the test `config_core_guard` reads its sources). The wrappers
are the only files that include anything of Arduino's.

## The channels

A piano is its twenty-four instrument channels (`up_channels.h`), in five groups, each a target, in
physical units where it has them, that the voice eases toward at its group's rate; the law writes
every partial from them at the note ([the law](#the-law)). Each channel's range is
`channelRange`'s: what a control can sensibly ask for, and what the tests drive it between. Every
value inside it is legal. The tool draws, sweeps and fits each along it, on a log scale where it
spans a decade or more (`channelSpan`, `up_fit.h`).

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

**The rates** (`up::Tuning`, `groupOf`, `groupRate`): the strings group eases at `rateStrings`
(8 /s), the hammer at `rateHammer` (8), the body, the generator and the damper at `rateBody` (4: the
instrument's slow things), every partial channel at `rateModes` (8), the note at `ratePitch` (0: a
jump, the glide channel easing it) and the pedal at `ratePedal` (30). A rate at or under zero
jumps. `setRate(channel, perSec)` sets one channel's rate, and the tuning's group rates stand on
every channel whose rate nobody set.

**Setting a piano**, the same calls on `up::Voice`, `AudioSynthUpright`, `up::Pool<M>` and
`AudioSynthUprightPool<M>`:

```cpp
piano.setShape(up::grand());                          // a preset's shape, written whole under one sequence number
piano.setShape(up::presetByName("rhodes")->shape());
piano.set(up::Channel::hardness, 300.0f);             // one target
up::Shape s = up::nanShape();                         // every channel nan: no write
s[up::Channel::tineT60] = 30.0f;
s[up::Channel::tineLoad] = 1.6f;
piano.setShape(s);                                    // only the channels written move
piano.setRate(up::Channel::boardArea, 0.5f);          // one channel's ease, per second
piano.setTuning(t);                                   // the group rates, and the engine's settings (Tuning)
```

A voice and its wrapper also take the conveniences that write a group's channels together:
`setStrings(length, bass, tension, gauge)`, `setUnison(strings, detune)`, `setHammer(hardness,
exponent, felt, massG)`, `setStrike(share)`, `setBody(area, loss, open)`, `setGenerator(g, gap,
offset)`, `setDamper(rate, top)`.

**The rules.** A nan is no write: a shape's nan channel stands where it was (a preset's partial and
player channels are nan, so `setShape` of a preset leaves the note, the glide and the pedal alone).
The last writer wins per channel by its sequence number: every write takes the next one, a shape
lands whole under one, and between the caller's table and the audio side's the newer write wins
channel by channel. A partial written by hand stands until the instrument is written again: the law
writes every partial under the sequence of the newest instrument write. Every value inside
`channelRange` is legal.

**The tine's two channels.** `tineT60` is the tine's fundamental t60 at C4 (s): the overtones' t60s
are tineT60 × ratio^−tineSlope (tineSlope 1.2), the reed's decays kReedT60 (0.6) times the tine's,
and every t60 scales along the keyboard by the rank's t60Scale. `tineLoad` multiplies the beam's
overtone ratios, n ≥ 2: the tuning spring's mass at the tip, 1 the bare beam (its 2nd partial at
6.27), 1.18 putting it at 7.39 (Shear's measured 7.4). Every preset, and so every fresh voice
(`defaultShape`), carries 10.4 s (the fit against Shear's Q for a 1974 Mark I) and 1.18. A small
tine that pings is rhodes with tineT60 2 and tineLoad 1; a big one that rings long with its
overtones stretched, tineT60 30 and tineLoad 1.6 (`02_Channels`' `tine 2 1` and `tine 30 1.6`).
Both ease at `rateBody` with the generator group and are inert on the string (generator −1). A nan
tineT60 in a shape handed straight to `partialsOf` holds at 1 ms, and a nan or non-positive
tineLoad reads as the bare beam's 1.

**The floors, measured** (`tests/test_floors.cpp`): each group of channels from one end of its
reach to the other alone at grand (rhodes for the pickup), every other channel at the preset's,
rendered 4 s at velocity 1 as `play` renders it, the partials tracked at the law's frequencies; the
floor is what the test holds.

| channels | from → to | the measure | measured | the floor |
|---|---|---|---|---|
| strings | the spinet's length and bass (0.601, 1.22 m) → a concert grand's (0.627, 2.13 m) | the 10th partial of C2, cents off 10 f0 | 16.8 cents (+20.9 → +4.1) | 15 cents: the tenor's span is the data's, ±2 %, and C2 is wound on every one |
| hammer | hardness 25.1 N/mm^p, felt 1e-3 s/mm → 803, 2.5e-4 (Chaigne & Askenfelt's C2 hammer's stiffness to their C6's) | the force's band (partials 8 .. 16 against 1 .. 3, the render's partial levels) at velocity 1, at attack 0 | 4.3 dB (−23.1 → −18.8; 4.6 at attack 1) | 3 dB |
| hammer | the same | the same at velocity 0.5 | 12.2 dB (−33.0 → −20.8; 12.3 at attack 1) | 10 dB, at attack 0: the string's own partials, without the attack's force in the tracker's first frame |
| strike | strikeShare 0.04 → 0.5 | the 8th partial, its band's power over 0.1 .. 1 s | 80.3 dB (−10.3 → −90.6: the node at the middle) | 20 dB |
| unison | one string → three 20 cents apart | the fundamental's beat depth (16 s) | 29.9 dB (0 → 29.9; the tuned three 4.2) | 6 dB |
| unison | one string → three tuned (0.5 cents) | the aftersound's ratio, early slope over late (16 s) | 5.60× (1.00 → 5.62; the wide three 0.24) | 2×, on the tuned three: on the wide three the beats swamp the two-slope fit |
| board | the spinet's (0.62 m², loss 0.049, closed) → a concert grand's (1.62 m², 0.018, open) | the octave about 100 Hz through the board, at A0 | 11.4 dB (−37.6 → −26.2; at C2 16.2) | 8 dB |
| board | the same | the board's first mode (the table) | 1.40 octaves (81.2 → 30.8 Hz) | an octave |
| damper | dampRate 60.6 → 6.6 /s | the release's T60 (released at 1 s), read on one string | 8.70× (0.100 → 0.868 s; three strings 7.91×) | 8×, on one string: the three strings' two decays tilt their fit |
| pickup | gap 3 mm, offset 1.5 → rhodesBark's 0.75, 0.3 | rhodes's E3: the second harmonic against the first | 24.2 dB (−29.4 → −5.2) | 20 dB |
| pickup | the same | rhodes's E3: the thd | 3.78 % → 61.15 % | under 5 % at the far end (the tine and its tonebar swing the tip into the curve), over 15 % close |
| generator | grand → rhodes | the partials on at E3 | 64 → 6 | over 30 → 6 |

## The law

`partialsOf(shape, rank)` (pure) and the writer `Law::update(channels, rank, noteMoved)` (when an
instrument channel is newer than its last call, or the note moved, every partial channel under the
sequence of the newest instrument write; a partial written by hand later stands until the
instrument is written again). Once per instrument write and once per note, never per sample.

**The slots** are the partials in order: slot k is partial k + 1 in every build (`UP_MAX_MODES`,
8 .. 96, default 64). A build of 8 keeps the first eight partials: a piano with no brightness.

**The build's sizes** (`UP_MAX_MODES`, `UP_UNISON_MODES`, `UP_BOARD_MODES`, `UP_CONTACT_RATE`,
`up_config.h`) are set in the build's flags, which reach the library's sources and the sketch
alike: under PlatformIO, `build_flags = -D UP_MAX_MODES=48`. The Arduino IDE has no place for
them, so a build there has the defaults. A `#define` in a sketch reaches the sketch alone, which
would then lay out every object differently from the library's own sources; such a build stops at
the link, at an undefined reference to
`up::UP_MAX_MODES_and_the_sizes_belong_in_the_build_flags<48, 16, 32, 2>::built` (the sketch's
sizes), which the library defines only at its own.

**The string** (generator −1): ratios n sqrt(1 + B n²) with the rank's B; decays sigma_n = lossRate
/ t60Scale + lossRate (f_n / lossKnee)^lossSlope at the note's partial frequencies: the floor (the
bridge and the air) is what the rank scales, and the frequency term, the wire's own bending and
viscoelastic loss (Chabassier's b3 ω²), is nearly the same at every note (his b3 moves 2.4 times
over seven octaves where t60Scale moves 23 times), so it is not scaled (the table holds 6.907755 /
(lossRate (1 + t60Scale (f_n / lossKnee)^lossSlope)) and the voice's × t60Scale lands sigma_n; the
tine's t60s and a table by hand keep the rank's scaling whole); gains sin(n π share) at the rank's
strike share, signed; outs n (−1)^(n+1), the slope at the bridge read with the tip's sign,
dimensionless (the rank's bridgeScale, π T / L, makes them newtons): a string's out and the beam's
share a sign in each of the first six slots. Grand's C4: B 3.51e-4, the fundamental's t60 13.9 s
(one string), its 2nd partial's 10.7, 4th 5.6, 8th 1.92, 16th 0.50; the fundamental's at C2 37.7 s,
C6 3.23, C7 1.26, C8 0.41, key 127's 52 ms.

**The tine** (generator 0): the clamped-free beam, ratios (beta_n / beta_1)² (`tineRatio(n,
load)`), the overtones (n ≥ 2) × the `tineLoad` channel (the tuning spring's mass at the tip; at
the presets' 1.18 the one measured Rhodes overtone at 7.4, the bare beam's 6.27: 1 : 7.395 : 20.71 :
40.58 : 67.07 : 100.2) over the first kTineModes (6) slots, the slots above off (the beam's series
continued, (2n − 1) π / 2, so the table stays ordered); a nan or non-positive load the bare beam's
1; decays the `tineT60` channel × ratio^−tineSlope (held over 1 ms, a nan too); gains the beam's
shape at the strike share and outs its shape at the tip, both over the tip's 2.

**The reed** (generator +1): the tine's ratios and shapes, the overtones' gains at kReedOvertone
(0.05) of the beam's, decays kReedT60 (0.6) of the tine's.

**Between the anchors**: from −1 to 0 every slot's ratio and t60 along the geometric mean's path,
the first six slots' gains and outs linearly, the string's slots above six faded by clamp(−g, 0,
1); from 0 to +1 the overtones' gains toward the reed's and the t60s toward the reed's. No gain or
out of the first six crosses zero on the way (along −1 .. 0 by 0.05 at A0, C4 and C7). `stringsOf`
caps the strings at 3 to g −0.5, falling to 2 at 0.

**The ceiling**: a partial over `tuning.ceiling × kDefaultFs` at the note gets gain 0 and out 0
(off).

**The rank** (`rankAt(shape, midi)`): L = min(length (f_c4 / f0)^rankLength, bass); mu = T / (4 L²
f0²), Z = T / (2 L f0); the plain wire d = sqrt(4 mu / (π rho)) × gauge, wrapped past dMax on a
core of `core` × (mu / muBreak)^coreGrow (muBreak = π rho dMax² / 4: the core thickens as the
winding grows); B = π³ E dCore⁴ / (64 T L²); the hammer's mass hammerMass (f_c4 / f0)^rankMass
(4.2 g at C2, 2.97 at C4, 2.1 at C6), its felt's stiffness × (f0 / f_c4)^rankHardLo below C4 and
^rankHardHi from C4 up (Chaigne & Askenfelt's three hammers bend at C4: 0.75 below, 1.41 above; K
26.2, 50.2, 142, 1003, 7081 N/mm^p at A0, C2, C4, C6, C8) and its power + rankP log2(f0 / f_c4)
(2.16, 2.5, 2.84), held 1.5 .. 4; the strike share strikeShare (f_c4 / f0)^rankStrike (0.128 at
A0, 0.120 at C4, 0.110 at C8); every t60 × (f_c4 / f0)^rankLoss held under t60ScaleMax (2.5: A0's
fundamental 38 s); the string's loss into the bridge tension × bridgeMobility / L (0.12 /s at A0,
0.32 at C4, 1.12 at C6, 4.0 at C8: the unison's coupling); no damper over damperTop; the free
longitudinal fundamental c_L / (2 L), c_L = sqrt(E π dCore² / 4 / mu) (a winding adds mass, not
stiffness: 685 Hz at A0, 1208 at C2, 4110 at C4, 14.6 kHz at C6). At a fixed pitch and tension the
plain wire's B goes as 1 / L⁶ and a wound string's as 1 / L^(2 + 8 coreGrow) (A0: L^−3.38). The
bass break on the spinet's strings, grand's and a concert grand's: A#2, F2, C2 (A0's core 1.63,
1.48, 1.34 mm, its B 8.2e-4, 3.2e-4, 1.25e-4).

**The hammer** (`up_hammer.h`): F = K c^p (1 + lambda dc/dt) while the felt is compressed, never
negative, by the table c^p = sqrt(c)^w × g(c), w = floor(2p), g(c) = c^(p − w / 2) from a 64-point
table over 0 .. 4 mm spaced in sqrt(c) (exact at the half powers, 0.26 % at the quarter);
symplectic Euler; the loss solved against the string's point (above); the contact ends when the
felt is free and the hammer moves back, or 20 ms after the strike; one step's force held at
kForceCap 2000 N (the chart's largest 1578 N). The tine's tip by the generator: hardness ×
2^(kTipHard (1 − |g|)), felt × 2^(−kTipLoss (1 − |g|)), kTipHard 2, kTipLoss 1. `stepExact` is the
explicit form by powf, the simulator's.

**The attack** (`up_attack.h`, [above](#one-block)): the blow's share kKnockForce 0.983 of the
hammer's force (calibrated at grand's C4); the thud kThud 6.75 over kThudMs 10 ms from kThudDelayMs
2 ms (the key on its bed after the hammer meets the string: Askenfelt & Jansson; the share
calibrated on the band under 150 Hz); kLongModes 4 (`up_config.h`) longitudinal modes ringing
kLongT60 0.4 s (Bank & Sujbert: a few hundred milliseconds; an estimate), read through kLongAngle
0.08 (a typical bearing angle, 4.6°), driven over kLongShare 0.0912 of the length (calibrated at
grand's A0 through the field).

**The pickup** (`up_pickup.h`): v = C'(x) x', x and x' the tip's displacement and velocity (Σ tip
re and Σ tip ω im over the first six slots of every string, tip the partial's out held to ±1), C(x)
= 1 / (1 + ((x − c) / gap)²), one Lorentzian whose centre c is the magnet's offset at the tine and
slides to the gap toward the reed, c = offset + clamp(g, 0, 1) (gap − offset): at the reed the
plate's curve, centred at the gap, where the reed's rest a gap from the plate's plane puts the
capacitance's peak. Bounded everywhere, |C'| ≤ 3 sqrt(3) / (8 gap) = 0.65 / gap; a sine at gap
1.5 mm through the plate's curve reads 3.3, 6.6, 9.8, 15.7 % thd at 0.1, 0.2, 0.3, 0.5 mm. The
amp's one pole with its −3 dB point at `ampHz` exactly.

**The plate** (`up_board.h`): the orthotropic simply supported plate's lowest 32 modes, kBoardH
9 mm, kBoardRho 400, kBoardEx 11 GPa, kBoardEy 2.75 GPa, H = sqrt(Dx Dy), kBoardAspect 0.477; decays
π f eta; radiation (f / fc)² / (1 + (f / fc)²), fc = kFc0 2^(kFcSpan (1 − open)), kFc0 80 Hz,
kFcSpan 1.5. At 1 m² the first modes are 50.0, 65.3, 90.8 Hz (Suzuki's Steinway L: 49.7, 76.5,
85.3), the 32nd 764 Hz. Above the top mode the board is the direct share and the field
([above](#one-block)): `kBoardShelf` 3133 µm/s per N, their scale (a flat mobility through the two
highpasses that averages over 2 .. 4 f_top what the swept transfer does over the top mode's octave
on grand's board; the infinite plate's 1 / (8 sqrt(H ρ h)), 3604, for comparison);
`kFieldDirect` 0.316 (−10 dB, a choice, not a measurement); `kFieldLines` 16 of `kFieldLengths`
59 .. 281 samples, `kFieldSamples` 2602 in all; `kFieldFitHz` 4 kHz (the one-pole's second point);
`kFieldGain` 715.8 µm/s per N (measured by `uptool board` on grand's board).

**The damper's rate**: `dampRateOf` = dampRate (1 + f / kDampKnee) (1 − pedal), kDampKnee 2 kHz, 0
for a note over damperTop.

**The constants** (`LawConstants`, the ones a fit may move; `up_law.cpp`). tension, loss,
lossSlope, l0, bass0 and massC4 are also the law's and the rank's fallbacks where a shape's channel
(`tension`, `lossRate`, `lossSlope`, `length`, `bass`, `hammerMass`) is nan; grand's channels hold
the same values.

| constant | value | from |
|---|---|---|
| tension, rankLength, dMax, core, coreGrow | 553 N, 0.915, 1.089 mm, 1.0 mm, 0.1725 | the measured B tables' fit |
| l0, bass0 | 0.614 m, 1.61375 m | the same: a grand's C4 speaking length and its longest string |
| loss, lossKnee, lossSlope, rankLoss, t60ScaleMax | 0.45 /s, 800 Hz, 2, 0.8, 2.5 | the loss and its slope estimates (C4's fundamental 13.9 s, one string); the knee where the wire's own loss, not scaled by the rank, equals the floor: against Chaigne 2016's three measured strings (D#1 8.29 s, C4 4.84 s at 1 kHz, E6 1.38 s at its fundamental) the law reads 7.82, 5.99, 2.41 s (rms log2 0.50); the cap A0's 38 s |
| bridgeMobility | 3.5e-4 m/s per N | the bridge's driving-point mobility, an estimate: Giordano's soundboard impedance 1 .. 3 × 10³ kg/s puts it 3e-4 .. 1e-3, the bridge stiffer than the board; grand's C4 string loses 0.315 /s into it, of its 0.45 /s loss rate. No table measures it: `fit --constants` names it and never moves it |
| bridgeReactive | 0.5 | the reactive part of the bridge's admittance over its resistive part, an estimate: enough that the prompt sound and the aftersound have turned 1.3 rad against each other where they cross at C4, so no null (a 45° bridge would be 1; a finite plate's driving point swings between spring-like and mass-like through its modes); its sign lowers the in-phase mode's pitch, checked by measurement. Named by the fit, never moved |
| massC4, rankMass | 2.97 g, 0.25 | Chaigne & Askenfelt's hammers |
| rankHardLo, rankHardHi, rankP, rankStrike | 0.75, 1.41, 0.17, 0.03 | Chaigne & Askenfelt's K below and above C4 and their p up the keyboard; Chabassier's and Chaigne's strike shares |
| tineSlope | 1.2 | an estimate |

## Tuning

`up::Tuning` (`up_tuning.h`) is how the engine behaves; the channels are the piano.

| field | default | what |
|---|---|---|
| `rateStrings`, `rateHammer`, `rateBody`, `rateModes`, `ratePitch`, `ratePedal` | 8, 8, 4, 8, 0, 30 /s | each channel group's ease (0 a jump): the strings, the hammer, the body with the generator and the damper, the partials, the note, the pedal ([the channels](#the-channels)); `setRate` sets one channel's |
| `hammerScale`, `velCurve` | 7 m/s, 2 | the hammer's speed hammerScale × vel^velCurve: velocity 0.3 is 0.63 m/s (a pianissimo), 0.5 1.75, 0.7 3.4, 1 7 (Askenfelt & Jansson's p 1, mf 2.6, f 5.2, ff 8.6 bracket it) |
| `outputGain` | 1 | the voice's pair to full scale, before the clamp |
| `silence` | 1e-9 mm² | every bank's summed re² + im² under which a voice with nothing pending sleeps (−90 dB of a mm): grand at C4 sleeps 49 s after one strike at velocity 1, C2 151 s, C6 8.6 s, C7 2.4 s |
| `stealRate` | 60 /s | the pool's fade of a stolen voice, by the one law: 27 blocks to zero, the new note on it 78 ms after its key |
| `ceiling` | 0.45 | a partial over this share of fs is off; held 0.01 .. 0.5 |
| `ampHz` | 6000 Hz | the pickup amp's pole |
| `attack` | 1 | the hammer's attack, the knock, the thud and the clank together (`up_attack.h`), held 0 .. 2: 0 is none, 1 the calibration (grand's C4: the attack's own 10 dB under the string over its first 10 ms, the thud 4.7 dB over the band under 150 Hz; grand's A0: the clank 10 dB under the note), 2 twice it |

`clamped()` holds what would end the voice: a nan falls back to the default, the ceiling inside its
span, the hammer's throw and the pole over 0, the attack inside 0 .. 2. The bank holds a decay
slower than a T60 of 1000 s (`kMaxT60`) at 1000 s, so no rounding lets a partial grow.

## The presets

Nine tables of the twenty-four instrument channels, each with a playing range (`up_presets.h`;
`up_presets.cpp` carries each table's provenance in channel terms): a preset's shape is its table,
built once on first use (the partial and player channels nan, so the law writes the partials at the
note and `setShape` leaves the note, the glide and the pedal alone), its accessor's (`grand()` ..)
the same object. `grand` is the reference and every voice's start (`defaultShape`). The two fitted
tables' strings come from `uptool fit --table` against the measured B tables; the rest is set by
hand. Every preset carries the tine's `tineT60` 10.4 s and `tineLoad` 1.18, inert on the string.

Every preset sounds at every semitone of its range (2 s at velocity 1, at the gain that brings its
octave from C4 to grand's, since at the plain gain wurlitzer's low notes sit under the floor: every
sample finite and the peak over 0.022, a quarter of grand's octave rms; 723 of 723 notes); through
the board every note's last 100 ms of 2 s sits under its first. `honkyTonk` beats 3.94 Hz on C4's
fundamental against `upright`'s 0 (its tuned unison locks); `rhodesBark`'s thd at E4 is 50.0 %
against `rhodes`'s 3.3.

The octave from C4 against grand's is each of the twelve notes C4 .. B4 struck at velocity 1 and
rendered 0.6 s as `play` renders it, its rms over the first half second, the octave the mean of the
twelve powers (grand's 0.0897 rms, −20.95 dB full scale). Nothing evens the presets out: a consumer
sets its own gain (`Tuning.outputGain`).

| preset | the channels that differ from grand's | range | where it came from | the octave from C4 against grand's |
|---|---|---|---|---|
| `grand` | ([the channels' table](#the-channels)) | 21 .. 108 | the reference: the measured B tables' strings, Chaigne & Askenfelt's C4 hammer, a 1 m² spruce board half open | +0.00 dB |
| `concertGrand` | length 0.6265, bass 2.1145, detune 1.28, boardArea 1.6245, boardLoss 0.018467, open 1 | 21 .. 108 | `fit --table inharmonicity_grand.txt` on the strings: rms ln 0.191 over the Steinway D's keys (0.194 from C1 to C7, the worst B4 71 %); the board and the detune by hand | +1.77 |
| `upright` | length 0.6082, bass 1.4221, hardness 100.41, felt 5.743e-4, boardArea 0.7120, boardLoss 0.042133, open 0.15 | 21 .. 108 | `fit --table inharmonicity_upright.txt` on the strings: rms ln 0.301 (0.259 from C1 to C7, the worst B6 123 %); a small damped board and a softer hammer by hand | −5.25 |
| `spinet` | length 0.6014, bass 1.2209, hardness 238.81, felt 4.061e-4, boardArea 0.6156, boardLoss 0.048735, open 0 | 21 .. 108 | by hand: the shortest strings, the smallest board, closed, a harder hammer | −3.88 |
| `honkyTonk` | `upright`'s strings and board; detune 20, hardness 59.70, felt 7.071e-4 | 21 .. 108 | `upright` with its unison 20 cents wide and its hammer soft (Bank's recipe) | −8.59 |
| `fortepiano` | length 0.6102, bass 1.4842, hardness 42.22, felt 8.123e-4, boardArea 0.7846, boardLoss 0.038237, open 0.25 | 29 .. 101 | by hand: strings a little shorter, soft hammers, a small board; the gauge 1 (below) | −4.27 |
| `rhodes` | generator 0, tonebar 1, detune 1.5137 | 28 .. 100 | by hand: the tine, the pickup set (gap 1.5 mm, offset 0.9 mm), the tonebar beside it at 1.51 cents (the strings capped at 2) | −5.01 |
| `rhodesBark` | `rhodes`'s, with hardness 337.73, felt 3.536e-4, gap 0.75, offset 0.3 | 28 .. 100 | by hand: the pickup close, a harder tip; the tonebar at rhodes's 1.51 cents | +0.15 |
| `wurlitzer` | generator 1, hardness 84.43, felt 6.156e-4, gap 1.2184, offset 0.72 | 33 .. 96 | by hand: the reed, the pickup a little close, a softer hammer | −16.34 |

`uptool fit --table` on the current law lands nearer the tables than the two fitted presets'
strings, which the presets keep: `inharmonicity_grand.txt` at length 0.6176, bass 2.1369 (rms ln
0.176), `inharmonicity_upright.txt` at 0.6359, 1.3795 (0.202).

`fortepiano` holds the gauge at 1 because through this rank the gauge moves only the wire's
stiffness (the mass is the tension's), so a gauge of 0.8 would lower a plain wire's B to 0.41 of
its value (a wound string's core does not follow it), the opposite of Bank's fortepiano (lighter
strings, more inharmonicity); the shorter strings already raise the bass's B. `uptool print
--preset NAME` writes a preset's channels as `--set` lines (its name and range above them);
`--preset NAME` names the piano on every command, `--set` over it, and the tool's three anchors are
the presets `grand`, `rhodes` and `wurlitzer`.

## The pool

`Pool<M>` (`up_pool.h`) holds M voices, a note table and one board in itself, nothing on the heap:
`PoolStorage<M>` the voices, their slots and their lanes (each voice's bridge force for the block,
its weights on the board and the note they are at), `PoolBase` the logic over them (a host tool can
size one at run time). Its output is the board on every voice's bridge force with every voice's
pickup added, each at its level, not clamped.

**The note table.** A voice is free (asleep, no note), held (its key down), released (its key up:
its damper on unless the pedal holds it, or ringing out over `damperTop`) or stolen (fading for a
note that waits for it). `noteOn(midi, vel)` takes, in order:

1. the key's own voice if it still sounds (held or released): struck again (`Voice::noteOn`), a
   fresh hammer on the moving string, nothing restarted;
2. a stolen voice already waiting for the key: the newer velocity;
3. a free voice, the one longest asleep: the note lands whole (the pool's pedal, then the note; the
   voice wakes with every channel on its target);
4. else the quietest sounding voice, held or released, by `Voice::energy()` (the last block's
   summed string energy: a cut is least heard where the string is quietest), a voice whose strike
   is still queued counting as loud while another sounds and ties going to the oldest note; it is
   stolen;
5. else (every voice already stolen) the oldest stolen one's waiting note is replaced.

**The steal.** A stolen voice rings on while the pool fades its share (its bridge force into the
board and its pickup) by the one law at `tuning.stealRate`, block by block, until the next step
would fall under 1 %, and across that block linearly to exactly zero; at the next block it is stopped (`Voice::stop`: its
share is already zero, so nothing is cut) and the waiting note lands on it whole. The board it fed
rings on, as a soundboard does. At the default 60 /s that is 27 blocks: the new note sounds 78 ms
after its key. Measured (`pool_steals_the_quietest_without_a_cut`): grand on four voices, C4, E4
(softly), G4 and C5, the fifth key (E5) steals E4's voice (its energy 0.00055 mm² against 1.21 ..
2.33), not the oldest; the stolen voice's share through a board of its own reads 0.174 on the click
measure through the steal, against 0.092 ringing on unstolen and the bound of 1.76 for the top
partial it still rings (6.0 kHz).

**The key's release and the pedal.** `noteOff(midi)` releases the key's voice: its damper lands
unless the pedal holds it (a note still waiting for a voice lands released). `setPedal(p)` writes
every voice's pedal channel and is the pool's pedal: a note that lands takes it (`03_Keyboard`'s CC
64). `allNotesOff()` releases every key. Measured: grand's C4 released at 1 s, its voice's force 60 dB
under its peak 0.210 s later (the damper's T60 0.345 s); under the pedal it rings on; the pedal's
fall at 2 s lands it 0.190 s later.

**Pool-wide writes** (`set`, `setRate`, `setShape`, `setPartial`, `setPartials`, `setTuning`) go
to every voice now, in the caller's order, and stand over later notes: a voice keeps its channel
table across notes, so a note lands on the piano the pool last wrote, the last writer winning per
channel by the sequence exactly as on one voice (a partial written by hand pool-wide stands on a
later note until the instrument is written again). The board's plate is the written body:
`setShape` sets it from the body channels (`boardArea`, `boardLoss`, `open`) where they are
written, `set` from the one channel; the plate lands at once (its table rebuilt only when it moved,
every voice's weights again when its sides did); the voices ease their own body channels, which
nothing in the pool reads.

**The board, shared.** Each block every voice renders its pair into scratch; each bridge force
goes into the board at the voice's bridge point's weights (`Board::inputWeights` at
`Voice::bridgePoint()`, the eased note, recomputed when it moved); the board renders once on all M
forces (a voice asleep putting in zeros); the pickups are summed in voice order and added. Measured
(`pool_is_its_voices_through_one_board`): four voices over 1.67 s, through a release, the pedal, a
shape that moves the plate and a re-strike, each voice driven alone the same way, their forces
through one board of the test's and their pickups added by hand: the pool's output to the bit.

**The same key twice** meets its moving string on one voice: grand's C4 at 0.8 struck again a
second later reads its onset 0.15 dB over a fresh strike's (the strings' energy 15.9 mm² after the
first strike, 1.9 before the second, 18.3 after it: a strike on a moving string, not the sum of
two).

Control runs once per 128 samples whatever size `render()` is called with: the same calls give the
same bytes at render sizes 1, 128 and 1000 (`pool_same_bytes_at_any_render_size`). Two pools share
nothing.

**The handoff on a Teensy** (`AudioSynthUprightPool<M>`): each setter queues one command on an
`up::PoolQueue` (`up_pool.h`: 64 commands) inside an interrupt mask (the caller's mask put back); a
command is an op, a 16-bit index (a channel or a partial's slot) and four floats (`setPartial`'s),
and a shape, a partial table or a tuning waits in a small ring beside the queue (two each). A
partial table or a tuning straight after one of its kind merges into it; a shape never does: each
shape is one sequence, so a partial in a later shape stands over what the law writes for an earlier
one's instrument channels. The wrapper holds two queues and the setters write one. A setter that
finds no room waits unmasked for `update()` to take the queue (a block, 2.9 ms), and gives up only
after 20 ms, counting it in `dropped()`. `update()` turns the setters to the other queue with
interrupts masked (an index: nothing is copied), then, unmasked, drains the one they wrote into the
pool in order (the board's table runs there when the body moved: the 32 frequencies from the mode
pairs found once and the field's sixteen losses, no search; the moved modes' turns follow in the
render) and renders. No setter writes the queue being drained (they write the other until the next
`update()`, which never preempts itself), so the drain needs no mask, and the queue the setters
come back to is empty. The queue is the core's, so the host tests it: 3 s of notes, a steal,
releases, the pedal, a shape, a tuning, sets, a rate, a partial and a table through the queue
drained each block are the same bytes as the calls made directly (`pool_queue_keeps_order`), and
so are calls through two queues turned at each block's top, some made while one drains, which
land a block later (`pool_queues_turn_at_each_block`).

Sizes at 64 partials: a voice 20,072 bytes (its attack 428), a slot 48, a lane 644, the board
14,184 (its field 10,668), `Pool<4>` 98,976, `Pool<8>` 182,096, a `PoolQueue` 5,736. Compiled for a
Teensy 4.1, `AudioSynthUpright` is 43,936 bytes, `AudioSynthUprightPool<8>` 193,928 and
`AudioSynthUprightPool<4>` 110,888, each pool wrapper with its two queues (at 48 partials,
`-DUP_MAX_MODES=48`, 163,208 and 94,504).

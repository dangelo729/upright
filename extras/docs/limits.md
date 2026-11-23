# Known limits

What the model does not do, or does only so far. `make test` reports eleven rows as `KNOWN`: a
check that does not pass, kept in its test with what it measured and counted apart from the
failures. Each is named below where it belongs, marked **known row**.

## Level and the board

- **The presets are not levelled.** Their octaves from C4 run from wurlitzer's −16.34 dB to
  concertGrand's +1.77 against grand's ([design.md](design.md#the-presets)), and a single note is
  the board's coincidence of partials, plate modes and the field's grain; a consumer sets its own
  gain (`Tuning.outputGain`), one gain for all in `03_Keyboard`. One key at velocity 1 on one voice
  (as `AudioSynthUpright` plays it: its own board, the pickup added), every key of each preset's
  range over its first 3 s on the host: the loudest key's peak at a gain of 1, and the most gain
  at which no key passes full scale (`01_Grand` takes 2):

  | Preset | Loudest key | Its peak at gain 1 | The most gain |
  | --- | --- | --- | --- |
  | grand | D1 | 0.438 | 2.28 |
  | concertGrand | B0 | 0.577 | 1.73 |
  | upright | F#1 | 0.368 | 2.71 |
  | spinet | G1 | 0.361 | 2.76 |
  | honkyTonk | F1 | 0.304 | 3.29 |
  | fortepiano | G6 | 0.330 | 3.02 |
  | rhodes | E7 | 0.329 | 3.03 |
  | rhodesBark | D#5 | 0.739 | 1.35 |
  | wurlitzer | C7 | 0.094 | 10.6 |
- **Keys struck in one block add their onsets.** Eight keys evenly across a preset's range at
  velocity 1 peak 0.22 (wurlitzer) to 2.27 (rhodesBark) at a gain of 1 (grand 0.59, rhodes 0.97);
  the pool's sum is not clamped and its wrapper clamps (`03_Keyboard` takes 0.4: at most 0.91).
- **The board blooms in the bass**: its low modes ring up over the first 0.3 .. 0.4 s, so a bass
  note's output can sit over its first 100 ms for a while; no note of a preset's range does so at
  2 s. The board is linear and stable.
- **The field tilts on a large board** (two **known rows**): its power follows its t60, so on the
  concert grand's board, where the top mode (470 Hz) rings 253 ms, the board's power over f_top ..
  2 f_top (470 .. 940 Hz) sits +3.61 dB above a flat shelf's and over 2 .. 4 f_top +3.21, where
  the test asks within 3 dB; over 4 .. 8 f_top +0.70. On the spinet's and grand's boards every
  band is within −0.72 .. +1.13 dB.
- **The field rings shorter than the plate between the crossover and 4 kHz**: each line's one-pole
  meets the plate's law at dc (the top mode's t60) and at 4 kHz and falls faster than 1 / f
  between, so the lines' mean t60 at 1.5 f_top is 0.69, 0.76 and 0.97 of the plate's on the
  spinet's, grand's and the concert grand's boards (0.57, 0.65 and 0.80 at the top mode). Its late
  decay there, by Schroeder's integration from 10 ms, lands on the one-pole's on the spinet's and
  the concert grand's boards (16.7 and 160.8 ms) and swings with the analysis on grand's.
- **The field is grainy**: its transfer swings with the frequency, as a diffuse field's does (the
  dB's standard deviation 4 .. 7 dB in each octave band above the modes, its mean 1.5 .. 3.5 dB
  under the power's), so a single note moves with where its partials land.
- **The field's lengths are fixed whatever the body and the rate**: one network mode per 16.9 Hz
  at 44.1 kHz on every board, the plate's own density (one per 19.3 Hz) at 1 m² alone; the
  spinet's plate (0.62 m²) has one per 31.3 Hz, the concert grand's (1.62 m²) one per 11.9, and at
  96 kHz the same lines give one per 36.9 Hz, 0.6 .. 2.9 ms long. The body's size shows in the
  modes and the crossover; lengths that moved under a ringing chord would click.
- **The field's first pass carries most of an impulse's energy**: sixteen arrivals of energy 1
  each against the network's 16.26 .. 20.0 at unit gain across the boards (80 to 98 %), so its
  level is its first pass's and moves 0.89 dB from the spinet's board to the concert grand's
  whatever the loss.
- **The field costs memory and time**: 10,668 bytes a board (a `Board` is 14,184 bytes,
  `Pool<8>` 182,096; each voice wrapper carries a board of its own, so `01_Grand`'s `piano` is
  43,936 bytes of RAM1), and on the host the board's block takes about 6 µs where nothing moves,
  against about 9 for a voice's; once a pool and once a voice wrapper.
- **The plate's rebuild runs in the audio interrupt**: the 32 frequencies, decays, radiations and
  output weights from the mode pairs found once, the field's sixteen losses (two pows and a sqrt a
  line) and the moved modes' turns, about 0.02 of a voice's block on the host; the voice wrapper
  every block while its eased body moves after a write, the pool wrapper once per body write in
  its drain.
- **A glide flickers through the board**: a partial sweeping the plate's resonances (Q 33 at
  grand's loss) rises and falls with each, and above them with the field's grain.

## The strings and the hammer

- **The strings' reach at C2 is 16.8 cents** on the 10th partial from the spinet's strings to a
  concert grand's: the tenor moves ±2 % (the measured B tables' fit) and C2 is wound on every one,
  so the bass break and the core carry it there.
- **The measured B tables** (four **known rows**, against a 50 % gate on the worst note): fitted on
  length and bass (`fit --table`), the grand's worst note B4 is 68.1 % off (the source's own
  outlier) and the upright's B6 71.0 %, their rms ln 0.177 and 0.182 from C1 to C7; the presets'
  own strings (`concertGrand`, `upright`) read B4 70.7 % and B6 123.4 %, their rms ln 0.194 and
  0.259.
- **The hammer's span reaches little at a fortissimo**: from Chaigne & Askenfelt's C2 hammer to
  their C6 (hardness 25.1 .. 803 N/mm^p, the felt 1e-3 .. 2.5e-4 s/mm) the force's band moves 4.3
  dB at velocity 1 and 12.2 at 0.5; at a fortissimo every felt is stiff.
- **The contact against KTH's at C4**: 23 % short at 0.28 m/s (velocity 0.2: the modal string's
  point response, or the felt's law at small compressions), within 5 % from 1.75 to 4.5 m/s
  (velocity 0.5 .. 0.8), and 14 % long at 7 m/s (velocity 1: 2.10 ms against 1.84).
- **C6's contact at 6 m/s is 1.04 ms** by the truth, over the 1.0 the test asks (a **known row**;
  Askenfelt & Jansson's ff 0.82): the string's tension and strike share set it.
- **The engine's step at the audio rate** (one substep) misses C6's peak by 15 % (a **known
  row**): the contact runs at two substeps a sample.
- **The phantom partials are not built.** The attack's clank is the string's free longitudinal
  modes driven while the hammer touches; the forced longitudinal response, at the sums and
  differences of the transverse partials for as long as they ring (Bank & Sujbert), is not, nor
  the damper's noise or the key's release.
- **The knock is relatively louder at a soft touch at C6**: the blow's share is the hammer's force,
  and from velocity 1 to 0.5 at grand's C6 it falls 12.44 dB over the first 10 ms where the
  string's own falls 14.37. At C4 it falls as far as the string (12.63 against 12.44) and at C2
  (12.16 against 11.75); the test prints C6 and does not hold it.
- **The thud reads as a dip.** The blow's share alone lifts the board's band under 150 Hz at C4 7.0
  dB over the string's own and the thud alone 8.9; together, two pushes 2 ms apart, they partly
  cancel in the plate's lowest modes, and the band reads +4.7 dB over the attack's absence.
- **The clank is the bass's.** Its modes sit over the contact's bandwidth, so it grows faster than
  the square of the blow (from velocity 0.5 to 1 it rises 30.0 dB at C2 where the strongest
  partial rises 12.0, and 42.8 dB at C6 where it rises 13.8). Set 10 dB under the note at grand's A0, it
  reads 41.5 dB under at C2, 46.5 at C4 and 32.6 at C6.
- **The attack in the generator's middle**: the attack keeps the bridge's weight, clamp(−g, 0, 1),
  while the string's partials fade toward the tine's, so between the string and the tine the knock
  stands over a quieter string.

## The unison and the decays

- **The tuned three's aftersound beats once, slowly.** The bridge's reactive part keeps the prompt
  sound and the aftersound from cancelling (no dip where they cross at 2.8 s), but the aftersound
  is two modes, the second string's and the third's against the main, and they beat against each
  other: grand's C4 fundamental dips 5.96 dB at 7.65 s. The three strings' release T60 reads 7.9
  times longer at dampRate 6.6 /s than at 60.6 (one string 8.7), their two decays tilting the fit;
  a wide unison's beats swamp the two-slope fit (three strings 20 cents apart: the aftersound's
  ratio 0.24, against 5.6 for the tuned three, read on 16 s).
- **The upper partials still beat.** The detune in hertz grows with the partial and the coupling
  does not, so at the tuned 0.5 cents C4's fundamental locks and its partials from the 2nd up beat
  (the 4th on two strings at 0.2919 Hz, a little under its free 0.3031); each of the 3rd .. 8th
  ripples 11 .. 13 dB over 0.3 .. 3 s. The loss keeps the higher ones short: the 8th is 60 dB down
  by 1.9 s.
- **The decays against the historic strings**: the law against Chaigne's three measured strings
  reads D#1 7.82 s (8.29 measured), C4 5.99 (4.84) at 1 kHz and E6's fundamental 2.41 (1.38): rms
  log2 0.50, E6 the furthest (the tool's literal 1 kHz reading at E6, below its fundamental, is
  2.95 s). Chabassier & Duruflé's model block in the same table, settings and not measurements,
  rings its low keys for many minutes where the law's ring under 40 s: the whole table reads rms
  log2 2.45 at grand's. A0's fundamental rings 38 s (the rank's T60 scale capped); the coupled
  three's aftersound rings at the internal loss alone (C4's fundamental 38 s by its rate). The top
  octave rings short but speaks: C8's fundamental 0.41 s, C7's 1.26, key 127's 52 ms.
- **The modal truncation's bass ceiling**: 64 partials cap the bottom octave and a half's
  brightness (A0's 64th partial sits at 2.68 kHz on grand, 2.17 on concertGrand); `UP_MAX_MODES` 96
  is the lever, at a voice's cost, set in the build's flags ([design.md](design.md#the-law)): the
  Arduino IDE has no place for it.
- **The bass sleeps late**: grand at C2 sleeps 151 s after a strike at velocity 1 (the aftersound
  rings at the internal loss, the bridge's share taken out of the capped T60), C4 49 s.
- **The silence gate is absolute**: grand's C8 struck at velocity 0.1 (0.07 m/s, under any
  pianissimo) sleeps within about 10 ms; the key does not speak.

## The electric side

- **The pickup's far end** (gap 3 mm, offset 1.5) reads 2.85 % thd at E4, over the 2 % the test
  asks (a **known row**), and 3.78 % at E3, the tine and its tonebar swinging the tip into the
  curve.
- **The tonebar rings longer than the tine** (kAftersound, 2.5 times), so rhodes at E3 sleeps at
  3.32 times its tine's t60 (the tine alone at 1.33), where the test asks 1.5 (a **known row**).
- **The generator's middle**: at −0.5 the bridge's fundamental is 11.8 dB under the string's (half
  the weight by design, the tine's gain near its clamp); in the string-to-tine half the pickup reads
  a string's partials, a thd at velocity 1 between 98 and 299 % from −0.9 to −0.3 through the
  chart's simulator.

## One voice and the pool

- **One voice re-struck at another note retunes what it rings, and grows in neither output.** A
  ringing partial moved up keeps its velocity ω |z| (its displacement × hz_old / hz_new), one moved
  down keeps its displacement (its velocity falls), so a key jump is bounded both ways: grand's C3
  jumped to C5 on one voice, the tip's velocity sum moves −0.45 dB and the board's output +0.86
  dB; C5 jumped to C3, the board falls 5.4 dB and the tip's velocity 12.6 (each over 1348 samples
  either side). A glide up keeps its velocity: grand's C4 gliding a fifth to G4 reads a click
  measure of 0.24 against the bound 4.18; a glide down keeps its displacement: the bridge force
  holds and the pickup's voltage falls by the pitch's ratio. A pool re-strikes a voice only at its
  own key; the single wrapper (`01_Grand`, `02_Channels`) moves its one voice between keys.
- **The thrash's pickup peaks over 1.5** (a **known row**): every 100 ms on one voice a random
  shape, each instrument channel drawn between the nine presets' lowest and highest values, the
  tonebar 1 − |g|, and a random note; over 30 s it peaks 1.71 on the pickup, over the 1.5 the
  test asks, and 0.085 on the board. The voice is not clamped; the wrappers clamp.
- **`fortepiano` holds the gauge at 1**: through this rank a thinner gauge would lower a plain
  wire's B (the gauge moves only the wire's stiffness), the opposite of Bank's fortepiano.
- **A strike lands one block late** (2.9 ms), as every write; two strikes in one block are the
  second.
- **The steal's latency is the fade**: a stolen voice's new note sounds 78 ms after its key at the
  default `stealRate` (60 a second), 9 ms at 600.
- **A voice takes 4,294,967,296 writes.** Each write to a channel is numbered in 32 bits and the
  newest number wins; once the numbers have gone round, a write can lose to an older one and its
  channel keeps the old value. Four channels written every block get there in 36 days.

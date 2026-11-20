# The host rig

Everything but the two Teensy wrappers compiles on a computer with clang and the C++ standard
library alone (`clang++ -std=c++17 -Wall -Wextra -Werror -O2 -fno-math-errno`). The rig lives in
`extras/host`; the root `Makefile` forwards to it.

## Make

- `make test` builds and runs every test in `extras/host/tests` (the wrappers' sources compile too,
  to nothing off the board, and `test_pool.cpp` includes `Upright.h`). The tests read `src/core`'s
  sources for libm calls and heap use (`config_core_guard`) and the reference tables in
  `extras/reference`. A row that does not pass and is kept with what it measured prints as
  `KNOWN` and is counted apart from the failures ([limits.md](limits.md) lists each).
- `make tool` builds `extras/host/build/uptool`.
- `make corpus` writes `extras/host/build/corpus`, the same bytes on every run: the library's byte
  baseline.
- `make clean`.
- `make board` (the root's) compiles every example for teensy41 with PlatformIO
  (`$HOME/.platformio/penv/bin/pio`), nothing flashed: the library staged under
  `extras/host/build/pio/lib` as a consumer gets it, the flags `-Wall -Wextra -fno-math-errno` and
  a sketch's own from its `// make board: FLAGS` line, any warning in the library's files or the
  sketch a failure (the framework's own warnings are not counted). It prints each example's flash
  and RAM lines and its largest objects in RAM.

## uptool

On every command the piano is grand, or `--preset NAME` (any of the nine), with `--set
CHANNEL=VALUE ...` over it (any channel; repeatable, each taking every pair up to the next option,
by spaces or commas), `--tune NAME=VALUE` (any `Tuning` field) and `--midi M`.

```
uptool partials [--preset NAME] [--set CHANNEL=VALUE ...] [--midi M] [--tune NAME=VALUE]
    the law's table at the note: slot, ratio, hz, cents off n f0, t60 at the note, gain, out, on;
    the rank's line. --set lands on the instrument channels before the law and on a partial
    channel after it
uptool partials --table FILE
    a reference table as read, beside the law at the shape its fit finds
uptool string ... [--from M --to M]
    the rank per semitone: f0, L, d, wound, mu, Z, B, the hammer's mass, its felt's K and p at
    the note, the strike share, the damper; B against each B table; the bass break
uptool hammer ... [--midi M --vel V]
    the contact by the simulator at 32 fs (the truth): the force every 0.1 ms, the contact, the
    peak, the impulse against the momentum, the force's spectrum, the band; a velocity ladder
    through the engine's scheme and the truth, KTH's C4 contact beside; the felt table against powf
uptool pickup ... [--amp A] [--from-hammer --midi M --vel V]
    the curves; a sine through the voltage, its harmonics and thd; a ladder of amplitudes
uptool board ... [--sweep --out FILE]
    the plate's modes, the bridge-line weights per octave, the listening weights; the shelf and
    the field, their levels against the swept transfer; --sweep: the transfer at 1/12 octave,
    20 Hz .. 10 kHz, on the spinet's, grand's and the concert grand's boards into FILE
uptool print [--preset NAME] [--set CHANNEL=VALUE ...]
    the piano's twenty-four instrument channels as --set lines, a line a group, each value read
    back to the same bits
uptool random [--seed S --count N --midi M]
    N pianos, every instrument channel drawn inside its span, each as print prints it, with its B,
    first t60, 8th partial's gain and strings; each rendered 2 s at M: the peak, the first half
    second's rms against grand's octave, sounds or not; both levels' spread
uptool fit --table FILE [--set CHANNEL=VALUE ...]
    the shape nearest a reference table, by its kind: length and bass (B, decay), hardness
    (hammer), generator and tineT60 (tine), a grid in fifths of each span, then descent to 0.001;
    a channel --set names held there; the channels found, each entry's residual, the --set lines
uptool fit --constants --tables DIR [--kind KIND] [--out FILE] [--fix NAME,...]
    the LawConstants and the channels every table shares against every table at once; each
    table's residual before and after; the proposed values into FILE. nothing in src/ changes
uptool chart [DIR]
    each instrument channel alone at 21 points along its span at grand, rhodes and wurlitzer, and
    five pairs of channels in fifths of their spans; the engine's strikes and the truth's;
    summary.txt (the cells that fail a check), floors.txt
uptool chart --audio [DIR]
    the 1-d sweeps rendered 2 s (C4; E3 at rhodes and wurlitzer), the table's measures beside the
    render's; audio_CHANNEL.txt, audio_summary.txt (the worst disagreements)
uptool play [--preset NAME | --table FILE] [--midi M --vel V --sec S --off-at S --restrike-at S
            --pedal --pedal-off-at S --glide-to M2 [--glide-at S --glide-sec S] -o x.wav --png p.png]
    one note through a voice and a board of its own: the law's table beside what the voice shows
    (each partial's hz, cents, level, t60), the peak, the first half second's rms, the first three
    t60s, the beat and the two slopes on the fundamental, the hammer's contact and peak, the thd
    (the electric side), the release's t60, the sleep. --table: a tine table set whole over the
    piano (rhodes's when no preset is named)
uptool corpus DIR [--tune NAME=VALUE ...]
    the byte baseline: grand, rhodes and wurlitzer at 36, 60, 84 at velocity 0.5 and 1; grand at
    60 re-struck, released, released under the pedal, gliding to 67; rhodes at 52 and 64;
    wurlitzer at 57; the measured tine whole at 64; every preset at the bottom, middle and top of
    its range at velocity 0.5 and 1; a Pool<4> of grand: a chord under the pedal and five keys on
    four voices (the fifth steals); a wav and a png each, and index.txt with what play read
uptool size
    sizeof(up::Voice) by part; the board; the pending table; Pool<4> and Pool<8>
```

A render that peaks over full scale is written to its WAV at the gain that brings it to 0.99, and
the gain printed.

The fits as they run: `inharmonicity_grand.txt` rms ln 0.176 (length 0.6176, bass 2.1369),
`inharmonicity_upright.txt` 0.202 (length 0.6359, bass 1.3795), `inharmonicity_grand_model.txt`
0.0919, `hammer_scale_grand.txt` at hardness 68.5, `tine_rhodes.txt` at the generator 0.025 and
tineT60 10.71; `decay_grand.txt` runs to the spans' ends (length 2, bass 4). Against the law's own
B at length 0.6191 m and bass 1.8043 m the fit finds both within 0.05 %
(`fit_finds_its_own_strings`). The constants' fit reads the tables' `# hold:` lines as channel
holds (the B tables at a concert grand's strings, length 0.6269 and bass 2.1330, and the
upright's at 0.6076 and 1.4036) and holds a tine table's generator at 0, fitting its tineT60 per
table.

## The rig's own instruments

- `up_wav`: 16-bit PCM.
- `up_png`: a spectrogram, 0 .. 8 kHz, 2048-point Hann frames, 80 dB, a tick every 100 ms.
- `up_analysis.h`: a radix-2 FFT; peaks parabola-interpolated; a narrow band's envelope from one
  FFT of the record; T60 by least squares from −6 to −40 dB; the two-slope fit; the beat reader;
  the tracker; THD; the click measure (the worst 10 ms frame's largest first difference over that
  frame's rms: a sampled sine at f reads 2√2 sin(π f / fs), the bound for a sound whose top
  partial is f 1.5 times that).
- `up_play.h`: a note rendered and read as `play` does it; how loud a piano sounds over an octave,
  and one note; a shape rendered and read as `random`, `fit` and the presets' test read it; the
  board's swept transfer.
- `up_contact.h`: the contact simulator, the truth.
- `up_fit.h`: the three anchors, the presets of their names; a shape's `--set` lines and their
  parser; each channel's span (`channelSpan`); random's draw; the reference tables read; each
  table's error at a shape; the channel fit and the constants' fit.
- `tests/up_shapes.h`, the tests' pianos: `with(shape, {{channel, v} ...})`, `withGenerator` (the
  tonebar 1 − |g| beside it), and five steps each of the strings (the spinet's to a concert
  grand's), the hammer (hardness 25.1 .. 803 N/mm^p with the felt 1e-3 .. 2.5e-4 s/mm) and the
  board (the spinet's to a concert grand's), grand's in the middle.

## The reference tables

`extras/reference/` holds them, plain text (`README.txt` names each file's source and what is
unverified): a Steinway D's and an upright's measured B per key, a Steinway-D model's B, hammers
and damping per key, Chaigne & Askenfelt's hammers, Askenfelt & Jansson's velocities and contact
times, measured decays at 1 kHz, a Rhodes tine's one overtone and its fundamental's Q per note,
the cantilever's roots, and a grand's and an upright's soundboard modes. None of them is compiled
into the library.

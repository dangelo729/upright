upright — extras/reference
The reference tables: published piano and electric-piano measurements and
model parameters, transcribed as data.

These files are read by the host tool (uptool partials --table, string, fit
--table, fit --constants) and the host tests. None of them is compiled into
the library: src/ holds no table and no number taken from them but the
constants the fit set (named in src/core/up_law.cpp), the Arduino build
compiles nothing under extras/, and library.json's export excludes extras/
from the PlatformIO package.

The format: plain text, `#` comments; a `# units:`, a `# source:` (and `#
via:` where an open secondary source quotes a paywalled primary), a `#
kind:` (B, decay, hammer, tine, cantilever, board) and one or more `#
columns:` lines, each naming the columns of the data lines after it; a data
line is `key value ...`, the key a midi number, a partial number or a name,
a value a number or `-`. `# hold: CHANNEL=V` holds a channel in the
constants' fit; `# midi:` names a tine table's note. Each file's header
says how its numbers were obtained: printed in a table, recovered from a
figure's vector marker coordinates, read by pixel from a figure image, read
by eye, or computed from printed numbers. The full provenance (file, page, table or
figure, a verbatim quote for each number) is in the stage's notes,
plans/upright-engine/out0/reftables/NOTES.md, outside the repository.

  inharmonicity_grand.txt
    J. Jaatinen and J. Pätynen, "Effect of inharmonicity on pitch perception
    and subjective tuning of piano tones", JASA 152(2) 1146-1157, 2022, open
    copy at acris.aalto.fi. A Steinway & Sons model D, one string per key,
    mezzoforte: B per key from A0 to G#5 (59 keys), recovered from fig. 4's
    vector markers (not a printed table). Keys above A5 are comments: the
    source calls them unreliable. B4 (2.74e-3) is an outlier as plotted.

  inharmonicity_upright.txt
    H. Hinrichsen, "Entropy-based tuning of musical instruments", arXiv
    1203.5101, 2012. An unnamed upright: B for all 88 keys, recovered from
    fig. 4(a)'s vector markers (not a printed table).

  inharmonicity_grand_model.txt
    J. Chabassier and M. Duruflé, "Physical parameters for piano modeling",
    INRIA RT-0425, 2012, appendix A. A Steinway-D-based model's string scale
    (C1 .. B7): B computed here from its printed L, d, T0 and E (not a
    measurement; its wrapped strings are homogenized model strings).

  decay_grand.txt
    Block 1: A. Chaigne, "Reconstruction of piano hammer force from string
    velocity", JASA 140(5) 2016: measured decay time constants at 1 kHz on
    D#1 and C4 of an 1873 Streicher and E6 of a 1960 Bösendorfer (printed;
    t60 computed as tau ln 1000). Block 2: Chabassier and Duruflé (as
    above): their damping coefficients R_u and eta_u per key, read as Bank's
    b1 and b3 (the mapping derived in the notes); model settings the source
    calls difficult to tune, not measurements. No open (midi, partial, t60)
    table for a grand was found: unverified as a measured decay law.

  hammer_params.txt
    A. Chaigne and A. Askenfelt, "Numerical simulations of piano strings" I
    and II, JASA 95, 1994 (paywalled), via B. Bank et al., "Physically
    informed signal processing methods for piano sound synthesis", EURASIP
    JASP 2003, table 1: mass, K and p for C2, C4 and a third note the via
    source prints as C6 (the original's C7 is unverified). Hammer velocities
    by dynamic and contact durations from A. Askenfelt and E. Jansson's KTH
    lecture "From touch to string vibrations" (figs. 6 and 7), read by pixel
    from the figure images.

  hammer_scale_grand.txt
    Chabassier and Duruflé (as above), appendix A: the model's hammer mass,
    K, p, damping R, strike point and length per key, C1 .. B7 (printed;
    model values following their fitted formulas).

  tine_rhodes.txt
    G. Shear, "The electromagnetically sustained Rhodes piano", MS thesis,
    UCSB 2011: one inharmonic partial printed on fig. 2.2 (1.95 kHz) over a
    fundamental read by eye (264 Hz; the note is not named, `# midi: 60` is
    the nearest key, an assignment), and the fundamental's Q for six notes of
    a 1974 Mark I (tables 2.1, 5.1; t60 computed); geometry, pickup and model
    numbers from G. Shear and M. Wright, NIME 2011, A. Falaize and T. Hélie,
    JSV 2016 and A. Falaize's web page. No open table of measured Rhodes
    partials across the keyboard was found (Gabrielli et al. 2020 and
    Münster & Pfeifle 2014 are not open), nor any Wurlitzer reed partials.

  cantilever.txt
    T. Irvine, "Bending frequencies of beams, rods, and pipes", 2012, n = 1
    .. 4 printed, cross-checked against Wikipedia's Euler-Bernoulli page; n =
    5, 6 computed by root-finding of the same equation.

  soundboard.txt
    Spruce's constants from J. Chabassier, A. Chaigne and P. Joly, INRIA
    RR-8181, 2012, table 1; the loss factor from X. Boutillon and K. Ege, JSV
    2013; a Steinway model L soundboard's first six modes from H. Suzuki,
    JASA 80(6) 1986 (paywalled) via K. Ege's thesis, 2009, tables 1.2, 1.3.

  soundboard_upright.txt
    K. Ege's thesis (as above), table 1.3: an Atlas upright's soundboard,
    the first twenty modes and their loss factors.

Unverified or guessed, in summary: the third Chaigne & Askenfelt hammer's
note; decay_grand's block 2 (model settings); the tine's 264 Hz fundamental
and its note; every figure-read value is a figure, not a printed number.

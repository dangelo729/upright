# Changelog

All notable changes to Upright are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2026-11-23

### Breaking

- The seed is gone. `AudioSynthUpright()`, `AudioSynthUprightPool<M>()`, `up::Voice(fs)`
  and `up::Pool<M>(fs)` take none, and `reseed` and `seedOf` are removed: nothing drew
  from it. A core `Voice(5)` or `Pool<M>(5)` still compiles, with 5 as its sample rate,
  so take the argument out.
- The pool's `pedal()` is `setPedal()`, as the voice's is.
- `UP_MAX_MODES`, or another of the build's sizes, defined in a sketch stops the build
  at the link; they belong in the build flags. It used to crash.
- `01_Grand` and the README's sketch play at `outputGain` 2, 6 dB quieter, where no key
  of `grand` clips at full velocity.

### Added

- `up::stringShape(state, y, n)` and `State::re`: the string's shape from a copied state.
- `up::finiteOr`.

### Changed

- `Voice::noteOn` ignores a note that is not finite, as the pool does.
- Both wrappers mask interrupts across a copy only. Each pool wrapper is 5,736 bytes
  larger.
- The wrappers compile only where the Teensy core's `AudioStream.h` is; elsewhere
  `Upright.h` gives the core alone.
- The docs give the steal's latency and `stealRate`, a table of the gain each preset
  takes, where the build's sizes are set, and how many writes a voice takes.

## [0.1.0] - 2026-11-23

The first release.

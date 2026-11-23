// upright — Upright.h
// the umbrella. everywhere it brings in the core, which compiles with nothing
// but the c++ standard library: the channel table, the tuning, the law and
// the rank, the bank, the hammer and the strings, the attack, the pitch
// axis, the pickup, the board, the telemetry, the voice, the presets and the
// pool. on a teensy, an arduino core with the teensy audio library's
// AudioStream.h, it brings in AudioSynthUpright and AudioSynthUprightPool
// too; on the host and on another arduino core those headers are empty.
#pragma once

#include "core/up_attack.h"
#include "core/up_bank.h"
#include "core/up_board.h"
#include "core/up_channels.h"
#include "core/up_config.h"
#include "core/up_hammer.h"
#include "core/up_law.h"
#include "core/up_pickup.h"
#include "core/up_pitch.h"
#include "core/up_pool.h"
#include "core/up_presets.h"
#include "core/up_rank.h"
#include "core/up_string.h"
#include "core/up_telemetry.h"
#include "core/up_tuning.h"
#include "core/up_voice.h"

#include "AudioSynthUpright.h"
#include "AudioSynthUprightPool.h"

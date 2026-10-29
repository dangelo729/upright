// upright — tests/test_channels.cpp  (plan §1; the tests' "config and channels" bullet)
//
// the last writer wins by sequence; latch; a nan is no write; tick eases and
// says what moves; a write under an older sequence loses; the four partial
// blocks contiguous and partialOf right at their edges; every name finds its
// channel again; every channel's span holds the default piano; the tuning's
// defaults and clamped(); a partial table through a shape and back; the
// law's writer and a partial set by hand.
#include <cmath>
#include <cstring>

#include "core/up_channels.h"
#include "core/up_knobs.h"
#include "core/up_law.h"
#include "core/up_rank.h"
#include "up_test.h"

using namespace up;

// every channel has a name that finds it again; the blocks are contiguous
// and of kMaxModes; the helpers place a partial's four channels in them and
// partialOf reads them back at every block's edges
UP_TEST(channels_names_and_blocks) {
    bool round = true;
    for (int i = 0; i < kChannelCount; ++i) {
        Channel c;
        round = round && channelByName(channelName((Channel)i), c) && c == (Channel)i;
    }
    CHECK(round);
    Channel c;
    CHECK(!channelByName("nope", c) && !channelByName(nullptr, c));
    CHECK(channelByName("ratio7", c) && c == ratioChannel(7));
    CHECK(channelByName("t60_7", c) && c == t60Channel(7));
    CHECK(channelByName("gain7", c) && c == gainChannel(7));
    CHECK(channelByName("out7", c) && c == outChannel(7));
    CHECK(std::strcmp(channelName(Channel::damperTop), "damperTop") == 0 && std::strcmp(channelName(outChannel(0)), "out0") == 0);
    CHECK(kChannelCount == 22 + 4 * kMaxModes + 3 && kInstrumentChannels == 22);
    bool blocks = true;
    for (int k = 0; k < kMaxModes; ++k) {
        blocks = blocks && (int)ratioChannel(k) == (int)Channel::ratio0 + k && (int)t60Channel(k) == (int)Channel::ratio0 + kMaxModes + k &&
                 (int)gainChannel(k) == (int)Channel::ratio0 + 2 * kMaxModes + k &&
                 (int)outChannel(k) == (int)Channel::ratio0 + 3 * kMaxModes + k;
        blocks = blocks && partialOf(ratioChannel(k)) == k && partialOf(t60Channel(k)) == k && partialOf(gainChannel(k)) == k &&
                 partialOf(outChannel(k)) == k;
        blocks = blocks && isPartialChannel(ratioChannel(k)) && isPartialChannel(outChannel(k)) && !isInstrumentChannel(gainChannel(k));
    }
    CHECK(blocks);
    // the edges: the first and last of each block, and the channels either side of the four
    const int last = kMaxModes - 1;
    CHECK(partialOf(Channel::ratio0) == 0 && partialOf(ratioChannel(last)) == last && partialOf(t60Channel(0)) == 0 &&
          partialOf(t60Channel(last)) == last && partialOf(gainChannel(0)) == 0 && partialOf(outChannel(last)) == last);
    CHECK(partialOf(Channel::damperTop) == -1 && partialOf(Channel::midi) == -1 && !isPartialChannel(Channel::damperTop) &&
          !isPartialChannel(Channel::midi));
    CHECK(isInstrumentChannel(Channel::length) && isInstrumentChannel(Channel::damperTop) && !isInstrumentChannel(Channel::ratio0) &&
          !isInstrumentChannel(Channel::pedal));
    CHECK(groupOf(Channel::lossSlope) == Group::strings && groupOf(Channel::hardness) == Group::hammer && groupOf(Channel::hammerMass) == Group::hammer &&
          groupOf(Channel::boardArea) == Group::body && groupOf(Channel::tonebar) == Group::body && groupOf(Channel::damperTop) == Group::body &&
          groupOf(gainChannel(3)) == Group::modes && groupOf(Channel::midi) == Group::pitch && groupOf(Channel::glide) == Group::pitch &&
          groupOf(Channel::pedal) == Group::pedal);
    printf("    %d channels: 22 instrument, 4 blocks of %d partials, 3 player\n", kChannelCount, kMaxModes);
}

// every channel's span holds the default piano, which is grand's with the
// law's partials at c4, at middle c, no glide, the pedal up
UP_TEST(channels_range_holds_the_default) {
    const Shape d = defaultShape();
    int outside = 0;
    for (int i = 0; i < kChannelCount; ++i) {
        float lo, hi;
        channelRange((Channel)i, lo, hi);
        const bool ok = lo < hi && d.v[i] >= lo && d.v[i] <= hi;
        outside += !ok;
        CHECK_MSG(ok, "%s: %g outside %g .. %g", channelName((Channel)i), d.v[i], lo, hi);
    }
    const Knobs grand = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}};
    const Shape b = pianoOf(grand);
    const PartialTable p = partialsOf(b, rankAt(b, 60.0f));
    bool same = true;
    for (int i = 0; i < kInstrumentChannels; ++i) same = same && d.v[i] == b.v[i];
    for (int k = 0; k < kMaxModes; ++k)
        same = same && d[ratioChannel(k)] == p.ratio[k] && d[t60Channel(k)] == p.t60[k] && d[gainChannel(k)] == p.gain[k] &&
               d[outChannel(k)] == p.out[k];
    CHECK(same);
    CHECK(d[Channel::midi] == 60.0f && d[Channel::glide] == 0.0f && d[Channel::pedal] == 0.0f && d[Channel::generator] == -1.0f);
    printf("    the default piano inside every channel's span: %d outside\n", outside);
}

// in one table, the last write to a channel is its target; a nan is no write
UP_TEST(channels_last_writer_wins) {
    Channels ch;
    Shape s = defaultShape();
    s[Channel::length] = 0.7f;
    s[Channel::tension] = 800.0f;
    ch.set(Channel::length, 0.5f);
    ch.set(Channel::generator, 0.0f);
    ch.setShape(s);
    ch.set(Channel::tension, 650.0f);
    CHECK(ch.target(Channel::length) == 0.7f);            // the shape came after
    CHECK(ch.target(Channel::generator) == s[Channel::generator]);
    CHECK(ch.target(Channel::tension) == 650.0f);         // the setter came after the shape
    CHECK(ch.seq(Channel::tension) > ch.seq(Channel::length));
    ch.set(Channel::midi, std::nanf(""));                 // a nan is no write
    CHECK(ch.target(Channel::midi) == s[Channel::midi]);
    Shape holes = s;
    for (int i = 0; i < kChannelCount; ++i) holes.v[i] = std::nanf("");
    holes[Channel::detune] = 4.0f;
    const uint32_t before = ch.seq(Channel::length);
    ch.setShape(holes);                                   // a shape's nans write nothing
    CHECK(ch.target(Channel::detune) == 4.0f && ch.seq(Channel::length) == before && ch.target(Channel::length) == 0.7f);
}

// between a writer's table and the audio side's, the newer write wins per
// channel, in sequence order; a latch with nothing new changes nothing; a
// stale table cannot undo a newer write
UP_TEST(channels_latch_by_sequence) {
    Channels pending, live;
    pending.set(Channel::length, 0.5f);                   // seq 1
    Shape s = defaultShape();
    s[Channel::length] = 0.8f;
    s[Channel::strikeShare] = 0.2f;
    pending.setShape(s);                                  // seq 2, every channel
    pending.set(Channel::strikeShare, 0.1f);              // seq 3
    pending.rate(Channel::strikeShare, 40.0f);            // seq 4
    live.latch(pending);
    CHECK(live.target(Channel::length) == 0.8f);
    CHECK(live.target(Channel::strikeShare) == 0.1f);
    CHECK(live.rateOf(Channel::strikeShare) == 40.0f);
    bool whole = true;
    for (int i = 0; i < kChannelCount; ++i) {
        const Channel c = (Channel)i;
        if (c == Channel::strikeShare) continue;
        whole = whole && live.target(c) == s[c];
    }
    CHECK(whole);
    const Shape before = live.shape();
    live.latch(pending);
    const Shape after = live.shape();
    CHECK(std::memcmp(&before, &after, sizeof(Shape)) == 0);
    Channels stale;
    stale.set(Channel::length, 1.9f);                     // seq 1 there
    live.latch(stale);
    CHECK(live.target(Channel::length) == 0.8f);
    pending.set(Channel::length, 0.6f);
    live.latch(pending);
    CHECK(live.target(Channel::length) == 0.6f);
    CHECK(live.lastSeq() == pending.lastSeq());
}

// a writer with its own sequence lands unless the channel's last write is newer
UP_TEST(channels_write_under_a_sequence) {
    Channels ch;
    ch.set(Channel::gauge, 1.1f);                         // seq 1
    ch.set(Channel::gauge, 1.2f);                         // seq 2
    ch.write(Channel::gauge, 0.9f, 1);                    // older: loses
    CHECK(ch.target(Channel::gauge) == 1.2f && ch.seq(Channel::gauge) == 2);
    ch.write(Channel::gauge, 1.05f, 2);                   // as new: lands
    CHECK(ch.target(Channel::gauge) == 1.05f);
    ch.write(Channel::gauge, 1.07f, 7);                   // newer: lands, and the counter follows it
    CHECK(ch.target(Channel::gauge) == 1.07f && ch.seq(Channel::gauge) == 7 && ch.lastSeq() == 7);
    ch.write(Channel::gauge, std::nanf(""), 9);           // a nan is no write
    CHECK(ch.target(Channel::gauge) == 1.07f && ch.seq(Channel::gauge) == 7);
    ch.set(Channel::length, 0.6f);
    CHECK(ch.seq(Channel::length) == 8);
}

// a rate at or below zero jumps, exactly; otherwise one block moves by the
// law; tick says the piano moves while a channel outside the pitch axis is
// over 0.1 % from its target, and not for the pitch axis
UP_TEST(channels_tick_eases_and_reports) {
    Channels ch;
    const float dt = (float)kBlock / kDefaultFs;
    ch.snap();
    CHECK(!ch.tick(dt));                                  // at rest
    ch.rate(Channel::length, 0.0f);
    ch.set(Channel::length, 0.75f);
    CHECK(!ch.tick(dt));                                  // a jump lands at once
    CHECK(ch.value(Channel::length) == 0.75f);
    ch.rate(Channel::gauge, -1.0f);
    ch.set(Channel::gauge, 1.3f);
    ch.tick(dt);
    CHECK(ch.value(Channel::gauge) == 1.3f);
    const float from = ch.value(Channel::generator);
    ch.rate(Channel::generator, 10.0f);
    ch.set(Channel::generator, from + 1.0f);
    CHECK(ch.tick(dt));                                   // moving
    const float want = from + 1.0f * (1.0f - std::exp(-10.0f * dt));
    CHECK(std::fabs(ch.value(Channel::generator) - want) < 1e-6f);
    for (int i = 0; i < 2000; ++i) ch.tick(dt);
    CHECK(!ch.tick(dt));                                  // settled within 0.1 %
    ch.rate(Channel::midi, 5.0f);
    ch.set(Channel::midi, 72.0f);
    CHECK(!ch.tick(dt));                                  // the pitch axis glides but the piano is still
    CHECK(ch.value(Channel::midi) > 60.0f && ch.value(Channel::midi) < 72.0f);
    ch.set(Channel::pedal, 1.0f);
    CHECK(ch.tick(dt));                                   // the pedal moves it
    ch.set(gainChannel(3), 0.0f);
    ch.snap();
    CHECK(ch.value(gainChannel(3)) == 0.0f && ch.value(Channel::midi) == 72.0f);
}

// rates default from the tuning by group; a new tuning moves only the rates
// nobody set
UP_TEST(channels_group_rates) {
    Tuning t;
    CHECK(t.rateStrings == 8.0f && t.rateHammer == 8.0f && t.rateBody == 4.0f && t.rateModes == 8.0f && t.ratePitch == 0.0f &&
          t.ratePedal == 30.0f);
    t.rateStrings = 3.0f;
    t.rateHammer = 5.0f;
    t.rateBody = 2.0f;
    t.rateModes = 4.0f;
    t.ratePedal = 6.0f;
    Channels ch(t);
    CHECK(ch.rateOf(Channel::length) == 3.0f && ch.rateOf(Channel::felt) == 5.0f && ch.rateOf(Channel::open) == 2.0f &&
          ch.rateOf(Channel::gap) == 2.0f && ch.rateOf(Channel::dampRate) == 2.0f && ch.rateOf(ratioChannel(5)) == 4.0f &&
          ch.rateOf(Channel::pedal) == 6.0f && ch.rateOf(Channel::midi) == 0.0f);
    ch.rate(Channel::length, 9.0f);
    t.rateStrings = 6.0f;
    ch.setTuning(t);
    CHECK(ch.rateOf(Channel::length) == 9.0f && ch.rateOf(Channel::bass) == 6.0f);
}

// the tuning: a nan falls back to the field's default; rates at or under zero
// stand; the ceiling held to 0.01 .. 0.5, the hammer's throw and the amp's
// pole over 0; nothing else limited
UP_TEST(channels_tuning_clamped) {
    const Tuning d;
    CHECK(d.hammerScale == 7.0f && d.velCurve == 2.0f && d.silence == 1e-9f && d.stealRate == 60.0f && d.ceiling == 0.45f && d.ampHz == 6000.0f);
    Tuning t;
    t.rateStrings = std::nanf("");
    t.rateModes = -2.0f;
    t.ceiling = 3.0f;
    t.ampHz = std::nanf("");
    t.outputGain = 7.0f;
    t.hammerScale = -1.0f;
    const Tuning c = clamped(t);
    CHECK(c.rateStrings == d.rateStrings && c.rateModes == -2.0f && c.ceiling == 0.5f && c.ampHz == d.ampHz && c.outputGain == 7.0f &&
          c.hammerScale > 0.0f);
    t.ceiling = -1.0f;
    t.ampHz = 0.0f;
    CHECK(clamped(t).ceiling == 0.01f && clamped(t).ampHz > 0.0f);
}

// a partial table through a shape and back: the same numbers, a nan where the
// table has one
UP_TEST(channels_partial_table_round_trip) {
    PartialTable p;
    for (int k = 0; k < kMaxModes; ++k) {
        p.ratio[k] = 1.0f + 1.01f * k;
        p.t60[k] = 10.0f / (1.0f + k);
        p.gain[k] = (k % 3) * 0.3f - 0.3f;
        p.out[k] = k == 4 ? std::nanf("") : (float)(k + 1);
    }
    Shape s = defaultShape();
    toShape(p, s);
    const PartialTable b = fromShape(s);
    bool same = true;
    for (int k = 0; k < kMaxModes; ++k) {
        same = same && b.ratio[k] == p.ratio[k] && b.t60[k] == p.t60[k] && b.gain[k] == p.gain[k];
        same = same && (k == 4 ? std::isnan(b.out[k]) : b.out[k] == p.out[k]);
    }
    CHECK(same);
}

// the law's writer: an instrument write rewrites every partial under its
// sequence; a partial written by hand later stands until the instrument is
// written again; a moved note rewrites under the same sequence (the hand's
// partial still stands); with nothing new it writes nothing
UP_TEST(channels_the_law_writes_the_partials) {
    Channels ch;
    Law law;
    const RankState c4 = rankAt(ch.shape(), 60.0f);
    CHECK(law.update(ch, c4, false));                     // a fresh writer writes
    CHECK(!law.update(ch, c4, false));                    // nothing new
    const Knobs spinet = {{-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}};
    ch.setShape(pianoOf(spinet));
    const RankState r = rankAt(ch.shape(), 60.0f);
    CHECK(law.update(ch, r, false));
    const PartialTable want = partialsOf(ch.shape(), r);
    const int j = kMaxModes - 1;                          // the top slot, and the one under it
    CHECK(ch.target(ratioChannel(j)) == want.ratio[j] && ch.seq(ratioChannel(j)) == ch.seq(Channel::length));
    ch.set(ratioChannel(j), 1000.0f);                     // a partial retuned by hand
    CHECK(!law.update(ch, r, false));
    CHECK(ch.target(ratioChannel(j)) == 1000.0f);
    const RankState r2 = rankAt(ch.shape(), 72.0f);
    CHECK(law.update(ch, r2, true));                      // the note moved: rewritten, the hand's partial stands
    CHECK(ch.target(ratioChannel(j)) == 1000.0f && ch.target(ratioChannel(j - 1)) == partialsOf(ch.shape(), r2).ratio[j - 1]);
    ch.set(Channel::detune, 3.0f);                        // the instrument written again
    CHECK(law.update(ch, r2, false));
    CHECK(ch.target(ratioChannel(j)) == partialsOf(ch.shape(), r2).ratio[j]);
    law.reset();
    CHECK(law.update(ch, r2, false));
}

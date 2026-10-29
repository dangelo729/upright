// upright — tests/up_shapes.h
// the tests' pianos off the presets: a shape with channels set over it, and
// the channel values the tests sweep, each in five steps with grand's in
// the middle.
#pragma once

#include <initializer_list>
#include <utility>

#include "core/up_channels.h"

namespace up {

// a shape with channels set over it
inline Shape with(Shape s, std::initializer_list<std::pair<Channel, float>> sets) {
    for (const auto& p : sets) s[p.first] = p.second;
    return s;
}

// the strings from the spinet's to a concert grand's: c4's speaking length and the longest string (m)
struct StringSize { float length, bass; };
constexpr StringSize kStringSizes[5] = {
    {0.6013641f, 1.2208763f}, {0.6076492f, 1.4036343f}, {0.614f, 1.61375f}, {0.6204172f, 1.8553188f}, {0.62690145f, 2.133049f},
};
inline Shape withStrings(const Shape& s, int i) {
    return with(s, {{Channel::length, kStringSizes[i].length}, {Channel::bass, kStringSizes[i].bass}});
}

// the hammer from soft to hard: the felt's stiffness at c4 (N/mm^p) and its loss (s/mm)
struct HammerFelt { float hardness, felt; };
constexpr HammerFelt kHammerFelts[5] = {
    {25.102291f, 0.001f}, {59.703648f, 0.0007071068f}, {142.0f, 0.0005f}, {337.7348f, 0.0003535534f}, {803.2733f, 0.00025f},
};
inline Shape withHammer(const Shape& s, int i) {
    return with(s, {{Channel::hardness, kHammerFelts[i].hardness}, {Channel::felt, kHammerFelts[i].felt}});
}

// the board from the spinet's (the smallest, closed) to a concert grand's (the largest, open): area (m²), loss, open
struct BoardPlate { float area, loss, open; };
constexpr BoardPlate kBoardPlates[5] = {
    {0.6155722f, 0.04873514f, 0.0f}, {0.7845841f, 0.038236815f, 0.25f}, {1.0f, 0.03f, 0.5f}, {1.2745606f, 0.023537522f, 0.75f},
    {1.6245048f, 0.018467166f, 1.0f},
};
inline Shape withBoard(const Shape& s, int i) {
    return with(s, {{Channel::boardArea, kBoardPlates[i].area}, {Channel::boardLoss, kBoardPlates[i].loss}, {Channel::open, kBoardPlates[i].open}});
}

// the generator with its tonebar beside it: the tonebar's weight 1 - |g|
inline Shape withGenerator(const Shape& s, float g) {
    return with(s, {{Channel::generator, g}, {Channel::tonebar, 1.0f - (g < 0.0f ? -g : g)}});
}

}  // namespace up

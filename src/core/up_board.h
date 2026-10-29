// upright — up_board.h  (plan §8)
// the soundboard's law: a simply supported orthotropic rectangular plate's
// modes, the lowest kBoardModes of the (m, n) grid in frequency order,
//
//     f_mn = (pi / 2) sqrt(1 / (rho h)) sqrt(Dx (m / a)⁴ + 2 H (m / a)² (n / b)² + Dy (n / b)⁴)
//
// with spruce along the grain (Dx = Ex h³ / 12, a the side along it), across
// it with the ribs' share folded in (Dy = Ey h³ / 12), H = sqrt(Dx Dy); a and b
// from the area at a fixed aspect. decays sigma = pi f eta. each mode's input
// weight for a note is the plate's shape where that note's string crosses
// the bridge line; its output weight is its radiation, (f / fc)² / (1 + (f /
// fc)²) with fc = kFc0 2^(kFcSpan (1 - open)) (a closed back raises the
// edge: a low shelf), times the shape at the listening point.
//
// the bridge line runs straight across the plate from a bass point near one
// side (kBassX, kBassY) to a treble point near the opposite corner (kTrebleX,
// kTrebleY), shares of a and b, the note from a0 (21) to c8 (108) along it.
// the listening point (kListenX, kListenY) sits off every low mode's node.
//
// the mode set is found once: at the fixed aspect a and b both scale by
// sqrt(area), every term under the formula's root by 1 / area², so every
// f_mn scales by 1 / area and the order of the modes, and so which (m, n)
// are the lowest kBoardModes, does not depend on the area. boardSearch runs
// the full search (m and n to 40, the lowest not yet taken each pass);
// boardOf keeps its (m, n) pairs at 1 m² in a table filled on the first
// call (a board's init: on a teensy, the wrapper's construction) and from
// then on evaluates kBoardModes frequencies, decays and radiations from the
// pairs, in frequency order: a rebuild costs those and, in the bank, the
// turns of the modes that moved. pure, per body write: libm is fine there.
//
// the Board rings it: a Bank<kBoardModes> (up_bank.h), every mode a phasor in
// micrometres of modal displacement. setPlate takes the eased body channels
// (the table and the field's loss rebuilt only when they moved, a mode's
// turn only where it moved); inputWeights(midi, w) is the plate's shape
// where the note's string crosses the bridge; render(forces, w, voices, out,
// n), per sample: every mode im += sum_v w_v,k F_v drive_k, drive_k = dt /
// (M omega_k) x 1e6 (the plate's modal mass M = rho h a b / 4, the force in
// N), the turn; the modal sum is the modes' velocity, sum outW_k omega_k
// im_k (um/s: a force into a mobility).
//
// above the top mode, the shelf: the voices' summed force (N, the
// injection's sum without the bridge points' weights) through two one-pole
// highpasses in series at the top mode's frequency, into the direct share
// and the field beside the modes. the direct share is that force times
// kFieldDirect of the shelf's mobility kBoardShelf (um/s per N): the
// spreading wave's first arrival. the field is the plate's diffuse response
// where its modes above the top one overlap, each ringing t60 = 6.907755 /
// (pi f eta): a feedback delay network of kFieldLines lines end to end in
// one buffer, their lengths fixed sample counts whatever the rate and the
// body (a network whose lengths moved under a ringing chord would click).
// per sample the lines' outputs d_i are mixed by householder, y_i = d_i -
// (2 / N) sum d_j (orthogonal: lossless), each y_i goes through its line's
// loss, a one-pole g_i (1 - a_i) / (1 - a_i z^-1), and is written back with
// the force added at 1; the field is the d_i summed at +1, -1, +1, ...
// times its gain. the loss, per rebuild in closed form: g_i the pass's loss
// at the top mode's t60 (under 1: stable), a_i so the pass's loss at
// kFieldFitHz is that of the plate's t60 there. the gain, kFieldGain x (the
// shelf's mobility / kBoardShelf), the same at every body: the field and the
// direct share together carry the shelf's mean power transfer (the network's
// impulse energy is mostly its first pass, sixteen arrivals, and moves little
// with the loss). nothing per sample but multiplies, adds and the lines'
// reads and writes.
//
// out = (the modal sum + the direct share + the field) x kBoardOut, the
// output weights ramped across each 128-sample block. the board does not
// feed back into the strings. setShelf scales the direct share and the field
// together: the tool's hook. shape(x, y) is the plate's displacement at a
// point (um), sin per mode: the app's call rate.
#pragma once

#include "up_bank.h"
#include "up_config.h"
#include "up_law.h"

namespace up {

// spruce and the plate
constexpr float kBoardH      = 0.009f;     // m: the board's thickness
constexpr float kBoardRho    = 400.0f;     // kg/m³: spruce
constexpr float kBoardEx     = 11.0e9f;    // Pa: spruce along the grain
constexpr float kBoardEy     = 2.75e9f;    // Pa: across the grain with the ribs' share (a quarter of Ex)
constexpr float kBoardAspect = 0.477f;     // a / b: the side along the grain over the side across it
// the radiation's low shelf
constexpr float kFc0         = 80.0f;      // hz: the -3 db low edge with the back open
constexpr float kFcSpan      = 1.5f;       // closed, an octave and a half higher
// the bridge line and the listening point, shares of a and b
constexpr float kBassX = 0.15f, kBassY = 0.80f;
constexpr float kTrebleX = 0.85f, kTrebleY = 0.12f;
constexpr float kListenX = 0.77f, kListenY = 0.77f;

struct BoardMode { int m, n; float hz, t60, radiation; };
struct BoardTable { BoardMode mode[kBoardModes]; float a, b; };   // the lowest kBoardModes, in frequency order; a, b in m

BoardTable boardOf(float areaM2, float loss, float open, const LawConstants& c = lawConstants());   // from the pairs found once
BoardTable boardSearch(float areaM2, float loss, float open);             // the full 40 x 40 search: the pairs' source, the tests'
float plateHz(int m, int n, float a, float b);                           // the formula
float radiationEdge(float open);                                         // fc: the radiation's -3 db low edge, hz
float radiationOf(float hz, float open);                                 // (f / fc)² / (1 + (f / fc)²)
void  bridgePoint(float midi, const BoardTable& t, float& x, float& y);  // where the note's string crosses the bridge line, m
float plateShape(int m, int n, float x, float y, const BoardTable& t);   // sin(m pi x / a) sin(n pi y / b)
void  inputWeights(const BoardTable& t, float x, float y, float* w);     // the shape at a bridge point, per mode
void  outputWeights(const BoardTable& t, float* w);                      // radiation x the shape at the listening point

// the shelf's mobility above the plate's modes, um/s per N, the direct share's and the field's scale: a flat
// mobility at this level through the two highpasses beside the modes averages over 2 f_top .. 4 f_top what the
// swept transfer does over the top mode's octave under it (f_top / 2 .. f_top) at grand's board
constexpr float kBoardShelf = 3133.0f;
// the field's lines: lengths in samples whatever the rate and the body, 1.3 .. 6.4 ms at 44.1 khz, kFieldSamples in
// all: one network mode per 17 hz, about the plate's own density at 1 m², n = (a b / 2) sqrt(rho h / H), one per 19 hz
constexpr int   kFieldLines = 16;
constexpr int   kFieldLengths[kFieldLines] = {59, 71, 83, 97, 107, 127, 139, 151, 167, 181, 197, 211, 227, 241, 263, 281};
constexpr int   kFieldSamples = 2602;
// hz: each line's one-pole meets the plate's t60 here, its gain at dc the top mode's (a one-pole cannot follow 1 / f
// further)
constexpr float kFieldFitHz = 4000.0f;
// the direct share of the shelf's mobility, 0.316 (-10 db): the spreading wave's first arrival, set by ear (the
// infinite plate's drive-point mobility is instantaneous; the bending wave crosses 0.7 m in about 3 ms at 1 khz)
constexpr float kFieldDirect = 0.316f;
// the field's gain, um/s per N: its square times the network's impulse energy (a unit impulse in, no highpass, 0.5 s;
// 17.25 at grand's board, 16.26 at the spinet's, 20.0 at the concert grand's) is (1 - kFieldDirect²) kBoardShelf² at
// grand's, within 0.7 db of it at the other two, so the field and the direct share together carry the shelf's mean
// power transfer (a flat gain G has sum h² = G²) (uptool board)
constexpr float kFieldGain = 715.8f;
// the board's output per um/s of radiated modal velocity: grand's octave c4 .. b4 at velocity 1 (each note's first
// half second's rms, the octave the mean of the twelve powers: up_play.h's octaveReading) is 0.0897 rms full scale
// (-20.9 db). grand's c4 peaks near 0.18
constexpr float kBoardOut = 1.235e-6f;

class Board {
public:
    void init(float fs);
    void reset();                                                        // every phasor and every line zero
    void setPlate(float areaM2, float loss, float open);                 // the eased body channels; what moved recomputed
    void setShelf(float gain) { shelf_ = gain; }                         // the shelf's mobility (kBoardShelf from init), the direct
                                                                         //   share's and the field's scale: the tool's sweep
    void inputWeights(float midi, float* w) const;                       // the plate's shape at the note's bridge point
    // per sample: the voices' forces (N) in at their weights, the turn, the radiated sum into out (n samples, written)
    void render(const float* const* forces, const float* const* w, int voices, float* out, int n);
    // one sample of the field alone: a force (N) in at every line as it is, no highpass; out um/s, at its gain
    float fieldStep(float force);
    void  fieldLine(int i, float& g, float& a) const;                    // line i's loss: its gain at dc and its pole
    float amplitude(int k) const { return bank_.amplitude(k); }         // um
    BoardMode modeInfo(int k) const { return table_.mode[k < 0 ? 0 : (k >= kBoardModes ? kBoardModes - 1 : k)]; }
    float shape(float x, float y) const;                                 // um at (x, y) m on the plate
    const BoardTable& table() const { return table_; }
    float energySq() const { return bank_.energySq(); }

private:
    Bank<kBoardModes> bank_;
    BoardTable        table_{};
    float fs_ = kDefaultFs;
    float area_ = -1.0f, loss_ = -1.0f, open_ = -1.0f;
    float shelf_ = kBoardShelf, shelfA_ = 1.0f;                          // the shelf's mobility; its poles' 1 - exp(-2 pi f_top / fs)
    float low1_ = 0.0f, low2_ = 0.0f;                                    // the shelf's two one-poles' state
    int   phase_ = 0;                                                    // samples into the current block
    float field_[kFieldSamples] = {};                                    // the field's lines, end to end
    int   fieldAt_[kFieldLines] = {};                                    // each line's read and write point
    float fieldG_[kFieldLines] = {}, fieldA_[kFieldLines] = {};          // each line's loss: its gain at dc, its pole
    float fieldLow_[kFieldLines] = {};                                   // the loss's state
    float fieldGain_ = 0.0f;                                             // the field's gain over shelf_: kFieldGain / kBoardShelf
};

}  // namespace up

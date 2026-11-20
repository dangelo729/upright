// upright — tests/test_config.cpp  (up_config.h: the shared helpers; the core's guard)
//
// the one smoothing law, the ramp, xorshift32, the note's frequency; and the
// guard that reads src/core's sources: no libm call outside the files and
// functions that run once per write or per note, and nothing on the heap
// anywhere.
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "core/up_config.h"
#include "up_test.h"

using namespace up;

// the one law: a rate at or below zero lands exactly; otherwise the formula
UP_TEST(config_approach) {
    CHECK(approach(0.1f, 0.3f, 0.0f, 0.01f) == 0.3f);
    CHECK(approach(0.1f, 0.3f, -5.0f, 0.01f) == 0.3f);
    const float x = approach(1.0f, 2.0f, 10.0f, 0.1f);
    CHECK(std::fabs(x - (1.0f + (1.0f - std::exp(-1.0f)))) < 1e-6f);
    CHECK(approachCoef(0.0f, 0.1f) == 1.0f);
}

// the ramp walks linearly and lands on its target after one block
UP_TEST(config_ramp) {
    Ramp r;
    r.snap(1.0f);
    r.to(2.0f);
    float v = 0.0f;
    for (int i = 0; i < kBlock; ++i) v = r.next();
    CHECK(std::fabs(v - 2.0f) < 1e-5f);
}

// xorshift32: the same seed, the same sequence; bipolar in [-1, 1)
UP_TEST(config_xorshift) {
    Rng a, b;
    a.seed(42);
    b.seed(42);
    bool same = true, inRange = true;
    for (int i = 0; i < 10000; ++i) {
        const float x = a.bipolar();
        same = same && x == b.bipolar();
        inRange = inRange && x >= -1.0f && x < 1.0f;
    }
    CHECK(same);
    CHECK(inRange);
    Rng z;
    z.seed(0);
    CHECK(z.next() != 0);                                 // a zero seed still runs
}

// the note's frequency: a4 440 hz, c4 261.63, an octave a doubling
UP_TEST(config_midi_to_hz) {
    CHECK(std::fabs(midiToHz(69.0f) - 440.0f) < 1e-3f);
    CHECK(std::fabs(midiToHz(60.0f) - 261.6256f) < 1e-3f);
    CHECK(std::fabs(midiToHz(72.0f) / midiToHz(60.0f) - 2.0f) < 1e-5f);
    CHECK(kMaxModes >= 8 && kMaxModes <= 96 && kUnisonModes <= kMaxModes && kBoardModes >= 4 && kBoardModes <= 64);
}

namespace {

// a source with its comments dropped (line and block), strings kept
std::string withoutComments(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s.compare(i, 2, "//") == 0) {
            i = s.find('\n', i);
            if (i == std::string::npos) break;
            out += '\n';
            continue;
        }
        if (s.compare(i, 2, "/*") == 0) {
            const size_t e = s.find("*/", i + 2);
            i = e == std::string::npos ? s.size() : e + 1;
            continue;
        }
        out += s[i];
    }
    return out;
}

// a definition's body, from its head to the brace that closes it
bool cutBody(std::string& text, const std::string& head) {
    const size_t at = text.find(head);
    if (at == std::string::npos) return false;
    size_t i = text.find('{', at);
    int depth = 0;
    for (; i < text.size(); ++i) {
        if (text[i] == '{') ++depth;
        if (text[i] == '}' && --depth == 0) break;
    }
    text.erase(at, i + 1 - at);
    return true;
}

}  // namespace

// the guard: src/core calls no libm (sin, cos, tan, exp, exp2, expm1, log,
// log2, pow, cosh, sinh, tanh and the rest, their f forms, std:: or not; sqrt
// is allowed everywhere, a single instruction) but where a function runs once
// per write, per note, per block, per strike or at the app's call rate: the
// whole of up_law.cpp, up_rank.cpp, up_tuning.cpp and up_channels.cpp, and
// these functions, each named: in up_config.h
// approachCoef (per block) and midiToHz (per write, per note); in
// up_bank.cpp rotationOf (per block, where a partial moved); in up_hammer.cpp
// set (per hammer write) and forceExact (the host's); in up_pickup.cpp
// AmpPole::set (per tuning write); in up_string.cpp Strings::setUnison (per
// block: the detune's exp2, the tonebar's aftersound by pow); in
// up_board.cpp the plate's law (plateHz, radiationEdge, radiationOf,
// boardOf, bridgePoint, plateShape, inputWeights, outputWeights: per body
// write and per note), Board::setPlate (per body write) and Board::shape
// (the app's); in up_voice.cpp Voice::block (the strike's speed, per
// strike); in up_attack.cpp Attack::init (the thud's turn, per rate) and
// Attack::setBlock (a cos a longitudinal mode, per block); in
// up_telemetry.cpp stringAt (the app's).
// the rank's longitudinal fundamental is a sqrt per note, allowed
// everywhere.
// anything else fails: the bank's turn, readback, injection and sums, the
// hammer's step, the strings' loop and their coupling (Strings::setCoupling
// and beginBlock too: multiplies), the pickup's curves and its stage, the
// board's render, the voice's samples, the attack's blow, its thud and its
// long bank's turn. and nothing in src/core on the heap:
// no new, malloc, std::vector, std::string
UP_TEST(config_core_guard) {
    const std::vector<std::string> perWriteFiles = {"up_law.cpp", "up_rank.cpp", "up_tuning.cpp", "up_channels.cpp"};
    struct Fn { const char* file; const char* head; };
    const Fn perWrite[] = {
        {"up_config.h", "inline float approachCoef("}, {"up_config.h", "inline float midiToHz("},
        {"up_bank.cpp", "void rotationOf("},
        {"up_hammer.cpp", "void Hammer::set("},        {"up_hammer.cpp", "float Hammer::forceExact("},
        {"up_pickup.cpp", "void AmpPole::set("},
        {"up_string.cpp", "void Strings::setUnison("},
        {"up_board.cpp", "float plateHz("},            {"up_board.cpp", "float radiationEdge("},
        {"up_board.cpp", "float radiationOf("},        {"up_board.cpp", "BoardTable boardOf("},
        {"up_board.cpp", "void bridgePoint("},         {"up_board.cpp", "float plateShape("},
        {"up_board.cpp", "void inputWeights("},        {"up_board.cpp", "void outputWeights("},
        {"up_board.cpp", "void Board::setPlate("},     {"up_board.cpp", "float Board::shape("},
        {"up_voice.cpp", "void Voice::block("},
        {"up_attack.cpp", "void Attack::init("},        {"up_attack.cpp", "void Attack::setBlock("},
        {"up_telemetry.cpp", "float stringAt("},
    };
    const int perWriteCount = (int)(sizeof perWrite / sizeof perWrite[0]);
    const std::regex libm(R"((^|[^A-Za-z0-9_])(std::)?(sin|cos|tan|asin|acos|atan|atan2|sinh|cosh|tanh|exp|exp2|expm1|log|log2|log10|log1p|pow|cbrt|hypot)f?\s*\()");
    const std::regex heap(R"((^|[^A-Za-z0-9_])(new|malloc|calloc|realloc|std::vector|std::string|std::map|std::list|std::deque|std::unique_ptr|std::shared_ptr|std::function)([^A-Za-z0-9_]|$))");
    int files = 0, calls = 0, heaps = 0, cut = 0, whole = 0;
    for (const auto& e : std::filesystem::directory_iterator(UP_CORE_DIR)) {
        const std::string name = e.path().filename().string();
        const std::string ext = e.path().extension().string();
        if (ext != ".h" && ext != ".cpp" && ext != ".inc") continue;
        ++files;
        std::ifstream in(e.path());
        std::stringstream ss;
        ss << in.rdbuf();
        std::string text = withoutComments(ss.str());
        for (std::sregex_iterator m(text.begin(), text.end(), heap), end; m != end; ++m) {
            ++heaps;
            CHECK_MSG(false, "%s: %s on the heap", name.c_str(), (*m)[0].str().c_str());
        }
        bool isWhole = false;
        for (const std::string& f : perWriteFiles) isWhole = isWhole || f == name;
        if (isWhole) { ++whole; continue; }
        for (const Fn& f : perWrite) {
            if (name != f.file) continue;
            const bool found = cutBody(text, f.head);
            CHECK_MSG(found, "%s: no %s", f.file, f.head);
            cut += found;
        }
        for (std::sregex_iterator m(text.begin(), text.end(), libm), end; m != end; ++m) {
            ++calls;
            CHECK_MSG(false, "%s calls %s outside a per-write or per-note function", name.c_str(), (*m)[0].str().c_str());
        }
    }
    CHECK(files >= 28);
    CHECK_MSG(whole == (int)perWriteFiles.size(), "%d of %d per-write files found", whole, (int)perWriteFiles.size());
    CHECK_MSG(cut == perWriteCount, "%d of %d named functions found", cut, perWriteCount);
    printf("    %d sources in src/core read: %d libm calls outside the %d per-write files and %d named per-write functions; %d heap uses\n",
           files, calls, (int)perWriteFiles.size(), perWriteCount, heaps);
}

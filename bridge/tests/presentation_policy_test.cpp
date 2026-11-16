#include "common/px_frame_generation.h"
#include <cassert>
#include <iostream>
using namespace PokemonXFrames;
int main() {
    Policy p;
    unsigned shown = 0, mids = 0;
    for (unsigned i = 0; i < 120; i++) {
        auto d = p.Step(30, true, true, 1, i * 16666667ULL);
        shown += d.kind != Kind::Skip;
    }
    assert(shown == 60);
    p.Reset();
    shown = 0;
    for (unsigned i = 0; i < 120; i++)
        shown += p.Step(30, i % 6 == 1, true, 1, i * 16666667ULL).kind != Kind::Skip;
    assert(shown == 20);
    p.Reset();
    shown = 0;
    for (unsigned i = 0; i < 120; i++) {
        auto d = p.Step(60, i % 2 == 0, true, 1, i * 16666667ULL);
        shown += d.kind != Kind::Skip;
        mids += d.kind == Kind::Intermediate;
    }
    assert(mids > 50 && shown > 110);
    p.Reset();
    for (unsigned i = 0; i < 60; i++)
        assert(p.Step(60, true, true, 1, i * 16666667ULL).kind == Kind::Original);
    auto d = p.Step(60, false, true, 2, 1000000001ULL);
    assert(d.kind == Kind::Skip && d.reset);
    p.Reset();
    for (unsigned i = 0; i < 100; i++)
        assert(p.Step(60, i % 2 == 0, true, 1, i * 30000000ULL).kind != Kind::Intermediate);
    p.Reset();
    for (unsigned i = 0; i < 100; i++)
        assert(p.Step(60, i % 2 == 0, false, 1, i * 16666667ULL).kind != Kind::Intermediate);
    p.Reset();
    p.Step(60, true, true, 1, 100);
    assert(p.Step(60, false, true, 1, 50).reset);
    std::cout << "PASS presentation30 cap, interpolated60 cadence, natural60 pass-through, "
                 "epoch/rewind resets, slow-host and UI fallback; no simulation clock access\n";
}

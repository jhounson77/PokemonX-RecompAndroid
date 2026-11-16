#include "frame_interpolation.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace PokemonX::Smoothing;
Matrix Pose(float x) {
    Matrix m{};
    m[0] = m[5] = m[10] = 1;
    m[3] = x;
    return m;
}
int main() {
    History history(16);
    Key actor{42, 1, 0};
    auto a = Pose(0), b = Pose(2);
    assert(history.Sample(actor, 0, a, true) == a);
    assert(history.Sample(actor, 1, a, true) == a);
    auto half = history.Sample(actor, 2, b, true);
    assert(half[3] == 1 && b[3] == 2);
    assert(history.Sample(actor, 2, b, true) == half); // repeated meshes share the same phase
    assert(history.Sample(actor, 3, b, true) == b);
    auto cut = Pose(100);
    assert(history.Sample(actor, 4, cut, false) == cut);
    assert(history.Sample(actor, 5, cut, true) == cut);
    Key replaced{42, 2, 0};
    assert(history.Sample(replaced, 6, Pose(200), true) == Pose(200));
    auto stale = Pose(400);
    assert(history.Sample(actor, 100, stale, true) == stale);
    history.Clear();
    assert(history.Sample(actor, 101, Pose(600), true) == Pose(600));
    Matrix live = Pose(7), smooth = Pose(6);
    try {
        RestoreOnExit restore(live, smooth);
        assert(live == smooth);
        throw std::runtime_error("draw failure");
    } catch (const std::runtime_error&) {
    }
    assert(live == Pose(7));
    {
        RestoreOnExit restore(live, smooth);
        {
            RestoreOnExit inner(live, Pose(5));
            assert(live == Pose(5));
        }
        assert(live == smooth);
    }
    assert(live == Pose(7));
    History bounded(2);
    for (unsigned i = 0; i < 100; i++)
        bounded.Sample({i, 1, 0}, i, Pose(i), true);
    assert(bounded.Size() <= 2);
    History disabled(0);
    assert(disabled.Sample(actor, 0, a, true) == a && disabled.Size() == 0);
    Matrix invalid = Pose(1);
    invalid[0] = NAN;
    assert(std::isnan(history.Sample(actor, 102, invalid, true)[0]));
    assert(history.Sample(actor, 103, Pose(3), true) == Pose(3));
    History backwards(2);
    backwards.Sample(actor, 10, a, true);
    assert(backwards.Sample(actor, 9, b, true) == b);
    History slow(2);
    slow.Sample(actor, 0, a, true);
    assert(slow.Sample(actor, 4, Pose(4), true)[3] == 1);
    assert(slow.Sample(actor, 5, Pose(4), true)[3] == 2);
    assert(slow.Sample(actor, 6, Pose(4), true)[3] == 3);
    assert(slow.Sample(actor, 7, Pose(4), true)[3] == 4);
    // Same logical simulation, with and without presentation-only smoothing.
    unsigned updates = 0, rng = 12345;
    float position = 0;
    History simulation(2);
    float last = -1;
    for (unsigned frame = 0; frame < 120; frame++) {
        if (frame % 2 == 0) {
            updates++;
            position += 2;
            rng = rng * 1664525U + 1013904223U;
        }
        Matrix source = Pose(position);
        auto image = simulation.Sample(actor, frame, source, true);
        {
            RestoreOnExit restore(source, image);
            if (frame >= 2)
                assert(source[3] > last);
            last = source[3];
        }
        assert(source == Pose(position));
    }
    unsigned reference = 12345;
    for (unsigned i = 0; i < 60; i++)
        reference = reference * 1664525U + 1013904223U;
    assert(updates == 60 && position == 120 && rng == reference);
    std::cout << "PASS 120 presentations/60 unchanged updates; matrix restoration, "
                 "exception/nesting, repeats, 15Hz cadence, cuts, generations, gaps, invalid data, "
                 "clock rewind and bounded storage\n";
}

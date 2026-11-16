#include "domain/lift/fallback_profile.h"
#include <cassert>
#include <iostream>
using hexlith::domain::lift::FallbackProfile;
int main() {
    FallbackProfile profile;
    for (unsigned i = 0; i < 200000; ++i) {
        profile.Observe(0x1000, false, false);
        profile.Observe(0x2000, true, true);
    }
    auto rows = profile.Snapshot();
    bool first = false, second = false;
    for (const auto& row : rows) {
        if (!row.count) continue;
        assert(row.error <= row.count);
        first |= row.pc == 0x1000 && !row.thumb && !row.native_yield;
        second |= row.pc == 0x2000 && row.thumb && row.native_yield;
    }
    assert(first && second && profile.Samples() > 100);
    assert(profile.Samples() < 1000);
    // Frequent sites remain visible despite a stream of distinct cold sites.
    for (unsigned i = 0; i < 500000; ++i) {
        profile.Observe(0x1000, false, false);
        if (i % 17 == 0) profile.Observe(0x4000 + i * 4, false, false);
    }
    first = false;
    for (const auto& row : profile.Snapshot()) {
        assert(row.error <= row.count);
        first |= row.pc == 0x1000 && row.count > row.error;
    }
    assert(first);
    assert(profile.Snapshot().size() == 32);
    std::cout << "PASS bounded sampled fallback profile, PC/mode/cause separation, heavy site retention, sampling bounds\n";
}

#include "domain/lift/runtime.h"
#include <cassert>
#include <iostream>
using namespace hexlith::domain::lift::runtime;
Pc Original(Context&, Pc) { return 0; }
Pc Extended(Context&, Pc) { return 0; }
int main() {
    EntryTable original, extended;
    original.Add(0x100, false, Original);
    extended.Add(0x100, false, Extended);
    extended.Add(0x120, false, Extended);
    LoadedModules modules;
    modules.Offer("Intro", "verified", extended);
    modules.Offer("Intro", "verified", original);
    modules.Offer("Other", "other-hash", original);
    assert(modules.Loaded("Intro", 0x8000, 0x100, 0x100, "verified") ==
           LoadedModules::Outcome::Lifted);
    assert(modules.Find(0x8100, false)->function == Extended &&
           modules.Find(0x8120, false)->function == Extended);
    assert(modules.Find(0x8120, false)->base == 0x8000);
    assert(!modules.Find(0x8120, true));
    assert(modules.Loaded("Other", 0xa000, 0x100, 0x100, "other-hash") ==
           LoadedModules::Outcome::Lifted);
    assert(modules.Find(0xa100, false)->function == Original);
    modules.Unloaded("Intro", 0x8000);
    assert(!modules.Find(0x8120, false));
    assert(modules.Find(0xa100, false));
    assert(modules.Loaded("Intro", 0x8000, 0x100, 0x100, "wrong") ==
           LoadedModules::Outcome::DigestDiffers);
    assert(!modules.Find(0x8120, false));
    std::cout << "PASS supplemental table priority, relocated PCs/mode, unrelated modules, unload "
                 "and digest mismatch refusal\n";
}

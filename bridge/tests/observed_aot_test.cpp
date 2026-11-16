#include "adapters/filesystem/directory_blob_sink.h"
#include "application/lift_emit.h"
#include "domain/analysis/tables.h"
#include "domain/arm/decode_arm.h"
#include "domain/arm/primitives/unpredictable.h"
#include "domain/lift/engine.h"
#include "domain/lift/runtime.h"
#include <cassert>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <random>
#include <vector>
using namespace hexlith;
std::vector<std::uint8_t> Fixture() {
    std::vector<std::uint8_t> code(512);
    auto word = [&](unsigned at, unsigned value) {
        for (unsigned i = 0; i < 4; i++)
            code[at + i] = value >> (i * 8);
    };
    word(0, 0xe12fff1e); // synthetic return
    std::vector<unsigned> loop{0xe3a00000, 0xe3a0100a, 0xe2800001, 0xe5820000,
                               0xe2511001, 0x1afffffb, 0xe6510f92, 0xe6810fb2};
    using O = domain::arm::Operation;
    std::map<O, unsigned> wanted{{O::Usat, 0},  {O::Pkhbt, 0},      {O::Smlad, 0},   {O::Uxtb16, 0},
                                 {O::Rev, 0},   {O::Usat16, 0},     {O::Rev16, 0},   {O::Pkhtb, 0},
                                 {O::Qadd, 0},  {O::QdoubleAdd, 0}, {O::Ssat16, 0},  {O::Qsub, 0},
                                 {O::Smusd, 0}, {O::Smuad, 0},      {O::Uxtab16, 0}, {O::Ssat, 0}};
    std::mt19937 random(0x78251);
    for (unsigned i = 0; i < 8000000; i++) {
        unsigned value = 0xe0000000U |
                         ((i % 3 == 0   ? 1U
                           : i % 3 == 1 ? 6U
                                        : 7U)
                          << 24) |
                         (random() & 0x00ffffffU);
        auto d = domain::arm::DecodeArm(value);
        if (!wanted.contains(d.operation) || wanted[d.operation] >= 5 || d.destination == 14 ||
            d.destination_high == 14 || domain::arm::primitives::HasUnpredictableOperands(d))
            continue;
        loop.push_back(value);
        ++wanted[d.operation];
        bool done = true;
        for (auto [op, count] : wanted)
            done &= count == 5;
        if (done)
            break;
    }
    for (auto [op, count] : wanted)
        assert(count == 5);
    loop.push_back(0xe12fff1e);
    for (unsigned i = 0; i < loop.size(); i++)
        word(32 + i * 4, loop[i]);
    return code;
}
int main(int argc, char** argv) {
    namespace a = domain::analysis;
    namespace arm = domain::arm;
    namespace lift = domain::lift;
    namespace memory = domain::memory;
    if (argc == 2) {
        auto code = Fixture();
        std::vector<a::FunctionRange> functions{{0x1000, 4, arm::InstructionSet::Arm}};
        auto regions =
            a::RenderRegions({{0x1000, 4, a::RegionKind::Code, arm::InstructionSet::Arm},
                              {0x1020, 356, a::RegionKind::Code, arm::InstructionSet::Arm}});
        for (int stage = 0; stage < 2; stage++) {
            if (stage) {
                a::FunctionRange observed{0x1020, 356, arm::InstructionSet::Arm};
                observed.source = a::Provenance::Observed;
                functions.push_back(observed);
            }
            adapters::filesystem::DirectoryBlobSink sink(std::filesystem::path(argv[1]) /
                                                         (stage ? "after" : "before"));
            assert(sink.Prepare());
            auto result = application::EmitLiftedCode(
                {code, 0x1000, a::RenderFunctions(functions), regions}, sink);
            assert(result);
        }
        return 0;
    }
    assert(argc == 3);
    auto code = Fixture();
    void* handles[] = {dlopen(argv[1], RTLD_NOW | RTLD_LOCAL),
                       dlopen(argv[2], RTLD_NOW | RTLD_LOCAL)};
    if (!handles[0] || !handles[1]) {
        std::cerr << dlerror() << '\n';
        return 2;
    }
    lift::runtime::EntryTable tables[2];
    for (unsigned i = 0; i < 2; i++) {
        auto reg = reinterpret_cast<void (*)(void*)>(dlsym(handles[i], "hexlith_lifted_register"));
        assert(reg);
        reg(&tables[i]);
    }
    assert(!tables[0].Find(0x1020, false) && tables[1].Find(0x1020, false));
    for (unsigned limit = 0; limit <= 160; limit++)
        for (bool fault : {false, true}) {
            memory::ProcessMemory memories[3];
            arm::CpuState states[3];
            for (unsigned i = 0; i < 3; i++) {
                assert(memories[i].Map(0x1000, 0x1000, memory::Backing::Allocate(0x1000), 0,
                                       memory::Access::ReadWrite));
                for (unsigned j = 0; j < code.size(); j++)
                    assert(memories[i].Write8(0x1000 + j, code[j]));
                assert(memories[i].Write32(0x1f00, 0xef000001));
                states[i].registers[15] = 0x1020;
                states[i].registers[14] = 0x1f00;
                states[i].registers[2] = fault ? 0x3000 : 0x1800;
                states[i].carry = true;
                states[i].overflow = true;
            }
            std::uint64_t counts[3]{};
            auto expected = arm::Run(states[0], memories[0], limit, counts[0]);
            for (unsigned i = 0; i < 2; i++) {
                lift::Engine engine(tables[i]);
                auto actual = engine.Run(states[i + 1], memories[i + 1], limit, counts[i + 1]);
                const auto retired = engine.RetiredStatistics();
                assert(retired.total == counts[i + 1] && retired.engine_fallback <= retired.total);
                if (i == 0)
                    assert(retired.engine_fallback == retired.total);
                if (i == 1 && limit == 160 && !fault)
                    assert(retired.engine_fallback == 1);
                assert(actual.reason == expected.reason && actual.detail == expected.detail &&
                       counts[i + 1] == counts[0]);
                assert(states[i + 1].registers == states[0].registers &&
                       arm::PackCpsr(states[i + 1]) == arm::PackCpsr(states[0]));
                assert(memories[i + 1].Read32(0x1800) == memories[0].Read32(0x1800));
                if (i == 1 && limit == 160 && !fault)
                    assert(engine.Counted().interpreted == 1);
            }
        }
    // Retirement counters accumulate across slices, while a fetch fault and
    // a zero budget must not retire another guest instruction.
    {
        memory::ProcessMemory memory;
        assert(memory.Map(0x1000, 0x1000, memory::Backing::Allocate(0x1000), 0,
                          memory::Access::ReadWrite));
        assert(memory.Write32(0x1000, 0xe1a00000) && memory.Write32(0x1004, 0xef000001));
        lift::runtime::EntryTable empty;
        lift::Engine engine(empty);
        arm::CpuState state;
        state.registers[15] = 0x1000;
        std::uint64_t count = 0;
        engine.Run(state, memory, 1, count);
        assert(count == 1 && engine.RetiredStatistics().total == 1);
        engine.Run(state, memory, 1, count);
        assert(count == 1 && engine.RetiredStatistics().total == 2);
        state.registers[15] = 0x3000;
        auto stop = engine.Run(state, memory, 1, count);
        assert(stop.reason == arm::StopReason::FetchFault && count == 0);
        assert(engine.RetiredStatistics().total == 2 &&
               engine.RetiredStatistics().engine_fallback == 2);
        engine.Run(state, memory, 0, count);
        assert(count == 0 && engine.RetiredStatistics().total == 2);
    }
    for (auto h : handles)
        dlclose(h);
    std::cout << "PASS synthetic observed AOT generation/registration, native loop vs interpreter, "
                 "budgets0..160, flags/registers/stores, SVC and write faults\n";
}

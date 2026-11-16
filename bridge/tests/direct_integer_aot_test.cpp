#include "adapters/filesystem/directory_blob_sink.h"
#include "application/lift_emit.h"
#include "domain/analysis/tables.h"
#include "domain/lift/engine.h"
#include "domain/lift/runtime.h"
#include <cassert>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <vector>
using namespace hexlith;
namespace arm = domain::arm;
namespace memory = domain::memory;
namespace lift = domain::lift;
namespace a = domain::analysis;
std::vector<std::uint8_t> Code() {
    std::vector<std::uint8_t> code(0x800);
    auto put = [&](unsigned offset, unsigned value, unsigned width) {
        for (unsigned b = 0; b < width; b++)
            code.at(offset + b) = value >> (b * 8);
    };
    for (unsigned cond = 0; cond < 15; cond++) {
        unsigned at = cond * 32;
        put(at, 0xe16f0f11, 4);
        put(at + 4, (cond << 28) | 0x06bf2f31, 4);
        put(at + 8, 0xe6bf3fb1, 4);
        put(at + 12, 0xe5840000, 4);
        put(at + 16, 0xe12fff1e, 4);
    }
    put(0x400, 0xba08, 2);
    put(0x402, 0xba4a, 2);
    put(0x404, 0x6020, 2);
    put(0x406, 0x4770, 2);
    return code;
}
int main(int argc, char** argv) {
    auto code = Code();
    if (argc == 2) {
        std::vector<a::FunctionRange> functions;
        std::vector<a::Region> regions;
        for (unsigned cond = 0; cond < 15; cond++) {
            functions.push_back({0x1000 + cond * 32, 20, arm::InstructionSet::Arm});
            regions.push_back(
                {0x1000 + cond * 32, 20, a::RegionKind::Code, arm::InstructionSet::Arm});
        }
        functions.push_back({0x1400, 8, arm::InstructionSet::Thumb});
        regions.push_back({0x1400, 8, a::RegionKind::Code, arm::InstructionSet::Thumb});
        adapters::filesystem::DirectoryBlobSink sink(argv[1]);
        assert(sink.Prepare());
        auto result = application::EmitLiftedCode(
            {code, 0x1000, a::RenderFunctions(functions), a::RenderRegions(regions)}, sink);
        assert(result);
        return 0;
    }
    assert(argc == 3);
    void* handle = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::cerr << dlerror() << '\n';
        return 2;
    }
    lift::runtime::EntryTable table;
    auto reg = reinterpret_cast<void (*)(void*)>(dlsym(handle, "hexlith_lifted_register"));
    assert(reg);
    reg(&table);
    memory::ProcessMemory memories[2];
    for (unsigned i = 0; i < 2; i++) {
        assert(memories[i].Map(0x1000, 0x1000, memory::Backing::Allocate(0x1000), 0,
                               memory::Access::ReadWrite));
        for (unsigned b = 0; b < code.size(); b++)
            assert(memories[i].Write8(0x1000 + b, code[b]));
        assert(memories[i].Write32(0x1f00, 0xef000001));
    }
    unsigned comparisons = 0;
    for (unsigned which = 0; which < 16; which++)
        for (unsigned flags = 0; flags < 16; flags++)
            for (unsigned input : {0U, 1U, 0x80000000U, 0xffffffffU, 0x12345678U})
                for (unsigned limit = 0; limit <= 7; limit++)
                    for (bool fault : {false, true}) {
                        arm::CpuState states[2];
                        for (unsigned i = 0; i < 2; i++) {
                            states[i] = arm::CpuState{};
                            assert(memories[i].Write32(0x1800, 0));
                            states[i].registers[1] = input;
                            states[i].registers[4] = fault ? 0x3000 : 0x1800;
                            states[i].registers[14] = 0x1f00;
                            states[i].registers[15] = which == 15 ? 0x1400 : 0x1000 + which * 32;
                            states[i].thumb = which == 15;
                            states[i].negative = flags & 8;
                            states[i].zero = flags & 4;
                            states[i].carry = flags & 2;
                            states[i].overflow = flags & 1;
                            states[i].saturated = true;
                            states[i].greater_or_equal = 9;
                        }
                        std::uint64_t count[2]{};
                        auto expected = arm::Run(states[0], memories[0], limit, count[0]);
                        lift::Engine engine(table);
                        auto actual = engine.Run(states[1], memories[1], limit, count[1]);
                        assert(actual.reason == expected.reason &&
                               actual.detail == expected.detail && count[0] == count[1]);
                        assert(states[0].registers == states[1].registers &&
                               arm::PackCpsr(states[0]) == arm::PackCpsr(states[1]));
                        assert(memories[0].Read32(0x1800) == memories[1].Read32(0x1800));
                        ++comparisons;
                    }
    dlclose(handle);
    std::cout << "PASS " << comparisons
              << " compiled ARM/Thumb direct integer vs interpreter comparisons; "
                 "all ARM conditions/NZCV, budgets0..7, SVC and store faults\n";
}

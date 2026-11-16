#include "domain/arm/interpreter/step.h"
#include "domain/arm/primitives/unpredictable.h"
#include "domain/lift/evaluate.h"
#include "domain/lift/lower.h"
#include "domain/lift/runtime.h"
#include <array>
#include <cassert>
#include <iostream>
#include <map>
#include <random>
using namespace hexlith::domain;
int main() {
    using O = arm::Operation;
    const std::array wanted{O::Usat,  O::Pkhbt, O::Smlad,   O::Uxtb16,     O::Rev,    O::Usat16,
                            O::Rev16, O::Pkhtb, O::Qadd,    O::QdoubleAdd, O::Ssat16, O::Qsub,
                            O::Smusd, O::Smuad, O::Uxtab16, O::Ssat};
    std::map<O, unsigned> counts;
    for (auto op : wanted)
        counts[op] = 0;
    memory::ProcessMemory memory;
    assert(memory.Map(0x1000, 0x1000, memory::Backing::Allocate(0x1000), 0,
                      memory::Access::ReadWrite));
    std::mt19937 random(0x78251);
    for (unsigned i = 0; i < 64000000; i++) {
        auto word = 0xe0000000U |
                    ((i % 3 == 0   ? 1U
                      : i % 3 == 1 ? 6U
                                   : 7U)
                     << 24) |
                    (random() & 0x00ffffffU);
        auto instruction = arm::DecodeArm(word);
        if (!counts.contains(instruction.operation) || counts[instruction.operation] >= 64 ||
            arm::primitives::HasUnpredictableOperands(instruction))
            continue;
        // Both passing and failing conditions must retain the original behavior.
        if (counts[instruction.operation] % 3 == 0)
            instruction = arm::DecodeArm(word & 0x0fffffffU);
        assert(memory.Write32(0x1000, instruction.encoding));
        auto lowered = lift::Lower({0x1000, instruction});
        const bool direct=instruction.operation==O::Rev||instruction.operation==O::Rev16;
        assert(!lowered.IsInterpreted() && lowered.IsExecutedByAHandler()!=direct);
        arm::CpuState expected;
        for (unsigned r = 0; r < 15; r++)
            expected.registers[r] = random();
        expected.registers[15] = 0x1000;
        expected.negative = random() & 1;
        expected.zero = random() & 1;
        expected.carry = random() & 1;
        expected.overflow = random() & 1;
        expected.saturated = random() & 1;
        expected.greater_or_equal = random() & 15;
        auto actual = expected;
        auto stop = arm::StepDecoded(expected, memory, instruction);
        if (lift::runtime::Passes(actual, lowered.condition)) {
            auto result = lift::Evaluate(lowered, actual, memory);
            assert(!stop.Stopped() && result.kind == lift::EvaluationKind::Completed);
        } else
            assert(!stop.Stopped());
        actual.registers[15] += 4;
        assert(expected.registers == actual.registers &&
               arm::PackCpsr(expected) == arm::PackCpsr(actual));
        ++counts[instruction.operation];
        bool done = true;
        for (auto [op, count] : counts)
            done &= count >= 64;
        if (done)
            break;
    }
    for (auto [op, count] : counts) {
        if (count != 64)
            std::cerr << arm::Mnemonic(op) << " samples=" << count << "\n";
        assert(count == 64);
    }
    std::cout << "PASS 16 media/saturating/DSP families,1024 synthetic states, passing/failing "
                 "conditions, full GPR/CPSR/GE/Q\n";
}

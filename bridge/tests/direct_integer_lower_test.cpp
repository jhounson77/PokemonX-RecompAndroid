#include "domain/arm/interpreter/step.h"
#include "domain/lift/evaluate.h"
#include "domain/lift/lower.h"
#include <array>
#include <cassert>
#include <iostream>
#include <random>
using namespace hexlith::domain;
int main() {
    memory::ProcessMemory memory;
    assert(memory.Map(0x1000, 0x1000, memory::Backing::Allocate(0x1000), 0,
                      memory::Access::ReadWrite));
    std::mt19937 random(0x16c12);
    unsigned tested = 0;
    for (unsigned kind = 0; kind < 5; kind++)
        for (unsigned sample = 0; sample < 4096; sample++) {
            const bool thumb = kind >= 3;
            unsigned rd = random() % (thumb ? 8 : 15), rm = random() % (thumb ? 8 : 15);
            auto d =
                thumb ? arm::DecodeThumb((kind == 3 ? 0xba00U : 0xba40U) | (rm << 3) | rd, 0)
                      : arm::DecodeArm((random() % 15 << 28) |
                                       (std::array{0x016f0f10U, 0x06bf0f30U, 0x06bf0fb0U}[kind]) |
                                       (rd << 12) | rm);
            assert(d.operation == (kind == 0                ? arm::Operation::CountLeadingZeros
                                   : kind == 1 || kind == 3 ? arm::Operation::Rev
                                                            : arm::Operation::Rev16));
            auto ir = lift::Lower({0x1000, d});
            assert(!ir.IsInterpreted() && !ir.IsExecutedByAHandler());
            arm::CpuState expected;
            for (unsigned r = 0; r < 15; r++)
                expected.registers[r] = random();
            expected.registers[rm] = sample < 32    ? (1U << sample)
                                     : sample == 32 ? 0
                                     : sample == 33 ? 0xffffffffU
                                                    : random();
            expected.registers[15] = 0x1000;
            expected.thumb = thumb;
            expected.negative = random() & 1;
            expected.zero = random() & 1;
            expected.carry = random() & 1;
            expected.overflow = random() & 1;
            expected.saturated = random() & 1;
            expected.greater_or_equal = random() & 15;
            auto actual = expected;
            auto stop = arm::StepDecoded(expected, memory, d);
            auto result = lift::Evaluate(ir, actual, memory);
            assert(!stop.Stopped() && result.kind == lift::EvaluationKind::Completed);
            actual.registers[15] += d.width;
            assert(expected.registers == actual.registers &&
                   arm::PackCpsr(expected) == arm::PackCpsr(actual));
            ++tested;
        }
    // PC operand forms must keep the existing unpredictable-operand fallback.
    for (unsigned word :
         {0xe16fff10U, 0xe16f0f1fU, 0xe6bfff30U, 0xe6bf0f3fU, 0xe6bfffb0U, 0xe6bf0fbfU})
        assert(lift::Lower({0x1000, arm::DecodeArm(word)}).IsInterpreted());
    std::cout << "PASS " << tested
              << " ARM/Thumb direct CLZ/REV/REV16 cases; aliased registers, zero/bit "
                 "edges, conditions, all GPR/CPSR/GE/Q and PC fallback\n";
}

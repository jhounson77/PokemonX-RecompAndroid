#include "domain/arm/interpreter/step.h"
#include "domain/lift/evaluate.h"
#include "domain/lift/lower.h"
#include <cassert>
#include <iostream>
#include <map>
#include <random>
using namespace hexlith::domain;
int main() {
    memory::ProcessMemory memory;
    assert(memory.Map(0x1000, 0x1000, memory::Backing::Allocate(0x1000), 0,
                      memory::Access::ReadWrite));
    std::mt19937 random(0x4142);
    std::map<std::pair<unsigned, unsigned>, arm::Instruction> variants;
    for (unsigned prefix_bits = 0; prefix_bits < 8; prefix_bits++)
        for (unsigned shape_bits = 0; shape_bits < 8; shape_bits++) {
            auto decoded = arm::DecodeArm(0xe6010f12U | (prefix_bits << 20) | (shape_bits << 5));
            if (decoded.operation == arm::Operation::ParallelArithmetic)
                variants[{unsigned(decoded.parallel_prefix), unsigned(decoded.parallel_shape)}] =
                    decoded;
        }
    assert(variants.size() == 36);
    for (unsigned prefix = 0; prefix < 6; prefix++)
        for (unsigned shape = 0; shape < 6; shape++)
            for (unsigned trial = 0; trial < 100; trial++) {
                auto instruction = variants.at({prefix, shape});
                assert(memory.Write32(0x1000, instruction.encoding));
                arm::CpuState expected;
                expected.registers[15] = 0x1000;
                expected.registers[1] = random();
                expected.registers[2] = random();
                expected.saturated = random() & 1;
                expected.greater_or_equal = random() & 15;
                auto actual = expected;
                auto lowered = lift::Lower({0x1000, instruction});
                assert(lowered.IsExecutedByAHandler());
                auto stop = arm::StepDecoded(expected, memory, instruction);
                auto evaluated = lift::Evaluate(lowered, actual, memory);
                assert(!stop.Stopped() && evaluated.kind == lift::EvaluationKind::Completed);
                actual.registers[15] += 4;
                assert(actual.registers == expected.registers &&
                       arm::PackCpsr(actual) == arm::PackCpsr(expected));
            }
    for (unsigned ge = 0; ge < 16; ge++)
        for (unsigned trial = 0; trial < 100; trial++) {
            auto instruction = arm::DecodeArm(0xe6810fb2);
            assert(instruction.operation == arm::Operation::Sel);
            assert(memory.Write32(0x1000, instruction.encoding));
            arm::CpuState expected;
            expected.registers[15] = 0x1000;
            expected.registers[1] = random();
            expected.registers[2] = random();
            expected.greater_or_equal = ge;
            expected.saturated = true;
            auto actual = expected;
            auto lowered = lift::Lower({0x1000, instruction});
            assert(lowered.IsExecutedByAHandler());
            auto stop = arm::StepDecoded(expected, memory, instruction);
            auto evaluated = lift::Evaluate(lowered, actual, memory);
            assert(!stop.Stopped() && evaluated.kind == lift::EvaluationKind::Completed);
            actual.registers[15] += 4;
            assert(actual.registers == expected.registers &&
                   arm::PackCpsr(actual) == arm::PackCpsr(expected));
            instruction.destination = 15;
            assert(lift::Lower({0x1000, instruction}).IsInterpreted());
        }
    std::cout << "PASS 36 parallel lane variants and SEL across all16 GE masks; registers/CPSR/Q, "
                 "randomized operands, invalid PC rejection\n";
}

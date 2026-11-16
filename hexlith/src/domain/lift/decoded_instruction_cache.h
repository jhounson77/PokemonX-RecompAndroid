#ifndef HEXLITH_DOMAIN_LIFT_DECODED_INSTRUCTION_CACHE_H
#define HEXLITH_DOMAIN_LIFT_DECODED_INSTRUCTION_CACHE_H

#include "domain/arm/interpreter/step.h"
#include <array>
#include <cstdint>

namespace hexlith::domain::lift {

// Cache decoding and instruction-only operand legality. Fetch remains live so code changes and access faults
// have exactly the same behavior as the interpreter.
class DecodedInstructionCache {
  public:
    struct Statistics {
        std::uint64_t hits = 0;
        std::uint64_t misses = 0;
    };

    arm::Stop Step(arm::CpuState& state, memory::ProcessMemory& memory) {
        const auto pc = state.registers.at(arm::ProgramCounter);
        std::uint32_t word = 0;
        if (!state.thumb) {
            const auto fetched = memory.Read32(pc);
            if (!fetched)
                return {.reason = arm::StopReason::FetchFault, .detail = pc};
            word = *fetched;
        } else {
            const auto first = memory.Read16(pc);
            if (!first)
                return {.reason = arm::StopReason::FetchFault, .detail = pc};
            word = *first;
            if ((*first >> 11) == 0b11110) {
                const auto second = memory.Read16(pc + 2);
                if (!second)
                    return {.reason = arm::StopReason::FetchFault, .detail = pc + 2};
                word |= std::uint32_t(*second) << 16;
            }
        }
        auto& set =
            sets_[(word ^ (word >> 11) ^ (word >> 21) ^ (state.thumb ? 0xa5 : 0)) & (SetCount - 1)];
        for (unsigned way = 0; way < 2; ++way) {
            auto& row = set.rows[way];
            if (row.valid && row.word == word && row.thumb == state.thumb) {
                ++statistics_.hits;
                set.victim = way ^ 1;
                return arm::StepDecodedImpl<true>(state, memory, row.instruction, row.unpredictable);
            }
        }
        auto& row = set.rows[set.victim];
        row.instruction =
            state.thumb ? arm::DecodeThumb(word & 0xffff, word >> 16) : arm::DecodeArm(word);
        row.unpredictable = arm::primitives::HasUnpredictableOperands(row.instruction);
        row.word = word;
        row.thumb = state.thumb;
        row.valid = true;
        set.victim ^= 1;
        ++statistics_.misses;
        return arm::StepDecodedImpl<true>(state, memory, row.instruction, row.unpredictable);
    }

    const Statistics& Counted() const { return statistics_; }

  private:
    struct Row {
        arm::Instruction instruction{};
        std::uint32_t word = 0;
        bool thumb = false;
        bool valid = false;
        bool unpredictable = false;
    };
    // Two ways avoid decoding alternating encodings that hash to the same set.
    // Fixed capacity keeps memory bounded, with no allocation in the hot path.
    static constexpr unsigned SetCount = 512;
    struct Set {
        std::array<Row, 2> rows{};
        unsigned victim = 0;
    };
    std::array<Set, SetCount> sets_{};
    Statistics statistics_{};
};
} // namespace hexlith::domain::lift
#endif

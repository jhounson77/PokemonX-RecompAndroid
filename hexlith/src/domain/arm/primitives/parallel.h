#ifndef HEXLITH_DOMAIN_ARM_PRIMITIVES_PARALLEL_H
#define HEXLITH_DOMAIN_ARM_PRIMITIVES_PARALLEL_H

#include "domain/arm/instruction.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

// The parallel arithmetic instructions: two halfwords or four bytes added or
// subtracted side by side in one register, the SIMD of a core that predates
// NEON.
//
// Thirty-six instructions, written once. The shape says which lane of each
// operand meets which, and whether they are added or subtracted; the prefix
// says what happens to each lane's exact result. Modular arithmetic wraps and
// sets one GE bit per byte of the result, saturating arithmetic clamps, and
// halving arithmetic keeps the top bits of the exact result.
//
// Source: the SADD16 to UHSUB8 pages of the ARMv7-A and ARMv7-R reference
// manual (DDI 0406C), which give each of the thirty-six its own pseudocode. The
// table form below is checked against all of them by the Unicorn comparison.
namespace hexlith::domain::arm::primitives {

struct ParallelResult {
    std::uint32_t value = 0;

    // GE[3:0], one bit per byte. A halfword lane sets the two bits of its two
    // bytes together. Meaningful only when the prefix is modular; the other
    // prefixes leave the flags alone, and the caller must not write this back.
    std::uint8_t greater_or_equal = 0;
};

namespace parallel_detail {

constexpr bool IsSigned(ParallelPrefix prefix) {
    return prefix == ParallelPrefix::Signed || prefix == ParallelPrefix::SignedSaturating ||
           prefix == ParallelPrefix::SignedHalving;
}

constexpr bool IsSaturating(ParallelPrefix prefix) {
    return prefix == ParallelPrefix::SignedSaturating ||
           prefix == ParallelPrefix::UnsignedSaturating;
}

constexpr bool IsHalving(ParallelPrefix prefix) {
    return prefix == ParallelPrefix::SignedHalving || prefix == ParallelPrefix::UnsignedHalving;
}

// Lane n of a value, extended to a full integer the way the prefix reads it.
constexpr std::int32_t Lane(std::uint32_t value, unsigned lane, unsigned width, bool is_signed) {
    const std::uint32_t mask = (1U << width) - 1U;
    const std::uint32_t raw = (value >> (lane * width)) & mask;
    if (!is_signed) {
        return static_cast<std::int32_t>(raw);
    }
    const std::uint32_t sign = 1U << (width - 1);
    return static_cast<std::int32_t>(raw ^ sign) - static_cast<std::int32_t>(sign);
}

// What each result lane is made of: which lane of the second operand it uses,
// and whether it is a sum or a difference. The first operand's lane is always
// the result lane's own.
struct LanePlan {
    unsigned other_lane = 0;
    bool subtract = false;
};

constexpr LanePlan Sum(unsigned other_lane) {
    return {.other_lane = other_lane, .subtract = false};
}

constexpr LanePlan Difference(unsigned other_lane) {
    return {.other_lane = other_lane, .subtract = true};
}

constexpr std::array<LanePlan, 4> Plan(ParallelShape shape) {
    switch (shape) {
    case ParallelShape::Add16:
        return {Sum(0), Sum(1)};
    case ParallelShape::AddSubtractExchange:
        // ASX: the low lane subtracts the other's high half and the high lane
        // adds its low half.
        return {Difference(1), Sum(0)};
    case ParallelShape::SubtractAddExchange:
        return {Sum(1), Difference(0)};
    case ParallelShape::Subtract16:
        return {Difference(0), Difference(1)};
    case ParallelShape::Add8:
        return {Sum(0), Sum(1), Sum(2), Sum(3)};
    case ParallelShape::Subtract8:
        return {Difference(0), Difference(1), Difference(2), Difference(3)};
    }
    return {};
}

constexpr bool IsByteShape(ParallelShape shape) {
    return shape == ParallelShape::Add8 || shape == ParallelShape::Subtract8;
}

// The GE condition for one lane in modular arithmetic. Signed: the exact result
// is not negative. Unsigned: a sum carried out of the lane, or a difference did
// not borrow.
constexpr bool
LaneGreaterOrEqual(std::int32_t exact, unsigned width, bool is_signed, bool subtract) {
    if (is_signed || subtract) {
        return exact >= 0;
    }
    return exact >= (std::int32_t{1} << width);
}

constexpr std::int32_t Clamp(std::int32_t exact, unsigned width, bool is_signed) {
    const std::int32_t maximum = is_signed ? (1 << (width - 1)) - 1 : (1 << width) - 1;
    const std::int32_t minimum = is_signed ? -(1 << (width - 1)) : 0;
    if (exact > maximum) {
        return maximum;
    }
    if (exact < minimum) {
        return minimum;
    }
    return exact;
}

// The arithmetic for one prefix and one shape. Both are template arguments so
// that the lane count, the plan and the clamping are decided when this is
// compiled, not on every call. The generic loop that chose among the thirty-six
// cases each time cost 4 percent of the user cycles of a boot to frame 1101.
template<ParallelPrefix Prefix, ParallelShape Shape>
constexpr ParallelResult Compute(std::uint32_t first, std::uint32_t second) {
    constexpr unsigned ByteWidth = 8;
    constexpr unsigned HalfWidth = 16;

    constexpr bool Signed = IsSigned(Prefix);
    constexpr unsigned Width = IsByteShape(Shape) ? ByteWidth : HalfWidth;
    constexpr unsigned Lanes = IsByteShape(Shape) ? 4 : 2;
    constexpr std::uint32_t LaneMask = (1U << Width) - 1U;
    constexpr std::array<LanePlan, 4> Plans = Plan(Shape);

    ParallelResult result;
    for (unsigned lane = 0; lane < Lanes; ++lane) {
        const LanePlan step = Plans.at(lane);
        const std::int32_t mine = Lane(first, lane, Width, Signed);
        const std::int32_t other = Lane(second, step.other_lane, Width, Signed);
        const std::int32_t exact = step.subtract ? mine - other : mine + other;

        std::int32_t kept = exact;
        if (IsSaturating(Prefix)) {
            kept = Clamp(exact, Width, Signed);
        } else if (IsHalving(Prefix)) {
            kept = exact >> 1;
        } else if (LaneGreaterOrEqual(exact, Width, Signed, step.subtract)) {
            // One GE bit per byte the lane covers.
            const unsigned bytes = Width / ByteWidth;
            const unsigned bits = (1U << bytes) - 1U;
            result.greater_or_equal =
                static_cast<std::uint8_t>(result.greater_or_equal | (bits << (lane * bytes)));
        }

        result.value |= (static_cast<std::uint32_t>(kept) & LaneMask) << (lane * Width);
    }
    return result;
}

using ComputeFunction = ParallelResult (*)(std::uint32_t, std::uint32_t);

constexpr std::size_t PrefixCount = 6;
constexpr std::size_t ShapeCount = 6;

template<std::size_t Index>
constexpr ComputeFunction ComputeAt() {
    return &Compute<static_cast<ParallelPrefix>(Index / ShapeCount),
                    static_cast<ParallelShape>(Index % ShapeCount)>;
}

template<std::size_t... Indexes>
constexpr std::array<ComputeFunction, PrefixCount * ShapeCount>
ComputeTable(std::index_sequence<Indexes...> /*indexes*/) {
    return {ComputeAt<Indexes>()...};
}

inline constexpr std::array<ComputeFunction, PrefixCount * ShapeCount> ComputeFor =
    ComputeTable(std::make_index_sequence<PrefixCount * ShapeCount>{});

}  // namespace parallel_detail

[[nodiscard]] constexpr ParallelResult ParallelArithmetic(ParallelPrefix prefix,
                                                          ParallelShape shape,
                                                          std::uint32_t first,
                                                          std::uint32_t second) {
    using namespace parallel_detail;
    const std::size_t index =
        (static_cast<std::size_t>(prefix) * ShapeCount) + static_cast<std::size_t>(shape);
    return ComputeFor.at(index)(first, second);
}

// Whether the prefix sets the GE flags, which only modular arithmetic does.
[[nodiscard]] constexpr bool SetsGreaterOrEqual(ParallelPrefix prefix) {
    return prefix == ParallelPrefix::Signed || prefix == ParallelPrefix::Unsigned;
}

// SEL: each byte from the first operand where its GE bit is set, and from the
// second where it is clear.
[[nodiscard]] constexpr std::uint32_t
Select(std::uint32_t first, std::uint32_t second, std::uint8_t greater_or_equal) {
    constexpr unsigned ByteWidth = 8;
    constexpr std::uint32_t ByteMask = 0xFF;
    std::uint32_t result = 0;
    for (unsigned lane = 0; lane < 4; ++lane) {
        const bool from_first = ((greater_or_equal >> lane) & 1U) != 0;
        const std::uint32_t mask = ByteMask << (lane * ByteWidth);
        result |= (from_first ? first : second) & mask;
    }
    return result;
}

}  // namespace hexlith::domain::arm::primitives

#endif  // HEXLITH_DOMAIN_ARM_PRIMITIVES_PARALLEL_H

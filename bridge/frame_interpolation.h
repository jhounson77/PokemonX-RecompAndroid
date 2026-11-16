#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace PokemonX::Smoothing {
using Matrix = std::array<float, 12>;

// The adapter must supply a verified object lifetime, not just a reused address.
struct Key {
    std::uint64_t object;
    std::uint32_t generation;
    std::uint32_t matrix;
    bool operator==(const Key&) const = default;
};
struct KeyHash {
    std::size_t operator()(const Key& key) const {
        return std::hash<std::uint64_t>{}(key.object) ^
               (std::hash<std::uint32_t>{}(key.generation) << 1) ^
               (std::hash<std::uint32_t>{}(key.matrix) << 2);
    }
};

// Presentation-only history. No guest-memory pointer, clock, input or RNG is retained.
// A render adapter must prove continuity and restore all modified matrices before update.
class History {
  public:
    explicit History(std::size_t capacity) : capacity_(std::min(capacity, MaximumEntries)) {
        entries_.reserve(capacity_);
    }
    void Clear() { entries_.clear(); }
    std::size_t Size() const { return entries_.size(); }

    Matrix Sample(Key key, std::uint64_t frame, const Matrix& value, bool continuous) {
        if (capacity_ == 0)
            return value;
        const bool finite =
            std::all_of(value.begin(), value.end(), [](float x) { return std::isfinite(x); });
        auto found = entries_.find(key);
        if (!finite) {
            if (found != entries_.end())
                entries_.erase(found);
            return value;
        }
        if (found == entries_.end()) {
            if (entries_.size() == capacity_) {
                const auto oldest = std::min_element(
                    entries_.begin(), entries_.end(),
                    [](const auto& a, const auto& b) { return a.second.seen < b.second.seen; });
                entries_.erase(oldest);
            }
            entries_.emplace(key, Fresh(frame, value));
            return value;
        }
        Record& record = found->second;
        if (!continuous || frame < record.seen || frame - record.seen > MaximumGap) {
            record = Fresh(frame, value);
            return value;
        }
        record.seen = frame;
        if (record.current != value) {
            const auto period = frame - record.changed;
            // More than one source value in the same frame is not a stable matrix stream.
            if (period == 0 || period > MaximumGap) {
                record = Fresh(frame, value);
                return value;
            }
            record.previous = record.current;
            record.current = value;
            record.period = period;
            record.changed = frame;
            record.has_previous = true;
        }
        if (!record.has_previous || frame - record.changed >= record.period - 1)
            return value;
        const float alpha =
            static_cast<float>(frame - record.changed + 1) / static_cast<float>(record.period);
        Matrix result;
        for (std::size_t i = 0; i < result.size(); ++i) {
            // A double intermediate avoids overflow for finite opposite-sign float endpoints.
            const double first = record.previous[i];
            result[i] = static_cast<float>(first + (double(record.current[i]) - first) * alpha);
        }
        return result;
    }

  private:
    static constexpr std::size_t MaximumEntries = 4096;
    static constexpr std::uint64_t MaximumGap = 8;
    struct Record {
        Matrix previous, current;
        std::uint64_t changed, seen, period;
        bool has_previous;
    };
    static Record Fresh(std::uint64_t frame, const Matrix& value) {
        return {value, value, frame, frame, 1, false};
    }
    const std::size_t capacity_;
    std::unordered_map<Key, Record, KeyHash> entries_;
};

// Only for a validated native draw scope whose matrix storage remains alive.
// Guest-memory adapters must implement the equivalent checked read/write transaction.
class RestoreOnExit {
  public:
    RestoreOnExit(Matrix& target, const Matrix& image) : target_(target), original_(target) {
        target_ = image;
    }
    ~RestoreOnExit() { target_ = original_; }
    RestoreOnExit(const RestoreOnExit&) = delete;
    RestoreOnExit& operator=(const RestoreOnExit&) = delete;

  private:
    Matrix& target_;
    const Matrix original_;
};
} // namespace PokemonX::Smoothing

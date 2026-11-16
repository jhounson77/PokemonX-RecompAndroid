#ifndef HEXLITH_DOMAIN_LIFT_EXECUTION_PROFILE_H
#define HEXLITH_DOMAIN_LIFT_EXECUTION_PROFILE_H

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace hexlith::domain::lift::profile {
enum class Stage : unsigned { Lookup, Native, Fallback, Count };
constexpr unsigned StageCount = static_cast<unsigned>(Stage::Count);
struct Row { std::uint64_t calls = 0, ns = 0, retired = 0; };
struct Snapshot {
    std::uint64_t runs = 0, sampled_runs = 0, sampled_ns = 0;
    std::array<Row, StageCount> stages{};
};
struct AtomicRow { std::atomic<std::uint64_t> calls{0}, ns{0}, retired{0}; };
inline std::atomic<std::uint64_t> runs{0}, sampled_runs{0}, sampled_ns{0};
inline std::array<AtomicRow, StageCount> stages{};
using Clock = std::chrono::steady_clock;
inline std::uint64_t Nanoseconds(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}
// Independent of guest state. Nonzero xorshift state selects approximately
// one in 256 runs without coupling sampling to a periodic guest loop.
inline bool Select(std::uint32_t& state) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    runs.fetch_add(1, std::memory_order_relaxed);
    return (state & 255) == 0;
}
class Run {
public:
    explicit Run(bool selected) : selected_(selected) {
        if (selected_) start_ = Clock::now();
    }
    ~Run() {
        if (!selected_) return;
        const auto elapsed = Nanoseconds(start_);
        for (unsigned i = 0; i < StageCount; ++i) {
            stages[i].calls.fetch_add(local_[i].calls, std::memory_order_relaxed);
            stages[i].ns.fetch_add(local_[i].ns, std::memory_order_relaxed);
            stages[i].retired.fetch_add(local_[i].retired, std::memory_order_relaxed);
        }
        sampled_ns.fetch_add(elapsed, std::memory_order_relaxed);
        sampled_runs.fetch_add(1, std::memory_order_relaxed);
    }
    bool Selected() const { return selected_; }
    Row& At(Stage stage) { return local_[static_cast<unsigned>(stage)]; }
    Run(const Run&) = delete;
    Run& operator=(const Run&) = delete;
private:
    bool selected_;
    Clock::time_point start_{};
    std::array<Row, StageCount> local_{};
};
template<bool Enabled = true>
class Scope {
public:
    Scope(Run& run, Stage stage) : Scope(&run, stage) {}
    Scope(Run* run, Stage stage) : row_(nullptr) {
        if constexpr (Enabled) row_ = run && run->Selected() ? &run->At(stage) : nullptr;
        if (row_) start_ = Clock::now();
    }
    ~Scope() { if (row_) { row_->ns += Nanoseconds(start_); ++row_->calls; } }
    void Retired(std::uint64_t count) { if (row_) row_->retired += count; }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
private:
    Row* row_;
    Clock::time_point start_{};
};
inline Snapshot Take() {
    Snapshot s;
    s.runs = runs.exchange(0, std::memory_order_relaxed);
    s.sampled_runs = sampled_runs.exchange(0, std::memory_order_relaxed);
    s.sampled_ns = sampled_ns.exchange(0, std::memory_order_relaxed);
    for (unsigned i = 0; i < StageCount; ++i) {
        s.stages[i].calls = stages[i].calls.exchange(0, std::memory_order_relaxed);
        s.stages[i].ns = stages[i].ns.exchange(0, std::memory_order_relaxed);
        s.stages[i].retired = stages[i].retired.exchange(0, std::memory_order_relaxed);
    }
    return s;
}
} // namespace hexlith::domain::lift::profile
#endif

#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace PokemonXTiming {
struct Sample {
    std::uint64_t guest_calls{}, guest_ns{}, readbacks{}, sync_ns{};
    std::uint64_t staged_bytes{}, scaled_readbacks{}, inside_guest_ns{};
};
struct Counters {
    std::atomic<std::uint64_t> guest_calls{}, guest_ns{}, readbacks{}, sync_ns{};
    std::atomic<std::uint64_t> staged_bytes{}, scaled_readbacks{}, inside_guest_ns{};
};
inline Counters counters;
inline thread_local unsigned guest_depth = 0;
inline void RecordGuest(std::uint64_t ns) {
    counters.guest_ns.fetch_add(ns, std::memory_order_relaxed);
    counters.guest_calls.fetch_add(1, std::memory_order_relaxed);
}
inline void RecordReadback(std::uint64_t ns, std::uint64_t bytes, bool scaled) {
    counters.sync_ns.fetch_add(ns, std::memory_order_relaxed);
    counters.staged_bytes.fetch_add(bytes, std::memory_order_relaxed);
    counters.readbacks.fetch_add(1, std::memory_order_relaxed);
    if (scaled)
        counters.scaled_readbacks.fetch_add(1, std::memory_order_relaxed);
    if (guest_depth)
        counters.inside_guest_ns.fetch_add(ns, std::memory_order_relaxed);
}
inline Sample Take() {
    // Each counter is exact over its lifetime; windows can straddle one event.
    return {counters.guest_calls.exchange(0, std::memory_order_relaxed),
            counters.guest_ns.exchange(0, std::memory_order_relaxed),
            counters.readbacks.exchange(0, std::memory_order_relaxed),
            counters.sync_ns.exchange(0, std::memory_order_relaxed),
            counters.staged_bytes.exchange(0, std::memory_order_relaxed),
            counters.scaled_readbacks.exchange(0, std::memory_order_relaxed),
            counters.inside_guest_ns.exchange(0, std::memory_order_relaxed)};
}
enum class Stage : std::size_t {
    Dma,
    Submit,
    Fill,
    DisplayTransfer,
    TextureCopy,
    CacheFlush,
    Other,
    PipelineLookup,
    PipelineTryBuild,
    PipelineWorkerWait,
    ShaderWait,
    PipelineCacheProbe,
    PipelineCreate,
    Count
};
inline constexpr auto StageCount = static_cast<std::size_t>(Stage::Count);
struct StageSample {
    std::uint64_t calls{}, ns{}, max_ns{};
};
struct StageCounters {
    std::atomic<std::uint64_t> calls{}, ns{}, max_ns{};
};
inline std::array<StageCounters, StageCount> stage_counters;
inline Stage CommandStage(std::uint32_t id) {
    // Service::GSP::CommandId assigns DMA..CacheFlush the values0..5.
    return id < 6 ? static_cast<Stage>(id) : Stage::Other;
}
inline const char* StageName(Stage stage) {
    constexpr std::array names{"command_dma",          "command_submit",
                               "command_fill",         "command_display_transfer",
                               "command_texture_copy", "command_cache_flush",
                               "command_other",        "pipeline_lookup",
                               "pipeline_try_build",   "pipeline_worker_wait",
                               "pipeline_shader_wait", "pipeline_cache_probe",
                               "pipeline_create"};
    const auto index = static_cast<std::size_t>(stage);
    return index < names.size() ? names[index] : "unknown";
}
inline void RecordStage(Stage stage, std::uint64_t ns) {
    const auto index = static_cast<std::size_t>(stage);
    if (index >= StageCount)
        return;
    auto& counter = stage_counters[index];
    counter.calls.fetch_add(1, std::memory_order_relaxed);
    counter.ns.fetch_add(ns, std::memory_order_relaxed);
    auto previous = counter.max_ns.load(std::memory_order_relaxed);
    while (previous < ns &&
           !counter.max_ns.compare_exchange_weak(previous, ns, std::memory_order_relaxed)) {
    }
}
inline std::array<StageSample, StageCount> TakeStages() {
    std::array<StageSample, StageCount> result{};
    for (std::size_t i = 0; i < StageCount; ++i) {
        auto& c = stage_counters[i];
        result[i] = {c.calls.exchange(0, std::memory_order_relaxed),
                     c.ns.exchange(0, std::memory_order_relaxed),
                     c.max_ns.exchange(0, std::memory_order_relaxed)};
    }
    return result;
}
using Clock = std::chrono::steady_clock;
inline std::uint64_t Elapsed(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}
struct StageScope {
    Stage stage;
    Clock::time_point start{Clock::now()};
    explicit StageScope(Stage value) : stage(value) {}
    ~StageScope() { RecordStage(stage, Elapsed(start)); }
    StageScope(const StageScope&) = delete;
    StageScope& operator=(const StageScope&) = delete;
};
struct GuestScope {
    Clock::time_point start{Clock::now()};
    GuestScope() { ++guest_depth; }
    ~GuestScope() {
        --guest_depth;
        RecordGuest(Elapsed(start));
    }
    GuestScope(const GuestScope&) = delete;
    GuestScope& operator=(const GuestScope&) = delete;
};
} // namespace PokemonXTiming

#include "domain/lift/engine.h"

#include "domain/lift/runtime.h"
#if defined(PX_PROFILE_EXECUTION)
#include "domain/lift/execution_profile.h"
#endif

namespace hexlith::domain::lift {
namespace {

struct LiftedCode {
    runtime::Function function = nullptr;
    bool in_module = false;
};

// The lifted code at an address: the main executable's, or the code of a module
// that is loaded there, in which case the context is given its base.
[[gnu::always_inline]] inline LiftedCode FindLiftedCode(const runtime::EntryTable& entries,
                          const runtime::LoadedModules* modules,
                          runtime::Context& context,
                          std::uint32_t pc,
                          bool thumb) {
    if (entries.MayCover(pc)) {
        if (const runtime::Function function = entries.Find(pc, thumb)) {
            return {.function = function, .in_module = false};
        }
    }
    if (modules != nullptr) {
        if (const auto found = modules->Find(pc, thumb)) {
            context.base = found->base;
            return {.function = found->function, .in_module = true};
        }
    }
    return {};
}

}  // namespace

arm::Stop Engine::Run(arm::CpuState& state,
                      memory::ProcessMemory& memory,
                      std::uint64_t limit,
                      std::uint64_t& executed) const {
#if defined(PX_PROFILE_EXECUTION)
    if (profile::Select(execution_sample_state_)) {
        profile::Run sample(true);
        return RunCore<true>(state, memory, limit, executed, &sample);
    }
#endif
    return RunCore<false>(state, memory, limit, executed, nullptr);
}

// Keep the common untimed loop in a separate function with its own small stack
// and register allocation. The sampled accumulator must not bloat that frame.
template<bool Sampled>
[[gnu::noinline]] arm::Stop Engine::RunCore(arm::CpuState& state,
                      memory::ProcessMemory& memory,
                      std::uint64_t limit,
                      std::uint64_t& executed,
                      profile::Run* sample) const {
    runtime::Context context{.cpu = &state,
                             .memory = &memory,
                             .entries = entries_,
                             .modules = modules_,
                             .budget = static_cast<std::int64_t>(limit),
                             .stop = {}};

    const auto finish = [&](arm::Stop stop) {
        executed = limit - static_cast<std::uint64_t>(context.budget);
        retired_ += executed;
        return stop;
    };

    while (context.budget > 0) {
        const std::uint32_t pc = state.registers.at(arm::ProgramCounter);

        // Lifted code runs until the guest leaves it, and answers where the
        // guest continues. An address is somewhere to continue; the two other
        // answers say the lifted code gave up, and have the interpreter run the
        // instruction at r15 or report why it stopped.
        const LiftedCode lifted = [&] {
#if defined(PX_PROFILE_EXECUTION)
            profile::Scope<Sampled> timing(sample, profile::Stage::Lookup);
#endif
            return FindLiftedCode(*entries_, modules_, context, pc, state.thumb);
        }();
        if (lifted.function != nullptr) {
            ++statistics_.entries;
            statistics_.module_entries += lifted.in_module ? 1 : 0;
            const runtime::Pc next = [&] {
#if defined(PX_PROFILE_EXECUTION)
                profile::Scope<Sampled> timing(sample, profile::Stage::Native);
                const auto before = context.budget;
#endif
                const auto result = lifted.function(context, pc);
#if defined(PX_PROFILE_EXECUTION)
                timing.Retired(static_cast<std::uint64_t>(before - context.budget));
#endif
                return result;
            }();
            if (next == runtime::Stopped) {
                return finish(context.stop);
            }
            if (next != runtime::Interpret) {
                state.registers.at(arm::ProgramCounter) = next;
                continue;
            }
            // The block that gave up may have been entered with nothing left.
            if (context.budget <= 0) {
                break;
            }
        }

#if defined(PX_NATIVE_FALLBACK_PROFILE)
        fallback_profile_.Observe(state.registers.at(arm::ProgramCounter), state.thumb,
                                  lifted.function != nullptr);
#elif defined(PX_PROFILE_EXECUTION)
        // Only the already selected diagnostic runs collect PC presence. The
        // common untimed loop has no per-instruction sampling work.
        if constexpr (Sampled) {
            fallback_profile_.Observe(state.registers.at(arm::ProgramCounter), state.thumb,
                                      lifted.function != nullptr);
        }
#endif
        const arm::Stop stop = [&] {
#if defined(PX_PROFILE_EXECUTION)
            profile::Scope<Sampled> timing(sample, profile::Stage::Fallback);
#endif
            const auto result = decode_cache_.Step(state, memory);
#if defined(PX_PROFILE_EXECUTION)
            timing.Retired(!result.Stopped() || result.reason == arm::StopReason::SupervisorCall);
#endif
            return result;
        }();
        ++statistics_.interpreted;
        if (stop.Stopped()) {
            // A supervisor call has completed: r15 is already past it.
            if (stop.reason == arm::StopReason::SupervisorCall) {
                --context.budget;
            } else {
                ++nonretired_fallback_;
            }
            return finish(stop);
        }
        --context.budget;
    }
    return finish(arm::Stop{.reason = arm::StopReason::StepLimit, .detail = 0});
}

}  // namespace hexlith::domain::lift

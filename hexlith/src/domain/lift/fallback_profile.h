#ifndef HEXLITH_DOMAIN_LIFT_FALLBACK_PROFILE_H
#define HEXLITH_DOMAIN_LIFT_FALLBACK_PROFILE_H

#include <array>
#include <cstdint>

namespace hexlith::domain::lift {

// Bounded Space-Saving sketch of sampled fallback sites. Counts estimate
// samples, never total instructions or CPU time. No memory or code bytes read.
class FallbackProfile {
  public:
    struct Site {
        std::uint32_t pc = 0;
        bool thumb = false;
        bool native_yield = false;
        std::uint64_t count = 0;
        std::uint64_t error = 0;
    };

    void Observe(std::uint32_t pc, bool thumb, bool native_yield) {
        if (--remaining_ != 0)
            return;
        // Vary the stride to avoid locking onto a loop whose length divides1024.
        random_ = random_ * 1664525U + 1013904223U;
        remaining_ = 512 + ((random_ >> 16) & 1023);
        ++samples_;
        Site* victim = &sites_[0];
        for (auto& site : sites_) {
            if (site.count && site.pc == pc && site.thumb == thumb &&
                site.native_yield == native_yield) {
                ++site.count;
                return;
            }
            if (site.count < victim->count)
                victim = &site;
        }
        const auto minimum = victim->count;
        *victim = {.pc = pc,
                   .thumb = thumb,
                   .native_yield = native_yield,
                   .count = minimum + 1,
                   .error = minimum};
    }

    std::array<Site, 32> Snapshot() const { return sites_; }
    std::uint64_t Samples() const { return samples_; }

  private:
    std::array<Site, 32> sites_{};
    std::uint32_t remaining_ = 1024;
    std::uint32_t random_ = 0x419f03;
    std::uint64_t samples_ = 0;
};
} // namespace hexlith::domain::lift
#endif

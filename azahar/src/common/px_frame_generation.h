#pragma once
#include <atomic>
#include <cstdint>

namespace PokemonXFrames {
inline std::atomic<int> target{30};
inline std::atomic<std::uint64_t> epoch{0}, submitted{0}, generated{0};
enum class Kind { Skip, Original, Previous, Intermediate };
struct Decision {
    Kind kind;
    bool capture = false, reset = false;
};

// Output policy only. It cannot change emulated clocks, inputs or game updates.
class Policy {
  public:
    void Reset() { *this = Policy{}; }
    Decision Step(int target, bool updated, bool eligible, std::uint64_t epoch, std::uint64_t now) {
        bool reset = !initialized_ || target != target_ || epoch != epoch_ || now < time_ ||
                     eligible != eligible_;
        if (reset) {
            Reset();
            initialized_ = true;
            target_ = target;
            epoch_ = epoch;
            eligible_ = eligible;
        }
        if (!reset && now - time_ > 24000000ULL)
            cooldown_ = 60;
        time_ = now;
        if (cooldown_)
            --cooldown_;
        auto frame = frame_++;
        if (target != 60) {
            pending_ |= updated;
            bool show = pending_ && (!have_present_ || frame - last_present_ >= 2);
            if (show) {
                pending_ = false;
                have_present_ = true;
                last_present_ = frame;
            }
            return {show ? Kind::Original : Kind::Skip, false, reset};
        }
        if (!eligible) {
            stable_ = 0;
            paired_ = false;
            return {updated ? Kind::Original : Kind::Skip, false, reset};
        }
        if (updated) {
            auto gap = have_update_ ? frame - last_update_ : 0;
            last_update_ = frame;
            have_update_ = true;
            stable_ = gap == 2 ? stable_ + 1 : 0;
            paired_ = stable_ >= 2 && cooldown_ == 0;
            return {paired_ ? Kind::Previous : Kind::Original, gap != 1, reset};
        }
        return {paired_ && frame - last_update_ == 1 && cooldown_ == 0 ? Kind::Intermediate
                                                                       : Kind::Skip,
                false, reset};
    }

  private:
    bool pending_ = false, have_present_ = false;
    std::uint64_t last_present_ = 0;
    bool initialized_ = false, eligible_ = false, have_update_ = false, paired_ = false;
    int target_ = 30;
    std::uint64_t epoch_ = 0, time_ = 0, frame_ = 0, last_update_ = 0;
    unsigned stable_ = 0, cooldown_ = 0;
};
} // namespace PokemonXFrames

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <aaudio/AAudio.h>
#include <atomic>
#include <string>
#include "audio_core/sink.h"
namespace AudioCore {
// UI-safe mute for backgrounding/pausing; input capture stays disabled.
void SetAndroidOutputMuted(bool muted);
// The owning emulation thread opens/closes the stream. Callbacks only fill PCM
// and update counters; they never log, stop, close, or allocate stream resources.
class AAudioSink final : public Sink {
public:
    explicit AAudioSink(std::string_view);
    ~AAudioSink() override;
    unsigned int GetNativeSampleRate() const override;
    void SetCallback(std::function<void(s16*, std::size_t)> cb) override;
    std::string Diagnostics() const;
private:
    static aaudio_data_callback_result_t Data(AAudioStream*, void*, void*, int32_t);
    static void Error(AAudioStream*, void*, aaudio_result_t);
    AAudioStream* stream = nullptr;
    std::function<void(s16*, std::size_t)> callback;
    std::atomic<uint64_t> frames{0}, audible_frames{0};
    std::atomic<int32_t> error{AAUDIO_OK};
};
}

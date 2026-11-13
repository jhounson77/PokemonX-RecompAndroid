// SPDX-License-Identifier: GPL-2.0-or-later
#include "audio_core/aaudio_sink.h"
#include <algorithm>
#include <stdexcept>
namespace AudioCore {
namespace { std::atomic<bool> output_muted{false}; }
void SetAndroidOutputMuted(bool muted) { output_muted.store(muted, std::memory_order_relaxed); }
AAudioSink::AAudioSink(std::string_view) {
    AAudioStreamBuilder* builder = nullptr;
    auto result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) throw std::runtime_error("AAudio builder: " + std::to_string(result));
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, 2);
    AAudioStreamBuilder_setSampleRate(builder, 48000);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDataCallback(builder, Data, this);
    AAudioStreamBuilder_setErrorCallback(builder, Error, this);
    result = AAudioStreamBuilder_openStream(builder, &stream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK) throw std::runtime_error("AAudio open: " + std::to_string(result));
    if (AAudioStream_getFormat(stream) != AAUDIO_FORMAT_PCM_I16 ||
        AAudioStream_getChannelCount(stream) != 2 || AAudioStream_getSampleRate(stream) <= 0) {
        AAudioStream_close(stream); stream = nullptr;
        throw std::runtime_error("AAudio incompatible PCM format");
    }
    // Three bursts absorb scheduler jitter without a large queued audio backlog.
    AAudioStream_setBufferSizeInFrames(stream, 3 * AAudioStream_getFramesPerBurst(stream));
}
AAudioSink::~AAudioSink() {
    if (stream) { AAudioStream_requestStop(stream); AAudioStream_close(stream); }
}
unsigned int AAudioSink::GetNativeSampleRate() const {
    return static_cast<unsigned>(AAudioStream_getSampleRate(stream));
}
void AAudioSink::SetCallback(std::function<void(s16*, std::size_t)> cb) {
    // Called once, before starting. DSP destroys this sink before its callback data.
    callback = std::move(cb);
    const auto result = AAudioStream_requestStart(stream);
    if (result != AAUDIO_OK) throw std::runtime_error("AAudio start: " + std::to_string(result));
}
aaudio_data_callback_result_t AAudioSink::Data(AAudioStream*, void* user, void* output, int32_t count) {
    auto& self = *static_cast<AAudioSink*>(user);
    if (count <= 0) return AAUDIO_CALLBACK_RESULT_CONTINUE;
    auto* pcm = static_cast<s16*>(output);
    if (self.callback) self.callback(pcm, static_cast<std::size_t>(count));
    else std::fill_n(pcm, static_cast<std::size_t>(count) * 2, s16{});
    if (output_muted.load(std::memory_order_relaxed))
        std::fill_n(pcm, static_cast<std::size_t>(count) * 2, s16{});
    self.frames.fetch_add(count, std::memory_order_relaxed);
    // Count frames with any signal; no PCM samples are written into logs.
    uint64_t nonzero = 0;
    for (int32_t i = 0; i < count; ++i) nonzero += pcm[i*2] != 0 || pcm[i*2+1] != 0;
    self.audible_frames.fetch_add(nonzero, std::memory_order_relaxed);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
void AAudioSink::Error(AAudioStream*, void* user, aaudio_result_t value) {
    static_cast<AAudioSink*>(user)->error.store(value, std::memory_order_relaxed);
}
std::string AAudioSink::Diagnostics() const {
    return "AAudio PCM16 stereo: rate=" + std::to_string(GetNativeSampleRate()) +
        ", frames=" + std::to_string(frames.load(std::memory_order_relaxed)) +
        ", signal_frames=" + std::to_string(audible_frames.load(std::memory_order_relaxed)) +
        ", xruns=" + std::to_string(AAudioStream_getXRunCount(stream)) +
        ", state=" + std::to_string(AAudioStream_getState(stream)) +
        ", error=" + std::to_string(error.load(std::memory_order_relaxed));
}
}

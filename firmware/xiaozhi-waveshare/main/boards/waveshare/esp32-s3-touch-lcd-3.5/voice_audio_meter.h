// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Read-only visualization tap. Audio tasks publish bounded 10 ms PCM envelopes;
// the UI reads a number without allocating, taking an audio lock or retaining PCM.
class VoiceAudioMeter {
public:
    void Publish(const int16_t* pcm, size_t count, int sample_rate, uint32_t now_ms) {
        if (!pcm || sample_rate < 100 || !count)
            return;
        const size_t window = sample_rate / 100;
        const size_t windows = std::min(kWindows, (count + window - 1) / window);
        for (size_t i = 0; i < windows; ++i) {
            const size_t n = std::min(window, count - i * window);
            int64_t sum = 0;
            uint64_t squares = 0;
            for (size_t j = 0; j < n; ++j) {
                int32_t v = pcm[i * window + j];
                sum += v;
                squares += static_cast<int64_t>(v) * v;
            }
            const double mean = static_cast<double>(sum) / n;
            const float rms =
                std::sqrt(std::max(0.0, static_cast<double>(squares) / n - mean * mean)) / 32768.0;
            // Perceptual compression keeps quiet speech visible without clipping ordinary speech.
            const float level = std::min(1.0f, 2.0f * std::sqrt(rms));
            levels_[i].store(static_cast<uint8_t>(level * 255), std::memory_order_relaxed);
        }
        stamp_.store((now_ms << 8) | windows, std::memory_order_release);
    }

    uint8_t Read(uint32_t now_ms) const {
        const uint32_t stamp = stamp_.load(std::memory_order_acquire);
        const uint32_t windows = stamp & 255;
        const uint32_t age = (now_ms - (stamp >> 8)) & 0xffffff;
        if (!windows || age >= windows * 10 + 20)
            return 0;
        return levels_[std::min(age / 10, windows - 1)].load(std::memory_order_relaxed);
    }

private:
    static constexpr size_t kWindows = 12;  // maximum supported 120 ms Opus frame
    std::array<std::atomic<uint8_t>, kWindows> levels_{};
    std::atomic<uint32_t> stamp_{0};
};

struct VoiceAudioMeters {
    VoiceAudioMeter microphone;
    VoiceAudioMeter playback;
};

inline VoiceAudioMeters& GetVoiceAudioMeters() {
    static VoiceAudioMeters meters;
    return meters;
}

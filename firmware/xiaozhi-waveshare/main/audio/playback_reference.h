// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>

// Fixed-capacity, timestamped PCM history. The codec owns storage and serializes
// the microphone/playback tasks; missing/stale samples are silence, never old echo.
class PlaybackReference {
public:
    static constexpr size_t kCapacity = 8192;
    explicit PlaybackReference(uint64_t* storage, int rate) : slots_(storage), rate_(rate) {
        std::fill_n(slots_, kCapacity, uint64_t{0});
    }
    void Publish(const int16_t* pcm, size_t count, int64_t start_us, bool audible = true) {
        start_us = std::max(start_us, queued_until_us_);
        queued_until_us_ = start_us + count * 1000000 / rate_;
        const uint64_t first = Position(start_us);
        for (size_t i = 0; i < count; ++i) {
            const uint64_t position = first + i;
            slots_[position % kCapacity] =
                ((position + 1) << 16) | static_cast<uint16_t>(audible ? pcm[i] : 0);
        }
    }
    void Read(int16_t* out, size_t count, size_t stride, int64_t start_us) const {
        const uint64_t first = Position(std::max<int64_t>(0, start_us));
        for (size_t i = 0; i < count; ++i) {
            const uint64_t position = first + i;
            const uint64_t slot = slots_[position % kCapacity];
            out[i * stride] = slot >> 16 == position + 1 ? static_cast<int16_t>(slot & 0xffff) : 0;
        }
    }

private:
    uint64_t Position(int64_t us) const { return static_cast<uint64_t>(us) * rate_ / 1000000; }
    uint64_t* slots_;
    int rate_;
    int64_t queued_until_us_ = 0;
};

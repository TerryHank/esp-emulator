#pragma once

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include "audio/playback_reference.h"
#include "codecs/es8311_audio_codec.h"
#include "config.h"
#include "voice_audio_meter.h"

#include <esp_io_expander.h>
#include <esp_log.h>
#include <cstdlib>
#include <memory>
#include <mutex>

class WaveshareAudioCodec : public Es8311AudioCodec {
    esp_io_expander_handle_t expander_;
    std::mutex pa_mutex_;
#if CONFIG_WAVESHARE_SPEECH_ENHANCEMENT
    std::mutex reference_mutex_;
    uint64_t* reference_storage_ = nullptr;
    std::unique_ptr<PlaybackReference> reference_;
    std::vector<int16_t> microphone_;
    int64_t capture_origin_us_ = 0;
    uint64_t captured_samples_ = 0;
#endif

public:
    WaveshareAudioCodec(i2c_master_bus_handle_t bus, esp_io_expander_handle_t expander)
        : Es8311AudioCodec(bus, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                           AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
                           AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN, AUDIO_CODEC_PA_PIN,
                           AUDIO_CODEC_ES8311_ADDR),
          expander_(expander) {
        output_volume_ = 100;
        input_gain_ = 18;  // Keep ADC headroom for speaker echo before AEC.
#if CONFIG_WAVESHARE_SPEECH_ENHANCEMENT
        reference_storage_ = static_cast<uint64_t*>(heap_caps_calloc(
            PlaybackReference::kCapacity, sizeof(uint64_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (reference_storage_) {
            reference_ =
                std::make_unique<PlaybackReference>(reference_storage_, input_sample_rate_);
            input_reference_ = true;
            input_channels_ = 2;  // Logical M/R. The physical ES8311 ADC remains mono.
            microphone_.reserve(1440);
        } else {
            ESP_LOGE("WaveshareAudio",
                     "Playback-reference allocation failed; retaining mono input");
        }
#endif
    }

    bool InputData(std::vector<int16_t>& data) override {
#if CONFIG_WAVESHARE_SPEECH_ENHANCEMENT
        if (reference_) {
            const size_t frames = data.size() / 2;
            microphone_.resize(frames);
            if (!Es8311AudioCodec::InputData(microphone_))
                return false;
            const int64_t end_us = esp_timer_get_time();
            // Keep the ADC sample clock continuous across task scheduling jitter.
            int64_t start_us =
                capture_origin_us_ + captured_samples_ * 1000000 / input_sample_rate_;
            const int64_t expected_end = start_us + frames * 1000000 / input_sample_rate_;
            if (!capture_origin_us_ || std::abs(end_us - expected_end) > 100000) {
                capture_origin_us_ = end_us - frames * 1000000 / input_sample_rate_;
                captured_samples_ = 0;
                start_us = capture_origin_us_;
            }
            captured_samples_ += frames;
            for (size_t i = 0; i < frames; ++i)
                data[2 * i] = microphone_[i];
            {
                std::lock_guard<std::mutex> lock(reference_mutex_);
                reference_->Read(data.data() + 1, frames, 2, start_us);
            }
            GetVoiceAudioMeters().microphone.Publish(microphone_.data(), frames, input_sample_rate_,
                                                     end_us / 1000);
            return true;
        }
#endif
        const bool ok = Es8311AudioCodec::InputData(data);
        if (ok)
            GetVoiceAudioMeters().microphone.Publish(data.data(), data.size(), input_sample_rate_,
                                                     esp_timer_get_time() / 1000);
        return ok;
    }

    void OutputData(std::vector<int16_t>& data) override {
        const int64_t submitted_us = esp_timer_get_time();
#if CONFIG_WAVESHARE_SPEECH_ENHANCEMENT
        if (reference_) {
            // Publish before the blocking hardware write. DMA may already play
            // the beginning of this frame while Write() is waiting for space.
            std::lock_guard<std::mutex> lock(reference_mutex_);
            reference_->Publish(data.data(), data.size(), submitted_us, output_volume_ > 0);
        }
#endif
        Es8311AudioCodec::OutputData(data);
        GetVoiceAudioMeters().playback.Publish(data.data(), data.size(), output_sample_rate_,
                                               esp_timer_get_time() / 1000);
    }

    void EnableOutput(bool enable) override {
        std::lock_guard<std::mutex> lock(pa_mutex_);
        // EXIO7 drives the active-high NS4150B speaker amplifier control.
        if (!enable) {
            ESP_ERROR_CHECK(esp_io_expander_set_level(expander_, IO_EXPANDER_PIN_NUM_7, 0));
        }
        Es8311AudioCodec::EnableOutput(enable);
        if (enable) {
            ESP_ERROR_CHECK(esp_io_expander_set_level(expander_, IO_EXPANDER_PIN_NUM_7, 1));
        }
        uint32_t level = 0;
        esp_err_t result = esp_io_expander_get_level(expander_, IO_EXPANDER_PIN_NUM_7, &level);
        ESP_LOGI("WaveshareAudio", "Speaker PA %s: EXIO7 level=%lu status=%d",
                 enable ? "on" : "off", (unsigned long)level, result);
    }
};

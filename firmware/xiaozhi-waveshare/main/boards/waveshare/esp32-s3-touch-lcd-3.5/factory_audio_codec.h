#pragma once

#include "audio_codec.h"
#include "config.h"

#include <driver/i2c_master.h>
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>
#include <esp_log.h>
#include <algorithm>
#include <array>
#include <cmath>

// Keep this board's known-working factory recording/playback setup together.
class FactoryAudioCodec : public AudioCodec {
    esp_codec_dev_handle_t output_dev_ = nullptr;
    esp_codec_dev_handle_t input_dev_ = nullptr;
    bool logged_pcm_ = false;

    int Read(int16_t* data, int samples) override {
        int result = esp_codec_dev_read(input_dev_, data, samples * sizeof(int16_t));
        if (result != ESP_CODEC_DEV_OK) {
            ESP_LOGE("FactoryAudio", "read failed: %d", result);
            return 0;
        }
        return samples;
    }

    int Write(const int16_t* data, int samples) override {
        int result =
            esp_codec_dev_write(output_dev_, const_cast<int16_t*>(data), samples * sizeof(int16_t));
        if (!logged_pcm_) {
            int peak = 0;
            for (int i = 0; i < samples; ++i) {
                peak = std::max(peak, std::abs(static_cast<int>(data[i])));
            }
            if (peak > 0) {
                ESP_LOGI("FactoryAudio", "PCM playback: samples=%d peak=%d result=%d", samples,
                         peak, result);
                logged_pcm_ = true;
            }
        }
        if (result != ESP_CODEC_DEV_OK) {
            ESP_LOGE("FactoryAudio", "write failed: %d", result);
            return 0;
        }
        return samples;
    }

public:
    explicit FactoryAudioCodec(i2c_master_bus_handle_t bus) {
        duplex_ = true;
        input_sample_rate_ = AUDIO_INPUT_SAMPLE_RATE;
        output_sample_rate_ = AUDIO_OUTPUT_SAMPLE_RATE;
        input_gain_ = 40;
        output_volume_ = 100;

        auto channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        ESP_ERROR_CHECK(i2s_new_channel(&channel, &tx_handle_, &rx_handle_));
        i2s_std_config_t i2s = {};
        i2s.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
        i2s.slot_cfg =
            I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
        i2s.gpio_cfg.mclk = AUDIO_I2S_GPIO_MCLK;
        i2s.gpio_cfg.bclk = AUDIO_I2S_GPIO_BCLK;
        i2s.gpio_cfg.ws = AUDIO_I2S_GPIO_WS;
        i2s.gpio_cfg.dout = AUDIO_I2S_GPIO_DOUT;
        i2s.gpio_cfg.din = AUDIO_I2S_GPIO_DIN;
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &i2s));
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &i2s));
        ESP_ERROR_CHECK(i2s_channel_enable(tx_handle_));
        ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));

        audio_codec_i2s_cfg_t data_config = {};
        data_config.rx_handle = rx_handle_;
        data_config.tx_handle = tx_handle_;
        auto data_if = audio_codec_new_i2s_data(&data_config);
        audio_codec_i2c_cfg_t control_config = {};
        control_config.addr = ES8311_CODEC_DEFAULT_ADDR;
        control_config.bus_handle = bus;
        auto control_if = audio_codec_new_i2c_ctrl(&control_config);
        auto gpio_if = audio_codec_new_gpio();
        es8311_codec_cfg_t codec_config = {};
        codec_config.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
        codec_config.ctrl_if = control_if;
        codec_config.gpio_if = gpio_if;
        codec_config.pa_pin = GPIO_NUM_NC;
        codec_config.use_mclk = true;
        codec_config.hw_gain.pa_voltage = 5.0;
        codec_config.hw_gain.codec_dac_voltage = 3.3;
        auto codec_if = es8311_codec_new(&codec_config);

        esp_codec_dev_cfg_t device = {};
        device.dev_type = ESP_CODEC_DEV_TYPE_OUT;
        device.codec_if = codec_if;
        device.data_if = data_if;
        output_dev_ = esp_codec_dev_new(&device);
        device.dev_type = ESP_CODEC_DEV_TYPE_IN;
        input_dev_ = esp_codec_dev_new(&device);
        assert(output_dev_ && input_dev_);
        ESP_ERROR_CHECK(esp_codec_set_disable_when_closed(output_dev_, false));
        ESP_ERROR_CHECK(esp_codec_set_disable_when_closed(input_dev_, false));

        esp_codec_dev_sample_info_t format = {};
        format.sample_rate = input_sample_rate_;
        format.channel = 1;
        format.bits_per_sample = 16;
        ESP_ERROR_CHECK(esp_codec_dev_open(output_dev_, &format));
        ESP_ERROR_CHECK(esp_codec_dev_open(input_dev_, &format));
        ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(input_dev_, input_gain_));
        ESP_LOGI("FactoryAudio", "Factory-compatible codec ready: rate=%d, independent IN/OUT",
                 input_sample_rate_);
    }

    void Start() override {
        AudioCodec::Start();
        SetOutputVolume(output_volume_);
        for (int reg : {0x00, 0x01, 0x02, 0x03, 0x09, 0x0A, 0x31, 0x32, 0x37, 0x44}) {
            int value = 0;
            int result = esp_codec_dev_read_reg(output_dev_, reg, &value);
            ESP_LOGI("FactoryAudio", "ES8311 register 0x%02x=0x%02x status=%d", reg, value, result);
        }
        std::array<int16_t, 480> tone;
        for (size_t i = 0; i < tone.size(); ++i) {
            tone[i] = static_cast<int16_t>(
                12000 * std::sin(2.0 * 3.141592653589793 * 1000 * i / output_sample_rate_));
        }
        ESP_LOGI("FactoryAudio",
                 "LOCAL_PCM_TEST_START: 1000Hz, 1 second, volume=%d, network not started",
                 output_volume_);
        for (int i = 0; i < 100; ++i) {
            Write(tone.data(), tone.size());
        }
        logged_pcm_ = false;
        ESP_LOGI("FactoryAudio", "LOCAL_PCM_TEST_END");
    }

    void SetOutputVolume(int volume) override {
        ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev_, volume));
        AudioCodec::SetOutputVolume(volume);
    }
};

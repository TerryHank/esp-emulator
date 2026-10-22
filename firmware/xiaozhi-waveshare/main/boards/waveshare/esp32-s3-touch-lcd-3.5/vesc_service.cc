#include "vesc_service.h"

#include "settings.h"

#include <driver/uart.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#error "The VESC build needs a USB console: GPIO43/44 are connected to the controller UART."
#endif

namespace {
constexpr uart_port_t kUart = UART_NUM_1;
constexpr const char* kTag = "VescService";
uint64_t NowMs() { return esp_timer_get_time() / 1000; }
}  // namespace

VescService& VescService::Instance() {
    static VescService instance;
    return instance;
}

void VescService::Start(std::function<void()> on_connected) {
    on_connected_ = std::move(on_connected);
    Settings settings("vesc_hud");
    vesc::Config config;
    config.baud = settings.GetInt("baud", config.baud);
    config.wheel_mm = settings.GetInt("wheel_mm", 0);
    config.pole_pairs = settings.GetInt("pole_pairs", 0);
    config.gear_milli = settings.GetInt("gear_milli", 0);
    if (!vesc::ValidConfig(config)) {
        config = {};
    }
    snapshot_.config = config;
    if (xTaskCreate([](void* context) { static_cast<VescService*>(context)->Run(); }, "vesc_uart",
                    4096, this, 2, nullptr) != pdPASS) {
        ESP_LOGE(kTag, "Could not create UART task");
    }
}

vesc::Snapshot VescService::GetSnapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = snapshot_;
    result.RefreshAge(NowMs());
    return result;
}

bool VescService::SaveConfig(const vesc::Config& config) {
    if (!vesc::ValidConfig(config)) {
        return false;
    }
    {
        Settings settings("vesc_hud", true);
        settings.SetInt("baud", config.baud);
        settings.SetInt("wheel_mm", config.wheel_mm);
        settings.SetInt("pole_pairs", config.pole_pairs);
        settings.SetInt("gear_milli", config.gear_milli);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.config = config;
    return true;
}

void VescService::Run() {
    auto config = GetSnapshot().config;
    uart_config_t uart_config = {};
    uart_config.baud_rate = config.baud;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    auto error = uart_param_config(kUart, &uart_config);
    if (error == ESP_OK) {
        error = uart_set_pin(kUart, kTxGpio, kRxGpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (error == ESP_OK) {
        error = uart_driver_install(kUart, 2048, 0, 0, nullptr, 0);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot_.uart_ok = error == ESP_OK;
    }
    if (error != ESP_OK) {
        ESP_LOGE(kTag, "UART init failed: %s", esp_err_to_name(error));
        vTaskDelete(nullptr);
        return;
    }
    ESP_LOGI(kTag, "Read-only UART1 TX=%d RX=%d baud=%d, poll=5Hz, stale=%llu ms", kTxGpio, kRxGpio,
             config.baud, vesc::kStaleAfterMs);
    vesc::Parser parser;
    uint8_t bytes[128];
    auto values_request = vesc::GetValuesRequest();
    constexpr uint8_t version_request[] = {2, 1, 0, 0, 0, 3};
    uint64_t next_poll = 0, next_version = 0, last_rx = 0;
    bool was_online = false;
    while (true) {
        auto state = GetSnapshot();
        if (state.config.baud != config.baud) {
            config.baud = state.config.baud;
            uart_set_baudrate(kUart, config.baud);
            uart_flush_input(kUart);
            parser.Reset();
            std::lock_guard<std::mutex> lock(mutex_);
            snapshot_.has_sample = false;
            snapshot_.firmware_major = snapshot_.firmware_minor = -1;
            next_poll = next_version = 0;
        }
        auto now = NowMs();
        if (now >= next_poll) {
            uart_write_bytes(kUart, values_request.data(), values_request.size());
            next_poll = now + 200;
        }
        if (now >= next_version && state.firmware_major < 0) {
            uart_write_bytes(kUart, version_request, sizeof(version_request));
            next_version = now + 5000;
        }
        auto count = uart_read_bytes(kUart, bytes, sizeof(bytes), pdMS_TO_TICKS(20));
        if (count > 0) {
            if (NowMs() - last_rx > 100) {
                parser.Reset();
            }
            last_rx = NowMs();
            for (int i = 0; i < count; ++i) {
                if (!parser.Push(bytes[i])) {
                    continue;
                }
                vesc::Values values;
                std::lock_guard<std::mutex> lock(mutex_);
                if (vesc::DecodeValues(parser.payload(), parser.length(), values)) {
                    snapshot_.values = values;
                    snapshot_.has_sample = true;
                    snapshot_.sample_ms = NowMs();
                    ++snapshot_.sequence;
                } else if (parser.length() >= 3 && parser.payload()[0] == 0) {
                    snapshot_.firmware_major = parser.payload()[1];
                    snapshot_.firmware_minor = parser.payload()[2];
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snapshot_.crc_errors = parser.errors();
        }
        state = GetSnapshot();
        if (state.online != was_online) {
            ESP_LOGI(kTag, "%s, sequence=%llu age=%llu ms", state.State(), state.sequence,
                     state.age_ms);
            was_online = state.online;
            if (state.online) {
                on_connected_();
            }
        }
    }
}

cJSON* VescService::GetStatusJson() { return vesc::StatusJson(GetSnapshot()); }

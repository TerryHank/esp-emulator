#pragma once

#include "vesc_protocol.h"

#include <cJSON.h>
#include <functional>
#include <mutex>

class VescService {
public:
    // J8 pin 25 = GPIO43/TX, pin 27 = GPIO44/RX, pin 29/30 = GND (Waveshare A01).
    // UART1 owns these pads. This build routes the console exclusively to USB Serial/JTAG.
    static constexpr int kTxGpio = 43, kRxGpio = 44;
    static VescService& Instance();
    void Start(std::function<void()> on_connected);
    vesc::Snapshot GetSnapshot();
    bool SaveConfig(const vesc::Config& config);
    cJSON* GetStatusJson();

private:
    void Run();
    std::mutex mutex_;
    vesc::Snapshot snapshot_;
    std::function<void()> on_connected_;
};

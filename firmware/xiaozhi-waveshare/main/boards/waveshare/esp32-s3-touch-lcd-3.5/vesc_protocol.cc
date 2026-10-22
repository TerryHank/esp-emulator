#include "vesc_protocol.h"

#include <cmath>
#include <cstring>

namespace vesc {

uint16_t Crc16(const uint8_t* bytes, size_t length) {
    uint16_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(bytes[i]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
        }
    }
    return crc;
}

std::array<uint8_t, 6> GetValuesRequest() {
    auto crc = Crc16(&kGetValues, 1);
    return {2, 1, kGetValues, static_cast<uint8_t>(crc >> 8), static_cast<uint8_t>(crc), 3};
}

void Parser::Drop(size_t count) {
    used_ -= count;
    std::memmove(buffer_.data(), buffer_.data() + count, used_);
}

bool Parser::Push(uint8_t byte) {
    if (consumed_) {
        Drop(consumed_);
        consumed_ = 0;
    }
    if (used_ == buffer_.size()) {
        Drop(1);
    }
    buffer_[used_++] = byte;
    while (used_ > 0) {
        size_t header = buffer_[0];
        if (header != 2 && header != 3) {
            Drop(1);
            continue;
        }
        if (used_ < header) {
            return false;
        }
        size_t length = header == 2 ? buffer_[1] : (buffer_[1] << 8) | buffer_[2];
        if (length == 0 || length > 512 || (header == 3 && length < 255)) {
            Drop(1);
            continue;
        }
        size_t total = header + length + 3;
        if (used_ < total) {
            return false;
        }
        auto crc = Crc16(buffer_.data() + header, length);
        if (buffer_[total - 1] != 3 || buffer_[total - 3] != (crc >> 8) ||
            buffer_[total - 2] != (crc & 0xff)) {
            ++errors_;
            Drop(1);
            continue;
        }
        header_ = header;
        payload_length_ = length;
        consumed_ = total;
        return true;
    }
    return false;
}

bool DecodeValues(const uint8_t* payload, size_t length, Values& values) {
    // VESC release_6_02 COMM_GET_VALUES: mandatory prefix through fault_code, 54 bytes.
    // Ignore newer appended fields; never interpret a selective-values reply as this layout.
    if (length < 54 || payload[0] != kGetValues) {
        return false;
    }
    size_t offset = 1;
    auto i16 = [&]() -> int32_t {
        uint32_t raw = (static_cast<uint32_t>(payload[offset]) << 8) | payload[offset + 1];
        offset += 2;
        return raw & 0x8000 ? static_cast<int32_t>(raw) - 65536 : raw;
    };
    auto i32 = [&]() -> int32_t {
        uint32_t raw = 0;
        for (int i = 0; i < 4; ++i) {
            raw = (raw << 8) | payload[offset++];
        }
        return raw <= INT32_MAX ? static_cast<int32_t>(raw)
                                : static_cast<int32_t>(static_cast<int64_t>(raw) - 0x100000000LL);
    };
    Values result;
    result.fet_c = i16() / 10.0;
    result.motor_c = i16() / 10.0;
    result.motor_a = i32() / 100.0;
    result.input_a = i32() / 100.0;
    i32();  // id
    i32();  // iq
    result.duty = i16() / 1000.0;
    result.erpm = i32();
    result.voltage = i16() / 10.0;
    result.amp_hours = i32() / 10000.0;
    result.amp_hours_charged = i32() / 10000.0;
    result.watt_hours = i32() / 10000.0;
    result.watt_hours_charged = i32() / 10000.0;
    result.tachometer = i32();
    result.tachometer_abs = i32();
    result.fault = payload[offset];
    values = result;
    return true;
}

bool Config::HasGeometry() const { return wheel_mm > 0 && pole_pairs > 0 && gear_milli > 0; }

bool ValidConfig(const Config& config) {
    // Zero geometry means unknown. These are input sanity bounds, not vehicle limits.
    return config.baud >= 9600 && config.baud <= 1000000 && config.wheel_mm >= 0 &&
           config.wheel_mm <= 3000 && config.pole_pairs >= 0 && config.pole_pairs <= 200 &&
           config.gear_milli >= 0 && config.gear_milli <= 1000000;
}

std::optional<double> Snapshot::SpeedKmh() const {
    if (!online || !config.HasGeometry()) {
        return std::nullopt;
    }
    return std::abs(static_cast<double>(values.erpm)) / config.pole_pairs / config.gear_milli *
           config.wheel_mm * 3.14159265358979323846 * 60.0 / 1000.0;
}

void Snapshot::RefreshAge(uint64_t now_ms) {
    age_ms = has_sample && now_ms >= sample_ms ? now_ms - sample_ms : 0;
    online = uart_ok && has_sample && now_ms >= sample_ms && age_ms <= kStaleAfterMs;
}

const char* Snapshot::State() const {
    if (!uart_ok) {
        return "uart_error";
    }
    return online ? "online" : (has_sample ? "stale" : "waiting");
}

const char* FaultName(uint8_t code) {
    static constexpr const char* names[] = {"NONE",
                                            "OVER_VOLTAGE",
                                            "UNDER_VOLTAGE",
                                            "DRV",
                                            "ABS_OVER_CURRENT",
                                            "OVER_TEMP_FET",
                                            "OVER_TEMP_MOTOR",
                                            "GATE_DRIVER_OVER_VOLTAGE",
                                            "GATE_DRIVER_UNDER_VOLTAGE",
                                            "MCU_UNDER_VOLTAGE",
                                            "BOOTING_FROM_WATCHDOG_RESET",
                                            "ENCODER_SPI",
                                            "ENCODER_SINCOS_BELOW_MIN_AMPLITUDE",
                                            "ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE",
                                            "FLASH_CORRUPTION",
                                            "HIGH_OFFSET_CURRENT_SENSOR_1",
                                            "HIGH_OFFSET_CURRENT_SENSOR_2",
                                            "HIGH_OFFSET_CURRENT_SENSOR_3",
                                            "UNBALANCED_CURRENTS",
                                            "BRK",
                                            "RESOLVER_LOT",
                                            "RESOLVER_DOS",
                                            "RESOLVER_LOS",
                                            "FLASH_CORRUPTION_APP_CFG",
                                            "FLASH_CORRUPTION_MC_CFG",
                                            "ENCODER_NO_MAGNET",
                                            "ENCODER_MAGNET_TOO_STRONG",
                                            "PHASE_FILTER",
                                            "ENCODER_FAULT",
                                            "LV_OUTPUT_FAULT"};
    return code < sizeof(names) / sizeof(names[0]) ? names[code] : "UNKNOWN";
}

}  // namespace vesc

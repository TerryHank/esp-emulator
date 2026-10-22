#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

struct cJSON;

namespace vesc {

constexpr uint8_t kGetValues = 4;
constexpr uint64_t kStaleAfterMs = 1500;

struct Values {
    double fet_c = 0, motor_c = 0;
    double motor_a = 0, input_a = 0, duty = 0, voltage = 0;
    int32_t erpm = 0, tachometer = 0, tachometer_abs = 0;
    double amp_hours = 0, amp_hours_charged = 0;
    double watt_hours = 0, watt_hours_charged = 0;
    uint8_t fault = 0;
};

struct Config {
    int baud = 115200;
    int wheel_mm = 0;
    int pole_pairs = 0;
    int gear_milli = 0;  // Motor revolutions per wheel revolution; 1000 means 1:1.
    bool HasGeometry() const;
};

struct Snapshot {
    Values values;
    Config config;
    bool has_sample = false, online = false, uart_ok = false;
    uint64_t sample_ms = 0, age_ms = 0, sequence = 0;
    uint32_t crc_errors = 0;
    int firmware_major = -1, firmware_minor = -1;
    void RefreshAge(uint64_t now_ms);
    std::optional<double> SpeedKmh() const;
    const char* State() const;
};

uint16_t Crc16(const uint8_t* bytes, size_t length);
std::array<uint8_t, 6> GetValuesRequest();
bool DecodeValues(const uint8_t* payload, size_t length, Values& values);
const char* FaultName(uint8_t code);
bool ValidConfig(const Config& config);
cJSON* StatusJson(const Snapshot& snapshot);  // Caller owns the returned cJSON object.

// Bounded streaming parser. The returned payload remains valid until the next Push().
class Parser {
public:
    bool Push(uint8_t byte);
    void Reset() { used_ = consumed_ = 0; }
    const uint8_t* payload() const { return buffer_.data() + header_; }
    size_t length() const { return payload_length_; }
    uint32_t errors() const { return errors_; }

private:
    void Drop(size_t count);
    std::array<uint8_t, 518> buffer_{};
    size_t used_ = 0, header_ = 0, payload_length_ = 0, consumed_ = 0;
    uint32_t errors_ = 0;
};

}  // namespace vesc

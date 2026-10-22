#include "vesc_protocol.h"
#include <cJSON.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

using namespace vesc;

static void Near(double actual, double expected) { assert(std::abs(actual - expected) < 1e-6); }

int main() {
    // Generated independently with Python struct.pack('>BhhiiiihihiiiiiiB', ...) /
    // binascii.crc_hqx(..., 0), using VESC release_6_02 field order and scales.
    const uint8_t packet[] = {
        0x02, 0x36, 0x04, 0x01, 0xa9, 0xff, 0xc9, 0x00, 0x00, 0x04, 0xd2, 0xff, 0xff, 0xfd, 0xca,
        0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x01, 0xc8, 0xfe, 0xc0, 0xff, 0xff, 0xf7, 0x04, 0x02,
        0xf1, 0x00, 0x00, 0x30, 0x39, 0x00, 0x00, 0x09, 0x29, 0x00, 0x12, 0xd6, 0x87, 0x00, 0x03,
        0x94, 0x47, 0xff, 0xff, 0xfc, 0x18, 0x00, 0x00, 0x07, 0xd0, 0x06, 0x6a, 0x87, 0x03};
    // Verify the known, complete GET_VALUES request separately.
    const uint8_t expected_request[] = {2, 1, 4, 0x40, 0x84, 3};
    auto request = GetValuesRequest();
    assert(std::memcmp(request.data(), expected_request, sizeof(expected_request)) == 0);
    const uint8_t check[] = "123456789";
    assert(Crc16(check, 9) == 0x31c3);

    Values values;
    assert(DecodeValues(packet + 2, 54, values));
    Near(values.fet_c, 42.5);
    Near(values.motor_c, -5.5);
    Near(values.motor_a, 12.34);
    Near(values.input_a, -5.66);
    Near(values.duty, -0.32);
    assert(values.erpm == -2300);
    Near(values.voltage, 75.3);
    Near(values.amp_hours, 1.2345);
    Near(values.amp_hours_charged, 0.2345);
    Near(values.watt_hours, 123.4567);
    Near(values.watt_hours_charged, 23.4567);
    assert(values.tachometer == -1000 && values.tachometer_abs == 2000 && values.fault == 6);
    for (size_t length = 0; length < 54; ++length) {
        assert(!DecodeValues(packet + 2, length, values));
    }
    auto payload = std::vector<uint8_t>(packet + 2, packet + 56);
    payload.push_back(0xaa);  // Extended response prefix remains compatible.
    assert(DecodeValues(payload.data(), payload.size(), values));
    payload[0] = 50;
    assert(!DecodeValues(payload.data(), payload.size(), values));
    payload[0] = 4;

    auto frame = [](const std::vector<uint8_t>& data) {
        std::vector<uint8_t> result;
        if (data.size() <= 255) {
            result = {2, static_cast<uint8_t>(data.size())};
        } else {
            result = {3, static_cast<uint8_t>(data.size() >> 8), static_cast<uint8_t>(data.size())};
        }
        result.insert(result.end(), data.begin(), data.end());
        auto crc = Crc16(data.data(), data.size());
        result.push_back(crc >> 8);
        result.push_back(crc & 255);
        result.push_back(3);
        return result;
    };
    Parser parser;
    int golden_received = 0;
    for (auto byte : packet) {
        golden_received += parser.Push(byte);
    }
    assert(golden_received == 1);
    for (size_t length : {size_t(1), size_t(54), size_t(255), size_t(256), size_t(512)}) {
        auto data = std::vector<uint8_t>(length, 0x55);
        auto bytes = frame(data);
        int received = 0;
        for (auto byte : bytes) {
            if (parser.Push(byte)) {
                ++received;
                assert(parser.length() == data.size());
                assert(std::memcmp(parser.payload(), data.data(), data.size()) == 0);
            }
        }
        assert(received == 1);
    }
    parser.Reset();  // Must clear the previous frame's deferred-consume position too.
    int received = 0;
    auto bytes = frame(std::vector<uint8_t>(54, 0x55));
    auto broken = bytes;
    broken[broken.size() - 2] ^= 1;
    for (auto byte : broken) {
        assert(!parser.Push(byte));
    }
    for (auto byte : bytes) {
        received += parser.Push(byte);
    }
    assert(received == 1 && parser.errors() >= 1);
    parser.Reset();
    const uint8_t invalid[] = {0, 255, 2, 0, 3, 0xff, 0xff};
    for (auto byte : invalid) {
        assert(!parser.Push(byte));
    }
    received = 0;
    for (auto byte : bytes) {
        received += parser.Push(byte);
    }
    assert(received == 1);
    // Exercise arbitrary serial noise and timeout resets under ASan/UBSan.
    std::mt19937 random(6302);
    for (int i = 0; i < 100000; ++i) {
        parser.Push(random() & 255);
        if (i % 53 == 0) {
            parser.Reset();
        }
    }

    Snapshot state;
    assert(!state.SpeedKmh());
    state.uart_ok = true;
    state.RefreshAge(100);
    assert(!state.online && std::strcmp(state.State(), "waiting") == 0);
    state.has_sample = true;
    state.sample_ms = 100;
    state.values = values;
    state.config = {115200, 500, 23, 1000};
    state.RefreshAge(1600);
    assert(state.online);
    Near(*state.SpeedKmh(), 9.42477796076938);
    state.config.gear_milli = 2000;
    Near(*state.SpeedKmh(), 4.71238898038469);
    state.config.pole_pairs = 0;
    assert(!state.SpeedKmh());
    state.config.pole_pairs = 23;
    state.RefreshAge(1601);
    assert(!state.online && !state.SpeedKmh() && std::strcmp(state.State(), "stale") == 0);
    state.RefreshAge(99);
    assert(!state.online);
    state.uart_ok = false;
    assert(std::strcmp(state.State(), "uart_error") == 0);
    assert(ValidConfig(Config{}));
    assert(!ValidConfig({0, 500, 23, 1000}));
    assert(!ValidConfig({115200, -1, 23, 1000}));
    assert(std::strcmp(FaultName(6), "OVER_TEMP_MOTOR") == 0);
    assert(std::strcmp(FaultName(255), "UNKNOWN") == 0);
    // MCP must report absent/stale metrics as null, including fault_code (never false "no fault").
    state.uart_ok = true;
    state.config = {115200, 500, 23, 1000};
    state.RefreshAge(101);
    auto json = StatusJson(state);
    assert(cJSON_IsTrue(cJSON_GetObjectItem(json, "online")));
    Near(cJSON_GetObjectItem(json, "speed_kmh")->valuedouble, 9.42477796076938);
    Near(cJSON_GetObjectItem(json, "power_w")->valuedouble, 75.3 * -5.66);
    assert(std::strcmp(cJSON_GetObjectItem(json, "fault_name")->valuestring, "OVER_TEMP_MOTOR") ==
           0);
    cJSON_Delete(json);
    state.config.pole_pairs = 0;
    json = StatusJson(state);
    assert(cJSON_IsNull(cJSON_GetObjectItem(json, "speed_kmh")));
    assert(cJSON_IsFalse(cJSON_GetObjectItem(json, "speed_configured")));
    cJSON_Delete(json);
    state.RefreshAge(1601);
    json = StatusJson(state);
    for (const char* name : {"speed_kmh", "voltage_v", "battery_current_a", "power_w", "fault_code",
                             "fault_name", "erpm", "motor_temperature_c"}) {
        assert(cJSON_IsNull(cJSON_GetObjectItem(json, name)));
    }
    assert(cJSON_GetObjectItem(json, "sample_age_ms")->valuedouble == 1501);
    cJSON_Delete(json);
    state.has_sample = false;
    state.RefreshAge(2000);
    json = StatusJson(state);
    assert(cJSON_IsNull(cJSON_GetObjectItem(json, "sample_uptime_ms")));
    assert(cJSON_IsNull(cJSON_GetObjectItem(json, "sample_age_ms")));
    cJSON_Delete(json);
    std::puts(
        "PASS: VESC 6.02 decode, stream/CRC/noise, freshness, geometry and MCP JSON null "
        "semantics");
}

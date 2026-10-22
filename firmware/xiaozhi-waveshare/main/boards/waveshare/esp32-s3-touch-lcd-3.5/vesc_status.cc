#include "vesc_service.h"

cJSON* vesc::StatusJson(const Snapshot& state) {
    auto json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "status", state.State());
    cJSON_AddBoolToObject(json, "online", state.online);
    cJSON_AddStringToObject(json, "transport", "uart");
    cJSON_AddNumberToObject(json, "tx_gpio", VescService::kTxGpio);
    cJSON_AddNumberToObject(json, "rx_gpio", VescService::kRxGpio);
    cJSON_AddNumberToObject(json, "baud", state.config.baud);
    cJSON_AddNumberToObject(json, "sequence", state.sequence);
    cJSON_AddNumberToObject(json, "stale_after_ms", vesc::kStaleAfterMs);
    cJSON_AddNumberToObject(json, "crc_errors", state.crc_errors);
    auto number = [json](const char* name, bool valid, double value) {
        if (valid) {
            cJSON_AddNumberToObject(json, name, value);
        } else {
            cJSON_AddNullToObject(json, name);
        }
    };
    number("sample_uptime_ms", state.has_sample, state.sample_ms);
    number("sample_age_ms", state.has_sample, state.age_ms);
    number("firmware_major", state.firmware_major >= 0, state.firmware_major);
    number("firmware_minor", state.firmware_minor >= 0, state.firmware_minor);
    number("wheel_diameter_mm", state.config.wheel_mm > 0, state.config.wheel_mm);
    number("motor_pole_pairs", state.config.pole_pairs > 0, state.config.pole_pairs);
    number("motor_to_wheel_ratio", state.config.gear_milli > 0, state.config.gear_milli / 1000.0);
    cJSON_AddBoolToObject(json, "speed_configured", state.config.HasGeometry());
    auto speed = state.SpeedKmh();
    number("speed_kmh", speed.has_value(), speed.value_or(0));
    number("voltage_v", state.online, state.values.voltage);
    number("battery_current_a", state.online, state.values.input_a);
    number("motor_current_a", state.online, state.values.motor_a);
    number("power_w", state.online, state.values.voltage * state.values.input_a);
    number("motor_temperature_c", state.online, state.values.motor_c);
    number("controller_temperature_c", state.online, state.values.fet_c);
    number("erpm", state.online, state.values.erpm);
    number("duty_ratio", state.online, state.values.duty);
    number("amp_hours", state.online, state.values.amp_hours);
    number("amp_hours_charged", state.online, state.values.amp_hours_charged);
    number("watt_hours", state.online, state.values.watt_hours);
    number("watt_hours_charged", state.online, state.values.watt_hours_charged);
    number("tachometer", state.online, state.values.tachometer);
    number("tachometer_abs", state.online, state.values.tachometer_abs);
    number("fault_code", state.online, state.values.fault);
    if (state.online) {
        cJSON_AddStringToObject(json, "fault_name", vesc::FaultName(state.values.fault));
    } else {
        cJSON_AddNullToObject(json, "fault_name");
    }
    return json;
}

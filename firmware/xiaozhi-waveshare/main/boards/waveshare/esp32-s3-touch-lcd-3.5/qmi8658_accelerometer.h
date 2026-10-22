#pragma once

#include <driver/i2c_master.h>
#include <cstdint>

class Qmi8658Accelerometer {
public:
    bool Initialize(i2c_master_bus_handle_t bus) {
        i2c_device_config_t config = {};
        config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        config.device_address = 0x6b;
        config.scl_speed_hz = 400000;
        if (i2c_master_bus_add_device(bus, &config, &device_) != ESP_OK) {
            return false;
        }
        uint8_t id = 0;
        if (!Read(0x00, &id, 1) || id != 0x05) {
            return false;
        }
        // Disable sensors, enable little-endian address increment, then 2g/125Hz + LPF.
        return Write(0x08, 0x00) && Write(0x02, 0x40) && Write(0x03, 0x06) && Write(0x06, 0x01) &&
               Write(0x08, 0x01);
    }

    bool ReadAcceleration(int16_t& x, int16_t& y, int16_t& z) {
        uint8_t data[6];
        if (!Read(0x35, data, sizeof(data))) {
            return false;
        }
        x = static_cast<int16_t>(data[0] | (data[1] << 8));
        y = static_cast<int16_t>(data[2] | (data[3] << 8));
        z = static_cast<int16_t>(data[4] | (data[5] << 8));
        return true;
    }

private:
    i2c_master_dev_handle_t device_ = nullptr;

    bool Read(uint8_t reg, uint8_t* data, size_t length) {
        return i2c_master_transmit_receive(device_, &reg, 1, data, length, 20) == ESP_OK;
    }
    bool Write(uint8_t reg, uint8_t value) {
        uint8_t data[] = {reg, value};
        return i2c_master_transmit(device_, data, sizeof(data), 20) == ESP_OK;
    }
};

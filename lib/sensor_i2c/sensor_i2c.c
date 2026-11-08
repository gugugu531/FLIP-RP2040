/* SPDX-License-Identifier: MIT */
#include "sensor_i2c.h"
enum { TIMEOUT_US = 2000 };
bool sensor_i2c_read(i2c_inst_t *bus, uint8_t address, uint8_t reg, uint8_t *out, size_t len,
                     uint32_t *errors) {
    if (i2c_write_timeout_us(bus, address, &reg, 1, true, TIMEOUT_US) != 1 ||
        i2c_read_timeout_us(bus, address, out, len, false, TIMEOUT_US) != (int)len) {
        (*errors)++;
        return false;
    }
    return true;
}
bool sensor_i2c_write(i2c_inst_t *bus, uint8_t address, uint8_t reg, uint8_t value,
                      uint32_t *errors) {
    uint8_t data[2] = {reg, value};
    if (i2c_write_timeout_us(bus, address, data, 2, false, TIMEOUT_US) != 2) {
        (*errors)++;
        return false;
    }
    return true;
}

/* SPDX-License-Identifier: MIT */
#include "qmi8658.h"
#include "sensor_i2c.h"
#include "pico/time.h"

enum {
    REG_WHO_AM_I = 0x00,
    REG_CTRL1 = 0x02,
    REG_CTRL2 = 0x03,
    REG_CTRL7 = 0x08,
    REG_TIMESTAMP = 0x30,
    REG_ACCEL = 0x35,
    REG_RESET = 0x60,
    WHO_AM_I = 0x05,
};

static bool read_reg(Qmi8658 *imu, uint8_t reg, uint8_t *out, size_t len) {
    return sensor_i2c_read(imu->bus, imu->address, reg, out, len, &imu->errors);
}
static bool write_reg(Qmi8658 *imu, uint8_t reg, uint8_t value) {
    return sensor_i2c_write(imu->bus, imu->address, reg, value, &imu->errors);
}

bool qmi8658_init(Qmi8658 *imu, i2c_inst_t *bus) {
    *imu = (Qmi8658){.bus = bus};
    bool found = false;
    for (uint8_t address = 0x6a; address <= 0x6b; address++) {
        uint8_t id = 0;
        imu->address = address;
        if (read_reg(imu, REG_WHO_AM_I, &id, 1) && id == WHO_AM_I) {
            found = true;
            break;
        }
    }
    if (found) {
        /* Preserve the established reset/configuration sequence. */
        write_reg(imu, REG_RESET, 0xb0);
        sleep_ms(30);
        found = write_reg(imu, REG_CTRL1, 0x40) && /* Auto-increment, little-endian. */
                write_reg(imu, REG_CTRL2, 0x25) && /* +/-8g, 250 Hz. */
                write_reg(imu, REG_CTRL7, 0x01);   /* Accelerometer only. */
    }
    if (!found)
        imu->address = 0;
    imu->errors = 0; /* Address-probe NACKs and setup errors are not runtime errors. */
    return found;
}

bool qmi8658_read_accel(Qmi8658 *imu, Qmi8658Accel *sample) {
    uint8_t data[12];
    /* Keep the complete output burst used by the validated firmware, even
     * though only its first six bytes (acceleration) are consumed. */
    if (!imu->address || !read_reg(imu, REG_ACCEL, data, sizeof(data)))
        return false;
    *sample = (Qmi8658Accel){
        .x = (int16_t)((data[1] << 8) | data[0]),
        .y = (int16_t)((data[3] << 8) | data[2]),
        .z = (int16_t)((data[5] << 8) | data[4]),
    };
    return true;
}

bool qmi8658_read_diagnostics(Qmi8658 *imu, Qmi8658Diagnostics *diagnostics) {
    Qmi8658Diagnostics result;
    if (!imu->address || !read_reg(imu, REG_WHO_AM_I, result.registers, sizeof(result.registers)) ||
        !read_reg(imu, REG_TIMESTAMP, result.timestamp, sizeof(result.timestamp)))
        return false;
    *diagnostics = result;
    return true;
}

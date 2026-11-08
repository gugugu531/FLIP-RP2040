/* SPDX-License-Identifier: MIT */
#ifndef FLIP_RP2040_QMI8658_H
#define FLIP_RP2040_QMI8658_H

#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

/* Fixed configuration: +/-8g, 250 Hz, accelerometer only. */
#define QMI8658_ACCEL_LSB_PER_G 4096

typedef struct {
    i2c_inst_t *bus;
    uint32_t errors; /* Failed runtime transactions; startup probes excluded. */
    uint8_t address; /* Zero when unavailable. Fields are owned by the driver. */
} Qmi8658;

typedef struct {
    int16_t x, y, z; /* Sensor axes, before screen mapping or filtering. */
} Qmi8658Accel;

typedef struct {
    uint8_t registers[9]; /* WHO_AM_I through CTRL7. */
    uint8_t timestamp[3]; /* Little-endian sensor timestamp, register 0x30. */
} Qmi8658Diagnostics;

/* The caller initializes I2C/pins first. All calls must use the same core as
 * other users of the bus; this driver does not synchronize shared I2C access. */
bool qmi8658_init(Qmi8658 *imu, i2c_inst_t *bus);
/* Outputs remain unchanged on failure. An unavailable device causes no I/O. */
bool qmi8658_read_accel(Qmi8658 *imu, Qmi8658Accel *sample);
bool qmi8658_read_diagnostics(Qmi8658 *imu, Qmi8658Diagnostics *diagnostics);

#endif

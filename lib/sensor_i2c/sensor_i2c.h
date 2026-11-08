/* SPDX-License-Identifier: MIT */
#ifndef FLIP_RP2040_SENSOR_I2C_H
#define FLIP_RP2040_SENSOR_I2C_H
#include "hardware/i2c.h"
/* Synchronous core-0 register transactions: 2 ms per SDK operation, exact
 * transfer length required, one error counted per failed register transaction.
 * Caller serializes access to the bus and supplies a valid error counter. */
bool sensor_i2c_read(i2c_inst_t *bus, uint8_t address, uint8_t reg, uint8_t *out, size_t len,
                     uint32_t *errors);
bool sensor_i2c_write(i2c_inst_t *bus, uint8_t address, uint8_t reg, uint8_t value,
                      uint32_t *errors);
#endif

/* SPDX-License-Identifier: MIT */
#ifndef FLIP_RP2040_CST816_H
#define FLIP_RP2040_CST816_H
#include "hardware/i2c.h"
typedef struct {
    i2c_inst_t *bus;
    uint32_t errors;
    bool available;
} Cst816;
typedef struct {
    uint16_t x, y; /* Raw panel pixels, not simulation grid coordinates. */
    bool touching;
} Cst816Sample;
/* I2C must already be initialized. Caller owns bus serialization. */
bool cst816_init(Cst816 *touch, i2c_inst_t *bus, unsigned reset_pin, unsigned interrupt_pin);
/* Failure leaves sample unchanged; unavailable devices cause no I/O. */
bool cst816_read(Cst816 *touch, Cst816Sample *sample);
#endif

/* SPDX-License-Identifier: MIT */
#include "cst816.h"
#include "sensor_i2c.h"
#include "hardware/gpio.h"
#include "pico/time.h"
enum {
    ADDRESS = 0x15,
    REG_POINTS = 0x02,
    REG_ID = 0xa7,
    REG_AUTOSLEEP = 0xfe,
    REG_IRQ_MODE = 0xfa
};
bool cst816_init(Cst816 *touch, i2c_inst_t *bus, unsigned reset_pin, unsigned interrupt_pin) {
    *touch = (Cst816){.bus = bus};
    gpio_init(reset_pin);
    gpio_set_dir(reset_pin, GPIO_OUT);
    gpio_put(reset_pin, 0);
    sleep_ms(10);
    gpio_put(reset_pin, 1);
    sleep_ms(100);
    gpio_init(interrupt_pin);
    gpio_set_dir(interrupt_pin, GPIO_IN);
    gpio_pull_up(interrupt_pin);
    uint8_t id = 0;
    touch->available = sensor_i2c_read(bus, ADDRESS, REG_ID, &id, 1, &touch->errors) &&
                       (id == 0xb4 || id == 0xb5 || id == 0xb6);
    if (touch->available)
        touch->available = sensor_i2c_write(bus, ADDRESS, REG_AUTOSLEEP, 1, &touch->errors) &&
                           sensor_i2c_write(bus, ADDRESS, REG_IRQ_MODE, 0x41, &touch->errors);
    touch->errors = 0;
    return touch->available;
}
bool cst816_read(Cst816 *touch, Cst816Sample *sample) {
    uint8_t data[5];
    if (!touch->available ||
        !sensor_i2c_read(touch->bus, ADDRESS, REG_POINTS, data, sizeof(data), &touch->errors))
        return false;
    *sample = (Cst816Sample){
        .x = (uint16_t)(((data[1] & 15) << 8) | data[2]),
        .y = (uint16_t)(((data[3] & 15) << 8) | data[4]),
        .touching = (data[0] & 15) > 0,
    };
    return true;
}

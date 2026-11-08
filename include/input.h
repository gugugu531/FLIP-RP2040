#ifndef FLIP_RP2040_INPUT_H
#define FLIP_RP2040_INPUT_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    int x_sign, y_sign;
    bool swap;
} InputMapping;
typedef struct {
    InputMapping mapping; /* May be changed by the application between polls. */
    int gx_mg, gy_mg, raw_x, raw_y, raw_z;
    uint16_t touch_x, touch_y; /* Panel pixels; main maps to the selected grid. */
    uint8_t imu_address;
    bool touching, touch_available;
    uint32_t i2c_errors;
} InputState;
typedef struct {
    uint8_t registers[9], timestamp[3];
} InputDiagnostics;
/* Single board input service, exclusively on core 0 after DEV_Module_Init. */
void input_init(InputState *state);
void input_poll(InputState *state);
bool input_read_diagnostics(InputState *state, InputDiagnostics *diagnostics);
#endif

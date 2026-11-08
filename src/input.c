#include "input.h"
#include "DEV_Config.h"
#include "qmi8658.h"
#include "cst816.h"
#include <string.h>
#ifndef PENDANT_IMU_SWAP
#define PENDANT_IMU_SWAP 1
#endif
#ifndef PENDANT_IMU_X_SIGN
#define PENDANT_IMU_X_SIGN 1
#endif
#ifndef PENDANT_IMU_Y_SIGN
#define PENDANT_IMU_Y_SIGN (-1)
#endif
static Qmi8658 imu;
static Cst816 touch;
void input_init(InputState *state) {
    *state = (InputState){.gy_mg = 1000,
                          .mapping = {PENDANT_IMU_X_SIGN, PENDANT_IMU_Y_SIGN, PENDANT_IMU_SWAP}};
    qmi8658_init(&imu, SENSOR_I2C_PORT);
    cst816_init(&touch, SENSOR_I2C_PORT, Touch_RST_PIN, Touch_INT_PIN);
    state->imu_address = imu.address;
    state->touch_available = touch.available;
}
void input_poll(InputState *state) {
    Qmi8658Accel acceleration;
    if (qmi8658_read_accel(&imu, &acceleration)) {
        state->raw_x = acceleration.x;
        state->raw_y = acceleration.y;
        state->raw_z = acceleration.z;
        int x = state->mapping.swap ? acceleration.y : acceleration.x;
        int y = state->mapping.swap ? acceleration.x : acceleration.y;
        state->gx_mg +=
            (state->mapping.x_sign * x * 1000 / QMI8658_ACCEL_LSB_PER_G - state->gx_mg) / 4;
        state->gy_mg +=
            (state->mapping.y_sign * y * 1000 / QMI8658_ACCEL_LSB_PER_G - state->gy_mg) / 4;
    }
    Cst816Sample sample;
    if (cst816_read(&touch, &sample)) {
        state->touching = sample.touching;
        state->touch_x = sample.x;
        state->touch_y = sample.y;
    } else {
        state->touching = false;
    }
    state->i2c_errors = imu.errors + touch.errors;
}
bool input_read_diagnostics(InputState *state, InputDiagnostics *diagnostics) {
    Qmi8658Diagnostics result;
    bool ok = qmi8658_read_diagnostics(&imu, &result);
    state->i2c_errors = imu.errors + touch.errors;
    if (ok) {
        memcpy(diagnostics->registers, result.registers, sizeof(result.registers));
        memcpy(diagnostics->timestamp, result.timestamp, sizeof(result.timestamp));
    }
    return ok;
}

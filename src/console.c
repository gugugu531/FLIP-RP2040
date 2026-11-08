#include "console.h"
#include "benchmark.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include <stdio.h>
void console_announce(void) {
    puts("flip_rp2040 ready. 1/2/3 profiles, r reset, b benchmark, +/- iterations, u BOOTSEL, x/y "
         "axis sign, s swap");
}
bool console_poll(Fluid *fluid, InputState *input) {
    int cmd = getchar_timeout_us(0);
    if (cmd == 'u')
        reset_usb_boot(0, 0);
    if (cmd == 'p') {
        printf("STAGES integrate=%lu separate=%lu p2g=%lu project=%lu g2p=%lu\n",
               (unsigned long)fluid->stage_us[0], (unsigned long)fluid->stage_us[1],
               (unsigned long)fluid->stage_us[2], (unsigned long)fluid->stage_us[3],
               (unsigned long)fluid->stage_us[4]);
    }
    if (cmd == 'd') {
        InputDiagnostics diagnostics;
        if (input_read_diagnostics(input, &diagnostics)) {
            printf("IMU regs:");
            for (int j = 0; j < 9; j++)
                printf(" %02x", diagnostics.registers[j]);
            printf(" timestamp=%02x%02x%02x\n", diagnostics.timestamp[2], diagnostics.timestamp[1],
                   diagnostics.timestamp[0]);
        } else
            puts("IMU unavailable or diagnostic read failed");
    }
    if (cmd == 'b') {
        benchmark_run(fluid);
        return true;
    }
    if (cmd >= '1' && cmd <= '3')
        fluid_init(fluid, 32 + 8 * (cmd - '1'), 8 + 4 * (cmd - '1'));
    if (cmd == 'r')
        fluid_init(fluid, fluid->n, fluid->iterations);
    if (cmd == '+' && fluid->iterations < 40)
        fluid->iterations += 2;
    if (cmd == '-' && fluid->iterations > 2)
        fluid->iterations -= 2;
    if (cmd == 'x')
        input->mapping.x_sign = -input->mapping.x_sign;
    if (cmd == 'y')
        input->mapping.y_sign = -input->mapping.y_sign;
    if (cmd == 's')
        input->mapping.swap = !input->mapping.swap;
    return false;
}

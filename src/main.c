#include "fluid.h"
#include "render.h"
#include "input.h"
#include "display.h"
#include "console.h"
#include "telemetry.h"
#include "DEV_Config.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#ifndef PENDANT_SYS_KHZ
#define PENDANT_SYS_KHZ 133000
#endif
/* Core 0 owns simulation and input; display owns its snapshots and core 1. */
static Fluid fluid;
int main(void) {
    set_sys_clock_khz(PENDANT_SYS_KHZ, true);
    clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, clock_get_hz(clk_sys),
                    clock_get_hz(clk_sys));
    DEV_Module_Init();
    display_init();
    InputState input;
    input_init(&input);
    fluid_init(&fluid, 32, 8);
    display_start();
    watchdog_enable(8000, true);
    uint64_t deadline = time_us_64();
    Telemetry stats;
    telemetry_reset(&stats, deadline);
    console_announce();
    for (;;) {
        watchdog_update();
        if (console_poll(&fluid, &input)) {
            deadline = time_us_64();
            telemetry_reset(&stats, deadline);
        }
        uint64_t now = time_us_64();
        if (now < deadline) {
            sleep_us((uint32_t)(deadline - now));
            continue;
        }
        deadline += 1000000 / FLUID_HZ;
        input_poll(&input);
        uint32_t start = time_us_32();
        fluid_step(&fluid, input.gx_mg, input.gy_mg, input.touch_x * fluid.n / RENDER_WIDTH,
                   input.touch_y * fluid.n / RENDER_HEIGHT, input.touching);
        telemetry_record_step(&stats, time_us_32() - start);
        FluidFrame *frame = display_acquire_frame();
        if (frame) {
            fluid_snapshot(&fluid, frame);
            display_submit_frame();
        }
        if (time_us_64() > deadline + 1000000 / FLUID_HZ) {
            /* Keep fixed dt; skip overdue wall-clock ticks rather than enlarge dt. */
            stats.late++;
            deadline = time_us_64();
        }
        DisplayStats display;
        while (display_take_stats(&display))
            telemetry_add_display(&stats, &display);
        telemetry_report(&stats, time_us_64(), &fluid, &input, clock_get_hz(clk_sys),
                         display_spi_hz());
    }
}

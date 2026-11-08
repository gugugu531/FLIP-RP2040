#include "benchmark.h"
#include "display.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include <stdio.h>
void benchmark_run(Fluid *fluid) {
    puts("BENCH_BEGIN clock_hz,spi_hz,grid,particles,iters,mean_us,max_us,clipped");
    for (int n = 32; n <= 48; n += 8)
        for (int it = 8; it <= 20; it += 6) {
            fluid_init(fluid, n, it);
            uint64_t sum = 0;
            uint32_t peak = 0;
            for (int t = 0; t < 150; t++) {
                watchdog_update();
                uint32_t a = time_us_32();
                int phase = (t / 25) % 4;
                int gx = phase == 1   ? 1000
                         : phase == 3 ? -1000
                                      : 0,
                    gy = phase == 0   ? 1000
                         : phase == 2 ? -1000
                                      : 0;
                fluid_step(fluid, gx, gy, 0, 0, false);
                uint32_t us = time_us_32() - a;
                if (t >= 30) {
                    sum += us;
                    if (us > peak)
                        peak = us;
                }
                if (t % 2 == 0) {
                    FluidFrame *frame = display_acquire_frame();
                    if (frame) {
                        fluid_snapshot(fluid, frame);
                        display_submit_frame();
                    }
                }
                DisplayStats discard;
                while (display_take_stats(&discard)) {
                }
            }
            printf("BENCH,%lu,%lu,%d,%d,%d,%lu,%lu,%lu\n", (unsigned long)clock_get_hz(clk_sys),
                   (unsigned long)display_spi_hz(), n, fluid->count, it, (unsigned long)(sum / 120),
                   (unsigned long)peak, (unsigned long)fluid->clipped);
        }
    puts("BENCH_END");
    fluid_init(fluid, 32, 8);
}

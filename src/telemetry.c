#include "telemetry.h"
#include <stdio.h>
void telemetry_reset(Telemetry *stats, uint64_t now) {
    *stats = (Telemetry){.report_at = now};
}
void telemetry_record_step(Telemetry *stats, uint32_t us) {
    stats->ticks++;
    stats->sim_sum += us;
    if (us > stats->sim_peak)
        stats->sim_peak = us;
}
void telemetry_add_display(Telemetry *stats, const DisplayStats *display) {
    stats->display_count += display->frames;
    stats->raster += display->raster_us;
    stats->display_time += display->total_us;
}
void telemetry_report(Telemetry *stats, uint64_t now, const Fluid *fluid, const InputState *input,
                      uint32_t clock_hz, uint32_t spi_hz) {
    if (now - stats->report_at < 2000000)
        return;
    uint32_t ms = (uint32_t)((now - stats->report_at) / 1000);
    printf("STAT clock=%lu spi=%lu grid=%d p=%d it=%d sim_hz_x10=%lu fps_x10=%lu "
           "sim_us=%lu peak_us=%lu raster_us=%lu lcd_us=%lu late=%lu clip=%lu imu=%02x "
           "raw=%d/%d/%d touch=%d i2c_err=%lu axis=%d/%d/%d\n",
           (unsigned long)clock_hz, (unsigned long)spi_hz, fluid->n, fluid->count,
           fluid->iterations, (unsigned long)(stats->ticks * 10000 / ms),
           (unsigned long)(stats->display_count * 10000 / ms),
           (unsigned long)(stats->ticks ? stats->sim_sum / stats->ticks : 0),
           (unsigned long)stats->sim_peak,
           (unsigned long)(stats->display_count ? stats->raster / stats->display_count : 0),
           (unsigned long)(stats->display_count ? stats->display_time / stats->display_count : 0),
           (unsigned long)stats->late, (unsigned long)fluid->clipped, input->imu_address,
           input->raw_x, input->raw_y, input->raw_z, input->touch_available,
           (unsigned long)input->i2c_errors, input->mapping.x_sign, input->mapping.y_sign,
           input->mapping.swap);
    telemetry_reset(stats, now);
}

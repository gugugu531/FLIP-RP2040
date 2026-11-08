#ifndef FLIP_RP2040_TELEMETRY_H
#define FLIP_RP2040_TELEMETRY_H
#include "fluid.h"
#include "input.h"
#include "display.h"
typedef struct {
    uint64_t report_at;
    uint32_t ticks, sim_sum, sim_peak, late, display_count, raster, display_time;
} Telemetry;
void telemetry_reset(Telemetry *stats, uint64_t now);
void telemetry_record_step(Telemetry *stats, uint32_t us);
void telemetry_add_display(Telemetry *stats, const DisplayStats *display);
void telemetry_report(Telemetry *stats, uint64_t now, const Fluid *fluid, const InputState *input,
                      uint32_t clock_hz, uint32_t spi_hz);
#endif

#ifndef FLIP_RP2040_DISPLAY_H
#define FLIP_RP2040_DISPLAY_H
#include "fluid_frame.h"
#include <stdbool.h>
typedef struct {
    uint32_t frames, raster_us, total_us;
} DisplayStats;
/* Single display service. Init/start once on core 0; core 1 owns LCD thereafter. */
void display_init(void);
void display_start(void);
uint32_t display_spi_hz(void);
/* Core-0 producer: NULL means all buffers are busy; skip this publication.
 * At most one acquired frame; fill it then submit. Never retain/access its
 * pointer after submission. No extra full-frame or density staging buffer. */
FluidFrame *display_acquire_frame(void);
void display_submit_frame(void);
bool display_take_stats(DisplayStats *stats);
#endif

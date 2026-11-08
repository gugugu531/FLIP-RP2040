#ifndef FLIP_RP2040_RENDER_H
#define FLIP_RP2040_RENDER_H
#include "fluid_frame.h"
#include <stdbool.h>
#define RENDER_WIDTH 240
#define RENDER_HEIGHT 240
/* Single renderer (static caches): start at y0=0, then emit strips in order.
 * Require 0 <= y0 < 240, 1 <= rows <= 240-y0 and capacity >= 240*rows.
 * f must remain unchanged across strips. Output is byte-swapped RGB565;
 * desktop consumers swap each word back before interpreting RGB565 channels. */
void fluid_render_rows(const FluidFrame *f, uint16_t *pixels, int y0, int rows);

#endif

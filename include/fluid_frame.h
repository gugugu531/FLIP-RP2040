#ifndef FLIP_RP2040_FLUID_FRAME_H
#define FLIP_RP2040_FLUID_FRAME_H
#include <stdint.h>
#define FLUID_MAX_N 48
#define FLUID_CELLS (FLUID_MAX_N * FLUID_MAX_N)
/* Display interchange format: no particle state or hardware dependency. */
typedef struct {
    int n;
    /* Only the first n*n cells are valid; density units: 256 per particle. */
    uint16_t density[FLUID_CELLS];
} FluidFrame;
#endif

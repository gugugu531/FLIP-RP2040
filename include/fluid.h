#ifndef PENDANT_FLUID_H
#define PENDANT_FLUID_H
#include "fluid_frame.h"
#include <stdbool.h>
#define FLUID_MAX_PARTICLES 2200
#define FLUID_ONE 4096
#define FLUID_HZ 60
/* Grid units, Q20.12 positions and displacement/tick. No floating point or heap. */
typedef struct {
    int32_t x, y, vx, vy;
} Particle;
typedef struct {
    /* n: active square grid; flip: FLIP fraction / 256 (230 by default). */
    int n, count, iterations, separation, flip;
    /* Density and interpolation weights use 256 units per particle. */
    int32_t rest_density;
    Particle particles[FLUID_MAX_PARTICLES];
    int32_t u[FLUID_CELLS], v[FLUID_CELLS];
    int32_t old_u[FLUID_CELLS], old_v[FLUID_CELLS];
    int32_t weight[FLUID_CELLS], density[FLUID_CELLS];
    int16_t heads[FLUID_CELLS], next[FLUID_MAX_PARTICLES];
    uint16_t active[FLUID_CELLS]; /* Wet cell indices for sparse projection. */
    /* faces bits: open left, right, top, bottom neighbours, respectively. */
    uint8_t solid[FLUID_CELLS], wet[FLUID_CELLS], faces[FLUID_CELLS];
    int active_count;
    uint32_t clipped;
    uint32_t stage_us[5]; /* integrate, separate, P2G+density, project, G2P; host: zero. */
} Fluid;
/* Resets the state; clamps n to [24,48] and iterations to [1,40].
 * Allocate Fluid statically on RP2040: it is much larger than a core's stack. */
void fluid_init(Fluid *s, int n, int iterations);
/* One 1/60 s tick. Gravity: milli-g in screen axes (+X right, +Y down).
 * Touch: integer grid coordinates, ignored when touching is false.
 * Solver calls must be serialized, including initial lazy lookup-table setup. */
void fluid_step(Fluid *s, int gx_mg, int gy_mg, int touch_x, int touch_y, bool touching);
/* Caller owns f exclusively while copying; s must not change during the copy. */
void fluid_snapshot(const Fluid *s, FluidFrame *f);
#endif

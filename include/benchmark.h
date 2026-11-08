#ifndef FLIP_RP2040_BENCHMARK_H
#define FLIP_RP2040_BENCHMARK_H
#include "fluid.h"
/* Blocking board benchmark: caller resynchronizes scheduling/statistics after
 * return. Display remains running; restores the default 32/8 fluid profile. */
void benchmark_run(Fluid *fluid);
#endif

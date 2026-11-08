#ifndef FLIP_RP2040_FIXED_MATH_H
#define FLIP_RP2040_FIXED_MATH_H
#include <stdint.h>
/* Floor(sqrt(x)), exact for the entire uint32_t range. */
uint32_t fixed_isqrt(uint32_t x);
#endif

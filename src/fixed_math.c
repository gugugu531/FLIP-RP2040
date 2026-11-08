#include "fixed_math.h"
#ifdef PICO_ON_DEVICE
#define HOT __attribute__((section(".time_critical.math")))
#else
#define HOT
#endif
HOT uint32_t fixed_isqrt(uint32_t x) {
    uint32_t r = 0, b = 1u << 30;
    while (b > x)
        b >>= 2;
    while (b) {
        if (x >= r + b) {
            x -= r + b;
            r = (r >> 1) + b;
        } else
            r >>= 1;
        b >>= 2;
    }
    return r;
}

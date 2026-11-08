#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "render FAIL at %d: %s\n", __LINE__, #c);                              \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static FluidFrame frame;
static uint16_t pixels[240 * 240], rotated[240 * 240], stripe[240 * 8];
static uint16_t native(uint16_t c) {
    return (uint16_t)((c >> 8) | (c << 8));
}
int main(void) {
    for (int n = 32; n <= 48; n += 8) {
        frame.n = n;
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
                frame.density[y * n + x] = y >= n / 2 ? 700 : 0;
        fluid_render_rows(&frame, pixels, 0, 240);
        uint16_t shallow = pixels[128 * 240 + 120], deep = pixels[188 * 240 + 120];
        CHECK(shallow != deep);
        CHECK(((native(shallow) >> 5) & 63) > ((native(deep) >> 5) & 63));
        /* Rotate the free surface from horizontal to vertical: colours must
           follow water depth, including deep water in the upper half-screen. */
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
                frame.density[y * n + x] = x >= n / 2 ? 700 : 0;
        fluid_render_rows(&frame, rotated, 0, 240);
        CHECK(rotated[120 * 240 + 128] == shallow);
        CHECK(rotated[120 * 240 + 188] == deep);
        CHECK(rotated[92 * 240 + 188] == deep);
        for (int row = 0; row < 240; row += 8) {
            fluid_render_rows(&frame, stripe, row, 8);
            CHECK(memcmp(stripe, &rotated[row * 240], sizeof(stripe)) == 0);
        }
        /* The stationary bezel must be independent of the liquid, and have
           sub-4px detail rather than repeating a whole logical pixel block. */
        CHECK(pixels[3 * 240 + 120] == rotated[3 * 240 + 120]);
        CHECK(pixels[0 * 240 + 120] != pixels[3 * 240 + 120]);
        printf("PASS grid=%d: surface-relative depth, native bezel, strip equality\n", n);
    }
    return 0;
}

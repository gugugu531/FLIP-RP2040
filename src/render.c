#include "render.h"
#include "fixed_math.h"
#include <string.h>
#include <assert.h>
#ifdef PICO_ON_DEVICE
#define HOT __attribute__((section(".time_critical.render")))
#else
#define HOT
#endif
#ifndef FLUID_RENDER_SCALE
#define FLUID_RENDER_SCALE 4
#endif
#define SCALE FLUID_RENDER_SCALE
#define WIDTH (240 / SCALE)
_Static_assert(SCALE == 1 || SCALE == 2 || SCALE == 4, "Renderer supports 1x, 2x or 4x pixels");
/* A deliberately small palette: ink, enamel, aged brass and luminous water.
 * The water uses solid logical pixels; only the native-resolution bezel is antialiased. */
enum {
    BLACK,
    INK,
    BACK,
    BACK_LIGHT,
    BRASS_DARK,
    BRASS,
    BRASS_LIGHT,
    WATER_DARK,
    WATER_MID,
    WATER_LIGHT,
    TURQUOISE,
    MINT,
    FOAM,
    COLOUR_COUNT
};
static uint16_t colours[COLOUR_COUNT];
static uint16_t coords[WIDTH];
static int cached_n;
/* Depth is the chamfer distance to air inside the tank, not to the wall.
 * Weights 3/4 approximate Euclidean distance without per-pixel ray marching. */
static uint16_t depth[FLUID_CELLS];
static uint16_t base_row[240];
/* Only the annulus/exterior is stored, leaving the fluid pixel art untouched. */
static uint16_t rim_data[22000], rim_offset[240], rim_count[240];
static bool rim_ready;
static int clamp(int x, int lo, int hi) {
    return x < lo ? lo : x > hi ? hi : x;
}
static uint16_t pack(int r, int g, int b) {
    /* Match the Waveshare byte-buffer convention; 16-bit DMA swaps back before
     * SPI shifts MSB first. See docs/architecture.md before changing byte order. */
    uint16_t c = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    return (uint16_t)((c >> 8) | (c << 8));
}
static void prepare(int n) {
    static const uint8_t rgb[COLOUR_COUNT][3] = {
        {0, 0, 0},      {8, 14, 29},     {15, 25, 43},   {24, 39, 57},  {59, 53, 48},
        {142, 119, 75}, {222, 195, 128}, {15, 62, 99},   {19, 94, 132}, {22, 132, 153},
        {44, 185, 174}, {117, 228, 191}, {218, 248, 210}};
    for (int c = 0; c < COLOUR_COUNT; c++)
        colours[c] = pack(rgb[c][0], rgb[c][1], rgb[c][2]);
    for (int x = 0; x < WIDTH; x++)
        coords[x] = (uint16_t)clamp(((2 * x + 1) * n * 128) / WIDTH - 128, 0, (n - 2) * 256 + 255);
    cached_n = n;
}
static void prepare_depth(const FluidFrame *f) {
    int n = f->n, r = n - 4, seed_r = n - 6;
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            int dx = 2 * x + 1 - n, dy = 2 * y + 1 - n, r2 = dx * dx + dy * dy;
            depth[y * n + x] =
                r2 > r * r ? 65535
                           : (r2 < seed_r * seed_r && f->density[y * n + x] < 140 ? 0 : 16383);
        }
    }
    for (int y = 1; y < n - 1; y++)
        for (int x = 1; x < n - 1; x++) {
            int k = y * n + x;
            if (depth[k] == 65535)
                continue;
            int d = depth[k], a = depth[k - 1] + 3, b = depth[k - n] + 3;
            int c = depth[k - n - 1] + 4, e = depth[k - n + 1] + 4;
            if (a < d)
                d = a;
            if (b < d)
                d = b;
            if (c < d)
                d = c;
            if (e < d)
                d = e;
            depth[k] = (uint16_t)d;
        }
    for (int y = n - 2; y > 0; y--)
        for (int x = n - 2; x > 0; x--) {
            int k = y * n + x;
            if (depth[k] == 65535)
                continue;
            int d = depth[k], a = depth[k + 1] + 3, b = depth[k + n] + 3;
            int c = depth[k + n - 1] + 4, e = depth[k + n + 1] + 4;
            if (a < d)
                d = a;
            if (b < d)
                d = b;
            if (c < d)
                d = c;
            if (e < d)
                d = e;
            depth[k] = (uint16_t)d;
        }
    /* Clamp wall/fully enclosed values before interpolation to avoid a
     * spurious large gradient across the curved edge. */
    for (int k = 0; k < n * n; k++)
        if (depth[k] > 3 * n)
            depth[k] = (uint16_t)(3 * n);
}
static void rim_sample(int x, int y, int rgb[3]) {
    int r2 = x * x + y * y;
    if (r2 >= 476 * 476) {
        rgb[0] = rgb[1] = rgb[2] = 0;
        return;
    }
    if (r2 < 444 * 444) {
        rgb[0] = 8;
        rgb[1] = 14;
        rgb[2] = 29;
        return;
    }
    int light = clamp(128 - (x + y) / 4, 0, 256);
    if (r2 < 460 * 460) {
        rgb[0] = 112 + light * 110 / 256;
        rgb[1] = 92 + light * 103 / 256;
        rgb[2] = 57 + light * 71 / 256;
    } else {
        rgb[0] = 45 + light * 97 / 256;
        rgb[1] = 42 + light * 77 / 256;
        rgb[2] = 38 + light * 37 / 256;
    }
    if (r2 < 460 * 460 && ((x > -6 && x < 6) || (y > -6 && y < 6))) {
        rgb[0] = 8;
        rgb[1] = 14;
        rgb[2] = 29;
    }
}
static void prepare_rim(void) {
    unsigned offset = 0;
    for (int y = 0; y < 240; y++) {
        int dy = 2 * y - 239;
        int inside = 220 * 220 - dy * dy;
        int count = inside <= 0 ? 120 : (239 - (int)fixed_isqrt((uint32_t)inside)) / 2 + 1;
        rim_offset[y] = (uint16_t)offset;
        rim_count[y] = (uint16_t)count;
        assert(offset + 2 * count <= sizeof(rim_data) / sizeof(rim_data[0]));
        for (int side = 0; side < 2; side++)
            for (int j = 0; j < count; j++) {
                int x = side ? 240 - count + j : j;
                int sum[3] = {0}, rgb[3];
                /* 2x2 coverage samples at the native LCD resolution. */
                for (int oy = -1; oy <= 1; oy += 2)
                    for (int ox = -1; ox <= 1; ox += 2) {
                        rim_sample(4 * x - 478 + ox, 4 * y - 478 + oy, rgb);
                        for (int c = 0; c < 3; c++)
                            sum[c] += rgb[c];
                    }
                rim_data[offset++] = pack(sum[0] / 4, sum[1] / 4, sum[2] / 4);
            }
    }
    rim_ready = true;
}
/* Fixed screen-space decoration. The liquid shading still comes entirely from
 * the simulated density: small detached drops are never invented by the art. */
static int background(int px, int py) {
    /* Sparse two-tone Bayer texture, anchored to logical 4px blocks. */
    int c = ((px + 2 * py) & 7) == 0 ? BACK_LIGHT : BACK;
    /* Tiny reflection flecks, not blinking or animated noise. */
    if ((px == 19 && py == 13) || (px == 43 && py == 20) || (px == 11 && py == 31))
        c = BRASS;
    return c;
}
HOT void fluid_render_rows(const FluidFrame *f, uint16_t *pixels, int y0, int rows) {
    int n = f->n;
    if (cached_n != n)
        prepare(n);
    /* Start every frame at row zero; subsequent calls may split it into strips. */
    if (y0 == 0)
        prepare_depth(f);
    if (!rim_ready)
        prepare_rim();
    int radius = 120 - 480 / n, limit = (radius + 2) * (radius + 2);
    for (int y = y0; y < y0 + rows; y++) {
        uint16_t *out = pixels;
        if ((y % SCALE) == 0 || y == y0) {
            pixels = base_row;
            int gy = coords[y / SCALE], iy = gy >> 8, fy = gy & 255;
            int sy = (y / SCALE) * SCALE + SCALE / 2;
            int dy = sy - 120, py = sy / 4;
            const uint16_t *row = &f->density[iy * n];
            for (int x = 0; x < WIDTH; x++) {
                int sx = x * SCALE + SCALE / 2;
                int dx = sx - 120, r2 = dx * dx + dy * dy, px = sx / 4;
                int c;
                if (r2 > limit)
                    c = INK;
                else {
                    int gx = coords[x], ix = gx >> 8, fx = gx & 255;
                    int a = row[ix] * (256 - fx) + row[ix + 1] * fx;
                    int b = row[ix + n] * (256 - fx) + row[ix + n + 1] * fx;
                    int d = (a * (256 - fy) + b * fy) >> 16;
                    c = background(px, py);
                    if (d >= 140) {
                        /* A dark silhouette, then one-pixel mint/cream crest.
                         * Suppress foam against the container wall. */
                        bool wall = r2 > (radius - 4) * (radius - 4);
                        if (d < 215)
                            c = WATER_DARK;
                        else if (d < 380 && !wall)
                            c = ((px + py) & 3) == 0 ? MINT : FOAM;
                        else if (d < 510 && !wall)
                            c = MINT;
                        else {
                            const uint16_t *z = &depth[iy * n];
                            int za = z[ix] * (256 - fx) + z[ix + 1] * fx;
                            int zb = z[ix + n] * (256 - fx) + z[ix + n + 1] * fx;
                            int cells3 = (za * (256 - fy) + zb * fy) >> 16;
                            int shade =
                                cells3 * 80 / n; /* Distance from free surface in LCD pixels. */
                            /* Ordered dithering only along the two colour bands. */
                            int pattern = (px & 1) + 2 * (py & 1);
                            if (shade < 18 || (shade < 22 && pattern == 0))
                                c = TURQUOISE;
                            else if (shade < 44 || (shade < 48 && pattern == 0))
                                c = WATER_LIGHT;
                            else
                                c = WATER_MID;
                            if (wall)
                                c = WATER_DARK;
                        }
                    }
                    /* Short stepped reflection on the glass, fixed to the case. */
                    if ((px == 9 && py >= 18 && py <= 23) || (px == 10 && py >= 16 && py <= 17) ||
                        (px == 11 && py >= 14 && py <= 15))
                        c = d >= 215 ? MINT : BACK_LIGHT;
                }
                uint16_t colour = colours[c];
                for (int repeat = 0; repeat < SCALE; repeat++)
                    *pixels++ = colour;
            }
        }
        memcpy(out, base_row, sizeof(base_row));
        int count = rim_count[y], offset = rim_offset[y];
        memcpy(out, &rim_data[offset], (size_t)count * 2);
        memcpy(out + 240 - count, &rim_data[offset + count], (size_t)count * 2);
        pixels = out + 240;
    }
}

#include "fluid.h"
#include "render.h"
#include <stdio.h>
#include <math.h>
static Fluid s;
static FluidFrame f;
static uint16_t pixels[240 * 240];
int main(int argc, char **argv) {
    if (argc != 2)
        return 1;
    FILE *out = fopen(argv[1], "wb");
    if (!out)
        return 1;
    fluid_init(&s, 32, 8);
    for (int t = 0; t < 480; t++) {
        double a = t < 90 ? 0 : sin((t - 90) * .018) * 2.2;
        fluid_step(&s, (int)(sin(a) * 1000), (int)(cos(a) * 1000), 16, 22, t > 350 && t < 375);
        if (t % 2)
            continue;
        fluid_snapshot(&s, &f);
        fluid_render_rows(&f, pixels, 0, 240);
        for (int k = 0; k < 240 * 240; k++) {
            unsigned c = (pixels[k] >> 8) | (pixels[k] << 8);
            unsigned char rgb[3] = {(c >> 11 & 31) * 255 / 31, (c >> 5 & 63) * 255 / 63,
                                    (c & 31) * 255 / 31};
            if (fwrite(rgb, 1, 3, out) != 3)
                return 1;
        }
    }
    fclose(out);
    return 0;
}

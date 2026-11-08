#include "fluid.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                                   \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static Fluid s;
int main(void) {
    for (int n = 32; n <= 48; n += 8) {
        fluid_init(&s, n, 8 + (n - 32) / 2);
        int count = s.count;
        double before = 0, after = 0, energy_start = 0, energy_end = 0;
        for (int t = 0; t < 2400; t++) {
            int gx = 0, gy = 1000;
            if (t >= 600 && t < 1800) {
                gx = (int)(sin(t * .031) * 2200);
                gy = (int)(cos(t * .031) * 2200);
            }
            if (t >= 1800)
                gx = gy = 0;
            fluid_step(&s, gx, gy, n / 2, n / 2, t % 200 < 20 && t > 600);
            CHECK(s.count == count);
            if (t == 1800 || t == 2399) {
                double e = 0;
                for (int i = 0; i < count; i++) {
                    double vx = s.particles[i].vx, vy = s.particles[i].vy;
                    e += vx * vx + vy * vy;
                }
                if (t == 1800)
                    energy_start = e;
                else
                    energy_end = e;
            }
            for (int j = 0; j < s.active_count; j++) {
                int k = s.active[j];
                double a = s.old_u[k + 1] - s.old_u[k] + s.old_v[k + n] - s.old_v[k] - s.weight[k];
                double b = s.u[k + 1] - s.u[k] + s.v[k + n] - s.v[k] - s.weight[k];
                before += a * a;
                after += b * b;
                if (s.solid[k] || s.solid[k - 1])
                    CHECK(s.u[k] == 0);
                if (s.solid[k] || s.solid[k - n])
                    CHECK(s.v[k] == 0);
            }
            for (int i = 0; i < count; i++) {
                Particle *p = &s.particles[i];
                int dx = (p->x - n * FLUID_ONE / 2) / 16, dy = (p->y - n * FLUID_ONE / 2) / 16,
                    r = (n / 2 - 2) * FLUID_ONE / 16;
                CHECK(dx * dx + dy * dy <= (r + 1) * (r + 1));
                CHECK(abs(p->vx) <= FLUID_ONE * 3 / 4 && abs(p->vy) <= FLUID_ONE * 3 / 4);
            }
            int density = 0;
            for (int k = 0; k < n * n; k++)
                density += s.density[k];
            CHECK(density == count * 256);
            if (t == 599) {
                long long cy = 0;
                for (int i = 0; i < count; i++)
                    cy += s.particles[i].y;
                CHECK(cy / count > n * FLUID_ONE / 2);
            }
        }
        CHECK(after < before * .5);
        CHECK(energy_end < energy_start);
        printf("zero-g energy ratio=%.4f\n", energy_end / energy_start);
        printf("projection residual RMS ratio=%.4f\n", sqrt(after / before));
        printf("host grid=%d particles=%d steps=2400 clipped=%u\n", n, count, s.clipped);
    }
    puts("PASS: containment, mass, velocity bounds, gravity response, rotate/shake/touch/zero-g, "
         "120 simulated seconds total");
}

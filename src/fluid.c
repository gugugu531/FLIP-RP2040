#include "fluid.h"
#include "fixed_math.h"
#include <string.h>
#include <stdlib.h>
#ifdef PICO_ON_DEVICE
#include "pico/time.h"
#define STAMP() time_us_32()
#define HOT __attribute__((section(".time_critical.fluid")))
#else
#define STAMP() 0u
#define HOT
#endif
#define Q FLUID_ONE
#define SPEED (Q * 3 / 4)
static int clamp(int x, int lo, int hi) {
    return x < lo ? lo : x > hi ? hi : x;
}
/* Coordinates remain positive. Weights sum exactly to 256. */
static HOT int stencil(int x, int y, int n, int *w) {
    unsigned ux = (unsigned)x, uy = (unsigned)y;
    int ix = (int)(ux >> 12), iy = (int)(uy >> 12), tx = (ux >> 4) & 255, ty = (uy >> 4) & 255;
    w[0] = ((256 - tx) * (256 - ty)) >> 8;
    w[1] = (tx * (256 - ty)) >> 8;
    w[2] = ((256 - tx) * ty) >> 8;
    w[3] = 256 - w[0] - w[1] - w[2];
    return iy * n + ix;
}
static HOT void collide(Fluid *s, Particle *p) {
    int c = s->n * Q / 2, radius = (s->n / 2 - 2) * Q;
    int dx = (p->x - c) / 16, dy = (p->y - c) / 16, r = radius / 16;
    uint32_t d2 = (uint32_t)(dx * dx + dy * dy);
    if (d2 > (uint32_t)(r * r)) {
        int d = (int)fixed_isqrt(d2) + 1;
        int nx = dx * Q / d, ny = dy * Q / d;
        p->x = c + nx * (radius / Q);
        p->y = c + ny * (radius / Q);
        int vn = (p->vx * nx + p->vy * ny) / Q;
        if (vn > 0) {
            p->vx -= vn * nx / Q;
            p->vy -= vn * ny / Q;
        }
    }
    p->x = clamp(p->x, Q, (s->n - 2) * Q);
    p->y = clamp(p->y, Q, (s->n - 2) * Q);
}
void fluid_init(Fluid *s, int n, int iterations) {
    memset(s, 0, sizeof(*s));
    s->n = clamp(n, 24, FLUID_MAX_N);
    s->iterations = clamp(iterations, 1, 40);
    s->separation = 1;
    s->flip = 230;
    n = s->n;
    int c = n * Q / 2, r = (n / 2 - 2) * Q;
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            int dx = x * Q + Q / 2 - c, dy = y * Q + Q / 2 - c;
            /* Scaled distance avoids overflow at the largest profile. */
            dx /= 16;
            dy /= 16;
            s->solid[y * n + x] = (x == 0 || y == 0 || x == n - 1 || y == n - 1 ||
                                   dx * dx + dy * dy > (r / 16 + 160) * (r / 16 + 160));
        }
    for (int y = 1; y < n - 1; y++)
        for (int x = 1; x < n - 1; x++) {
            int k = y * n + x;
            s->faces[k] = (!s->solid[k - 1]) | ((!s->solid[k + 1]) << 1) |
                          ((!s->solid[k - n]) << 2) | ((!s->solid[k + n]) << 3);
        }
    int row = 0;
    for (int y = c; y < c + r - Q / 2; y += 2304, row++)
        for (int x = c - r + Q / 2 + (row % 2) * 1331; x < c + r; x += 2662) {
            int dx = (x - c) / 16, dy = (y - c) / 16;
            if (dx * dx + dy * dy < (r / 16 - 48) * (r / 16 - 48) && s->count < FLUID_MAX_PARTICLES)
                s->particles[s->count++] = (Particle){x, y, 0, 0};
        }
    s->rest_density = 700; /* ~2.73 particles/cell, triangular spacing .65 x .5625 */
}
/* Separation factor versus squared Q8 distance. Quantization is <=16/256^2
   cell^2; tiny distances use the exact path. Built once, shared read-only. */
static int32_t separation_factor[1600];
static bool factors_ready;
static HOT void separate_pair(Particle *p, Particle *b, int parity) {
    int dx = (b->x - p->x) / 16, dy = (b->y - p->y) / 16;
    if (abs(dx) >= 160 || abs(dy) >= 160)
        return;
    int d2 = dx * dx + dy * dy;
    if (d2 >= 160 * 160)
        return;
    if (!d2) {
        dx = parity ? 1 : -1;
        dy = 1;
        d2 = 2;
    }
    int factor;
    if (d2 < 16) {
        int d = (int)fixed_isqrt((uint32_t)d2);
        factor = (160 - d) * 2048 / d;
    } else
        factor = separation_factor[d2 >> 4];
    int ox = dx * factor / 256, oy = dy * factor / 256;
    p->x -= ox;
    p->y -= oy;
    b->x += ox;
    b->y += oy;
}
static HOT void separate(Fluid *s) {
    int n = s->n;
    if (!factors_ready) {
        for (int k = 1; k < 1600; k++) {
            int d = (int)fixed_isqrt((uint32_t)(k * 16 + 8));
            separation_factor[k] = (160 - d) * 2048 / d;
        }
        factors_ready = true;
    }
    memset(s->heads, 0xff, (size_t)n * n * sizeof(s->heads[0]));
    for (int i = 0; i < s->count; i++) {
        Particle *p = &s->particles[i];
        int k = (p->y / Q) * n + p->x / Q;
        s->next[i] = s->heads[k];
        s->heads[k] = (int16_t)i;
    }
    /* Visit each unordered cell pair only once. All particles are contained
       at least one cell inside the array, so these four offsets are safe. */
    int offsets[4] = {1, n - 1, n, n + 1};
    for (int k = n + 1; k < n * n - n - 1; k++) {
        for (int i = s->heads[k]; i >= 0; i = s->next[i]) {
            Particle *p = &s->particles[i];
            for (int j = s->next[i]; j >= 0; j = s->next[j])
                separate_pair(p, &s->particles[j], i & 1);
            for (int o = 0; o < 4; o++)
                for (int j = s->heads[k + offsets[o]]; j >= 0; j = s->next[j])
                    separate_pair(p, &s->particles[j], i & 1);
        }
    }
    for (int i = 0; i < s->count; i++)
        collide(s, &s->particles[i]);
}
static HOT void transfer(Fluid *s, bool to_grid) {
    int n = s->n, cells = n * n, off[4] = {0, 1, n, n + 1};
    for (int axis = 0; axis < 2; axis++) {
        int32_t *f = axis ? s->v : s->u, *old = axis ? s->old_v : s->old_u;
        if (to_grid) {
            memset(f, 0, (size_t)cells * 4);
            memset(s->weight, 0, (size_t)cells * 4);
        }
        for (int i = 0; i < s->count; i++) {
            Particle *p = &s->particles[i];
            int w[4];
            int k = stencil(p->x - (axis ? Q / 2 : 0), p->y - (axis ? 0 : Q / 2), n, w);
            int32_t *pv = axis ? &p->vy : &p->vx;
            if (to_grid)
                for (int j = 0; j < 4; j++) {
                    f[k + off[j]] += *pv * w[j];
                    s->weight[k + off[j]] += w[j];
                }
            else {
                int sum = 0, pic = 0, delta = 0, offset = axis ? n : 1;
                for (int j = 0; j < 4; j++) {
                    int a = k + off[j];
                    if (s->wet[a] || s->wet[a - offset]) {
                        sum += w[j];
                        pic += f[a] * w[j];
                        delta += (f[a] - old[a]) * w[j];
                    }
                }
                if (sum) {
                    int vpic, vflip;
                    if (sum == 256) {
                        vpic = pic / 256;
                        vflip = *pv + delta / 256;
                    } else {
                        vpic = pic / sum;
                        vflip = *pv + delta / sum;
                    }
                    int v = (vpic * (256 - s->flip) + vflip * s->flip) / 256;
                    if (abs(v) > SPEED)
                        s->clipped++;
                    *pv = clamp(v, -SPEED, SPEED);
                }
            }
        }
        if (to_grid)
            for (int k = 0; k < cells; k++) {
                if (s->weight[k])
                    f[k] /= s->weight[k];
                int offset = axis ? n : 1;
                if (k < offset || s->solid[k] || s->solid[k - offset])
                    f[k] = 0;
            }
    }
}
HOT void fluid_step(Fluid *s, int gx_mg, int gy_mg, int tx, int ty, bool touching) {
    uint32_t stamp = STAMP();
    int n = s->n, cells = n * n;
    /* A 1g acceleration travels one tank diameter in ~1 second. */
    int ax = clamp(gx_mg, -2500, 2500) * n / 440, ay = clamp(gy_mg, -2500, 2500) * n / 440;
    for (int i = 0; i < s->count; i++) {
        Particle *p = &s->particles[i];
        p->vx = clamp(p->vx + ax, -SPEED, SPEED);
        p->vy = clamp(p->vy + ay, -SPEED, SPEED);
        if (touching) {
            int dx = p->x / Q - tx, dy = p->y / Q - ty, d2 = dx * dx + dy * dy;
            if (d2 > 0 && d2 < 25) {
                p->vx = clamp(p->vx + dx * 80, -SPEED, SPEED);
                p->vy = clamp(p->vy + dy * 80, -SPEED, SPEED);
            }
        }
        p->x += p->vx;
        p->y += p->vy;
        collide(s, p);
    }
    s->stage_us[0] = STAMP() - stamp;
    stamp = STAMP();
    for (int i = 0; i < s->separation; i++)
        separate(s);
    s->stage_us[1] = STAMP() - stamp;
    stamp = STAMP();
    memset(s->wet, 0, (size_t)cells);
    memset(s->density, 0, (size_t)cells * 4);
    int off[4] = {0, 1, n, n + 1};
    for (int i = 0; i < s->count; i++) {
        Particle *p = &s->particles[i];
        int w[4], k = stencil(p->x - Q / 2, p->y - Q / 2, n, w);
        s->wet[(p->y / Q) * n + p->x / Q] = 1;
        for (int j = 0; j < 4; j++)
            s->density[k + off[j]] += w[j];
    }
    transfer(s, true);
    memcpy(s->old_u, s->u, (size_t)cells * 4);
    memcpy(s->old_v, s->v, (size_t)cells * 4);
    s->active_count = 0;
    for (int k = n + 1; k < cells - n - 1; k++)
        if (s->wet[k] && !s->solid[k]) {
            s->active[s->active_count++] = (uint16_t)k;
            s->weight[k] = clamp((s->density[k] - s->rest_density) * 2, 0, Q / 4);
        }
    /* In-place MAC projection like flip.html. Precomputed solid face mask;
       reciprocal+SOR coefficients replace division in the pressure hot loop. */
    s->stage_us[2] = STAMP() - stamp;
    stamp = STAMP();
    const int coeff[5] = {0, 435, 218, 145, 109};
    for (int it = 0; it < s->iterations; it++)
        for (int j = 0; j < s->active_count; j++) {
            int k = s->active[j], m = s->faces[k];
            int div = s->u[k + 1] - s->u[k] + s->v[k + n] - s->v[k] - s->weight[k];
            if (m == 15) {
                int p = -div * 109 / 256;
                s->u[k] -= p;
                s->u[k + 1] += p;
                s->v[k] -= p;
                s->v[k + n] += p;
            } else {
                int a = m & 1, b = (m >> 1) & 1, c = (m >> 2) & 1, d = (m >> 3) & 1;
                int p = -div * coeff[a + b + c + d] / 256;
                s->u[k] -= a * p;
                s->u[k + 1] += b * p;
                s->v[k] -= c * p;
                s->v[k + n] += d * p;
            }
        }
    s->stage_us[3] = STAMP() - stamp;
    stamp = STAMP();
    transfer(s, false);
    s->stage_us[4] = STAMP() - stamp;
}
void fluid_snapshot(const Fluid *s, FluidFrame *f) {
    f->n = s->n;
    for (int k = 0; k < s->n * s->n; k++)
        f->density[k] = (uint16_t)clamp(s->density[k], 0, 4095);
}

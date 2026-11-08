#include "fluid.h"
#include "render.h"
#include <SDL.h>
#include <math.h>
static Fluid s;
static FluidFrame frame;
static uint16_t pixels[240 * 240];
int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
        return 1;
    SDL_Window *w =
        SDL_CreateWindow("flip_rp2040 | arrows: gravity | mouse: stir | 1/2/3: quality | R: reset",
                         100, 100, 720, 720, 0);
    if (!w) {
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *r = SDL_CreateRenderer(w, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!r)
        r = SDL_CreateRenderer(w, -1, SDL_RENDERER_SOFTWARE);
    if (!r) {
        SDL_DestroyWindow(w);
        SDL_Quit();
        return 1;
    }
    SDL_Texture *tex =
        SDL_CreateTexture(r, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 240, 240);
    if (!tex) {
        SDL_DestroyRenderer(r);
        SDL_DestroyWindow(w);
        SDL_Quit();
        return 1;
    }
    fluid_init(&s, 32, 8);
    int run = 1;
    double angle = 0;
    Uint64 last = SDL_GetPerformanceCounter();
    double acc = 0;
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                run = 0;
            if (e.type == SDL_KEYDOWN) {
                int k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE)
                    run = 0;
                if (k == SDLK_r)
                    fluid_init(&s, s.n, s.iterations);
                if (k >= SDLK_1 && k <= SDLK_3)
                    fluid_init(&s, 32 + 8 * (k - SDLK_1), 8 + 4 * (k - SDLK_1));
            }
        }
        Uint64 cur = SDL_GetPerformanceCounter();
        acc += (double)(cur - last) / SDL_GetPerformanceFrequency();
        last = cur;
        if (acc > .05)
            acc = .05;
        const Uint8 *keys = SDL_GetKeyboardState(0);
        angle += (keys[SDL_SCANCODE_RIGHT] - keys[SDL_SCANCODE_LEFT]) * .04;
        int mx, my;
        int down = SDL_GetMouseState(&mx, &my) & SDL_BUTTON(SDL_BUTTON_LEFT);
        while (acc >= 1.0 / 60) {
            fluid_step(&s, (int)(sin(angle) * 1000), (int)(cos(angle) * 1000), mx * s.n / 720,
                       my * s.n / 720, down);
            acc -= 1.0 / 60;
        }
        fluid_snapshot(&s, &frame);
        fluid_render_rows(&frame, pixels, 0, 240);
        for (int i = 0; i < 240 * 240; i++)
            pixels[i] = (uint16_t)((pixels[i] >> 8) | (pixels[i] << 8));
        SDL_UpdateTexture(tex, 0, pixels, 480);
        SDL_RenderClear(r);
        SDL_RenderCopy(r, tex, 0, 0);
        SDL_RenderPresent(r);
        SDL_Delay(1);
    }
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}

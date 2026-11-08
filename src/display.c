#include "display.h"
#include "render.h"
#include "DEV_Config.h"
#include "LCD_1in28.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include <assert.h>
#include <string.h>
#ifndef PENDANT_SPI_HZ
#define PENDANT_SPI_HZ 66500000
#endif
static FluidFrame frames[3];
static uint16_t stripes[2][240 * 8];
static queue_t free_frames, ready_frames, render_stats;
static int acquired = -1; /* Core-0 only; queue operations transfer ownership. */
static uint32_t spi_hz;
static void display_core(void) {
    DisplayStats stats = {0};
    for (;;) {
        int id, next;
        queue_remove_blocking(&ready_frames, &id);
        /* Keep the newest immutable snapshot; return every superseded buffer. */
        while (queue_try_remove(&ready_frames, &next)) {
            queue_add_blocking(&free_frames, &id);
            id = next;
        }
        uint32_t start = time_us_32();
        spi_set_format(LCD_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        LCD_1IN28_SetWindows(0, 0, 240, 240);
        gpio_put(LCD_DC_PIN, 1);
        spi_set_format(LCD_SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        for (int y = 0; y < 240; y += 8) {
            /* Render into the alternate stripe while DMA sends the previous one.
             * Wait before launching the next DMA; never overwrite its source. */
            uint32_t t = time_us_32();
            int b = (y / 8) & 1;
            fluid_render_rows(&frames[id], stripes[b], y, 8);
            stats.raster_us += time_us_32() - t;
            DEV_SPI_Wait_Async_Complete(LCD_SPI_PORT);
            DEV_SPI_Write_RGB565_Async(LCD_SPI_PORT, stripes[b], 240 * 8);
        }
        DEV_SPI_Wait_Async_Complete(LCD_SPI_PORT);
        stats.total_us += time_us_32() - start;
        stats.frames++;
        queue_add_blocking(&free_frames, &id);
        if (queue_try_add(&render_stats, &stats))
            memset(&stats, 0, sizeof(stats));
    }
}

void display_init(void) {
    spi_hz = spi_set_baudrate(LCD_SPI_PORT, PENDANT_SPI_HZ);
    LCD_1IN28_Init(HORIZONTAL);
    DEV_SET_PWM(65);
    queue_init(&free_frames, sizeof(int), 3);
    queue_init(&ready_frames, sizeof(int), 3);
    queue_init(&render_stats, sizeof(DisplayStats), 4);
    for (int i = 0; i < 3; i++)
        queue_add_blocking(&free_frames, &i);
}
void display_start(void) {
    multicore_launch_core1(display_core);
}
uint32_t display_spi_hz(void) {
    return spi_hz;
}
FluidFrame *display_acquire_frame(void) {
    assert(acquired == -1);
    if (!queue_try_remove(&free_frames, &acquired))
        return NULL;
    return &frames[acquired];
}
void display_submit_frame(void) {
    assert(acquired >= 0);
    /* Holding one of three IDs guarantees space in the 3-entry ready queue. */
    queue_add_blocking(&ready_frames, &acquired);
    acquired = -1;
}
bool display_take_stats(DisplayStats *stats) {
    return queue_try_remove(&render_stats, stats);
}

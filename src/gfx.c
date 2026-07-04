#include "gfx.h"
#include "st7735.h"

void gfx_draw_pixel(uint8_t x, uint8_t y, uint16_t color)
{
    st7735_draw_pixel(x, y, color);
}

void gfx_draw_hline(uint8_t x, uint8_t y, uint8_t w, uint16_t color)
{
    for (uint8_t i = 0; i < w; i++) {
        gfx_draw_pixel(x + i, y, color);
    }
}

void gfx_draw_vline(uint8_t x, uint8_t y, uint8_t h, uint16_t color)
{
    for (uint8_t i = 0; i < h; i++) {
        gfx_draw_pixel(x, y + i, color);
    }
}

void gfx_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
    gfx_draw_hline(x, y, w, color);
    gfx_draw_hline(x, y + h - 1, w, color);
    gfx_draw_vline(x, y, h, color);
    gfx_draw_vline(x + w - 1, y, h, color);
}

void gfx_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
    for (uint8_t yy = 0; yy < h; yy++) {
        gfx_draw_hline(x, y + yy, w, color);
    }
}
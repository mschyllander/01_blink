#ifndef GFX_H
#define GFX_H

#include <stdint.h>

void gfx_draw_pixel(uint8_t x, uint8_t y, uint16_t color);
void gfx_draw_hline(uint8_t x, uint8_t y, uint8_t w, uint16_t color);
void gfx_draw_vline(uint8_t x, uint8_t y, uint8_t h, uint16_t color);
void gfx_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void gfx_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

#endif
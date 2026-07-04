#ifndef ST7735_H
#define ST7735_H

#include <stdint.h>

#define ST7735_WIDTH   128
#define ST7735_HEIGHT  160

#define ST7735_BLACK   0x0000
#define ST7735_WHITE   0xFFFF
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_BLUE    0x001F

void st7735_init(void);
void st7735_fill(uint16_t color);
void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color);
void st7735_draw_framebuffer(const uint16_t *buffer);

#endif
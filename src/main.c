#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/adc.h>
#include <stdint.h>
#include <string.h>

#include "st7735.h"
#include "rgb.h"

#define W ST7735_WIDTH
#define H ST7735_HEIGHT
#define CX (W / 2)
#define CY (H / 2)

#define STAR_COUNT 120
#define ADC_CHANNEL 4
#define ADC_RESOLUTION 12
#define ADC_MAX 4095

#define PLASMA_CX 64
#define PLASMA_R  30

static uint16_t fb[W * H];

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t speed;
} Star;

static Star stars[STAR_COUNT];

static uint8_t sin8[256];
static uint16_t palette[256];

static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));
static int pot_filtered = 0;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}

static uint8_t wave8(uint8_t x)
{
    int v = x;

    if (v > 127) {
        v = 255 - v;
    }

    v = v - 64;

    int y = 255 - ((v * v) >> 4);

    if (y < 0) y = 0;
    if (y > 255) y = 255;

    return (uint8_t)y;
}

static void make_tables(void)
{
    for (int i = 0; i < 256; i++) {
        sin8[i] = wave8((uint8_t)i);

        uint8_t r = wave8((uint8_t)(i + 0));
        uint8_t g = wave8((uint8_t)(i + 85));
        uint8_t b = wave8((uint8_t)(i + 170));

        palette[i] = rgb565(r, g, b);
    }
}

static void pot_init(void)
{
    if (!device_is_ready(adc_dev)) {
        printk("ADC not ready\n");
        return;
    }

    struct adc_channel_cfg cfg = {
        .gain = ADC_GAIN_1,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .channel_id = ADC_CHANNEL,
    };

    int ret = adc_channel_setup(adc_dev, &cfg);
    if (ret < 0) {
        printk("ADC setup error: %d\n", ret);
    }
}

static int pot_read_speed(void)
{
    int16_t sample = 0;

    struct adc_sequence seq = {
        .channels = BIT(ADC_CHANNEL),
        .buffer = &sample,
        .buffer_size = sizeof(sample),
        .resolution = ADC_RESOLUTION,
    };

    int ret = adc_read(adc_dev, &seq);
    if (ret < 0) {
        return 2;
    }

    if (sample < 0) sample = 0;
    if (sample > ADC_MAX) sample = ADC_MAX;

    pot_filtered = (pot_filtered * 7 + sample) / 8;

    return 1 + (pot_filtered * 5) / ADC_MAX;   // 1..6
}

static void fb_clear(uint16_t color)
{
    for (int i = 0; i < W * H; i++) {
        fb[i] = color;
    }
}

static void fb_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= W || y < 0 || y >= H) {
        return;
    }

    fb[y * W + x] = color;
}

static void fb_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        fb_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void plasma_circle(int cy, uint8_t frame)
{
    int r2 = PLASMA_R * PLASMA_R;

    for (int y = -PLASMA_R; y <= PLASMA_R; y++) {
        for (int x = -PLASMA_R; x <= PLASMA_R; x++) {
            int d2 = x * x + y * y;

            if (d2 > r2) {
                continue;
            }

            uint8_t dist = (uint8_t)(d2 >> 3);

            uint8_t v =
                sin8[(uint8_t)(x * 3 + frame)] +
                sin8[(uint8_t)(y * 3 + frame * 2)] +
                sin8[(uint8_t)(dist * 3 - frame * 2)];

            fb_pixel(PLASMA_CX + x, cy + y, palette[v]);
        }
    }
}

static void stars_init(void)
{
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].x = ((i * 73) % 256) - 128;
        stars[i].y = ((i * 41) % 256) - 128;
        stars[i].z = 64 + ((i * 97) % 448);
        stars[i].speed = 3 + (i % 5);
    }
}

static void reset_star(int i)
{
    stars[i].x = ((i * 91 + stars[i].z) % 256) - 128;
    stars[i].y = ((i * 47 + stars[i].x) % 256) - 128;
    stars[i].z = 512;
    stars[i].speed = 3 + (i % 5);
}

static void stars_update_draw(uint8_t frame, int speed_boost)
{
    for (int i = 0; i < STAR_COUNT; i++) {
        int old_z = stars[i].z;

        int old_angle = (frame + i * 3) & 0xFF;
        int old_cosv = (int)sin8[(uint8_t)(old_angle + 64)] - 128;
        int old_sinv = (int)sin8[(uint8_t)(old_angle)] - 128;

        int old_rx = ((stars[i].x * old_cosv) - (stars[i].y * old_sinv)) / 128;
        int old_ry = ((stars[i].x * old_sinv) + (stars[i].y * old_cosv)) / 128;

        int old_sx = CX + (old_rx * 96) / old_z;
        int old_sy = CY + (old_ry * 96) / old_z;

        stars[i].z -= stars[i].speed + speed_boost;

        if (stars[i].z <= 24) {
            reset_star(i);
            continue;
        }

        int angle = (frame + i * 3) & 0xFF;
        int cosv = (int)sin8[(uint8_t)(angle + 64)] - 128;
        int sinv = (int)sin8[(uint8_t)(angle)] - 128;

        int rx = ((stars[i].x * cosv) - (stars[i].y * sinv)) / 128;
        int ry = ((stars[i].x * sinv) + (stars[i].y * cosv)) / 128;

        int sx = CX + (rx * 96) / stars[i].z;
        int sy = CY + (ry * 96) / stars[i].z;

        if (sx < 0 || sx >= W || sy < 0 || sy >= H) {
            reset_star(i);
            continue;
        }

        int b = 255 - (stars[i].z >> 1);
        if (b < 40) b = 40;
        if (b > 255) b = 255;

        uint16_t c = rgb565((uint8_t)b, (uint8_t)b, (uint8_t)b);

        fb_line(old_sx, old_sy, sx, sy, c);

        if (stars[i].z < 80) {
            fb_pixel(sx + 1, sy, ST7735_WHITE);
            fb_pixel(sx, sy + 1, ST7735_WHITE);
        }
    }
}

static const uint8_t font5x7[][5] = {
    [' '] = {0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x00,0x00,0x5F,0x00,0x00},
    ['.'] = {0x00,0x60,0x60,0x00,0x00},
    ['A'] = {0x7E,0x11,0x11,0x11,0x7E},
    ['B'] = {0x7F,0x49,0x49,0x49,0x36},
    ['C'] = {0x3E,0x41,0x41,0x41,0x22},
    ['D'] = {0x7F,0x41,0x41,0x22,0x1C},
    ['E'] = {0x7F,0x49,0x49,0x49,0x41},
    ['F'] = {0x7F,0x09,0x09,0x09,0x01},
    ['G'] = {0x3E,0x41,0x49,0x49,0x7A},
    ['H'] = {0x7F,0x08,0x08,0x08,0x7F},
    ['I'] = {0x00,0x41,0x7F,0x41,0x00},
    ['J'] = {0x20,0x40,0x41,0x3F,0x01},
    ['K'] = {0x7F,0x08,0x14,0x22,0x41},
    ['L'] = {0x7F,0x40,0x40,0x40,0x40},
    ['M'] = {0x7F,0x02,0x0C,0x02,0x7F},
    ['N'] = {0x7F,0x04,0x08,0x10,0x7F},
    ['O'] = {0x3E,0x41,0x41,0x41,0x3E},
    ['P'] = {0x7F,0x09,0x09,0x09,0x06},
    ['Q'] = {0x3E,0x41,0x51,0x21,0x5E},
    ['R'] = {0x7F,0x09,0x19,0x29,0x46},
    ['S'] = {0x46,0x49,0x49,0x49,0x31},
    ['T'] = {0x01,0x01,0x7F,0x01,0x01},
    ['U'] = {0x3F,0x40,0x40,0x40,0x3F},
    ['V'] = {0x1F,0x20,0x40,0x20,0x1F},
    ['W'] = {0x7F,0x20,0x18,0x20,0x7F},
    ['X'] = {0x63,0x14,0x08,0x14,0x63},
    ['Y'] = {0x07,0x08,0x70,0x08,0x07},
    ['Z'] = {0x61,0x51,0x49,0x45,0x43},
};

static void fb_char(int x, int y, char c, uint16_t color)
{
    if (c < 0 || c > 127) {
        return;
    }

    for (int col = 0; col < 5; col++) {
        uint8_t bits = font5x7[(int)c][col];

        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                fb_pixel(x + col, y + row, color);
            }
        }
    }
}

static void scroll_text_wave(int x, int base_y, const char *text, uint8_t frame)
{
    int char_index = 0;

    while (*text) {
        int wave = ((int)sin8[(uint8_t)(frame * 3 + char_index * 9)] - 128) / 16;
        uint16_t c = palette[(uint8_t)(frame * 4 + char_index * 16)];

        fb_char(x, base_y + wave, *text, c);

        x += 6;
        text++;
        char_index++;
    }
}

int main(void)
{
    printk("Stable demo with pot speed\n");

    st7735_init();
    rgb_init();
    rgb_set(1, 0, 1);

    pot_init();
    make_tables();
    stars_init();

    uint8_t frame = 0;

    const char *scroll = "   MR MATZO STRIKES AGAIN   ZEPHYR ST7735 AMIGA STYLE DEMO   ";
    int scroll_x = W;

    int plasma_y = 45 << 8;
    int plasma_vy = 90;

    while (1) {
        int speed = pot_read_speed();

        fb_clear(ST7735_BLACK);

        stars_update_draw(frame, speed / 2);

        plasma_y += plasma_vy;

        int top_limit = (PLASMA_R + 5) << 8;
        int bottom_limit = (124 - PLASMA_R) << 8;

        if (plasma_y <= top_limit) {
            plasma_y = top_limit;
            plasma_vy = -plasma_vy;
        }

        if (plasma_y >= bottom_limit) {
            plasma_y = bottom_limit;
            plasma_vy = -plasma_vy;
        }

        plasma_circle(plasma_y >> 8, frame);

        scroll_text_wave(scroll_x, 132, scroll, frame);

        scroll_x -= speed;

        if (scroll_x < -(int)(strlen(scroll) * 6)) {
            scroll_x = W;
        }

        st7735_draw_framebuffer(fb);

        frame += speed;
    }
}
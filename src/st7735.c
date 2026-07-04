#include "st7735.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <stddef.h>

#define PIN_DC   2
#define PIN_RST  0
#define PIN_CS   1

static const struct device *gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(spi2));

static struct spi_config spi_cfg = {
    .frequency = 20000000,
    .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
    .slave = 0,
};

static int st7735_write(const uint8_t *data, size_t len)
{
    struct spi_buf buf = {
        .buf = (void *)data,
        .len = len,
    };

    struct spi_buf_set tx = {
        .buffers = &buf,
        .count = 1,
    };

    return spi_write(spi, &spi_cfg, &tx);
}

static void st7735_cmd(uint8_t cmd)
{
    gpio_pin_set(gpio, PIN_DC, 0);
    gpio_pin_set(gpio, PIN_CS, 0);
    st7735_write(&cmd, 1);
    gpio_pin_set(gpio, PIN_CS, 1);
}

static void st7735_data(const uint8_t *data, size_t len)
{
    gpio_pin_set(gpio, PIN_DC, 1);
    gpio_pin_set(gpio, PIN_CS, 0);
    st7735_write(data, len);
    gpio_pin_set(gpio, PIN_CS, 1);
}

static void st7735_data8(uint8_t data)
{
    st7735_data(&data, 1);
}

static void st7735_reset(void)
{
    gpio_pin_set(gpio, PIN_RST, 1);
    k_msleep(50);

    gpio_pin_set(gpio, PIN_RST, 0);
    k_msleep(100);

    gpio_pin_set(gpio, PIN_RST, 1);
    k_msleep(150);
}

static void st7735_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t data[4];

    st7735_cmd(0x2A);
    data[0] = 0x00;
    data[1] = x0;
    data[2] = 0x00;
    data[3] = x1;
    st7735_data(data, 4);

    st7735_cmd(0x2B);
    data[0] = 0x00;
    data[1] = y0;
    data[2] = 0x00;
    data[3] = y1;
    st7735_data(data, 4);

    st7735_cmd(0x2C);
}

void st7735_init(void)
{
    if (!device_is_ready(gpio)) {
        printk("ST7735 GPIO not ready\n");
        return;
    }

    if (!device_is_ready(spi)) {
        printk("ST7735 SPI not ready\n");
        return;
    }

    gpio_pin_configure(gpio, PIN_DC, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(gpio, PIN_RST, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure(gpio, PIN_CS, GPIO_OUTPUT_ACTIVE);

    st7735_reset();

    st7735_cmd(0x01);      // software reset
    k_msleep(150);

    st7735_cmd(0x11);      // sleep out
    k_msleep(150);

    st7735_cmd(0x3A);      // color mode
    st7735_data8(0x05);    // RGB565

    st7735_cmd(0x36);      // MADCTL
    st7735_data8(0x00);

    st7735_cmd(0x20);      // inversion OFF
    k_msleep(10);

    st7735_cmd(0x13);      // normal display mode ON
    k_msleep(10);

    st7735_cmd(0x29);      // display ON
    k_msleep(150);
}

void st7735_fill(uint16_t color)
{
    uint8_t pixel[2] = {
        (uint8_t)(color >> 8),
        (uint8_t)(color & 0xFF)
    };

    st7735_set_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);

    gpio_pin_set(gpio, PIN_DC, 1);
    gpio_pin_set(gpio, PIN_CS, 0);

    for (int i = 0; i < ST7735_WIDTH * ST7735_HEIGHT; i++) {
        st7735_write(pixel, 2);
    }

    gpio_pin_set(gpio, PIN_CS, 1);
}

void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) {
        return;
    }

    uint8_t pixel[2] = {
        (uint8_t)(color >> 8),
        (uint8_t)(color & 0xFF)
    };

    st7735_set_window(x, y, x, y);
    st7735_data(pixel, 2);
}

void st7735_draw_framebuffer(const uint16_t *buffer)
{
    static uint8_t txbuf[ST7735_WIDTH * ST7735_HEIGHT * 2];

    for (int i = 0; i < ST7735_WIDTH * ST7735_HEIGHT; i++) {
        txbuf[i * 2 + 0] = (uint8_t)(buffer[i] >> 8);
        txbuf[i * 2 + 1] = (uint8_t)(buffer[i] & 0xFF);
    }

    st7735_set_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);

    gpio_pin_set(gpio, PIN_DC, 1);
    gpio_pin_set(gpio, PIN_CS, 0);

    st7735_write(txbuf, sizeof(txbuf));

    gpio_pin_set(gpio, PIN_CS, 1);
}
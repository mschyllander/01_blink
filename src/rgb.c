#include "rgb.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

static const struct gpio_dt_spec led_red =
    GPIO_DT_SPEC_GET(DT_ALIAS(led_red), gpios);

static const struct gpio_dt_spec led_green =
    GPIO_DT_SPEC_GET(DT_ALIAS(led_green), gpios);

static const struct gpio_dt_spec led_blue =
    GPIO_DT_SPEC_GET(DT_ALIAS(led_blue), gpios);

void rgb_init(void)
{
    if (!gpio_is_ready_dt(&led_red) ||
        !gpio_is_ready_dt(&led_green) ||
        !gpio_is_ready_dt(&led_blue)) {
        printk("RGB GPIO not ready\n");
        return;
    }

    gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
}

void rgb_set(int r, int g, int b)
{
    gpio_pin_set_dt(&led_red, r);
    gpio_pin_set_dt(&led_green, g);
    gpio_pin_set_dt(&led_blue, b);
}

void rgb_off(void)
{
    rgb_set(0, 0, 0);
}
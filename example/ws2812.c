#include <math.h>

#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "ws2812.h"

RGBColor hsv2rgb(float H, float S, float V) {
    float r = 0, g = 0, b = 0;

    float h = H / 360;
    float s = S / 100;
    float v = V / 100;

    int i = floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    RGBColor color;
    color.r = r * 255;
    color.g = g * 255;
    color.b = b * 255;

    return color;
}

static inline void __time_critical_func(ws2812_transmit_bit)(int gpio, uint8_t bit)
{
    if (!bit)
    {
        gpio_put(gpio, true);

        // 0.125 CPU cycle per 1ns
        // 8 ns per CPU cycle = 0.008 us

        // 1 + 3 * 16 + 1 = 50 CPU cycles = 50 * 0.008us = 0.4us
        asm volatile(
            "mov  r0, #16\n"    		// 1 cycle
            "1: sub  r0, r0, #1\n"	// 1 cycle
            "bne   1b\n"          	// 2 cycles if loop taken, 1 if not
        );

        gpio_put(gpio, false);

        // 1 + 3 * 32 + 1 = 100 CPU cycles = 100 * 0.008us = 0.8us
        asm volatile(
            "mov  r0, #32\n"    		// 1 cycle
            "3: sub  r0, r0, #1\n"	// 1 cycle
            "bne   3b\n"          	// 2 cycles if loop taken, 1 if not
        );
    }
    else
    {
        gpio_put(gpio, true);

        // 0.125 CPU cycle per 1ns
        // 8 ns per CPU cycle = 0.008 us

        // 1 + 3 * 32 + 1 = 100 CPU cycles = 100 * 0.008us = 0.8us
        asm volatile(
            "mov  r0, #32\n"    		// 1 cycle
            "2: sub  r0, r0, #1\n"	// 1 cycle
            "bne   2b\n"          	// 2 cycles if loop taken, 1 if not
        );

        gpio_put(gpio, false);

        // 1 + 3 * 16 + 1 = 40 CPU cycles = 40 * 0.008us = 0.4us
        asm volatile(
            "mov  r0, #16\n"    		// 1 cycle
            "4: sub  r0, r0, #1\n"	// 1 cycle
            "bne   4b\n"          	// 2 cycles if loop taken, 1 if not
        );
    }
}

void __time_critical_func(ws2812_transmit_color)(int gpio, uint8_t data)
{
    ws2812_transmit_bit(gpio, data & 0x80);
    ws2812_transmit_bit(gpio, data & 0x40);
    ws2812_transmit_bit(gpio, data & 0x20);
    ws2812_transmit_bit(gpio, data & 0x10);
    ws2812_transmit_bit(gpio, data & 0x8);
    ws2812_transmit_bit(gpio, data & 0x4);
    ws2812_transmit_bit(gpio, data & 0x2);
    ws2812_transmit_bit(gpio, data & 0x1);
}

void __time_critical_func(ws2812_transmit)(int gpio, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t ints = save_and_disable_interrupts();
    ws2812_transmit_color(gpio, r);
    ws2812_transmit_color(gpio, g);
    ws2812_transmit_color(gpio, b);
    restore_interrupts(ints);
}

void __time_critical_func(ws2812_transmit_4)(int gpio, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    uint32_t ints = save_and_disable_interrupts();
    ws2812_transmit_color(gpio, r);
    ws2812_transmit_color(gpio, g);
    ws2812_transmit_color(gpio, b);
    ws2812_transmit_color(gpio, w);
    restore_interrupts(ints);
}

void ws2812_reset(int gpio)
{
    gpio_put(gpio, false);
    uint32_t ints = save_and_disable_interrupts();
    busy_wait_us(52);
    restore_interrupts(ints);
}

void ws2812_init(int gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    ws2812_reset(gpio);
}

void internal_ws2812_init()
{
#ifdef PICO_DEFAULT_WS2812_PIN
    ws2812_init(PICO_DEFAULT_WS2812_PIN);
#endif
}

void internal_ws2812_reset()
{
#ifdef PICO_DEFAULT_WS2812_PIN
    ws2812_reset(PICO_DEFAULT_WS2812_PIN);
#endif
}

void internal_ws2812_transmit(uint8_t r, uint8_t g, uint8_t b)
{
#ifdef PICO_DEFAULT_WS2812_PIN
    ws2812_transmit(PICO_DEFAULT_WS2812_PIN, r, g, b);
#else
    (void)r;
    (void)g;
    (void)b;
#endif
}

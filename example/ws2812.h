#pragma once

#include <stdint.h>

void ws2812_transmit(int gpio, uint8_t r, uint8_t g, uint8_t b);
void ws2812_transmit_4(int gpio, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void ws2812_transmit_color(int gpio, uint8_t data);
void ws2812_reset(int gpio);
void ws2812_init(int gpio);

void internal_ws2812_init();
void internal_ws2812_reset();
void internal_ws2812_transmit(uint8_t r, uint8_t g, uint8_t b);

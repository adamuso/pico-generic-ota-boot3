#pragma once

#include <stdint.h>

typedef struct RGBColor
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGBColor;

static inline struct RGBColor rgb_from_uint32(uint32_t color)
{
	return (struct RGBColor) {
		.r = (color & 0xff0000) >> 16,
		.g = (color & 0xff00) >> 8,
		.b = (color & 0xff),
	};
}

RGBColor hsv2rgb(float H, float S, float V);

void ws2812_transmit(int gpio, uint8_t r, uint8_t g, uint8_t b);
void ws2812_transmit_4(int gpio, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void ws2812_transmit_color(int gpio, uint8_t data);
void ws2812_reset(int gpio);
void ws2812_init(int gpio);

void internal_ws2812_init();
void internal_ws2812_reset();
void internal_ws2812_transmit(uint8_t r, uint8_t g, uint8_t b);

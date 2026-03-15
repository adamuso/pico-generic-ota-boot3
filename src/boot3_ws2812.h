#pragma once

#include <stdint.h>

#include "boot3_internal.h"

void boot3_ws2812_transmit(int gpio, uint8_t r, uint8_t g, uint8_t b);
void boot3_ws2812_transmit_color(int gpio, uint8_t data);
void boot3_ws2812_reset(int gpio);

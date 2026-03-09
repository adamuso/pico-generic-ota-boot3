#pragma once
#define PICO_RP2040 1

#include <stdint.h>
#include <stddef.h>

void boot3_flash_range_erase(uint32_t flash_offs, size_t count);
void boot3_flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);

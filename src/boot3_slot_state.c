/* Copyright (c) 2026 Adam Ogiba - Licensed under MIT */

// TODO: Somehow get this from project config instead of hardcoding it here
#define PICO_RP2040 1

#include "hardware/flash.h"

#if BOOT3_CLEAR_PENDING_STATE
const uint8_t boot3_slot_state_clear[FLASH_SECTOR_SIZE * 2] __attribute__((section(".boot3_slot_state"))) = { [0 ... FLASH_SECTOR_SIZE * 2 - 1] = 0xFF };
#endif

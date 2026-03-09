#pragma once

#include <assert.h>
#include <stdint.h>

#include "pico.h"

#define in_boot3_section __attribute__((section(".boot3")))
#define in_boot3_data __attribute__((section(".boot3.data")))
#define in_boot3_critical_section __attribute__((section(".boot3.critical")))

#define BOOT3_STATE_MAGIC 0x553007B0

extern char __boot3_end;
extern char __boot3_critical;
extern char __boot3_critical_end;
extern char __boot3_ram_start;

/// @brief Prelude of the boot3 state, which contains a magic value and checksum to validate the state, and distinguish between different versions of the state struct
struct Boot3StatePrelude
{
    uint64_t checksum;
    uint32_t program_size; 
    // A magic value to indicate the state is valid, and to distinguish between different versions of the state struct
    uint32_t magic; 
};

struct Boot3StateConfig
{
    uint8_t* flash_binary_start;
    uint8_t* flash_binary_end;
};

struct Boot3StateCopyProgress
{
    // Each byte represents 4096 of flash. 2048 bytes * 4096 = 8MiB, which is the half of max flash size of the RP2040, 
    // so this is enough to track progress of copying out the whole flash.
    char data[2048];
};

struct Boot3StateData
{
    struct Boot3StateConfig config;
    char padding[2048 - sizeof(struct Boot3StateConfig) - sizeof(struct Boot3StatePrelude)];
    struct Boot3StateCopyProgress progress;
};

struct Boot3State 
{
    struct Boot3StatePrelude prelude; 
    struct Boot3StateData data;
};

static_assert(sizeof(struct Boot3State) == 4096, "Boot3State must be exactly 4096 bytes to fit in the reserved space at the end of the boot3 section");

static inline struct Boot3State *boot3_get_current_state() {
    return (struct Boot3State *)(&__boot3_end - sizeof(struct Boot3State));
}

static inline struct Boot3State *boot3_get_pending_state() {
    return (struct Boot3State *)(&__boot3_end - sizeof(struct Boot3State) + (PICO_FLASH_SIZE_BYTES / 2));
}

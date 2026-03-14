#pragma once

#include <assert.h>
#include <stdint.h>

#include "pico.h"

#define in_boot3_section __attribute__((section(".boot3")))
#define in_boot3_data __attribute__((section(".boot3.data")))
#define in_boot3_bss __attribute__((section(".boot3.bss")))
#define in_boot3_critical_section __attribute__((section(".boot3.critical")))
#define in_boot3_user_data __attribute__((section(".boot3_state.user_data")))

#define BOOT3_STATE_MAGIC 0x553007B0

extern char __boot3_end;
extern char __boot3_critical;
extern char __boot3_critical_end;
extern char __boot3_ram_start;
extern char __boot3_bss;
extern char __boot3_bss_end;

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
    const char data[2048];
};

struct Boot3StateInternalData
{
    struct Boot3StateConfig config;
    char padding[1024 - sizeof(struct Boot3StateConfig) - sizeof(struct Boot3StatePrelude)];
};

static_assert(
    sizeof(struct Boot3StatePrelude) + sizeof(struct Boot3StateInternalData) == 1024, 
    "Boot3StatePrelude + Boot3StateInternalData must be exactly 1024 bytes to fit in the reserved space at the end of the boot3 section"
);

struct Boot3State 
{
    struct Boot3StatePrelude prelude; 
    struct Boot3StateInternalData data;
    char user_data[1024];
    struct Boot3StateCopyProgress progress;
};

static_assert(sizeof(struct Boot3State) == 4096, "Boot3State must be exactly 4096 bytes to fit in the reserved space at the end of the boot3 section");

static inline const struct Boot3State *boot3_get_current_state() {
    return (const struct Boot3State *)(&__boot3_end - sizeof(struct Boot3State));
}

static inline const struct Boot3State *boot3_get_pending_state() {
    return (const struct Boot3State *)(&__boot3_end - sizeof(struct Boot3State) + (PICO_FLASH_SIZE_BYTES / 2));
}

uint64_t in_boot3_section boot3_fnv1a_64(const uint8_t *data, size_t len);

// API
bool boot3_flash_erase_pending_data(size_t len);
void boot3_flash_program_pending_data(size_t offset, const uint8_t* data, size_t len);
bool boot3_validate_state(const struct Boot3State* state);

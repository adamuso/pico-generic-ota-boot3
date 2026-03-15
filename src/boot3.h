/* Copyright (c) 2026 Adam Ogiba - Licensed under MIT */
#pragma once

#include <assert.h>
#include <stdint.h>

#include "pico.h"

#define in_boot3_user_data __attribute__((section(".boot3_state.user_data")))

#define BOOT3_STATE_MAGIC 0x553007B0

extern char __boot3_end;

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
    bool (*should_update)(bool checksum_mismatch);
    uint64_t (*fnv1a_64)(const uint8_t *data, size_t len);
    bool (*should_recover)(bool checksum_mismatch);
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

static inline uint64_t boot3_fnv1a_64(const uint8_t *data, size_t len)
{
    return boot3_get_current_state()->data.config.fnv1a_64(data, len);
}

/// @brief Erase the flash range for the pending state. The pending state is stored at the second half of the flash, 
/// so the offset to the second half of the flash is added automatically. The caller just needs to provide the length
/// of the pending program, and the function will erase the corresponding flash range for the pending state, starting 
/// from the offset of the pending state. The caller should erase whole flash for the program in one go and do not split
/// it into multiple calls, so the `len` should be the total length of the pending program.
/// @param len The length of the whole pending program
/// @return true if the flash erase is successful, false otherwise
bool boot3_flash_erase_pending_data(size_t len);

/// @brief Program the pending state data to flash. The pending state is stored at the second half of the flash, the offset 
/// to the second half of the flash is added automatically. When programming the pending state, the caller should ensure 
/// that the program is provided as whole program with boot2 and boot3 included, so that when the pending state is applied, 
/// it will address the parts of the flash correctly.
///
/// Flash must be programmed in pages (256 bytes), so the `len` must be a multiple of flash page size.
/// @param offset The offset within the pending state region to start programming
/// @param data The data to program
/// @param len The length of the data to program
void boot3_flash_program_pending_data(size_t offset, const uint8_t* data, size_t len);

/// @brief Validate the boot3 state by checking the magic value, program size, and checksum. The checksum is calculated based 
/// on the program data in flash, so it can be used to verify the integrity of the program data, and also to distinguish between 
/// a pending state that has been applied but not cleared, and a pending state that has never been applied (since they will have different checksums).
/// @param state The boot3 state to validate
/// @return true if the state is valid, false otherwise
bool boot3_validate_state(const struct Boot3State* state);

/// @brief Determine if the program should be updated. This is a weak function that can be overridden by the user to provide 
/// custom logic for determining whether to apply the pending state. When no function is provided by the user, the default behavior
/// is to update whenever there is a checksum mismatch between the current and pending state. 
/// @param checksum_mismatch Indicates if there is a checksum mismatch between the current and pending state
/// @return true if the program should be updated, false otherwise
bool __attribute__((weak)) boot3_should_update(bool checksum_mismatch);

/// @brief Determine if the current state should be recovered from the pending state. This is a weak function that can be overridden 
/// by the user to provide custom logic for determining whether to recover the current state from the pending state when the current 
/// state is invalid. Current state is invalid when it has an invalid magic value, or its checksum does not match the program data in flash. 
/// When no function is provided by the user, the default behavior is to recover whenever there is a mismatch between checksum saved
/// in the current state and the checksum calculated from actual program data in flash.
/// @param checksum_mismatch Indicates if there is a checksum mismatch between the current and pending state
/// @return true if the current state should be recovered, false otherwise
bool __attribute__((weak)) boot3_should_recover(bool checksum_mismatch);

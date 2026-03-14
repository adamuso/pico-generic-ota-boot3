#include <stdint.h>

#include "boot3.h"
#include "boot3_internal.h"

extern char __flash_binary_start, __flash_binary_end;

const uint64_t boot3_checksum __attribute__((section(".boot3_state.checksum"))) = 0;
const uint32_t boot3_program_size __attribute__((section(".boot3_state.program_size"))) = 0xFFFFFFFF;
const uint32_t boot3_magic __attribute__((section(".boot3_state.magic"))) = BOOT3_STATE_MAGIC; 

struct Boot3StateInternalData boot3_current_state_data __attribute__((section(".boot3_state.data"))) = {
    .config = {
        .flash_binary_start = (uint8_t *)&__flash_binary_start,
        .flash_binary_end = (uint8_t *)&__flash_binary_end,
        .should_update = &boot3_should_update,
        .fnv1a_64 = &boot3_fnv1a_64_internal,
    },
};

struct Boot3StateCopyProgress boot3_current_progress __attribute__((section(".boot3_state.progress"))) = {
    // Initialize progress to all 0xFF, which is the erased state of flash, to distinguish from a state that has been written with valid progress data.
    .data = { [0 ... sizeof(struct Boot3StateCopyProgress) - 1] = 0xFF }
};
 
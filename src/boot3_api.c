#include <stddef.h>
#include <stdint.h>

#include "hardware/flash.h"

#include "boot3.h"

bool boot3_flash_erase_pending_data(size_t len)
{
    if (len == 0 || len > (PICO_FLASH_SIZE_BYTES / 2)) {
        return false;
    }

    size_t sectors = (len - 1) / FLASH_SECTOR_SIZE + 1;

    flash_range_erase(PICO_FLASH_SIZE_BYTES / 2, sectors * FLASH_SECTOR_SIZE);
    return true;
}

void boot3_flash_program_pending_data(size_t offset, const uint8_t* data, size_t len) 
{
    // We want to program flash pending state. The pending state starts at the middle of the flash, 
    // so we need to add the offset of the pending state to the flash offset when programming.    
    flash_range_program((PICO_FLASH_SIZE_BYTES / 2) + offset, data, len);
}

bool boot3_validate_state(const struct Boot3State* state) 
{
    return state->prelude.magic == BOOT3_STATE_MAGIC && 
        state->prelude.program_size <= (uint32_t)(PICO_FLASH_SIZE_BYTES / 2) - 8192 &&
        state->prelude.program_size > 0 && 
        state->prelude.checksum == boot3_fnv1a_64(
            (uint8_t *)&__boot3_end + (PICO_FLASH_SIZE_BYTES / 2), state->prelude.program_size
        );
}

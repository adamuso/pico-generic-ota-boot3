// TODO: Somehow get this from project config instead of hardcoding it here
#define PICO_RP2040 1

#include "boot3.h"
#include "boot3_internal.h"

#include <stddef.h>

#include "hardware/regs/addressmap.h"
#include "hardware/regs/m0plus.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/resets.h"
#include "hardware/sync.h"
#include "hardware/structs/resets.h"
#include "pico/platform/compiler.h"

#define FNV1A_64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV1A_64_PRIME        0x00000100000001b3ULL

extern void boot3_flash_range_erase(uint32_t flash_offs, size_t count);
extern void boot3_flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);

uint64_t in_boot3_section boot3_internal_fnv1a_64(const uint8_t *data, size_t len) {
    uint64_t hash = FNV1A_64_OFFSET_BASIS;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= FNV1A_64_PRIME;
    }
    return hash;
}

/// @brief Copy out the critical section of boot3 to SRAM, and ensure all memory accesses are complete before returning
static void in_boot3_section boot3_internal_copyout() 
{
    const volatile uint32_t *copy_from_start = (uint32_t *)&__boot3_critical;
    const volatile uint32_t *copy_from_end = (uint32_t *)&__boot3_critical_end;
    const size_t len = copy_from_end - copy_from_start;

    volatile uint32_t *copy_to = (uint32_t *)&__boot3_ram_start;

    for (size_t i = 0; i < len; ++i)
        copy_to[i] = copy_from_start[i];

    const volatile uint32_t *copy_from_bss_start = (uint32_t *)&__boot3_bss;
    const volatile uint32_t *copy_from_bss_end = (uint32_t *)&__boot3_bss_end;
    const size_t bss_len = copy_from_bss_end - copy_from_bss_start;

    for (size_t i = 0; i < bss_len; ++i)
        copy_to[len + i] = 0;

    __compiler_memory_barrier();
}

static void in_boot3_critical_section boot3_internal_exit()
{
    uint32_t vector_base = (uint32_t)&__boot3_end;

    volatile uint32_t *vtor = (uint32_t *)(PPB_BASE + M0PLUS_VTOR_OFFSET);
    *vtor = vector_base;

    uint32_t initial_sp = *((uint32_t *)vector_base);
    uint32_t reset_handler = *((uint32_t *)(vector_base + 4));

    // Set MSP to the new initial SP
    __asm__ volatile ("msr msp, %0" :: "r" (initial_sp) : );

    // Branch to reset handler (Thumb bit should be set in the vector table)
    __asm__ volatile ("bx %0" :: "r" (reset_handler) : );

    for(;;) { }
}

static void in_boot3_critical_section boot3_internal_memcpy(const volatile uint8_t *src, uint8_t *dst, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        dst[i] = src[i];
    }
}

static in_boot3_bss uint8_t buffer[FLASH_PAGE_SIZE] = { 0 };
static in_boot3_bss uint8_t progress_buffer[FLASH_SECTOR_SIZE / 2] = { 0 };

static void in_boot3_critical_section boot3_internal_copy_pending_to_current_and_exit()
{
    // This function assumes that the pending state has already been validated and the checksum has been verified to match the pending program,
    // so it doesn't re-validate the pending state, and just directly copies the pending state program to the first half of the flash where the 
    // current state is located, to apply the pending state.

    const struct Boot3State* pending_state = boot3_get_pending_state();

    uint32_t ints = save_and_disable_interrupts();
    uint32_t flash_to_erase = (pending_state->prelude.program_size - 1) / FLASH_SECTOR_SIZE + 1; // Round up to nearest flash sector

    // Erase the current state by erasing the first half of the flash.
    boot3_flash_range_erase(8192, flash_to_erase * FLASH_SECTOR_SIZE);
    
    const uint8_t* pending_program = (const uint8_t *)XIP_BASE + PICO_FLASH_SIZE_BYTES / 2 + FLASH_SECTOR_SIZE * 2; 

    // Copy pending flash binary to the first half of the flash, where the current state is located.
    // We will track the copy progress in the progress struct of the state, and update it after copying each chunk, 
    // so that if power is lost in the middle of the copy, we can resume from the last updated progress when power is back.
    // Without needing to re-copy from the beginning.
        volatile uint8_t *progress_buffer_volatile = (volatile uint8_t *)progress_buffer;

    for (size_t i = 0; i < sizeof(progress_buffer); ++i) {
        progress_buffer_volatile[i] = 0xff;
    }

    int toggle = 0;

    for (size_t i = 0; i < pending_state->prelude.program_size; i += sizeof(buffer)) {
        // Copy a chunk of the pending program from flash to RAM buffer
        // We can assume that all programs are aligned to 256 flash pages
        boot3_internal_memcpy(pending_program + i, buffer, sizeof(buffer));

        // Program the chunk (always a full page size when passed to the flash helper)
        boot3_flash_range_program(FLASH_SECTOR_SIZE * 2 + i, buffer, sizeof(buffer));

        if (i % FLASH_SECTOR_SIZE == 0) 
        {
            gpio_put(8, toggle);
            toggle = !toggle;
        }

        // Update copy progress in flash. Each byte in progress represents 4096 bytes of flash, so we update it after copying each 4096 chunk of the program.
        if (i % FLASH_SECTOR_SIZE == 0) {
            // Copy current progress from flash to RAM buffer, update the progress byte, 
            // then program it back to flash, to avoid erasing the whole progress area 
            // for each update which would cause unnecessary wear on flash and also lose the whole progress history in case of power loss.
            size_t current_byte = i / FLASH_SECTOR_SIZE;
            size_t current_page_offset = current_byte / FLASH_PAGE_SIZE * FLASH_PAGE_SIZE; 

            progress_buffer[current_byte] = 0;
            // boot3_memcpy(&current_state->progress.data[current_page_offset], buffer, sizeof(buffer));
            // buffer[current_byte % FLASH_PAGE_SIZE] = 0;

            boot3_flash_range_program(
                FLASH_SECTOR_SIZE + offsetof(struct Boot3State, progress) + current_page_offset, 
                &progress_buffer[current_page_offset], 
                FLASH_PAGE_SIZE
            );
        }
    }

    volatile const uint8_t* pending_state_bytes = (volatile const uint8_t*)pending_state;

    // Erase current_state area and copy new state from pending_state. This is safe even if the power is lost in the middle of the process.
    // When power is back, if the state copy is not complete, the pending state will still be valid (since we haven't erased it), but the checksum of
    // current_state won't match, this will cause retry of the copy process until it is complete. 
    boot3_flash_range_erase(FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);

    gpio_put(7, 0);

    for (size_t i = 0; i < sizeof(struct Boot3State); i += sizeof(buffer)) {
        // Copy a chunk of the pending state from flash to RAM buffer
        boot3_internal_memcpy(pending_state_bytes + i, buffer, sizeof(buffer));

        // Program the chunk from RAM buffer to the 4096 offset of flash, which is the location of the current state
        boot3_flash_range_program(4096 + i, buffer, sizeof(buffer));
    }

    gpio_put(7, 1);

#if 0 // TODO: Should we allow for repropgramming our boot3 bootloader when new program is flashed?
    boot3_flash_range_erase(0, 8192);

    // Currently we are in state that does not have boot2 and boot3 flash. Firstly we need to copy them
    // from pending state to first half of the flash.
    for (size_t i = 0; i < 8192; i += sizeof(buffer)) {
        size_t chunk_size = 8192 - i;
        if (chunk_size > sizeof(buffer)) {
            chunk_size = sizeof(buffer);
        }

        // Copy a chunk of the pending program from flash to RAM buffer
        for (size_t j = 0; j < chunk_size; ++j) {
            buffer[j] = pending_source[i + j];
        }

        // Program the chunk from RAM buffer to the first half of flash
        boot3_flash_range_program(i, buffer, chunk_size);
    }
#endif

    restore_interrupts(ints);

    gpio_put(8, 0);

    boot3_internal_exit();
}

void in_boot3_section boot3_internal_check_state()
{
    const struct Boot3State* current_state = boot3_get_current_state();
    const struct Boot3State* pending_state = boot3_get_pending_state();

    gpio_put(7, 1);

    // When current state is invalid, we need to try recover from the second slot if possible
    bool current_state_valid = current_state->prelude.magic == BOOT3_STATE_MAGIC && 
        current_state->prelude.checksum == boot3_internal_fnv1a_64(&__boot3_end, current_state->prelude.program_size);

    // We should update when current and pending state checksum mismatch
    bool should_update = current_state->prelude.checksum != pending_state->prelude.checksum;
        
    // But we allow user to override this logic with the should_update callback in the state config, 
    // which can implement custom logic to decide whether to update when there is a checksum mismatch, 
    // for example based on some user data in the state, or based on other conditions like battery level, etc.
    if (current_state_valid && current_state->data.config.should_update != NULL) 
    {
        should_update = current_state->data.config.should_update(should_update);
    }

    // Pending state is valid when:
    // - It has the correct magic value
    // - Its program size is not larger than half of the flash size (since we store the pending state 
    //   at the end of the flash, it can't represent a program larger than half of the flash)
    // - By default: its checksum is different from the current state's checksum (to distinguish between a pending that has been 
    //   applied but not cleared, and a pending state that has never been applied)
    //   User override: the should_update callback can override the default checksum mismatch logic
    //   OR
    //   The current state is invalid (to allow recovery from a corrupted current state when the pending state is valid)
    if (
        pending_state->prelude.magic == BOOT3_STATE_MAGIC && 
        pending_state->prelude.program_size <= (uint32_t)(PICO_FLASH_SIZE_BYTES / 2) - 8192 &&
        pending_state->prelude.program_size > 0 && 
        (should_update || !current_state_valid)
    ) {
        // When pending state is valid, calculate the checksum of the pending state program, 
        // and if it matches the checksum in the prelude, copy the pending state to the current state to apply it.
        if (
            boot3_internal_fnv1a_64( 
                (uint8_t *)&__boot3_end + (PICO_FLASH_SIZE_BYTES / 2), pending_state->prelude.program_size
            ) == pending_state->prelude.checksum
        ) {
            gpio_put(8, 1);

            boot3_internal_copy_pending_to_current_and_exit();
            for (;;);
        }
    }

    gpio_put(7, 0);
}

void in_boot3_section boot3_gpio_set_function(uint gpio, gpio_function_t fn) {
    check_gpio_param(gpio);
    invalid_params_if(HARDWARE_GPIO, ((uint32_t)fn << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB) & ~IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS);
    // Set input enable on, output disable off
    hw_write_masked(&pads_bank0_hw->io[gpio],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
    );
    // Zero all fields apart from fsel; we want this IO to do what the peripheral tells it.
    // This doesn't affect e.g. pullup/pulldown, as these are in pad controls.
    io_bank0_hw->io[gpio].ctrl = fn << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
#if HAS_PADS_BANK0_ISOLATION
    // Remove pad isolation now that the correct peripheral is in control of the pad
    hw_clear_bits(&pads_bank0_hw->io[gpio], PADS_BANK0_GPIO0_ISO_BITS);
#endif
}

void in_boot3_section boot3_main(void) 
{
    boot3_internal_copyout();

    reset_unreset_block_num_wait_blocking(RESET_IO_BANK0);
    reset_unreset_block_num_wait_blocking(RESET_PADS_BANK0);

    // TODO: Add defines for the GPIOs we use for progress indication, instead of hardcoding 7 and 8 here.
    // Also allow to disable them when user does not want that.
    boot3_gpio_set_function(7, GPIO_FUNC_SIO);
    gpio_set_dir(7, GPIO_OUT);
    boot3_gpio_set_function(8, GPIO_FUNC_SIO);
    gpio_set_dir(8, GPIO_OUT);

    boot3_internal_check_state();
    
    boot3_internal_exit();
}

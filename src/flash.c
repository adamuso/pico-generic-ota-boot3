#include "flash.h"

#include "hardware/gpio.h"
#include "pico/assert.h"
#include "pico/bootrom.h"
#include "hardware/flash.h"
#include "hardware/structs/pads_qspi.h"
#include "hardware/xip_cache.h"

#include "sections.h"
#include "boot3.h"
#include "boot3_internal.h"

#define PICO_RP2040 1
#define FLASH_BLOCK_ERASE_CMD 0xd8

typedef struct flash_hardware_save_state {
#if !PICO_RP2040
    flash_rp2350_qmi_save_state_t qmi_save;
#endif
    uint32_t qspi_pads[count_of(pads_qspi_hw->io)];
} flash_hardware_save_state_t;


#define BOOT2_SIZE_WORDS 64

static in_boot3_bss uint32_t boot2_copyout[BOOT2_SIZE_WORDS] = { 0 };
static in_boot3_bss bool boot2_copyout_valid = false;

static void __no_inline_not_in_flash_func(flash_init_boot2_copyout)(void) {
    if (boot2_copyout_valid)
        return;
    // todo we may want the option of boot2 just being a free function in
    //      user RAM, e.g. if it is larger than 256 bytes
#if PICO_RP2040
    const volatile uint32_t *copy_from = (uint32_t *)XIP_BASE;
#else
    const volatile uint32_t *copy_from = (uint32_t *)BOOTRAM_BASE;
#endif
    for (int i = 0; i < BOOT2_SIZE_WORDS; ++i)
        boot2_copyout[i] = copy_from[i];
    __compiler_memory_barrier();
    boot2_copyout_valid = true;
}

static void __no_inline_not_in_flash_func(flash_enable_xip_via_boot2)(void) {
    ((void (*)(void))((intptr_t)boot2_copyout+1))();
}

static void __no_inline_not_in_flash_func(flash_save_hardware_state)(flash_hardware_save_state_t *state) {
    // Commit any pending writes to external RAM, to avoid losing them in a subsequent flush:
    xip_cache_clean_all();
    for (size_t i = 0; i < count_of(pads_qspi_hw->io); ++i) {
        state->qspi_pads[i] = pads_qspi_hw->io[i];
    }
#if !PICO_RP2040
    flash_rp2350_save_qmi_cs1(&state->qmi_save);
#endif
}

static void __no_inline_not_in_flash_func(flash_restore_hardware_state)(flash_hardware_save_state_t *state) {
    for (size_t i = 0; i < count_of(pads_qspi_hw->io); ++i) {
        pads_qspi_hw->io[i] = state->qspi_pads[i];
    }
#if !PICO_RP2040
    // Tail call!
    flash_rp2350_restore_qmi_cs1(&state->qmi_save);
#endif
}

void __no_inline_not_in_flash_func(boot3_flash_range_erase)(uint32_t flash_offs, size_t count) {
#ifdef PICO_FLASH_SIZE_BYTES
    hard_assert(flash_offs + count <= PICO_FLASH_SIZE_BYTES);
#endif
    invalid_params_if(HARDWARE_FLASH, flash_offs & (FLASH_SECTOR_SIZE - 1));
    invalid_params_if(HARDWARE_FLASH, count & (FLASH_SECTOR_SIZE - 1));

    rom_connect_internal_flash_fn connect_internal_flash_func = (rom_connect_internal_flash_fn)rom_func_lookup_inline(ROM_FUNC_CONNECT_INTERNAL_FLASH);
    rom_flash_exit_xip_fn flash_exit_xip_func = (rom_flash_exit_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP);
    rom_flash_range_erase_fn flash_range_erase_func = (rom_flash_range_erase_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_RANGE_ERASE);
    rom_flash_flush_cache_fn flash_flush_cache_func = (rom_flash_flush_cache_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_FLUSH_CACHE);
    assert(connect_internal_flash_func && flash_exit_xip_func && flash_range_erase_func && flash_flush_cache_func);
    flash_init_boot2_copyout();
    flash_hardware_save_state_t state;
    flash_save_hardware_state(&state);

    // No flash accesses after this point
    __compiler_memory_barrier();

    connect_internal_flash_func();
    flash_exit_xip_func();
    flash_range_erase_func(flash_offs, count, FLASH_BLOCK_SIZE, FLASH_BLOCK_ERASE_CMD);
    flash_flush_cache_func(); // Note this is needed to remove CSn IO force as well as cache flushing
    flash_enable_xip_via_boot2();
    flash_restore_hardware_state(&state);
}

void __no_inline_not_in_flash_func(boot3_flash_range_program)(uint32_t flash_offs, const uint8_t *data, size_t count) {
#ifdef PICO_FLASH_SIZE_BYTES
    hard_assert(flash_offs + count <= PICO_FLASH_SIZE_BYTES);
#endif
    invalid_params_if(HARDWARE_FLASH, flash_offs & (FLASH_PAGE_SIZE - 1));
    invalid_params_if(HARDWARE_FLASH, count & (FLASH_PAGE_SIZE - 1));
    rom_connect_internal_flash_fn connect_internal_flash_func = (rom_connect_internal_flash_fn)rom_func_lookup_inline(ROM_FUNC_CONNECT_INTERNAL_FLASH);
    rom_flash_exit_xip_fn flash_exit_xip_func = (rom_flash_exit_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP);
    rom_flash_range_program_fn flash_range_program_func = (rom_flash_range_program_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_RANGE_PROGRAM);
    rom_flash_flush_cache_fn flash_flush_cache_func = (rom_flash_flush_cache_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_FLUSH_CACHE);
    assert(connect_internal_flash_func && flash_exit_xip_func && flash_range_program_func && flash_flush_cache_func);
    flash_init_boot2_copyout();
    flash_hardware_save_state_t state;
    flash_save_hardware_state(&state);

    __compiler_memory_barrier();

    connect_internal_flash_func();
    flash_exit_xip_func();
    flash_range_program_func(flash_offs, data, count);
    flash_flush_cache_func(); // Note this is needed to remove CSn IO force as well as cache flushing
    flash_enable_xip_via_boot2();

    flash_restore_hardware_state(&state);
}

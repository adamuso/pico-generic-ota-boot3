#define PICO_RP2040 1
#include "boot3.h"

#include <stddef.h>

#include "hardware/regs/addressmap.h"
#include "hardware/regs/m0plus.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/resets.h"
#include "hardware/structs/resets.h"
#include "pico/platform/compiler.h"
#include "flash.h"

#define FNV1A_64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV1A_64_PRIME        0x00000100000001b3ULL

extern char __flash_binary_start, __flash_binary_end;

const uint64_t boot3_checksum __attribute__((section(".boot3_state.checksum"))) = 0;
const uint32_t boot3_program_size __attribute__((section(".boot3_state.program_size"))) = 0xFFFFFFFF;
const uint32_t boot3_magic __attribute__((section(".boot3_state.magic"))) = BOOT3_STATE_MAGIC; 

struct Boot3StateData boot3_current_state_data __attribute__((section(".boot3_state.data"))) = {
    .config = {
        .flash_binary_start = (uint8_t *)&__flash_binary_start,
        .flash_binary_end = (uint8_t *)&__flash_binary_end,
    },
    // Initialize progress to all 0xFF, which is the erased state of flash, to distinguish from a state that has been written with valid progress data.
    .progress = {
        .data = { [0 ... sizeof(struct Boot3StateCopyProgress) - 1] = 0xFF }
     }, 
};
 
/// @brief Copy out the critical section of boot3 to SRAM, and ensure all memory accesses are complete before returning
void in_boot3_section boot3_copyout() 
{
    const volatile uint32_t *copy_from_start = (uint32_t *)&__boot3_critical;
    const volatile uint32_t *copy_from_end = (uint32_t *)&__boot3_critical_end;
    const size_t len = (copy_from_end - copy_from_start) / sizeof(uint32_t);

    volatile uint32_t *copy_to = (uint32_t *)&__boot3_ram_start;

    for (size_t i = 0; i < len; ++i)
        copy_to[i] = copy_from_start[i];

    __compiler_memory_barrier();
}

void in_boot3_section boot3_exit()
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

uint64_t in_boot3_section fnv1a_64(const uint8_t *data, size_t len) {
    uint64_t hash = FNV1A_64_OFFSET_BASIS;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= FNV1A_64_PRIME;
    }
    return hash;
}

void in_boot3_section boot3_check_state()
{
    struct Boot3State* current_state = boot3_get_current_state();
    struct Boot3State* pending_state = boot3_get_pending_state();

    if (pending_state->prelude.magic == BOOT3_STATE_MAGIC && current_state->prelude.checksum != pending_state->prelude.checksum)
    {
        // Handle valid pending state
    }

    if (
        current_state->prelude.magic == BOOT3_STATE_MAGIC && 
        current_state->prelude.checksum != fnv1a_64(&__boot3_end, current_state->prelude.program_size)
    ) {
        gpio_put(7, 1);
    }
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
    boot3_copyout();

    reset_unreset_block_num_wait_blocking(RESET_IO_BANK0);
    reset_unreset_block_num_wait_blocking(RESET_PADS_BANK0);

    boot3_gpio_set_function(7, GPIO_FUNC_SIO);
    gpio_set_dir(7, GPIO_OUT);
    boot3_gpio_set_function(8, GPIO_FUNC_SIO);
    gpio_set_dir(8, GPIO_OUT);

    boot3_check_state();
    
    busy_wait_at_least_cycles(SYS_CLK_HZ / 10);    
    
    boot3_exit();
}

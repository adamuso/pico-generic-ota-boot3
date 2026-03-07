#include "hardware/regs/addressmap.h"
#include "hardware/regs/m0plus.h"
#include "hardware/flash.h"

#include <stdint.h>

extern char __boot3_end;

// Place this function in the .boot3 section so it lives alongside the
// assembler entry.
void boot3_main(void) __attribute__((noreturn, section(".boot3")));

void boot3_main(void) {
    flash_range_erase(0, 1);

    // Compute vector base: XIP_BASE + 0x100 + size of boot3
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

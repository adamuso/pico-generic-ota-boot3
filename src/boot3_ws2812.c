// TODO: Somehow get this from project config instead of hardcoding it here
#define PICO_RP2040 1

#include <math.h>

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "hardware/timer.h"
#include "boot3_ws2812.h"

#if BOOT3_ENABLE_XOSC
void in_boot3_critical_section boot3_ws2812_transmit_color(int gpio, uint8_t data)
{
    // 12MHz
    // 1 cycle = 0.083us

    int mask = 1 << gpio;
    uint32_t moved_data = data << 24;

    asm volatile(
        ".syntax unified\n"

        // bit 8
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "bcs    8f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "8:\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        
        // bit 7
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "bcs    7f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "7:\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle

        // bit 6
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "bcs    6f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "6:\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle

        // bit 5
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "bcs    5f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "5:\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle

        // bit 4
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "bcs    4f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "4:\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle

        // bit 3
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "bcs    3f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "3:\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle


        // bit 2
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "bcs    2f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "2:\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle

        
        // bit 1
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "bcs    1f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "1:\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle
        : [v] "+l" (moved_data)
        : [sio] "r" (SIO_BASE), [set_off] "r" (SIO_GPIO_OUT_SET_OFFSET), [clr_off] "r" (SIO_GPIO_OUT_CLR_OFFSET), [mask] "r" (mask)
        : "cc"
    );
}
#else
void in_boot3_critical_section boot3_ws2812_transmit_color(int gpio, uint8_t data)
{
    int mask = 1 << gpio;
    uint32_t moved_data = data << 24;

    asm volatile(
        ".syntax unified\n"

        // bit 8
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "bcs    8f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "8:\n"
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        
        // bit 7
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "bcs    7f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "7:\n"
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle

        // bit 6
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "bcs    6f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "6:\n"
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle


        // bit 5
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "bcs    5f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "5:\n"
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle

        // bit 4
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "bcs    4f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "4:\n"
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle


        // bit 3
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "bcs    3f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "3:\n"
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle


        // bit 2
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "bcs    2f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "2:\n"
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle

        
        // bit 1
        "lsls %[v], %[v], #1\n"                         // 1 cycle: sets carry bit
        
        "str %[mask], [%[sio], %[set_off]]\n"       // 1 cycle
        "bcs    1f\n"                               // 1 cycle if not taken, 3 cycles if taken

        "str %[mask], [%[sio], %[clr_off]]\n"
        "nop\n"                                     // 1 cycle
        "nop\n"                                     // 1 cycle

        "1:\n"
        "str %[mask], [%[sio], %[clr_off]]\n"       // 1 cycle
        "nop\n"                                     // 1 cycle
        : [v] "+l" (moved_data)
        : [sio] "r" (SIO_BASE), [set_off] "r" (SIO_GPIO_OUT_SET_OFFSET), [clr_off] "r" (SIO_GPIO_OUT_CLR_OFFSET), [mask] "r" (mask)
        : "cc"
    );
}
#endif

void in_boot3_critical_section boot3_ws2812_transmit(int gpio, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t ints = save_and_disable_interrupts();
    boot3_ws2812_transmit_color(gpio, r);
    boot3_ws2812_transmit_color(gpio, g);
    boot3_ws2812_transmit_color(gpio, b);
    restore_interrupts(ints);
}

void in_boot3_critical_section boot3_ws2812_reset(int gpio)
{
    gpio_put(gpio, false);
    uint32_t ints = save_and_disable_interrupts();

    // 1 + 3 * 90 + 1 = 272 CPU cycles = 272 * 0.2us = 54.4us
    asm volatile(
        "mov  r0, #90\n"    	// 1 cycle
        "4: sub  r0, r0, #1\n"	// 1 cycle
        "bne   4b\n"          	// 2 cycles if loop taken, 1 if not
    );

    restore_interrupts(ints);
}

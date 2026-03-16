/*
 * Copyright (c) 2026 Adam Ogiba
 * SPDX-License-Identifier: MIT
 *
 * This file contains code derived from the Raspberry Pi Pico SDK
 * (hardware_xosc/xosc.c, hardware_clocks/clocks.c), which is:
 *   Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *   SPDX-License-Identifier: BSD-3-Clause
 * The derived portions are marked below.
 */

// TODO: Somehow get this from project config instead of hardcoding it here
#define PICO_RP2040 1

#include "hardware/clocks.h"
#include "hardware/xosc.h"

#include "boot3_internal.h"

#if XOSC_HZ < (1 * MHZ) || XOSC_HZ > (50 * MHZ)
// Note: Although an external clock can be supplied up to 50 MHz, the maximum frequency the
// XOSC cell is specified to work with a crystal is less, please see the appropriate RP-series datasheet.
#error XOSC_HZ must be in the range 1,000,000-50,000,000 i.e. 1-50MHz XOSC frequency
#endif

#define STARTUP_DELAY ((((XOSC_HZ / KHZ) + 128) / 256) * PICO_XOSC_STARTUP_DELAY_MULTIPLIER)

// The DELAY field in xosc_hw->startup is 14 bits wide.
#if STARTUP_DELAY >= (1 << 13)
#error PICO_XOSC_STARTUP_DELAY_MULTIPLIER is too large: XOSC STARTUP.DELAY must be < 8192 
#endif

void in_boot3_section boot3_xosc_init(void) {
    // Assumes 1-15 MHz input, checked above.
    xosc_hw->ctrl = XOSC_CTRL_FREQ_RANGE_VALUE_1_15MHZ;

    // Set xosc startup delay
    xosc_hw->startup = STARTUP_DELAY;

    // Set the enable bit now that we have set freq range and startup delay
    hw_set_bits(&xosc_hw->ctrl, XOSC_CTRL_ENABLE_VALUE_ENABLE << XOSC_CTRL_ENABLE_LSB);

    // Wait for XOSC to be stable
    while(!(xosc_hw->status & XOSC_STATUS_STABLE_BITS)) {
        tight_loop_contents();
    }
}

// Clock muxing consists of two components:
// - A glitchless mux, which can be switched freely, but whose inputs must be
//   free-running
// - An auxiliary (glitchy) mux, whose output glitches when switched, but has
//   no constraints on its inputs
// Not all clocks have both types of mux.
static __always_inline bool has_glitchless_mux(clock_handle_t clock) {
    return clock == clk_sys || clock == clk_ref;
}

static in_boot3_bss uint32_t boot3_configured_freq[CLK_COUNT];

static void in_boot3_section boot3_clock_configure_internal(clock_handle_t clock, uint32_t src, uint32_t auxsrc, uint32_t actual_freq, uint32_t div) {
    clock_hw_t *clock_hw = &clocks_hw->clk[clock];

    // If increasing divisor, set divisor before source. Otherwise set source
    // before divisor. This avoids a momentary overspeed when e.g. switching
    // to a faster source and increasing divisor to compensate.
    if (div > clock_hw->div)
        clock_hw->div = div;

    // If switching a glitchless slice (ref or sys) to an aux source, switch
    // away from aux *first* to avoid passing glitches when changing aux mux.
    // Assume (!!!) glitchless source 0 is no faster than the aux source.
    if (has_glitchless_mux(clock) && src == CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX) {
        hw_clear_bits(&clock_hw->ctrl, CLOCKS_CLK_REF_CTRL_SRC_BITS);
        while (!(clock_hw->selected & 1u))
            tight_loop_contents();
    }
    // If no glitchless mux, cleanly stop the clock to avoid glitches
    // propagating when changing aux mux. Note it would be a really bad idea
    // to do this on one of the glitchless clocks (clk_sys, clk_ref).
    else {
        // Disable clock. On clk_ref and clk_sys this does nothing,
        // all other clocks have the ENABLE bit in the same position.
        hw_clear_bits(&clock_hw->ctrl, CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS);
        if (boot3_configured_freq[clock] > 0) {
            // Delay for 3 cycles of the target clock, for ENABLE propagation.
            // Note XOSC_COUNT is not helpful here because XOSC is not
            // necessarily running, nor is timer...
            uint delay_cyc = boot3_configured_freq[clk_sys] / boot3_configured_freq[clock] + 1;
            busy_wait_at_least_cycles(delay_cyc * 3);
        }
    }

    // Set aux mux first, and then glitchless mux if this clock has one
    hw_write_masked(&clock_hw->ctrl,
        (auxsrc << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB),
        CLOCKS_CLK_SYS_CTRL_AUXSRC_BITS
    );

    if (has_glitchless_mux(clock)) {
        hw_write_masked(&clock_hw->ctrl,
            src << CLOCKS_CLK_REF_CTRL_SRC_LSB,
            CLOCKS_CLK_REF_CTRL_SRC_BITS
        );
        while (!(clock_hw->selected & (1u << src)))
            tight_loop_contents();
    }

    // Enable clock. On clk_ref and clk_sys this does nothing,
    // all other clocks have the ENABLE bit in the same position.
    hw_set_bits(&clock_hw->ctrl, CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS);

    // Now that the source is configured, we can trust that the user-supplied
    // divisor is a safe value.
    clock_hw->div = div;
    boot3_configured_freq[clock] = actual_freq;
}

bool in_boot3_section boot3_clock_configure_12mhz(clock_handle_t clock, uint32_t src, uint32_t auxsrc) {
    // assert(src_freq >= freq);

    // if (freq > src_freq)
    //     return false;

    // uint64_t div64 =((((uint64_t) src_freq) << CLOCKS_CLK_GPOUT0_DIV_INT_LSB) / freq);
    // uint32_t div, actual_freq;

    const uint32_t div = ((12000000u << CLOCKS_CLK_GPOUT0_DIV_INT_LSB) / 12000000);
  
    // if (div64 >> 32) {
    //     // set div to 0 for maximum clock divider
    //     div = 0;
    //     actual_freq = src_freq >> (32 - CLOCKS_CLK_GPOUT0_DIV_INT_LSB);
    // } else {
    //     div = (uint32_t) div64;
#if PICO_RP2040
        // on RP2040 only clock divider of 1, or  >= 2 are supported
        // if (div < (2u << CLOCKS_CLK_GPOUT0_DIV_INT_LSB)) {
        //     div = (1u << CLOCKS_CLK_GPOUT0_DIV_INT_LSB);
        // }

    const uint32_t div2 = div < (2u << CLOCKS_CLK_GPOUT0_DIV_INT_LSB) ? (1u << CLOCKS_CLK_GPOUT0_DIV_INT_LSB) : div;
#else
    const uint32_t div2 = div;
#endif
    //     actual_freq = (uint32_t) ((((uint64_t) src_freq) << CLOCKS_CLK_GPOUT0_DIV_INT_LSB) / div);
    // }


    const uint32_t actual_freq = (12000000u << CLOCKS_CLK_GPOUT0_DIV_INT_LSB) / div2;

    boot3_clock_configure_internal(clock, src, auxsrc, actual_freq, div2);
    // Store the configured frequency
    return true;
}


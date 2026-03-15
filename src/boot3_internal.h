/* Copyright (c) 2026 Adam Ogiba - Licensed under MIT */
#pragma once 

#include <stdint.h>
#include <stddef.h>

#if BOOT3_WS2812_ENABLE
    #include "boot3_ws2812.h"
#endif

#define in_boot3_section __attribute__((section(".boot3")))
#define in_boot3_data __attribute__((section(".boot3.data")))
#define in_boot3_bss __attribute__((section(".boot3.bss")))
#define in_boot3_critical_section __attribute__((section(".boot3.critical")))

extern char __boot3_critical;
extern char __boot3_critical_end;
extern char __boot3_ram_start;
extern char __boot3_bss;
extern char __boot3_bss_end;

#define BOOT3_ENABLE_XOSC 1

#if BOOT3_WS2812_ENABLE
    #ifdef BOOT3_WS2812_GPIO
        #define BOOT3_WS2812_GPIO_PIN BOOT3_WS2812_GPIO
    #elif defined(PICO_DEFAULT_WS2812_PIN)  
        #define BOOT3_WS2812_GPIO_PIN PICO_DEFAULT_WS2812_PIN
    #endif
#endif

#if BOOT3_STATUS_ENABLE
    #ifdef BOOT3_STATUS_LED_GPIO
        #define BOOT3_STATUS_LED_GPIO_PIN BOOT3_STATUS_LED_GPIO
    #elif defined(PICO_DEFAULT_LED_PIN)  
        #define BOOT3_STATUS_LED_GPIO_PIN PICO_DEFAULT_LED_PIN
    #endif

    #ifdef BOOT3_PROGRAM_LED_GPIO
        #define BOOT3_PROGRAM_LED_GPIO_PIN BOOT3_PROGRAM_LED_GPIO
    #elif defined(PICO_DEFAULT_LED_PIN)  
        #define BOOT3_PROGRAM_LED_GPIO_PIN PICO_DEFAULT_LED_PIN
    #endif
#endif

#if defined(BOOT3_STATUS_LED_GPIO_PIN) && defined(BOOT3_WS2812_GPIO_PIN)
    #define boot3_status_led_set(value) gpio_put(BOOT3_STATUS_LED_GPIO_PIN, value); \
        boot3_ws2812_reset(BOOT3_WS2812_GPIO_PIN); \
        boot3_ws2812_transmit(BOOT3_WS2812_GPIO_PIN, 0, 0, value ? 128 : 0)
#elif defined(BOOT3_WS2812_GPIO_PIN)
    #define boot3_status_led_set(value) boot3_ws2812_reset(BOOT3_WS2812_GPIO_PIN); \
        boot3_ws2812_transmit(BOOT3_WS2812_GPIO_PIN, 0, 0, value ? 128 : 0)
#elif defined(BOOT3_STATUS_LED_GPIO_PIN)
    #define boot3_status_led_set(value) gpio_put(BOOT3_STATUS_LED_GPIO_PIN, value)
#else
    #define boot3_status_led_set(value) (void)(value)
#endif

#if defined(BOOT3_PROGRAM_LED_GPIO_PIN) && defined(BOOT3_WS2812_GPIO_PIN)
    #define boot3_program_led_set(value) gpio_put(BOOT3_PROGRAM_LED_GPIO_PIN, value); \
        boot3_ws2812_reset(BOOT3_WS2812_GPIO_PIN); \
        boot3_ws2812_transmit(BOOT3_WS2812_GPIO_PIN, value ? 128 : 0, 0, 0)
#elif defined(BOOT3_WS2812_GPIO_PIN)
    #define boot3_program_led_set(value) boot3_ws2812_reset(BOOT3_WS2812_GPIO_PIN); \
        boot3_ws2812_transmit(BOOT3_WS2812_GPIO_PIN, value ? 128 : 0, 0, 0)
#elif defined(BOOT3_PROGRAM_LED_GPIO_PIN)
    #define boot3_program_led_set(value) gpio_put(BOOT3_PROGRAM_LED_GPIO_PIN, value)
#else
    #define boot3_program_led_set(value) (void)(value)
#endif

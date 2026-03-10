#include <math.h>
#include <stdio.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "ws2812.h"
#include "boot3.h"

extern char __flash_binary_start, __flash_binary_end;

int main(void) {
    internal_ws2812_init();

    internal_ws2812_reset();
    internal_ws2812_transmit(0, 0, 255);

    stdio_init_all();

    printf("Hello, world!\n");
    printf("Program checksum: %llx\n", boot3_get_current_state()->prelude.checksum);
    printf("Pointers: %p %p %d\n", &__flash_binary_start, &__boot3_end, &__flash_binary_start - &__boot3_end);
    printf("Calc checksum: %llx\n", boot3_fnv1a_64(&__boot3_end, boot3_get_current_state()->prelude.program_size));
    printf("Flash size: %u bytes\n", (uint32_t)(&__flash_binary_end - &__flash_binary_start));
    printf("Program size: %u bytes\n", boot3_get_current_state()->prelude.program_size);
    printf("Program magic: %x\n", boot3_get_current_state()->prelude.magic);

    // busy_wait_ms(2000);

    // for (size_t i = 0; i < 360; i++) {
    //     RGBColor color = hsv2rgb((float)i, 100, 10);
    //     internal_ws2812_reset();
    //     internal_ws2812_transmit(color.r, color.g, color.b);
    //     sleep_ms(15);
    // }

    busy_wait_ms(4000);
    stdio_getchar();

    internal_ws2812_reset();
    internal_ws2812_transmit(128, 0, 0);

    rom_reset_usb_boot(0, 0);

    for(;;) {
        tight_loop_contents();
    }

    return 0;
}

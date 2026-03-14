#include <math.h>
#include <stdio.h>
#include <malloc.h>

#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

#include "ws2812.h"
#include "boot3.h"

extern char __flash_binary_start, __flash_binary_end;

struct Boot3UserData
{
    uint32_t version;
};

struct Boot3UserData in_boot3_user_data boot3_user_data = {
    .version = 1,
};

bool boot3_should_update() 
{
    struct Boot3UserData* current = (struct Boot3UserData*)boot3_get_current_state()->user_data;
    struct Boot3UserData* pending = (struct Boot3UserData*)boot3_get_pending_state()->user_data;

    return current->version != pending->version;
}

int main(void) {
    internal_ws2812_init();

    internal_ws2812_reset();
    internal_ws2812_transmit(0, 0, 128);

    stdio_init_all();

    printf("Hello, world! 12\n");
    printf("Program checksum: %llx\n", boot3_get_current_state()->prelude.checksum);
    printf("Pending checksum: %llx\n", boot3_get_pending_state()->prelude.checksum);
    printf("Pointers: %p %p %d\n", &__flash_binary_start, &__boot3_end, &__flash_binary_start - &__boot3_end);
    printf("Calc checksum: %llx\n", boot3_fnv1a_64(&__boot3_end, boot3_get_current_state()->prelude.program_size));
    printf("Flash size: %u bytes\n", (uint32_t)(&__flash_binary_end - &__flash_binary_start));
    printf("Program size: %u bytes\n", boot3_get_current_state()->prelude.program_size);
    printf("Program magic: %x\n", boot3_get_current_state()->prelude.magic);
    printf("Is pending state valid: %d\n", boot3_validate_state(boot3_get_pending_state()));

    for (int i = 0; i < 2048; i++) {
        printf("%02x ", boot3_get_current_state()->progress.data[i]);

        if (i % 32 == 31) {
            printf("\n");
        }
    }

    // busy_wait_ms(2000);

    // for (size_t i = 0; i < 360; i++) {
    //     RGBColor color = hsv2rgb((float)i, 100, 10);
    //     internal_ws2812_reset();
    //     internal_ws2812_transmit(color.r, color.g, color.b);
    //     sleep_ms(15);
    // }

    uint8_t* new_program = malloc(64 * 1024);
    int read = 0;
    size_t offset = 0;

    internal_ws2812_reset();
    internal_ws2812_transmit(0, 128, 0);

    printf("Waiting for new program for 10 seconds...\n");
    absolute_time_t timeout = make_timeout_time_ms(10000);

    stdio_set_translate_crlf(&stdio_usb, false);

    while ((read = stdio_get_until(new_program + offset, 256, timeout)) >= 0)
    {
        if (offset + read > 64 * 1024) 
        {
            offset = 0;
            printf("Received program is too large, max size is 64KB\n");
            break;
        }

        if (read == 0)
        {
            continue;
        }

        offset += read;
        printf("Read %d bytes, offset: %u\r\n", read, offset);
        timeout = make_timeout_time_ms(500);
    }

    stdio_set_translate_crlf(&stdio_usb, true);

    printf("Finished reading new program, total size: %u bytes\n", offset);

    bool pending_program_valid = false;

    if (offset > 0)
    {
        internal_ws2812_reset();
        internal_ws2812_transmit(128, 128, 0);

        printf("Erasing flash for pending state...\n");
        uint32_t ints = save_and_disable_interrupts();
        boot3_flash_erase_pending_data(offset);
        restore_interrupts(ints);

        printf("Pending program magic: %x\n", boot3_get_pending_state()->prelude.magic);

        printf("Programming pending state...\n");
        uint32_t pages = (offset - 1) / FLASH_PAGE_SIZE + 1; // Round up to nearest flash page

        ints = save_and_disable_interrupts();
        boot3_flash_program_pending_data(0, new_program, pages * FLASH_PAGE_SIZE);
        restore_interrupts(ints);

        pending_program_valid = boot3_validate_state(boot3_get_pending_state());

        printf("Pending program magic: %x\n", boot3_get_pending_state()->prelude.magic);
        printf("New program checksum: %llx\n", boot3_fnv1a_64(new_program, offset));
        printf("Is pending state valid: %d\n", pending_program_valid);
    }
    else 
    {
        internal_ws2812_reset();
        internal_ws2812_transmit(128, 0, 128);
    }

    if (!pending_program_valid || offset == 0)
    {
        printf("Waiting for key stroke before reboot...\n");
        stdio_getchar();
    }

    internal_ws2812_reset();
    internal_ws2812_transmit(128, 0, 0);

    watchdog_enable(0, true);
    // rom_reset_usb_boot(0, 0);

    for(;;) {
        tight_loop_contents();
    }

    return 0;
}

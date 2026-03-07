#include "pico/stdlib.h"
#include "ws2812.h"

int main(void) {
    internal_ws2812_init();

    internal_ws2812_reset();
    internal_ws2812_transmit(0, 255, 0);

    for(;;) {
        tight_loop_contents();
    }

    return 0;
}

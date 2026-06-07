#include <core/system.h>
#include <lib/led.h>
#include <core/systick.h>

int main(void) {
    system_init();
    led_init();

    uint8_t cycle = 0;
    for (;;) {
        for (uint8_t led_idx = 0; led_idx < LED_COUNT; led_idx++) {
            led_set_state((led_t)led_idx, cycle == led_idx);
        }

        systick_delay_ms(200);
        cycle = (cycle + 1) % LED_COUNT;
    }

    return 0;
}

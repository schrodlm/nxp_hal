#include <stdio.h>
#include <core/system.h>
#include <core/systick.h>
#include <lib/btn.h>
#include <lib/btn_cnt.h>
#include <lib/led.h>

int main(void) {
    system_init();
    led_init();
    btn_init();
    btn_cnt_init();

    printf("Press buttons to increment their counters.\n");

    uint8_t cycle = 0;
    for (;;) {
        for (uint8_t led_idx = 0; led_idx < LED_COUNT; led_idx++) {
            led_set_state(led_idx, cycle == led_idx);
        }

        printf("Button press/release counters:");
        for (uint8_t btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
            printf(" %02u", btn_cnt_get_count((btn_t)btn_idx));
        }
        printf("\n");

        systick_delay_ms(200);
        cycle = (cycle + 1) % LED_COUNT;
    }

    return 0;
}

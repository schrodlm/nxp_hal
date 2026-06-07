#include <core/system.h>
#include <core/systick.h>
#include <lib/kpad.h>
#include <lib/led.h>
#include <stdio.h>

static const char kpad_chars[] = "123456789*0#";

int main(void) {
    system_init();
    led_init();
    kpad_init();

    printf("Press keys on the keypad.\n");

    uint32_t prev_scan = 0;
    uint32_t blink_counter = 0;
    bool led_on = false;

    for (;;) {
        uint32_t scan = kpad_scan();

        if (scan != prev_scan) {
            if (scan != 0) {
                printf("Scan: %08lX,  Keys:", scan);
                for (uint32_t i = 0; i < KPAD_COLS * KPAD_ROWS; i++) {
                    if (scan & (1u << i)) {
                        printf(" %c", kpad_chars[i]);
                    }
                }
                printf("\n");
            } else {
                printf("Scan: %08lX,  No keys\n", scan);
            }
            prev_scan = scan;
        }

        blink_counter++;
        if (blink_counter >= 50) {
            blink_counter = 0;
            led_on = !led_on;
        }
        led_set_state(LED_YELLOW, led_on);

        systick_delay_ms(10);
    }

    return 0;
}

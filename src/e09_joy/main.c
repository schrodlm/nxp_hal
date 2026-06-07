#include <core/system.h>
#include <core/systick.h>
#include <lib/joy.h>
#include <lib/led.h>
#include <stdio.h>

int main(void) {
    system_init();
    led_init();
    joy_init();

    printf("Joystick demo. Move stick and press button.\n");

    uint32_t blink_counter = 0;
    bool led_on = false;

    for (;;) {
        joy_result_t joy = joy_read();

        printf("X: %4u  Y: %4u  BTN: %u\n", joy.x, joy.y, joy.pressed);

        blink_counter++;
        if (blink_counter >= 50) {
            blink_counter = 0;
            led_on = !led_on;
        }
        led_set_state(LED_BLUE, led_on);

        systick_delay_ms(10);
    }

    return 0;
}

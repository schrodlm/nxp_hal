#include <core/system.h>
#include <core/systick.h>
#include <lib/enc.h>
#include <lib/led.h>
#include <lib/servo.h>
#include <stdio.h>

/** @brief Maximum encoder position (saturation limit) */
#define ENC_MAX_POS 24

/** @brief Microseconds of pulse range per encoder tick */
#define US_PER_TICK ((SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US) / 2 / ENC_MAX_POS)

int main(void) {
    system_init();
    led_init();
    enc_init();
    servo_init();

    printf("Rotate encoder to control servo position. Press button to reset the position.\n");

    uint32_t blink_counter = 0;
    bool led_on = false;

    for (;;) {
        // Stop servo and reset encoder on button press
        if (enc_get_pressed()) {
            enc_reset_position();
            servo_set_pulse(SERVO_PULSE_MIDDLE_US);
        }

        // Read encoder position with saturation
        int16_t pos = enc_get_position();
        if (pos > ENC_MAX_POS) {
            pos = ENC_MAX_POS;
        }
        if (pos < -ENC_MAX_POS) {
            pos = -ENC_MAX_POS;
        }

        // Map encoder position to pulse width: 0 -> 1500, ±ENC_MAX_POS -> 1500 ± 500
        uint32_t pulse_us = (uint32_t)(SERVO_PULSE_MIDDLE_US - pos * US_PER_TICK);
        servo_set_pulse(pulse_us);

        // Blink orange LED with 500ms toggle interval (50 * 10ms)
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

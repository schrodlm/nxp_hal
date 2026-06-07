#include <stdio.h>
#include <core/system.h>
#include <core/systick.h>
#include <lib/btn.h>
#include <lib/btn_cnt.h>
#include <lib/led.h>
#include <lib/enc.h>

static systick_listener_t systick_listener;

static int16_t last_enc_pos;

static void systick_callback(void) {
    if (enc_get_pressed()) {
        enc_reset_position();
    }

    int16_t enc_pos = enc_get_position();
    if (enc_pos != last_enc_pos) {
        printf("Pos: %d\n", enc_get_position());
    }

    last_enc_pos = enc_pos;
}

int main(void) {
    system_init();
    led_init();
    enc_init();

    last_enc_pos = enc_get_position();

    systick_register_listener(&systick_listener, &systick_callback);

    printf("Rotate the encoder to see its position. Press encoder button to reset it.\n");

    uint8_t cycle = 0;
    for (;;) {
        for (uint8_t led_idx = 0; led_idx < LED_COUNT; led_idx++) {
            led_set_state(led_idx, cycle == led_idx);
        }

        systick_delay_ms(200);
        cycle = (cycle + 1) % LED_COUNT;
    }

    return 0;
}

#include <core/system.h>
#include <core/systick.h>
#include <lib/lcd.h>
#include <lib/led.h>
#include <lib/spi.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    system_init();
    led_init();
    spi_init();
    lcd_init();

    lcd_update();

    for (int i = 0; i < 8; i++) {
        lcd_draw_string(1, i * 8, "ABCDEFGHIJKLMNOPQRSTUV");
    }
    for (int y = 0; y < 64; y++) {
        lcd_set_pixel(0, y, true);
        lcd_set_pixel(127, y, true);
    }

    lcd_update();

    char out[16];
    memset(out, ' ', sizeof(out));

    uint8_t cycle = 0;
    uint8_t x = 0;
    for (;;) {
        for (uint8_t led_idx = 0; led_idx < LED_COUNT; led_idx++) {
            led_set_state((led_t)led_idx, cycle == led_idx);
        }

        snprintf(out, sizeof(out), "   Cycle:  %d   ", cycle);
        lcd_draw_string(20, 28, out);
        lcd_update();

        // lcd_clear_buffer();
        for (uint8_t i = 0; i < 5; i++) {
            for (uint8_t j = 0; j < 5; j++) {
                lcd_set_pixel(x + i, 20 + j, true);
            }
        }
        for (uint8_t j = 0; j < 5; j++) {
            lcd_set_pixel(x-1, 20 + j, false);
        }

        for (uint8_t i = 0; i < 6; i++) {
            lcd_set_pixel(x + i, 35, true);
            lcd_set_pixel(x + i, 36, true);
            lcd_set_pixel(x + i + 2, 41, true);
            lcd_set_pixel(x + i + 2, 42, true);
            lcd_set_pixel(x, 35 + i + 2, true);
            lcd_set_pixel(x + 1, 35 + i + 2, true);
            lcd_set_pixel(x + 6, 35 + i, true);
            lcd_set_pixel(x + 7, 35 + i, true);
        }
        for (uint8_t i = 0; i < 4; i++) {
            lcd_set_pixel(x + 5, 37 + i, false);
        }
        for (uint8_t i = 0; i < 8; i++) {
            lcd_set_pixel(x - 1, 35 + i, false);
        }
        lcd_update();

        x += 1;
        if (x>=128) {
            x=0;
        }

        systick_delay_ms(50);
        cycle = (cycle + 1) % LED_COUNT;
    }

    return 0;
}

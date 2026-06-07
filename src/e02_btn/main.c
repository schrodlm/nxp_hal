#include <stdio.h>
#include <core/system.h>
#include <core/systick.h>
#include <lib/btn.h>
#include <lib/btn_cnt.h>
#include <lib/led.h>

static void test_interrupt_callback(void) {
    btn_clear_isfr();

    printf("Button state:");
    for (uint8_t btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
        printf(" %u", btn_get_pressed(btn_idx));
    }
    printf("\n");
}


int main(void) {
    system_init();
    led_init();
    btn_init();
    btn_set_isr_callback(&test_interrupt_callback);

    for (uint8_t btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
        btn_set_irqc(btn_idx, BTN_IRQC_EITHER);
    }

    printf("Press/release buttons to see how their state.\n");

    btn_enable_isr();

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

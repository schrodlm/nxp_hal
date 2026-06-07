#include <core/system.h>
#include <core/systick.h>
#include <lib/eeprom.h>
#include <lib/led.h>
#include <lib/spi.h>
#include <stdio.h>
#include <string.h>

static uint8_t data_out[EEPROM_PAGE_SIZE];
static uint8_t data_in[EEPROM_PAGE_SIZE];

static void compare_data(void) {
    bool errors = false;
    for (int i = 0; i < EEPROM_PAGE_SIZE; i++) {
        if (data_in[i] != data_out[i]) {
            errors = true;
            break;
        }
    }

    led_set_state(LED_GREEN, !errors);
    led_set_state(LED_RED, errors);
}

#ifdef SPI_WITH_DMA
static void write_done_callback(void) {
    eeprom_read_page(0, data_in, &compare_data);
}

#endif

int main(void) {
    system_init();
    led_init();
    spi_init();
    eeprom_init();

    led_set_state(LED_GREEN, false);
    led_set_state(LED_RED, false);

    // Prepare test pattern
    for (int i = 0; i < EEPROM_PAGE_SIZE; i++) {
        data_out[i] = (uint8_t)(i + 0x10);
    }

    // Clear read buffer
    memset(data_in, 0, EEPROM_PAGE_SIZE);

#ifdef SPI_WITH_DMA
    eeprom_write_page(0, data_out, &write_done_callback);
#else
    eeprom_write_page(0, data_out);
    eeprom_read_page(0, data_in);
    compare_data();
#endif

    for (;;) {
        led_set_state(LED_BLUE, true);
        systick_delay_ms(500);
        led_set_state(LED_BLUE, false);
        systick_delay_ms(500);
    }

    return 0;
}

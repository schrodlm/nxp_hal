#include <core/system.h>
#include <core/systick.h>
#include <lib/btn.h>
#include <lib/eeprom.h>
#include <lib/enc.h>
#include <lib/lcd.h>
#include <lib/led.h>
#include <lib/sht40.h>
#include <lib/spi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EEPROM_PAGE 0
#define EEPROM_SAVE_INTERVAL_MS 30000
#define EEPROM_VALID_MARKER 0xA5

/** @brief Duration in ms the encoder button must be held to reset min/max values */
#define ENC_BTN_RESET_HOLD_MS 2000

typedef struct {
    uint8_t valid_marker;
    bool blink_enabled;
    bool minmax_initialized;
    int32_t temp_min;
    int32_t temp_max;
    int32_t hum_min;
    int32_t hum_max;
} eeprom_data_t;

static eeprom_data_t eeprom_data;
static bool dirty = false;
static uint32_t last_save_ms = 0;

static bool enc_btn_prev = false;
static uint32_t enc_btn_press_start = 0;
static bool enc_btn_held_reset = false;

static bool btn4_prev = false;

static void eeprom_save(void) {
    uint8_t buf[EEPROM_PAGE_SIZE];
    memset(buf, 0xFF, EEPROM_PAGE_SIZE);
    memcpy(buf, &eeprom_data, sizeof(eeprom_data));
    eeprom_write_page(EEPROM_PAGE, buf);
    dirty = false;
    last_save_ms = systick_get_ms();
}

static void eeprom_load(void) {
    uint8_t buf[EEPROM_PAGE_SIZE];
    eeprom_read_page(EEPROM_PAGE, buf);
    memcpy(&eeprom_data, buf, sizeof(eeprom_data));

    if (eeprom_data.valid_marker != EEPROM_VALID_MARKER) {
        eeprom_data.valid_marker = EEPROM_VALID_MARKER;
        eeprom_data.blink_enabled = true;
        eeprom_data.minmax_initialized = false;
        eeprom_save();
    }
}

static void minmax_update(sht40_result_t result) {
    if (!result.valid) {
        return;
    }

    if (!eeprom_data.minmax_initialized) {
        eeprom_data.temp_min = result.temperature;
        eeprom_data.temp_max = result.temperature;
        eeprom_data.hum_min = result.humidity;
        eeprom_data.hum_max = result.humidity;
        eeprom_data.minmax_initialized = true;
        dirty = true;
        return;
    }

    if (result.temperature < eeprom_data.temp_min) {
        eeprom_data.temp_min = result.temperature;
        dirty = true;
    }
    if (result.temperature > eeprom_data.temp_max) {
        eeprom_data.temp_max = result.temperature;
        dirty = true;
    }
    if (result.humidity < eeprom_data.hum_min) {
        eeprom_data.hum_min = result.humidity;
        dirty = true;
    }
    if (result.humidity > eeprom_data.hum_max) {
        eeprom_data.hum_max = result.humidity;
        dirty = true;
    }
}


static void draw_current_screen(sht40_result_t result) {
    char line[32];

    lcd_clear_buffer();
    lcd_draw_string(42, 2, "CURRENT");

    if (!result.valid) {
        lcd_draw_string(30, 30, "No data yet");

    } else {
        snprintf(line, sizeof(line), "Temp: %3ld.%03d C", result.temperature / 1000, abs(result.temperature % 1000));
        lcd_draw_string(1, 22, line);

        snprintf(line, sizeof(line), "RelH: %3ld.%03ld %%", result.humidity / 1000, result.humidity % 1000);
        lcd_draw_string(1, 38, line);
    }

    lcd_update();
}

static void draw_minmax_screen(void) {
    char line[64];

    lcd_clear_buffer();
    lcd_draw_string(36, 2, "MIN / MAX");

    if (!eeprom_data.minmax_initialized) {
        lcd_draw_string(30, 30, "No data yet");

    } else {
        snprintf(line, sizeof(line), "T:  %ld.%03d - %ld.%03d",
                 eeprom_data.temp_min / 1000, abs(eeprom_data.temp_min % 1000),
                 eeprom_data.temp_max / 1000, abs(eeprom_data.temp_max % 1000));
        lcd_draw_string(1, 22, line);

        snprintf(line, sizeof(line), "RH: %ld.%03ld - %ld.%03ld",
                 eeprom_data.hum_min / 1000, eeprom_data.hum_min % 1000,
                 eeprom_data.hum_max / 1000, eeprom_data.hum_max % 1000);
        lcd_draw_string(1, 38, line);
    }

    lcd_update();
}

int main(void) {
    system_init();
    led_init();
    btn_init();
    enc_init();
    spi_init();
    lcd_init();
    eeprom_init();
    sht40_init();

    lcd_update();
    eeprom_load();
    last_save_ms = systick_get_ms();

    uint32_t blink_counter = 0;
    bool led_on = false;

    for (;;) {
        sht40_result_t result = sht40_get_result();

        minmax_update(result);

        // Toggle blink on BTN_4 press (edge detection)
        bool btn4_now = btn_get_pressed(BTN_4);
        if (btn4_now && !btn4_prev) {
            eeprom_data.blink_enabled = !eeprom_data.blink_enabled;
            eeprom_save();
        }
        btn4_prev = btn4_now;

        // Reset min/max when encoder button held for >2s
        bool enc_btn_now = enc_get_pressed();
        if (enc_btn_now && !enc_btn_prev) {
            enc_btn_press_start = systick_get_ms();
            enc_btn_held_reset = false;
        }
        if (enc_btn_now && (enc_btn_held_reset || systick_get_ms() - enc_btn_press_start >= ENC_BTN_RESET_HOLD_MS)) {
            eeprom_data.minmax_initialized = false;
            enc_btn_held_reset = true;
        }
        if (!enc_btn_now && enc_btn_held_reset) {
            enc_btn_held_reset = false;
            eeprom_save();
        }

        enc_btn_prev = enc_btn_now;

        // Save to EEPROM every 30s if dirty
        if (dirty && (systick_get_ms() - last_save_ms >= EEPROM_SAVE_INTERVAL_MS)) {
            eeprom_save();
        }

        // Encoder selects screen: even position = current, odd = min/max
        uint32_t screen = (((uint32_t)enc_get_position() + 2) / 4) & 1;
        if (screen == 0) {
            draw_current_screen(result);
        } else {
            draw_minmax_screen();
        }

        // Blink LED with 500ms toggle interval (50 * 10ms)
        blink_counter++;
        if (blink_counter >= 50) {
            blink_counter = 0;
            led_on = !led_on;
        }
        led_set_state(LED_YELLOW, eeprom_data.blink_enabled && led_on);

        systick_delay_ms(10);
    }

    return 0;
}

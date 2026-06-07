#include <core/system.h>
#include <core/systick.h>
#include <lib/btn.h>
#include <lib/can.h>
#include <lib/eeprom.h>
#include <lib/enc.h>
#include <lib/joy.h>
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

/** @brief CAN reporting interval in milliseconds */
#define CAN_REPORT_INTERVAL_MS 1000


/** @brief Text message buffer dimensions */
#define TEXT_BUF_LINES      40
#define TEXT_BUF_COLS       50

/** @brief Number of visible text lines on LCD */
#define TEXT_VISIBLE_LINES  7

/** @brief Joystick deadzone and scroll threshold */
#define JOY_DEADZONE        1024
#define JOY_CENTER          2048

/** @brief Joystick scroll repeat interval in loop iterations (10ms each) */
#define JOY_SCROLL_REPEAT   15

typedef struct {
    uint8_t valid_marker;
    bool blink_enabled;
    bool minmax_initialized;
    int32_t temp_min;
    int32_t temp_max;
    int32_t hum_min;
    int32_t hum_max;
    uint16_t raw_temp_min;
    uint16_t raw_temp_max;
    uint16_t raw_hum_min;
    uint16_t raw_hum_max;
} eeprom_data_t;

static eeprom_data_t eeprom_data;
static bool dirty = false;
static uint32_t last_save_ms = 0;

static bool enc_btn_prev = false;
static uint32_t enc_btn_press_start = 0;
static bool enc_btn_held_reset = false;

static bool btn4_prev = false;

static uint32_t last_can_report_ms = 0;

// Text message buffer
static char text_buf[TEXT_BUF_LINES][TEXT_BUF_COLS + 1]; // +1 for null terminator
static uint32_t text_line_count = 0;
static uint32_t text_cur_col = 0;
static uint32_t text_scroll_pos = 0;
static uint32_t joy_scroll_counter = 0;

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
        eeprom_data.raw_temp_min = result.raw_temp;
        eeprom_data.raw_temp_max = result.raw_temp;
        eeprom_data.raw_hum_min = result.raw_hum;
        eeprom_data.raw_hum_max = result.raw_hum;
        eeprom_data.minmax_initialized = true;
        dirty = true;
        return;
    }

    if (result.temperature < eeprom_data.temp_min) {
        eeprom_data.temp_min = result.temperature;
        eeprom_data.raw_temp_min = result.raw_temp;
        dirty = true;
    }
    if (result.temperature > eeprom_data.temp_max) {
        eeprom_data.temp_max = result.temperature;
        eeprom_data.raw_temp_max = result.raw_temp;
        dirty = true;
    }
    if (result.humidity < eeprom_data.hum_min) {
        eeprom_data.hum_min = result.humidity;
        eeprom_data.raw_hum_min = result.raw_hum;
        dirty = true;
    }
    if (result.humidity > eeprom_data.hum_max) {
        eeprom_data.hum_max = result.humidity;
        eeprom_data.raw_hum_max = result.raw_hum;
        dirty = true;
    }
}

static void text_buf_init(void) {
    for (uint32_t i = 0; i < TEXT_BUF_LINES; i++) {
        text_buf[i][0] = '\0';
    }
    text_line_count = 0;
    text_cur_col = 0;
    text_scroll_pos = 0;
}

static void text_buf_new_line(void) {
    if (text_line_count < TEXT_BUF_LINES) {
        text_line_count++;
    } else {
        // Rotate: shift all lines up by one
        for (uint32_t i = 0; i < TEXT_BUF_LINES - 1; i++) {
            memcpy(text_buf[i], text_buf[i + 1], TEXT_BUF_COLS + 1);
        }
    }
    uint32_t line = (text_line_count > 0) ? text_line_count - 1 : 0;
    text_buf[line][0] = '\0';
    text_cur_col = 0;
}

static void text_buf_append_char(char c) {
    if (text_line_count == 0) {
        text_line_count = 1;
        text_buf[0][0] = '\0';
        text_cur_col = 0;
    }

    uint32_t line = text_line_count - 1;
    if (text_cur_col < TEXT_BUF_COLS) {
        text_buf[line][text_cur_col] = c;
        text_cur_col++;
        text_buf[line][text_cur_col] = '\0';
    }
    // If over TEXT_BUF_COLS, trim (ignore extra characters)
}

static void process_text_message(const uint8_t *data, uint8_t dlc) {
    for (uint32_t i = 0; i < dlc; i++) {
        uint8_t byte = data[i];
        bool new_line = (byte & 0x80) != 0;
        char c = (char)(byte & 0x7F);

        if (new_line) {
            text_buf_new_line();
        }

        if (c != '\0') {
            text_buf_append_char(c);
        }
    }

    if (text_line_count > TEXT_VISIBLE_LINES) {
        text_scroll_pos = text_line_count - TEXT_VISIBLE_LINES;
    } else {
        text_scroll_pos = 0;
    }
}

static void can_report(sht40_result_t result) {
    if (!result.valid) {
        return;
    }

    // Send current temperature and humidity (raw 16-bit values)
    can_send_env_current(result.raw_temp, result.raw_hum);

    // Send min/max values if initialized
    if (eeprom_data.minmax_initialized) {
        can_send_env_minmax(eeprom_data.raw_temp_min, eeprom_data.raw_temp_max,
                            eeprom_data.raw_hum_min, eeprom_data.raw_hum_max);
    }
}

static void can_process_rx(void) {
    // Check for reset command
    if (can_receive_env_reset()) {
        eeprom_data.minmax_initialized = false;
        eeprom_save();
    }

    // Check for text messages
    uint8_t data[8];
    uint8_t dlc;
    while (can_receive_env_text(data, &dlc)) {
        process_text_message(data, dlc);
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

static void draw_text_screen(void) {
    lcd_clear_buffer();
    lcd_draw_string(36, 0, "MESSAGES");

    if (text_line_count == 0) {
        lcd_draw_string(30, 30, "No messages");
    } else {
        for (uint32_t i = 0; i < TEXT_VISIBLE_LINES; i++) {
            uint32_t line_idx = text_scroll_pos + i;
            if (line_idx >= text_line_count) {
                break;
            }
            lcd_draw_string(1, 8 + i * 8, text_buf[line_idx]);
        }
    }

    lcd_update();
}

static void update_text_scroll(void) {
    joy_result_t joy = joy_read();
    int32_t y_offset = JOY_CENTER - (int32_t)joy.y;

    if (y_offset > JOY_DEADZONE || y_offset < -JOY_DEADZONE) {
        joy_scroll_counter++;
        if (joy_scroll_counter >= JOY_SCROLL_REPEAT) {
            joy_scroll_counter = 0;
            if (y_offset > JOY_DEADZONE && text_scroll_pos + TEXT_VISIBLE_LINES < text_line_count) {
                text_scroll_pos++;
            } else if (y_offset < -JOY_DEADZONE && text_scroll_pos > 0) {
                text_scroll_pos--;
            }
        }
    } else {
        joy_scroll_counter = 0;
    }
}

int main(void) {
    system_init();
    led_init();
    btn_init();
    enc_init();
    joy_init();
    spi_init();
    lcd_init();
    eeprom_init();
    sht40_init();
    can_init();

    lcd_update();
    eeprom_load();
    text_buf_init();
    last_save_ms = systick_get_ms();
    last_can_report_ms = systick_get_ms();

    uint32_t blink_counter = 0;
    bool led_on = false;

    for (;;) {
        sht40_result_t result = sht40_get_result();

        minmax_update(result);

        // Process incoming CAN messages
        can_process_rx();

        // Periodic CAN reporting
        if (systick_get_ms() - last_can_report_ms >= CAN_REPORT_INTERVAL_MS) {
            can_report(result);
            last_can_report_ms = systick_get_ms();
        }

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
            eeprom_save();
        }

        enc_btn_prev = enc_btn_now;

        // Save to EEPROM every 30s if dirty
        if (dirty && (systick_get_ms() - last_save_ms >= EEPROM_SAVE_INTERVAL_MS)) {
            eeprom_save();
        }

        // Encoder selects screen: 0=current, 1=min/max, 2=messages
        uint32_t screen = (((uint32_t)enc_get_position() + 2) / 4) % 3;
        if (screen == 0) {
            draw_current_screen(result);
        } else if (screen == 1) {
            draw_minmax_screen();
        } else {
            update_text_scroll();
            draw_text_screen();
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

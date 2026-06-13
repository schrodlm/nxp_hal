#include <lib/lcd.h>

#include <core/systick.h>
#include <device_registers.h>
#include <lib/lcd_font.h>
#include <lib/spi.h>

#include <string.h>

#define LCD_DC_PIN  10  // PTA10
#define LCD_RST_PIN 11  // PTA11

#define LCD_CS_PIN  5  //PTB5

#define LPSPI0_PCS  0

#define PAGE_SIZE   8  // how many rows of the actual display is one page
_Static_assert(LCD_FONT_CHAR_HEIGHT <= PAGE_SIZE, "Provided font is larger than the LCD page");

_Static_assert(LCD_HEIGHT % PAGE_SIZE == 0, "LCD_HEIGHT must be divisible by PAGE_SIZE");

constexpr uint8_t rows = (LCD_HEIGHT / PAGE_SIZE);
constexpr uint8_t cols = LCD_WIDTH;
/*
 * Framebuffer layout (matches ST7565's native memory format, so lcd_update()
 * can stream it to the panel verbatim).
 *
 * The display is divided into horizontal "pages" of 8 pixels each. A single
 * byte holds 8 vertically-stacked pixels in one column of one page:
 *   bit 0 = top pixel, bit 7 = bottom pixel.
 *
 * To address a single pixel (x, y):
 *   byte index = (y / PAGE_SIZE) * LCD_WIDTH + x
 *   bit mask   = 1 << (y % PAGE_SIZE)
 */
static uint8_t framebuffer[rows * cols];

static void _lcd_reset() {
    PTA->PCOR = (1u << LCD_RST_PIN);
    systick_delay_ms(50);
    PTA->PSOR = (1u << LCD_RST_PIN);
    systick_delay_ms(50);
}

static void _lcd_write_command(uint8_t *cmd, uint16_t len) {
    PTA->PCOR = (1u << LCD_DC_PIN);  //drive LCD_DC_PIN LOW (sending commands)

    spi_txrx(cmd, 0, len, LPSPI0_PCS);
}

static void _lcd_write_data(uint8_t *data, uint16_t len) {
    PTA->PSOR = (1u << LCD_DC_PIN);  //drive LCD_DC_PIN HIGH (sending data)

    spi_txrx(data, 0, len, LPSPI0_PCS);
}

void lcd_init(void) {
    // Clock the ports we need
    PCC->PCCn[PCC_PORTA_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_PORTB_INDEX] = PCC_PCCn_CGC_MASK;


    //DC and RST as plain GPIO
    PORTA->PCR[LCD_DC_PIN] = PORT_PCR_MUX(1);
    PORTA->PCR[LCD_RST_PIN] = PORT_PCR_MUX(1);

    // CS managed by LPSPI: route PTB5 to LPSPI0_PCS0 (MUX=4)
    PORTB->PCR[LCD_CS_PIN] = PORT_PCR_MUX(4);

    //DC and RST are outputs
    PTA->PDDR |= (1u << LCD_DC_PIN) | (1u << LCD_RST_PIN);

    _lcd_reset();

    static uint8_t init_cmds[] = {
        0xA2,  // Bias 1/9
        0xA0,  // ADC normal
        0xC8,  // COM output reverse
        0x25,  // Resistor ratio (coarse contrast)
        0x81,  // Electronic Volume Register Set (fine contrast)
        0x26,  // Value
        0x2F,  // Power control: all on
        0x40,  // Start line = 0
        0xAF   // Display ON
    };

    _lcd_write_command(init_cmds, sizeof(init_cmds));
    systick_delay_ms(50);
}

void lcd_update(void) {
    for (uint8_t page = 0; page < rows; page++) {
        //dump per row
        uint8_t cmds[] = {
            0xB0 | page,  // set page address
            0x10,         // set column high nibble = 0
            0x00,         // set column low nibble = 0
        };
        _lcd_write_command(cmds, sizeof(cmds));
        _lcd_write_data(&framebuffer[page * LCD_WIDTH], LCD_WIDTH);
    }
}

void lcd_set_pixel(uint32_t x, uint32_t y, bool color) {
    if (x > LCD_WIDTH || y >= LCD_HEIGHT) return;

    uint8_t *slice = &framebuffer[((y / PAGE_SIZE) * LCD_WIDTH) + x];
    uint8_t mask = 1u << (y % PAGE_SIZE);
    if (color) {
        *slice |= mask;
    } else {
        *slice &= ~mask;
    }
}

void lcd_draw_char(uint32_t x, uint32_t y, char c) {
    if (c < LCD_FONT_START_CHAR || c > LCD_FONT_END_CHAR) return;

    const uint8_t *bitmap = lcd_font[c - LCD_FONT_START_CHAR];
    for (uint8_t col = 0; col < LCD_FONT_CHAR_WIDTH; col++) {
        uint8_t bits = bitmap[col];
        for (uint8_t row = 0; row < LCD_FONT_CHAR_HEIGHT; row++) {
            bool on = (bits >> row) & 1u;
            lcd_set_pixel(x + col, y + row, on);
        }
    }
}

void lcd_draw_string(uint32_t x, uint32_t y, const char *str) {
    while (*str) {
        lcd_draw_char(x, y, *str);
        x += LCD_FONT_CHAR_WIDTH + 1;  // 5 + 1 spacing
        str++;
    }
}

void lcd_clear_buffer(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
}
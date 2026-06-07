/**
 * @file lcd.h
 * @ingroup lib
 * @brief ST7565 LCD display driver
 *
 * Provides hardware abstraction for ST7565-based 128x64 monochrome LCD display
 * with SPI communication interface.
 *
 * The implementation is intentionally split into two layers:
 * - a framebuffer in RAM manipulated by drawing functions
 * - an update step that transfers the framebuffer to the LCD controller
 */

#ifndef LIB_LCD_H
#define LIB_LCD_H

#include <stdbool.h>
#include <stdint.h>

/** @brief LCD width in pixels */
#define LCD_WIDTH  128

/** @brief LCD height in pixels */
#define LCD_HEIGHT 64

/** @brief LCD framebuffer size in bytes (128 x 64 / 8) */
#define LCD_BUFFER_SIZE (LCD_WIDTH * LCD_HEIGHT / 8)

/**
 * @brief Initialize LCD hardware
 *
 * Configures SPI (LPSPI0), GPIO pins for control signals, and initializes
 * the ST7565 display controller. Sets up default display parameters including
 * bias, ADC direction, COM output direction, and contrast.
 *
 * @note Must be called before using any other LCD functions
 *
 * @note Display data is not updated as part of init. If the display is supposed to be cleared at the start,
 * user must call lcd_update() after lcd_init to clear the display.
 *
 * High-level implementation outline:
 * 1. Make sure the shared SPI peripheral has already been initialized.
 * 2. Enable clock gates for the PORTs used by:
 *    - the LCD chip-select pin (if controlled by the SPI peripheral)
 *    - reset pin
 *    - data/command pin
 * 3. Configure the reset and D/C pins as GPIO outputs.
 * 4. Configure the CS pin to the appropriate LPSPI alternate function.
 * 5. Perform a hardware reset sequence on the display.
 * 6. Send the controller initialization command sequence over SPI in command
 *    mode to set bias, scan direction, power/contrast, and display enable.
 * 7. Clear the software framebuffer in RAM.
 *
 * What to read in the reference manual:
 * - PCC and PORT chapters: clock gating and pin mux configuration
 * - GPIO chapter: controlling reset and D/C pins
 * - LPSPI chapter: how command bytes are sent over SPI
 * - LCD controller datasheet: meaning and required order of the init commands
 */
void lcd_init(void);

/**
 * @brief Update display from framebuffer
 *
 * Transfers the entire framebuffer to the LCD hardware. Should be called
 * after drawing operations to make changes visible on the display.
 *
 * High-level implementation outline:
 * 1. Iterate over LCD pages (rows of 8 vertical pixels).
 * 2. For each page, send the page/column address commands in command mode.
 * 3. Switch to data mode and transfer the corresponding slice of the
 *    framebuffer over SPI.
 * 4. If using DMA, queue the same sequence asynchronously page by page.
 *
 * What to read in the reference manual:
 * - LPSPI chapter: frame transfers
 * - LCD controller datasheet: page addressing and distinction between
 *   command bytes and display data bytes
 */
void lcd_update(void);

/**
 * @brief Set a pixel in the framebuffer
 *
 * Sets or clears a pixel at the specified coordinates in the framebuffer.
 * Does not update the display - call lcd_update() to show changes.
 *
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param color `true` to set pixel (black), `false` to clear pixel (white)
 *
 * High-level implementation outline:
 * 1. Reject out-of-range coordinates.
 * 2. Convert `(x, y)` into:
 *    - framebuffer byte index
 *    - bit position within that byte
 * 3. Set or clear only that bit in the RAM framebuffer.
 *
 * @note This function updates only software state in RAM. It does not access
 *       hardware registers directly.
 */
void lcd_set_pixel(uint32_t x, uint32_t y, bool color);

/**
 * @brief Draw a character in the framebuffer
 *
 * Draws a single 5x7 pixel character at the specified position using the
 * built-in font. Does not update the display - call lcd_update()
 * to show changes.
 *
 * @param x X coordinate of character top-left corner (0-127)
 * @param y Y coordinate of character top-left corner (0-63)
 * @param c Character to draw (printable ASCII)
 *
 * High-level implementation outline:
 * 1. Validate that the character exists in the font table.
 * 2. Look up its bitmap in the font data.
 * 3. For each font column and row, map the font bit to a framebuffer pixel by
 *    repeatedly calling `lcd_set_pixel()` or equivalent logic.
 *
 * @note This is a framebuffer operation; students should look at the font data
 *       format rather than the reference manual here.
 */
void lcd_draw_char(uint32_t x, uint32_t y, char c);

/**
 * @brief Draw a string in the framebuffer
 *
 * Draws a null-terminated string starting at the specified position.
 * Characters are 5 pixels wide with 1 pixel spacing (6 pixels total per char).
 * Does not update the display - call lcd_update() to show changes.
 *
 * @param x X coordinate of string start (0-127)
 * @param y Y coordinate of string start (0-63)
 * @param str Null-terminated string to draw
 *
 * High-level implementation outline:
 * 1. Iterate through the string until the terminating null byte.
 * 2. Draw one character after another with fixed horizontal advance.
 * 3. Optionally clear the spacing column between neighbouring characters.
 *
 * @note This function is entirely about framebuffer manipulation in RAM.
 */
void lcd_draw_string(uint32_t x, uint32_t y, const char* str);

/**
 * @brief Clear the framebuffer
 *
 * Clears all pixels in the framebuffer (sets all bytes to 0x00).
 * Does not update the display - call lcd_update() to show changes.
 *
 * High-level implementation outline:
 * 1. Overwrite the entire framebuffer array in RAM with zero.
 *
 * @note This does not clear the physical display immediately; the content
 *       becomes visible only after `lcd_update()`.
 */
void lcd_clear_buffer(void);

#endif

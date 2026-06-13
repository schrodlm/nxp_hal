#ifndef LCD_FONT_H
#define LCD_FONT_H

#include <stdint.h>

#define LCD_FONT_START_CHAR 0x20
#define LCD_FONT_END_CHAR 0x7F
#define LCD_FONT_CHAR_WIDTH 5
#define LCD_FONT_CHAR_HEIGHT 7

/**
 * @brief LCD font (5x7 pixels)
 *
 * The font starts at character 0x20 (space) and contains 96 characters (i.e., ends at character 127).
 * Each character is represented by 5 bytes, where each byte corresponds to a column of 7 pixels (left-to-right).
 * The least significant bit is on the top, the most significant bit of the 7 bits is on the bottom.
 */
extern const uint8_t lcd_font[96][5];

#endif

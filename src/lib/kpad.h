/**
 * @file kpad.h
 * @ingroup lib
 * @brief 3x4 matrix keypad driver
 *
 * Provides keypad initialization and scanning for a 3-column, 4-row
 * matrix keypad. Returns a bitmask of all currently pressed keys.
 *
 * This module is still GPIO-based, but unlike LEDs/buttons it requires
 * students to reason about a matrix: some pins are driven, others are read,
 * and the roles interact during scanning.
 */

#ifndef LIB_KPAD_H
#define LIB_KPAD_H

#include <stdint.h>

#define KPAD_COLS 3
#define KPAD_ROWS 4

#define KPAD_KEY_1    (1u << 0)
#define KPAD_KEY_2    (1u << 1)
#define KPAD_KEY_3    (1u << 2)
#define KPAD_KEY_4    (1u << 3)
#define KPAD_KEY_5    (1u << 4)
#define KPAD_KEY_6    (1u << 5)
#define KPAD_KEY_7    (1u << 6)
#define KPAD_KEY_8    (1u << 7)
#define KPAD_KEY_9    (1u << 8)
#define KPAD_KEY_STAR (1u << 9)
#define KPAD_KEY_0    (1u << 10)
#define KPAD_KEY_HASH (1u << 11)

/**
 * @brief Initialize keypad hardware
 *
 * High-level implementation outline:
 * 1. Enable clock gates for all PORT peripherals that contain keypad row or
 *    column pins.
 * 2. Configure column pins as GPIO and set them up so they can alternate
 *    between:
 *    - inactive / released state
 *    - actively driven low during a scan step
 * 3. Configure row pins as GPIO inputs.
 * 4. If needed, enable internal pull resistors so unpressed rows have a
 *    defined logic level.
 *
 * What to read in the reference manual:
 * - PCC chapter: clock gating for the used PORTs
 * - PORT chapter: `PCR` fields for GPIO mux and pull configuration
 * - GPIO chapter: direction control and input reading
 */
void kpad_init(void);

/**
 * @brief Scan the keypad matrix
 *
 * Scans all columns and rows to detect pressed keys. Multiple
 * simultaneous key presses are supported up to the physical limits of the hardware (e.g. 14 and 12 are correctly recognized, 124 and 125 will be recognized as 1245).
 *
 * @return Bitmask of pressed keys (use KPAD_KEY_* defines to test)
 *
 * High-level implementation outline:
 * 1. Iterate over columns one by one.
 * 2. For the currently tested column, drive only that column into the active
 *    state while leaving the others inactive.
 * 3. Wait briefly so the signals settle.
 * 4. Read all row inputs from `GPIOx->PDIR`.
 * 5. Convert the detected row/column connections into bits in the returned
 *    key mask.
 * 6. Release the active column and continue with the next one.
 *
 * What to read in the reference manual:
 * - GPIO chapter: changing pin direction/output state and reading `PDIR`
 *
 * @note The reference manual explains the registers, but students also need
 *       to reason from the keypad wiring itself to understand why the scan
 *       works.
 */
uint32_t kpad_scan(void);

#endif

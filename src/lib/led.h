/**
 * @file led.h
 * @ingroup lib
 * @brief LED hardware abstraction
 *
 * Provides LED initialization and control for the four onboard LEDs.
 *
 * This module is intentionally simple and is a good first example of the
 * common S32 peripheral setup pattern used throughout the exercises:
 * 1. Enable the peripheral clock in PCC
 * 2. Configure the pin function in PORT PCR
 * 3. Configure GPIO direction/data registers
 */

#ifndef LIB_LED_H
#define LIB_LED_H

#include <stdbool.h>

/**
 * @brief LED identifiers
 */
typedef enum {
    LED_BLUE,   /**< Blue LED on PTD6 */
    LED_RED,    /**< Red LED on PTD5 */
    LED_GREEN,  /**< Green LED on PTC0 */
    LED_YELLOW, /**< Yellow LED on PTC1 */
	LED_COUNT,  /**< Total number of LEDs */
} led_t;

/**
 * @brief Initialize LED hardware
 *
 * Prepares all LED pins for GPIO output.
 *
 * High-level implementation outline:
 * 1. Find which ports contain the LED pins.
 * 2. In `PCC->PCCn[...]`, enable the clock gate for each of those PORT
 *    peripherals before touching their pin configuration registers.
 * 3. In each corresponding `PORTx->PCR[n]`, select the GPIO pin function
 *    (`MUX = 1`).
 * 4. In each corresponding `GPIOx->PDDR`, set the pin direction to output.
 * 5. Ensure the initial output state is off using the GPIO output registers
 *    (`PSOR` / `PCOR`, depending on how the LED is wired on the board).
 *
 * What to read in the reference manual:
 * - PCC chapter: how clock gating for PORT peripherals works
 * - PORT chapter: pin control register (`PCR`) and the `MUX` field
 * - GPIO chapter: direction register (`PDDR`) and output set/clear registers
 *
 * @note The function should only perform one-time hardware configuration.
 *       Turning LEDs on/off belongs in `led_set_state()`.
 */
void led_init(void);

/**
 * @brief Set LED state
 * @param led LED to control
 * @param state true to turn on (logic high), false to turn off (logic low)
 *
 * High-level implementation outline:
 * 1. Translate the logical LED identifier to the correct GPIO peripheral and
 *    pin number.
 * 2. Use the GPIO output registers to change only that pin:
 *    - write to `PSOR` to drive the pin high
 *    - write to `PCOR` to drive the pin low
 * 3. Do not rewrite the whole output register when only one LED should change.
 *
 * What to read in the reference manual:
 * - GPIO chapter: `PDOR`, `PSOR`, `PCOR`, and why set/clear registers are
 *   preferred for single-bit updates
 */
void led_set_state(led_t led, bool state);

#endif

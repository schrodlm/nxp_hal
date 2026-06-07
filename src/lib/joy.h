/**
 * @file joy.h
 * @ingroup lib
 * @brief PS2-like joystick driver
 *
 * Provides hardware abstraction for a 2-axis analog joystick with button.
 * Uses ADC0 for X/Y axis reading and GPIO for button state.
 *
 * This is the first module where students need to combine an analog
 * peripheral (ADC) with a simple GPIO input.
 */

#ifndef LIB_JOY_H
#define LIB_JOY_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Joystick reading result
 */
typedef struct {
    uint16_t x;    /**< X axis raw ADC value (0-4095) */
    uint16_t y;    /**< Y axis raw ADC value (0-4095) */
    bool pressed;  /**< Button state: true if pressed */
} joy_result_t;

/**
 * @brief Initialize joystick hardware
 *
 * Configures ADC0 for 12-bit conversions and sets up the button GPIO pin.
 *
 * @note Must be called before using joy_read()
 *
 * High-level implementation outline:
 * 1. Enable clock gates for the PORTs used by the analog pins and button pin.
 * 2. Configure the button pin as GPIO input.
 * 3. Enable the clock gate for `ADC0` and select its clock source in `PCC`.
 * 4. Configure ADC resolution, divider, trigger mode, and any optional
 *    averaging/compare features you need.
 * 5. Leave the ADC in an idle state until an actual conversion is requested.
 *
 * What to read in the reference manual:
 * - PCC chapter: clock source selection for `ADC0`
 * - PORT/ADC pin chapter: how the analog channels map to pins
 * - ADC chapter: `CFG1`, `CFG2`, `SC1`, `SC2`, `SC3`, result register
 * - GPIO chapter: button input readout
 */
void joy_init(void);

/**
 * @brief Read joystick state
 *
 * Performs blocking ADC conversions for both axes and reads the button GPIO.
 * Each ADC conversion takes a few microseconds.
 *
 * @return Current joystick state (X, Y, button)
 *
 * High-level implementation outline:
 * 1. Start an ADC conversion for the X channel by writing the selected
 *    channel number into the appropriate `SC1` slot.
 * 2. Wait for conversion complete, then read the result register.
 * 3. Repeat the same process for the Y channel.
 * 4. Read the joystick push-button level from `GPIOx->PDIR`.
 * 5. Return all three values packed into `joy_result_t`.
 *
 * What to read in the reference manual:
 * - ADC chapter: channel selection, conversion complete flag, result register
 * - GPIO chapter: `PDIR`
 */
joy_result_t joy_read(void);

#endif

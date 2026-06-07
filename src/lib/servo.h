/**
 * @file servo.h
 * @ingroup lib
 * @brief Servo driver
 *
 * Provides PWM-based control for a servo using FTM0.
 * The servo is controlled by setting the pulse width in microseconds.
 *
 * This module uses FTM in the more familiar PWM mode, so it complements
 * `enc.h`, where FTM is used for quadrature decoding.
 */

#ifndef LIB_SERVO_H
#define LIB_SERVO_H

#include <stdint.h>

/** Minimum servo pulse width in microseconds */
#define SERVO_PULSE_MIN_US         500
/** Maximum servo pulse width in microseconds */
#define SERVO_PULSE_MAX_US         2500
/** Middle servo pulse width (center / neutral angle position) in microseconds */
#define SERVO_PULSE_MIDDLE_US      1500

/**
 * @brief Initialize servo hardware
 *
 * Configures FTM0 for edge-aligned PWM output on PTB12 (FTM0_CH0)
 * with a 20ms (50 Hz) period. Sets initial pulse width to 1500 us
 * (center / neutral angle position).
 *
 * @note Must be called before using any other servo functions
 *
 * High-level implementation outline:
 * 1. Enable the clock gate for the PORT containing the servo pin and for
 *    `FTM0` itself.
 * 2. Configure the servo pin in `PORTx->PCR[n]` to the FTM channel alternate
 *    function.
 * 3. Disable or stop the FTM while configuring it.
 * 4. Configure the FTM counter period so that one PWM cycle is 20 ms.
 * 5. Configure the selected channel for edge-aligned high-true PWM output.
 * 6. Set the initial compare value corresponding to the middle pulse width.
 * 7. Start the FTM counter with the chosen clock source and prescaler.
 *
 * What to read in the reference manual:
 * - PCC chapter: FTM clock gating and clock source selection
 * - PORT chapter: alternate function selection for the FTM output pin
 * - FTM chapter: `SC`, `MODE`, `CNTIN`, `MOD`, channel status/control, `CnV`
 */
void servo_init(void);

/**
 * @brief Set servo PWM pulse width
 *
 * Sets the PWM pulse width in microseconds. The value is clamped
 * to the range [SERVO_PULSE_MIN_US, SERVO_PULSE_MAX_US] (500-2500 us).
 * For a standard positional (angle) servo, the pulse width selects the
 * shaft angle:
 * - SERVO_PULSE_MIN_US (500 us)    = one end of the angular range
 * - SERVO_PULSE_MIDDLE_US (1500 us) = center / neutral position
 * - SERVO_PULSE_MAX_US (2500 us)   = other end of the angular range
 *
 * @param pulse_us Pulse width in microseconds (clamped to 500-2500)
 *
 * High-level implementation outline:
 * 1. Clamp the requested pulse width to the supported min/max range.
 * 2. Convert microseconds to the timer tick value used by the configured FTM
 *    counter period and prescaler.
 * 3. Write that value into the channel compare/value register (`CnV`).
 *
 * What to read in the reference manual:
 * - FTM chapter: relation between counter ticks, `MOD`, and channel value `CnV`
 */
void servo_set_pulse(uint32_t pulse_us);

#endif

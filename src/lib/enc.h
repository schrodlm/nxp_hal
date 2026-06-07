/**
 * @file enc.h
 * @ingroup lib
 * @brief Rotary encoder with button hardware abstraction
 *
 * Provides rotary encoder quadrature decoding using FTM1 in quadrature decoder mode,
 * with integrated button support.
 *
 * This module is useful because it exposes students to a peripheral mode that
 * is more specific than plain GPIO: FTM is not used here for PWM, but for
 * hardware quadrature decoding.
 */

#ifndef LIB_ENC_H
#define LIB_ENC_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize encoder hardware
 *
 * Configures FTM1 in quadrature decoder mode for Phase A (PTB3) and Phase B (PTB2),
 * and configures the encoder button (PTC14) as GPIO input.
 * Enables FTM1 clock, sets up quadrature decoder mode.
 *
 * @note Must be called before using any other encoder functions
 *
 * Outline of required initilization steps for FTM
 * - Enable peripheral clocks for quadrature decoder pins and FTM1.
 * - Configure quadrature decoder pins (Phase A and Phase B). Set appropriate MUX value for FTM1 quadrature decoder function.
 * - Optionally set CNTIN, MOD and reset CNT - not really needed, the default values are fine.
 * - Optionally configure input filters (FILTER register) to reduce glitches due to noise.
 * - Enable quadrature decoder mode (QDCTRL register) and optionally enable phase filters
 *
 * High-level implementation outline:
 * 1. Enable clock gates for the PORTs containing the encoder pins and for
 *    `FTM1` itself.
 * 2. In `PORTx->PCR[n]`, set phase A and phase B pins to the FTM alternate
 *    function used by the quadrature decoder.
 * 3. Configure the push-button pin as GPIO input.
 * 4. If desired, configure digital filtering for the button and/or the FTM
 *    phase inputs.
 * 5. Set up the FTM counter registers as needed.
 * 6. Enable quadrature decoder operation in the FTM quadrature control
 *    register and choose the phase polarity/filter options.
 *
 * What to read in the reference manual:
 * - PCC chapter: clock gating for PORT and FTM
 * - PORT chapter: alternate function selection and optional digital filtering
 * - FTM chapter: quadrature decoder mode, `QDCTRL`, `FILTER`, and counter use
 * - GPIO chapter: reading the encoder push-button
 */
void enc_init(void);

/**
 * @brief Get current encoder position
 *
 * Returns the current encoder position as a signed 16-bit integer read directly
 * from the FTM1 quadrature counter. The position increments when rotating clockwise
 * and decrements when rotating counter-clockwise (depending on encoder wiring and
 * phase polarity settings). The counter wraps around on 16-bit overflow; overflows
 * are not tracked in software.
 *
 * @return Current encoder position (positive or negative)
 *
 * @note This function is safe to call from interrupt context
 *
 * High-level implementation outline:
 * 1. Read the current FTM quadrature counter value.
 * 2. Convert it to the API's signed return type.
 *
 * What to read in the reference manual:
 * - FTM chapter: counter register used by quadrature decode mode
 */
int16_t enc_get_position(void);

/**
 * @brief Reset encoder position to zero
 *
 * Resets the encoder position counter to zero.
 *
 * @note This function is safe to call from interrupt context
 *
 * High-level implementation outline:
 * 1. Write the FTM counter register so that the current logical position
 *    becomes zero again.
 * 2. If your design uses `CNTIN`, check whether writing `CNT` loads from
 *    `CNTIN` or whether both registers need to be considered.
 *
 * What to read in the reference manual:
 * - FTM chapter: interaction between `CNT`, `CNTIN`, and counter reloading
 */
void enc_reset_position(void);

/**
 * @brief Read encoder button state
 *
 * @return true if button is pressed (logic high), false otherwise
 *
 * @note This function is safe to call from interrupt context
 *
 * High-level implementation outline:
 * 1. Read the GPIO input register for the encoder button pin.
 * 2. Mask the corresponding bit and convert it to a boolean value.
 *
 * What to read in the reference manual:
 * - GPIO chapter: `PDIR`
 */
bool enc_get_pressed(void);

#endif

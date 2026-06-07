/**
 * @file clock.h
 * @ingroup core
 * @brief System clock initialization
 *
 * This module provides system clock configuration using the 40MHz external
 * crystal oscillator.
 */

#ifndef CORE_CLOCK_H
#define CORE_CLOCK_H

/**
 * @brief Initialize the system clock
 *
 * Configures the System Oscillator (SOSC) to use the 40MHz external crystal.
 * This function sets up SOSCDIV1_CLK and SOSCDIV2_CLK to provide clock at 40MHz.
 *
 * @note This function blocks until the System OSC becomes valid
 *
 * @warning Must be called before any peripheral initialization that depends on the system clock
 */
void clock_init(void);

#endif

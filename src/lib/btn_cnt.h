/**
 * @file btn_cnt.h
 * @ingroup lib
 * @brief Button press counter with debouncing
 *
 * Higher-level button module that tracks button press counts with
 * software debouncing. Uses button and systick listeners.
 *
 * Unlike the lower-level HAL modules, this one is mainly about coordinating
 * existing building blocks:
 * - `btn` provides GPIO reads and PORT interrupt configuration
 * - `systick` provides a 1 ms time base
 * - this module adds a software state machine on top
 *
 * ## Debouncing Strategy
 *
 * The module implements a state machine per button to handle debouncing:
 *
 * ### States
 * - **Idle**: Button not pressed, waiting for press event
 * - **Pressed**: Button is currently pressed
 * - **Debounce**: Button was released, waiting for debounce timeout
 *
 * ### State Transitions and IRQC Configuration
 *
 * The interrupt configuration (IRQC) which determines whether the interrupt is off or delivered on rising or falling edge
 * is dynamically changed to minimize interrupt count while ensuring reliable button detection:
 *
 * @code{.unparsed}
 * State: IDLE
 *   IRQC = RISING
 *   On rising edge interrupt:
 *     if button is pressed and not in debounce:
 *       increment counter
 *       state = PRESSED
 *       IRQC = FALLING
 *
 * State: PRESSED
 *   IRQC = FALLING
 *   On falling edge interrupt:
 *     if button is released:
 *       state = DEBOUNCE
 *       start debounce timer (record current time)
 *       IRQC = DISABLED
 *
 * State: DEBOUNCE
 *   IRQC = DISABLED (no interrupts during debounce)
 *   On systick callback (every 1ms):
 *     if time since debounce start > BTN_CNT_DEBOUNCE_MS:
 *       state = IDLE
 *       IRQC = RISING
 * @endcode
 *
 * ### Interrupt Guarantee
 *
 * Maximum interrupts per button press/release cycle:
 * - 1 interrupt on button press (rising edge)
 * - 1 interrupt on button release (falling edge)
 * - Total: **2 interrupts maximum** per button event
 *
 * During the debounce period, interrupts are disabled, preventing
 * bouncing from generating additional interrupts.
 */

#ifndef LIB_BTN_CNT_H
#define LIB_BTN_CNT_H

#include <lib/btn.h>
#include <stdint.h>

/** @brief Debounce time in milliseconds */
#define BTN_CNT_DEBOUNCE_MS 100

/**
 * @brief Initialize button counter module
 *
 * Initializes button state tracking and registers listeners for
 * button interrupts and systick callbacks.
 *
 * Initial configuration:
 * - All button states set to IDLE
 * - All counters reset to zero
 * - IRQC set to RISING for all buttons
 * - Registers button interrupt listener
 * - Registers systick listener for debounce timeout handling
 * - Enables button interrupts
 *
 * High-level implementation outline:
 * 1. Prepare one software state record per button: counter value, current
 *    pressed/released state, debounce state, and timestamp of last release.
 * 2. Configure the underlying button module so that each button initially
 *    generates an interrupt on the press edge only.
 * 3. Register a button ISR callback through `btn_set_isr_callback()`.
 * 4. Register a SysTick listener through `systick_register_listener()`.
 * 5. In the button callback, update the software state machine and, when
 *    needed, switch the per-button `IRQC` between rising/falling/disabled.
 * 6. In the SysTick callback, watch the debounce timeout and re-enable
 *    rising-edge detection once the timeout expires.
 *
 * What to read in the reference manual:
 * - no new peripheral chapter is needed here beyond `btn` and `systick`
 * - students should instead study the interaction between:
 *   `PORT PCR.IRQC`, `PORT ISFR`, NVIC delivery, and the 1 ms SysTick tick
 *
 * @note This module is a good place to think in terms of states and events,
 *       not only individual register writes.
 */
void btn_cnt_init(void);

/**
 * @brief Get button press count
 * @param button Button to query
 * @return Number of times button has been pressed
 *
 * High-level implementation outline:
 * 1. Select the software state structure belonging to the requested button.
 * 2. Return its stored counter value.
 *
 * @note This function reads module state only; it does not access hardware
 *       registers directly.
 */
uint32_t btn_cnt_get_count(btn_t button);

/**
 * @brief Reset button press count
 * @param button Button to reset
 *
 * High-level implementation outline:
 * 1. Select the software state structure belonging to the requested button.
 * 2. Overwrite its counter value with zero.
 *
 * @note This function resets only the software counter. It should not
 *       reconfigure the button hardware or debounce state unless explicitly
 *       required by your design.
 */
void btn_cnt_reset_count(btn_t button);

#endif

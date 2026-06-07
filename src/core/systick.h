/**
 * @file systick.h
 * @ingroup core
 * @brief System tick timer
 *
 * This module provides a 1ms system tick timer with listener support for
 * periodic callbacks. Uses the ARM Cortex-M0+ SysTick timer.
 */

#ifndef CORE_SYSTICK_H
#define CORE_SYSTICK_H

#include <stdint.h>

/**
 * @brief System tick listener callback function type
 *
 * Callback functions registered with systick_register_listener() must match
 * this signature. They are called from the SysTick interrupt handler.
 */
typedef void (*systick_listener_callback_t)(void);

/**
 * @brief System tick listener node structure
 *
 * This structure represents a node in the statically-allocated linked list
 * of system tick listeners. Clients must allocate this structure statically
 * and pass it to systick_register_listener().
 *
 * @note Do not modify the fields directly - use systick_register_listener()
 */
typedef struct systick_listener_t {
    systick_listener_callback_t callback; /**< Callback function to invoke */
    struct systick_listener_t *next;      /**< Next listener in the list */
} systick_listener_t;

/**
 * @brief Initialize the system tick timer
 *
 * Configures the ARM Cortex-M0+ SysTick timer to generate interrupts every 1ms.
 * The timer uses the core clock as its source and is automatically enabled.
 *
 * @note Must be called after clock_init() to ensure proper tick rate
 * @note SysTick interrupt priority is 0 (highest) by default
 */
void systick_init(void);

/**
 * @brief Get current system tick count in milliseconds
 *
 * Returns the number of milliseconds elapsed since systick_init() was called.
 *
 * @return Current millisecond count (wraps around after ~49.7 days)
 *
 * @note This function is safe to call from interrupt context
 */
uint32_t systick_get_ms(void);

/**
 * @brief Blocking delay for specified milliseconds
 *
 * Busy-waits until the specified number of milliseconds have elapsed.
 *
 * @param ms Number of milliseconds to delay
 *
 * @warning This is a blocking delay that consumes CPU cycles
 * @note Actual delay may be slightly longer due to timing granularity
 */
void systick_delay_ms(uint32_t ms);

/**
 * @brief Register a listener for system tick events
 *
 * Adds a callback function to the list of listeners that are called every 1ms
 * from the SysTick interrupt handler. The listener node must be statically
 * allocated by the caller and will be added to the head of the listener list.
 *
 * @param listener Pointer to statically-allocated listener node
 * @param callback Function to call every 1ms from SysTick interrupt
 *
 * @note Listeners are called from interrupt context - keep callbacks short
 * @note There is no mechanism to unregister listeners
 * @note Listeners are executed in the reverse order in which they have been registered
 *
 * Example usage:
 * @code
 * static systick_listener_t my_listener;
 *
 * void my_callback(void) {
 *     // Called every 1ms from interrupt context
 * }
 *
 * int main(void) {
 *     systick_init();
 *     systick_register_listener(&my_listener, my_callback);
 * }
 * @endcode
 */
void systick_register_listener(systick_listener_t *listener, systick_listener_callback_t callback);

#endif

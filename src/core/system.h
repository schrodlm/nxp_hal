/**
 * @file system.h
 * @ingroup core
 * @brief System initialization convenience function
 *
 * Provides a single function to initialize all core system components in
 * the correct order.
 */

#ifndef CORE_SYSTEM_H
#define CORE_SYSTEM_H

/**
 * @brief Initialize all core system components
 *
 * Performs complete system initialization by calling initialization functions
 * for all core modules in the proper order:
 * 1. clock_init() - Configure system clock (40 MHz from external crystal)
 * 2. systick_init() - Set up 1ms system tick timer
 * 3. debug_uart_init() - Initialize debug UART at 1.6 Mbps
 *
 * After calling this function, the system is ready for application code with:
 * - Stable system clock
 * - Working millisecond timer and delays
 * - Functional printf/scanf via debug UART
 *
 * @note This is a convenience function - you can call individual init functions
 *       if you need different initialization order or selective initialization
 *
 * Example usage:
 * @code
 * int main(void) {
 *     system_init();
 *     printf("System initialized\n");
 *     // Application code here
 * }
 * @endcode
 */
void system_init(void);

#endif

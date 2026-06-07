/**
 * @file system.c
 * @ingroup core
 * @brief System initialization implementation
 *
 * Implements the system_init() convenience function that initializes all
 * core system components in the proper order.
 */

#include <core/system.h>
#include <core/clock.h>
#include <core/debug_uart.h>
#include <core/systick.h>

void system_init(void) {
    clock_init();
    systick_init();
    debug_uart_init();
}
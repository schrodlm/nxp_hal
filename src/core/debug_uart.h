/**
 * @file debug_uart.h
 * @ingroup core
 * @brief Debug UART interface
 *
 * This module provides a high-speed UART interface (LPUART1) for debugging
 * and console I/O at 1 Mbps. Supports printf/scanf via syscalls integration.
 */

#ifndef DEBUG_UART_H
#define DEBUG_UART_H

/**
 * @brief Initialize the debug UART (LPUART1)
 *
 * Configures LPUART1 at 1 Mbps (8N1) using the following pins:
 * - PTC7: TX (ALT2)
 * - PTC6: RX (ALT2)
 *
 * Configuration details:
 * - Baud rate: 1 Mbps
 * - Data format: 8 data bits, no parity, 1 stop bit
 * - Clock source: SOSCDIV1 (40 MHz)
 * - FIFOs: Enabled for both TX and RX
 */
void debug_uart_init(void);

/**
 * @brief Transmit a character via debug UART
 *
 * Sends a single character through LPUART1. This function blocks until
 * there is space in the transmit FIFO.
 *
 * @param ch Character to transmit
 *
 * @note Used by printf via syscalls
 * @note Blocks if TX FIFO is full (waits for space)
 */
void debug_uart_putchar(int ch);

/**
 * @brief Receive a character from debug UART
 *
 * Reads a single character from LPUART1. This function blocks until
 * a character is available in the receive buffer.
 *
 * @return Received character
 *
 * @note Used by scanf/getchar via syscalls
 * @note Blocks until data is available (RDRF flag set)
 */
int debug_uart_getchar(void);

#endif

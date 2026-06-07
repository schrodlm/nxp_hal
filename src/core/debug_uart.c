/**
 * @file debug_uart.c
 * @ingroup core
 * @brief Debug UART implementation
 *
 * Implements a high-speed (1 Mbps) debug UART using LPUART1 peripheral
 * with FIFO support.
 */

#include <core/debug_uart.h>

#include <S32K118/startup/system_S32K118.h>
#include <device_registers.h>

// Hardware configuration
#define DEBUG_UART_PORT PORTC
#define DEBUG_UART_PCC_INDEX PCC_PORTC_INDEX
#define DEBUG_UART_TX_PIN 7
#define DEBUG_UART_RX_PIN 6
#define DEBUG_UART_PIN_MUX 2  // ALT2

#define DEBUG_UART_IRQ_PRIORITY 0


void debug_uart_init(void) {
    // Enable clock for PORTC and LPUART1
    PCC->PCCn[DEBUG_UART_PCC_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_LPUART1_INDEX] = PCC_PCCn_PCS(1) | PCC_PCCn_CGC_MASK; // Use SOSCDIV1 (40MHz) as clock source

    // Configure PTC7 as LPUART1_TX (ALT2)
    DEBUG_UART_PORT->PCR[DEBUG_UART_TX_PIN] = PORT_PCR_MUX(DEBUG_UART_PIN_MUX);

    // Configure PTC6 as LPUART1_RX (ALT2)
    DEBUG_UART_PORT->PCR[DEBUG_UART_RX_PIN] = PORT_PCR_MUX(DEBUG_UART_PIN_MUX);

    // Disable LPUART1 transmitter and receiver before configuration
    LPUART1->CTRL &= ~(LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK);

    // Calculate baud rate divisor for 1Mbps
    // Baud rate = clock / (OSR + 1) / SBR
    // 1000000 = 40000000 / (19 + 1) / 2
    LPUART1->BAUD =
        LPUART_BAUD_OSR(19)        // Oversample ratio = 19+1 = 20
        | LPUART_BAUD_SBR(2)       // Baud rate divisor = 2
        | LPUART_BAUD_SBNS(0)      // One stop bit
        | LPUART_BAUD_BOTHEDGE(1)  // Both edges for better sampling at high speed
    ;

    // Enable transmit FIFO
    LPUART1->FIFO = LPUART_FIFO_TXFE(1) | LPUART_FIFO_RXFE(1);

    // Configure: 8-bit data, no parity
    LPUART1->CTRL = LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK;  // Enable transmitter, receiver
}

void debug_uart_putchar(int ch) {
    while (((LPUART1->WATER & LPUART_WATER_TXCOUNT_MASK) >> LPUART_WATER_TXCOUNT_SHIFT) == 4);
    LPUART1->DATA = ch;
}

int debug_uart_getchar(void) {
    // Wait until data is available in the receive buffer
    while (!(LPUART1->STAT & LPUART_STAT_RDRF_MASK));
    return LPUART1->DATA;
}

#include "device_registers.h"
#include <lib/spi.h>
#include <stdint.h>

#define SPI_SCK_PIN  0  // PTE0
#define SPI_MISO_PIN 1  // PTE1
#define SPI_MOSI_PIN 2  // PTE2

void spi_init(void) {
    // enable setup the clock source and clock gates for SPI ports
    PCC->PCCn[PCC_PORTE_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_LPSPI0_INDEX] = PCC_PCCn_PCS(1) | PCC_PCCn_CGC_MASK;

    //set up alternate functions
    PORTE->PCR[SPI_SCK_PIN] = PORT_PCR_MUX(2);
    PORTE->PCR[SPI_MISO_PIN] = PORT_PCR_MUX(2);
    PORTE->PCR[SPI_MOSI_PIN] = PORT_PCR_MUX(2);

    //reset the SPI peripheral (first we write reset, then release it)
    LPSPI0->CR = LPSPI_CR_RST_MASK;
    LPSPI0->CR = 0;

    //set SPI peripheral as the MASTER
    LPSPI0->CFGR1 = LPSPI_CFGR1_MASTER_MASK;

    //set the frequency the SPI bus runs on
    LPSPI0->CCR = LPSPI_CCR_SCKDIV(0) | LPSPI_CCR_DBT(1)  // 1-tick bus delay between back-to-back transfers
                  | LPSPI_CCR_PCSSCK(1)                   // 1-tick delay after CS goes LOW before first SCK edge
                  | LPSPI_CCR_SCKPCS(1);                  // 1-tick delay after last SCK edge before CS goes HIGH

    // set up flags for TX and RX buffer, desribing that both buffers are empty
    LPSPI0->FCR = LPSPI_FCR_TXWATER(0) | LPSPI_FCR_RXWATER(0);
    // enable the SPI module
    LPSPI0->CR = LPSPI_CR_MEN_MASK;
}

void spi_txrx(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t count, uint32_t pcs) {
    // 1. Push command into the FIFO
    LPSPI0->TCR = LPSPI_TCR_FRAMESZ(7)     // 8-bit frames (FRAMESZ+1)
                  | LPSPI_TCR_PRESCALE(2)  // 4 -> sck = 40 MHz / 4 / 2 = 5 MHz
                  | LPSPI_TCR_PCS(pcs);    // chip select

    // 2. per-byte-loop: push one TX, wait, pull one RX
    for (uint32_t i = 0; i < count; i++) {
        uint8_t tx = tx_buf ? tx_buf[i] : 0xFF;

        while (!(LPSPI0->SR & LPSPI_SR_TDF_MASK))
            ;  // wait for TX space
        LPSPI0->TDR = tx;

        while (!(LPSPI0->SR & LPSPI_SR_RDF_MASK))
            ;  //wait for RX byte
        uint8_t rx = LPSPI0->RDR;
        if (rx_buf) rx_buf[i] = rx;
    }

    // 3. Waut for frame complete, then clear the flag (write-1-to-clear)
    while (!(LPSPI0->SR & LPSPI_SR_FCF_MASK))
        ;
    LPSPI0->SR = LPSPI_SR_FCF_MASK;  // write-1-to-clear
}
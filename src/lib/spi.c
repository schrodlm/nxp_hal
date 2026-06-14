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
    // enable the SPI module; DBGEN keeps LPSPI running while halted in the
    // debugger so single-stepping doesn't abort in-flight frames.
    LPSPI0->CR = LPSPI_CR_MEN_MASK | LPSPI_CR_DBGEN_MASK;
}

void spi_txrx(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t count, uint32_t pcs) {
    if (count == 0) return;

    // Reset both FIFOs and clear status from any prior transfer.
    LPSPI0->CR |= LPSPI_CR_RTF_MASK | LPSPI_CR_RRF_MASK;
    LPSPI0->SR = LPSPI_SR_TCF_MASK | LPSPI_SR_FCF_MASK | LPSPI_SR_WCF_MASK;

    const uint32_t tcr_base = LPSPI_TCR_FRAMESZ(7)
                            | LPSPI_TCR_PRESCALE(2)
                            | LPSPI_TCR_PCS(pcs);

    // CONT=1: hold PCS low across all bytes in this call.
    LPSPI0->TCR = tcr_base | LPSPI_TCR_CONT_MASK;

    // Interleave TX and RX in the same loop. Critical RM rule (51.4.2.2):
    // in CONT mode the RX byte is only pushed into the RX FIFO when (a) the
    // next TX word is consumed, or (b) TCR is written with CONT=0. So we
    // must push the terminator TCR immediately after the last TDR without waiting for RDF
    // otherwise the last RX byte never appears and we deadlock.
    uint32_t tx_i = 0, rx_i = 0;
    while (rx_i < count) {
        if (tx_i < count && (LPSPI0->SR & LPSPI_SR_TDF_MASK)) {
            LPSPI0->TDR = tx_buf ? tx_buf[tx_i] : 0xFFu;
            tx_i++;
            if (tx_i == count) {
                // Last byte pushed -> queue the CONT=0 terminator immediately.
                LPSPI0->TCR = tcr_base;
            }
        }
        if (LPSPI0->SR & LPSPI_SR_RDF_MASK) {
            uint8_t rx = (uint8_t)LPSPI0->RDR;
            if (rx_buf) rx_buf[rx_i] = rx;
            rx_i++;
        }
    }

    // Wait until fully idle.
    while (!(LPSPI0->SR & LPSPI_SR_TCF_MASK)) ;
    LPSPI0->SR = LPSPI_SR_TCF_MASK | LPSPI_SR_FCF_MASK | LPSPI_SR_WCF_MASK;
}
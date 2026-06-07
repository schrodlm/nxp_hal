/**
 * @file spi.h
 * @ingroup lib
 * @brief SPI driver
 *
 * SPI driver with optional support for DMA.
 * Uses ports:
 *   PTE0 - SCK
 *   PTE1 - MISO
 *   PTE2 - MOSI
 *
 * This module sits between simple GPIO-style peripherals and more complex
 * devices such as the LCD or EEPROM. Students should expect to configure both
 * pin multiplexing and the LPSPI peripheral itself.
 */

#ifndef LIB_SPI_H
#define LIB_SPI_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize SPI hardware
 *
 * Configures SPI (LPSPI0) pins and peripheral. If SPI_WITH_DMA is defined, initializes DMA channels 0 and 1.
 *
 * NOTE: Due to relatively large capacity on the SPI signal lines keep the SPI clock below 3MHz.
 *
 * @note Must be called before using any other SPI functions
 *
 * High-level implementation outline:
 * 1. Enable clock gates for the SPI pin PORTs and for `LPSPI0` in `PCC`.
 * 2. In `PORTx->PCR[n]`, set the SCK/MISO/MOSI pins to the LPSPI alternate
 *    function.
 * 3. Disable `LPSPI0` before configuration.
 * 4. Configure the master settings in the LPSPI control/configuration
 *    registers (`CR`, `CFGR*`, `CCR`, FIFO/watermark registers).
 * 5. Set the clock polarity/phase and the divider so the resulting SPI clock
 *    matches the requirements of the connected devices.
 * 6. If `SPI_WITH_DMA` is enabled (optional), also configure:
 *    - DMA clocking
 *    - DMAMUX channel routing
 *    - DMA transfer control descriptors / channel settings
 *    - IRQ enable for the DMA/LPSPI completion path
 * 7. Re-enable `LPSPI0`.
 *
 * What to read in the reference manual:
 * - PCC chapter: clock gating and clock source selection
 * - PORT chapter: pin multiplexing for the LPSPI signals
 * - LPSPI chapter: master configuration, transfer command register, FIFO flags
 * - DMAMUX and DMA chapters if building the DMA variant
 */
void spi_init(void);

/**
 * @brief Check if SPI is idle
 *
 * @return true if SPI is idle, false if a DMA transfer is in progress
 *
 * High-level implementation outline:
 * 1. In the blocking build, this can simply report "idle".
 * 2. In the DMA build, this should consult the module's software state that
 *    tracks whether TX/RX DMA activity is still in progress.
 *
 * @note This function is mainly a software guard around asynchronous use of
 *       the SPI bus; it is not a full hardware bus-status query.
 */
bool spi_is_idle(void);

#ifdef SPI_WITH_DMA
// DMA parts are not compulsory. They are optional for those who want to try more.

typedef void (*spi_dma_callback_t)(void);

/**
 * @brief Queue a DMA transfer over SPI
 *
 * The function is non blocking. If it returned true, it will call the respective callback once the TX/RX is complete.
 *
 * The DMA transfer transfers data by uint32_t. Thus tx_buf and rx_buf must be aligned to 4 bytes (using `uint8_t buf[] __attribute__((aligned(4))) = {...}`).
 *
 * This is achieved using the following settings:
 * - SPI is configured to transfer real_count * 8 bits
 * - DMA is configured transfer actual_count bytes in total (minor loop * major loop)
 * - DMA uses 4 bytes transfer for both source and destination.
 * - The minor loop size is 4 bytes, processed in 1 transfer step.
 * - Byte swap is enabled. This is because ARM Cortex M is little endian. This is also the reason why the last
 *   incomplete word has to be right aligned. After being swapped, the relevant bytes must appear on the left before
 *   the cutoff by the real_count limit.
 *
 * @param tx_buf Pointer to data buffer to transmit (can be NULL if only receiving). It must be aligned to 4 bytes.
 * @param rx_buf Pointer to data buffer to receive into (can be NULL if only transmitting). It must be aligned to 4 bytes.
 * @param count Number of bytes to transfer over SPI
 * @param pcs SPI peripheral chip select number (0..2)
 * @param tx_callback Callback function to execute after DMA transfer completes
 * @param rx_callback Callback function to execute after DMA transfer completes
 * @return true if DMA transfer queued successfully, false if DMA is busy
 *
 * High-level implementation outline:
 * 1. Reject the request if a previous asynchronous transfer is still active.
 * 2. Configure the LPSPI transfer parameters for this transaction in `TCR`:
 *    frame size, CPOL/CPHA, prescaler, and PCS selection.
 * 3. Prepare DMA state for TX and RX, including the special handling of a
 *    final incomplete word if the byte count is not divisible by 4.
 * 4. Configure the TX and RX DMA channels to move data between memory and the
 *    LPSPI data registers.
 * 5. Start the DMA channels and let the completion IRQ path invoke the
 *    supplied callbacks.
 *
 * What to read in the reference manual:
 * - LPSPI chapter: `TCR`, data registers, FIFO behaviour, DMA request enables
 * - DMAMUX chapter: mapping a peripheral request to a DMA channel
 * - DMA chapter: source/destination sizes, minor/major loops, interrupts
 */
bool spi_dma_txrx(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t count, uint32_t pcs, spi_dma_callback_t tx_callback, spi_dma_callback_t rx_callback);

#else

/**
 * @brief Write/read data over SPI in blocking mode
 *
 * The function is blocking and will not return until the transfer is complete.
 *
 * @param tx_buf Pointer to data buffer to transmit (can be NULL if only receiving).
 * @param rx_buf Pointer to data buffer to receive into (can be NULL if only transmitting).
 * @param count Number of bytes to transmit/receive.
 * @param pcs SPI peripheral chip select number (0..2)
 *
 * High-level implementation outline:
 * 1. Configure the LPSPI transfer command register (`TCR`) for this
 *    transaction: frame size, CPOL/CPHA, prescaler, and PCS.
 * 2. Feed transmit data into the SPI TX data path whenever the TX flag says
 *    space is available.
 * 3. Read received data from the RX data path whenever the RX flag says data
 *    is available.
 * 4. Wait until the frame-complete condition indicates the whole transaction
 *    has finished, then clear the completion flag.
 *
 * What to read in the reference manual:
 * - LPSPI chapter: `TCR`, `TDR`, `RDR`, status flags, frame complete flag
 */
void spi_txrx(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t count, uint32_t pcs);

#endif

#endif

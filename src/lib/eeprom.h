/**
 * @file eeprom.h
 * @ingroup lib
 * @brief M95080 SPI EEPROM driver
 *
 * Provides hardware abstraction for M95080-RMN6TP 1KB SPI EEPROM.
 * Operates on whole 32-byte pages.
 *
 * Two build variants are available depending on the SPI driver mode:
 * - If `SPI_WITH_DMA` is defined, operations are non-blocking and completion
 *   is signalled via a callback. `eeprom_init()` registers a systick listener
 *   used for WIP polling between write and ready states.
 * - Otherwise, operations are blocking; they return only after the SPI transfer
 *   (and, for writes, the WIP polling loop) has completed.
 *
 * Students should treat this module as "protocol on top of SPI": the key work
 * is no longer just configuring registers, but building the right command
 * sequence for the external device.
 */

#ifndef LIB_EEPROM_H
#define LIB_EEPROM_H

#include <stdbool.h>
#include <stdint.h>
#include <lib/spi.h>

/** @brief EEPROM page size in bytes */
#define EEPROM_PAGE_SIZE  32

/** @brief Total number of pages (1024 bytes / 32 bytes per page) */
#define EEPROM_PAGE_COUNT 32

/**
 * @brief Initialize EEPROM hardware
 *
 * Configures the EEPROM chip-select GPIO pin (LPSPI0 PCS2). Note that the
 * SPI peripheral itself must be initialized separately via `spi_init()`.
 *
 * If `SPI_WITH_DMA` is defined, additionally registers a systick listener
 * used for WIP polling during asynchronous write operations.
 *
 * @note Must be called before using any other EEPROM functions
 *
 * High-level implementation outline:
 * 1. Make sure the shared SPI peripheral has already been initialized.
 * 2. Enable the clock gate for the PORT containing the EEPROM chip-select pin.
 * 3. Configure that pin to the LPSPI alternate function corresponding to the
 *    chosen PCS line.
 * 4. In the DMA build (optional), initialize the module state machine and register a
 *    SysTick listener that will be used to poll write completion.
 *
 * What to read in the reference manual:
 * - PCC and PORT chapters: clock gating and PCS pin multiplexing
 * - LPSPI chapter: hardware-controlled chip select
 * - EEPROM datasheet: command set (`WREN`, `READ`, `WRITE`, `RDSR`)
 */
void eeprom_init(void);

#ifdef SPI_WITH_DMA
// DMA parts are not compulsory. They are optional for those who want to try more.

/**
 * @brief Read a page from EEPROM
 *
 * Starts a non-blocking DMA transfer to read 32 bytes from the specified
 * page. The data is copied into the provided buffer when the transfer
 * completes.
 *
 * If the EEPROM is not idle (previous read or write in progress), it returns false.
 *
 * @param page Page index (0 to EEPROM_PAGE_COUNT-1)
 * @param buf  Buffer to receive the data (must be at least EEPROM_PAGE_SIZE bytes)
 * @param callback Callback function to be called when read is complete
 * @return true if read operation was started, false if EEPROM is busy
 *
 * High-level implementation outline:
 * 1. Check the module state and reject the request if a previous operation is
 *    still running.
 * 2. Validate the page number.
 * 3. Build the SPI command frame: `READ` opcode + address bytes + dummy bytes
 *    needed to clock data out of the EEPROM.
 * 4. Start an SPI DMA transfer using the shared SPI driver.
 * 5. When the transfer completes, copy the received payload bytes from the SPI
 *    buffer into the user buffer and call the callback.
 *
 * What to read in the reference manual:
 * - no new MCU peripheral chapter beyond SPI/DMA is needed here
 * - EEPROM datasheet: read command format and address width
 */
bool eeprom_read_page(uint32_t page, uint8_t buf[EEPROM_PAGE_SIZE], void (*callback)(void));

/**
 * @brief Write a page to EEPROM
 *
 * Starts a non-blocking DMA transfer to write 32 bytes to the specified
 * page. The write sequence includes WREN command, page write, and
 * automatic WIP polling until the write cycle completes.
 *
 * If the EEPROM is not idle (previous read or write in progress), it returns false.
 *
 * @param page Page index (0 to EEPROM_PAGE_COUNT-1)
 * @param buf  Buffer containing data to write (must be at least EEPROM_PAGE_SIZE bytes)
 * @param callback Callback function to be called when write is complete
 * @return true if write operation was started, false if EEPROM is busy
 *
 * High-level implementation outline:
 * 1. Check the module state and validate the page number.
 * 2. Start with the `WREN` command so the EEPROM accepts a following write.
 * 3. Send the `WRITE` command frame containing opcode, address, and page data.
 * 4. After the write frame has been sent, poll the EEPROM status register
 *    (`RDSR`) until the WIP bit indicates the internal write cycle is done.
 * 5. Once ready, return to the idle state and call the callback.
 *
 * What to read in the reference manual:
 * - SPI/DMA chapters for the transport mechanism
 * - EEPROM datasheet: write-enable latch, page write sequence, status polling
 */
bool eeprom_write_page(uint32_t page, const uint8_t buf[EEPROM_PAGE_SIZE], void (*callback)(void));

#else

/**
 * @brief Read a page from EEPROM (blocking)
 *
 * Reads 32 bytes from the specified page. The data is copied into the provided buffer.
 * The call returns only after the SPI transfer has completed.
 *
 * If `page` is out of range (>= EEPROM_PAGE_COUNT) the function returns without
 * touching the buffer.
 *
 * @param page Page index (0 to EEPROM_PAGE_COUNT-1)
 * @param buf  Buffer to receive the data (must be at least EEPROM_PAGE_SIZE bytes)
 *
 * High-level implementation outline:
 * 1. Validate the page number.
 * 2. Build the `READ` command frame: opcode + address + dummy bytes.
 * 3. Perform one blocking SPI transaction for the whole frame.
 * 4. Copy the received payload bytes into the user buffer.
 *
 * What to read in the reference manual:
 * - LPSPI chapter for the blocking transfer path
 * - EEPROM datasheet: read command format
 */
void eeprom_read_page(uint32_t page, uint8_t buf[EEPROM_PAGE_SIZE]);

/**
 * @brief Write a page to EEPROM (blocking)
 *
 * Performs a write of 32 bytes to the specified page. The write sequence
 * includes the WREN command, page write, and automatic WIP polling until the
 * write cycle completes. The call returns only after the write is fully finished.
 *
 * @param page Page index (0 to EEPROM_PAGE_COUNT-1). The value is not validated;
 *             the caller is responsible for passing a valid index.
 * @param buf  Buffer containing data to write (must be at least EEPROM_PAGE_SIZE bytes)
 *
 * High-level implementation outline:
 * 1. Send `WREN` in a first SPI transaction.
 * 2. Build and send the `WRITE` command frame containing opcode, address, and
 *    page data.
 * 3. Repeatedly send `RDSR` and inspect the returned WIP bit until the EEPROM
 *    reports that the internal write cycle has finished.
 *
 * What to read in the reference manual:
 * - LPSPI chapter for the blocking transfer path
 * - EEPROM datasheet: write cycle and status polling
 */
void eeprom_write_page(uint32_t page, const uint8_t buf[EEPROM_PAGE_SIZE]);

#endif

#endif

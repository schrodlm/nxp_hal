#include "core/systick.h"
#include <lib/eeprom.h>

#include <device_registers.h>
#include <lib/spi.h>
#include <string.h>

#define EEPROM_CS_PIN           6  // PTE6, MUX=2 -> LPSPI0_PCS2
#define LPSPI0_PCS              2

#define EEPROM_HEADER_SIZE      3

#define EEPROM_WRITE_CMD        0x02
#define EEPROM_READ_CMD         0x03
#define EEPROM_RDSR_CMD         0x05
#define EEPROM_WRITE_ENABLE_CMD 0x06

void eeprom_init(void) {
    PCC->PCCn[PCC_PORTE_INDEX] = PCC_PCCn_CGC_MASK;
    // Route PTE6 to LPSPI0_PCS2, LPSPI auto-toggles CS per spi_txrx call.
    PORTE->PCR[EEPROM_CS_PIN] = PORT_PCR_MUX(2);
}

void eeprom_read_page(uint32_t page, uint8_t buf[EEPROM_PAGE_SIZE]) {
    if (page >= EEPROM_PAGE_COUNT) return;

    uint16_t addr = page * EEPROM_PAGE_SIZE;
    uint8_t tx[EEPROM_HEADER_SIZE + EEPROM_PAGE_SIZE] = {
        EEPROM_READ_CMD,
        (addr >> 8) & 0xFF,
        addr & 0xFF,
    };
    uint8_t rx[EEPROM_HEADER_SIZE + EEPROM_PAGE_SIZE];

    spi_txrx(tx, rx, sizeof(tx), LPSPI0_PCS);
    memcpy(buf, rx + EEPROM_HEADER_SIZE, EEPROM_PAGE_SIZE);
}

void eeprom_write_page(uint32_t page, const uint8_t buf[EEPROM_PAGE_SIZE]) {
    uint16_t addr = page * EEPROM_PAGE_SIZE;

    // 1. WREN
    uint8_t wren = EEPROM_WRITE_ENABLE_CMD;
    spi_txrx(&wren, 0, 1, LPSPI0_PCS);

    // 2. WRITE + address + 32 data bytes
    uint8_t write_buf[EEPROM_HEADER_SIZE + EEPROM_PAGE_SIZE] = {
        EEPROM_WRITE_CMD,
        (addr >> 8) & 0xFF,
        addr & 0xFF,
    };
    memcpy(write_buf + EEPROM_HEADER_SIZE, buf, EEPROM_PAGE_SIZE);
    spi_txrx(write_buf, 0, sizeof(write_buf), LPSPI0_PCS);

    // 3. Poll RDSR until WIP=0
    uint8_t rdsr_tx[2] = {EEPROM_RDSR_CMD, 0};
    uint8_t rdsr_rx[2];
    do {
        spi_txrx(rdsr_tx, rdsr_rx, 2, LPSPI0_PCS);
        systick_delay_ms(1);
    } while (rdsr_rx[1] & 0x01);
}
/**
 * @file sht40.c
 * @brief SHT40 temperature & humidity sensor — blocking I2C driver.
 *
 * Pin map: PTA2=SDA, PTA3=SCL (both MUX=3 -> LPI2C0).
 * Sensor 7-bit I2C address: 0x44 or 0x45 depending on variant. This board
 * uses 0x44 — change SHT40_ADDR if your variant responds elsewhere.
 *
 * One measurement cycle:
 *   1. Master writes one command byte (0xFD = "measure T+RH, high precision").
 *   2. Wait ~10 ms for the sensor's internal conversion.
 *   3. Master reads 6 bytes: T_msb, T_lsb, T_crc, RH_msb, RH_lsb, RH_crc.
 *   4. Convert raw 16-bit values to milli-units per datasheet formulas.
 *
 * LPI2C transport:
 *   The TX FIFO accepts "command words" (MTDR). Each write encodes both an
 *   opcode (CMD field) and a data byte. We only use a few opcodes:
 *     CMD=0 — TX DATA (send byte)
 *     CMD=1 — RX DATA (receive DATA+1 bytes)
 *     CMD=2 — GENERATE STOP
 *     CMD=4 — GENERATE START + send address byte
 *   The address byte's bit 0 is the R/W bit (0=write, 1=read).
 */

#include <lib/sht40.h>

#include <core/systick.h>
#include <device_registers.h>

#define SHT40_ADDR      0x45   // 7-bit I2C address
#define SHT40_CMD_MEAS  0xFD   // high-precision T+RH measurement
#define SHT40_T_MS      10     // max conversion time per datasheet

#define I2C_SDA_PIN     2      // PTA2
#define I2C_SCL_PIN     3      // PTA3

// Helper macros for MTDR command words
#define MTDR_TX(byte)     ((0u << 8) | ((byte) & 0xFF))
#define MTDR_RX(n)        ((1u << 8) | (((n) - 1) & 0xFF))   // request n bytes
#define MTDR_STOP         (2u << 8)
#define MTDR_START(addr_rw) ((4u << 8) | ((addr_rw) & 0xFF))

static sht40_result_t cached = { 0 };

void sht40_init(void) {
    // Clocks: PORTA + LPI2C0. PCS=1 -> SOSCDIV1 (40 MHz).
    PCC->PCCn[PCC_PORTA_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_LPI2C0_INDEX] = PCC_PCCn_PCS(1) | PCC_PCCn_CGC_MASK;

    // PTA2/PTA3 -> ALT3 = LPI2C0 SDA/SCL.
    PORTA->PCR[I2C_SDA_PIN] = PORT_PCR_MUX(3);
    PORTA->PCR[I2C_SCL_PIN] = PORT_PCR_MUX(3);

    // LPI2C0 master mode setup.
    // Reset, then clear MEN so we can configure.
    LPI2C0->MCR = LPI2C_MCR_RST_MASK;
    LPI2C0->MCR = 0;

    // Bus timing: ~100 kHz @ 40 MHz LPI2C clock.
    //   prescaler = 2  -> functional clock = 40 MHz / 4 = 10 MHz
    //   CLKHI + CLKLO = 100 -> 100 kHz I2C bit period
    // SETHOLD/DATAVD chosen conservatively (datasheet allows wide ranges).
    LPI2C0->MCFGR1 = LPI2C_MCFGR1_PRESCALE(2);
    LPI2C0->MCCR0  = LPI2C_MCCR0_CLKHI(40)
                   | LPI2C_MCCR0_CLKLO(50)
                   | LPI2C_MCCR0_SETHOLD(40)
                   | LPI2C_MCCR0_DATAVD(25);

    // Enable master.
    LPI2C0->MCR = LPI2C_MCR_MEN_MASK;
}

static void i2c_wait_tx_space(void) {
    while (!(LPI2C0->MSR & LPI2C_MSR_TDF_MASK)) ;
}

static uint8_t i2c_wait_rx(void) {
    while (!(LPI2C0->MSR & LPI2C_MSR_RDF_MASK)) ;
    return (uint8_t)LPI2C0->MRDR;
}

static void i2c_write_cmd(uint8_t cmd) {
    // START + address(write) + 1 data byte + STOP.
    i2c_wait_tx_space();
    LPI2C0->MTDR = MTDR_START((SHT40_ADDR << 1) | 0);  // write
    i2c_wait_tx_space();
    LPI2C0->MTDR = MTDR_TX(cmd);
    i2c_wait_tx_space();
    LPI2C0->MTDR = MTDR_STOP;
}

static void i2c_read_n(uint8_t *buf, uint32_t n) {
    // START + address(read) + RX(n) + STOP.
    i2c_wait_tx_space();
    LPI2C0->MTDR = MTDR_START((SHT40_ADDR << 1) | 1);  // read
    i2c_wait_tx_space();
    LPI2C0->MTDR = MTDR_RX(n);
    i2c_wait_tx_space();
    LPI2C0->MTDR = MTDR_STOP;

    for (uint32_t i = 0; i < n; i++) {
        buf[i] = i2c_wait_rx();
    }
}

sht40_result_t sht40_get_result(void) {
    // 1. Send measurement command.
    i2c_write_cmd(SHT40_CMD_MEAS);

    // 2. Wait for conversion.
    systick_delay_ms(SHT40_T_MS);

    // 3. Read 6 bytes: T_msb, T_lsb, T_crc, RH_msb, RH_lsb, RH_crc.
    uint8_t buf[6];
    i2c_read_n(buf, sizeof(buf));

    // 4. Combine MSB/LSB into raw 16-bit values; ignore CRC bytes.
    uint16_t raw_t = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_h = ((uint16_t)buf[3] << 8) | buf[4];

    // 5. Convert to milli-units per SHT40 datasheet:
    //   T_C  = -45 + 175 * raw_t / 65535
    //   RH%  = -6  + 125 * raw_h / 65535  (clamp to 0..100)
    int32_t t_milli  = -45000 + (int32_t)((175000LL * raw_t) / 65535);
    int32_t rh_milli = -6000  + (int32_t)((125000LL * raw_h) / 65535);
    if (rh_milli < 0)       rh_milli = 0;
    if (rh_milli > 100000)  rh_milli = 100000;

    cached.temperature = t_milli;
    cached.humidity    = rh_milli;
    cached.raw_temp    = raw_t;
    cached.raw_hum     = raw_h;
    cached.valid       = true;

    return cached;
}

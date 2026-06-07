#include <core/system.h>
#include <core/systick.h>
#include <device_registers.h>
#include <lib/eeprom.h>
#include <lib/spi.h>
#include <unity.h>

#include <string.h>

/** @brief EEPROM page used for testing */
#define TEST_PAGE 0

static uint8_t data_out[EEPROM_PAGE_SIZE];
static uint8_t data_in[EEPROM_PAGE_SIZE];

/**
 * @brief Write and read back a sequential pattern
 *
 * **Test ID:** EEPROM-01
 *
 * **Preconditions:**
 * - System, SPI, and EEPROM initialized
 *
 * **Steps:**
 * 1. Fill output buffer with sequential pattern (0x10, 0x11, ...)
 * 2. Write the buffer to EEPROM page
 * 3. Clear the input buffer
 * 4. Read the page back from EEPROM
 * 5. Compare output and input buffers
 *
 * **Success Criteria:**
 * - All 32 bytes read back match the written data
 */
void test_write_read_sequential(void) {
    for (int i = 0; i < EEPROM_PAGE_SIZE; i++) {
        data_out[i] = (uint8_t)(i + 0x10);
    }

    eeprom_write_page(TEST_PAGE, data_out);

    memset(data_in, 0, EEPROM_PAGE_SIZE);
    eeprom_read_page(TEST_PAGE, data_in);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(data_out, data_in, EEPROM_PAGE_SIZE);
}

/**
 * @brief Write and read back all 0xFF bytes
 *
 * **Test ID:** EEPROM-02
 *
 * **Preconditions:**
 * - System, SPI, and EEPROM initialized
 *
 * **Steps:**
 * 1. Fill output buffer with 0xFF
 * 2. Write the buffer to EEPROM page
 * 3. Clear the input buffer to 0x00
 * 4. Read the page back from EEPROM
 * 5. Compare output and input buffers
 *
 * **Success Criteria:**
 * - All 32 bytes read back are 0xFF
 */
void test_write_read_all_ff(void) {
    memset(data_out, 0xFF, EEPROM_PAGE_SIZE);

    eeprom_write_page(TEST_PAGE, data_out);

    memset(data_in, 0x00, EEPROM_PAGE_SIZE);
    eeprom_read_page(TEST_PAGE, data_in);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(data_out, data_in, EEPROM_PAGE_SIZE);
}

/**
 * @brief Write and read back all 0x00 bytes
 *
 * **Test ID:** EEPROM-03
 *
 * **Preconditions:**
 * - System, SPI, and EEPROM initialized
 *
 * **Steps:**
 * 1. Fill output buffer with 0x00
 * 2. Write the buffer to EEPROM page
 * 3. Fill the input buffer with 0xFF
 * 4. Read the page back from EEPROM
 * 5. Compare output and input buffers
 *
 * **Success Criteria:**
 * - All 32 bytes read back are 0x00
 */
void test_write_read_all_00(void) {
    memset(data_out, 0x00, EEPROM_PAGE_SIZE);

    eeprom_write_page(TEST_PAGE, data_out);

    memset(data_in, 0xFF, EEPROM_PAGE_SIZE);
    eeprom_read_page(TEST_PAGE, data_in);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(data_out, data_in, EEPROM_PAGE_SIZE);
}

/**
 * @brief Overwrite a page and verify new data
 *
 * **Test ID:** EEPROM-04
 *
 * **Preconditions:**
 * - System, SPI, and EEPROM initialized
 *
 * **Steps:**
 * 1. Write a page with pattern A (0xAA)
 * 2. Write the same page with pattern B (0x55)
 * 3. Read the page back
 * 4. Verify the data matches pattern B
 *
 * **Success Criteria:**
 * - Read data matches the second write (pattern B), not the first
 */
void test_overwrite(void) {
    memset(data_out, 0xAA, EEPROM_PAGE_SIZE);
    eeprom_write_page(TEST_PAGE, data_out);

    memset(data_out, 0x55, EEPROM_PAGE_SIZE);
    eeprom_write_page(TEST_PAGE, data_out);

    memset(data_in, 0x00, EEPROM_PAGE_SIZE);
    eeprom_read_page(TEST_PAGE, data_in);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(data_out, data_in, EEPROM_PAGE_SIZE);
}

void setUp(void) {
}

void tearDown(void) {
}

int main(void) {
    system_init();
    spi_init();
    eeprom_init();

    UNITY_BEGIN();

    RUN_TEST(test_write_read_sequential);
    RUN_TEST(test_write_read_all_ff);
    RUN_TEST(test_write_read_all_00);
    RUN_TEST(test_overwrite);

    return UNITY_END();
}

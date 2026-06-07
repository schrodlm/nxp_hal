/**
 * @file sht40.h
 * @ingroup lib
 * @brief SHT40 I2C temperature and humidity sensor driver
 *
 * Provides hardware abstraction for SHT40 temperature/humidity sensor
 * with asynchronous periodic measurement using LPIT channel 0.
 * Measurements are performed automatically every ~1 second.
 *
 * Pin configuration:
 * - I2C SCL: PTA3 (LPI2C0_SCL)
 * - I2C SDA: PTA2 (LPI2C0_SDA)
 *
 * This module combines two peripherals:
 * - `LPI2C0` transports commands/data to the external sensor
 * - `LPIT0` provides the periodic schedule for alternating "start measurement"
 *   and "read result" phases
 */

#ifndef LIB_SHT40_H
#define LIB_SHT40_H

#include <stdbool.h>
#include <stdint.h>

/** @brief SHT40 measurement result */
typedef struct {
    int32_t temperature;  /**< Temperature in millidegrees Celsius (e.g., 25123 = 25.123 °C) */
    int32_t humidity;     /**< Relative humidity in milli-percent (e.g., 45200 = 45.200 %) */
    uint16_t raw_temp;    /**< Raw 16-bit temperature value from sensor */
    uint16_t raw_hum;     /**< Raw 16-bit humidity value from sensor */
    bool valid;           /**< Measurement validity flag */
} sht40_result_t;

/**
 * @brief Initialize SHT40 sensor
 *
 * Configures LPI2C0 peripheral, GPIO pins, and LPIT channel 0 for
 * periodic asynchronous measurement. After initialization, measurements
 * are performed automatically every ~1 second.
 *
 * @note Must be called after clock_init()
 *
 * High-level implementation outline:
 * 1. Enable clock gates for the PORT containing the I2C pins.
 * 2. Configure SDA and SCL pins to the LPI2C alternate function.
 * 3. Enable the clock gate for `LPI2C0` and choose its clock source in `PCC`.
 * 4. Configure the LPI2C master timing registers for the target bus speed.
 * 5. Enable LPI2C master mode.
 * 6. Enable the clock gate for `LPIT0`, configure one channel in periodic or
 *    one-shot style, and enable its interrupt in both LPIT and NVIC.
 * 7. Initialize the driver state/result storage.
 * 8. Kick off the first measurement cycle: send the measure command and arm
 *    the timer that will later trigger the readout.
 *
 * What to read in the reference manual:
 * - PCC and PORT chapters: pin mux and clocking
 * - LPI2C chapter: master command FIFO, timing registers, status flags
 * - LPIT chapter: timer channel configuration and interrupt generation
 * - SHT40 datasheet: measurement command and data format
 */
void sht40_init(void);

/**
 * @brief Get the last measured result
 *
 * Returns the most recent temperature and humidity measurement.
 * This function does not trigger a new measurement - it returns
 * the cached result from the last automatic measurement cycle.
 *
 * @return Last measured temperature and humidity values.
 *         Returns zeros until the first measurement completes (~10ms after init).
 *
 * High-level implementation outline:
 * 1. Return the most recent result structure stored by the background
 *    measurement state machine.
 *
 * @note This function does not talk to the sensor directly. The I2C activity
 *       happens asynchronously in the background logic.
 */
sht40_result_t sht40_get_result(void);

#endif

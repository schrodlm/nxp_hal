#include <S32K118/startup/system_S32K118.h>
#include <core/system.h>
#include <core/systick.h>
#include <device_registers.h>
#include <lib/led.h>
#include <unity.h>

/**
 * Common test setup:
 * - PTB13 (aka GPIO_6) must be physically wired to LED_GREEN (PTC0). PTC8 is referred to as test pin in the test specification.
 */


// PTB13 is used to read LED_GREEN state (PTC0)
#define TEST_PIN_PORT PORTB
#define TEST_PIN_PORT_INDEX PCC_PORTB_INDEX
#define TEST_PIN_GPIO PTB
#define TEST_PIN 13

// Helper function to read test pin state
static bool read_test_pin(void) {
    return (TEST_PIN_GPIO->PDIR & (1 << TEST_PIN)) != 0;
}

/**
 * @brief LED toggle test
 *
 * **Test ID:** LED-001
 *
 * **Preconditions:**
 * - System initialized (system_init, led_init)
 * - Test pin configured as GPIO input
 * - Test pin hardwired to LED_GREEN (PTC0)
 * - LED_GREEN initially off
 *
 * **Steps:**
 * 1. Set LED_GREEN to off state
 * 2. Wait 1ms and verify test pin reads 0
 * 3. Set LED_GREEN to on state
 * 4. Wait 1ms and verify test pin reads 1
 * 5. Set LED_GREEN to off state
 * 6. Wait 1ms and verify test pin reads 0
 *
 * **Success Criteria:**
 * - Test pin correctly follows LED state changes
 */
void test_led_toggle(void) {
    // Start with LED off
    led_set_state(LED_GREEN, false);
    systick_delay_ms(1);
    TEST_ASSERT_FALSE(read_test_pin());

    // Turn LED on
    led_set_state(LED_GREEN, true);
    systick_delay_ms(1);
    TEST_ASSERT_TRUE(read_test_pin());

    // Turn LED off again
    led_set_state(LED_GREEN, false);
    systick_delay_ms(1);
    TEST_ASSERT_FALSE(read_test_pin());
}

void setUp(void) {
    // Ensure LED is off before each test
    led_set_state(LED_GREEN, false);
    systick_delay_ms(1);
}

void tearDown(void) {
    // Clean up - turn LED off
    led_set_state(LED_GREEN, false);
}

int main(void) {
    // Initialize system
    system_init();
    led_init();

    // Configure test pin as GPIO input for testing
    PCC->PCCn[TEST_PIN_PORT_INDEX] = PCC_PCCn_CGC_MASK;
    TEST_PIN_GPIO->PDDR &= ~(1 << TEST_PIN);  // Set as input
    TEST_PIN_PORT->PCR[TEST_PIN] = PORT_PCR_MUX(1);  // GPIO function

    UNITY_BEGIN();

    RUN_TEST(test_led_toggle);

    return UNITY_END();
}

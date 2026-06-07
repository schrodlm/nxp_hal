#include <S32K118/startup/system_S32K118.h>
#include <core/system.h>
#include <core/systick.h>
#include <device_registers.h>
#include <lib/btn.h>
#include <lib/btn_cnt.h>
#include <unity.h>

/**
 * Common test setup:
 * - PTE5 (aka GPIO_3) must be physically wired to BTN_1 (PTD15). PTE5 is referred to as test pin in the test specification.
 */

// PTE5 is used to simulate button pulses on PTD15 (BTN_1)
#define TEST_PIN_PORT PORTE
#define TEST_PIN_PORT_INDEX PCC_PORTE_INDEX
#define TEST_PIN_GPIO PTE
#define TEST_PIN 5

#define NOPon() NOP(); NOP(); NOP(); NOP(); NOP()
#define NOPoff() NOP(); NOP(); NOP(); NOP(); NOP()

/**
 * @brief Short pulse series debouncing test
 *
 * **Test ID:** BTN-CNT-01
 *
 * **Preconditions:**
 * - System initialized
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 * - BTN_1 counter reset to 0
 * - Test pin initially low
 *
 * **Steps:**
 * 1. Generate 10 consecutive short pulses on the test pin
 * 2. Wait 10ms for interrupt processing
 * 3. Read BTN_1 press count
 *
 * **Success Criteria:**
 * - BTN_1 press count equals exactly 1
 */
void test_short_pulse_series(void) {
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    NOPon();
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    NOPoff();

    // Wait a bit for ISR to process
    systick_delay_ms(10);

    // Should register as exactly one button press
    TEST_ASSERT_EQUAL_UINT8(1, btn_cnt_get_count(BTN_1));
}

/**
 * @brief Single 1ms pulse detection test
 *
 * **Test ID:** BTN-CNT-02
 *
 * **Preconditions:**
 * - System initialized
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 * - BTN_1 counter reset to 0
 * - Test pin initially low
 *
 * **Steps:**
 * 1. Set test pin high
 * 2. Wait 1ms (button held)
 * 3. Set test pin low
 * 4. Wait 10ms for interrupt processing
 * 5. Read BTN_1 press count
 *
 * **Success Criteria:**
 * - BTN_1 press count equals exactly 1
 * - Single clean 1ms pulse correctly detected as one press
 */
void test_1ms_pulse(void) {
    // Generate 1ms pulse
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    systick_delay_ms(1);
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);

    // Wait for ISR to process
    systick_delay_ms(10);

    // Should register as exactly one button press
    TEST_ASSERT_EQUAL_UINT8(1, btn_cnt_get_count(BTN_1));
}

/**
 * @brief Debounce timeout test - two separate presses
 *
 * **Test ID:** BTN-CNT-03
 *
 * **Preconditions:**
 * - System initialized
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 * - BTN_1 counter reset to 0
 * - Test pin initially low
 *
 * **Steps:**
 * 1. Generate first pulse: Set test pin high for 1ms, then low
 * 2. Wait 105ms (debounce period + 5ms margin)
 * 3. Generate second pulse: Set test pin high for 1ms, then low
 * 4. Wait 10ms for interrupt processing
 * 5. Read BTN_1 press count
 *
 * **Success Criteria:**
 * - BTN_1 press count equals exactly 2
 */
void test_two_pulses_105ms_apart(void) {
    // First pulse
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    systick_delay_ms(1);
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);

    // Wait for debounce period + 5ms margin (105ms total)
    systick_delay_ms(105);

    // Second pulse
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    systick_delay_ms(1);
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);

    // Wait for ISR to process
    systick_delay_ms(10);

    // Should register as exactly two button presses
    TEST_ASSERT_EQUAL_UINT8(2, btn_cnt_get_count(BTN_1));
}

void setUp(void) {
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);

    // Wait for settling
    systick_delay_ms(150);

    btn_cnt_reset_count(BTN_1);
}

void tearDown(void) {
}

int main(void) {
    // Initialize system
    system_init();
    btn_init();
    btn_cnt_init();

    // Configure test pin as GPIO output
    PCC->PCCn[TEST_PIN_PORT_INDEX] = PCC_PCCn_CGC_MASK;
    TEST_PIN_GPIO->PDDR |= (1 << TEST_PIN);
    TEST_PIN_PORT->PCR[TEST_PIN] = PORT_PCR_MUX(1);

    UNITY_BEGIN();
    RUN_TEST(test_short_pulse_series);
    RUN_TEST(test_1ms_pulse);
    RUN_TEST(test_two_pulses_105ms_apart);
    return UNITY_END();
}

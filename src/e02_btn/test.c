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

// Test listener state
static volatile uint32_t test_interrupt_count = 0;

// Test interrupt callback
static void test_interrupt_callback(void) {
    test_interrupt_count++;
    btn_clear_isfr();
}

/**
 * @brief Button state reading test
 *
 * **Test ID:** BTN-01
 *
 * **Preconditions:**
 * - System initialized
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 *
 * **Steps:**
 * 1. Set test pin low, read BTN_1 state
 * 2. Set test pin high, read BTN_1 state
 * 3. Set test pin low again, read BTN_1 state
 *
 * **Success Criteria:**
 * - btn_get_pressed(BTN_1) returns false when test pin is low
 * - btn_get_pressed(BTN_1) returns true when test pin is high
 */
void test_btn_get_pressed(void) {
    // Test pin low - button not pressed
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    systick_delay_ms(1);
    TEST_ASSERT_FALSE(btn_get_pressed(BTN_1));

    // Test pin high - button pressed
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    systick_delay_ms(1);
    TEST_ASSERT_TRUE(btn_get_pressed(BTN_1));

    // Test pin low again - button not pressed
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    systick_delay_ms(1);
    TEST_ASSERT_FALSE(btn_get_pressed(BTN_1));
}

/**
 * @brief Rising edge interrupt test
 *
 * **Test ID:** BTN-02
 *
 * **Preconditions:**
 * - System initialized
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 * - Test listener registered
 * - Test pin initially low
 * - Interrupt counter reset to 0
 *
 * **Steps:**
 * 1. Configure BTN_1 for rising edge interrupts
 * 2. Enable button ISR
 * 3. Generate rising edge (low to high transition)
 * 4. Wait for interrupt processing
 * 5. Check interrupt counter
 *
 * **Success Criteria:**
 * - Interrupt counter increments by exactly 1
 */
void test_rising_edge_interrupt(void) {
    // Configure for rising edge
    btn_set_irqc(BTN_1, BTN_IRQC_RISING);
    btn_enable_isr();

    // Generate rising edge
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    systick_delay_ms(10);  // Wait for interrupt

    // Should have triggered exactly one interrupt
    TEST_ASSERT_EQUAL_UINT32(1, test_interrupt_count);
}

/**
 * @brief Falling edge interrupt test
 *
 * **Test ID:** BTN-03
 *
 * **Preconditions:**
 * - System initialized
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 * - Test listener registered
 * - Test pin initially high
 * - Interrupt counter reset to 0
 *
 * **Steps:**
 * 1. Configure BTN_1 for falling edge interrupts
 * 2. Enable button ISR
 * 3. Generate falling edge (high to low transition)
 * 4. Wait for interrupt processing
 * 5. Check interrupt counter
 *
 * **Success Criteria:**
 * - Interrupt counter increments by exactly 1
 */
void test_falling_edge_interrupt(void) {
    // Start with test pin high
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);

    // Configure for falling edge
    btn_set_irqc(BTN_1, BTN_IRQC_FALLING);
    btn_enable_isr();

    // Generate falling edge
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    systick_delay_ms(10);  // Wait for interrupt

    // Should have triggered exactly one interrupt
    TEST_ASSERT_EQUAL_UINT32(1, test_interrupt_count);
}

/**
 * @brief Interrupt disable test
 *
 * **Test ID:** BTN-04
 *
 * **Preconditions:**
 * - System initialized (system_init, btn_init)
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 * - Test pin initially low
 * - Test listener registered
 * - Interrupt counter reset to 0
 *
 * **Steps:**
 * 1. Configure BTN_1 for rising edge interrupts
 * 2. Disable button ISR
 * 3. Generate rising edge
 * 4. Wait for potential interrupt processing
 * 5. Check interrupt counter
 *
 * **Success Criteria:**
 * - Interrupt counter remains 0
 */
void test_disable_isr(void) {
    // Configure for rising edge but disable ISR
    btn_set_irqc(BTN_1, BTN_IRQC_RISING);
    btn_disable_isr();

    // Generate rising edge
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    systick_delay_ms(10);  // Wait

    // Should NOT have triggered interrupt
    TEST_ASSERT_EQUAL_UINT32(0, test_interrupt_count);
}

/**
 * @brief Interrupt flag clear test
 *
 * **Test ID:** BTN-05
 *
 * **Preconditions:**
 * - System initialized (system_init, btn_init)
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 * - Test pin initially low
 *
 * **Steps:**
 * 1. Configure BTN_1 for rising edge interrupts
 * 2. Disable ISR to prevent interrupt handling
 * 3. Generate rising edge (sets ISF flag but no interrupt)
 * 4. Verify that ISFR is set.
 * 5. Clear ISF flags manually
 * 6. Verify that ISFR is not set.
 *
 * **Success Criteria:**
 * - ISFR is set after the rising edge
 * - ISFR is not set after clearing it
 */
void test_clear_isfr(void) {
    // Configure for rising edge but keep ISR disabled
    btn_set_irqc(BTN_1, BTN_IRQC_RISING);

    // Generate rising edge (sets flag but no interrupt)
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    systick_delay_ms(1);

    TEST_ASSERT_TRUE(PORTD->ISFR == (1 << 15));

    // Clear the interrupt flag
    btn_clear_isfr();

    TEST_ASSERT_FALSE(PORTD->ISFR == (1 << 15));
}

/**
 * @brief IRQC disabled test
 *
 * **Test ID:** BTN-06
 *
 * **Preconditions:**
 * - System initialized (system_init, btn_init)
 * - Test pin configured as GPIO output and physically wired to BTN_1 (PTD15)
 * - Test pin initially low
 * - Test listener registered
 * - Interrupt counter reset to 0
 * - ISR enabled
 *
 * **Steps:**
 * 1. Set IRQC to disabled
 * 2. Enable ISR
 * 3. Generate rising edge
 * 4. Generate falling edge
 * 5. Wait for potential interrupt processing
 * 6. Check interrupt counter
 *
 * **Success Criteria:**
 * - Interrupt counter remains 0
 */
void test_irqc_disabled(void) {
    // Set IRQC to disabled
    btn_set_irqc(BTN_1, BTN_IRQC_DISABLED);
    btn_enable_isr();

    // Generate both edges
    TEST_PIN_GPIO->PSOR = (1 << TEST_PIN);
    systick_delay_ms(1);
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);
    systick_delay_ms(10);

    // Should NOT have triggered any interrupts
    TEST_ASSERT_EQUAL_UINT32(0, test_interrupt_count);
}

void setUp(void) {
    // Disable interrupts and clear flags
    btn_disable_isr();
    btn_set_irqc(BTN_1, BTN_IRQC_DISABLED);
    btn_clear_isfr();
    S32_NVIC->ICPR[PORT_IRQn >> 5] = (1 << (PORT_IRQn & 0x1F));  // Clear any pending PORT interrupt in NVIC

    // Reset test state
    test_interrupt_count = 0;
    TEST_PIN_GPIO->PCOR = (1 << TEST_PIN);

    // Wait for settling
    systick_delay_ms(10);
}

void tearDown(void) {
}

int main(void) {
    // Initialize system
    system_init();
    btn_init();
    btn_set_isr_callback(&test_interrupt_callback);

    // Configure test pin as GPIO output
    PCC->PCCn[TEST_PIN_PORT_INDEX] = PCC_PCCn_CGC_MASK;
    TEST_PIN_GPIO->PDDR |= (1 << TEST_PIN);
    TEST_PIN_PORT->PCR[TEST_PIN] = PORT_PCR_MUX(1);

    UNITY_BEGIN();
    RUN_TEST(test_btn_get_pressed);
    RUN_TEST(test_rising_edge_interrupt);
    RUN_TEST(test_falling_edge_interrupt);
    RUN_TEST(test_disable_isr);
    RUN_TEST(test_clear_isfr);
    RUN_TEST(test_irqc_disabled);
    return UNITY_END();
}

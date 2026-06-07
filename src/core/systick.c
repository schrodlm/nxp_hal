/**
 * @file systick.c
 * @ingroup core
 * @brief System tick timer implementation
 *
 * Implements a 1ms system tick timer using the ARM Cortex-M0+ SysTick
 * hardware timer with support for registering callback listeners.
 */

#include <core/systick.h>
#include <S32K118/startup/system_S32K118.h>
#include <device_registers.h>
#include <stddef.h>

volatile uint32_t systick_ms = 0;
static systick_listener_t *systick_listeners = NULL;

uint32_t systick_get_ms(void) {
    return systick_ms;
}

void SysTick_Handler(void) {
    S32_SCB->ICSR = S32_SCB_ICSR_PENDSTCLR_MASK;
    systick_ms++;

    // Call all registered listeners
    systick_listener_t *current = systick_listeners;
    while (current != NULL) {
        if (current->callback != NULL) {
            current->callback();
        }
        current = current->next;
    }
}

void systick_delay_ms(uint32_t ms) {
    uint32_t start = systick_ms;
    while (systick_ms - start < ms);
}

void systick_register_listener(systick_listener_t *listener, systick_listener_callback_t callback) {
    listener->callback = callback;
    listener->next = systick_listeners;
    systick_listeners = listener;
}

void systick_init(void) {
    // Setup systick to count every 1ms
    uint32_t ticks = SystemCoreClock / 1000; // Counts in 1ms ticks.

    // Documentation to SysTick can be found in "Cortex-M0+ Devices - Generic User Guide" (DUI0662B_cortex_m0p_r0p1_dgug.pdf)
    S32_SysTick->RVR = ticks - 1;
    S32_SysTick->CVR = 0;
    S32_SysTick->CSR =
        S32_SysTick_CSR_ENABLE(1)
        | S32_SysTick_CSR_CLKSOURCE(1)
        | S32_SysTick_CSR_TICKINT(1)
    ;
    // Default priority of systick is 0
}

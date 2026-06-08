/**
 * @file btn.c
 * @brief Button HAL implementation.
 *
 * How a button event reaches the user
 * ----------------------------------
 * The four buttons live on two ports (PORT D and PORT E). Each button pin is
 * configured as a plain GPIO input (PORTx->PCR[n].MUX = 1). The buttons are
 * wired to VCC through the switch, with an external pull-down to ground, so
 * a pressed button reads as a logic high on its pin (PTx->PDIR bit set).
 *
 * All four buttons share a single hardware interrupt line on the chip
 * (PORT_IRQn = 9). When any configured edge fires, the CPU jumps into the
 * one ISR defined in this file: PORT_IRQHandler. There are no per-pin or
 * per-port handlers.
 *
 * Three things have to be true for the user's callback to ever run:
 *   1. The button's pin is configured to flag the desired edge 
 *      (the IRQC field of PORTx->PCR[n]). Set per button via btn_set_irqc().
 *   2. The shared interrupt line is enabled in the NVIC (S32_NVIC->ISER[0])
 *      so that the CPU actually receives the request. Enabled/disabled by
 *      btn_enable_isr() and btn_disable_isr(). When enabling, any leftover
 *      pending request is cleared first (S32_NVIC->ICPR[0]) so a stale event
 *      from before doesn't fire immediately.
 *   3. A callback has been registered via btn_set_isr_callback().
 *
 * Callback contract
 * -----------------
 * The user callback is responsible for clearing the interrupt flag itself
 * (the per-pin bit in PORTx->ISFR) by calling btn_clear_isfr(). If it
 * forgets, the flag stays set and the ISR fires again immediately. That's an
 * infinite loop in interrupt context.
 *
 * The callback runs in interrupt context: keep it short, do not call printf,
 * and do not block.
 *
 */

#include <lib/btn.h>
#include <stdbool.h>

#include <device_registers.h>

#define BTN_1_PIN 15  // PTD15
#define BTN_2_PIN 9   // PTE9
#define BTN_3_PIN 16  // PTD16
#define BTN_4_PIN 8   // PTE8

typedef struct {
    PORT_Type *port;
    GPIO_Type *gpio;
    uint8_t pin;
} btn_info_t;

static const btn_info_t btn_map[] = {
    [BTN_1] = {PORTD, PTD, 15},
    [BTN_2] = {PORTE, PTE, 9},
    [BTN_3] = {PORTD, PTD, 16},
    [BTN_4] = {PORTE, PTE, 8},
};

bool btn_isfr_pending(btn_t btn) {
    if(btn >= BTN_COUNT) return false;

    const btn_info_t *b = &btn_map[btn];
    uint32_t isfr = b->port->ISFR;
    return (isfr >> b->pin) & 1u;
}

_Static_assert(sizeof(btn_map) / sizeof(btn_map[0]) == BTN_COUNT, "btn_map size must match BTN_COUNT");

static btn_isr_callback_t user_callback = 0;

void PORT_IRQHandler(void) {
    if(user_callback) {
        user_callback();
    } else {
        btn_clear_isfr();
    }
};

//=======================BTN API IMPLEMENTATION ==============================================

void btn_init(void) {
    // Enable clock ticking on the btn peripherals
    PCC->PCCn[PCC_PORTD_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_PORTE_INDEX] = PCC_PCCn_CGC_MASK;

    for (int i = 0; i < BTN_COUNT; i++) {
        btn_map[i].port->PCR[btn_map[i].pin] = PORT_PCR_MUX(1);
    }
}

bool btn_get_pressed(btn_t button) {
    if (button >= BTN_COUNT) return false;
    const btn_info_t *b = &btn_map[button];
    bool high = (b->gpio->PDIR >> b->pin) & 1u;
    return high;
}

void btn_set_isr_callback(btn_isr_callback_t callback) {
    user_callback = callback;
}

void btn_set_irqc(btn_t button, uint32_t irqc) {
    if (button >= BTN_COUNT) return;
    if (irqc != BTN_IRQC_DISABLED && irqc != BTN_IRQC_FALLING && irqc != BTN_IRQC_RISING && irqc != BTN_IRQC_EITHER)
        return;

    const btn_info_t *b = &btn_map[button];
    uint32_t pcr = b->port->PCR[b->pin];
    pcr &= ~PORT_PCR_IRQC_MASK;  // clear old IRQC
    pcr |= PORT_PCR_IRQC(irqc);  // set new IRQC
    b->port->PCR[b->pin] = pcr;
}

void btn_clear_isfr(void) {
    for(int i = 0; i < BTN_COUNT; i++) {
        const btn_info_t *b = &btn_map[i];
        b->port->ISFR = (1u << b->pin);   // write-1-to-clear
    }
}

void btn_enable_isr(void) {
    S32_NVIC->ICPR[0] = (1u << PORT_IRQn);  // clear any stale pending
    S32_NVIC->ISER[0] = (1u << PORT_IRQn); // write-to-act register
}

void btn_disable_isr(void) {
    S32_NVIC->ICER[0] = (1u << PORT_IRQn); // write-to-act register
}
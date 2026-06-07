#include <lib/led.h>

#include <device_registers.h>

#define LED_BLUE_PIN   6
#define LED_RED_PIN    5
#define LED_YELLOW_PIN 1
#define LED_GREEN_PIN  0

void led_init(void) {
    // Enable clock on the LED peripherals

    // 1. LED BLUE and LED RED are routed by PORT D
    PCC->PCCn[PCC_PORTD_INDEX] = PCC_PCCn_CGC_MASK;
    // Set the function of the pins to GPIO
    PORTD->PCR[LED_BLUE_PIN] = PORT_PCR_MUX(1);
    PORTD->PCR[LED_RED_PIN] = PORT_PCR_MUX(1);
    // Set the pins as OUTPUT
    PTD->PDDR |= (1 << LED_BLUE_PIN) | (1 << LED_RED_PIN);
    // Initially turn off the GPIO pins
    // PCOR is a write-1-to-act register so = is correct
    PTD->PCOR = (1 << LED_BLUE_PIN) | (1 << LED_RED_PIN);

    // 2. LED_YELLOW and LED GREEN are routed by PORT C
    PCC->PCCn[PCC_PORTC_INDEX] = PCC_PCCn_CGC_MASK;
    // Set the function of the pins to GPIO
    PORTC->PCR[LED_YELLOW_PIN] = PORT_PCR_MUX(1);
    PORTC->PCR[LED_GREEN_PIN] = PORT_PCR_MUX(1);
    // Set the pins as OUTPUT
    PTC->PDDR |= (1 << LED_YELLOW_PIN) | (1 << LED_GREEN_PIN);
    // Initially turn off the GPIO pins
    // PCOR is a write-1-to-act register so = is correct
    PTC->PCOR = (1 << LED_YELLOW_PIN) | (1 << LED_GREEN_PIN);
}

void led_set_state(led_t led, bool state) {
    switch (led) {
    case LED_BLUE:
        state ? (PTD->PSOR = (1 << LED_BLUE_PIN)) : (PTD->PCOR = (1 << LED_BLUE_PIN));
        break;
    case LED_RED:
        state ? (PTD->PSOR = (1 << LED_RED_PIN)) : (PTD->PCOR = (1 << LED_RED_PIN));
        break;
    case LED_YELLOW:
        state ? (PTC->PSOR = (1 << LED_YELLOW_PIN)) : (PTC->PCOR = (1 << LED_YELLOW_PIN));
        break;
    case LED_GREEN:
        state ? (PTC->PSOR = (1 << LED_GREEN_PIN)) : (PTC->PCOR = (1 << LED_GREEN_PIN));
        break;
    default:
        break;
    }
}
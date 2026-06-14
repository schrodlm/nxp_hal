#include <lib/joy.h>

#include <device_registers.h>

#define JOY_BTN_PIN 15 //PTC15
#define JOY_X_PIN 16 //PTC16
#define JOY_Y_PIN 17 //PTC17

#define JOY_Y_CHANNEL 15 // ADC0_SE15
#define JOY_X_CHANNEL 14 // ADC0_SE14

static uint16_t adc_read(uint8_t channel) {
    ADC0->SC1[0] = ADC_SC1_ADCH(channel);
    while(!(ADC0->SC1[0] & ADC_SC1_COCO_MASK));
    return ADC0->R[0];
}

void joy_init(void) {
    // Enable clock ticking on the peripherals
    PCC->PCCn[PCC_PORTC_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_ADC0_INDEX] = PCC_PCCn_PCS(1) | PCC_PCCn_CGC_MASK; //set clock source (SOSCDIV1)

    PORTC->PCR[JOY_X_PIN] = PORT_PCR_MUX(0); // analog
    PORTC->PCR[JOY_Y_PIN] = PORT_PCR_MUX(0); // analog
    PORTC->PCR[JOY_BTN_PIN] = PORT_PCR_MUX(1); // GPIO input


    // ADC0 configuration
    ADC0->CFG1 = ADC_CFG1_MODE(1); // MODE=1 -> 12-bit (0..4095)
    ADC0->SC2 = 0; //software trigger (ADTRG=0), default voltage ref
}

joy_result_t joy_read(void) {
    joy_result_t r;

    r.y = adc_read(JOY_Y_CHANNEL);
    r.x = adc_read(JOY_X_CHANNEL);
    r.pressed = !((PTC->PDIR >> JOY_BTN_PIN) & 1u);

    return r;
}

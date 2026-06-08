/**
 * @file servo.c
 * @brief Servo HAL — 50 Hz PWM on PTB12 via FTM0 channel 0.
 *
 * What the servo wants
 * --------------------
 * A hobby servo is driven by a single signal: a pulse that goes HIGH for
 * 500..2500 us, then LOW for the rest of a 20 ms period (50 Hz). The HIGH
 * width selects the shaft angle: 
 * - 1500 us = center, 
 * - 500 us = lowest 
 * - 2500 us = highest
 *
 * How FTM produces that
 * ---------------------
 * FTM0 is a counter that ticks at 40 MHz / 128 = 312500 Hz (one tick =
 * 3.2 us). With MOD = 6249 the counter wraps every 6250 ticks = 20 ms,
 * which is the 50 Hz period of the servo.
 *
 * Channel 0 (FTM0_CH0) is set to edge-aligned PWM. It drives the pin 
 * HIGH at every counter overflow and LOW when CNT reaches CnV.
 *
 * Two non-obvious bits the chapters don't cluster together
 * --------------------------------------------------------
 * 1. FTM_SC.PWMEN0 must be 1 for the channel output to actually reach
 *    the pad. This gate lives in the chip's System Integration Module
 *    (SIM), not in the FTM chapter: by default the SIM keeps FTM-channel
 *    outputs tristated and only releases them when PWMENn=1. Forgetting
 *    this leaves the pin stuck at 0 despite a fully configured FTM.
 */

#include <lib/servo.h>

#include <device_registers.h>

#define SERVO_PWM_PIN 12  //PTB12


static uint32_t us_to_ticks(uint32_t us) {
    // FTM0 runs with 40 MHz / 128 = 312500 Hz
    // 1 tick = 3.2 us
    // we use this tick to divide by 3.2 
    // it uses bit shifts instead of floating point arithmetic
    return (us*5) / 16;
}

void servo_init(void) {
    // Clock the port and FTM0. FTM0 needs a clock source via PCS
    // SOSCDIV1 (40 MHz) is fine
    PCC->PCCn[PCC_PORTB_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_FTM0_INDEX] = PCC_PCCn_PCS(1) | PCC_PCCn_CGC_MASK;

    // PTB12 -> FTM0_CH0 (ALT2)
    PORTB->PCR[SERVO_PWM_PIN] = PORT_PCR_MUX(2);

    // Configure FTM0 for edge-aligned PWM
    FTM0->MODE |= FTM_MODE_WPDIS_MASK;  // unlock writes
    FTM0->SC = 0;                       // stop the counter while configuring
    //FTM0->MODE |= FTM_MODE_FTMEN_MASK;  // enable advanced features

    // 50 Hz PWM: with PS=7 (/128) the tick rate is 40 MHz / 128 = 312500 Hz.
    // One 20 ms period = 6250 ticks so MOD = 6249 (+1)
    FTM0->CNTIN = 0;
    FTM0->MOD = 6249;
    FTM0->CNT = 0;

    // Channel 0: edge-aligned PWM, high-true polarity.
    // MSB=1, ELSB=0 selects edge-aligned PWM
    // the channel drives HIGH while CNT < CnV and LOW while CNT >= CnV.
    FTM0->CONTROLS[0].CnSC = FTM_CnSC_MSB_MASK | FTM_CnSC_ELSB_MASK;

    // Initial pulse width: 1500 us = center. Convert us -> ticks using 5/16
    // (1 us = 0.3125 ticks at this tick rate).
    FTM0->CONTROLS[0].CnV = us_to_ticks(SERVO_PULSE_MIDDLE_US);

    // Start the counter and enable PWM output on channel 0.
    // PWMEN0=1 is mandatory: without it the channel output stays tristated
    // (the SIM's FTM-pin-enable logic only releases the pad to FTM when
    // PWMENn=1). CLKS=1 selects the system clock, PS=7 prescales /128 to
    // give the 50 Hz period.
    FTM0->SC = FTM_SC_PWMEN0_MASK | FTM_SC_CLKS(1) | FTM_SC_PS(7);
}

void servo_set_pulse(uint32_t pulse_us) {
    if(pulse_us < SERVO_PULSE_MIN_US) pulse_us = SERVO_PULSE_MIN_US;
    if(pulse_us > SERVO_PULSE_MAX_US) pulse_us = SERVO_PULSE_MAX_US;
    FTM0->CONTROLS[0].CnV = us_to_ticks(pulse_us);
}
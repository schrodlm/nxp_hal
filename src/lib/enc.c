/**
 * @file enc.c
 * @brief Rotary-encoder HAL using FTM1 in quadrature-decoder mode.
 *
 * What FTM1 does in this mode
 * ---------------------------
 * FTM1 is a 16-bit counter. In quadrature-decoder mode the counter's
 * increment/decrement input is replaced by logic that watches two pins
 * (FTM1_QD_PHA and FTM1_QD_PHB). The count is maintained entirely
 * by the FTM peripheral.
 *
 * The reading API just returns the current count (FTM1->CNT).
 */

#include <lib/enc.h>

#include <device_registers.h>

#define ENC_A_PIN      3   // PTB3
#define ENC_B_PIN      2   // PTB2
#define ENC_SWITCH_PIN 14  // PTC14

#define ENC_PHASE_MUX  4  // PTB3/PTB2 ALT4 = FTM1_QD_PHA/PHB
#define ENC_SWITCH_MUX 1  // GPIO

void enc_init(void) {
    // 1. PCC: ungate port clocks and FTM1. FTM1 also needs a clock source and 40 MHz from clock_init is fine
    PCC->PCCn[PCC_PORTB_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_PORTC_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_FTM1_INDEX] = PCC_PCCn_PCS(1) | PCC_PCCn_CGC_MASK;

    // 2. PORT mux: phase pins to FTM1's quad-decoder inputs, switch to GPIO.
    PORTB->PCR[ENC_A_PIN] = PORT_PCR_MUX(ENC_PHASE_MUX);
    PORTB->PCR[ENC_B_PIN] = PORT_PCR_MUX(ENC_PHASE_MUX);
    PORTC->PCR[ENC_SWITCH_PIN] = PORT_PCR_MUX(ENC_SWITCH_MUX);

    // 3. FTM1 setup.
    //    a. Unlock write-protected registers.
    //    b. Stop the timer (CLKS=0) while we configure.
    //    c. Enable advanced FTM features (required for quad-decoder mode).
    //    d. Configure counter range and reset count.
    //    e. Enable digital input filters on phase A/B to reject any
    //       residual noise that survived the board's RC filter.
    //    f. Enable quadrature decoder mode.
    //    g. Start the timer (CLKS=1, system clock). Even in quad mode the
    //       FTM logic that watches the phase pins needs the system clock
    //       to be running — QUADEN selects the *mode*, but the peripheral
    //       still needs CLKS != 00 to operate.
    FTM1->MODE |= FTM_MODE_WPDIS_MASK;
    FTM1->SC = 0;
    FTM1->MODE |= FTM_MODE_FTMEN_MASK;

    FTM1->CNTIN = 0;
    FTM1->MOD = 0xFFFF;
    FTM1->CNT = 0;

    FTM1->FILTER = FTM_FILTER_CH0FVAL(15) | FTM_FILTER_CH1FVAL(15);
    FTM1->QDCTRL = FTM_QDCTRL_QUADEN_MASK | FTM_QDCTRL_PHAFLTREN_MASK | FTM_QDCTRL_PHBFLTREN_MASK;

    FTM1->SC = FTM_SC_CLKS(1);
}

int16_t enc_get_position(void) {
    return (int16_t)FTM1->CNT;
}

void enc_reset_position(void) {
    FTM1->CNT = 0;
}

bool enc_get_pressed(void) {
    // PTC14: active-low. Pressed = pin LOW.
    bool high = (PTC->PDIR >> ENC_SWITCH_PIN) & 1u;
    return !high;
}
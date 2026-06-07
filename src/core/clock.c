/**
 * @file clock.c
 * @ingroup core
 * @brief System clock initialization implementation
 *
 * Implements the clock initialization using the System
 * Oscillator (SOSC) with a 40MHz external crystal.
 */

#include <core/clock.h>
#include <S32K118/startup/system_S32K118.h>
#include <device_registers.h>

void clock_init(void) {
    // Clock settings to use 40MHz crystal as SOSC_CLK
    SCG->SOSCDIV = SCG_SOSCDIV_SOSCDIV1(1) | SCG_SOSCDIV_SOSCDIV2(1);  // SOSCDIV1 divide by 1, SOSCDIV2 divide by 1
    SCG->SOSCCFG  =	SCG_SOSCCFG_RANGE(3) | SCG_SOSCCFG_HGO(0) | SCG_SOSCCFG_EREFS(1);  // Range=3: High frequency range selected, HGO=0: Config xtal osc for low power, EREFS=1: Input is external XTAL
    while (SCG->SOSCCSR & SCG_SOSCCSR_LK_MASK); // Ensure SOSCCSR unlocked

    // LK=0:          This Control Status Register can be written.
    // SOSCCMRE=0:    Clock Monitor generates interrupt when error detected
    // SOSCCM=0:      System OSC Clock Monitor is disabled
    // SOSCEN=1:      System OSC is enabled
    SCG->SOSCCSR = SCG_SOSCCSR_SOSCCMRE(0) | SCG_SOSCCSR_SOSCCM(0) | SCG_SOSCCSR_SOSCEN(1);

    while(!(SCG->SOSCCSR & SCG_SOSCCSR_SOSCVLD_MASK)); // Wait for sys OSC clk valid
}

/**
 * @file btn.h
 * @ingroup lib
 * @brief Button hardware abstraction
 *
 * Provides button initialization, state reading, and interrupt management
 * with listener support for button events.
 *
 * Compared to LEDs, buttons introduce one more layer of hardware:
 * the pin is still configured in PORT/GPIO, but edge detection and interrupt
 * flagging are handled by the PORT peripheral and then forwarded to NVIC.
 */

#ifndef LIB_BTN_H
#define LIB_BTN_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Button identifiers
 */
typedef enum {
    BTN_1,    /**< Button 1 on PTD15 */
    BTN_2,    /**< Button 2 on PTE9 */
    BTN_3,    /**< Button 3 on PTD16 */
    BTN_4,    /**< Button 4 on PTE8 */
    BTN_COUNT /**< Total number of buttons */
} btn_t;

/** @brief Disable button interrupt */
#define BTN_IRQC_DISABLED 0b000
/** @brief Trigger interrupt on rising edge */
#define BTN_IRQC_RISING  0b1001
/** @brief Trigger interrupt on falling edge */
#define BTN_IRQC_FALLING  0b1010
/** @brief Trigger interrupt on rising and falling edge */
#define BTN_IRQC_EITHER  0b1011

/**
 * @brief Button ISR callback function type
 *
 * The callback is responsible for calling btn_clear_isfr() to clear interrupt flags.
 */
typedef void (*btn_isr_callback_t)(void);

/**
 * @brief Initialize button hardware
 *
 * Prepares all button pins as GPIO inputs and leaves their interrupts disabled.
 *
 * High-level implementation outline:
 * 1. Find which ports contain the button pins.
 * 2. In `PCC->PCCn[...]`, enable the clock gate for each corresponding PORT
 *    peripheral before accessing its registers.
 * 3. In each `PORTx->PCR[n]`, select GPIO function (`MUX = 1`).
 * 4. Configure the interrupt-related fields in `PCR` to a safe initial state:
 *    clear any pending pin flag and set `IRQC` to disabled.
 * 5. In `GPIOx->PDDR`, make sure the button pins are inputs.
 * 6. If you decide to use input filtering or pull resistors, this is the place
 *    to study the related PORT/GPIO options in the manual and board schematic.
 *
 * What to read in the reference manual:
 * - PCC chapter: clock gating for PORT peripherals
 * - PORT chapter: `PCR`, especially `MUX`, `IRQC`, and interrupt flag handling
 * - GPIO chapter: input direction and pin data input register
 *
 * @note This function should only configure the hardware. It should not enable
 *       the NVIC interrupt yet and should not assume any particular callback.
 */
void btn_init(void);

/**
 * @brief Read button state
 * @param button Button to read
 * @return true if button is pressed (logic high), false otherwise
 *
 * High-level implementation outline:
 * 1. Translate the logical button identifier to the correct GPIO peripheral and
 *    pin number.
 * 2. Read the current input level from `GPIOx->PDIR`.
 * 3. Mask the bit for the selected pin and convert it to a boolean value.
 *
 * What to read in the reference manual:
 * - GPIO chapter: `PDIR` and how input pin values are sampled
 */
bool btn_get_pressed(btn_t button);

/**
 * @brief Set the button ISR callback
 * @param callback Function to call when button interrupt occurs, or NULL to clear
 *
 * Only one callback can be registered at a time. The callback must call btn_clear_isfr()
 * to clear the interrupt flags, as the ISR handler does not do this automatically.
 * If no callback is set, the ISR will clear the interrupt flags automatically.
 *
 * High-level implementation outline:
 * 1. Store the function pointer in a module-global variable.
 * 2. In the shared `PORT_IRQHandler`, check whether a callback is registered.
 * 3. If yes, call it; otherwise clear the PORT interrupt flags directly.
 *
 * What to read in the reference manual:
 * - Cortex-M / NVIC interrupt model overview
 * - device startup/vector table files to see how `PORT_IRQHandler` is connected
 *
 * @note The PORT interrupt is shared. The callback is therefore a software
 *       dispatch mechanism built on top of the single hardware IRQ handler.
 */
void btn_set_isr_callback(btn_isr_callback_t callback);

/**
 * @brief Set interrupt configuration for a button
 * @param button Button to configure
 * @param irqc Interrupt configuration (BTN_IRQC_DISABLED, BTN_IRQC_RISING, BTN_IRQC_FALLING, or BTN_IRQC_EITHER)
 *
 * High-level implementation outline:
 * 1. Locate the selected button's `PORTx->PCR[n]`.
 * 2. Update only the `IRQC` field in that register.
 * 3. Preserve the other `PCR` fields already configured by `btn_init()`.
 *
 * What to read in the reference manual:
 * - PORT chapter: meaning of the `IRQC` field and which values correspond to
 *   disabled / rising / falling / either-edge interrupt detection
 */
void btn_set_irqc(btn_t button, uint32_t irqc);

/**
 * @brief Clear interrupt flags for all buttons
 *
 * High-level implementation outline:
 * 1. For each button pin, find the corresponding PORT instance.
 * 2. Clear its interrupt status flag through the PORT interrupt flag register
 *    (`ISFR`) using the write-1-to-clear semantics described in the manual.
 *
 * What to read in the reference manual:
 * - PORT chapter: `ISFR` and write-1-to-clear behaviour
 *
 * @note Students usually need to understand this together with `IRQC`:
 *       edge detection sets the flag in PORT, and the flag must be cleared
 *       before the next event can be handled cleanly.
 */
void btn_clear_isfr(void);

/**
 * @brief Enable PORT interrupt for all buttons
 *
 * High-level implementation outline:
 * 1. Clear any pending interrupt in NVIC for the shared PORT IRQ line.
 * 2. Enable that IRQ line in `NVIC ISER`.
 * 3. Keep in mind that this only enables delivery to the CPU; an individual
 *    button still needs `PCR.IRQC` configured to generate events.
 *
 * What to read in the reference manual:
 * - Cortex-M / NVIC chapter: pending vs enable state
 * - device header for the exact IRQ number used by the PORT peripheral
 */
void btn_enable_isr(void);

/**
 * @brief Disable PORT interrupt for all buttons
 *
 * High-level implementation outline:
 * 1. Optionally clear any pending interrupt in NVIC.
 * 2. Disable the shared PORT IRQ line in `NVIC ICER`.
 *
 * What to read in the reference manual:
 * - Cortex-M / NVIC chapter: interrupt clear-enable and clear-pending registers
 *
 * @note Disabling the NVIC line does not undo the pin configuration in
 *       `PORTx->PCR[n]`; it only prevents the CPU from servicing the IRQ.
 */
void btn_disable_isr(void);

#endif

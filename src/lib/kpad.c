/**
 * @file kpad.c
 * @brief 3×4 matrix keypad HAL implementation.
 *
 * How the matrix works
 * --------------------
 * The keypad has 12 keys but only 7 wires to the chip: 3 columns and 4 rows.
 * Each key sits at one (row, column) intersection and, when pressed,
 * electrically connects its row wire to its column wire.
 *
 * Because 12 keys share 7 wires, we can't read them all in parallel. We have
 * to drive one column with a known signal and see which rows pick
 * it up through a pressed key.
 *
 * Pin roles and electrical states
 * -------------------------------
 * Each GPIO pin can be in three useful states:
 *   1. Input / high-impedance: pin is listening, not driving anything
 *      (PTx->PDDR bit = 0).
 *   2. Output driving LOW (PTx->PDDR bit = 1, output value loaded via
 *      PTx->PCOR).
 *   3. Output driving HIGH (PTx->PDDR bit = 1, output value loaded via
 *      PTx->PSOR).
 *
 * Row pins are inputs with the chip's internal pull-up
 * enabled (PE=1, PS=1 in PORTx->PCR[n]). With no key pressed, the pull-up
 * holds the row at HIGH. With a key pressed, the closed switch shorts
 * the row to whatever the column is driving.
 *
 * Column pins sit at rest and briefly switch to
 * state output driving LOW during their turn in the scan. The output value is
 * pre-loaded to LOW once in kpad_init() (a single write to PTx->PCOR), so
 * during the scan we only have to flip PTx->PDDR to start and stop driving.
 *
 * Why inactive columns must stay high-impedance
 * ---------------------------------------------
 * Two keys pressed in the same row but different columns short the row to
 * both columns. If an inactive column were driving HIGH, two MCU pins would
 * fight each other through the closed key. Wrong reading, possible damage.
 * High-Z keeps the inactive column passive so the driven column wins.
 *
 */

#include <lib/kpad.h>

#include <device_registers.h>

#define ROW1_PIN 7  // PTA7
#define ROW2_PIN 6  // PTA6
#define ROW3_PIN 7  // PTE7
#define ROW4_PIN 8  // PTC8

#define COL1_PIN 0  // PTB0
#define COL2_PIN 9  // PTC9
#define COL3_PIN 1  // PTB1

typedef struct {
    PORT_Type *port;
    GPIO_Type *gpio;
    uint8_t pin;
} kpad_info_t;

typedef enum { KPAD_ROW_1, KPAD_ROW_2, KPAD_ROW_3, KPAD_ROW_4, KPAD_COL_1, KPAD_COL_2, KPAD_COL_3, KPAD_COUNT } kpad_t;

static const kpad_info_t kpad_map[] = {
    [KPAD_ROW_1] = {PORTA, PTA, 7}, [KPAD_ROW_2] = {PORTA, PTA, 6}, [KPAD_ROW_3] = {PORTE, PTE, 7},
    [KPAD_ROW_4] = {PORTC, PTC, 8}, [KPAD_COL_1] = {PORTB, PTB, 0}, [KPAD_COL_2] = {PORTC, PTC, 9},
    [KPAD_COL_3] = {PORTB, PTB, 1},
};

static const uint32_t key_map[KPAD_ROWS][KPAD_COLS] = {{KPAD_KEY_1, KPAD_KEY_2, KPAD_KEY_3},
                                                       {KPAD_KEY_4, KPAD_KEY_5, KPAD_KEY_6},
                                                       {KPAD_KEY_7, KPAD_KEY_8, KPAD_KEY_9},
                                                       {KPAD_KEY_STAR, KPAD_KEY_0, KPAD_KEY_HASH}};

void kpad_init(void) {
    PCC->PCCn[PCC_PORTA_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_PORTB_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_PORTC_INDEX] = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_PORTE_INDEX] = PCC_PCCn_CGC_MASK;

    // Rows: GPIO with internal pull-up enabled (PE=1, PS=1)
    // The row reads HIGH by default
    for (int i = KPAD_ROW_1; i <= KPAD_ROW_4; i++) {
        const kpad_info_t *k = &kpad_map[i];
        k->port->PCR[k->pin] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
        // PDDR bit default to 0, which represents GPIO input
    }

    // Columns: GPIO input, no pull. It is input at rest, it is flipped to
    // output during scan to drive the line low.
    for (int i = KPAD_COL_1; i <= KPAD_COL_3; i++) {
        const kpad_info_t *k = &kpad_map[i];
        k->port->PCR[k->pin] = PORT_PCR_MUX(1);
        k->gpio->PCOR = (1u << k->pin);
    }
}

uint32_t kpad_scan(void) {
    uint32_t pressed = 0;

    for (int c = 0; c < KPAD_COLS; c++) {
        const kpad_info_t *col = &kpad_map[KPAD_COL_1 + c];

        // Set col as an GPIO output
        col->gpio->PDDR |= (1u << col->pin);

        // settle
        for (volatile int i = 0; i < 200; i++)
            ;

        //read all rows, LOW row means the (r,c) key is shorting the row to the drive-LOW column
        for (int r = 0; r < KPAD_ROWS; r++) {
            const kpad_info_t *row = &kpad_map[KPAD_ROW_1 + r];
            bool high = (row->gpio->PDIR >> row->pin) & 1u;
            if (!high) pressed |= key_map[r][c];
        }
        // Set COL back to GPIO input
        col->gpio->PDDR &= ~(1u << col->pin);
    }
    return pressed;
}
#include <lib/btn_cnt.h>

#include <core/systick.h>
#include <lib/btn.h>

typedef enum {
    IDLE,
    PRESSED,
    DEBOUNCE,
} btn_cnt_state_t;

typedef struct {
    btn_cnt_state_t state;
    uint32_t cnt;
    uint32_t last_release_ms;
} btn_record_t;

static btn_record_t records[] = {
    [BTN_1] = {IDLE, 0, 0},
    [BTN_2] = {IDLE, 0, 0},
    [BTN_3] = {IDLE, 0, 0},
    [BTN_4] = {IDLE, 0, 0},
};

static systick_listener_t btn_cnt_listener;

static void on_systick(void) {
    uint32_t now_ms = systick_get_ms();
    for (int i = 0; i < BTN_COUNT; i++) {
        btn_record_t *btn_rec = &records[i];
        if (btn_rec->state == DEBOUNCE && (now_ms - btn_rec->last_release_ms) >= BTN_CNT_DEBOUNCE_MS) {
            btn_set_irqc((btn_t)i, BTN_IRQC_RISING);
            btn_rec->state = IDLE;
        }
    }
}

void btn_cnt_callback(void) {
    //determine which button was pressed
    for (int i = 0; i < BTN_COUNT; i++) {
        //check ISFR for this button's bin
        if (btn_isfr_pending((btn_t)i)) {
            btn_record_t *btn_rec = &records[i];
            switch (btn_rec->state) {
            case IDLE:
                btn_rec->cnt++;
                btn_set_irqc((btn_t)i, BTN_IRQC_FALLING);
                btn_rec->state = PRESSED;
                break;

            case PRESSED:
                btn_set_irqc((btn_t)i, BTN_IRQC_DISABLED);
                btn_rec->state = DEBOUNCE;
                btn_rec->last_release_ms = systick_get_ms();
                break;

            default:
                break;
            }
        }
    }
    btn_clear_isfr();
}

void btn_cnt_init(void) {
    btn_set_isr_callback(&btn_cnt_callback);
    for (int i = 0; i < BTN_COUNT; i++) {
        btn_set_irqc((btn_t)i, BTN_IRQC_RISING);
    }
    btn_enable_isr();
    systick_register_listener(&btn_cnt_listener, &on_systick);
}

uint32_t btn_cnt_get_count(btn_t btn) {
    if (btn >= BTN_COUNT) return 0;

    return records[btn].cnt;
}

void btn_cnt_reset_count(btn_t btn) {
    if (btn >= BTN_COUNT) return;
    records[btn].cnt = 0;
}
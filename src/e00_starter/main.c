#include <stdio.h>
#include <core/system.h>
#include <core/systick.h>

int main(void) {
    system_init();

    for (uint32_t cnt = 0; ; cnt++) {
        printf("[%010lu] Counter %lu\n", systick_get_ms(), cnt);
        systick_delay_ms(1000);
    }

    return 0;
}

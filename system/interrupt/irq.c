#include "pic.h"
#include "../drivers/pit.h"

void timer_handler(void) {
    pit_tick_handler();  // atualiza ticks e timers
    pic_send_eoi(0);
}
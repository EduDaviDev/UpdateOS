#include "pic.h"

void timer_handler(void) {
    pic_send_eoi(0);
}

// keyboard_isr agora está em drivers/keyboard.c
// Não definimos mais aqui.

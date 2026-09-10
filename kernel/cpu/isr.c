#include "isr.h"
#include "idt.h"

isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

/* Chamado pelo stub em assembly quando ocorre uma exceção da CPU (0-31) */
void isr_handler(registers_t *regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        /* Exceção sem handler: por enquanto, apenas para o sistema */
        __asm__ volatile ("cli");
        for (;;) {
            __asm__ volatile ("hlt");
        }
    }
}

void isr_install(void) {
    /* Reservado para instalar handlers padrão no futuro (ex.: page fault) */
}
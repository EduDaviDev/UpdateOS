#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "isr.h"

extern void irq0_handler(void);
extern void irq1_handler(void);

void isr_install(void) {
    gdt_init();
    idt_init();
    pic_remap();

    idt_set_handler(0x20, irq0_handler);
    idt_set_handler(0x21, irq1_handler);

    pic_enable_irq(0);
    pic_enable_irq(1);

    enable_interrupts();
}

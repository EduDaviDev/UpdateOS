#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "isr.h"
#include "../memory/paging.h"
#include "tss.h"

extern void irq0_handler(void);
extern void irq1_handler(void);
extern void syscall_isr(void);

void isr_install(void) {
    idt_init();        // carrega IDT com handlers padrão
    pic_remap();       // remapeia PIC para 0x20 e 0x28
    // registrar handlers
    idt_set_handler(0x20, irq0_handler);
    idt_set_handler(0x21, irq1_handler);
    idt_set_handler(0x80, syscall_isr);
    pic_enable_irq(0);
    pic_enable_irq(1);
    enable_interrupts();
}
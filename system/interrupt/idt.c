#include "idt.h"
#include "../libs/string.h"

static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

static void default_handler(void) {
    txt_print("Exceção não tratada!\n");
    disable_interrupts();
    while (1) { __asm__ volatile ("hlt"); }
}

static void idt_set_gate(uint8_t vector, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[vector].base_low = base & 0xFFFF;
    idt[vector].base_high = (base >> 16) & 0xFFFF;
    idt[vector].selector = selector;
    idt[vector].zero = 0;
    idt[vector].flags = flags;
}

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)default_handler, 0x08, 0x8E);
    }

    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}

void idt_set_handler(uint8_t vector, void (*handler)(void)) {
    if (vector < 256) {
        idt_set_gate(vector, (uint32_t)handler, 0x08, 0x8E);
    }
}

#include "gdt.h"
#include "tss.h"
#include <stdint.h>

static gdt_entry_t gdt[4];
static gdt_ptr_t gdt_ptr;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low   = (base & 0xFFFF);
    gdt[num].base_mid   = (base >> 16) & 0xFF;
    gdt[num].base_high  = (base >> 24) & 0xFF;
    gdt[num].limit_low  = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access     = access;
}

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint32_t)&gdt;

    gdt_set_gate(GDT_NULL_INDEX, 0, 0, 0, 0);
    gdt_set_gate(GDT_CODE_INDEX, 0, 0xFFFFFFFF, 0x9A, 0xCF);  // kernel code
    gdt_set_gate(GDT_DATA_INDEX, 0, 0xFFFFFFFF, 0x92, 0xCF);  // kernel data
    gdt_set_gate(GDT_TSS_INDEX, 0, 0, 0x89, 0x40);            // será atualizado depois

    __asm__ volatile ("lgdt %0" : : "m"(gdt_ptr));
    __asm__ volatile (
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
        : : : "eax"
    );
}

void gdt_update_tss(uint32_t base, uint32_t limit) {
    gdt_set_gate(GDT_TSS_INDEX, base, limit, 0x89, 0x40);
    __asm__ volatile ("lgdt %0" : : "m"(gdt_ptr));
}
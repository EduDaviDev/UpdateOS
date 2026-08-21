#include "tss.h"
#include "gdt.h"
#include "../memory/paging.h"
#include "../libs/memory.h"
#include "../libs/string.h"

static tss_t tss __attribute__((aligned(4096)));

void tss_init(void) {
    memset(&tss, 0, sizeof(tss_t));

    // Pilha do kernel – uma página estática
    static uint8_t kernel_stack[4096] __attribute__((aligned(4096)));
    uint32_t kernel_stack_top = (uint32_t)(kernel_stack + sizeof(kernel_stack));

    tss.esp0 = kernel_stack_top;
    tss.ss0  = GDT_DATA_SEL;       // 0x10
    tss.iopb_offset = sizeof(tss_t); // desabilita I/O

    uint32_t tss_base = (uint32_t)&tss;
    uint32_t tss_limit = sizeof(tss_t) - 1;
    gdt_update_tss(tss_base, tss_limit);

    __asm__ volatile ("ltr %0" : : "r"((uint16_t)GDT_TSS_SEL));
}
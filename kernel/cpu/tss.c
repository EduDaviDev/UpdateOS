#include "tss.h"
#include "gdt.h"
#include "../libs/string.h"

tss_t tss;

/* Descritor de TSS:
 *   access  = 0x89 → present=1, DPL=0, S=0, type=1001 (32-bit TSS available)
 *   gran    = 0x00 → granularidade em bytes, sem DB bit
 *   limit   = sizeof(tss) - 1 = 103
 *
 * O bit "busy" (type 1011) é setado pelo hardware quando 'ltr' roda.
 * Gravamos como 1001 (available) e deixamos a CPU mudar pra busy. */
void tss_init(uint32_t kernel_stack_top) {
    /* Zera tudo — importante porque campos não usados ficam lixo em .bss */
    uint8_t *p = (uint8_t *)&tss;
    for (uint32_t i = 0; i < sizeof(tss); i++) p[i] = 0;

    tss.ss0        = GDT_KERNEL_DATA;   /* 0x10 */
    tss.esp0       = kernel_stack_top;
    tss.iomap_base = sizeof(tss);       /* 104 > limit(103) → sem bitmap */

    /* Grava o descritor no índice 5 da GDT (seletor 0x28) */
    gdt_set_gate(5, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x00);

    /* Carrega o Task Register com 0x28.
     * A partir daqui a CPU sabe onde achar o esp0 quando
     * uma interrupção vier de ring 3. */
    __asm__ volatile ("ltr %%ax" : : "a"((uint16_t)GDT_TSS));
}

void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}
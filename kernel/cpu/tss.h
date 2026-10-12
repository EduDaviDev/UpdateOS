#ifndef CPU_TSS_H
#define CPU_TSS_H

#include <stdint.h>

/* Layout exato do TSS de 32-bit (sem bitmap de I/O) = 104 bytes */
typedef struct {
    uint32_t prev_tss;      /* usado só em hardware task switch — ignoramos */
    uint32_t esp0;          /* stack do kernel quando CPU sobe pra ring 0 */
    uint32_t ss0;           /* segmento de dados do kernel (0x10) */
    uint32_t esp1, ss1;     /* ring 1 — não usamos */
    uint32_t esp2, ss2;     /* ring 2 — não usamos */
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;    /* offset do bitmap de I/O; > limit = sem bitmap */
} __attribute__((packed)) tss_t;

extern tss_t tss;

/* Preenche a struct, grava o descritor no índice 5 da GDT e carrega o TR.
 * Chame DEPOIS de gdt_init() e com uma stack de kernel já válida. */
void tss_init(uint32_t kernel_stack_top);

/* Atualiza só o esp0 (para trocar de processo/trocar de stack). */
void tss_set_kernel_stack(uint32_t esp0);

#endif
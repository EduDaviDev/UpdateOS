#ifndef CPU_ISR_H
#define CPU_ISR_H

#include <stdint.h>

typedef struct registers {
    uint32_t ds;                                     /* segmento de dados empilhado */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
    uint32_t int_no, err_code;                       /* número da interrupção + erro */
    uint32_t eip, cs, eflags, useresp, ss;           /* empurrados pela CPU */
} registers_t;

typedef void (*isr_t)(registers_t *);

void isr_install(void);
void register_interrupt_handler(uint8_t n, isr_t handler);
void isr_handler(registers_t *regs);

#endif
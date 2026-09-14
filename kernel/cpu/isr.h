#ifndef CPU_ISR_H
#define CPU_ISR_H

#include <stdint.h>

typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
} registers_t;

typedef void (*isr_handler_t)(registers_t *);

/* Array global: cada vetor (0..255) pode ter um handler registrado. */
extern isr_handler_t interrupt_handlers[256];

/* Registra um handler para o vetor n. */
void register_interrupt_handler(uint8_t n, isr_handler_t handler);

/* Registra os 32 stubs de exceção no IDT.
 * Chame depois de idt_init(). IRQs são registradas por irq_install(). */
void isr_init(void);

/* Dispatcher em C, chamado pelo asm. Não chame direto. */
void isr_handler_c(registers_t *regs);

const char *isr_exception_name(uint32_t int_no);

#endif /* CPU_ISR_H */
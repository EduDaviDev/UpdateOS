#ifndef PAGE_FAULT_H
#define PAGE_FAULT_H

#include <stdint.h>
#include "isr.h"      /* for registers_t */

/* Handler C. Compatível com isr_t (recebe regs). */
void page_fault_handler(registers_t *regs);

/* Trigger deliberado de um page fault, para testar o handler.
 * NÃO RETORNA — o handler vai imprimir e halter. */
void page_fault_test(void);

#endif /* PAGE_FAULT_H */
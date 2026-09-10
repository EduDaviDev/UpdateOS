#ifndef CPU_IRQ_H
#define CPU_IRQ_H

#include "isr.h"

void irq_install(void);
void irq_handler(registers_t *regs);

#endif
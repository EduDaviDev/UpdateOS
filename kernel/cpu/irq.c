#include "irq.h"
#include "pic.h"
#include "isr.h"
#include "../io.h"

extern isr_t interrupt_handlers[256];

/* Handler padrão para IRQs sem handler registrado */
static void irq_default_handler(registers_t *regs)
{
	(void)regs;
}

void irq_install(void)
{
	/* Instala handler padrão para todas as IRQs (32-47) */
	for (int i = 32; i < 48; i++)
	{
		register_interrupt_handler(i, irq_default_handler);
	}
}

/* Chamado pelo stub em assembly para todas as IRQs */
void irq_handler(registers_t *regs)
{
	/* Envia EOI ao PIC escravo se a IRQ veio dele */
	if (regs->int_no >= 40)
	{
		outb(PIC2_COMMAND, PIC_EOI);
	}
	/* EOI ao PIC mestre */
	outb(PIC1_COMMAND, PIC_EOI);

	if (interrupt_handlers[regs->int_no] != 0)
	{
		isr_t handler = interrupt_handlers[regs->int_no];
		handler(regs);
	}
}
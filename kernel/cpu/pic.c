#include "pic.h"
#include "../io.h"

void pic_remap(void)
{
	uint8_t mask1 = inb(PIC1_DATA);
	uint8_t mask2 = inb(PIC2_DATA);

	/* ICW1: inicia sequência de inicialização */
	outb(PIC1_COMMAND, 0x11);
	io_wait();
	outb(PIC2_COMMAND, 0x11);
	io_wait();

	/* ICW2: offsets dos vetores */
	outb(PIC1_DATA, PIC1_OFFSET); // mestre -> 0x20 (32)
	io_wait();
	outb(PIC2_DATA, PIC2_OFFSET); // escravo -> 0x28 (40)
	io_wait();

	/* ICW3: conexão entre mestre e escravo */
	outb(PIC1_DATA, 0x04); // escravo no IRQ2
	io_wait();
	outb(PIC2_DATA, 0x02); // identidade do escravo
	io_wait();

	/* ICW4: modo 8086 */
	outb(PIC1_DATA, 0x01);
	io_wait();
	outb(PIC2_DATA, 0x01);
	io_wait();

	/* Restaura as máscaras salvas */
	outb(PIC1_DATA, mask1);
	outb(PIC2_DATA, mask2);
}

void pic_send_eoi(uint8_t irq)
{
	if (irq >= 8)
	{
		outb(PIC2_COMMAND, PIC_EOI);
	}
	outb(PIC1_COMMAND, PIC_EOI);
}

void pic_set_mask(uint8_t irq)
{
	uint16_t port;
	uint8_t value;

	if (irq < 8)
	{
		port = PIC1_DATA;
	}
	else
	{
		port = PIC2_DATA;
		irq -= 8;
	}
	value = inb(port) | (1 << irq);
	outb(port, value);
}

void pic_clear_mask(uint8_t irq)
{
	uint16_t port;
	uint8_t value;

	if (irq < 8)
	{
		port = PIC1_DATA;
	}
	else
	{
		port = PIC2_DATA;
		irq -= 8;
	}
	value = inb(port) & ~(1 << irq);
	outb(port, value);
}
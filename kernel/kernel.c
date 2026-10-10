#include <stdint.h>
#include <stddef.h>

#include "updgr.h"
#include "mboot.h"
#include "drivers/serial.h"
#include "drivers/keyboard.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/pic.h"
#include "cpu/isr.h"
#include "cpu/irq.h"
#include "cpu/paging.h"
#include "cpu/pmm.h"
#include "cpu/heap.h"
#include "libs/memory.h"
#include "libs/string.h"
#include "fonts/all-fonts.h"

struct multiboot_info *g_multiboot_info = NULL;
extern uint8_t _kernel_end;
extern uint8_t _kernel_start;

const char *int_to_str(int v) {
	return "nothing";
}

void kernel_main(uint32_t magic, void *mb_info) {
	serial_init();

	serial_print("kernel_end   = "); serial_print_hex32((uint32_t)&_kernel_end);  serial_print("\n");
	serial_print("heap_start   = "); serial_print_hex32(_kernel_end);             serial_print("\n");
	serial_print("heap_size    = "); serial_print_dec(_kernel_end);               serial_print("\n");

	if (magic == MULTIBOOT2_BOOTLOADER_MAGIC && mb_info) {
        g_multiboot_info = (struct multiboot_info *)mb_info;
    }
	if (!g_multiboot_info) {
		serial_print("Multiboot: struct cannot be initialized!");
		while(1);
	}

	gdt_init();
	idt_init();
	isr_install();
	kbd_init();
	paging_init();
	pmm_init(g_multiboot_info);
	heap_init();

	__asm__ volatile("sti");

	if (!gfx_init()) {
		serial_print("Gfx: Cannot initialize them");
		while (1);
	}

	// Estado das teclas seguradas (fora do loop)
	int y = 0;
	int velocity = 1;
	int max_velocity = 50;

	while (1) {
	    /* Dorme até a próxima IRQ (teclado, timer, etc).
	     * Isso libera a CPU e evita busy-loop. */
	    __asm__ volatile("hlt");

	    if (velocity < max_velocity) velocity += 2;
		else if (velocity >= max_velocity) {gfx_print(0,0, int_to_str(1), 0x00FFFFFF); while(1);}

		y += velocity;
		
		gfx_clear(0x00000000);
		gfx_fill_rect(((gfx_width / 2) - 10), y, 20, 20, 0x00FFFF00);
		gfx_refresh();
	}
}
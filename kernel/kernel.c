#include <stdint.h>

#include "mboot.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/irq.h"
#include "cpu/paging.h"
#include "libs/memory.h"
#include "libs/string.h"

/* =====================================================================
 * kernel_main
 *
 * Ordem de boot:
 *   1. VGA        — para conseguirmos imprimir qualquer coisa
 *   2. Multiboot  — validar magic e coletar RAM/módulos
 *   3. GDT        — segmentos do kernel
 *   4. IDT + ISR  — exceções (isr0..31)
 *   5. IRQ        — IRQs de hardware (irq0..15) + PIC
 *   6. Paging     — substitui o PD bootstrap pelo definitivo
 *   7. Heap       — malloc/calloc/realloc/free
 *   8. Drivers    — teclado, etc.
 *   9. sti        — habilita interrupções
 *  10. Demo + loop principal
 * ===================================================================== */
void kernel_main(uint32_t magic, struct multiboot_info *mbi) {

    /* -----------------------------------------------------------------
     * 1. VGA — primeira coisa, precisamos imprimir
     * ----------------------------------------------------------------- */
    vga_init();
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    vga_printf_color(VGA_LIGHT_GREEN, VGA_BLACK,
                     "====================================\n");
    vga_printf_color(VGA_LIGHT_GREEN, VGA_BLACK,
                     "  UpdateOS  -  boot\n");
    vga_printf_color(VGA_LIGHT_GREEN, VGA_BLACK,
                     "====================================\n");

    /* -----------------------------------------------------------------
     * 2. Multiboot2
     * ----------------------------------------------------------------- */
    if (!mboot_verify_magic(magic)) {
        vga_printf_color(VGA_LIGHT_RED, VGA_BLACK,
                         "[FATAL] magic Multiboot2 invalido: 0x%08x\n"
                         "        esperado: 0x%08x\n",
                         magic, MULTIBOOT2_BOOTLOADER_MAGIC);
        for (;;) __asm__ volatile("cli; hlt");
    }

    if (!mboot_is_valid(mbi)) {
        vga_printf_color(VGA_LIGHT_RED, VGA_BLACK,
                         "[FATAL] mbi invalido\n");
        for (;;) __asm__ volatile("cli; hlt");
    }

    /* Resumo de uma linha do bootloader + RAM + módulos */
    mboot_summary(mbi);

    const char *bl = mboot_bootloader(mbi);
    if (bl) vga_printf("  bootloader : %s\n", bl);

    const char *cmd = mboot_cmdline(mbi);
    if (cmd && *cmd) vga_printf("  cmdline    : \"%s\"\n", cmd);

    /* -----------------------------------------------------------------
     * 3. GDT
     * ----------------------------------------------------------------- */
    gdt_init();
    vga_printf("[ok] GDT\n");

    /* -----------------------------------------------------------------
     * 4. IDT + ISR (exceções 0..31)
     * ----------------------------------------------------------------- */
    idt_init();
    isr_init();
    vga_printf("[ok] IDT + ISR (excecoes)\n");

    /* -----------------------------------------------------------------
     * 5. IRQ (hardware) + PIC
     * ----------------------------------------------------------------- */
    irq_install();
    vga_printf("[ok] IRQ + PIC\n");

    /* -----------------------------------------------------------------
     * 6. Paging
     *
     * O boot.asm ja ativou uma paginacao bootstrap cobrindo 16 MiB.
     * Agora instalamos o Page Directory definitivo, cobrindo toda a RAM.
     * ----------------------------------------------------------------- */
    uint32_t mem_bytes = mboot_mem_bytes(mbi);
    if (mem_bytes == 0) {
        vga_printf_color(VGA_YELLOW, VGA_BLACK,
                         "[!] RAM nao detectada, assumindo 128 MiB\n");
        mem_bytes = 128u * 1024u * 1024u;
    }

    paging_init(mem_bytes);
    vga_printf("[ok] Paging   - RAM=%u MiB  frames: %u/%u livres\n",
               mem_bytes / (1024u * 1024u),
               pmm_free_frames(), pmm_total_frames());

    /* -----------------------------------------------------------------
     * 7. Heap (malloc family)
     * ----------------------------------------------------------------- */
    heap_init();
    vga_printf("[ok] Heap     - %u KiB VA reservado\n",
               (uint32_t)(heap_total() / 1024));

    /* -----------------------------------------------------------------
     * 8. Drivers
     * ----------------------------------------------------------------- */
    kbd_init();
    vga_printf("[ok] Teclado\n");

    /* -----------------------------------------------------------------
     * 9. Habilita interrupcoes
     * ----------------------------------------------------------------- */
    __asm__ volatile("sti");
    vga_printf("[ok] Interrupcoes habilitadas\n");

    /* -----------------------------------------------------------------
     * 10. Testes rápidos (remova quando nao precisar mais)
     * ----------------------------------------------------------------- */
    vga_printf_color(VGA_LIGHT_CYAN, VGA_BLACK, "\n--- self-test ---\n");

    /* Teste do heap: alocar, escrever, realocar, liberar */
    {
        char *buf = (char *)malloc(64);
        if (buf) {
            for (int i = 0; i < 63; i++) buf[i] = 'A' + (i % 26);
            buf[63] = '\0';
            vga_printf("  malloc(64)  -> %p  \"%s\"\n", (void *)buf, buf);

            char *big = (char *)realloc(buf, 4096);
            if (big) {
                vga_printf("  realloc(4K) -> %p\n", (void *)big);
                free(big);
            } else {
                free(buf);
            }
        }

        int *arr = (int *)calloc(128, sizeof(int));
        if (arr) {
            arr[0] = 0xDEADBEEF;
            arr[127] = 0xCAFEBABE;
            vga_printf("  calloc(128) -> %p  arr[0]=0x%08x arr[127]=0x%08x\n",
                       (void *)arr, arr[0], arr[127]);
            free(arr);
        }
    }

    /* Teste do paging: traduzir endereço virtual -> físico */
    vga_printf("  virt %p -> phys %p\n",
               (void *)0x00100000, (void *)paging_get_physical(0x00100000));

    heap_dump();

    /* -----------------------------------------------------------------
     * 11. Pronto
     * ----------------------------------------------------------------- */
    vga_printf_color(VGA_LIGHT_GREEN, VGA_BLACK,
                     "\n[UpdateOS] Pronto.\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    /* -----------------------------------------------------------------
     * Loop principal
     * ----------------------------------------------------------------- */
    for (;;) {
    	if (kbd_has_event) vga_putchar(kbd_getc());
	    __asm__ volatile("hlt");
	}
}
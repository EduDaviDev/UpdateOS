#include <stdint.h>
#include <stddef.h>
#include "mboot.h"
#include "drivers/vga.h"

/* Função principal do kernel, chamada por boot.asm */
void kernel_main(uint32_t magic, struct multiboot_info *mb_info)
{
    /* 1. Inicializar o driver VGA */
    vga_init();

    /* 2. Verificar se fomos carregados pelo GRUB com Multiboot2 */
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        vga_print("ERRO: Bootloader nao e compativel com Multiboot2!\n");
        vga_print("Magic recebido: 0x");
        vga_print_hex(magic);
        vga_print("\n");
        return;
    }

    /* 3. Exibir mensagem de boas-vindas */
    vga_clear();
    vga_print("UpdateOS v0.1\n");
    vga_print("===============\n\n");
    vga_print("Kernel carregado com sucesso via Multiboot2.\n");
    vga_print("Sistema operacional leve e estavel.\n\n");

    /* 4. Exibir informações de memória (se disponíveis) */
    struct multiboot_tag *tag;
    for (tag = mb_info->tags;
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7)))
    {
        if (tag->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO) {
            struct multiboot_tag_basic_meminfo *mem = (struct multiboot_tag_basic_meminfo *)tag;
            vga_print("Memoria baixa: ");
            vga_print_dec(mem->mem_lower);
            vga_print(" KB\n");
            vga_print("Memoria alta:  ");
            vga_print_dec(mem->mem_upper);
            vga_print(" KB\n");
        }
    }

    vga_print("\nFunciona BR BR BR\n");

    /* 5. Loop infinito (o kernel não deve retornar) */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
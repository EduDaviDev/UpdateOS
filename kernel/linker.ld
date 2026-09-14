/* linker.ld - Script de linkagem do UpdateOS */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* Símbolo do início do kernel (alinhado a 4 KiB) */
    kernel_start = .;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot_header)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /* Fim do kernel — útil para o alocador de frames */
    . = ALIGN(4K);
    kernel_end = .;
}
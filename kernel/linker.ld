/* linker.ld - Script de linkagem do UpdateOS */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    /* O kernel é carregado a partir de 1 MiB */
    . = 1M;

	_kernel_start = .;

    /* Seção de código + dados somente leitura */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot_header)
        *(.text)
    }

    /* Dados somente leitura */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Dados inicializados */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Dados não inicializados (BSS) */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /* Símbolos para o final do kernel (útil para alocador de memória) */
	_kernel_end = .;
}
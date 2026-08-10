/* link.ld - Script de linker para kernel Multiboot2 (i386) */

OUTPUT_FORMAT(elf32-i386)           /* Saída em ELF 32 bits para i386 */
ENTRY(start)                        /* Ponto de entrada: símbolo start */

SECTIONS
{
    /* Endereço base padrão para kernels bootados pelo GRUB (1 MiB) */
    . = 0x100000;

    /* ============================================================
     * Cabeçalho Multiboot2 – deve vir primeiro e estar alinhado a 8
     * ============================================================ */
    .multiboot ALIGN(8) : {
        KEEP(*(.multiboot))         /* Mantém mesmo se não referenciado */
    }

    /* ============================================================
     * Seção de código executável
     * ============================================================ */
    .text ALIGN(4K) : {
        *(.text .text.*)            /* .text e subseções (ex: .text.startup) */
    }

    /* ============================================================
     * Dados somente leitura (constantes, strings, etc.)
     * ============================================================ */
    .rodata ALIGN(4K) : {
        *(.rodata .rodata.*)
    }

    /* ============================================================
     * Dados inicializados (variáveis globais com valor)
     * ============================================================ */
    .data ALIGN(4K) : {
        *(.data .data.*)
    }

    /* ============================================================
     * Dados não inicializados (BSS) – não ocupam espaço no arquivo
     * ============================================================ */
    .bss ALIGN(4K) (NOLOAD) : {
        *(.bss .bss.*)
        *(COMMON)                   /* Símbolos comuns do linker */
    }

    /* ============================================================
     * Seções de depuração (DWARF, stabs) – úteis para debugger
     * ============================================================ */
    .debug_info     0 : { *(.debug_info) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }

    /* ============================================================
     * Seções a descartar (não necessárias no kernel final)
     * ============================================================ */
    /DISCARD/ : {
        *(.comment)                 /* Comentários do compilador */
        *(.note*)                   /* Notas (ex: .note.gnu.build-id) */
        *(.eh_frame)                /* Tratamento de exceções (não usado) */
        *(.eh_frame_hdr)
    }
}
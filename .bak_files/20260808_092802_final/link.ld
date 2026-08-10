ENTRY(_start)

SECTIONS {
    /* O cabeçalho Multiboot2 deve estar nos primeiros 8 KB */
    . = 0x100000;

    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}

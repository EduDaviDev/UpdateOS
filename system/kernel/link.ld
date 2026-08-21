OUTPUT_FORMAT(elf32-i386)
ENTRY(start)

SECTIONS {
    . = 0x100000;
    .multiboot ALIGN(8) : { KEEP(*(.multiboot)) }
    .text ALIGN(4K) : { *(.text .text.*) }
    .rodata ALIGN(4K) : { *(.rodata .rodata.*) }
    .data ALIGN(4K) : { *(.data .data.*) }
    .bss ALIGN(4K) (NOLOAD) : { *(.bss .bss.*) *(COMMON) }
}
/* apps.linker.ld — com 2 PT_LOAD, sem segmento fantasma */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

USER_BASE = 0x08048000;

PHDRS
{
    text PT_LOAD FLAGS(5);   /* R + X */
    data PT_LOAD FLAGS(6);   /* R + W */
}

SECTIONS
{
    . = USER_BASE;

    .text : ALIGN(0x1000)
    {
        KEEP(*(.text.entry))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    } :text

    .data : ALIGN(0x1000)
    {
        *(.data)
        *(.data.*)
    } :data

    .bss : ALIGN(0x1000)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } :data

    __user_start = USER_BASE;
    __user_end   = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.gcc_except_table*)
    }
}
; mboot.asm - Multiboot2 header e entry point

section .multiboot
align 8
    dd 0xE85250D6                 ; magic number
    dd 0                           ; architecture (i386)
    dd header_end - header_start   ; header length
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) ; checksum

header_start:
    ; tag de terminação (obrigatória)
    align 8
    dw 0   ; type
    dw 0   ; flags
    dd 8   ; size
header_end:

section .text
global start
extern kernel_main

start:
    mov esp, stack_top
    push eax    ; magic number
    push ebx    ; multiboot info struct
    call kernel_main
    cli
    hlt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
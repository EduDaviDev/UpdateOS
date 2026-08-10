; mboot.asm – Multiboot2 padrão para i386
BITS 32

section .multiboot
align 4
    dd 0xE85250D6                 ; magic
    dd 0                          ; architecture (0 = i386)
    dd header_end - header_start  ; header length
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) ; checksum

header_start:
    ; Tag de terminação obrigatória
    align 8
    dw 0                          ; type
    dw 0                          ; flags
    dd 8                          ; size
header_end:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    push eax      ; magic
    push ebx      ; addr da estrutura Multiboot2
    call kernel_main
    cli
    hlt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

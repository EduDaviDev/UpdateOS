; boot.asm - Ponto de entrada do UpdateOS
; Suporta Multiboot2, arquitetura i386 (32 bits)

MB2_MAGIC      equ 0xE85250D6
MB2_ARCH       equ 0            ; 0 = i386 (32-bit)
MB2_HEADER_LEN equ mb2_header_end - mb2_header_start
MB2_CHECKSUM   equ -(MB2_MAGIC + MB2_ARCH + MB2_HEADER_LEN)

section .multiboot_header
align 8
mb2_header_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd MB2_HEADER_LEN
    dd MB2_CHECKSUM

    ; Tag de fim (obrigatória)
    align 8
    dw 0        ; type = 0 (end tag)
    dw 0        ; flags
    dd 8        ; size = 8
mb2_header_end:

section .text
global _start
extern kernel_main

_start:
    ; A stack ainda não está configurada. O GRUB não garante uma stack válida.
    ; Vamos configurar uma stack temporária de 16 KiB.
    mov esp, stack_top

    ; Limpar o registrador de flags de interrupção (opcional, mas seguro)
    cli

    ; Chamar o kernel_main em C
    ; Os argumentos Multiboot2 (magic e info) são passados em EAX e EBX
    ; Vamos passá-los como argumentos para kernel_main
    push ebx        ; ponteiro para a estrutura de informações Multiboot2
    push eax        ; magic number (deve ser 0x36d76289)
    call kernel_main

    ; Se kernel_main retornar, entra em loop infinito
.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384      ; 16 KiB de stack
stack_top:
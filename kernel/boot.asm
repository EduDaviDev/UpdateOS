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

    ; ---- Framebuffer request tag (type=5) ----
    align 8
    dw 5            ; type  = 5 (framebuffer request)
    dw 1            ; flags = 1 (optional: continuar mesmo se não suportado)
    dd 20           ; size  = 20 bytes
    dd 1024         ; width desejada
    dd 768          ; height desejada
    dd 32           ; depth em bits (32bpp)

    ; ---- End tag (obrigatória) ----
    align 8
    dw 0            ; type  = 0
    dw 0            ; flags
    dd 8            ; size  = 8
mb2_header_end:

section .text
global _start
extern kernel_main
extern paging_init_early

_start:
    ; Configurar stack temporária
    mov esp, stack_top
    cli

    ; -------------------------------------------------------------------
    ; 1) PRESERVAR os argumentos do Multiboot2 ANTES de chamar a paginação
    ;    (paging_init_early clobbera EAX)
    ; -------------------------------------------------------------------
    push ebx        ; info (struct multiboot2)
    push eax        ; magic (0x36D76289)

    ; -------------------------------------------------------------------
    ; 2) Inicializar paginação (identity map 0..4 MiB, CR3, CR0.PG=1)
    ; -------------------------------------------------------------------
    call paging_init_early

    ; -------------------------------------------------------------------
    ; 3) Recuperar os argumentos da stack
    ;    A call empilha/desempilha o return address sozinha, então os
    ;    pushes de antes continuam no topo.
    ; -------------------------------------------------------------------
    pop eax         ; magic
    pop ebx         ; info

    ; -------------------------------------------------------------------
    ; 4) Passar para kernel_main (cdecl: último argumento primeiro)
    ; -------------------------------------------------------------------
    push ebx        ; 2º arg (mb_info)
    push eax        ; 1º arg (magic)
    call kernel_main

.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
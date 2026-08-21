; mboot.asm - Multiboot2 header, paginação inicial e entry point

section .multiboot
align 8
    dd 0xE85250D6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start))

header_start:
    align 8
    dw 0
    dw 0
    dd 8
header_end:

section .text
global start
extern kernel_main

start:
    ; ---- Limpa page directory (0x1000) ----
    mov edi, 0x1000
    mov ecx, 1024
    xor eax, eax
    rep stosd

    ; ---- Page table 1 (0x2000) para 0-4MB ----
    mov edi, 0x2000
    mov ecx, 1024
    xor eax, eax
    rep stosd

    mov edi, 0x2000
    mov eax, 0x3
    mov ecx, 1024
.fill_pt1:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .fill_pt1

    ; ---- Page table 2 (0x3000) para 4-8MB ----
    mov edi, 0x3000
    mov ecx, 1024
    xor eax, eax
    rep stosd

    mov edi, 0x3000
    mov eax, 0x3 | (1024 << 12)   ; primeiro frame da segunda tabela é 1024 (4MB)
    mov ecx, 1024
.fill_pt2:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .fill_pt2

    ; ---- Configura page directory ----
    ; Entrada 0 aponta para PT1 (0x2000)
    mov dword [0x1000], 0x2003
    ; Entrada 1 aponta para PT2 (0x3000)
    mov dword [0x1004], 0x3003    ; 0x1004 = offset da segunda entrada

    ; ---- Carrega CR3 ----
    mov eax, 0x1000
    mov cr3, eax

    ; ---- Ativa paginação ----
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; ---- Pilha e chama kernel ----
    mov esp, stack_top
    push ebx
    push eax
    call kernel_main
    cli
    hlt
    jmp $

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
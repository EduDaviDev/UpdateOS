; boot.asm - Ponto de entrada do UpdateOS
; Suporta Multiboot2, arquitetura i386 (32 bits)
;
; Fluxo:
;   1. Configura stack temporária
;   2. Monta paginação mínima (identity map dos primeiros 16 MiB)
;   3. Ativa CR3 + CR0.PG
;   4. Chama kernel_main(magic, mb2_info)
;
; O kernel C deve, em kernel_main, chamar paging_init(mem_size) para
; instalar o Page Directory definitivo cobrindo toda a RAM.

MB2_MAGIC      equ 0xE85250D6
MB2_ARCH       equ 0            ; 0 = i386 (32-bit)
MB2_HEADER_LEN equ mb2_header_end - mb2_header_start
MB2_CHECKSUM   equ -(MB2_MAGIC + MB2_ARCH + MB2_HEADER_LEN)

; Flags de entrada de página
PAGE_PRESENT   equ 1 << 0
PAGE_RW        equ 1 << 1

; Cobertura do bootstrap: 4 page tables * 4 MiB = 16 MiB
BOOT_PT_COUNT  equ 4

section .multiboot_header
align 8
mb2_header_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd MB2_HEADER_LEN
    dd MB2_CHECKSUM

    align 8
    dw 0        ; type = 0 (end tag)
    dw 0        ; flags
    dd 8        ; size = 8
mb2_header_end:

section .text
global _start
extern kernel_main

_start:
    ; ------------------------------------------------------------------
    ; 1. Stack temporária
    ; ------------------------------------------------------------------
    cli
    mov esp, stack_top

    ; ------------------------------------------------------------------
    ; 2. Preservar argumentos Multiboot2 em registradores que o loop
    ;    de preenchimento NÃO usa (o loop usa EDI/EAX/EBX/ECX).
    ; ------------------------------------------------------------------
    mov esi, eax                    ; magic  (0x36d76289)
    mov ebp, ebx                    ; ptr para mb2_info

    ; ------------------------------------------------------------------
    ; 3. Construir as 4 Page Tables (identity map 0..16 MiB)
    ; ------------------------------------------------------------------
    mov     edi, boot_page_table0
    mov     eax, PAGE_PRESENT | PAGE_RW
    mov     ecx, BOOT_PT_COUNT * 1024
.fill_pt:
    mov     [edi], eax
    add     edi, 4
    add     eax, 0x1000
    loop    .fill_pt

    ; ------------------------------------------------------------------
    ; 4. Construir o Page Directory
    ; ------------------------------------------------------------------
    mov     edi, boot_page_directory
    mov     eax, boot_page_table0
    mov     ecx, BOOT_PT_COUNT
.fill_pd:
    mov     ebx, eax
    or      ebx, PAGE_PRESENT | PAGE_RW
    mov     [edi], ebx
    add     edi, 4
    add     eax, 0x1000
    loop    .fill_pd

    ; ------------------------------------------------------------------
    ; 5. Carregar o Page Directory em CR3
    ; ------------------------------------------------------------------
    mov     eax, boot_page_directory
    mov     cr3, eax

    ; ------------------------------------------------------------------
    ; 6. Ativar paginação (CR0.PG = bit 31)
    ; ------------------------------------------------------------------
    mov     eax, cr0
    or      eax, 0x80000000
    mov     cr0, eax

    ; ------------------------------------------------------------------
    ; 7. Chamar kernel_main(magic, mb2_info)
    ;    cdecl: push do 2º argumento, depois do 1º.
    ;    ESI = magic, EBP = mb2_info  (preservados até aqui).
    ; ------------------------------------------------------------------
    push    ebp                     ; 2º argumento: mb2_info
    push    esi                     ; 1º argumento: magic
    call    kernel_main

.halt:
    hlt
    jmp .halt

; ======================================================================
; Dados não inicializados
; ======================================================================
section .bss

; Page Directory alinhado a 4 KiB
align 4096
boot_page_directory:
    resb 4096

; 4 Page Tables contíguas, cada uma cobrindo 4 MiB
boot_page_table0:
    resb 4096
boot_page_table1:
    resb 4096
boot_page_table2:
    resb 4096
boot_page_table3:
    resb 4096

; Stack do kernel (16 KiB)
align 16
stack_bottom:
    resb 16384
stack_top:
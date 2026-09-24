; paging.asm - Operações de baixo nível de paginação x86 (32 bits)
; NASM syntax

section .data
align 4096
global boot_page_directory
global boot_page_table0

; Diretório de páginas (1024 entradas de 4 bytes = 4 KiB)
boot_page_directory:
    ; Entrada 0 -> aponta para boot_page_table0, flags = Present | R/W (0x03)
    dd boot_page_table0 + 0x03
    times 1023 dd 0x00000000    ; demais entradas não presentes

align 4096
; Tabela de páginas que mapeia os primeiros 4 MiB (1024 páginas de 4 KiB)
boot_page_table0:
%assign i 0
%rep 1024
    dd (i * 0x1000) | 0x03      ; endereço físico da página | Present | R/W
    %assign i i+1
%endrep

section .text
global paging_init_early
global paging_load_directory
global paging_enable
global paging_disable
global paging_invalidate_tlb
global paging_get_fault_address
global paging_read_cr0
global paging_read_cr3

; ---------------------------------------------------------------------------
; void paging_init_early(void)
; Carrega o diretório estático em CR3 e habilita o bit PG de CR0.
; Deve ser chamado ANTES de qualquer acesso a endereços > 4 MiB.
; ---------------------------------------------------------------------------
paging_init_early:
    mov eax, boot_page_directory
    mov cr3, eax

    mov eax, cr0
    or  eax, 0x80000000         ; seta PG (bit 31)
    mov cr0, eax
    ret

; ---------------------------------------------------------------------------
; void paging_load_directory(uint32_t *dir)
; Carrega um diretório de páginas em CR3.
; ---------------------------------------------------------------------------
paging_load_directory:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

; ---------------------------------------------------------------------------
; void paging_enable(void)
; ---------------------------------------------------------------------------
paging_enable:
    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax
    ret

; ---------------------------------------------------------------------------
; void paging_disable(void)
; ---------------------------------------------------------------------------
paging_disable:
    mov eax, cr0
    and eax, 0x7FFFFFFF
    mov cr0, eax
    ret

; ---------------------------------------------------------------------------
; void paging_invalidate_tlb(uint32_t virt)
; Invalida a entrada de uma página virtual na TLB.
; ---------------------------------------------------------------------------
paging_invalidate_tlb:
    mov eax, [esp + 4]
    invlpg [eax]
    ret

; ---------------------------------------------------------------------------
; uint32_t paging_get_fault_address(void)
; Retorna o valor de CR2 (endereço que gerou o page fault).
; ---------------------------------------------------------------------------
paging_get_fault_address:
    mov eax, cr2
    ret

; ---------------------------------------------------------------------------
; uint32_t paging_read_cr0(void)
; ---------------------------------------------------------------------------
paging_read_cr0:
    mov eax, cr0
    ret

; ---------------------------------------------------------------------------
; uint32_t paging_read_cr3(void)
; ---------------------------------------------------------------------------
paging_read_cr3:
    mov eax, cr3
    ret
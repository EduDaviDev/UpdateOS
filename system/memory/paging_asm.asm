global enable_paging
global reload_cr3

section .text

; void enable_paging(uint32_t page_dir)
enable_paging:
    mov eax, [esp + 4]      ; page_dir físico
    mov cr3, eax            ; carrega CR3 com endereço do page directory

    mov eax, cr0
    or eax, 0x80000000      ; seta bit PG (paginação)
    mov cr0, eax

    ret

reload_cr3:
    mov eax, cr3
    mov cr3, eax            ; recarrega CR3 para flush TLB
    ret
; interrupt.asm
; Contém:
;   - gdt_flush / idt_flush
;   - stubs de exceções  isr0..isr31
;   - stubs de IRQ       irq0..irq15
;   - duas common stubs (isr_common -> isr_handler_c, irq_common -> irq_handler)

section .text

extern isr_handler_c
extern irq_handler

; =====================================================================
; GDT / IDT flush
; =====================================================================
global gdt_flush
gdt_flush:
    mov eax, [esp + 4]      ; argumento: ponteiro para gdtr
    lgdt [eax]

    mov ax, 0x10            ; seletor de dados do kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.reload_cs     ; far jump para recarregar CS
.reload_cs:
    ret

global idt_flush
idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; =====================================================================
; Exceções (0..31)
;   - Se a CPU não empilha error code: stub empilha 0
;   - Sempre empilha int_no
; =====================================================================
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0
    push dword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push dword %1           ; error code já empilhado pela CPU
    jmp isr_common
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; =====================================================================
; IRQs (0..15 -> vetores 32..47)
;   IRQs NÃO empilham error code. Sempre empilha 0.
; =====================================================================
%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0
    push dword %2           ; vetor (32 + n)
    jmp irq_common
%endmacro

IRQ  0, 32
IRQ  1, 33
IRQ  2, 34
IRQ  3, 35
IRQ  4, 36
IRQ  5, 37
IRQ  6, 38
IRQ  7, 39
IRQ  8, 40
IRQ  9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; =====================================================================
; isr_common -> isr_handler_c(registers_t*)
;
; Layout da pilha ao entrar:
;   [esp+0]  int_no
;   [esp+4]  err_code
;   [esp+8]  eip
;   [esp+12] cs
;   [esp+16] eflags
; =====================================================================
isr_common:
    pusha

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler_c
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds

    popa
    add esp, 8              ; descarta int_no + err_code
    iret

; =====================================================================
; irq_common -> irq_handler(registers_t*)
; =====================================================================
irq_common:
    pusha

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds

    popa
    add esp, 8
    iret
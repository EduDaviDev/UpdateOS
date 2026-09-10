[BITS 32]

; ============================================================
;  Exportações
; ============================================================
global gdt_flush
global idt_flush

; Stubs de exceções (ISRs)
%assign i 0
%rep 32
    global isr%+i
%assign i i+1
%endrep

; Stubs de IRQs
%assign i 0
%rep 16
    global irq%+i
%assign i i+1
%endrep

extern isr_handler
extern irq_handler

; ============================================================
;  GDT flush
; ============================================================
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10        ; seletor de dados do kernel (GDT_KERNEL_DATA)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.reload_cs ; seletor de código do kernel (GDT_KERNEL_CODE)
.reload_cs:
    ret

; ============================================================
;  IDT flush
; ============================================================
idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; ============================================================
;  Stub comum de ISR (exceções)
; ============================================================
isr_common_stub:
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8      ; limpa int_no e err_code
    iret

; ============================================================
;  Stub comum de IRQ
; ============================================================
irq_common_stub:
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    iret

; ============================================================
;  Macros para gerar stubs
; ============================================================
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push dword 0        ; err_code dummy
    push dword %1       ; int_no
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push dword %1       ; int_no (err_code já foi empilhado pela CPU)
    jmp isr_common_stub
%endmacro

%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0
    push dword %2
    jmp irq_common_stub
%endmacro

; ============================================================
;  Exceções da CPU (0-31)
;  Erros com código: 8, 10, 11, 12, 13, 14, 17
; ============================================================
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; ============================================================
;  IRQs remapeadas (vetores 32-47)
; ============================================================
IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47
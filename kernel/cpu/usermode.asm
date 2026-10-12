; kernel/cpu/usermode.asm
; Entrada em ring 3 via iret.

[BITS 32]

section .text
global enter_user_mode

; ---------------------------------------------------------------------------
; void enter_user_mode(uint32_t entry, uint32_t user_stack) __attribute__((noreturn));
;
; cdecl: [esp+4] = entry, [esp+8] = user_stack
; NUNCA retorna.
; ---------------------------------------------------------------------------
enter_user_mode:
    cli                         ; sem interrupção durante a transição

    mov eax, [esp + 4]          ; entry point
    mov ebx, [esp + 8]          ; user stack top

    ; Segmentos de dados: user data
    ;   USER_SS = GDT_USER_DATA | RPL 3 = 0x20 | 3 = 0x23
    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    ; Frame que o iret espera (ordem: SS, ESP, EFLAGS, CS, EIP)
    push dword 0x23             ; SS  = user data RPL 3
    push ebx                    ; ESP = user stack top
    push dword 0x202            ; EFLAGS: IF=1 (interrupções ligadas), resto zero
    push dword 0x1B             ; CS  = GDT_USER_CODE | RPL 3 = 0x18 | 3
    push eax                    ; EIP = entry point

    iret                        ; cai em ring 3

    ; Se chegar aqui, o iret falhou — trava
.halt:
    hlt
    jmp .halt
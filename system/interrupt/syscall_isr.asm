; syscall_isr.asm
[GLOBAL syscall_isr]
[EXTERN syscall_eax]
[EXTERN syscall_ebx]
[EXTERN syscall_ecx]
[EXTERN syscall_edx]
[EXTERN syscall_esi]
[EXTERN syscall_edi]
[EXTERN syscall_ebp]
[EXTERN uex_syscall_handler]

syscall_isr:
    ; Salva os registradores nas variáveis globais
    mov [syscall_eax], eax
    mov [syscall_ebx], ebx
    mov [syscall_ecx], ecx
    mov [syscall_edx], edx
    mov [syscall_esi], esi
    mov [syscall_edi], edi
    mov [syscall_ebp], ebp

    ; Chama o handler C
    call uex_syscall_handler

    ; Restaura os registradores (opcional, mas seguro)
    mov eax, [syscall_eax]
    mov ebx, [syscall_ebx]
    mov ecx, [syscall_ecx]
    mov edx, [syscall_edx]
    mov esi, [syscall_esi]
    mov edi, [syscall_edi]
    mov ebp, [syscall_ebp]

    iret
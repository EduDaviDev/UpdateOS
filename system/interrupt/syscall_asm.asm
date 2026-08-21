; syscall_asm.asm
[GLOBAL syscall_isr]
[GLOBAL syscall_eax]
[GLOBAL syscall_ebx]
[GLOBAL syscall_ecx]
[GLOBAL syscall_edx]
[GLOBAL syscall_esi]
[GLOBAL syscall_edi]
[GLOBAL syscall_ebp]

[EXTERN syscall_handler]

section .data
syscall_eax dd 0
syscall_ebx dd 0
syscall_ecx dd 0
syscall_edx dd 0
syscall_esi dd 0
syscall_edi dd 0
syscall_ebp dd 0

section .text
syscall_isr:
    ; Salva os registradores nas variáveis globais
    mov [syscall_eax], eax
    mov [syscall_ebx], ebx
    mov [syscall_ecx], ecx
    mov [syscall_edx], edx
    mov [syscall_esi], esi
    mov [syscall_edi], edi
    mov [syscall_ebp], ebp

    call syscall_handler

    ; (Opcional) restaura os registradores
    mov eax, [syscall_eax]
    mov ebx, [syscall_ebx]
    mov ecx, [syscall_ecx]
    mov edx, [syscall_edx]
    mov esi, [syscall_esi]
    mov edi, [syscall_edi]
    mov ebp, [syscall_ebp]

    iret
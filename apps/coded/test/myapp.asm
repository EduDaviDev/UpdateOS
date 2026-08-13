global _start

section .text
_start:
    ; Obtém o endereço da string via call/pop
    call get_msg
    db 'Hello World!', 0x0A, 0   ; string com newline
get_msg:
    pop ebx              ; EBX agora aponta para a string
    mov eax, 1           ; sys_write
    mov ecx, 12          ; tamanho (sem contar o terminador)
    int 0x80

    mov eax, 0           ; sys_exit
    xor ebx, ebx
    int 0x80
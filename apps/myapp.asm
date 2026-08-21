global _start

section .text
_start:
    mov eax, 1
	mov ebx, hello
	mov ecx, 13
	int 0x80

	mov eax, 0
	mov ebx, 0
	int 0x80

	ret

hello: db "HELLO WORLD!",0x0a,0
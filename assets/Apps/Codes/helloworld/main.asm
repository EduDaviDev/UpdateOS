section .text
global _start

_start:
	mov eax, 1
	push msg
	int 0x80
	
	mov eax, 0
	push 0
	int 0x80

section .data
msg:
	db "Hello, world!", 0x0a, 0x00
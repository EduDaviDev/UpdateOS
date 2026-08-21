#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_EXIT        0
#define SYS_PRINT       1
#define SYS_CMD         2
#define SYS_GETPID      3
#define SYS_YIELD       4

#define MAX_SYSCALLS    64

typedef void (*syscall_func_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void syscall_init(void);
void register_syscall(uint32_t num, syscall_func_t func);

/* Handler único: processa a syscall com base nos registradores globais */
void syscall_handler(void);

/* Função para chamar syscall a partir de C: prepara registradores e chama handler */
void syscall(uint32_t num, uint32_t p1, uint32_t p2, uint32_t p3,
             uint32_t p4, uint32_t p5);

/* Macros */
#define syscall0(num)               syscall(num, 0,0,0,0,0)
#define syscall1(num, a)            syscall(num, a,0,0,0,0)
#define syscall2(num, a,b)          syscall(num, a,b,0,0,0)
#define syscall3(num, a,b,c)        syscall(num, a,b,c,0,0)
#define syscall4(num, a,b,c,d)      syscall(num, a,b,c,d,0)
#define syscall5(num, a,b,c,d,e)    syscall(num, a,b,c,d,e)

/* Syscalls padrão (implementações) */
void sys_exit(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void sys_print(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void sys_cmd(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void sys_getpid(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void sys_yield(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

/* Variáveis globais para comunicação com assembly (definidas em syscall_asm.asm) */
extern uint32_t syscall_eax, syscall_ebx, syscall_ecx, syscall_edx;
extern uint32_t syscall_esi, syscall_edi, syscall_ebp;

#endif
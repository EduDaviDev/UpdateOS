#include "syscall.h"
#include "../drivers/video.h"
#include "../kernel/upm.h"
#include <stdint.h>

/* Tabela de syscalls */
static syscall_func_t syscall_table[MAX_SYSCALLS] = {0};

/* Registro */
void register_syscall(uint32_t num, syscall_func_t func) {
    if (num < MAX_SYSCALLS)
        syscall_table[num] = func;
    else
        txt_printf("[syscall] Número %d inválido\n", num);
}

/* Handler único: lê as variáveis globais e chama a função apropriada */
void syscall_handler(void) {
    uint32_t num = syscall_eax;
    if (num >= MAX_SYSCALLS || syscall_table[num] == NULL) {
        txt_printf("[syscall] Syscall %d não registrada\n", num);
        return;
    }
    syscall_table[num](syscall_ebx, syscall_ecx, syscall_edx,
                       syscall_esi, syscall_edi, syscall_ebp);
}

/* Chamada a partir de C: coloca parâmetros nas variáveis globais e chama o handler */
void syscall(uint32_t num, uint32_t p1, uint32_t p2, uint32_t p3,
             uint32_t p4, uint32_t p5) {
    /* Escreve nos globais */
    syscall_eax = num;
    syscall_ebx = p1;
    syscall_ecx = p2;
    syscall_edx = p3;
    syscall_esi = p4;
    syscall_edi = p5;
    syscall_ebp = 0;   /* opcional, não usado */
    /* Chama o handler diretamente */
    syscall_handler();
}

/* Implementações das syscalls padrão */
void sys_exit(uint32_t code, uint32_t u1, uint32_t u2, uint32_t u3, uint32_t u4, uint32_t u5) {
    (void)u1; (void)u2; (void)u3; (void)u4; (void)u5;
    txt_printf("[syscall] Exit com código %d\n", code);
}

void sys_print(uint32_t str_ptr, uint32_t size, uint32_t u1, uint32_t u2, uint32_t u3, uint32_t u4) {
    (void)u1; (void)u2; (void)u3; (void)u4;
    if (!str_ptr) return;
    char *str = (char*)str_ptr;
    if (size == 0) txt_print(str);
    else for (uint32_t i = 0; i < size && str[i]; i++) txt_putc(str[i]);
}

void sys_cmd(uint32_t cmd_ptr, uint32_t param_ptr, uint32_t u1, uint32_t u2, uint32_t u3, uint32_t u4) {
    (void)u1; (void)u2; (void)u3; (void)u4;
    txt_printf("[syscall] Comando: %s (params: %s)\n", (char*)cmd_ptr, (char*)param_ptr);
}

void sys_getpid(uint32_t u1, uint32_t u2, uint32_t u3, uint32_t u4, uint32_t u5, uint32_t u6) {
    (void)u1; (void)u2; (void)u3; (void)u4; (void)u5; (void)u6;
    process_t *cur = upm_get_current();
    txt_printf("[syscall] PID: %d\n", cur ? cur->pid : 0);
}

void sys_yield(uint32_t u1, uint32_t u2, uint32_t u3, uint32_t u4, uint32_t u5, uint32_t u6) {
    (void)u1; (void)u2; (void)u3; (void)u4; (void)u5; (void)u6;
    txt_print("[syscall] Yield\n");
}

/* Inicialização */
void syscall_init(void) {
    register_syscall(SYS_EXIT,    sys_exit);
    register_syscall(SYS_PRINT,   sys_print);
    register_syscall(SYS_CMD,     sys_cmd);
    register_syscall(SYS_GETPID,  sys_getpid);
    register_syscall(SYS_YIELD,   sys_yield);
    txt_print("[syscall] Inicializado.\n");
}
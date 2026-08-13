#include "uex.h"
#include "../drivers/fatfs/ff.h"
#include "../drivers/video.h"
#include "../libs/memory.h"
#include "../libs/string.h"
#include <stddef.h>

// ==============================
// Array global de syscalls (inicializado com zeros)
// ==============================
Syscall Syscalls[MAX_SYSCALLS] = {0};

// ==============================
// Variáveis globais para capturar registradores da ISR
// ==============================
uint32_t syscall_eax, syscall_ebx, syscall_ecx, syscall_edx,
         syscall_esi, syscall_edi, syscall_ebp;

// ==============================
// Implementação das syscalls padrão
// ==============================

// Syscall 0: Exit
static void sys_exit(uint32_t code, uint32_t u1, uint32_t u2,
                     uint32_t u3, uint32_t u4, uint32_t u5) {
    (void)u1; (void)u2; (void)u3; (void)u4; (void)u5;
    txt_print("App finalizado com código ");
    txt_print_int(code);
    txt_newl();
}

// Syscall 1: Write
static void sys_write(uint32_t str_ptr, uint32_t len, uint32_t u1,
                      uint32_t u2, uint32_t u3, uint32_t u4) {
    (void)u1; (void)u2; (void)u3; (void)u4;
    if (!str_ptr) return;
    char *str = (char*)str_ptr;
    if (len == 0) {
        txt_print(str);
    } else {
        for (uint32_t i = 0; i < len && str[i]; i++) {
            txt_putc(str[i]);
        }
    }
}

// ==============================
// Registro de syscalls
// ==============================
static void register_syscall(uint32_t cmd, void (*func)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t), uint8_t paramcount) {
    (void)paramcount;
    if (cmd < MAX_SYSCALLS) {
        Syscalls[cmd].command = cmd;
        Syscalls[cmd].function = func;
        Syscalls[cmd].paramcount = paramcount;
    } else {
        txt_printf("Erro: syscall %d excede o limite de %d\n", cmd, MAX_SYSCALLS);
    }
}

// ==============================
// Handler de syscall (chamado pela ISR)
// ==============================
void uex_syscall_handler(void) {
    uint32_t cmd = syscall_eax;

    if (cmd >= MAX_SYSCALLS) {
        txt_printf("Syscall %d fora do intervalo permitido\n", cmd);
        return;
    }

    Syscall *sc = &Syscalls[cmd];
    if (sc->function == NULL) {
        txt_printf("Syscall %d não registrada\n", cmd);
        return;
    }

    sc->function(syscall_ebx, syscall_ecx, syscall_edx,
                 syscall_esi, syscall_edi, syscall_ebp);
}

// ==============================
// Inicialização do subsistema UEX
// ==============================
void uex_init(void) {
    register_syscall(0, sys_exit, 1);
    register_syscall(1, sys_write, 2);
    // Adicione mais syscalls aqui
}

// ==============================
// Carregador UEX (load_and_execute e uex_run)
// ==============================

static int load_and_execute(uint8_t *file_data, uint32_t file_size) {
    if (file_size < UEX_HEADER_SIZE) {
        txt_print("Arquivo muito pequeno.\n");
        return -1;
    }

    UEX_Header *header = (UEX_Header*)file_data;
    if (memcmp(header->signature, UEX_SIGNATURE, 3) != 0) {
        txt_print("Assinatura UEX inválida.\n");
        return -1;
    }

    uint32_t stack_size = header->stack_size;
    uint32_t entry_offset = header->entry_offset;

    if (entry_offset >= file_size) {
        txt_print("Offset de entrada inválido.\n");
        return -1;
    }

    uint32_t code_size = file_size - entry_offset;
    uint8_t *code = (uint8_t*)malloc(code_size);
    if (!code) {
        txt_print("Falha ao alocar código.\n");
        return -1;
    }

    memcpy(code, file_data + entry_offset, code_size);

    uint8_t *stack = (uint8_t*)malloc(stack_size);
    if (!stack) {
        txt_print("Falha ao alocar pilha.\n");
        free(code);
        return -1;
    }

    memset(stack, 0, stack_size);
    uint32_t stack_top = (uint32_t)(stack + stack_size);

    txt_printf("Executando app: code=0x%x, stack=0x%x, size=%u\n",
               (uint32_t)code, stack_top, stack_size);

    // ============================================================
    // CORREÇÃO: assembly reescrito para evitar conflitos
    // ============================================================
    uint32_t old_esp;
    // Salva o ESP atual
    __asm__ volatile (
        "mov %%esp, %0\n\t"
        : "=r" (old_esp)
        :
        : "memory"
    );

    // Troca para a pilha do app, salva old_esp, chama o código, restaura
    __asm__ volatile (
        "mov %0, %%esp\n\t"          // nova pilha
        "push %1\n\t"                // salva old_esp na pilha do app
        "call *%2\n\t"               // chama o app (ele retorna com ret)
        "pop %%esp\n\t"              // restaura old_esp do app
        : : "r" (stack_top), "r" (old_esp), "r" (code)
        : "memory"
    );

    txt_print("App retornou ao kernel.\n");
    free(stack);
    free(code);
    return 0;
}

int uex_run(const char *filename) {
    FIL file;
    FRESULT res;
    UINT br;

    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        txt_printf("Erro ao abrir %s (código %d)\n", filename, res);
        return -1;
    }

    uint32_t file_size = f_size(&file);
    uint8_t *file_data = (uint8_t*)malloc(file_size);
    if (!file_data) {
        txt_print("Erro de memória.\n");
        f_close(&file);
        return -1;
    }

    res = f_read(&file, file_data, file_size, &br);
    f_close(&file);

    if (res != FR_OK || br != file_size) {
        txt_printf("Erro ao ler arquivo (código %d)\n", res);
        free(file_data);
        return -1;
    }

    int result = load_and_execute(file_data, file_size);
    free(file_data);
    return result;
}
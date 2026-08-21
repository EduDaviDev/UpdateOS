#include "uex.h"
#include "../drivers/fatfs/ff.h"
#include "../drivers/video.h"
#include "../libs/memory.h"
#include "../libs/string.h"
#include "../memory/paging.h"
#include <stddef.h>

#define BINARY_LOAD_ADDR  0x400000   // endereço fixo (deve bater com -Ttext do app)
#define APP_STACK_SIZE    4096

/* Variáveis para syscall (definidas no assembly) */
extern uint32_t syscall_eax, syscall_ebx, syscall_ecx, syscall_edx,
                syscall_esi, syscall_edi, syscall_ebp;

/* ============================================================
   Carregador de binário puro (flat binary)
   ============================================================ */
static int load_and_execute(uint8_t *file_data, uint32_t file_size) {
    if (file_size == 0) {
        txt_print("Arquivo vazio.\n");
        return -1;
    }

    // Copia o binário para o endereço fixo
    uint8_t *load_addr = (uint8_t*)BINARY_LOAD_ADDR;
    memcpy(load_addr, file_data, file_size);

    // Aloca pilha do app (usando malloc – pilha em heap, mapeada)
    uint8_t *stack = (uint8_t*)malloc(APP_STACK_SIZE);
    if (!stack) {
        txt_print("Falha ao alocar pilha.\n");
        return -1;
    }
    memset(stack, 0, APP_STACK_SIZE);
    uint32_t stack_top = (uint32_t)(stack + APP_STACK_SIZE);

    // (Opcional) Mapeia a pilha se necessário – já está no heap mapeado.

    txt_printf("Executando binário em 0x%x, pilha em 0x%x, tamanho %u\n",
               (uint32_t)load_addr, stack_top, file_size);

    // Salva ESP atual
    uint32_t old_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r"(old_esp));

    // Troca para pilha do app e salta para o binário
    __asm__ volatile (
        "mov %0, %%esp\n"
        "push %1\n"          // salva old_esp na pilha do app
        "call *%2\n"         // chama o endereço do binário
        "pop %%esp\n"        // restaura old_esp quando retornar
        : : "r"(stack_top), "r"(old_esp), "r"(load_addr)
        : "memory"
    );

    txt_print("App retornou ao kernel.\n");
    free(stack);
    return 0;
}

/* ============================================================
   uex_run – abre o arquivo e chama o carregador
   ============================================================ */
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
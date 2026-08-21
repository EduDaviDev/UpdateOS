#include "upm.h"
#include "../memory/paging.h"
#include "../drivers/video.h"
#include "../libs/memory.h"
#include "../libs/string.h"

static process_t *process_list = NULL;
static process_t *current_process = NULL;
static uint32_t next_pid = 1;

void upm_init(void) {
    process_list = NULL;
    current_process = NULL;
    txt_print("[UPM] Inicializado.\n");
}

int upm_create_process(const char *name, uint32_t entry, uint32_t stack) {
    process_t *new_proc = (process_t*)malloc(sizeof(process_t));
    if (!new_proc) return -1;

    memset(new_proc, 0, sizeof(process_t));
    new_proc->pid = next_pid++;
    strncpy(new_proc->name, name, 31);
    new_proc->name[31] = '\0';
    new_proc->entry_point = entry;
    new_proc->stack_top = stack;
    // Por enquanto, usa o mesmo page directory do kernel
    uint32_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    new_proc->cr3 = cr3;
    new_proc->active = true;

    // Adiciona à lista
    if (process_list == NULL) {
        process_list = new_proc;
    } else {
        process_t *p = process_list;
        while (p->next) p = p->next;
        p->next = new_proc;
    }

    if (current_process == NULL) {
        current_process = new_proc;
    }

    txt_printf("[UPM] Processo '%s' criado (PID %d)\n", name, new_proc->pid);
    return new_proc->pid;
}

void upm_terminate_process(uint32_t pid) {
    process_t *prev = NULL;
    process_t *p = process_list;
    while (p) {
        if (p->pid == pid) {
            if (prev) prev->next = p->next;
            else process_list = p->next;
            if (current_process == p) current_process = NULL;
            free(p);
            txt_printf("[UPM] Processo %d terminado.\n", pid);
            return;
        }
        prev = p;
        p = p->next;
    }
    txt_printf("[UPM] Processo %d não encontrado.\n", pid);
}

process_t* upm_get_current(void) {
    return current_process;
}

void upm_yield(void) {
    // Por enquanto, apenas imprime (futuramente alternará processos)
    txt_print("[UPM] Yield chamado.\n");
}
#ifndef UPM_H
#define UPM_H

#include <stdint.h>
#include <stdbool.h>

/* Estrutura de um processo */
typedef struct process {
    uint32_t pid;
    char name[32];
    uint32_t entry_point;   // endereço virtual onde o código foi carregado
    uint32_t stack_top;
    uint32_t cr3;           // page directory (se cada processo tiver o seu)
    bool active;
    struct process *next;
} process_t;

/* Funções */
void upm_init(void);
int  upm_create_process(const char *name, uint32_t entry, uint32_t stack);
void upm_terminate_process(uint32_t pid);
process_t* upm_get_current(void);
void upm_yield(void);   // para futura multitarefa

#endif /* UPM_H */
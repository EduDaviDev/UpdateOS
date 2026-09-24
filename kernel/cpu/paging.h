#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

// Estrutura de uma entrada de diretório/tabela de páginas
typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t wt         : 1;
    uint32_t cd         : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t ps         : 1;
    uint32_t global     : 1;
    uint32_t avail      : 3;
    uint32_t frame      : 20;
} __attribute__((packed)) page_entry_t;

// -------- Rotinas em assembly (paging.asm) --------
extern void     paging_init_early(void);
extern void     paging_load_directory(uint32_t *dir);
extern void     paging_enable(void);
extern void     paging_disable(void);
extern void     paging_invalidate_tlb(uint32_t virt);
extern uint32_t paging_get_fault_address(void);
extern uint32_t paging_read_cr0(void);
extern uint32_t paging_read_cr3(void);

// Diretório/Tabela estáticos definidos em paging.asm
extern page_entry_t boot_page_directory[1024];
extern page_entry_t boot_page_table0[1024];

// -------- API de alto nível em C (paging.c) --------
void paging_init(void);
void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void paging_unmap_page(uint32_t virt);

#endif
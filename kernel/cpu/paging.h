#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* Flags de página (bit 0..2), usadas em paging_map_page / paging_map_page_in */
#define PG_PRESENT 0x01
#define PG_WRITE   0x02
#define PG_USER    0x04

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

/* Cria um novo address space, copiando as entradas de kernel
 * do diretório atual. Retorna o endereço físico do PD novo. */
uint32_t paging_create_address_space(void);

/* Troca o CR3 pro PD indicado (endereço físico) */
void     paging_switch_directory(uint32_t pd_phys);

/* Retorna o PD do kernel (o boot_page_directory) */
uint32_t paging_kernel_directory(void);

/* Libera todos os frames mapeados em modo user do PD e depois o próprio PD */
void     paging_destroy_directory(uint32_t pd_phys);

/* Mapeia uma página num PD arbitrário (endereço físico do PD). */
void paging_map_page_in(uint32_t pd_phys, uint32_t virt,
                        uint32_t phys, uint32_t flags);

#endif
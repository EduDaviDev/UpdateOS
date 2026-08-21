#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>

/* Estruturas das tabelas de página */
typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t unused     : 7;
    uint32_t frame      : 20;
} __attribute__((packed)) page_entry_t;

typedef struct {
    page_entry_t entries[1024];
} page_table_t;

typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t ignored    : 7;
    uint32_t frame      : 20;
} __attribute__((packed)) dir_entry_t;

typedef struct {
    dir_entry_t entries[1024];
} page_directory_t;

/* Flags comuns */
#define PAGE_PRESENT       1
#define PAGE_RW            2
#define PAGE_USER          4
#define PAGE_WRITE_THROUGH 8
#define PAGE_CACHE_DISABLE 16
#define PAGE_ACCESSED      32
#define PAGE_DIRTY         64

/* Funções */
void init_paging(void);
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void map_pages(uint32_t virt, uint32_t phys, uint32_t count, uint32_t flags);
void unmap_page(uint32_t virt);
void unmap_pages(uint32_t virt, uint32_t count);
uint32_t get_physical_address(uint32_t virt);
void reload_cr3(void);

/* Alocador de frames (público) */
uint32_t alloc_frame(void);
void free_frame(uint32_t frame);

#endif
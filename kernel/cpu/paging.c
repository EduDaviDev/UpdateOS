#include "paging.h"
#include "../libs/string.h"

/* Pool de page tables estático, em .bss (identity-mapped, dentro dos 4 MiB) */
#define MAX_PAGE_TABLES 32   /* 32 × 4 MiB = 128 MiB mapeáveis */
static page_entry_t page_table_pool[MAX_PAGE_TABLES][1024] __attribute__((aligned(4096)));
static uint32_t next_free_pt = 0;

static page_entry_t *alloc_page_table(void) {
    if (next_free_pt >= MAX_PAGE_TABLES) return NULL;
    page_entry_t *pt = page_table_pool[next_free_pt++];
    memset(pt, 0, sizeof(page_entry_t) * 1024);
    return pt;
}

/* NÃO zera o boot_page_directory! Ele já foi montado em paging_init_early. */
void paging_init(void) {
    paging_load_directory((uint32_t *)boot_page_directory);
    paging_enable();
}

void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    page_entry_t *pde = &boot_page_directory[pd_index];

    if (!pde->present) {
        page_entry_t *new_pt = alloc_page_table();
        if (!new_pt) return;
        pde->frame   = ((uint32_t)new_pt) >> 12;
        pde->present = 1;
        pde->rw      = 1;
        pde->user    = 0;
    }

    page_entry_t *pt = (page_entry_t *)(pde->frame << 12);
    pt[pt_index].frame   = phys >> 12;
    pt[pt_index].present = (flags & 1) ? 1 : 0;
    pt[pt_index].rw      = (flags & 2) ? 1 : 0;
    pt[pt_index].user    = (flags & 4) ? 1 : 0;

    paging_invalidate_tlb(virt);
}

void paging_unmap_page(uint32_t virt) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    page_entry_t *pde = &boot_page_directory[pd_index];
    if (!pde->present) return;

    page_entry_t *pt = (page_entry_t *)(pde->frame << 12);
    pt[pt_index].present = 0;

    paging_invalidate_tlb(virt);
}
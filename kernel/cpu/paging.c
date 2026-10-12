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

/* Fim do identity-map inicial.
 * Precisa cobrir: kernel (~1.4 MiB) + heap (4..20 MiB) + folga.
 * 32 MiB dá bastante margem e é barato: só 8 page tables. */
#define IDENTITY_MAP_END 0x02000000u   /* 32 MiB */

void paging_init(void) {
    /* Os primeiros 4 MiB já vêm mapeados pelo boot_page_table0 (asm).
     * Mapeia identity da faixa [4 MiB, 32 MiB) usando o pool de PEs. */
    for (uint32_t addr = 0x00400000u; addr < IDENTITY_MAP_END; addr += 0x1000u) {
        paging_map_page(addr, addr, 0x3);   /* present | rw */
    }
}

/* ---- O resto do arquivo fica IGUAL ---- */

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

/* ============================================================
 *  Address space por processo
 * ============================================================ */

uint32_t paging_create_address_space(void) {
    /* 1. Aloca um frame físico pro novo PD */
    void *pd_phys_v = pmm_alloc_frame();
    if (!pd_phys_v) return 0;

    uint32_t pd_phys = (uint32_t)(uintptr_t)pd_phys_v;
    page_entry_t *new_pd = (page_entry_t *)pd_phys;   /* identity-mapped */

    /* 2. Zera tudo */
    memset(new_pd, 0, 4096);

    /* 3. Copia as entradas de KERNEL.
     *    Seu kernel vive nos primeiros 4 MiB identity-mapped
     *    (boot_page_directory[0] aponta pra boot_page_table0).
     *    Também copia as entradas do pool de page tables que
     *    mapeiam até 32 MiB.
     *
     *    A regra: tudo que é identity-map do kernel (baixo)
     *    vai pro novo PD; tudo que é user (>= 0x08048000, na
     *    verdade a partir de 0x08000000) fica vazio.
     *
     *    Simples: copia TODAS as entradas que já existem no
     *    diretório atual. O loader vai sobrescrever as que
     *    pertencem ao user. */
    /* Copia só as PDEs de kernel (identity map do kernel).
     * IDENTITY_MAP_END = 0x02000000 → 8 PDEs × 4 MiB = 32 MiB.
     * As demais ficam zeradas — o ELF loader preenche. */
    #define KERNEL_PDE_COUNT  (IDENTITY_MAP_END >> 22)   /* = 8 */
    for (uint32_t i = 0; i < KERNEL_PDE_COUNT; i++)
        new_pd[i] = boot_page_directory[i];

    return pd_phys;
}

void paging_switch_directory(uint32_t pd_phys) {
    paging_load_directory((uint32_t *)pd_phys);
}

uint32_t paging_kernel_directory(void) {
    return (uint32_t)(uintptr_t)boot_page_directory;
}

void paging_destroy_directory(uint32_t pd_phys) {
    if (!pd_phys || pd_phys == paging_kernel_directory()) return;

    page_entry_t *pd = (page_entry_t *)pd_phys;

    /* Itera todas as PD entries. Aquelas que apontam pra
     * page tables user (i.e., fora do pool de kernel e com
     * user=1) precisam ser destruídas. */
    for (uint32_t i = KERNEL_PDE_COUNT; i < 1024; i++) {
        if (!pd[i].present) continue;
        if (!pd[i].user) continue;    /* entrada de kernel: não mexe */

        uint32_t pt_phys = pd[i].frame << 12;
        page_entry_t *pt = (page_entry_t *)pt_phys;

        /* Libera cada frame mapeado na PT */
        for (int j = 0; j < 1024; j++) {
            if (!pt[j].present) continue;
            uint32_t frame_phys = pt[j].frame << 12;
            pmm_free_frame((void *)(uintptr_t)frame_phys);
        }

        /* Libera a própria page table */
        pmm_free_frame((void *)(uintptr_t)pt_phys);
    }

    /* Libera o PD */
    pmm_free_frame((void *)(uintptr_t)pd_phys);
}

/* Mapeia uma página num PD arbitrário. Aloca PT do PMM (não do pool). */
void paging_map_page_in(uint32_t pd_phys, uint32_t virt,
                        uint32_t phys, uint32_t flags)
{
    page_entry_t *pd = (page_entry_t *)pd_phys;
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    page_entry_t *pde = &pd[pd_idx];

    if (!pde->present) {
        void *pt_phys = pmm_alloc_frame();
        if (!pt_phys) return;
        memset(pt_phys, 0, 4096);

        pde->frame   = ((uint32_t)pt_phys) >> 12;
        pde->present = 1;
        pde->rw      = 1;
        pde->user    = (flags & 4) ? 1 : 0;
    } else {
        if (flags & 4) pde->user = 1;
    }

    page_entry_t *pt = (page_entry_t *)(pde->frame << 12);
    pt[pt_idx].frame   = phys >> 12;
    pt[pt_idx].present = (flags & 1) ? 1 : 0;
    pt[pt_idx].rw      = (flags & 2) ? 1 : 0;
    pt[pt_idx].user    = (flags & 4) ? 1 : 0;

    paging_invalidate_tlb(virt);
}
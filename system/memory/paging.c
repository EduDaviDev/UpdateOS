#include "paging.h"
#include "../libs/memory.h"
#include "../libs/string.h"
#include "../drivers/video.h"
#include <stdint.h>

/* ============================================================
   Alocador de frames (bitmap) – agora suporta até 64 MiB
   ============================================================ */
#define MEMORY_SIZE_MB  64
#define FRAME_COUNT     (MEMORY_SIZE_MB * 1024 * 1024 / 4096)  // 64MB -> 16384 frames
static uint32_t frame_bitmap[FRAME_COUNT / 32] = {0};

void set_frame(uint32_t frame) {
    if (frame < FRAME_COUNT)
        frame_bitmap[frame / 32] |= (1 << (frame % 32));
}

void clear_frame(uint32_t frame) {
    if (frame < FRAME_COUNT)
        frame_bitmap[frame / 32] &= ~(1 << (frame % 32));
}

uint32_t alloc_frame(void) {
    for (uint32_t i = 0; i < FRAME_COUNT / 32; i++) {
        if (frame_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(frame_bitmap[i] & (1 << j))) {
                    uint32_t frame = i * 32 + j;
                    set_frame(frame);
                    return frame;
                }
            }
        }
    }
    return 0xFFFFFFFF;  // sem frames livres
}

void free_frame(uint32_t frame) {
    if (frame < FRAME_COUNT)
        clear_frame(frame);
}

/* ============================================================
   Gerenciamento da paginação
   ============================================================ */
static page_directory_t *page_dir = NULL;

/* Função interna para mapear uma única página */
static void map_page_internal(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    dir_entry_t *dir = &page_dir->entries[pd_index];

    if (!dir->present) {
        uint32_t pt_frame = alloc_frame();
        if (pt_frame == 0xFFFFFFFF) {
            txt_print("[paging] ERRO: sem memória para page table!\n");
            return;
        }
        page_table_t *pt = (page_table_t*)(pt_frame * 4096);
        memset(pt, 0, sizeof(page_table_t));
        dir->present = 1;
        dir->rw = 1;
        dir->user = 1;
        dir->frame = pt_frame;
    }

    page_table_t *pt = (page_table_t*)(dir->frame * 4096);
    pt->entries[pt_index].present = 1;
    pt->entries[pt_index].rw = (flags & PAGE_RW) ? 1 : 0;
    pt->entries[pt_index].user = (flags & PAGE_USER) ? 1 : 0;
    pt->entries[pt_index].frame = phys >> 12;
}

/* ============================================================
   Funções públicas
   ============================================================ */

void init_paging(void) {
    // Verifica se o page directory já está em 0x1000 (do mboot)
    uint32_t *check = (uint32_t*)0x1000;
    if (check[0] == 0x2003) {
        page_dir = (page_directory_t*)0x1000;
        txt_print("[paging] Reutilizando page directory do mboot (0x1000)\n");
    } else {
        // Cria novo page directory (não deve acontecer, pois mboot já criou)
        uint32_t pd_frame = alloc_frame();
        if (pd_frame == 0xFFFFFFFF) {
            txt_print("[paging] ERRO: sem memória para page directory!\n");
            return;
        }
        page_dir = (page_directory_t*)(pd_frame * 4096);
        memset(page_dir, 0, sizeof(page_directory_t));

        // Mapeia 4MB identity (0x00000000 - 0x3FFFFF)
        uint32_t pt_frame = alloc_frame();
        if (pt_frame == 0xFFFFFFFF) {
            txt_print("[paging] ERRO: sem memória para page table!\n");
            return;
        }
        page_table_t *pt = (page_table_t*)(pt_frame * 4096);
        memset(pt, 0, sizeof(page_table_t));
        for (uint32_t i = 0; i < 1024; i++) {
            pt->entries[i].present = 1;
            pt->entries[i].rw = 1;
            pt->entries[i].user = 1;
            pt->entries[i].frame = i;
        }
        page_dir->entries[0].present = 1;
        page_dir->entries[0].rw = 1;
        page_dir->entries[0].user = 1;
        page_dir->entries[0].frame = pt_frame;

        __asm__ volatile ("mov %0, %%cr3" : : "r"((uint32_t)page_dir));
    }

    // Ativa paginação (se já não estiver)
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    if (!(cr0 & 0x80000000)) {
        __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0 | 0x80000000));
        txt_print("[paging] Paginação ativada.\n");
    }
    txt_print("[paging] Inicializado.\n");
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    if (!page_dir) { txt_print("[paging] ERRO: paginação não inicializada!\n"); return; }
    map_page_internal(virt, phys, flags);
    reload_cr3();  // flush TLB
}

void map_pages(uint32_t virt, uint32_t phys, uint32_t count, uint32_t flags) {
    if (!page_dir) { txt_print("[paging] ERRO: paginação não inicializada!\n"); return; }
    for (uint32_t i = 0; i < count; i++) {
        map_page_internal(virt + i * 4096, phys + i * 4096, flags);
    }
    reload_cr3();
}

void unmap_page(uint32_t virt) {
    if (!page_dir) return;
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    dir_entry_t *dir = &page_dir->entries[pd_index];
    if (!dir->present) return;
    page_table_t *pt = (page_table_t*)(dir->frame * 4096);
    if (!pt->entries[pt_index].present) return;
    pt->entries[pt_index].present = 0;
    reload_cr3();
}

void unmap_pages(uint32_t virt, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        unmap_page(virt + i * 4096);
    }
}

uint32_t get_physical_address(uint32_t virt) {
    if (!page_dir) return 0;
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    dir_entry_t *dir = &page_dir->entries[pd_index];
    if (!dir->present) return 0;
    page_table_t *pt = (page_table_t*)(dir->frame * 4096);
    if (!pt->entries[pt_index].present) return 0;
    return (pt->entries[pt_index].frame << 12) | (virt & 0xFFF);
}
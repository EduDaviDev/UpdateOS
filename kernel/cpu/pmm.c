#include "pmm.h"
#include "../libs/string.h"

/* Suporta até 4 GiB de RAM → 1 bit por frame de 4 KiB → 128 KiB de bitmap */
#define PMM_MAX_FRAMES  (1024u * 1024u)
#define PMM_BITMAP_SIZE (PMM_MAX_FRAMES / 8u)

static uint8_t  bitmap[PMM_BITMAP_SIZE];
static uint32_t total_frames = 0;
static uint32_t used_frames  = 0;

/* Símbolos do linker.ld (adicione — veja seção 4) */
extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

/* ---------- bitmap helpers ---------- */
static inline void bm_set  (uint32_t i) { bitmap[i >> 3] |=  (1u << (i & 7)); }
static inline void bm_clear(uint32_t i) { bitmap[i >> 3] &= ~(1u << (i & 7)); }
static inline int  bm_test (uint32_t i) { return (bitmap[i >> 3] >> (i & 7)) & 1; }

static void mark_used(uint64_t base, uint64_t len) {
    uint64_t start = base / PMM_FRAME_SIZE;
    uint64_t end   = (base + len + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;
    for (uint64_t i = start; i < end && i < total_frames; i++) {
        if (!bm_test((uint32_t)i)) { bm_set((uint32_t)i); used_frames++; }
    }
}

static void mark_free(uint64_t base, uint64_t len) {
    /* Alinha para dentro: não libera parcialmente um frame já usado */
    uint64_t start = (base + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;
    uint64_t end   = (base + len) / PMM_FRAME_SIZE;
    for (uint64_t i = start; i < end && i < total_frames; i++) {
        if (bm_test((uint32_t)i)) { bm_clear((uint32_t)i); used_frames--; }
    }
}

/* ---------- init ---------- */
void pmm_init(struct multiboot_info *mb_info) {
    /* 1) começa tudo USADO (pessimista) */
    memset(bitmap, 0xFF, PMM_BITMAP_SIZE);
    total_frames = PMM_MAX_FRAMES;
    used_frames  = PMM_MAX_FRAMES;

    if (!mb_info) return;

    /* 2) primeira passada: descobre o topo da RAM */
    uint64_t max_addr = 0;
    uint8_t *p   = (uint8_t *)mb_info->tags;
    uint8_t *end = (uint8_t *)mb_info + mb_info->total_size;

    while (p < end) {
        struct multiboot_tag *tag = (struct multiboot_tag *)p;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct multiboot_tag_mmap *mm = (struct multiboot_tag_mmap *)p;
            uint8_t *e    = (uint8_t *)mm->entries;
            uint8_t *eend = p + mm->size;
            while (e < eend) {
				struct multiboot_mmap_entry *ent = (struct multiboot_mmap_entry *)e;
				if (ent->type == 1) {                    /* ← só RAM utilizável */
				    uint64_t top = ent->addr + ent->len;
				    if (top > max_addr) max_addr = top;
				}
				e += mm->entry_size;
            }
        }
        p += (tag->size + 7) & ~7u;
    }

    if (max_addr == 0) return;
    uint64_t cap = (uint64_t)PMM_MAX_FRAMES * PMM_FRAME_SIZE;
    if (max_addr > cap) max_addr = cap;

    total_frames = (uint32_t)(max_addr / PMM_FRAME_SIZE);
    used_frames  = total_frames;

    /* 3) segunda passada: libera o que o GRUB disse ser type=1 */
    p = (uint8_t *)mb_info->tags;
    while (p < end) {
        struct multiboot_tag *tag = (struct multiboot_tag *)p;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct multiboot_tag_mmap *mm = (struct multiboot_tag_mmap *)p;
            uint8_t *e    = (uint8_t *)mm->entries;
            uint8_t *eend = p + mm->size;
            while (e < eend) {
                struct multiboot_mmap_entry *ent = (struct multiboot_mmap_entry *)e;
                if (ent->type == 1) mark_free(ent->addr, ent->len);
                e += mm->entry_size;
            }
        }
        p += (tag->size + 7) & ~7u;
    }

    /* 4) reserva tudo que é "território do kernel/boot" */
    mark_used(0x00000000, 0x00400000);   /* primeiros 4 MiB: BIOS, VGA, kernel, page tables */

    /* 5) reserva explicitamente o intervalo do kernel (por segurança) */
    mark_used((uint64_t)(uintptr_t)&_kernel_start,
              (uint64_t)((uintptr_t)&_kernel_end - (uintptr_t)&_kernel_start));
}

/* ---------- alloc/free ---------- */
void *pmm_alloc_frame(void) {
    for (uint32_t i = 0; i < total_frames; i++) {
        if (!bm_test(i)) {
            bm_set(i);
            used_frames++;
            return (void *)(uintptr_t)(i * PMM_FRAME_SIZE);
        }
    }
    return NULL;
}

void pmm_free_frame(void *frame) {
    uint32_t i = (uint32_t)(uintptr_t)frame / PMM_FRAME_SIZE;
    if (i < total_frames && bm_test(i)) {
        bm_clear(i);
        used_frames--;
    }
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames(void)  { return used_frames;  }
uint32_t pmm_free_frames(void)  { return total_frames - used_frames; }
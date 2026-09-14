#include "paging.h"
#include "../drivers/vga.h"
#include "../libs/memory.h"
#include "../libs/string.h"
#include "isr.h"

/* =====================================================================
 * Estruturas estáticas de bootstrap
 *
 * kernel_pd       -> Page Directory (1024 PDEs, 4 KiB)
 * kernel_pt[N][]  -> N Page Tables (cada cobre 4 MiB)
 *
 * Tudo é alinhado a 4 KiB e fica no BSS. NÃO colocar nada disso na stack!
 * ===================================================================== */
static pde_t kernel_pd[PAGE_DIR_ENTRIES]              __attribute__((aligned(PAGE_SIZE)));
static pte_t kernel_pt[PAGING_MAX_TABLES][PAGE_TABLE_ENTRIES]
                                                       __attribute__((aligned(PAGE_SIZE)));

/* =====================================================================
 * Bitmap allocator de frames físicos
 * ===================================================================== */
#define BITMAP_WORDS (PAGING_MAX_FRAMES / 32)
static uint32_t frame_bitmap[BITMAP_WORDS];
static uint32_t total_frames = 0;
static uint32_t used_frames  = 0;

static inline void bm_set  (uint32_t b) { frame_bitmap[b >> 5] |=  (1u << (b & 31)); }
static inline void bm_clear(uint32_t b) { frame_bitmap[b >> 5] &= ~(1u << (b & 31)); }
static inline int  bm_test (uint32_t b) { return (frame_bitmap[b >> 5] >> (b & 31)) & 1u; }

/* =====================================================================
 * Page Fault callback
 * ===================================================================== */
static void (*fault_cb)(uint32_t) = NULL;

/* =====================================================================
 * Helpers de CR/TLB
 * ===================================================================== */
static inline phys_addr_t read_cr3(void) {
    phys_addr_t v; __asm__ volatile("mov %%cr3, %0" : "=r"(v)); return v;
}
static inline void write_cr3(phys_addr_t v) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(v) : "memory");
}
static inline void invlpg(virt_addr_t v) {
    __asm__ volatile("invlpg (%0)" :: "r"(v) : "memory");
}
static inline pde_t *active_pd(void) {
    /* Identidade: phys == virt para as nossas estruturas. */
    return (pde_t *)read_cr3();
}

/* =====================================================================
 * PMM público
 * ===================================================================== */
phys_addr_t pmm_alloc_frame(void) {
    for (uint32_t i = 0; i < total_frames; i++) {
        if (!bm_test(i)) {
            bm_set(i);
            used_frames++;
            phys_addr_t addr = i * PAGE_SIZE;
            memset((void *)addr, 0, PAGE_SIZE);   /* zerar é importante! */
            return addr;
        }
    }
    return 0;
}

void pmm_free_frame(phys_addr_t frame) {
    uint32_t bit = frame >> 12;
    if (bit < total_frames && bm_test(bit)) {
        bm_clear(bit);
        used_frames--;
    }
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames (void) { return used_frames;  }
uint32_t pmm_free_frames (void) { return total_frames - used_frames; }
uint32_t pmm_total_memory(void) { return total_frames * PAGE_SIZE;    }
uint32_t pmm_free_memory (void) { return (total_frames - used_frames) * PAGE_SIZE; }

/* =====================================================================
 * Símbolos exportados pelo linker.ld
 * ===================================================================== */
extern uint8_t kernel_start[];
extern uint8_t kernel_end[];

/* =====================================================================
 * Inicialização
 * ===================================================================== */
void paging_init(uint32_t mem_size) {
    /* ---------- Sanidade ---------- */
    if (mem_size == 0)
        mem_size = (uint32_t)PAGING_MAX_MB * 1024u * 1024u;

    const uint32_t max_bytes = (uint32_t)PAGING_MAX_MB * 1024u * 1024u;
    if (mem_size > max_bytes) mem_size = max_bytes;

    /* Mínimo de 4 MiB (1 PT). */
    if (mem_size < 4u * 1024u * 1024u) mem_size = 4u * 1024u * 1024u;

    /* ---------- Limpa todas as estruturas estáticas ---------- */
    memset(kernel_pd,    0, sizeof(kernel_pd));
    memset(kernel_pt,    0, sizeof(kernel_pt));
    memset(frame_bitmap, 0, sizeof(frame_bitmap));

    total_frames = mem_size / PAGE_SIZE;
    used_frames  = 0;

    /* Garante que total_frames não estoura o bitmap. */
    if (total_frames > PAGING_MAX_FRAMES)
        total_frames = PAGING_MAX_FRAMES;

    /* =================================================================
     * Reservas no bitmap
     * =================================================================
     *
     * Ordem importa: primeiro o que é intocável, depois o resto.
     *
     * Precisamos marcar:
     *   (a) memória baixa  [0 .. 1 MiB)
     *   (b) imagem do kernel [kernel_start .. kernel_end)
     *
     * Note que kernel_pd, kernel_pt e frame_bitmap vivem no BSS, ou seja,
     * estão DENTRO de [kernel_start .. kernel_end). Não precisamos
     * marcá-los individualmente.
     * ================================================================= */

    /* (a) Memória baixa: IVT, BDA, VGA, EBDA, BIOS, etc. */
    for (uint32_t i = 0; i < 0x100000u / PAGE_SIZE; i++) {
        if (!bm_test(i)) { bm_set(i); used_frames++; }
    }

    /* (b) Imagem do kernel (arredondando para páginas) */
    uint32_t k_begin = (uint32_t)kernel_start & PAGE_MASK;
    uint32_t k_end   = ((uint32_t)kernel_end + PAGE_SIZE - 1) & PAGE_MASK;

    /* Segurança: se por algum motivo kernel_end vier antes, aborta. */
    if (k_end <= k_begin) {
        /* Não deveria acontecer; mas evita loop infinito. */
        k_end = k_begin + PAGE_SIZE;
    }

    for (uint32_t a = k_begin; a < k_end; a += PAGE_SIZE) {
        uint32_t b = a >> 12;
        if (b >= total_frames) break;         /* fora da RAM detectada */
        if (!bm_test(b)) { bm_set(b); used_frames++; }
    }

    /* Opcional: garante que o bitmap em si não seja sobrescrito por
     * pmm_alloc_frame em um cenário de "RAM muito pequena". Já está
     * coberto por (b) se frame_bitmap estiver no BSS — o que é o caso. */

    /* =================================================================
     * Identity mapping da RAM
     * ================================================================= */
    uint32_t num_tables =
        (mem_size + (4u * 1024u * 1024u) - 1) / (4u * 1024u * 1024u);
    if (num_tables > PAGING_MAX_TABLES)
        num_tables = PAGING_MAX_TABLES;

    for (uint32_t t = 0; t < num_tables; t++) {
        pte_t *pt = kernel_pt[t];
        uint32_t base = t * PAGE_TABLE_ENTRIES * PAGE_SIZE;

        for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            uint32_t phys = base + i * PAGE_SIZE;
            pt[i] = phys | PAGE_PRESENT | PAGE_RW;
        }

        kernel_pd[t] = ((uint32_t)pt) | PAGE_PRESENT | PAGE_RW;
    }

    /* =================================================================
     * Ativa paginação
     * ================================================================= */
    write_cr3((phys_addr_t)kernel_pd);
    paging_enable();
}

/* =====================================================================
 * Controle de CR0 / CR3
 * ===================================================================== */
void paging_load_directory(phys_addr_t pd_phys) { write_cr3(pd_phys); }
phys_addr_t paging_get_directory(void)          { return read_cr3();  }

void paging_enable(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;                 /* PG */
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

void paging_disable(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000u;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

/* =====================================================================
 * Mapeamento / desmapeamento de páginas
 * ===================================================================== */

/* Retorna o PT que cobre `virt`, alocando um se necessário (e se permitido). */
static pte_t *get_or_create_pt(virt_addr_t virt, uint32_t flags, int create) {
    pde_t *pd = active_pd();
    uint32_t pd_index = virt >> 22;
    pde_t pde = pd[pd_index];

    if (!(pde & PAGE_PRESENT)) {
        if (!create) return NULL;
        phys_addr_t pt_phys = pmm_alloc_frame();
        if (!pt_phys) return NULL;
        pd[pd_index] = pt_phys
                     | PAGE_PRESENT | PAGE_RW
                     | (flags & PAGE_USER);
        invlpg(virt);
        return (pte_t *)pt_phys;             /* identidade */
    }

    if (pde & PAGE_4MB) return NULL;         /* não suportamos split de 4MB */
    return (pte_t *)(pde & PAGE_MASK);
}

int paging_map_page(virt_addr_t virt, phys_addr_t phys, uint32_t flags) {
    pte_t *pt = get_or_create_pt(virt, flags, 1);
    if (!pt) return -1;

    uint32_t idx = (virt >> 12) & 0x3FF;
    pt[idx] = (phys & PAGE_MASK) | (flags & 0xFFF) | PAGE_PRESENT;
    invlpg(virt);
    return 0;
}

int paging_unmap_page(virt_addr_t virt) {
    pde_t *pd = active_pd();
    uint32_t pd_index = virt >> 22;
    pde_t pde = pd[pd_index];
    if (!(pde & PAGE_PRESENT) || (pde & PAGE_4MB)) return -1;

    pte_t *pt = (pte_t *)(pde & PAGE_MASK);
    uint32_t idx = (virt >> 12) & 0x3FF;
    if (!(pt[idx] & PAGE_PRESENT)) return -1;

    pt[idx] = 0;
    invlpg(virt);
    return 0;
}

phys_addr_t paging_get_physical(virt_addr_t virt) {
    pde_t *pd = active_pd();
    uint32_t pd_index = virt >> 22;
    pde_t pde = pd[pd_index];
    if (!(pde & PAGE_PRESENT)) return 0;

    if (pde & PAGE_4MB) {
        phys_addr_t base = pde & 0xFFC00000u;
        return base + (virt & 0x3FFFFFu);
    }

    pte_t *pt = (pte_t *)(pde & PAGE_MASK);
    pte_t pte = pt[(virt >> 12) & 0x3FF];
    if (!(pte & PAGE_PRESENT)) return 0;
    return (pte & PAGE_MASK) + (virt & PAGE_OFFSET_MASK);
}

int paging_is_mapped(virt_addr_t virt) {
    pde_t *pd = active_pd();
    pde_t pde = pd[virt >> 22];
    if (!(pde & PAGE_PRESENT)) return 0;
    if (pde & PAGE_4MB) return 1;
    pte_t *pt = (pte_t *)(pde & PAGE_MASK);
    return (pt[(virt >> 12) & 0x3FF] & PAGE_PRESENT) ? 1 : 0;
}

int paging_identity_map_range(phys_addr_t start, uint32_t size, uint32_t flags) {
    /* Alinha para baixo / para cima */
    phys_addr_t begin = start & PAGE_MASK;
    phys_addr_t end   = (start + size + PAGE_SIZE - 1) & PAGE_MASK;

    for (phys_addr_t a = begin; a < end; a += PAGE_SIZE) {
        if (paging_is_mapped(a)) continue;
        if (paging_map_page(a, a, flags) != 0) return -1;
    }
    return 0;
}

/* =====================================================================
 * Page Fault handler (exceção 14)
 * ===================================================================== */
void paging_set_fault_callback(void (*handler)(uint32_t)) {
    fault_cb = handler;
}

static inline uint32_t read_cr2(void) {
    uint32_t v;
    __asm__ volatile("mov %%cr2, %0" : "=r"(v));
    return v;
}

/* Traduz o error code do Page Fault (Intel SDM Vol 3, 4.7) */
static const char *pf_reason(uint32_t e) { return (e & 1) ? "protection" : "not-present"; }
static const char *pf_access(uint32_t e) { return (e & 2) ? "write"      : "read";       }
static const char *pf_mode  (uint32_t e) { return (e & 4) ? "user"       : "supervisor"; }

void paging_fault_handler(uint32_t error_code, registers_t *r) {
    uint32_t fault_addr = read_cr2();
    uint32_t eip = r ? r->eip : 0;
    uint32_t cs  = r ? r->cs  : 0;

    if (!vga_printf) {
        for (;;) __asm__ volatile("cli; hlt");
    }

    vga_printf("\n=== PAGE FAULT ===\n");
    vga_printf("  fault addr (CR2) = 0x%08x\n", fault_addr);
    vga_printf("  eip              = 0x%08x  cs=0x%04x\n", eip, cs);
    vga_printf("  error code       = 0x%x\n", error_code);
    vga_printf("    reason : %s\n", pf_reason(error_code));
    vga_printf("    access : %s\n", pf_access(error_code));
    vga_printf("    mode   : %s\n", pf_mode(error_code));
    if (error_code & 8)  vga_printf("    ** reserved bit set **\n");
    if (error_code & 16) vga_printf("    ** instruction fetch **\n");

    /* Informação útil de contexto */
    vga_printf("  pd @ 0x%08x\n", paging_get_directory());
    vga_printf("  page mapped? %s\n", paging_is_mapped(fault_addr) ? "SIM" : "NAO");

    if (r) {
        vga_printf("  eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
                   r->eax, r->ebx, r->ecx, r->edx);
        vga_printf("  esi=%08x edi=%08x ebp=%08x esp=%08x\n",
                   r->esi, r->edi, r->ebp, r->esp_dummy);
        vga_printf("  ds=%04x es=%04x fs=%04x gs=%04x eflags=%08x\n",
                   r->ds, r->es, r->fs, r->gs, r->eflags);
    }

    vga_printf("[HALT]\n");

    /* Sem recuperação: congela. */
    for (;;) __asm__ volatile("cli; hlt");
}
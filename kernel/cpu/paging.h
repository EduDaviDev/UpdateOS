#ifndef CPU_PAGING_H
#define CPU_PAGING_H

#include <stdint.h>
#include <stddef.h>
#include "isr.h" 

/* ---------------- Constants ---------------- */
#define PAGE_SIZE             4096
#define PAGE_TABLE_ENTRIES    1024
#define PAGE_DIR_ENTRIES      1024
#define PAGE_MASK             0xFFFFF000u
#define PAGE_OFFSET_MASK      0x00000FFFu

/* ---------------- Entry flags ---------------- */
#define PAGE_PRESENT   0x0001
#define PAGE_RW        0x0002
#define PAGE_USER      0x0004
#define PAGE_PWT       0x0008
#define PAGE_PCD       0x0010
#define PAGE_ACCESSED  0x0020
#define PAGE_DIRTY     0x0040
#define PAGE_4MB       0x0080   /* PDE: page size (requires PSE) */
#define PAGE_GLOBAL    0x0100

#define PAGE_KERNEL    (PAGE_PRESENT | PAGE_RW)
#define PAGE_USER_RW   (PAGE_PRESENT | PAGE_RW | PAGE_USER)

/* ---------------- Types ---------------- */
typedef uint32_t pde_t;
typedef uint32_t pte_t;
typedef uint32_t phys_addr_t;
typedef uint32_t virt_addr_t;

/* ---------------- Config ---------------- */
/* Ajuste se sua VM tiver mais RAM. Afeta o tamanho das tabelas estáticas. */
#ifndef PAGING_MAX_MB
#define PAGING_MAX_MB       512
#endif
#define PAGING_MAX_FRAMES   (PAGING_MAX_MB * 256)        /* 1 frame = 4 KiB */
#define PAGING_MAX_TABLES   (PAGING_MAX_MB / 4)          /* 1 table = 4 MiB */

/* ---------------- Public API ---------------- */

/* Inicializa e habilita paginação.
 * mem_size = bytes de RAM (0 = usa PAGING_MAX_MB).
 * Mapeia por identidade toda a RAM até mem_size. */
void paging_init(uint32_t mem_size);

void        paging_load_directory(phys_addr_t pd_phys);
void        paging_enable(void);
void        paging_disable(void);
phys_addr_t paging_get_directory(void);

/* Mapeia uma página de 4 KiB (aloca PT se necessário). 0 = ok, -1 = falha. */
int paging_map_page(virt_addr_t virt, phys_addr_t phys, uint32_t flags);

/* Desmapeia uma página de 4 KiB. 0 = ok, -1 = não mapeada. */
int paging_unmap_page(virt_addr_t virt);

/* Traduz virtual -> físico. 0 se não mapeada. */
phys_addr_t paging_get_physical(virt_addr_t virt);

/* 1 se a página está mapeada, 0 caso contrário. */
int paging_is_mapped(virt_addr_t virt);

/* Mapeia por identidade [start, start+size). 0 = ok. */
int paging_identity_map_range(phys_addr_t start, uint32_t size, uint32_t flags);

/* ---------------- Physical Memory Manager ---------------- */
phys_addr_t pmm_alloc_frame(void);   /* retorna 0 se sem memória */
void        pmm_free_frame(phys_addr_t frame);
uint32_t    pmm_total_frames(void);
uint32_t    pmm_used_frames(void);
uint32_t    pmm_free_frames(void);
uint32_t    pmm_total_memory(void);  /* bytes */
uint32_t    pmm_free_memory(void);   /* bytes */

/* ---------------- Page Fault ---------------- */
/* Handler de Page Fault.
 * error_code = o que a CPU empurrou
 * regs       = ponteiro para todos os registradores salvos (pode ser NULL) */
void paging_fault_handler(uint32_t error_code, registers_t *regs);
void paging_set_fault_callback(void (*handler)(uint32_t));

#endif /* CPU_PAGING_H */
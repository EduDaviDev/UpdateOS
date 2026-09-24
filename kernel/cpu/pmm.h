#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "../mboot.h"

#define PMM_FRAME_SIZE  4096u   /* 4 KiB, igual à página */

/* Inicializa o PMM a partir do memory map do Multiboot2.
 * Marca como usados: primeiro 4 MiB (BIOS/VGA/boot), kernel e o próprio bitmap.
 * Tudo mais em regiões type=1 fica livre. */
void pmm_init(struct multiboot_info *mb_info);

/* Aloca/libera um frame físico. Retorna endereço físico ou NULL. */
void *pmm_alloc_frame(void);
void  pmm_free_frame(void *frame);

/* Estatísticas */
uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);

#endif
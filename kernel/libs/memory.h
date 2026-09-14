#ifndef LIBS_MEMORY_H
#define LIBS_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* ====================================================================
 * Heap (malloc family)
 *
 * Dependência: paging_init() deve ter sido chamada antes.
 * A primeira chamada de malloc/calloc/realloc auto-inicializa o heap.
 *
 * Região virtual reservada: 0x40000000 .. 0x44000000 (64 MiB).
 * Cada página só é mapeada (e um frame físico alocado) sob demanda.
 * ==================================================================== */

void   heap_init(void);

void  *malloc(size_t size);
void  *calloc(size_t nmemb, size_t size);
void  *realloc(void *ptr, size_t size);
void   free(void *ptr);

/* Estatísticas (em bytes) */
size_t heap_total(void);
size_t heap_used(void);
size_t heap_free_bytes(void);
size_t heap_peak_used(void);

/* Resumo no VGA (no-op se vga_printf não existir). */
void   heap_dump(void);

#endif /* LIBS_MEMORY_H */
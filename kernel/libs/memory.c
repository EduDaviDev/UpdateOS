#include "memory.h"
#include "../cpu/paging.h"

/* vga_printf é opcional: se não existir, heap_dump vira no-op. */
extern int vga_printf(const char *fmt, ...) __attribute__((weak));

/* =====================================================================
 * Heap — free-list com coalescência e crescimento on-demand
 *
 * Layout do bloco (8-aligned):
 *
 *   +----------------+
 *   | size  (payload)|
 *   | used           |
 *   | next           |
 *   | prev           |
 *   | magic          |
 *   | _pad           |
 *   +----------------+  <-- ptr devolvido por malloc
 *   | payload        |
 *   +----------------+
 *
 * header = 24 bytes -> payload sempre 8-aligned.
 * ===================================================================== */

#define HEAP_START   0x40000000u                        /* 1 GiB */
#define HEAP_SIZE    (64u * 1024u * 1024u)              /* 64 MiB */
#define HEAP_END     (HEAP_START + HEAP_SIZE)

#define BLOCK_MAGIC_USED  0xA110C8EDu
#define BLOCK_MAGIC_FREE  0xF4EEF4EEu

typedef struct block {
    uint32_t       size;    /* payload em bytes (sem o header) */
    uint32_t       used;    /* 0 = livre, 1 = em uso */
    struct block  *next;
    struct block  *prev;
    uint32_t       magic;
    uint32_t       _pad;    /* mantém payload 8-aligned */
} block_t;

#define BLOCK_HDR_SIZE  ((uint32_t)sizeof(block_t))    /* 24 */
#define MIN_PAYLOAD     8u
#define ALIGN           8u

static inline uint32_t align_up(uint32_t x) {
    return (x + (ALIGN - 1u)) & ~(ALIGN - 1u);
}

/* Estado global */
static block_t *heap_head       = NULL;
static uint32_t heap_break      = HEAP_START;   /* fim do VA já mapeado */
static size_t   heap_mapped     = 0;            /* bytes VA mapeados */
static size_t   heap_used_bytes = 0;            /* payload em uso */
static size_t   heap_peak_bytes = 0;
static int      heap_ready      = 0;

/* ---------------------------------------------------------------------
 * Cresce o heap mapeando páginas até cobrir min_payload de um único
 * bloco livre. Faz coalescência com o último bloco se for contíguo.
 * Retorna 0 em sucesso, -1 se não há mais VA ou RAM física.
 * --------------------------------------------------------------------- */
static int heap_grow(uint32_t min_payload) {
    while (heap_break + PAGE_SIZE <= HEAP_END) {
        phys_addr_t frame = pmm_alloc_frame();
        if (!frame) return -1;

        if (paging_map_page(heap_break, frame, PAGE_KERNEL) != 0) {
            pmm_free_frame(frame);
            return -1;
        }

        block_t *b = (block_t *)heap_break;
        b->size  = PAGE_SIZE - BLOCK_HDR_SIZE;
        b->used  = 0;
        b->magic = BLOCK_MAGIC_FREE;
        b->next  = NULL;
        b->prev  = NULL;

        if (!heap_head) {
            heap_head = b;
        } else {
            /* Encontra a cauda */
            block_t *t = heap_head;
            while (t->next) t = t->next;

            /* Se a cauda for livre e terminar exatamente onde b começa,
             * funde os dois. */
            uint8_t *t_end = (uint8_t *)t + BLOCK_HDR_SIZE + t->size;
            if (!t->used && t_end == (uint8_t *)b) {
                t->size += BLOCK_HDR_SIZE + b->size;
                b = t;  /* b passa a ser o bloco fundido */
            } else {
                t->next = b;
                b->prev = t;
            }
        }

        heap_break  += PAGE_SIZE;
        heap_mapped += PAGE_SIZE;

        if (b->size >= min_payload) return 0;
    }
    return -1;
}

/* ---------------------------------------------------------------------
 * Split: divide um bloco livre grande em [size][resto].
 * --------------------------------------------------------------------- */
static void split_block(block_t *b, uint32_t size) {
    if (b->size < size + BLOCK_HDR_SIZE + MIN_PAYLOAD) return; /* não vale */

    block_t *rest = (block_t *)((uint8_t *)b + BLOCK_HDR_SIZE + size);
    rest->size  = b->size - size - BLOCK_HDR_SIZE;
    rest->used  = 0;
    rest->magic = BLOCK_MAGIC_FREE;
    rest->next  = b->next;
    rest->prev  = b;

    if (b->next) b->next->prev = rest;
    b->next = rest;
    b->size = size;
}

/* ---------------------------------------------------------------------
 * Coalescência com vizinho anterior e/ou próximo (se livres e contíguos).
 * --------------------------------------------------------------------- */
static block_t *coalesce(block_t *b) {
    /* Com o próximo */
    if (b->next && !b->next->used) {
        block_t *n = b->next;
        b->size += BLOCK_HDR_SIZE + n->size;
        b->next  = n->next;
        if (n->next) n->next->prev = b;
    }
    /* Com o anterior */
    if (b->prev && !b->prev->used) {
        block_t *p = b->prev;
        p->size += BLOCK_HDR_SIZE + b->size;
        p->next  = b->next;
        if (b->next) b->next->prev = p;
        b = p;
    }
    return b;
}

/* ---------------------------------------------------------------------
 * heap_init: prepara o heap com uma página inicial.
 * --------------------------------------------------------------------- */
void heap_init(void) {
    if (heap_ready) return;

    heap_head       = NULL;
    heap_break      = HEAP_START;
    heap_mapped     = 0;
    heap_used_bytes = 0;
    heap_peak_bytes = 0;
    heap_ready      = 1;

    /* Uma página já cobre alocações pequenas. */
    heap_grow(MIN_PAYLOAD);
}

/* ---------------------------------------------------------------------
 * malloc
 * --------------------------------------------------------------------- */
void *malloc(size_t size) {
    if (size == 0) return NULL;
    if (!heap_ready) heap_init();

    uint32_t need = align_up((uint32_t)size);
    if (need < MIN_PAYLOAD) need = MIN_PAYLOAD;

    /* First-fit */
    for (;;) {
        for (block_t *b = heap_head; b; b = b->next) {
            if (b->used || b->size < need) continue;

            split_block(b, need);
            b->used  = 1;
            b->magic = BLOCK_MAGIC_USED;

            heap_used_bytes += b->size;
            if (heap_used_bytes > heap_peak_bytes)
                heap_peak_bytes = heap_used_bytes;

            return (uint8_t *)b + BLOCK_HDR_SIZE;
        }

        /* Nada serviu: cresce. */
        if (heap_grow(need) != 0) return NULL;
    }
}

/* ---------------------------------------------------------------------
 * calloc
 * --------------------------------------------------------------------- */
void *calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) return NULL;
    size_t total = nmemb * size;
    /* Overflow check simples */
    if (total / size != nmemb) return NULL;

    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

/* ---------------------------------------------------------------------
 * free
 * --------------------------------------------------------------------- */
void free(void *ptr) {
    if (!ptr) return;

    block_t *b = (block_t *)((uint8_t *)ptr - BLOCK_HDR_SIZE);
    if (b->magic != BLOCK_MAGIC_USED) {
        /* Ponteiro inválido ou double-free: ignora silenciosamente. */
        return;
    }

    b->used  = 0;
    b->magic = BLOCK_MAGIC_FREE;

    if (heap_used_bytes >= b->size)
        heap_used_bytes -= b->size;
    else
        heap_used_bytes = 0;

    coalesce(b);
}

/* ---------------------------------------------------------------------
 * realloc
 * --------------------------------------------------------------------- */
void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }

    block_t *b = (block_t *)((uint8_t *)ptr - BLOCK_HDR_SIZE);
    if (b->magic != BLOCK_MAGIC_USED) return NULL;

    uint32_t need = align_up((uint32_t)size);
    if (need < MIN_PAYLOAD) need = MIN_PAYLOAD;

    /* Já cabe? */
    if (b->size >= need) {
        /* Opcional: encolher devolvendo o excesso à free-list */
        split_block(b, need);
        return ptr;
    }

    /* Tenta fundir com o próximo livre para caber in-place */
    if (b->next && !b->next->used &&
        b->size + BLOCK_HDR_SIZE + b->next->size >= need) {
        block_t *n = b->next;
        b->size += BLOCK_HDR_SIZE + n->size;
        b->next  = n->next;
        if (n->next) n->next->prev = b;
        split_block(b, need);
        return ptr;
    }

    /* Realoca em outro lugar */
    void *np = malloc(size);
    if (!np) return NULL;
    memcpy(np, ptr, b->size < size ? b->size : size);
    free(ptr);
    return np;
}

/* ---------------------------------------------------------------------
 * Estatísticas
 * --------------------------------------------------------------------- */
size_t heap_total(void)      { return heap_mapped; }
size_t heap_used(void)       { return heap_used_bytes; }
size_t heap_peak_used(void)  { return heap_peak_bytes; }

size_t heap_free_bytes(void) {
    size_t f = 0;
    for (block_t *b = heap_head; b; b = b->next)
        if (!b->used) f += b->size;
    return f;
}

/* ---------------------------------------------------------------------
 * Dump
 * --------------------------------------------------------------------- */
void heap_dump(void) {
    if (!vga_printf) return;

    vga_printf("[heap] VA=%u KiB  payload usado=%u B  livre=%u B  pico=%u B\n",
               (uint32_t)(heap_mapped / 1024),
               (uint32_t)heap_used(),
               (uint32_t)heap_free_bytes(),
               (uint32_t)heap_peak_used());

    int i = 0;
    for (block_t *b = heap_head; b && i < 16; b = b->next, i++) {
        vga_printf("  [%d] %s size=%u\n",
                   i, b->used ? "USED" : "free", b->size);
    }
}
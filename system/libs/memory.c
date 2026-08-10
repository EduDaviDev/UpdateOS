#include "memory.h"
#include "string.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Estrutura de bloco livre */
typedef struct Block {
    size_t size;
    struct Block *next;
    bool used;
} Block;

#define HEAP_SIZE (1024 * 1024)  /* 1 MB de heap estático */

static uint8_t heap[HEAP_SIZE];
static Block *free_list = NULL;

/* Inicializa o heap na primeira chamada a malloc */
static void heap_init(void) {
    if (free_list == NULL) {
        Block *first = (Block*)heap;
        first->size = HEAP_SIZE - sizeof(Block);
        first->next = NULL;
        first->used = false;
        free_list = first;
    }
}

/* Alinhamento de 8 bytes para facilitar */
static inline size_t align_up(size_t size) {
    return (size + 7) & ~7;
}

/* Malloc – first‑fit */
void *malloc(size_t size) {
    heap_init();

    if (size == 0) return NULL;

    size = align_up(size);  /* alinha o tamanho solicitado */

    Block *prev = NULL;
    Block *curr = free_list;

    while (curr) {
        if (!curr->used && curr->size >= size) {
            /* Se o bloco é maior que o necessário, divide */
            if (curr->size > size + sizeof(Block) + 8) {
                Block *new_block = (Block*)((uint8_t*)curr + sizeof(Block) + size);
                new_block->size = curr->size - size - sizeof(Block);
                new_block->next = curr->next;
                new_block->used = false;

                curr->size = size;
                curr->next = new_block;
            }

            curr->used = true;
            return (uint8_t*)curr + sizeof(Block);
        }

        prev = curr;
        curr = curr->next;
    }

    /* Não há memória suficiente */
    return NULL;
}

/* Free – libera bloco e coalesce com vizinho seguinte se livre */
void free(void *ptr) {
    if (!ptr) return;

    Block *block = (Block*)((uint8_t*)ptr - sizeof(Block));
    block->used = false;

    /* Coalesce com o próximo se estiver livre */
    if (block->next && !block->next->used) {
        block->size += sizeof(Block) + block->next->size;
        block->next = block->next->next;
    }

    /* Coalesce com o anterior (percorremos a lista para encontrar o anterior) */
    Block *prev = NULL;
    Block *curr = free_list;
    while (curr && curr != block) {
        prev = curr;
        curr = curr->next;
    }

    if (prev && !prev->used) {
        prev->size += sizeof(Block) + prev->next->size;
        prev->next = prev->next->next;
    }
}

/* Calloc – aloca e zera */
void *calloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

/* Realloc – realoca (simples, usando malloc/memcpy/free) */
void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }

    Block *block = (Block*)((uint8_t*)ptr - sizeof(Block));
    if (block->size >= size) {
        /* Se o bloco atual já é suficiente, apenas retorna o mesmo ponteiro */
        /* Mas poderíamos também dividir o bloco se sobrar muito, mas vamos simplificar */
        return ptr;
    }

    /* Aloca novo bloco, copia e libera antigo */
    void *new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size < size ? block->size : size);
        free(ptr);
    }
    return new_ptr;
}
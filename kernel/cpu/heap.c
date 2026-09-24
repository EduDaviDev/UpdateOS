#include "heap.h"
#include "../libs/string.h"

// Estrutura do cabeçalho de cada bloco
typedef struct block_header {
    size_t size;                // Tamanho do bloco (incluindo cabeçalho)
    uint8_t is_free;            // 1 = livre, 0 = alocado
    struct block_header *next;  // Próximo bloco na lista
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)
#define ALIGN4(x) (((x) + 3) & ~3)

static block_header_t *heap_start = NULL;
static block_header_t *heap_end   = NULL;

/**
 * Inicializa o heap criando um único bloco livre que abrange toda a região.
 */
void heap_init(void) {
    heap_start = (block_header_t *)HEAP_START;
    heap_start->size = HEAP_SIZE;
    heap_start->is_free = 1;
    heap_start->next = NULL;

    heap_end = (block_header_t *)((uint8_t *)heap_start + HEAP_SIZE);
}

/**
 * Aloca um bloco de memória no heap (first-fit).
 * @param size  Tamanho solicitado em bytes.
 * @param align Alinhamento (0 = padrão, 4 bytes).
 * @return Ponteiro para o bloco alocado ou NULL.
 */
void *heap_alloc(size_t size, size_t align) {
    if (size == 0) return NULL;

    // Alinhar o tamanho
    size_t aligned_size = ALIGN4(size);
    if (align > 4) {
        aligned_size = (aligned_size + align - 1) & ~(align - 1);
    }

    block_header_t *current = heap_start;

    while (current) {
        if (current->is_free && current->size >= aligned_size + HEADER_SIZE) {
            // Bloco grande o suficiente: dividir se sobrar espaço
            size_t remaining = current->size - (aligned_size + HEADER_SIZE);

            if (remaining > HEADER_SIZE) {
                // Criar um novo bloco livre após o bloco alocado
                block_header_t *new_block = (block_header_t *)((uint8_t *)current + HEADER_SIZE + aligned_size);
                new_block->size = remaining;
                new_block->is_free = 1;
                new_block->next = current->next;

                current->size = aligned_size + HEADER_SIZE;
                current->next = new_block;
            }

            current->is_free = 0;
            return (void *)((uint8_t *)current + HEADER_SIZE);
        }
        current = current->next;
    }

    return NULL; // Sem memória suficiente
}

/**
 * Libera um bloco previamente alocado.
 */
void heap_free(void *ptr) {
    if (!ptr) return;

    block_header_t *block = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    block->is_free = 1;

    // Coalescência com o próximo bloco, se estiver livre
    if (block->next && block->next->is_free) {
        block->size += block->next->size;
        block->next = block->next->next;
    }
}

/**
 * Redimensiona um bloco alocado.
 */
void *heap_realloc(void *ptr, size_t size) {
    if (!ptr) return heap_alloc(size, 0);
    if (size == 0) {
        heap_free(ptr);
        return NULL;
    }

    block_header_t *block = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    size_t old_size = block->size - HEADER_SIZE;

    if (old_size >= size) {
        // O bloco atual já é grande o suficiente
        return ptr;
    }

    // Alocar um novo bloco, copiar os dados e liberar o antigo
    void *new_ptr = heap_alloc(size, 0);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, old_size);
    heap_free(ptr);
    return new_ptr;
}
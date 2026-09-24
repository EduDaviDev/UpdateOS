#include "memory.h"
#include "../cpu/heap.h"
#include "string.h"   // Para memcpy, memset, memcmp

/**
 * Aloca um bloco de memória.
 * Usa o heap do kernel.
 */
void *malloc(size_t size) {
    return heap_alloc(size, 0);
}

/**
 * Aloca e zera um bloco de memória.
 */
void *calloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = heap_alloc(total, 0);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

/**
 * Redimensiona um bloco alocado.
 */
void *realloc(void *ptr, size_t size) {
    return heap_realloc(ptr, size);
}

/**
 * Libera um bloco alocado.
 */
void free(void *ptr) {
    heap_free(ptr);
}
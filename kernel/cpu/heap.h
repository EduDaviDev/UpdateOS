#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

// Endereço inicial do heap (após o kernel)
#define HEAP_START 0x00400000
#define HEAP_SIZE  0x01000000  // 16 MB

// Inicializa o heap
void heap_init(void);

// Funções de alocação (usadas por memory.c)
void *heap_alloc(size_t size, size_t align);
void  heap_free(void *ptr);
void *heap_realloc(void *ptr, size_t size);

#endif
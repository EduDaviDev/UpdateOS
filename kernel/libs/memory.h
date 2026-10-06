#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

// Funções padrão de alocação
void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);

#endif
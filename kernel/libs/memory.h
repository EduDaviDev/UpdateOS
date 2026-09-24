#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

// Funções padrão de alocação
void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);

// Funções de memória (já podem existir em string.h)
// Incluímos aqui para clareza
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int   memcmp(const void *s1, const void *s2, size_t n);

#endif
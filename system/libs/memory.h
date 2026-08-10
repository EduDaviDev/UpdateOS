#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
/**
 * Limpa o buffer de entrada preenchendo-o com zeros (nulos).
 * 
 * @param buffer  Ponteiro para o início do buffer
 * @param size Tamanho total do buffer em bytes
 */
void clear_buffer(char *buffer, size_t size);

#endif
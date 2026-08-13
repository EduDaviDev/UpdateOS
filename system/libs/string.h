#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdarg.h>

/* Tamanho de string */
size_t strlen(const char *str);

/* Cópia de strings */
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

/* Concatenação */
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);

/* Comparação */
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

/* Busca em strings */
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);

/* Memória */
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

/* snprintf – formata uma string com limite de tamanho */
int snprintf(char *str, size_t size, const char *format, ...);

/* vsnprintf – versão com va_list */
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

#endif
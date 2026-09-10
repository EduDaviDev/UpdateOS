#ifndef KERNEL_LIBS_STRING_H
#define KERNEL_LIBS_STRING_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================
 *  Memória
 * ============================================================ */

void  *memset(void *dest, int c, size_t n);
void  *memcpy(void *dest, const void *src, size_t n);
void  *memmove(void *dest, const void *src, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
void  *memchr(const void *s, int c, size_t n);

/* ============================================================
 *  Strings
 * ============================================================ */

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
char  *strcpy(char *dest, const char *src);
char  *strncpy(char *dest, const char *src, size_t n);
char  *strcat(char *dest, const char *src);
char  *strncat(char *dest, const char *src, size_t n);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
char  *strstr(const char *haystack, const char *needle);

/* ============================================================
 *  Conversão
 * ============================================================ */

int    atoi(const char *s);
long   atol(const char *s);

/* ============================================================
 *  Extras (não padronizadas, mas úteis no kernel)
 * ============================================================ */

/* Inverte uma string in-place */
void   strrev(char *s);

/* Converte inteiro para string em uma base (2..36) */
void   itoa(int value, char *buf, int base);
void   utoa(uint32_t value, char *buf, int base);

/* Converte uint64 para string em base 10 (útil em logs) */
void   utoa64(uint64_t value, char *buf, int base);

/* Compara apenas para igualdade (mais rápido que strcmp em alguns casos) */
int    streq(const char *a, const char *b);

#endif /* KERNEL_LIBS_STRING_H */
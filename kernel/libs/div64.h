#ifndef KERNEL_LIBS_DIV64_H
#define KERNEL_LIBS_DIV64_H

#include <stdint.h>

/* ============================================================
 *  Helpers chamados pelo GCC quando você usa / e % em tipos
 *  de 64 bits em código 32 bits. Os nomes DEVEM ser exatamente
 *  estes, senão o linker não encontra.
 * ============================================================ */

uint64_t __udivdi3(uint64_t n, uint64_t d);
uint64_t __umoddi3(uint64_t n, uint64_t d);
int64_t  __divdi3 (int64_t  n, int64_t  d);
int64_t  __moddi3 (int64_t  n, int64_t  d);

/* Helper interno (GCC pode chamar diretamente também) */
uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t *rem);

/* ============================================================
 *  Shifts de 64 bits (em alguns casos o GCC emite chamada)
 * ============================================================ */

uint64_t __lshrdi3(uint64_t a, int b);
int64_t  __ashldi3(int64_t  a, int b);
int64_t  __ashrdi3(int64_t  a, int b);

/* ============================================================
 *  Multiplicação de 64 bits
 * ============================================================ */

int64_t  __muldi3(int64_t a, int64_t b);

#endif /* KERNEL_LIBS_DIV64_H */
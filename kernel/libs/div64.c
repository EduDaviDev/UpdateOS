#include "div64.h"

/* ============================================================
 *  Divisão sem sinal de 64 bits, retornando quociente e resto.
 *
 *  Estratégia:
 *    1. Casos triviais (d == 0, d > n, d == n).
 *    2. Se d cabe em 32 bits, usa a instrução `divl` do x86
 *       (que divide EDX:EAX por um operando de 32 bits).
 *    3. Senão, faz shift-subtract bit-a-bit (raro).
 * ============================================================ */

uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t *rem) {
    /* Divisão por zero: comportamento indefinido. Retornamos 0
       para evitar trap dentro do kernel (o chamador deve tratar). */
    if (d == 0) {
        if (rem) *rem = 0;
        return 0;
    }

    /* Casos triviais */
    if (d > n) {
        if (rem) *rem = n;
        return 0;
    }
    if (d == n) {
        if (rem) *rem = 0;
        return 1;
    }

    /* ---------- Caminho rápido: divisor cabe em 32 bits ---------- */
    if ((d >> 32) == 0) {
        uint32_t d32  = (uint32_t)d;
        uint32_t n_hi = (uint32_t)(n >> 32);
        uint32_t n_lo = (uint32_t)n;

        uint32_t q_lo, q_hi, r;

        if (n_hi < d32) {
            /* (n_hi:n_lo) / d32 -> cabe em 32 bits.
               divl divide EDX:EAX por operando; resultado vai em EAX,
               resto em EDX. */
            __asm__ volatile (
                "divl %4"
                : "=a"(q_lo), "=d"(r)
                : "a"(n_lo), "d"(n_hi), "r"(d32)
            );
            if (rem) *rem = (uint64_t)r;
            return (uint64_t)q_lo;
        } else {
            /* Quociente não cabe em 32 bits: divide em duas etapas.
               Primeiro (0:n_hi) / d32 -> q_hi e resto r_hi.
               Depois (r_hi:n_lo) / d32 -> q_lo e resto final. */
            __asm__ volatile (
                "divl %4"
                : "=a"(q_hi), "=d"(r)
                : "a"(n_hi), "d"(0u), "r"(d32)
            );

            uint32_t r_hi = r;
            __asm__ volatile (
                "divl %4"
                : "=a"(q_lo), "=d"(r)
                : "a"(n_lo), "d"(r_hi), "r"(d32)
            );

            if (rem) *rem = (uint64_t)r;
            return ((uint64_t)q_hi << 32) | q_lo;
        }
    }

    /* ---------- Caminho lento: divisor > 32 bits ----------
       Shift-subtract. Só acontece quando os dois operandos são
       realmente grandes (ex.: dividir por 0x1_0000_0000+). */
    uint64_t q = 0;
    uint64_t r = 0;

    /* Encontra o bit mais significativo de n */
    int shift = 63;
    while (shift > 0 && ((n >> shift) & 1) == 0) {
        shift--;
    }

    for (int i = shift; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
            q |= ((uint64_t)1 << i);
        }
    }

    if (rem) *rem = r;
    return q;
}

/* ============================================================
 *  Wrappers com os nomes que o GCC/LD procuram
 * ============================================================ */

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    return __udivmoddi4(n, d, 0);
}

uint64_t __umoddi3(uint64_t n, uint64_t d) {
    uint64_t r;
    __udivmoddi4(n, d, &r);
    return r;
}

int64_t __divdi3(int64_t n, int64_t d) {
    int      neg = 0;
    uint64_t un  = (uint64_t)n;
    uint64_t ud  = (uint64_t)d;

    if (n < 0) { un = (uint64_t)(-(int64_t)un); neg ^= 1; }
    if (d < 0) { ud = (uint64_t)(-(int64_t)ud); neg ^= 1; }

    uint64_t q = __udivmoddi4(un, ud, 0);
    return neg ? -(int64_t)q : (int64_t)q;
}

int64_t __moddi3(int64_t n, int64_t d) {
    int      neg = 0;
    uint64_t un  = (uint64_t)n;
    uint64_t ud  = (uint64_t)d;

    if (n < 0) { un = (uint64_t)(-(int64_t)un); neg = 1; }
    if (d < 0) { ud = (uint64_t)(-(int64_t)ud); }

    uint64_t r;
    __udivmoddi4(un, ud, &r);
    return neg ? -(int64_t)r : (int64_t)r;
}

/* ============================================================
 *  Shifts de 64 bits
 *
 *  Implementados com shifts de 32 bits (que o GCC inline).
 *  `b` é sempre um valor pequeno em prática, mas cobrimos
 *  b >= 64 também por segurança.
 * ============================================================ */

uint64_t __lshrdi3(uint64_t a, int b) {
    if (b <= 0)  return a;
    if (b >= 64) return 0;

    uint32_t lo = (uint32_t)a;
    uint32_t hi = (uint32_t)(a >> 32);

    if (b >= 32) {
        lo = hi >> (b - 32);
        hi = 0;
    } else {
        lo = (lo >> b) | (hi << (32 - b));
        hi = hi >> b;
    }
    return ((uint64_t)hi << 32) | lo;
}

int64_t __ashldi3(int64_t a, int b) {
    if (b <= 0)  return a;
    if (b >= 64) return 0;

    uint32_t lo = (uint32_t)(uint64_t)a;
    uint32_t hi = (uint32_t)((uint64_t)a >> 32);

    if (b >= 32) {
        hi = lo << (b - 32);
        lo = 0;
    } else {
        hi = (hi << b) | (lo >> (32 - b));
        lo = lo << b;
    }
    return (int64_t)(((uint64_t)hi << 32) | lo);
}

int64_t __ashrdi3(int64_t a, int b) {
    if (b <= 0)  return a;

    int sign = (a < 0);

    if (b >= 64) {
        return sign ? -1 : 0;
    }

    uint64_t ua = __lshrdi3((uint64_t)a, b);

    if (sign) {
        ua |= (~(uint64_t)0) << (64 - b);
    }
    return (int64_t)ua;
}

/* ============================================================
 *  Multiplicação de 64 bits
 *
 *  Quebra em 4 produtos de 32 bits. Só os três primeiros
 *  importam para o resultado de 64 bits (hi*hi é descartado).
 * ============================================================ */

int64_t __muldi3(int64_t a, int64_t b) {
    uint64_t ua = (uint64_t)a;
    uint64_t ub = (uint64_t)b;

    uint32_t a_lo = (uint32_t)ua;
    uint32_t a_hi = (uint32_t)(ua >> 32);
    uint32_t b_lo = (uint32_t)ub;
    uint32_t b_hi = (uint32_t)(ub >> 32);

    uint64_t lo_lo = (uint64_t)a_lo * b_lo;
    uint64_t hi_lo = (uint64_t)a_hi * b_lo;
    uint64_t lo_hi = (uint64_t)a_lo * b_hi;

    uint64_t mid = (lo_lo >> 32)
                 + (uint32_t)hi_lo
                 + (uint32_t)lo_hi;

    return (int64_t)((lo_lo & 0xFFFFFFFFu) | (mid << 32));
}
#ifndef MATH_H
#define MATH_H

#include <stdint.h>

/* ============================================================
   Funções inteiras
   ============================================================ */

int abs(int n);
int min(int a, int b);
int max(int a, int b);
long long pow_int(int base, unsigned int exp);
unsigned int isqrt(unsigned int n);

/* ============================================================
   Funções de ponto flutuante (software)
   ============================================================ */

float sqrt_float(float x);
float exp_float(float x);
float log_float(float x);
float pow_float(float base, float exp);

/* ============================================================
   Funções auxiliares
   ============================================================ */

void itoa(int n, char *str);
void utoa(unsigned int n, char *str, int base);
int char_to_int(char c);
int string_to_int(const char *str);

/* ============================================================
   Funções de suporte para libgcc (divisão de 64 bits)
   ============================================================ */

uint64_t __udivdi3(uint64_t a, uint64_t b);
uint64_t __umoddi3(uint64_t a, uint64_t b);
int64_t  __divdi3(int64_t a, int64_t b);
int64_t  __moddi3(int64_t a, int64_t b);

/* ============================================================
   Funções para ponto flutuante (isnan, isinf)
   ============================================================ */

int isnan(float x);
int isinf(float x);

#endif /* MATH_H */
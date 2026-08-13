#include "math.h"
#include <stdint.h>

/* ============================================================
   Funções inteiras
   ============================================================ */

int abs(int n) {
    return (n < 0) ? -n : n;
}

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

long long pow_int(int base, unsigned int exp) {
    long long result = 1;
    for (unsigned int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

unsigned int isqrt(unsigned int n) {
    if (n == 0) return 0;
    unsigned int x = n;
    unsigned int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

/* ============================================================
   Funções de ponto flutuante (software)
   ============================================================ */

#define E 2.718281828459045f

float sqrt_float(float x) {
    if (x < 0) return -1;
    if (x == 0) return 0;
    float guess = x;
    float epsilon = 0.00001f;
    while (1) {
        float new_guess = (guess + x / guess) / 2;
        if (new_guess - guess < epsilon && new_guess - guess > -epsilon)
            return new_guess;
        guess = new_guess;
    }
}

float exp_float(float x) {
    float sum = 1.0f;
    float term = 1.0f;
    for (int i = 1; i < 20; i++) {
        term *= x / i;
        sum += term;
    }
    return sum;
}

float log_float(float x) {
    if (x <= 0) return -1;
    float y = x - 1;
    float sum = 0;
    float term = y;
    float sign = 1;
    for (int i = 1; i < 30; i++) {
        sum += sign * term / i;
        term *= y;
        sign = -sign;
    }
    return sum;
}

float pow_float(float base, float exp) {
    if (base == 0) {
        if (exp == 0) return 1;
        else return 0;
    }
    if (base < 0) return 0;
    return exp_float(exp * log_float(base));
}

/* ============================================================
   Funções auxiliares
   ============================================================ */

void itoa(int n, char *str) {
    int i = 0;
    int sign = n;
    if (n < 0) {
        n = -n;
        str[i++] = '-';
    }
    char temp[16];
    int j = 0;
    if (n == 0) {
        temp[j++] = '0';
    } else {
        while (n > 0) {
            temp[j++] = '0' + (n % 10);
            n /= 10;
        }
    }
    while (j > 0) {
        str[i++] = temp[--j];
    }
    str[i] = '\0';
}

void utoa(unsigned int n, char *str, int base) {
    int i = 0;
    char temp[32];
    int j = 0;
    if (n == 0) {
        temp[j++] = '0';
    } else {
        while (n > 0) {
            int digit = n % base;
            temp[j++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
            n /= base;
        }
    }
    while (j > 0) {
        str[i++] = temp[--j];
    }
    str[i] = '\0';
}

int char_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    // Opcional: se quiser suportar letras hexadecimais (A-F, a-f)
    // else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    // else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1; // caractere inválido
}

int string_to_int(const char *str) {
    int result = 0;
    int sign = 1;
    int i = 0;

    // Ignora espaços iniciais
    while (str[i] == ' ') i++;

    // Verifica sinal
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    // Converte dígitos
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result * sign;
}

/* ============================================================
   Funções de suporte para libgcc (divisão de 64 bits)
   ============================================================ */

uint64_t __udivdi3(uint64_t a, uint64_t b) {
    if (b == 0) return 0; // evitar divisão por zero
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r <<= 1;
        r |= (a >> i) & 1;
        if (r >= b) {
            r -= b;
            q |= (1ULL << i);
        }
    }
    return q;
}

uint64_t __umoddi3(uint64_t a, uint64_t b) {
    if (b == 0) return 0;
    uint64_t q = __udivdi3(a, b);
    return a - q * b;
}

int64_t __divdi3(int64_t a, int64_t b) {
    if (b == 0) return 0;
    int sign = ((a < 0) ^ (b < 0)) ? -1 : 1;
    uint64_t ua = (a < 0) ? -a : a;
    uint64_t ub = (b < 0) ? -b : b;
    uint64_t q = __udivdi3(ua, ub);
    return sign * q;
}

int64_t __moddi3(int64_t a, int64_t b) {
    if (b == 0) return 0;
    uint64_t ua = (a < 0) ? -a : a;
    uint64_t ub = (b < 0) ? -b : b;
    uint64_t r = __umoddi3(ua, ub);
    return (a < 0) ? -r : r;
}

/* ============================================================
   Funções para ponto flutuante (isnan, isinf)
   ============================================================ */

int isnan(float x) {
    // NaN é o único valor que não é igual a si mesmo
    return x != x;
}

int isinf(float x) {
    // Infinito positivo ou negativo
    return (x == (1.0f / 0.0f)) || (x == (-1.0f / 0.0f));
}
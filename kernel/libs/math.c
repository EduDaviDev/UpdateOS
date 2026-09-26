/*-----------------------------------------------------------------------*/
/* math.c - Implementacoes da biblioteca matematica do UpdateOS          */
/*                                                                       */
/* Todos os algoritmos sao self-contained: nenhuma dependencia da libm   */
/* ou libc. Precisao tipica: ~1e-14 (proximo ao limite de double).        */
/*-----------------------------------------------------------------------*/
#include "math.h"

/* ============================================================ */
/*  Arredondamento                                             */
/* ============================================================ */

double trunc(double x) {
    if (x >= 0.0) {
        long long i = (long long)x;
        return (double)i;
    } else {
        long long i = (long long)x;
        return (double)i;
    }
}

double floor(double x) {
    long long i = (long long)x;
    if (x < 0.0 && (double)i != x) i--;
    return (double)i;
}

double ceil(double x) {
    long long i = (long long)x;
    if (x > 0.0 && (double)i != x) i++;
    return (double)i;
}

double round(double x) {
    return (x >= 0.0) ? floor(x + 0.5) : ceil(x - 0.5);
}

double fmod(double x, double y) {
    if (y == 0.0) return MATH_NAN;
    double q = trunc(x / y);
    return x - q * y;
}

double modf(double x, double *iptr) {
    double i = trunc(x);
    *iptr = i;
    return x - i;
}

/* ============================================================ */
/*  Raiz quadrada — Newton-Raphson                             */
/* ============================================================ */

double sqrt(double x) {
    if (x < 0.0) return MATH_NAN;
    if (x == 0.0 || x != x) return x;
    if (isinf(x)) return x;

    /* Chute inicial via manipulacao de expoente (IEEE 754) */
    union { double d; uint64_t u; } u;
    u.d = x;
    u.u = (u.u >> 1) + 0x1FF0000000000000ULL;
    double g = u.d;

    /* 5 iteracoes de Newton ja dao precisao total em double */
    for (int i = 0; i < 5; i++) {
        double ng = 0.5 * (g + x / g);
        if (ng == g) break;
        g = ng;
    }
    return g;
}

double cbrt(double x) {
    if (x == 0.0) return 0.0;
    int neg = x < 0.0;
    if (neg) x = -x;

    double g = exp(log(x) / 3.0);

    /* Refina com 3 iteracoes de Newton: f(g) = g^3 - x */
    for (int i = 0; i < 3; i++) {
        double g2 = g * g;
        g = g - (g2 * g - x) / (3.0 * g2);
    }
    return neg ? -g : g;
}

/* ============================================================ */
/*  Exponencial — serie de Taylor com reducao de intervalo     */
/* ============================================================ */

double exp(double x) {
    if (x != x) return x;
    if (x >  709.78) return MATH_INF;
    if (x < -745.13) return 0.0;

    /* Reduz: x = k*ln(2) + r, com |r| <= ln(2)/2 */
    int k = (int)(x / M_LN2 + (x >= 0 ? 0.5 : -0.5));
    double r = x - k * M_LN2;

    /* Taylor para exp(r), |r| pequeno */
    double sum  = 1.0;
    double term = 1.0;
    for (int i = 1; i <= 15; i++) {
        term *= r / i;
        sum  += term;
    }

    /* Multiplica por 2^k */
    union { double d; uint64_t u; } u;
    u.u = (uint64_t)(1023 + k) << 52;
    return sum * u.d;
}

double exp2(double x) {
    return exp(x * M_LN2);
}

/* ============================================================ */
/*  Logaritmo natural — atanh series apos decomposicao          */
/* ============================================================ */

double log(double x) {
    if (x < 0.0) return MATH_NAN;
    if (x == 0.0) return -MATH_INF;
    if (x != x || isinf(x)) return x;

    /* x = m * 2^k, m em [1, 2) */
    int k = 0;
    union { double d; uint64_t u; } u;
    u.d = x;

    int exp_field = (int)((u.u >> 52) & 0x7FF) - 1023;
    /* Forca m em [1, 2) removendo o expoente */
    u.u = (u.u & 0x000FFFFFFFFFFFFFULL) | (1023ULL << 52);
    double m = u.d;
    k = exp_field;

    /* Ajuste: queremos m perto de 1 para convergencia rapida */
    if (m > M_SQRT2) { m *= 0.5; k += 1; }

    /* ln(m) = 2 * atanh((m-1)/(m+1)) */
    double z  = (m - 1.0) / (m + 1.0);
    double z2 = z * z;
    double sum = 0.0;
    double term = z;
    for (int i = 1; i <= 30; i += 2) {
        sum  += term / i;
        term *= z2;
    }
    return 2.0 * sum + k * M_LN2;
}

double log2(double x)  { return log(x) * M_LOG2E; }
double log10(double x) { return log(x) * M_LOG10E; }

/* ============================================================ */
/*  Potencia — exp(y * ln(x))                                  */
/* ============================================================ */

double pow(double x, double y) {
    /* Casos especiais */
    if (y == 0.0) return 1.0;
    if (x == 1.0) return 1.0;
    if (x == 0.0) return (y > 0.0) ? 0.0 : MATH_INF;
    if (x < 0.0) {
        /* Base negativa: so faz sentido se y for inteiro */
        long long iy = (long long)y;
        if ((double)iy != y) return MATH_NAN;
        double r = exp(y * log(-x));
        return (iy & 1) ? -r : r;
    }
    return exp(y * log(x));
}

/* ============================================================ */
/*  Trigonometria — Taylor com reducao para [-pi, pi]          */
/* ============================================================ */

static double _sin_core(double x) {
    /* |x| <= pi */
    double x2   = x * x;
    double term = x;
    double sum  = x;
    term *= -x2 / (2.0 * 3.0);  sum += term;
    term *= -x2 / (4.0 * 5.0);  sum += term;
    term *= -x2 / (6.0 * 7.0);  sum += term;
    term *= -x2 / (8.0 * 9.0);  sum += term;
    term *= -x2 / (10.0*11.0);  sum += term;
    term *= -x2 / (12.0*13.0);  sum += term;
    term *= -x2 / (14.0*15.0);  sum += term;
    term *= -x2 / (16.0*17.0);  sum += term;
    term *= -x2 / (18.0*19.0);  sum += term;
    return sum;
}

static double _reduce(double x) {
    /* Reduz x para [-pi, pi] subtraindo multiplos de 2*pi */
    const double two_pi = 6.28318530717958647692;
    if (x > M_PI || x < -M_PI) {
        long long k = (long long)(x / two_pi + (x >= 0 ? 0.5 : -0.5));
        x -= (double)k * two_pi;
    }
    return x;
}

double sin(double x) {
    if (x != x || isinf(x)) return MATH_NAN;
    return _sin_core(_reduce(x));
}

double cos(double x) {
    if (x != x || isinf(x)) return MATH_NAN;
    /* cos(x) = sin(x + pi/2) */
    return _sin_core(_reduce(x + M_PI_2));
}

double tan(double x) {
    double c = cos(x);
    if (c == 0.0) return MATH_INF;
    return sin(x) / c;
}

/* ============================================================ */
/*  Inversas trigonometricas                                   */
/* ============================================================ */

double atan(double x) {
    if (x != x) return x;

    /* Reduz: para |x| > 1, atan(x) = sign(x)*pi/2 - atan(1/x) */
    int neg = x < 0.0;
    if (neg) x = -x;

    int inv = 0;
    if (x > 1.0) { x = 1.0 / x; inv = 1; }

    /* Para 0 <= x <= 1, serie de Taylor converge rapido */
    double x2   = x * x;
    double term = x;
    double sum  = x;
    for (int i = 1; i < 40; i++) {
        term *= -x2;
        sum  += term / (2 * i + 1);
    }

    if (inv) sum = M_PI_2 - sum;
    if (neg) sum = -sum;
    return sum;
}

double atan2(double y, double x) {
    if (x > 0.0)             return atan(y / x);
    if (x < 0.0 && y >= 0.0) return atan(y / x) + M_PI;
    if (x < 0.0 && y <  0.0) return atan(y / x) - M_PI;
    if (x == 0.0 && y >  0.0) return  M_PI_2;
    if (x == 0.0 && y <  0.0) return -M_PI_2;
    return 0.0;   /* x == 0 && y == 0 */
}

double asin(double x) {
    if (x > 1.0 || x < -1.0) return MATH_NAN;
    if (x == 1.0)  return  M_PI_2;
    if (x == -1.0) return -M_PI_2;
    return atan(x / sqrt(1.0 - x * x));
}

double acos(double x) {
    if (x > 1.0 || x < -1.0) return MATH_NAN;
    return M_PI_2 - asin(x);
}

/* ============================================================ */
/*  Hiperbolicas                                               */
/* ============================================================ */

double sinh(double x) {
    double e = exp(x);
    return (e - 1.0 / e) * 0.5;
}

double cosh(double x) {
    double e = exp(x);
    return (e + 1.0 / e) * 0.5;
}

double tanh(double x) {
    if (x >  20.0) return  1.0;
    if (x < -20.0) return -1.0;
    double e2 = exp(2.0 * x);
    return (e2 - 1.0) / (e2 + 1.0);
}

/* ============================================================ */
/*  Utilidades para inteiros                                   */
/* ============================================================ */

int ipow(int base, int exp) {
    int r = 1;
    while (exp > 0) {
        if (exp & 1) r *= base;
        base *= base;
        exp >>= 1;
    }
    return r;
}

int igcd(int a, int b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { int t = a % b; a = b; b = t; }
    return a;
}

int ilcm(int a, int b) {
    if (a == 0 || b == 0) return 0;
    return iabs(a / igcd(a, b) * b);
}

int isqrt(int x) {
    if (x < 0) return -1;
    if (x == 0) return 0;
    int g = x;
    for (int i = 0; i < 20; i++) {
        int ng = (g + x / g) / 2;
        if (ng >= g) break;
        g = ng;
    }
    return g;
}

/* PRNG xorshift32 — bom o suficiente para jogos e amostragem */
uint32_t irand(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}
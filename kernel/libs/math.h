/*-----------------------------------------------------------------------*/
/* math.h - Biblioteca matematica minima para o UpdateOS                 */
/*                                                                       */
/* Substitui a <math.h> da libc, que nao esta disponivel em ambiente     */
/* freestanding. Inclui constantes, macros utilitarias e implementacoes  */
/* das funcoes mais usadas em kernels: floor/ceil/round, sqrt, pow,      */
/* exp, log, sin/cos/tan, atan2, fmod, etc.                              */
/*-----------------------------------------------------------------------*/
#ifndef KERNEL_MATH_H
#define KERNEL_MATH_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================ */
/*  Constantes                                                 */
/* ============================================================ */
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_2_PI      0.63661977236758134308
#define M_1_PI      0.31830988618379067154
#define M_E         2.71828182845904523536
#define M_LN2       0.69314718055994530942
#define M_LN10      2.30258509299404568402
#define M_LOG2E     1.44269504088896340736
#define M_LOG10E    0.43429448190325182765
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440

/* ============================================================ */
/*  Macros utilitarias                                         */
/* ============================================================ */

#define MATH_PI      M_PI
#define MATH_TAU     (M_PI * 2.0)
#define DEG_TO_RAD(d) ((d) * M_PI / 180.0)
#define RAD_TO_DEG(r) ((r) * 180.0 / M_PI)

/* ============================================================ */
/*  Helpers inline — inteiros                                  */
/* ============================================================ */

static inline int    imin (int a, int b)              { return a < b ? a : b; }
static inline int    imax (int a, int b)              { return a > b ? a : b; }
static inline int    iabs (int x)                     { return x < 0 ? -x : x; }
static inline int    iclamp(int v, int lo, int hi)    { return v < lo ? lo : (v > hi ? hi : v); }

static inline long   lmin (long a, long b)            { return a < b ? a : b; }
static inline long   lmax (long a, long b)            { return a > b ? a : b; }
static inline long   labs_ (long x)                   { return x < 0 ? -x : x; }
static inline long   lclamp(long v, long lo, long hi) { return v < lo ? lo : (v > hi ? hi : v); }

static inline int    iabs_diff(int a, int b)          { return a > b ? a - b : b - a; }

/* ============================================================ */
/*  Helpers inline — ponto flutuante                           */
/* ============================================================ */

static inline double fmin   (double a, double b)              { return a < b ? a : b; }
static inline double fmax   (double a, double b)              { return a > b ? a : b; }
static inline double fclamp (double v, double lo, double hi)  { return v < lo ? lo : (v > hi ? hi : v); }
static inline double fsign  (double x)                        { return x < 0.0 ? -1.0 : (x > 0.0 ? 1.0 : 0.0); }
static inline double fsqr   (double x)                        { return x * x; }
static inline double fcube  (double x)                        { return x * x * x; }

static inline int    feq    (double a, double b, double eps)  { return fabs(a - b) < eps; }

/* fabs como macro: evita conflito com o builtin do GCC.
   __builtin_fabs gera uma unica instrucao (fabs em x87 ou andps em SSE). */
#ifndef fabs
#  define fabs(x) __builtin_fabs(x)
#endif

/* Comparacoes / classificacao (usam builtins do compilador) */
#define isnan(x)     __builtin_isnan(x)
#define isinf(x)     __builtin_isinf(x)
#define isfinite(x)  __builtin_isfinite(x)
#define signbit(x)   __builtin_signbit(x)

#define MATH_INF  (__builtin_inff())
#define MATH_NAN  (__builtin_nanf(""))

/* ============================================================ */
/*  Funcoes de arredondamento e resto                          */
/* ============================================================ */
double floor (double x);
double ceil  (double x);
double trunc (double x);
double round (double x);
double fmod  (double x, double y);
double modf  (double x, double *iptr);

/* ============================================================ */
/*  Funcoes elementares                                        */
/* ============================================================ */
double sqrt (double x);
double cbrt (double x);
double pow  (double x, double y);
double exp  (double x);
double exp2 (double x);
double log  (double x);
double log2 (double x);
double log10(double x);

/* ============================================================ */
/*  Trigonometria                                              */
/* ============================================================ */
double sin  (double x);
double cos  (double x);
double tan  (double x);
double asin (double x);
double acos (double x);
double atan (double x);
double atan2(double y, double x);
double sinh (double x);
double cosh (double x);
double tanh (double x);

/* ============================================================ */
/*  Utilidades para inteiros (implementadas em math.c)         */
/* ============================================================ */
int    ipow (int base, int exp);         /* base^exp para exp >= 0      */
int    igcd (int a, int b);              /* MDC Euclidiano              */
int    ilcm (int a, int b);              /* MMC                         */
int    isqrt(int x);                     /* raiz quadrada inteira       */
uint32_t irand(uint32_t *state);         /* PRNG xorshift simples       */

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_MATH_H */
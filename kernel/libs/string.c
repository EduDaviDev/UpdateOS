#include "string.h"

/* ============================================================
 *  Memória
 * ============================================================ */

void *memset(void *dest, int c, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    uint8_t  v = (uint8_t)c;
    while (n--) {
        *d++ = v;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t       *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t       *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || n == 0) {
        return dest;
    }

    if (d < s) {
        /* Copia para frente */
        while (n--) {
            *d++ = *s++;
        }
    } else {
        /* Copia para trás (overlap) */
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dest;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    while (n--) {
        if (*pa != *pb) {
            return (int)*pa - (int)*pb;
        }
        pa++;
        pb++;
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n) {
    const uint8_t *p = (const uint8_t *)s;
    uint8_t target = (uint8_t)c;
    while (n--) {
        if (*p == target) {
            return (void *)p;
        }
        p++;
    }
    return NULL;
}

/* ============================================================
 *  Strings
 * ============================================================ */

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

size_t strnlen(const char *s, size_t maxlen) {
    size_t len = 0;
    while (len < maxlen && s[len]) {
        len++;
    }
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++)) {
        /* vazio */
    }
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest + strlen(dest);
    while ((*d++ = *src++)) {
        /* vazio */
    }
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest + strlen(dest);
    size_t i = 0;
    for (; i < n && src[i]; i++) {
        d[i] = src[i];
    }
    d[i] = '\0';
    return dest;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

char *strchr(const char *s, int c) {
    char target = (char)c;
    while (*s) {
        if (*s == target) {
            return (char *)s;
        }
        s++;
    }
    return (target == '\0') ? (char *)s : NULL;
}

char *strrchr(const char *s, int c) {
    char  target = (char)c;
    char *last   = NULL;
    while (*s) {
        if (*s == target) {
            last = (char *)s;
        }
        s++;
    }
    if (target == '\0') {
        return (char *)s;
    }
    return last;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) {
        return (char *)haystack;
    }
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        if (!*n) {
            return (char *)haystack;
        }
    }
    return NULL;
}

/* ============================================================
 *  Conversão
 * ============================================================ */

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' ||
           c == '\r' || c == '\f' || c == '\v';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int atoi(const char *s) {
    return (int)atol(s);
}

long atol(const char *s) {
    long result = 0;
    int  sign   = 1;

    while (is_space(*s)) {
        s++;
    }

    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    while (is_digit(*s)) {
        result = result * 10 + (*s - '0');
        s++;
    }

    return sign * result;
}

/* ============================================================
 *  Extras
 * ============================================================ */

void strrev(char *s) {
    size_t len = strlen(s);
    if (len < 2) {
        return;
    }
    size_t i = 0;
    size_t j = len - 1;
    while (i < j) {
        char tmp = s[i];
        s[i]     = s[j];
        s[j]     = tmp;
        i++;
        j--;
    }
}

static const char digits_lower[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char digits_upper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void utoa(uint32_t value, char *buf, int base) {
    if (base < 2 || base > 36) {
        buf[0] = '\0';
        return;
    }
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    char *p = buf;
    while (value) {
        *p++ = digits_lower[value % base];
        value /= base;
    }
    *p = '\0';
    strrev(buf);
}

void utoa64(uint64_t value, char *buf, int base) {
    if (base < 2 || base > 36) {
        buf[0] = '\0';
        return;
    }
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    char *p = buf;
    while (value) {
        *p++ = digits_lower[value % base];
        value /= base;
    }
    *p = '\0';
    strrev(buf);
}

void itoa(int value, char *buf, int base) {
    if (base < 2 || base > 36) {
        buf[0] = '\0';
        return;
    }

    /* Caso especial: base 10 negativo */
    if (value < 0 && base == 10) {
        *buf++ = '-';
        /* Evita overflow em INT_MIN */
        uint32_t uval = (uint32_t)(-(int64_t)value);
        utoa(uval, buf, base);
        return;
    }

    utoa((uint32_t)value, buf, base);
}

int streq(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *a == *b;
}
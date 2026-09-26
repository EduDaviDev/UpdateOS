#ifndef KERNEL_RTC_H
#define KERNEL_RTC_H

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t		DWORD;	/* 32-bit unsigned */

/* Data/hora "humana" (1-based no mês e dia) */
typedef struct {
    uint16_t year;   /* 2000–2099 */
    uint8_t  month;  /* 1–12 */
    uint8_t  day;    /* 1–31 */
    uint8_t  hour;   /* 0–23 */
    uint8_t  minute; /* 0–59 */
    uint8_t  second; /* 0–59 */
} rtc_time_t;

/* Inicialização (opcional — só marca "pronto") */
void rtc_init(void);

/* Leitura/escrita da hora do CMOS */
void rtc_get_time(rtc_time_t *t);
void rtc_set_time(const rtc_time_t *t);

/* Atalho: hora em segundos desde 01/01/2000 (uso interno / debug) */
uint32_t rtc_unix_like(void);

/* Função que o FatFS espera quando FF_FS_NORTC == 0 */
#if !FF_FS_NORTC
DWORD get_fattime(void);
#endif

#endif /* KERNEL_RTC_H */
/*-----------------------------------------------------------------------*/
/* rtc.c - Relógio de tempo real (CMOS) para o UpdateOS                  */
/*                                                                       */
/* Acessa o chip CMOS via portas 0x70 (índice) e 0x71 (dados).           */
/* Trata modos BCD/binário, 12/24h e faz dupla leitura para evitar       */
/* leituras inconsistentes durante a virada de segundo.                  */
/*-----------------------------------------------------------------------*/
#include "rtc.h"
#include "io.h"

/* Registradores CMOS */
#define CMOS_ADDR   0x70
#define CMOS_DATA   0x71

#define RTC_SEC     0x00
#define RTC_MIN     0x02
#define RTC_HOUR    0x04
#define RTC_WDAY    0x06
#define RTC_DAY     0x07
#define RTC_MON     0x08
#define RTC_YEAR    0x09
#define RTC_CENTURY 0x32   /* opcional, só em alguns chips */
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

/* Bits de STATUS_B */
#define RTC_B_24H   0x02   /* 1 = modo 24h */
#define RTC_B_BIN   0x04   /* 1 = binário, 0 = BCD */

/* ------------------------------------------------------------------ */
static inline uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

static inline void cmos_write(uint8_t reg, uint8_t val) {
    outb(CMOS_ADDR, reg);
    outb(CMOS_DATA, val);
}

static inline int update_in_progress(void) {
    return cmos_read(RTC_STATUS_A) & 0x80;
}

/* Converte BCD -> binário */
static inline uint8_t bcd_to_bin(uint8_t b) {
    return (uint8_t)((b & 0x0F) + ((b >> 4) * 10));
}

/* Converte binário -> BCD */
static inline uint8_t bin_to_bcd(uint8_t b) {
    return (uint8_t)(((b / 10) << 4) | (b % 10));
}

/* ------------------------------------------------------------------ */
void rtc_init(void) {
    /* Nada obrigatório — o CMOS já está ligado desde o boot.
       Deixamos a função para simetria com os outros drivers. */
}

/* ------------------------------------------------------------------ */
void rtc_get_time(rtc_time_t *t) {
    uint8_t sec, min, hour, day, mon, year, cen;
    uint8_t status_b;

    /* Espera uma atualização terminar para ler valores estáveis */
    while (update_in_progress()) { /* spin */ }

    status_b = cmos_read(RTC_STATUS_B);

    /* Dupla leitura: se os valores mudarem entre as duas, repete */
    do {
        sec  = cmos_read(RTC_SEC);
        min  = cmos_read(RTC_MIN);
        hour = cmos_read(RTC_HOUR);
        day  = cmos_read(RTC_DAY);
        mon  = cmos_read(RTC_MON);
        year = cmos_read(RTC_YEAR);
        cen  = cmos_read(RTC_CENTURY);
    } while (sec != cmos_read(RTC_SEC));

    /* Se NÃO estiver em modo binário, converte de BCD */
    if (!(status_b & RTC_B_BIN)) {
        sec  = bcd_to_bin(sec);
        min  = bcd_to_bin(min);
        hour = bcd_to_bin(hour & 0x7F);   /* tira o bit PM */
        day  = bcd_to_bin(day);
        mon  = bcd_to_bin(mon);
        year = bcd_to_bin(year);
        cen  = bcd_to_bin(cen);
    }

    /* Corrige PM no modo 12h */
    if (!(status_b & RTC_B_24H)) {
        int pm = cmos_read(RTC_HOUR) & 0x80;
        if (pm && hour < 12) hour = (uint8_t)(hour + 12);
        if (!pm && hour == 12) hour = 0;
    }

    t->second = sec;
    t->minute = min;
    t->hour   = hour;
    t->day    = day;
    t->month  = mon;

    /* Século: alguns BIOS guardam em 0x32, outros não.
       Se vier 0 ou valor absurdo, assume 20xx. */
    if (cen >= 19 && cen <= 21) {
        t->year = (uint16_t)(cen * 100 + year);
    } else {
        t->year = (uint16_t)(2000 + year);
    }
}

/* ------------------------------------------------------------------ */
void rtc_set_time(const rtc_time_t *t) {
    uint8_t status_b = cmos_read(RTC_STATUS_B);
    int use_bcd = !(status_b & RTC_B_BIN);

    uint8_t sec  = t->second;
    uint8_t min  = t->minute;
    uint8_t hour = t->hour;
    uint8_t day  = t->day;
    uint8_t mon  = t->month;
    uint8_t year = (uint8_t)(t->year % 100);
    uint8_t cen  = (uint8_t)(t->year / 100);

    if (use_bcd) {
        sec  = bin_to_bcd(sec);
        min  = bin_to_bcd(min);
        hour = bin_to_bcd(hour);
        day  = bin_to_bcd(day);
        mon  = bin_to_bcd(mon);
        year = bin_to_bcd(year);
        cen  = bin_to_bcd(cen);
    }

    /* Desabilita NMI durante a escrita (bit 7 do registrador de índice) */
    outb(CMOS_ADDR, 0x80 | RTC_SEC);  outb(CMOS_DATA, sec);
    outb(CMOS_ADDR, 0x80 | RTC_MIN);  outb(CMOS_DATA, min);
    outb(CMOS_ADDR, 0x80 | RTC_HOUR); outb(CMOS_DATA, hour);
    outb(CMOS_ADDR, 0x80 | RTC_DAY);  outb(CMOS_DATA, day);
    outb(CMOS_ADDR, 0x80 | RTC_MON);  outb(CMOS_DATA, mon);
    outb(CMOS_ADDR, 0x80 | RTC_YEAR); outb(CMOS_DATA, year);
    outb(CMOS_ADDR, 0x80 | RTC_CENTURY); outb(CMOS_DATA, cen);
}

/* ------------------------------------------------------------------ */
uint32_t rtc_unix_like(void) {
    rtc_time_t t;
    rtc_get_time(&t);
    /* Segundos aproximados desde 01/01/2000 (ignora anos bissextos) */
    uint32_t days = (uint32_t)(t.year - 2000) * 365
                  + (uint32_t)(t.month - 1) * 30
                  + (t.day - 1);
    return days * 86400u
         + (uint32_t)t.hour * 3600u
         + (uint32_t)t.minute * 60u
         + t.second;
}

/* ------------------------------------------------------------------ */
/* Integração com o FatFS: chamada quando FF_FS_NORTC == 0            */
/* Formato FAT: bits 31-25 ano (desde 1980), 24-21 mês, 20-16 dia,    */
/*              15-11 hora, 10-5 minuto, 4-0 segundo/2                 */
/* ------------------------------------------------------------------ */
#if !FF_FS_NORTC
DWORD get_fattime(void)
{
    rtc_time_t t;
    rtc_get_time(&t);

    /* Ano: clamp para o intervalo representavel em 7 bits */
    uint32_t year = t.year;
    if (year < 1980) year = 1980;
    if (year > 2107) year = 2107;
    year -= 1980;

    /* Clamp dos demais campos para evitar lixo caso o CMOS retorne algo estranho */
    uint32_t month  = (t.month  < 1) ? 1  : (t.month  > 12) ? 12 : t.month;
    uint32_t day    = (t.day    < 1) ? 1  : (t.day    > 31) ? 31 : t.day;
    uint32_t hour   = (t.hour   > 23) ? 23 : t.hour;
    uint32_t minute = (t.minute > 59) ? 59 : t.minute;

    /* FAT guarda segundos em passos de 2 */
    uint32_t second = t.second / 2;
    if (second > 29) second = 29;

    return ((year   & 0x7F) << 25)
         | ((month  & 0x0F) << 21)
         | ((day    & 0x1F) << 16)
         | ((hour   & 0x1F) << 11)
         | ((minute & 0x3F) <<  5)
         |  (second & 0x1F);
}
#endif /* !FF_FS_NORTC */
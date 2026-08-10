#include "rtc.h"
#include "io.h"
#include <stdint.h>

// Portas padrão da CMOS
#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

// Registradores da CMOS
#define RTC_SECOND    0x00
#define RTC_MINUTE    0x02
#define RTC_HOUR      0x04
#define RTC_DAY       0x07
#define RTC_MONTH     0x08
#define RTC_YEAR      0x09
#define RTC_STATUS_A  0x0A
#define RTC_STATUS_B  0x0B

// Lê um registrador da CMOS
uint8_t rtc_read_register(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

// Escreve em um registrador da CMOS
void rtc_write_register(uint8_t reg, uint8_t data) {
    outb(CMOS_ADDR, reg);
    outb(CMOS_DATA, data);
}

// Converte BCD para binário
static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

// Converte binário para BCD
static uint8_t bin_to_bcd(uint8_t bin) {
    return ((bin / 10) << 4) | (bin % 10);
}

// Obtém a data/hora atual
void rtc_get_time(rtc_time_t *time) {
    // Verifica se o RTC está em formato BCD (geralmente sim)
    uint8_t status_b = rtc_read_register(RTC_STATUS_B);
    int is_bcd = !(status_b & 0x04);

    // Lê os registradores (espera um pouquinho para garantir consistência)
    uint8_t second = rtc_read_register(RTC_SECOND);
    uint8_t minute = rtc_read_register(RTC_MINUTE);
    uint8_t hour   = rtc_read_register(RTC_HOUR);
    uint8_t day    = rtc_read_register(RTC_DAY);
    uint8_t month  = rtc_read_register(RTC_MONTH);
    uint8_t year   = rtc_read_register(RTC_YEAR);

    // Converte se estiver em BCD
    if (is_bcd) {
        time->second = bcd_to_bin(second);
        time->minute = bcd_to_bin(minute);
        time->hour   = bcd_to_bin(hour);
        time->day    = bcd_to_bin(day);
        time->month  = bcd_to_bin(month);
        time->year   = bcd_to_bin(year);
    } else {
        time->second = second;
        time->minute = minute;
        time->hour   = hour;
        time->day    = day;
        time->month  = month;
        time->year   = year;
    }

    // Ajusta o ano (assumindo século 2000)
    time->year += 2000;
}

// Define a data/hora
void rtc_set_time(rtc_time_t *time) {
    // Verifica se o RTC está em formato BCD
    uint8_t status_b = rtc_read_register(RTC_STATUS_B);
    int is_bcd = !(status_b & 0x04);

    // Desabilita atualizações durante a escrita
    uint8_t status_a = rtc_read_register(RTC_STATUS_A);
    rtc_write_register(RTC_STATUS_A, status_a | 0x80);

    // Escreve os registradores
    if (is_bcd) {
        rtc_write_register(RTC_SECOND, bin_to_bcd(time->second));
        rtc_write_register(RTC_MINUTE, bin_to_bcd(time->minute));
        rtc_write_register(RTC_HOUR,   bin_to_bcd(time->hour));
        rtc_write_register(RTC_DAY,    bin_to_bcd(time->day));
        rtc_write_register(RTC_MONTH,  bin_to_bcd(time->month));
        rtc_write_register(RTC_YEAR,   bin_to_bcd(time->year - 2000));
    } else {
        rtc_write_register(RTC_SECOND, time->second);
        rtc_write_register(RTC_MINUTE, time->minute);
        rtc_write_register(RTC_HOUR,   time->hour);
        rtc_write_register(RTC_DAY,    time->day);
        rtc_write_register(RTC_MONTH,  time->month);
        rtc_write_register(RTC_YEAR,   time->year - 2000);
    }

    // Reabilita atualizações
    rtc_write_register(RTC_STATUS_A, status_a);
}

uint32_t get_fattime(void) {
    rtc_time_t now;
    rtc_get_time(&now);

    // Formato: bit 25-31: ano (1980..2107), bit 21-24: mês, bit 16-20: dia,
    // bit 11-15: hora, bit 5-10: minuto, bit 0-4: segundo/2
    return ((uint32_t)(now.year - 1980) << 25) |
           ((uint32_t)now.month << 21) |
           ((uint32_t)now.day << 16) |
           ((uint32_t)now.hour << 11) |
           ((uint32_t)now.minute << 5) |
           ((uint32_t)(now.second / 2));
}
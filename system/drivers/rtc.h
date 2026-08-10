#ifndef RTC_H
#define RTC_H

#include <stdint.h>

// Estrutura para armazenar a data e hora
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

// Funções principais
uint8_t rtc_read_register(uint8_t reg);
void rtc_write_register(uint8_t reg, uint8_t data);
void rtc_get_time(rtc_time_t *time);
void rtc_set_time(rtc_time_t *time);

#endif
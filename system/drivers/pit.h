#ifndef PIT_H
#define PIT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TICKS,
    MS,
    SEC,
    MIN,
    HR,
    DAY,
    WK,
    MO,
    YR
} TimeUnits;

typedef struct Timer {
    float rem_time;          // tempo restante na unidade rt_unit
    TimeUnits rt_unit;

    float past_time;         // tempo decorrido na unidade pt_unit
    TimeUnits pt_unit;

    float initial_time;      // tempo inicial na unidade initial_unit
    TimeUnits initial_unit;

    bool infinite;           // se true, reseta e chama callback ao final
    bool paused;

    void (*return_function)(void);  // callback quando o timer expira

    // Internos
    uint32_t start_tick;     // tick do PIT quando iniciado
    uint32_t elapsed_ticks;  // ticks decorridos (acumulado)
    bool active;
    bool expired;
} Timer;

// Inicializa o PIT (configura taxa de 1000 Hz)
void pit_init(void);

// Obtém ticks desde o boot (em unidades de 1ms)
uint32_t pit_get_ticks(void);
// Obtém ticks em milissegundos (equivalente)
uint32_t pit_get_ticks_ms(void);

// Espera bloqueante (apenas na thread atual) em unidades especificadas
void pit_wait(float time, TimeUnits unit);

// Gerenciamento de timers
void pit_register_timer(Timer *timer);
void pit_start_timer(Timer *timer);
void pit_reset_timer(Timer *timer);
void pit_stop_timer(Timer *timer);

// Deve ser chamada na ISR do PIT (IRQ0)
void pit_tick_handler(void);

#endif
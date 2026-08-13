#include "pit.h"
#include "../libs/io.h"
#include "../interrupt/pic.h"
#include <stddef.h>
#include <string.h>

#define PIT_FREQ 1000  // 1000 Hz = 1ms por tick

// Registradores do PIT
#define PIT_CMD  0x43
#define PIT_CH0  0x40

static uint32_t ticks = 0;           // contador de ticks (milissegundos)
static Timer *timers[16];            // lista de timers registrados (máx 16)
static int timer_count = 0;

// Converte tempo e unidade para milissegundos
static uint32_t to_ms(float time, TimeUnits unit) {
    switch (unit) {
        case TICKS: return (uint32_t)time;  // já em ms
        case MS:    return (uint32_t)time;
        case SEC:   return (uint32_t)(time * 1000);
        case MIN:   return (uint32_t)(time * 60000);
        case HR:    return (uint32_t)(time * 3600000);
        case DAY:   return (uint32_t)(time * 86400000);
        case WK:    return (uint32_t)(time * 604800000);
        case MO:    return (uint32_t)(time * 2592000000); // 30 dias
        case YR:    return (uint32_t)(time * 31536000000); // 365 dias
        default:    return (uint32_t)time;
    }
}

// Inicializa PIT (configura para 1000 Hz)
void pit_init(void) {
    uint32_t divisor = 1193182 / PIT_FREQ;
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);
    ticks = 0;
    timer_count = 0;
    for (int i = 0; i < 16; i++) timers[i] = NULL;
}

uint32_t pit_get_ticks(void) {
    return ticks;
}

uint32_t pit_get_ticks_ms(void) {
    return ticks;  // ticks já são milissegundos
}

// Espera bloqueante (busy loop)
void pit_wait(float time, TimeUnits unit) {
    uint32_t ms = to_ms(time, unit);
    uint32_t start = ticks;
    while ((ticks - start) < ms) {
        // busy wait
        __asm__ volatile ("pause");  // hint para eficiência
    }
}

// Registra um timer na lista
void pit_register_timer(Timer *timer) {
    if (timer_count < 16) {
        timers[timer_count++] = timer;
        timer->active = false;
        timer->expired = false;
    }
}

// Inicia um timer
void pit_start_timer(Timer *timer) {
    if (!timer) return;
    timer->start_tick = ticks;
    timer->elapsed_ticks = 0;
    timer->active = true;
    timer->expired = false;
    timer->paused = false;
    // Converte rem_time para ms
    timer->rem_time = (float)to_ms(timer->rem_time, timer->rt_unit);
    timer->rt_unit = MS;  // unifica em ms
    // Se for infinito, rem_time fica como 0 (tratamento especial)
    if (timer->infinite) {
        timer->rem_time = 0;
    }
}

// Reseta um timer (para o estado inicial)
void pit_reset_timer(Timer *timer) {
    if (!timer) return;
    timer->active = false;
    timer->expired = false;
    timer->paused = false;
    timer->elapsed_ticks = 0;
    // Restaura rem_time a partir do inicial
    timer->rem_time = (float)to_ms(timer->initial_time, timer->initial_unit);
    timer->rt_unit = MS;
}

// Para um timer
void pit_stop_timer(Timer *timer) {
    if (!timer) return;
    timer->active = false;
    timer->paused = true;
}

// Handler do PIT (chamado na IRQ0)
void pit_tick_handler(void) {
    ticks++;

    // Atualiza timers ativos
    for (int i = 0; i < timer_count; i++) {
        Timer *t = timers[i];
        if (!t || !t->active || t->paused) continue;

        t->elapsed_ticks++;
        if (t->infinite) {
            // Não expira, apenas chama callback periodicamente? Não, no enunciado diz: "ao acabar o tempo ele chama a função e reseta o timer".
            // Então para infinito, ele nunca "acaba", então não chamamos callback automaticamente.
            // Mas podemos chamar a cada período? Vamos ignorar por enquanto.
            continue;
        }

        // Verifica se expirou
        if (t->elapsed_ticks >= (uint32_t)t->rem_time) {
            t->active = false;
            t->expired = true;
            if (t->return_function) {
                t->return_function();
            }
            // Se for infinito, reseta? Não, pois não expira.
        }
    }
}

// Callback para o PIT (deve ser chamado na ISR)
// Já temos o timer_handler em irq.c, vamos substituir para chamar pit_tick_handler.
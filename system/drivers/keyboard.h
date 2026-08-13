#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "keydef.h"

// Estrutura combinada de modificadores
typedef struct {
    bool ctrl;
    bool shift;
    bool alt;
    bool sys;   // Windows / Command
} KeyModifiers;

// Estrutura de filtro de caracteres
typedef struct {
    bool nschar;   // Not Special Char (qualquer caractere imprimível)
    bool numchar;  // 0-9
    bool hexchar;  // 0-9, A-F, a-f
    bool spchar;   // Caracteres especiais: !@#$%^&*()_+-= etc.
    // Pode adicionar mais se quiser
} KeyFilter;

// Estrutura KeyEvent completa
typedef struct {
    char character;         // caractere ASCII (se houver)
    uint8_t scancode;       // scancode bruto
    KeyCodes keycode;       // código da tecla (keydef.h)
    KeyModifiers mods;      // modificadores pressionados no momento
    KeyFilter filter;       // filtros para o caractere (preenchido automaticamente)
    bool pressed;           // evento de pressionamento
    bool released;          // evento de soltura
    bool clicked;           // pressionado por pelo menos 300ms e solto (requer PIT)
    bool extended;          // se é scancode estendido (0xE0)
} KeyEvent;

// Funções públicas
void kbd_init(void);
KeyEvent kbd_gevent(void);     // obtém evento (não bloqueante)
KeyEvent kbd_ogevent(void);    // obtém evento com output (se houver, bloqueia até tecla)

#endif
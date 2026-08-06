#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "keydef.h"

typedef struct {
    char* Char;        // caractere ASCII ou NULL
    KeyCodes Code;     // código da tecla (keydef.h)
    uint8_t ScanCode;  // scan code bruto
    bool Ctrl;
    bool Shift;
    bool Alt;
    bool Sys;          // Windows/Super
    bool pressed;      // true se foi pressionada neste evento
    bool released;     // true se foi solta neste evento
    bool clicked;      // (não implementado)
} KeyEvent;

void keyboard_init(void);
KeyEvent keyboard_read(void); // NÃO bloqueante: retorna evento vazio se não houver tecla

#endif

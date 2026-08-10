#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "keydef.h"

typedef struct {
    char Char;          // caractere ASCII ('\0' se não mapeado)
    KeyCodes Code;      // código da tecla (keydef.h)
    uint8_t ScanCode;   // scan code bruto
    bool Ctrl;
    bool Shift;
    bool Alt;
    bool Sys;
    bool pressed;
    bool released;
    bool clicked;
} KeyEvent;

void keyboard_init(void);
KeyEvent keyboard_read(void); // não bloqueante

#endif
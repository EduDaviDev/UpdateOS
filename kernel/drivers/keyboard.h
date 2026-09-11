#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "keycodes.h"

extern bool kbd_has_event;
extern bool kbd_initialized;

typedef enum {
	// key pressed
	KS_PRESSED,
	// key released
	KS_RELEASED,
	// key pressed and released (future)
	KS_CLICKED,
} KBD_KeyState_t;

typedef struct {
	bool printable, control, special;
	bool alphanumeric, alphabetic, numeric;
	bool ascii_e; // ASCII-Extended
} KBD_CharFilers;

typedef struct {
	// Key Code
	KBD_KeyCodes_t keycode;
	// Key Character
	char character;
	// Key Scancode
	uint8_t scancode;

	// Key State
	KBD_KeyState_t state;
	// Key Character Filters
	KBD_CharFilers filters;
	// Key Modifiers
	bool control, shift, alt;
	bool sys; // Windows Key
} KBD_Event_t;

void			kbd_init();
KBD_Event_t		kbd_gete();
char			kbd_getc();
uint8_t			kbd_getsc();

#endif /* DRIVERS_KEYBOARD_H */
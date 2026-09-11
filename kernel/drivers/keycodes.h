#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    KEY_NONE,

    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E,
    KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,
    KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y,
    KEY_Z,

    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,
    KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,

    KEY_TILDE,        // ~ / `
    KEY_COMMA,        // , / <
    KEY_DOT,          // . / >
    KEY_SEMICOLON,    // ; / :
    KEY_QUOTE,        // ' / "
    KEY_DASH,         // - / _
    KEY_EQUAL,        // = / +
    KEY_LBRACKET,     // [ / {
    KEY_RBRACKET,     // ] / }
    KEY_BACKSLASH,    // \ / |
    KEY_SLASH,        // / / ?

    KEY_BACKSPACE,
    KEY_SPACE,
    KEY_RETURN,
    KEY_TAB,
    KEY_CAPS,
    KEY_ESC,
    KEY_DELETE,

    KEY_LSHIFT, KEY_RSHIFT,
    KEY_LCTRL,  KEY_RCTRL,
    KEY_LALT,   KEY_RALT,
    KEY_LSYSTEM, KEY_RSYSTEM,

    KEY_MENU,
    KEY_PRINT,
    KEY_SCROLL_LOCK,
    KEY_PAUSE,
    KEY_INSERT,
    KEY_HOME,
    KEY_END,
    KEY_PAGEUP,
    KEY_PAGEDOWN,

    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,

    KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5,  KEY_F6,
    KEY_F7,  KEY_F8,  KEY_F9,  KEY_F10, KEY_F11, KEY_F12,
    KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18,
    KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24,

    KEY_KP_0, KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4,
    KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9,
    KEY_KP_DIVIDE,
    KEY_KP_MULTIPLY,
    KEY_KP_SUBTRACT,
    KEY_KP_ADD,
    KEY_KP_ENTER,
    KEY_KP_DOT,
    KEY_NUM_LOCK,

    KEY_MEDIA_PLAY_PAUSE,
    KEY_MEDIA_STOP,
    KEY_MEDIA_NEXT,
    KEY_MEDIA_PREVIOUS,
    KEY_VOLUME_UP,
    KEY_VOLUME_DOWN,
    KEY_VOLUME_MUTE,

    KEY_NON_US_SLASH,
    KEY_KP_COMMA,

    KEY_POWER,
    KEY_SLEEP,
    KEY_WAKE,
    KEY_BATTERY_LOW,
    KEY_SCREEN_SAVER,

    KEY_BROWSER_BACK,
    KEY_BROWSER_FORWARD,
    KEY_BROWSER_REFRESH,
    KEY_BROWSER_STOP,
    KEY_BROWSER_SEARCH,
    KEY_BROWSER_FAVORITES,
    KEY_BROWSER_HOME,

    KEY_KP_EQUAL,
    KEY_KP_LEFT_PAREN,
    KEY_KP_RIGHT_PAREN,
    KEY_CLEAR,

    KEY_CALCULATOR,
    KEY_LAUNCH_MAIL,
    KEY_LAUNCH_MEDIA_PLAYER,
    KEY_EXPLORER,
    KEY_HIBERNATE,
    KEY_APP_HELP,

    KEY_M1, KEY_M2, KEY_M3, KEY_M4, KEY_M5,
    KEY_GAME_MODE,
    KEY_PROFILE_SWITCH,

    KEY_VOICE_COMMAND,
    KEY_EMOJI_PICKER,

    KEY_SYSREQ,
    KEY_ATTN,
    KEY_CANCEL
} KBD_KeyCodes_t;

typedef struct {
    char char_map[256];
    char char_ext_map[256];

    char char_shift_map[256];
    char char_shift_ext_map[256];

    KBD_KeyCodes_t keymap[256];
    KBD_KeyCodes_t extmap[256];
} KBD_ScanSet_t;

/* ================================================================
 *  Scan Code Set 1
 *  Referência: https://wiki.osdev.org/PS/2_Keyboard
 *  Break codes têm bit 7 (0x80) setado. Prefixo E0 = estendido.
 * ================================================================ */
static const KBD_ScanSet_t scancode_set1 = {

    /* --- char_map: scancode -> caractere (sem Shift) --- */
    .char_map = {
        [0x01] = 27,        /* ESC */
        [0x02] = '1',  [0x03] = '2',  [0x04] = '3',  [0x05] = '4',
        [0x06] = '5',  [0x07] = '6',  [0x08] = '7',  [0x09] = '8',
        [0x0A] = '9',  [0x0B] = '0',
        [0x0C] = '-',  [0x0D] = '=',
        [0x0E] = '\b', [0x0F] = '\t',
        [0x10] = 'q',  [0x11] = 'w',  [0x12] = 'e',  [0x13] = 'r',
        [0x14] = 't',  [0x15] = 'y',  [0x16] = 'u',  [0x17] = 'i',
        [0x18] = 'o',  [0x19] = 'p',
        [0x1A] = '[',  [0x1B] = ']',
        [0x1C] = '\n',
        [0x1E] = 'a',  [0x1F] = 's',  [0x20] = 'd',  [0x21] = 'f',
        [0x22] = 'g',  [0x23] = 'h',  [0x24] = 'j',  [0x25] = 'k',
        [0x26] = 'l',
        [0x27] = ';',  [0x28] = '\'', [0x29] = '`',
        [0x2B] = '\\',
        [0x2C] = 'z',  [0x2D] = 'x',  [0x2E] = 'c',  [0x2F] = 'v',
        [0x30] = 'b',  [0x31] = 'n',  [0x32] = 'm',
        [0x33] = ',',  [0x34] = '.',  [0x35] = '/',
        [0x37] = '*',
        [0x39] = ' ',
    },

    /* --- char_ext_map: E0 + scancode -> caractere (sem Shift) --- */
    .char_ext_map = {
        [0x1C] = '\n',      /* KP Enter */
    },

    /* --- char_shift_map: scancode -> caractere (com Shift) --- */
    .char_shift_map = {
        [0x01] = 27,
        [0x02] = '!',  [0x03] = '@',  [0x04] = '#',  [0x05] = '$',
        [0x06] = '%',  [0x07] = '^',  [0x08] = '&',  [0x09] = '*',
        [0x0A] = '(',  [0x0B] = ')',
        [0x0C] = '_',  [0x0D] = '+',
        [0x0E] = '\b', [0x0F] = '\t',
        [0x10] = 'Q',  [0x11] = 'W',  [0x12] = 'E',  [0x13] = 'R',
        [0x14] = 'T',  [0x15] = 'Y',  [0x16] = 'U',  [0x17] = 'I',
        [0x18] = 'O',  [0x19] = 'P',
        [0x1A] = '{',  [0x1B] = '}',
        [0x1C] = '\n',
        [0x1E] = 'A',  [0x1F] = 'S',  [0x20] = 'D',  [0x21] = 'F',
        [0x22] = 'G',  [0x23] = 'H',  [0x24] = 'J',  [0x25] = 'K',
        [0x26] = 'L',
        [0x27] = ':',  [0x28] = '"',  [0x29] = '~',
        [0x2B] = '|',
        [0x2C] = 'Z',  [0x2D] = 'X',  [0x2E] = 'C',  [0x2F] = 'V',
        [0x30] = 'B',  [0x31] = 'N',  [0x32] = 'M',
        [0x33] = '<',  [0x34] = '>',  [0x35] = '?',
        [0x37] = '*',
        [0x39] = ' ',
    },

    /* --- char_shift_ext_map: E0 + scancode -> caractere (com Shift) --- */
    .char_shift_ext_map = {
        [0x1C] = '\n',
    },

    /* --- keymap: scancode -> KBD_KeyCodes_t (sem prefixo E0) --- */
    .keymap = {
        [0x01] = KEY_ESC,
        [0x02] = KEY_1, [0x03] = KEY_2, [0x04] = KEY_3, [0x05] = KEY_4,
        [0x06] = KEY_5, [0x07] = KEY_6, [0x08] = KEY_7, [0x09] = KEY_8,
        [0x0A] = KEY_9, [0x0B] = KEY_0,
        [0x0C] = KEY_DASH, [0x0D] = KEY_EQUAL,
        [0x0E] = KEY_BACKSPACE, [0x0F] = KEY_TAB,
        [0x10] = KEY_Q, [0x11] = KEY_W, [0x12] = KEY_E, [0x13] = KEY_R,
        [0x14] = KEY_T, [0x15] = KEY_Y, [0x16] = KEY_U, [0x17] = KEY_I,
        [0x18] = KEY_O, [0x19] = KEY_P,
        [0x1A] = KEY_LBRACKET, [0x1B] = KEY_RBRACKET,
        [0x1C] = KEY_RETURN,
        [0x1D] = KEY_LCTRL,
        [0x1E] = KEY_A, [0x1F] = KEY_S, [0x20] = KEY_D, [0x21] = KEY_F,
        [0x22] = KEY_G, [0x23] = KEY_H, [0x24] = KEY_J, [0x25] = KEY_K,
        [0x26] = KEY_L,
        [0x27] = KEY_SEMICOLON, [0x28] = KEY_QUOTE, [0x29] = KEY_TILDE,
        [0x2A] = KEY_LSHIFT,
        [0x2B] = KEY_BACKSLASH,
        [0x2C] = KEY_Z, [0x2D] = KEY_X, [0x2E] = KEY_C, [0x2F] = KEY_V,
        [0x30] = KEY_B, [0x31] = KEY_N, [0x32] = KEY_M,
        [0x33] = KEY_COMMA, [0x34] = KEY_DOT, [0x35] = KEY_SLASH,
        [0x36] = KEY_RSHIFT,
        [0x37] = KEY_KP_MULTIPLY,
        [0x38] = KEY_LALT,
        [0x39] = KEY_SPACE,
        [0x3A] = KEY_CAPS,
        [0x3B] = KEY_F1,  [0x3C] = KEY_F2,  [0x3D] = KEY_F3,
        [0x3E] = KEY_F4,  [0x3F] = KEY_F5,  [0x40] = KEY_F6,
        [0x41] = KEY_F7,  [0x42] = KEY_F8,  [0x43] = KEY_F9,
        [0x44] = KEY_F10,
        [0x45] = KEY_NUM_LOCK,
        [0x46] = KEY_SCROLL_LOCK,
        [0x47] = KEY_KP_7, [0x48] = KEY_KP_8, [0x49] = KEY_KP_9,
        [0x4A] = KEY_KP_SUBTRACT,
        [0x4B] = KEY_KP_4, [0x4C] = KEY_KP_5, [0x4D] = KEY_KP_6,
        [0x4E] = KEY_KP_ADD,
        [0x4F] = KEY_KP_1, [0x50] = KEY_KP_2, [0x51] = KEY_KP_3,
        [0x52] = KEY_KP_0, [0x53] = KEY_KP_DOT,
        [0x57] = KEY_F11, [0x58] = KEY_F12,
    },

    /* --- extmap: E0 + scancode -> KBD_KeyCodes_t --- */
    .extmap = {
        [0x1C] = KEY_KP_ENTER,
        [0x1D] = KEY_RCTRL,
        [0x35] = KEY_KP_DIVIDE,
        [0x38] = KEY_RALT,
        [0x47] = KEY_HOME,
        [0x48] = KEY_UP,
        [0x49] = KEY_PAGEUP,
        [0x4B] = KEY_LEFT,
        [0x4D] = KEY_RIGHT,
        [0x4F] = KEY_END,
        [0x50] = KEY_DOWN,
        [0x51] = KEY_PAGEDOWN,
        [0x52] = KEY_INSERT,
        [0x53] = KEY_DELETE,
        [0x5B] = KEY_LSYSTEM,
        [0x5C] = KEY_RSYSTEM,
        [0x5D] = KEY_MENU,
    },
};

/* ================================================================
 *  Scan Code Set 2
 *  Referência: https://wiki.osdev.org/PS/2_Keyboard
 *  Break code = F0 seguido do make code.
 *  Prefixo E0 = estendido. E1 = sequência Pause.
 * ================================================================ */
static const KBD_ScanSet_t scancode_set2 = {

    /* --- char_map: make code -> caractere (sem Shift) --- */
    .char_map = {
        /* Teclas não-alfanuméricas */
        [0x0D] = '\t',
        [0x0E] = '`',
        [0x16] = '1', [0x1E] = '2', [0x26] = '3', [0x25] = '4',
        [0x2E] = '5', [0x36] = '6', [0x3D] = '7', [0x3E] = '8',
        [0x46] = '9', [0x45] = '0',
        [0x4E] = '-', [0x55] = '=',
        [0x54] = '[', [0x5B] = ']',
        [0x5D] = '\\',
        [0x4C] = ';', [0x52] = '\'', [0x41] = ',', [0x49] = '.',
        [0x4A] = '/',
        [0x66] = '\b',
        [0x29] = ' ',

        /* Linha QWERTY */
        [0x15] = 'q', [0x1D] = 'w', [0x24] = 'e', [0x2D] = 'r',
        [0x2C] = 't', [0x35] = 'y', [0x3C] = 'u', [0x43] = 'i',
        [0x44] = 'o', [0x4D] = 'p',

        /* Linha home */
        [0x1C] = 'a', [0x1B] = 's', [0x23] = 'd', [0x2B] = 'f',
        [0x34] = 'g', [0x33] = 'h', [0x3B] = 'j', [0x42] = 'k',
        [0x4B] = 'l',

        /* Linha inferior */
        [0x1A] = 'z', [0x22] = 'x', [0x21] = 'c', [0x2A] = 'v',
        [0x32] = 'b', [0x31] = 'n', [0x3A] = 'm',

        /* Teclado numérico */
        [0x70] = '0', [0x69] = '1', [0x72] = '2', [0x7A] = '3',
        [0x6B] = '4', [0x73] = '5', [0x74] = '6', [0x6C] = '7',
        [0x75] = '8', [0x7D] = '9', [0x71] = '.',
        [0x7C] = '*',
        [0x7B] = '-',
        [0x79] = '+',
        [0x5A] = '\n',      /* KP Enter */
    },

    /* --- char_ext_map: E0 + make code -> caractere (sem Shift) --- */
    .char_ext_map = {
        [0x5A] = '\n',      /* KP Enter estendido */
        [0x4A] = '/',       /* KP Divide estendido */
    },

    /* --- char_shift_map: make code -> caractere (com Shift) --- */
    .char_shift_map = {
        [0x0D] = '\t',
        [0x0E] = '~',
        [0x16] = '!', [0x1E] = '@', [0x26] = '#', [0x25] = '$',
        [0x2E] = '%', [0x36] = '^', [0x3D] = '&', [0x3E] = '*',
        [0x46] = '(', [0x45] = ')',
        [0x4E] = '_', [0x55] = '+',
        [0x54] = '{', [0x5B] = '}',
        [0x5D] = '|',
        [0x4C] = ':', [0x52] = '"', [0x41] = '<', [0x49] = '>',
        [0x4A] = '?',
        [0x66] = '\b',
        [0x29] = ' ',

        [0x15] = 'Q', [0x1D] = 'W', [0x24] = 'E', [0x2D] = 'R',
        [0x2C] = 'T', [0x35] = 'Y', [0x3C] = 'U', [0x43] = 'I',
        [0x44] = 'O', [0x4D] = 'P',

        [0x1C] = 'A', [0x1B] = 'S', [0x23] = 'D', [0x2B] = 'F',
        [0x34] = 'G', [0x33] = 'H', [0x3B] = 'J', [0x42] = 'K',
        [0x4B] = 'L',

        [0x1A] = 'Z', [0x22] = 'X', [0x21] = 'C', [0x2A] = 'V',
        [0x32] = 'B', [0x31] = 'N', [0x3A] = 'M',

        [0x70] = '0', [0x69] = '1', [0x72] = '2', [0x7A] = '3',
        [0x6B] = '4', [0x73] = '5', [0x74] = '6', [0x6C] = '7',
        [0x75] = '8', [0x7D] = '9', [0x71] = '.',
        [0x7C] = '*', [0x7B] = '-', [0x79] = '+',
        [0x5A] = '\n',
    },

    /* --- char_shift_ext_map --- */
    .char_shift_ext_map = {
        [0x5A] = '\n',
        [0x4A] = '/',
    },

    /* --- keymap: make code -> KBD_KeyCodes_t (Set 2) --- */
    .keymap = {
        [0x01] = KEY_F9,  [0x03] = KEY_F5,  [0x04] = KEY_F3,
        [0x05] = KEY_F1,  [0x06] = KEY_F2,  [0x07] = KEY_F12,
        [0x09] = KEY_F10, [0x0A] = KEY_F8,  [0x0B] = KEY_F6,
        [0x0C] = KEY_F4,
        [0x0D] = KEY_TAB,
        [0x0E] = KEY_TILDE,

        [0x11] = KEY_LALT,
        [0x12] = KEY_LSHIFT,
        [0x14] = KEY_LCTRL,

        [0x15] = KEY_Q, [0x16] = KEY_1,
        [0x1A] = KEY_Z, [0x1B] = KEY_S, [0x1C] = KEY_A, [0x1D] = KEY_W,
        [0x1E] = KEY_2,
        [0x21] = KEY_C, [0x22] = KEY_X, [0x23] = KEY_D, [0x24] = KEY_E,
        [0x25] = KEY_4, [0x26] = KEY_3,
        [0x29] = KEY_SPACE,
        [0x2A] = KEY_V, [0x2B] = KEY_F, [0x2C] = KEY_T, [0x2D] = KEY_R,
        [0x2E] = KEY_5,
        [0x31] = KEY_N, [0x32] = KEY_B, [0x33] = KEY_H, [0x34] = KEY_G,
        [0x35] = KEY_Y, [0x36] = KEY_6,
        [0x3A] = KEY_M, [0x3B] = KEY_J, [0x3C] = KEY_U,
        [0x3D] = KEY_7, [0x3E] = KEY_8,
        [0x41] = KEY_COMMA,
        [0x42] = KEY_K, [0x43] = KEY_I, [0x44] = KEY_O,
        [0x45] = KEY_0, [0x46] = KEY_9,
        [0x49] = KEY_DOT, [0x4A] = KEY_SLASH, [0x4B] = KEY_L,
        [0x4C] = KEY_SEMICOLON, [0x4D] = KEY_P, [0x4E] = KEY_DASH,
        [0x52] = KEY_QUOTE,
        [0x54] = KEY_LBRACKET,
        [0x55] = KEY_EQUAL,
        [0x58] = KEY_CAPS,
        [0x59] = KEY_RSHIFT,
        [0x5A] = KEY_RETURN,
        [0x5B] = KEY_RBRACKET,
        [0x5D] = KEY_BACKSLASH,
        [0x66] = KEY_BACKSPACE,

        /* Numpad */
        [0x69] = KEY_KP_1,
        [0x6B] = KEY_KP_4, [0x6C] = KEY_KP_7,
        [0x70] = KEY_KP_0, [0x71] = KEY_KP_DOT,
        [0x72] = KEY_KP_2, [0x73] = KEY_KP_5, [0x74] = KEY_KP_6,
        [0x75] = KEY_KP_8,
        [0x76] = KEY_ESC,
        [0x77] = KEY_NUM_LOCK,
        [0x78] = KEY_F11,
        [0x79] = KEY_KP_ADD,
        [0x7A] = KEY_KP_3,
        [0x7B] = KEY_KP_SUBTRACT,
        [0x7C] = KEY_KP_MULTIPLY,
        [0x7D] = KEY_KP_9,
        [0x7E] = KEY_SCROLL_LOCK,
        [0x83] = KEY_F7,
    },

    /* --- extmap: E0 + make code -> KBD_KeyCodes_t (Set 2) --- */
    .extmap = {
        [0x11] = KEY_RALT,
        [0x14] = KEY_RCTRL,
        [0x4A] = KEY_KP_DIVIDE,
        [0x5A] = KEY_KP_ENTER,
        [0x5B] = KEY_LSYSTEM,
        [0x5C] = KEY_RSYSTEM,
        [0x5D] = KEY_MENU,
        [0x69] = KEY_END,
        [0x6B] = KEY_LEFT,
        [0x6C] = KEY_HOME,
        [0x70] = KEY_INSERT,
        [0x71] = KEY_DELETE,
        [0x72] = KEY_DOWN,
        [0x74] = KEY_RIGHT,
        [0x75] = KEY_UP,
        [0x7A] = KEY_PAGEDOWN,
        [0x7C] = KEY_PRINT,
        [0x7D] = KEY_PAGEUP,
    },
};
#pragma once

#include <stdint.h>

typedef enum {
    KEY_NONE,

    // Letters
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E,
    KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,
    KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y,
    KEY_Z,

    // Numbers (Top Row)
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,
    KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,

    // Punctuation & Symbols (US Layout)
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

    // System & Control
    KEY_BACKSPACE,
    KEY_SPACE,
    KEY_RETURN,
    KEY_TAB,
    KEY_CAPS,
    KEY_ESC,
    KEY_DELETE,

    // Modifiers
    KEY_LSHIFT, KEY_RSHIFT,
    KEY_LCTRL,  KEY_RCTRL,
    KEY_LALT,   KEY_RALT,
    KEY_LSYSTEM, KEY_RSYSTEM,

    // Navigation
    KEY_MENU,
    KEY_PRINT,
    KEY_SCROLL_LOCK,
    KEY_PAUSE,
    KEY_INSERT,
    KEY_HOME,
    KEY_END,
    KEY_PAGEUP,
    KEY_PAGEDOWN,

    // Arrows
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,

    // Function Keys
    KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5,  KEY_F6,
    KEY_F7,  KEY_F8,  KEY_F9,  KEY_F10, KEY_F11, KEY_F12,
    KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18,
    KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24,

    // Numpad
    KEY_KP_0, KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4,
    KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9,
    KEY_KP_DIVIDE,
    KEY_KP_MULTIPLY,
    KEY_KP_SUBTRACT,
    KEY_KP_ADD,
    KEY_KP_ENTER,
    KEY_KP_DOT,
    KEY_NUM_LOCK,

    // Media Keys
    KEY_MEDIA_PLAY_PAUSE,
    KEY_MEDIA_STOP,
    KEY_MEDIA_NEXT,
    KEY_MEDIA_PREVIOUS,
    KEY_VOLUME_UP,
    KEY_VOLUME_DOWN,
    KEY_VOLUME_MUTE,

    // International (ABNT2)
    KEY_NON_US_SLASH,
    KEY_KP_COMMA,

    // Power / Battery
    KEY_POWER,
    KEY_SLEEP,
    KEY_WAKE,
    KEY_BATTERY_LOW,
    KEY_SCREEN_SAVER,

    // Browser
    KEY_BROWSER_BACK,
    KEY_BROWSER_FORWARD,
    KEY_BROWSER_REFRESH,
    KEY_BROWSER_STOP,
    KEY_BROWSER_SEARCH,
    KEY_BROWSER_FAVORITES,
    KEY_BROWSER_HOME,

    // Advanced Numpad
    KEY_KP_EQUAL,
    KEY_KP_LEFT_PAREN,
    KEY_KP_RIGHT_PAREN,
    KEY_CLEAR,

    // App Launchers
    KEY_CALCULATOR,
    KEY_LAUNCH_MAIL,
    KEY_LAUNCH_MEDIA_PLAYER,
    KEY_EXPLORER,
    KEY_HIBERNATE,
    KEY_APP_HELP,

    // Macro
    KEY_M1, KEY_M2, KEY_M3, KEY_M4, KEY_M5,
    KEY_GAME_MODE,
    KEY_PROFILE_SWITCH,

    // Modern OS
    KEY_VOICE_COMMAND,
    KEY_EMOJI_PICKER,

    // Legacy
    KEY_SYSREQ,
    KEY_ATTN,
    KEY_CANCEL
} KeyCodes;

static const char scset1_normal_map[0x6E] = {
    '\0', '\0', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\n', '\0', 'a',  's',
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  '\'', '`',  '\0', '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  '\0', '*',  '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3',  '0',  '.',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};

static const char scset1_shift_map[0x6E] = {
    '\0', '\0', '!',  '@',  '#',  '$',  '%',  '?',  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}',  '\n', '\0', 'A',  'S',
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',  '~',  '\0', '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '>',  '<',  '?',  '\0', '*',  '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3',  '0',  '.',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};

static const KeyCodes scset1_map[0x6E] = {
    KEY_NONE,        KEY_ESC,         KEY_1,           KEY_2,           KEY_3,           KEY_4,
    KEY_5,           KEY_6,           KEY_7,           KEY_8,           KEY_9,           KEY_0,
    KEY_DASH,        KEY_EQUAL,       KEY_BACKSPACE,   KEY_TAB,         KEY_Q,           KEY_W,
    KEY_E,           KEY_R,           KEY_T,           KEY_Y,           KEY_U,           KEY_I,
    KEY_O,           KEY_P,           KEY_LBRACKET,    KEY_RBRACKET,    KEY_RETURN,      KEY_LCTRL,
    KEY_A,           KEY_S,           KEY_D,           KEY_F,           KEY_G,           KEY_H,
    KEY_J,           KEY_K,           KEY_L,           KEY_SEMICOLON,   KEY_QUOTE,       KEY_TILDE,
    KEY_LSHIFT,      KEY_BACKSLASH,   KEY_Z,           KEY_X,           KEY_C,           KEY_V,
    KEY_B,           KEY_N,           KEY_M,           KEY_COMMA,       KEY_DOT,         KEY_SLASH,
    KEY_RSHIFT,      KEY_KP_MULTIPLY, KEY_LALT,        KEY_SPACE,       KEY_CAPS,        KEY_F1,
    KEY_F2,          KEY_F3,          KEY_F4,          KEY_F5,          KEY_F6,          KEY_F7,
    KEY_F8,          KEY_F9,          KEY_F10,         KEY_NUM_LOCK,    KEY_SCROLL_LOCK, KEY_KP_7,
    KEY_KP_8,        KEY_KP_9,        KEY_KP_SUBTRACT, KEY_KP_4,        KEY_KP_5,        KEY_KP_6,
    KEY_KP_ADD,      KEY_KP_1,        KEY_KP_2,        KEY_KP_3,        KEY_KP_0,        KEY_KP_DOT,
    KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_F11,         KEY_F12,         KEY_NONE,
    KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,
    KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,
    KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,        KEY_NONE,
    KEY_NONE,        KEY_NONE
};

static const KeyCodes scset1_ext_map[0x6E] = {
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_MEDIA_PREVIOUS,     KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_MEDIA_NEXT,         KEY_NONE,               KEY_NONE,
    KEY_KP_ENTER,           KEY_RCTRL,              KEY_NONE,               KEY_NONE,
    KEY_VOLUME_MUTE,        KEY_CALCULATOR,         KEY_MEDIA_PLAY_PAUSE,   KEY_NONE,
    KEY_MEDIA_STOP,         KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_VOLUME_DOWN,        KEY_NONE,
    KEY_VOLUME_UP,          KEY_NONE,               KEY_BROWSER_HOME,       KEY_NONE,
    KEY_NONE,               KEY_KP_DIVIDE,          KEY_NONE,               KEY_NONE,
    KEY_RALT,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_HOME,               KEY_UP,                 KEY_PAGEUP,
    KEY_NONE,               KEY_LEFT,               KEY_NONE,               KEY_RIGHT,
    KEY_NONE,               KEY_END,                KEY_DOWN,               KEY_PAGEDOWN,
    KEY_INSERT,             KEY_DELETE,             KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_NONE,               KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_LSYSTEM,            KEY_RSYSTEM,            KEY_MENU,
    KEY_POWER,              KEY_SLEEP,              KEY_NONE,               KEY_NONE,
    KEY_NONE,               KEY_WAKE,               KEY_NONE,               KEY_BROWSER_SEARCH,
    KEY_BROWSER_FAVORITES,  KEY_BROWSER_REFRESH,    KEY_BROWSER_STOP,       KEY_BROWSER_FORWARD,
    KEY_BROWSER_BACK,       KEY_EXPLORER,           KEY_LAUNCH_MAIL,        KEY_LAUNCH_MEDIA_PLAYER
};
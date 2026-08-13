#include "video.h"
#include "../libs/io.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define VIDEO_MEMORY 0xB8000
#define COLS 80
#define ROWS 25
#define DEFAULT_COLOR 0x0F // branco sobre preto

static uint16_t *video_buffer = (uint16_t*)VIDEO_MEMORY;
static uint8_t current_color = DEFAULT_COLOR;

// Variáveis globais do cursor (acessíveis pelo usuário)
uint8_t txt_curX = 0;
uint8_t txt_curY = 0;

// Cores: bg nos 4 bits altos, fg nos 4 bits baixos
static inline uint8_t make_color(uint8_t bg, uint8_t fg) {
    return (bg << 4) | (fg & 0x0F);
}

static inline uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)((color << 8) | (uint8_t)c);
}

// ------------------------------------------------------------
// Atualiza o cursor de hardware com base em txt_curX e txt_curY
// ------------------------------------------------------------
static void vga_update_cursor(void) {
    uint16_t pos = txt_curY * COLS + txt_curX;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// ------------------------------------------------------------
// Mostra ou oculta o cursor (hardware)
// ------------------------------------------------------------
void vga_cur_show(bool show) {
    if (show) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);
        outb(0x3D4, 0x0B);
        outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
    } else {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
    }
}

// ------------------------------------------------------------
// Função interna para rolagem da tela
// ------------------------------------------------------------
static void scroll(void) {
    for (int y = 1; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            video_buffer[(y-1) * COLS + x] = video_buffer[y * COLS + x];
        }
    }
    for (int x = 0; x < COLS; x++) {
        video_buffer[(ROWS-1) * COLS + x] = make_entry(' ', current_color);
    }
    if (txt_curY > 0) {
        txt_curY--;
        vga_update_cursor();
    }
}

// ------------------------------------------------------------
// Inicializa o vídeo
// ------------------------------------------------------------
void video_init(void) {
    txt_clear();
    vga_cur_show(true);
}

// ------------------------------------------------------------
// Define as cores para impressão
// ------------------------------------------------------------
void txt_setcol(uint8_t bg, uint8_t fg) {
    current_color = make_color(bg & 0x0F, fg & 0x0F);
}

void txt_setattr(uint8_t attr) {
    current_color = attr;
}

// ------------------------------------------------------------
// Move o cursor (atualiza variáveis e hardware)
// ------------------------------------------------------------
void txt_jump(uint8_t x, uint8_t y) {
    if (x < COLS) txt_curX = x;
    if (y < ROWS) txt_curY = y;
    vga_update_cursor();
}

// ------------------------------------------------------------
// Função interna para escrever um caractere na posição atual
// (com tratamento de \n, \r, \t)
// ------------------------------------------------------------
static void putc_internal(char c) {
    if (c == '\n') {
        txt_curX = 0;
        txt_curY++;
    } else if (c == '\r') {
        txt_curX = 0;
    } else if (c == '\t') {
        for (int i = 0; i < 4; i++) putc_internal(' ');
        return;
    } else {
        video_buffer[txt_curY * COLS + txt_curX] = make_entry(c, current_color);
        txt_curX++;
    }

    if (txt_curX >= COLS) {
        txt_curX = 0;
        txt_curY++;
    }
    if (txt_curY >= ROWS) {
        scroll();
    } else {
        vga_update_cursor();
    }
}

// ------------------------------------------------------------
// Imprime string na posição atual
// ------------------------------------------------------------
void txt_print(const char *str) {
    while (*str) {
        putc_internal(*str++);
    }
}

void txt_putc(char c) {
    putc_internal(c);
}

// ------------------------------------------------------------
// Imprime string em posição absoluta (não altera o cursor)
// ------------------------------------------------------------
void txt_pos_print(uint8_t x, uint8_t y, const char *str) {
    if (x >= COLS || y >= ROWS) return;
    uint16_t *pos = video_buffer + (y * COLS + x);
    while (*str) {
        if (x >= COLS) break;
        *pos++ = make_entry(*str++, current_color);
        x++;
    }
}

void txt_pos_putc(uint8_t x, uint8_t y, char c) {
    if (x >= COLS || y >= ROWS) return;
    video_buffer[y * COLS + x] = make_entry(c, current_color);
}

// ------------------------------------------------------------
// Preenche um retângulo com um caractere (usando cor atual)
// ------------------------------------------------------------
void txt_fill(uint8_t x, uint8_t y, uint8_t dx, uint8_t dy, char c) {
    if (x >= COLS || y >= ROWS) return;
    if (x + dx > COLS) dx = COLS - x;
    if (y + dy > ROWS) dy = ROWS - y;
    for (uint8_t row = 0; row < dy; row++) {
        for (uint8_t col = 0; col < dx; col++) {
            video_buffer[(y + row) * COLS + (x + col)] = make_entry(c, current_color);
        }
    }
}

// ------------------------------------------------------------
// Limpa a tela com espaços e cor atual
// ------------------------------------------------------------
void txt_clear(void) {
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            video_buffer[y * COLS + x] = make_entry(' ', current_color);
        }
    }
    txt_curX = 0;
    txt_curY = 0;
    vga_update_cursor();
}

void txt_clchar(char c) {
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            video_buffer[y * COLS + x] = make_entry(c, current_color);
        }
    }
    txt_curX = 0;
    txt_curY = 0;
    vga_update_cursor();
}

// ------------------------------------------------------------
// Funções auxiliares para formatação
// ------------------------------------------------------------
static char hex_digit(uint8_t d) {
    return (d < 10) ? ('0' + d) : ('A' + (d - 10));
}

static void print_unsigned_dec(unsigned int num) {
    char buf[12];
    int i = 0;
    if (num == 0) {
        txt_putc('0');
        return;
    }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        txt_putc(buf[--i]);
    }
}

void txt_print_int(int num) {
    if (num < 0) {
        txt_putc('-');
        if (num == -2147483648) {
            print_unsigned_dec(2147483648U);
            return;
        }
        print_unsigned_dec(-num);
    } else {
        print_unsigned_dec(num);
    }
}

void txt_print_uint(unsigned int num) {
    print_unsigned_dec(num);
}

void txt_print_hex(unsigned int num) {
    txt_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (num >> i) & 0xF;
        txt_putc(hex_digit(digit));
    }
}

// ------------------------------------------------------------
// txt_vprintf - versão com va_list
// ------------------------------------------------------------
static void txt_vprintf(const char *fmt, va_list args) {
    while (*fmt) {
        if (*fmt != '%') {
            txt_putc(*fmt++);
            continue;
        }

        fmt++;
        switch (*fmt) {
            case 'd': {
                int val = va_arg(args, int);
                txt_print_int(val);
                fmt++;
                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);
                txt_print_uint(val);
                fmt++;
                break;
            }
            case 'x':
            case 'X': {
                unsigned int val = va_arg(args, unsigned int);
                txt_print_hex(val);
                fmt++;
                break;
            }
            case 's': {
                const char *str = va_arg(args, const char*);
                txt_print(str);
                fmt++;
                break;
            }
            case 'c': {
                char ch = (char)va_arg(args, int);
                txt_putc(ch);
                fmt++;
                break;
            }
            case '%': {
                txt_putc('%');
                fmt++;
                break;
            }
            default: {
                txt_putc('%');
                txt_putc(*fmt++);
                break;
            }
        }
    }
}

// ------------------------------------------------------------
// txt_printf – printf simplificado
// ------------------------------------------------------------
void txt_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    txt_vprintf(fmt, args);
    va_end(args);
}

void txt_newl(void) {
    txt_putc('\n');
}

// ------------------------------------------------------------
// NOVAS FUNÇÕES: word wrap e posicionamento
// ------------------------------------------------------------

// Imprime string com quebra de linha automática (word wrap) na posição atual
void txt_printw(const char *str) {
    while (*str) {
        if (*str == '\n') {
            txt_putc('\n');
            str++;
            continue;
        }
        // Se chegou ao final da linha, tenta quebrar no último espaço
        if (txt_curX >= COLS - 1) {
            // Procura o último espaço antes da posição atual
            const char *last_space = str;
            // Volta até encontrar um espaço ou início da string
            while (last_space > str && *last_space != ' ') last_space--;
            if (*last_space == ' ' && last_space != str) {
                // Imprime do início até o espaço (inclusive)
                while (str <= last_space) {
                    if (*str != ' ') txt_putc(*str);
                    str++;
                }
                txt_putc('\n');
                // Pula o espaço
                if (*str == ' ') str++;
                continue;
            } else {
                // Não há espaço, quebra forçada
                txt_putc('\n');
                continue;
            }
        }
        txt_putc(*str++);
    }
}

// Imprime string com quebra de linha automática em posição absoluta
void txt_pos_printw(uint8_t x, uint8_t y, const char *str) {
    uint8_t old_x = txt_curX;
    uint8_t old_y = txt_curY;
    txt_jump(x, y);
    txt_printw(str);
    txt_jump(old_x, old_y);
}

// Imprime string formatada em posição absoluta (printf com posição)
void txt_pos_printf(uint8_t x, uint8_t y, const char *fmt, ...) {
    uint8_t old_x = txt_curX;
    uint8_t old_y = txt_curY;
    txt_jump(x, y);
    va_list args;
    va_start(args, fmt);
    txt_vprintf(fmt, args);
    va_end(args);
    txt_jump(old_x, old_y);
}
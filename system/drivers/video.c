#include "video.h"
#include "../libs/io.h"
#include <stdbool.h>
#include <stddef.h>

#define VIDEO_MEMORY 0xB8000
#define COLS 80
#define ROWS 25
#define DEFAULT_COLOR 0x0F // branco sobre preto

static uint16_t *video_buffer = (uint16_t*)VIDEO_MEMORY;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t current_color = DEFAULT_COLOR;

// Cores: bg nos 4 bits altos, fg nos 4 bits baixos
static inline uint8_t make_color(uint8_t bg, uint8_t fg) {
    return (bg << 4) | (fg & 0x0F);
}

static inline uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)((color << 8) | (uint8_t)c);
}

// ... no topo, após as variáveis estáticas

void txt_cur_pos(uint8_t *x, uint8_t *y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

void txt_show(bool show) {
    uint16_t pos = cursor_y * COLS + cursor_x;
    if (show) {
        // Habilita cursor (linha de 14 a 15)
        outb(0x3D4, 0x0A);
        outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);
        outb(0x3D4, 0x0B);
        outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
        // Define posição
        outb(0x3D4, 0x0F);
        outb(0x3D5, (uint8_t)(pos & 0xFF));
        outb(0x3D4, 0x0E);
        outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    } else {
        // Desabilita cursor
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
    }
}

// Função interna para rolagem da tela
static void scroll(void) {
    // Move todas as linhas para cima
    for (int y = 1; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            video_buffer[(y-1) * COLS + x] = video_buffer[y * COLS + x];
        }
    }
    // Limpa a última linha com espaços e cor atual
    for (int x = 0; x < COLS; x++) {
        video_buffer[(ROWS-1) * COLS + x] = make_entry(' ', current_color);
    }
    if (cursor_y > 0) cursor_y--;
}

// Inicializa o vídeo e limpa a tela
void video_init(void) {
    txt_clear();
}

// Define as cores
void txt_setcol(uint8_t bg, uint8_t fg) {
    current_color = make_color(bg & 0x0F, fg & 0x0F);
}

// Move cursor sem imprimir
void txt_jump(uint8_t x, uint8_t y) {
    if (x < COLS) cursor_x = x;
    if (y < ROWS) cursor_y = y;
}

// Função interna para escrever um caractere na posição atual (com tratamento de \n, \r, \t)
static void putc_internal(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        // Tab: 4 espaços
        for (int i = 0; i < 4; i++) putc_internal(' ');
        return;
    } else {
        video_buffer[cursor_y * COLS + cursor_x] = make_entry(c, current_color);
        cursor_x++;
    }

    // Verifica se precisa quebrar linha
    if (cursor_x >= COLS) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= ROWS) {
        scroll();
    }
}

// Imprime uma string na posição atual
void txt_print(const char *str) {
    while (*str) {
        putc_internal(*str++);
    }
}

// Imprime um caractere na posição atual
void txt_putc(char c) {
    putc_internal(c);
}

// Imprime string em posição absoluta (não altera o cursor)
void txt_pos_print(uint8_t x, uint8_t y, const char *str) {
    if (x >= COLS || y >= ROWS) return;
    uint16_t *pos = video_buffer + (y * COLS + x);
    while (*str) {
        if (x >= COLS) break;
        *pos++ = make_entry(*str++, current_color);
        x++;
    }
}

// Coloca caractere em posição absoluta
void txt_pos_putc(uint8_t x, uint8_t y, char c) {
    if (x >= COLS || y >= ROWS) return;
    video_buffer[y * COLS + x] = make_entry(c, current_color);
}

// Preenche um retângulo com o caractere (usando cor atual)
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

// Limpa a tela com espaços e cor atual
void txt_clear(void) {
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            video_buffer[y * COLS + x] = make_entry(' ', current_color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

// Limpa a tela com um caractere específico e a cor atual
void txt_clchar(char c) {
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            video_buffer[y * COLS + x] = make_entry(c, current_color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}
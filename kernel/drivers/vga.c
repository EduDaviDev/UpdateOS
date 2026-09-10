#include "vga.h"

/* Ponteiro para o buffer de texto VGA */
static volatile uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;

/* Posição atual do cursor */
static size_t vga_row = 0;
static size_t vga_col = 0;

/* Cor atual (padrão: cinza claro sobre preto) */
static uint8_t vga_current_color = 0;

/* ---- Funções auxiliares internas ---- */

/* Combina fg e bg em um byte de atributo VGA: (bg << 4) | fg */
static inline uint8_t vga_make_color(vga_color_t fg, vga_color_t bg)
{
    return (uint8_t)(bg << 4 | fg);
}

/* Combina caractere e cor em uma entrada de 16 bits para o buffer VGA */
static inline uint16_t vga_make_entry(char c, uint8_t color)
{
    return (uint16_t)((uint16_t)color << 8 | (uint8_t)c);
}

/* Atualiza o cursor de hardware para a posição (row, col) */
static void vga_update_cursor(void)
{
    uint16_t pos = (uint16_t)(vga_row * VGA_WIDTH + vga_col);

    /* Porta 0x3D4: índice do registrador do cursor
       Porta 0x3D5: valor do registrador */
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(pos & 0xFF)), "Nd"((uint16_t)0x3D5));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)((pos >> 8) & 0xFF)), "Nd"((uint16_t)0x3D5));
}

/* Rola a tela uma linha para cima quando o cursor passa da última linha */
static void vga_scroll(void)
{
    if (vga_row >= VGA_HEIGHT) {
        /* Move todas as linhas uma para cima */
        for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
        /* Limpa a última linha */
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
                vga_make_entry(' ', vga_current_color);
        }
        vga_row = VGA_HEIGHT - 1;
    }
}

/* ---- Implementação da API pública ---- */

void vga_init(void)
{
    vga_current_color = vga_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_row = 0;
    vga_col = 0;
    vga_clear();
    vga_update_cursor();
}

void vga_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] =
                vga_make_entry(' ', vga_current_color);
        }
    }
    vga_row = 0;
    vga_col = 0;
    vga_update_cursor();
}

void vga_putc(char c)
{
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
        vga_scroll();
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        /* Tabulação: avança até o próximo múltiplo de 4 */
        vga_col = (vga_col + 4) & ~3;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
            vga_scroll();
        }
    } else if (c == '\b') {
        /* Backspace: volta um caractere e apaga */
        if (vga_col > 0) {
            vga_col--;
        } else if (vga_row > 0) {
            vga_row--;
            vga_col = VGA_WIDTH - 1;
        }
        vga_buffer[vga_row * VGA_WIDTH + vga_col] =
            vga_make_entry(' ', vga_current_color);
    } else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] =
            vga_make_entry(c, vga_current_color);
        vga_col++;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
            vga_scroll();
        }
    }
    vga_update_cursor();
}

void vga_print(const char *str)
{
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_print_dec(uint32_t num)
{
    if (num == 0) {
        vga_putc('0');
        return;
    }

    char buf[11]; /* uint32_t max: 4294967295 (10 dígitos + '\0') */
    int i = 0;

    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    /* Inverte a string */
    for (int j = i - 1; j >= 0; j--) {
        vga_putc(buf[j]);
    }
}

void vga_print_hex(uint32_t num)
{
    if (num == 0) {
        vga_putc('0');
        return;
    }

    char buf[9]; /* uint32_t max: FFFFFFFF (8 dígitos + '\0') */
    int i = 0;

    while (num > 0) {
        uint32_t digit = num & 0xF;
        buf[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        num >>= 4;
    }

    for (int j = i - 1; j >= 0; j--) {
        vga_putc(buf[j]);
    }
}

void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    vga_current_color = vga_make_color(fg, bg);
}
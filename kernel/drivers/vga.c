#include "vga.h"
#include "../io.h"
#include <stdarg.h>

/* Buffer de memória do modo texto VGA */
#define VGA_MEMORY  ((volatile uint16_t *)0xB8000)

/* Portas do controlador de cursor VGA */
#define VGA_CTRL_REG    0x3D4
#define VGA_CTRL_DATA   0x3D5

/* ---------------- Estado global ---------------- */

int      vga_pos_x = 0;
int      vga_pos_y = 0;
uint8_t  vga_default_attr = 0x07;   /* cinza claro sobre preto */

/* ---------------- Helpers internos ---------------- */

static inline uint16_t vga_entry(char c, uint8_t attr)
{
    return (uint16_t)c | ((uint16_t)attr << 8);
}

static void vga_scroll(void)
{
    /* Move todas as linhas uma para cima */
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] =
                VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }
    /* Limpa a última linha */
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', vga_default_attr);
    }
}

/* ---------------- API ---------------- */

void vga_init(void)
{
    vga_pos_x = 0;
    vga_pos_y = 0;
    vga_default_attr = 0x07;
    vga_clear();
}

void vga_clear(void)
{
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] =
                vga_entry(' ', vga_default_attr);
        }
    }
    vga_pos_x = 0;
    vga_pos_y = 0;
    vga_update_cursor();
}

void vga_putchar(char c)
{
    switch (c) {
    case '\n':
        vga_pos_x = 0;
        vga_pos_y++;
        break;

    case '\r':
        vga_pos_x = 0;
        break;

    case '\t':
        /* Avança até o próximo múltiplo de 8 */
        vga_pos_x = (vga_pos_x + 8) & ~7;
        if (vga_pos_x >= VGA_WIDTH) {
            vga_pos_x = 0;
            vga_pos_y++;
        }
        break;

    case '\b':
        /* Não apaga aqui: vga_backspace() cuida disso.
           Se chegar sozinho, só move o cursor para trás. */
        if (vga_pos_x > 0) {
            vga_pos_x--;
        } else if (vga_pos_y > 0) {
            vga_pos_y--;
            vga_pos_x = VGA_WIDTH - 1;
        }
        break;

    default:
        if ((unsigned char)c < 0x20) {
            /* Outros controles: ignora */
            break;
        }
        VGA_MEMORY[vga_pos_y * VGA_WIDTH + vga_pos_x] =
            vga_entry(c, vga_default_attr);
        vga_pos_x++;
        if (vga_pos_x >= VGA_WIDTH) {
            vga_pos_x = 0;
            vga_pos_y++;
        }
        break;
    }

    /* Rolagem quando passa da última linha */
    if (vga_pos_y >= VGA_HEIGHT) {
        vga_scroll();
        vga_pos_y = VGA_HEIGHT - 1;
    }

    vga_update_cursor();
}

void vga_print(const char *s)
{
    while (*s) {
        vga_putchar(*s++);
    }
}

void vga_print_color(const char *s, vga_color_t fg, vga_color_t bg)
{
    uint8_t saved = vga_default_attr;
    vga_default_attr = (uint8_t)(bg << 4) | (uint8_t)(fg & 0x0F);
    vga_print(s);
    vga_default_attr = saved;
}

void vga_backspace(void)
{
    /* Move para trás, escreve espaço, volta de novo */
    if (vga_pos_x > 0) {
        vga_pos_x--;
    } else if (vga_pos_y > 0) {
        vga_pos_y--;
        vga_pos_x = VGA_WIDTH - 1;
    } else {
        return; /* Já está no canto superior esquerdo */
    }

    VGA_MEMORY[vga_pos_y * VGA_WIDTH + vga_pos_x] =
        vga_entry(' ', vga_default_attr);

    vga_update_cursor();
}

void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    vga_default_attr = (uint8_t)(bg << 4) | (uint8_t)(fg & 0x0F);
}

void vga_set_cursor(int x, int y)
{
    if (x < 0) x = 0;
    if (x >= VGA_WIDTH) x = VGA_WIDTH - 1;
    if (y < 0) y = 0;
    if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;

    vga_pos_x = x;
    vga_pos_y = y;
    vga_update_cursor();
}

void vga_update_cursor(void)
{
    uint16_t pos = (uint16_t)(vga_pos_y * VGA_WIDTH + vga_pos_x);

    outb(VGA_CTRL_REG, 0x0F);
    outb(VGA_CTRL_DATA, (uint8_t)(pos & 0xFF));

    outb(VGA_CTRL_REG, 0x0E);
    outb(VGA_CTRL_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

/* ---------------- Helpers de formatação ---------------- */

/* Converte uint32 em string numa base (2..16). Devolve o tamanho. */
static int utoa_base(uint32_t value, char *buf, int base, int uppercase)
{
    const char *digits = uppercase
        ? "0123456789ABCDEF"
        : "0123456789abcdef";
    char tmp[32];
    int i = 0;

    if (value == 0) {
        tmp[i++] = '0';
    } else {
        while (value) {
            tmp[i++] = digits[value % (uint32_t)base];
            value /= (uint32_t)base;
        }
    }

    /* tmp está invertido; copia ao contrário */
    int len = i;
    for (int j = 0; j < len; j++)
        buf[j] = tmp[len - 1 - j];
    buf[len] = '\0';
    return len;
}

/* Emite uma string com padding à esquerda.
 * pad = caractere ('0' ou ' '), width = largura mínima. */
static int emit_padded(const char *s, int len, char pad, int width)
{
    int written = 0;
    for (int i = len; i < width; i++) {
        vga_putchar(pad);
        written++;
    }
    for (int i = 0; i < len; i++) {
        vga_putchar(s[i]);
        written++;
    }
    return written;
}

/* Núcleo: recebe va_list para permitir reuso em cores diferentes. */
static int vga_vprintf_impl(const char *fmt, va_list ap)
{
    int written = 0;

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            vga_putchar(*p);
            written++;
            continue;
        }

        p++; /* consome '%' */

        /* Flags: '0' para zero-padding */
        char pad = ' ';
        if (*p == '0') { pad = '0'; p++; }

        /* Largura mínima (apenas dígitos decimais) */
        int width = 0;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        switch (*p) {
        case 'c': {
            char c = (char)va_arg(ap, int);
            vga_putchar(c);
            written++;
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int len = 0;
            while (s[len]) len++;
            written += emit_padded(s, len, pad, width);
            break;
        }
        case 'd':
        case 'i': {
            int32_t v = va_arg(ap, int32_t);
            char buf[32];
            int len;
            if (v < 0) {
                /* Imprime '-' e trata o resto como positivo */
                vga_putchar('-');
                written++;
                uint32_t uv = (uint32_t)(-(int64_t)v);
                len = utoa_base(uv, buf, 10, 0);
            } else {
                len = utoa_base((uint32_t)v, buf, 10, 0);
            }
            written += emit_padded(buf, len, pad, width);
            break;
        }
        case 'u': {
            uint32_t v = va_arg(ap, uint32_t);
            char buf[32];
            int len = utoa_base(v, buf, 10, 0);
            written += emit_padded(buf, len, pad, width);
            break;
        }
        case 'x':
        case 'X': {
            uint32_t v = va_arg(ap, uint32_t);
            char buf[32];
            int len = utoa_base(v, buf, 16, (*p == 'X'));
            written += emit_padded(buf, len, pad, width);
            break;
        }
        case 'p': {
            uint32_t v = (uint32_t)va_arg(ap, void *);
            vga_putchar('0'); vga_putchar('x'); written += 2;
            char buf[32];
            int len = utoa_base(v, buf, 16, 0);
            /* Preenche com zeros até 8 dígitos (endereço de 32 bits) */
            written += emit_padded(buf, len, '0', 8);
            break;
        }
        case '%': {
            vga_putchar('%');
            written++;
            break;
        }
        case '\0':
            /* '%' no fim da string: trata como literal */
            vga_putchar('%');
            written++;
            return written;
        default:
            /* Especificador desconhecido: imprime literal */
            vga_putchar('%');
            vga_putchar(*p);
            written += 2;
            break;
        }
    }

    return written;
}

int vga_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int written = vga_vprintf_impl(fmt, ap);
    va_end(ap);
    return written;
}

int vga_printf_color(vga_color_t fg, vga_color_t bg, const char *fmt, ...)
{
    uint8_t saved = vga_default_attr;
    vga_default_attr = (uint8_t)((bg << 4) | (fg & 0x0F));

    va_list ap;
    va_start(ap, fmt);
    int written = vga_vprintf_impl(fmt, ap);
    va_end(ap);

    vga_default_attr = saved;
    return written;
}
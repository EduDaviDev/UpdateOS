#include "vga.h"
#include "../io.h"

/* Buffer de memória do modo texto VGA */
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

/* Portas do controlador de cursor VGA */
#define VGA_CTRL_REG 0x3D4
#define VGA_CTRL_DATA 0x3D5

/* ---------------- Estado global ---------------- */

int vga_pos_x = 0;
int vga_pos_y = 0;
uint8_t vga_default_attr = 0x07; /* cinza claro sobre preto */

/* ---------------- Helpers internos ---------------- */

static inline uint16_t vga_entry(char c, uint8_t attr)
{
	return (uint16_t)c | ((uint16_t)attr << 8);
}

static void vga_scroll(void)
{
	/* Move todas as linhas uma para cima */
	for (int y = 1; y < VGA_HEIGHT; y++)
	{
		for (int x = 0; x < VGA_WIDTH; x++)
		{
			VGA_MEMORY[(y - 1) * VGA_WIDTH + x] =
				VGA_MEMORY[y * VGA_WIDTH + x];
		}
	}
	/* Limpa a última linha */
	for (int x = 0; x < VGA_WIDTH; x++)
	{
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
	for (int y = 0; y < VGA_HEIGHT; y++)
	{
		for (int x = 0; x < VGA_WIDTH; x++)
		{
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
	switch (c)
	{
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
		if (vga_pos_x >= VGA_WIDTH)
		{
			vga_pos_x = 0;
			vga_pos_y++;
		}
		break;

	case '\b':
		/* Não apaga aqui: vga_backspace() cuida disso.
		   Se chegar sozinho, só move o cursor para trás. */
		if (vga_pos_x > 0)
		{
			vga_pos_x--;
		}
		else if (vga_pos_y > 0)
		{
			vga_pos_y--;
			vga_pos_x = VGA_WIDTH - 1;
		}
		break;

	default:
		if ((unsigned char)c < 0x20)
		{
			/* Outros controles: ignora */
			break;
		}
		VGA_MEMORY[vga_pos_y * VGA_WIDTH + vga_pos_x] =
			vga_entry(c, vga_default_attr);
		vga_pos_x++;
		if (vga_pos_x >= VGA_WIDTH)
		{
			vga_pos_x = 0;
			vga_pos_y++;
		}
		break;
	}

	/* Rolagem quando passa da última linha */
	if (vga_pos_y >= VGA_HEIGHT)
	{
		vga_scroll();
		vga_pos_y = VGA_HEIGHT - 1;
	}

	vga_update_cursor();
}

void vga_print(const char *s)
{
	while (*s)
	{
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
	if (vga_pos_x > 0)
	{
		vga_pos_x--;
	}
	else if (vga_pos_y > 0)
	{
		vga_pos_y--;
		vga_pos_x = VGA_WIDTH - 1;
	}
	else
	{
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
	if (x < 0)
		x = 0;
	if (x >= VGA_WIDTH)
		x = VGA_WIDTH - 1;
	if (y < 0)
		y = 0;
	if (y >= VGA_HEIGHT)
		y = VGA_HEIGHT - 1;

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
#include "../drivers/video.h"
#include "../drivers/keyboard.h"
#include "../interrupt/isr.h"
#include "../libs/string.h"

void kernel_main(unsigned int magic, unsigned int addr) {
    (void)magic; (void)addr;

    video_init();
    keyboard_init();

    isr_install();

    txt_setcol(VGA_COL_BLACK, VGA_COL_WHITE);
    txt_clear();
    txt_print("Update OS com IRQ e teclado não bloqueante!\n");
    txt_print("Pressione teclas... (eventos via IRQ1)\n");

    while (1) {
        KeyEvent ev = keyboard_read();
        if (ev.pressed && ev.Char) {
            txt_putc(*ev.Char);
        } else if (ev.released) {
            // Ignora releases por enquanto
        }
    }
}

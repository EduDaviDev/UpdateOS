#include <stdint.h>
#include "drivers/vga.h"
#include "io.h"
#include "mboot.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/pic.h"
#include "cpu/irq.h"
#include "cpu/isr.h"
#include "libs/string.h"
#include "drivers/keyboard.h"

/* ---------------- Banner ---------------- */

static void draw_banner(void)
{
    vga_print("##################################################\n");
    vga_print("#  _   _           _       _        _____ _____  #\n");
    vga_print("# | | | |         | |     | |      |  _  /  ___| #\n");
    vga_print("# | | | |_ __   __| | __ _| |_ ___ | | | \\ `--.  #\n");
    vga_print("# | | | | '_ \\ / _` |/ _` | __/ _ \\| | | |`--. \\ #\n");
    vga_print("# | |_| | |_) | (_| | (_| | ||  __/\\ \\_/ /\\__/ / #\n");
    vga_print("#  \\___/| .__/ \\__,_|\\__,_|\\__\\___| \\___/\\____/  #\n");
    vga_print("#       | |                                      #\n");
    vga_print("#       |_|                                      #\n");
    vga_print("##################################################\n");
    vga_print("#          UpOS V1.0 Shell! (11/9/2026)          #\n");
    vga_print("#              By Eduardo Davi MS                #\n");
    vga_print("##################################################\n");
}

static void draw_shell(void)
{
    vga_print("\nUpOS> ");
}

/* ---------------- Shell ---------------- */

#define CMD_MAX 128
static char cmd[CMD_MAX];

static void cmd_clear(void)
{
    for (int i = 0; i < CMD_MAX; i++) cmd[i] = '\0';
}

static void cmd_help(void)
{
    vga_print("\nhelp        - mostra essa ajuda");
    vga_print("\nbanner      - desenha o banner");
    vga_print("\nclear       - limpa a tela");
    vga_print("\nshutdown    - desliga o computador");
    vga_print("\nreboot      - reinicia o computador");
    vga_print("\n");
}

static void execute(void)
{
    if (cmd[0] == '\0') {
        /* Comando vazio: não faz nada */
        return;
    }

    if (!strcmp(cmd, "help")) {
        cmd_help();
    }
    else if (!strcmp(cmd, "banner")) {
        vga_putchar('\n');
        draw_banner();
    }
    else if (!strcmp(cmd, "clear")) {
        vga_clear();
    }
    else if (!strcmp(cmd, "shutdown")) {
        vga_print("\nDesligando...\n");
        shutdown();
    }
    else if (!strcmp(cmd, "reboot")) {
        vga_print("\nReiniciando...\n");
        reboot();
    }
    else {
        vga_print("\nComando desconhecido: ");
        vga_print(cmd);
        vga_print("\nUse 'help' para ver os comandos.\n");
    }
}

static void shell(void)
{
    vga_clear();
    draw_banner();
    cmd_clear();
    draw_shell();

    int pos = 0;

    for (;;) {
        if (!kbd_has_event) {
            __asm__ volatile ("hlt");
            continue;
        }

        KBD_Event_t key = kbd_gete();

        /* Só processa quando a tecla foi realmente pressionada */
        if (key.state != KS_PRESSED) continue;

        if (key.filters.printable) {
            if (pos < CMD_MAX - 1) {
                vga_putchar(key.character);
                cmd[pos++] = key.character;
                cmd[pos]   = '\0';
            }
            /* Se buffer cheio, ignora silenciosamente */
        }
        else if (key.keycode == KEY_BACKSPACE) {
            if (pos > 0) {
                pos--;
                cmd[pos] = '\0';
                vga_backspace();
            }
        }
        else if (key.keycode == KEY_RETURN) {
            execute();
            cmd_clear();
            pos = 0;
            draw_shell();
        }
    }
}

/* ---------------- Entrada do kernel ---------------- */

void kernel_main(void)
{
    vga_init();

    gdt_init();
    idt_init();
    irq_install();

    kbd_init();

    __asm__ volatile ("sti");

    acpi_init();

    vga_print("ACPI: ");
    vga_print(acpi_ready ? "OK\n" : "nao encontrado\n");

    shell();

    /* Nunca chega aqui, mas por segurança: */
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
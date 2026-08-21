#include "../drivers/video.h"
#include "../drivers/keyboard.h"
#include "../drivers/serial.h"
#include "../drivers/io.h"
#include "../drivers/fatfs/ff.h"
#include "../interrupt/isr.h"
#include "../interrupt/gdt.h"
#include "../interrupt/tss.h"
#include "../interrupt/syscall.h"
#include "../memory/paging.h"
#include "../libs/string.h"
#include "../libs/memory.h"
#include "uex.h"

/* ============================================================
   Definições do shell (embutido no kernel)
   ============================================================ */

#define MAX_CMD_LEN 128
#define MAX_PATH_LEN 256

static char cwd[MAX_PATH_LEN] = "/";
static char cmd_buf[MAX_CMD_LEN];
static int cmd_pos = 0;

static FATFS fs;
static bool fs_mounted = false;

/* --- Constrói caminho absoluto a partir do cwd --- */
static void build_path(const char *rel, char *out, size_t out_size) {
    if (rel[0] == '/') {
        strncpy(out, rel, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }
    // Garante que cwd termine com '/'
    size_t cwd_len = strlen(cwd);
    if (cwd_len == 0 || cwd[cwd_len - 1] != '/') {
        snprintf(out, out_size, "%s/%s", cwd, rel);
    } else {
        snprintf(out, out_size, "%s%s", cwd, rel);
    }
}

/* --- Lista arquivos do diretório atual --- */
static void cmd_list(void) {
    DIR dir;
    FILINFO fno;
    FRESULT res = f_opendir(&dir, cwd);
    if (res != FR_OK) {
        txt_printf("Erro ao abrir diretório '%s': %d\n", cwd, res);
        return;
    }
    txt_print("Conteúdo de ");
    txt_print(cwd);
    txt_print(":\n");
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;
        if (fno.fname[0] == '.') continue;  // ocultos

        if (fno.fattrib & AM_DIR) {
            txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_CYAN);
            txt_printf("[DIR]  %s\n", fno.fname);
        } else {
            txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_GREEN);
            txt_printf("[FILE] %s (%lu bytes)\n", fno.fname, fno.fsize);
        }
        txt_setcol(VGA_COL_BLACK, VGA_COL_WHITE);
    }
    f_closedir(&dir);
}

/* --- Muda diretório --- */
static void cmd_cd(const char *arg) {
    if (!arg || arg[0] == '\0') {
        txt_print("Uso: cd <pasta>\n");
        return;
    }
    char new_path[MAX_PATH_LEN];
    build_path(arg, new_path, sizeof(new_path));

    DIR testdir;
    FRESULT res = f_opendir(&testdir, new_path);
    if (res == FR_OK) {
        f_closedir(&testdir);
        strncpy(cwd, new_path, sizeof(cwd) - 1);
        cwd[sizeof(cwd) - 1] = '\0';
        // Garante barra final
        size_t len = strlen(cwd);
        if (len > 0 && cwd[len - 1] != '/') {
            strncat(cwd, "/", sizeof(cwd) - len - 1);
        }
    } else {
        txt_printf("Diretório não encontrado: %s (código %d)\n", arg, res);
    }
}

/* --- Executa um binário --- */
static void cmd_run(const char *arg) {
    if (!arg || arg[0] == '\0') {
        txt_print("Uso: run <arquivo>\n");
        return;
    }
    char full_path[MAX_PATH_LEN];
    build_path(arg, full_path, sizeof(full_path));

    // Verifica se o arquivo existe antes de tentar abrir
    FILINFO info;
    FRESULT res = f_stat(full_path, &info);
    if (res != FR_OK) {
        txt_printf("Arquivo não encontrado: %s (código %d)\n", full_path, res);
        return;
    }

    uex_run(full_path);
}

/* --- Exibe conteúdo de arquivo texto --- */
static void cmd_read(const char *arg) {
    if (!arg || arg[0] == '\0') {
        txt_print("Uso: read <arquivo>\n");
        return;
    }
    char full_path[MAX_PATH_LEN];
    build_path(arg, full_path, sizeof(full_path));

    FIL file;
    FRESULT res = f_open(&file, full_path, FA_READ);
    if (res != FR_OK) {
        txt_printf("Erro ao abrir %s: %d\n", full_path, res);
        return;
    }
    char buffer[256];
    UINT br;
    while (f_read(&file, buffer, sizeof(buffer) - 1, &br) == FR_OK && br > 0) {
        buffer[br] = '\0';
        txt_print(buffer);
    }
    f_close(&file);
    txt_newl();
}

/* --- Comandos de sistema --- */
static void cmd_reboot(void) {
    txt_print("Reiniciando...\n");
    reboot();
}
static void cmd_shutdown(void) {
    txt_print("Desligando...\n");
    shutdown();
}
static void cmd_help(void) {
    txt_print("Comandos:\n");
    txt_print("  list            - lista arquivos\n");
    txt_print("  cd <pasta>      - muda diretório\n");
    txt_print("  run <arquivo>   - executa binário\n");
    txt_print("  read <arquivo>  - exibe arquivo texto\n");
    txt_print("  reboot          - reinicia\n");
    txt_print("  shutdown        - desliga\n");
    txt_print("  help            - esta ajuda\n");
    txt_print("  exit            - sai do shell\n");
}

/* --- Processa comando --- */
static bool process_command(char *cmd) {
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return false;

    char *arg = cmd;
    while (*arg && *arg != ' ') arg++;
    if (*arg == ' ') {
        *arg = '\0';
        arg++;
        while (*arg == ' ') arg++;
    } else {
        arg = NULL;
    }

    if (strcmp(cmd, "list") == 0) {
        cmd_list();
    } else if (strcmp(cmd, "cd") == 0) {
        cmd_cd(arg);
    } else if (strcmp(cmd, "run") == 0) {
        cmd_run(arg);
    } else if (strcmp(cmd, "read") == 0) {
        cmd_read(arg);
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(cmd, "shutdown") == 0) {
        cmd_shutdown();
    } else if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "exit") == 0) {
        txt_print("Saindo do shell...\n");
        return true;
    } else {
        txt_printf("Comando desconhecido: %s\n", cmd);
    }
    return false;
}

/* --- Shell principal --- */
static void shell_run(void) {
    if (!fs_mounted) {
        FRESULT res = f_mount(&fs, "0:", 1);
        if (res != FR_OK) {
            txt_printf("Erro ao montar FAT: %d\n", res);
            return;
        }
        fs_mounted = true;
        txt_print("Sistema de arquivos montado em 0:\n");
    }

    txt_printf("Shell UpdateOS - Digite 'help'.\n");
    txt_printf("Diretório atual: %s\n", cwd);

    bool exit_shell = false;
    while (!exit_shell) {
        txt_printf("%s> ", cwd);
        cmd_pos = 0;
        cmd_buf[0] = '\0';

        while (1) {
            KeyEvent key = kbd_gevent();
            if (!key.pressed) continue;

            char c = key.character;

            if (c == '\n' || key.keycode == KEY_RETURN) {
                txt_newl();
                if (cmd_pos > 0) {
                    cmd_buf[cmd_pos] = '\0';
                    if (process_command(cmd_buf)) {
                        exit_shell = true;
                    }
                }
                break;
            }
            else if (key.keycode == KEY_BACKSPACE) {
			    if (cmd_pos > 0) {
			        // 1. Remove do buffer
			        cmd_pos--;
			        cmd_buf[cmd_pos] = '\0';
				
			        // 2. Volta o cursor uma posição (se não estiver no início da linha)
			        if (txt_curX > 0) {
			            txt_curX--;
			            // 3. Escreve um espaço na posição atual (apaga o caractere)
			            txt_putc(' ');
			            // 4. Volta o cursor novamente para a posição do espaço
			            txt_curX--;
			        } else {
			            // Se estiver no início da linha, não faz nada (ou sobe linha)
			            // Mas geralmente não ocorre porque o prompt nunca está em coluna 0
			        }
			    }
			}
            else if (c >= 32 && c <= 126) {
                if (cmd_pos < MAX_CMD_LEN - 1) {
                    cmd_buf[cmd_pos++] = c;
                    cmd_buf[cmd_pos] = '\0';
                    txt_putc(c);
                }
            }
            // Ignora outras teclas
        }
    }
}

void kernel_main(void) {
    video_init();
    serial_init();
    txt_clear();
    txt_print("UpdateOS v1.0\n");

    gdt_init();
    tss_init();
    isr_install();
    syscall_init();
    kbd_init();

    txt_print("Sistema inicializado.\n");
    shell_run();
    while (1) __asm__("hlt");
}
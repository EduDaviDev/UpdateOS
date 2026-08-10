#include "../drivers/keyboard.h"
#include "../drivers/video.h"
#include "../interrupt/isr.h"
#include "../drivers/fatfs/ff.h"
#include "../drivers/fatfs/diskio.h"
#include "../libs/serial.h"
#include "../libs/memory.h"
#include "../libs/string.h"

// Layout fixo EN-US (se você tiver um arquivo de layout, inclua-o)
// #include "../drivers/keyboard_layout.h"

char fn[64];
char cf[128];
int pos = 0;
FATFS fs;
char disk = '0';          // Disco fixo: 0:

DIR folder;
FIL file;

// ============================================================
// comando_exe – processa os comandos do shell
// ============================================================
void command_exe(char com[64]) {
    // Remove quebra de linha (se houver)
    size_t len = strlen(com);
    if (len > 0 && (com[len-1] == '\n' || com[len-1] == '\r')) {
        com[len-1] = '\0';
        len--;
        if (len > 0 && com[len-1] == '\r') com[len-1] = '\0';
    }

    // Comando vazio → mostra prompt
    if (strlen(com) == 0) {
        txt_printf("%s> ", cf);
        return;
    }

    // ------------------------------------------------------------
    // ls – listar diretório atual
    // ------------------------------------------------------------
    if (strcmp(com, "ls") == 0) {
        DIR dir;
        FILINFO fno;
        FRESULT res = f_opendir(&dir, cf);
        if (res != FR_OK) {
            txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_RED);
            txt_printf("Erro ao abrir diretório: %d\n", res);
            txt_setcol(VGA_COL_BLACK, VGA_COL_WHITE);
            txt_printf("%s> ", cf);
            return;
        }
        txt_print("Conteúdo do diretório:\n");
        while (1) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;
            // Ignora "." e ".."
            if (fno.fname[0] == '.') continue;

            if (fno.fattrib & AM_DIR) {
                txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_CYAN);
                txt_printf("[DIR]  %s\n", fno.fname);
            } else {
                txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_GREEN);
                txt_printf("[FILE] %s (%lu bytes)\n", fno.fname, fno.fsize);
            }
        }
        f_closedir(&dir);
        txt_setcol(VGA_COL_BLACK, VGA_COL_WHITE);
        txt_printf("%s> ", cf);
        return;
    }

    // ------------------------------------------------------------
    // cd <pasta> – mudar diretório
    // ------------------------------------------------------------
    if (strncmp(com, "cd ", 3) == 0) {
        char *path = com + 3;
        while (*path == ' ') path++;   // remove espaços iniciais
        if (strlen(path) == 0) {
            txt_printf("Uso: cd <pasta>\n");
            txt_printf("%s> ", cf);
            return;
        }
        // Monta caminho completo
        char fullpath[128];
        if (cf[strlen(cf)-1] == '/') {
            strcpy(fullpath, cf);
            strcat(fullpath, path);
        } else {
            strcpy(fullpath, cf);
            strcat(fullpath, "/");
            strcat(fullpath, path);
        }
        // Verifica se o diretório existe
        DIR testdir;
        FRESULT res = f_opendir(&testdir, fullpath);
        if (res == FR_OK) {
            f_closedir(&testdir);
            strcpy(cf, fullpath);
        } else {
            txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_RED);
            txt_printf("Erro: diretório não encontrado (%d)\n", res);
            txt_setcol(VGA_COL_BLACK, VGA_COL_WHITE);
        }
        txt_printf("%s> ", cf);
        return;
    }

    // ------------------------------------------------------------
    // disk (comando mantido mas apenas informativo)
    // ------------------------------------------------------------
    if (strncmp(com, "disk ", 5) == 0) {
        txt_printf("Apenas o disco 0: é suportado.\n");
        txt_printf("%s> ", cf);
        return;
    }

    // ------------------------------------------------------------
    // Caso contrário, tenta ler o nome como arquivo
    // ------------------------------------------------------------
    char fullpath[128];
    if (cf[strlen(cf)-1] == '/') {
        strcpy(fullpath, cf);
        strcat(fullpath, com);
    } else {
        strcpy(fullpath, cf);
        strcat(fullpath, "/");
        strcat(fullpath, com);
    }

    FIL file;
    FRESULT res = f_open(&file, fullpath, FA_READ);
    if (res != FR_OK) {
        txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_RED);
        txt_printf("Arquivo não encontrado ou erro: %d\n", res);
        txt_setcol(VGA_COL_BLACK, VGA_COL_WHITE);
        txt_printf("%s> ", cf);
        return;
    }

    // Lê e exibe o conteúdo do arquivo
    char buffer[256];
    UINT br;
    txt_printf("Conteúdo de %s:\n", com);
    while (f_read(&file, buffer, sizeof(buffer)-1, &br) == FR_OK && br > 0) {
        buffer[br] = '\0';
        txt_print(buffer);
    }
    f_close(&file);
    txt_newl();
    txt_printf("%s> ", cf);
}

// ============================================================
// kernel_main – ponto de entrada
// ============================================================
void kernel_main() {
    // Inicializa interrupções, teclado e serial
    isr_install();
    keyboard_init();
    serial_init();

    // Define o layout do teclado como EN-US (padrão)
    // Se você tiver a função keyboard_set_layout, chame-a:
    // keyboard_set_layout(LAYOUT_EN_US);
    // Caso contrário, o driver de teclado já deve usar EN-US por padrão.

    txt_clear();
    txt_print("UpdateOS V1.1!\n");

    // ------------------------------------------------------------
    // Monta o disco fixo "0:" (sem pergunta ao usuário)
    // ------------------------------------------------------------
    char path[] = "0:";          // dois caracteres + \0
    path[0] = disk;             // agora disk = '0'  (corrigido: antes era ==)
    FRESULT mount_res = f_mount(&fs, path, 0);

    if (mount_res == FR_OK) {
        txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_GREEN);
        txt_printf("Drive %c mounted successfully!\n", disk);
    } else {
        txt_setcol(VGA_COL_BLACK, VGA_COL_LIGHT_RED);
        txt_printf("Drive %c mount error: %d\n", disk, mount_res);
    }
    txt_setcol(VGA_COL_BLACK, VGA_COL_WHITE);

    // Diretório atual = raiz do disco (ex: "/")
    strcpy(cf, "/");   // ou "0:/" se preferir

    // ------------------------------------------------------------
    // Exibe ajuda e prompt
    // ------------------------------------------------------------
    txt_newl();
    txt_print("Type \"ls\" to list files\n");
    txt_print("Type \"cd <folder>\" to change folder\n");
    txt_print("Type <filename> to read a file\n");
    txt_printf("Current disk: %c\n", disk);
    txt_print("This System supports: Fat12/16/32 & ExFAT\n");
    txt_newl();
    txt_printf("%s> ", cf);

    // ------------------------------------------------------------
    // Loop principal do shell
    // ------------------------------------------------------------
    while (1) {
        KeyEvent key = keyboard_read();

        // Tecla normal (exceto Backspace, Tab, Enter)
        if (key.pressed && key.Char != '\b' && key.Char != '\t' && key.Char != '\n' && key.Char != '\0') {
            fn[pos++] = key.Char;
            txt_putc(key.Char);
        }
        // Enter → executa comando
        else if (key.Code == KEY_RETURN && key.pressed) {
            fn[pos] = '\0';          // finaliza a string
            txt_newl();
            command_exe(fn);
            clear_buffer(fn, sizeof(fn));  // limpa o buffer (preenche com zeros)
            pos = 0;
        }
        // Backspace → apaga caractere
        else if (key.Code == KEY_BACKSPACE && key.pressed && pos > 0) {
            pos--;
            fn[pos] = '\0';
            // Apaga o caractere da tela
            if (txt_curX > 0) {
                txt_curX--;
                txt_putc(' ');
                txt_curX--;
            }
        }
    }
}
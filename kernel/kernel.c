/*-----------------------------------------------------------------------*/
/* kernel.c - Ponto de entrada do UpdateOS                               */
/*                                                                       */
/* Testa a integração completa:                                          */
/*   ATA PIO  ->  diskio.c  ->  FatFS  ->  aplicação                     */
/*   RTC      ->  get_fattime()  ->  timestamps dos arquivos             */
/*-----------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#include "io.h"
#include "libs/string.h"
#include "libs/memory.h"
#include "drivers/serial.h"
#include "drivers/vga.h"
#include "drivers/atapio.h"
#include "drivers/rtc.h"
#include "drivers/fatfs/ff.h"

/* ==================================================================== */
/* Saída: usa serial e VGA ao mesmo tempo (se ambos existirem)          */
/* ==================================================================== */

static void kputs(const char *s) {
    serial_print(s);   /* ajuste o nome se o seu serial.h usa outro */
    vga_print(s);      /* ajuste o nome se o seu vga.h usa outro     */
}

static void kputc(char c) {
    char buf[2] = { c, 0 };
    kputs(buf);
}

static void kput_uint(uint32_t v) {
    char buf[12];
    int i = 10;
    buf[11] = 0;
    if (v == 0) { kputc('0'); return; }
    while (v > 0 && i >= 0) {
        buf[i--] = '0' + (v % 10);
        v /= 10;
    }
    kputs(&buf[i + 1]);
}

static void kput_2digits(uint8_t v) {
    kputc('0' + (v / 10));
    kputc('0' + (v % 10));
}

/* ==================================================================== */
/* Impressão de data/hora                                                */
/* ==================================================================== */

static void print_time(const char *label, const rtc_time_t *t) {
    kputs(label);
    kput_uint(t->year); kputc('-');
    kput_2digits(t->month); kputc('-');
    kput_2digits(t->day);   kputc(' ');
    kput_2digits(t->hour);  kputc(':');
    kput_2digits(t->minute);kputc(':');
    kput_2digits(t->second);
    kputs("\r\n");
}

/* ==================================================================== */
/* Tradução de FRESULT para texto                                        */
/* ==================================================================== */

static const char *fr_str(FRESULT fr) {
    switch (fr) {
    case FR_OK:                 return "OK";
    case FR_DISK_ERR:           return "erro de disco";
    case FR_INT_ERR:            return "erro interno";
    case FR_NOT_READY:          return "disco nao pronto";
    case FR_NO_FILE:            return "arquivo nao encontrado";
    case FR_NO_PATH:            return "caminho nao encontrado";
    case FR_INVALID_NAME:       return "nome invalido";
    case FR_DENIED:             return "acesso negado";
    case FR_EXIST:              return "ja existe";
    case FR_INVALID_OBJECT:     return "objeto invalido";
    case FR_WRITE_PROTECTED:    return "protegido contra escrita";
    case FR_INVALID_DRIVE:      return "drive invalido";
    case FR_NOT_ENABLED:        return "volume nao montado";
    case FR_NO_FILESYSTEM:      return "sem sistema de arquivos";
    case FR_MKFS_ABORTED:       return "formatacao abortada";
    case FR_TIMEOUT:            return "timeout";
    case FR_LOCKED:             return "arquivo travado";
    case FR_NOT_ENOUGH_CORE:    return "sem memoria";
    case FR_TOO_MANY_OPEN_FILES:return "muitos arquivos abertos";
    case FR_INVALID_PARAMETER:  return "parametro invalido";
    default:                    return "desconhecido";
    }
}

static void print_result(const char *what, FRESULT fr) {
    kputs(what);
    kputs(" -> ");
    kputs(fr_str(fr));
    kputs("\r\n");
}

/* ==================================================================== */
/* Testes do FatFS                                                       */
/* ==================================================================== */

static FATFS fs;

static void test_mount(void) {
    kputs("\r\n[1] Montando o sistema de arquivos...\r\n");
    FRESULT fr = f_mount(&fs, "", 1);
    print_result("  f_mount", fr);

    if (fr == FR_NO_FILESYSTEM) {
        kputs("  Disco sem FAT\r\n");
    }

    if (fr != FR_OK) {
        kputs("  !! FatFS indisponivel. Testes abortados.\r\n");
    }
}

static void test_write_read(void) {
    kputs("\r\n[2] Criando e escrevendo arquivo...\r\n");

    FIL f;
    UINT bw, br;
    FRESULT fr;

    fr = f_open(&f, "teste.txt", FA_CREATE_ALWAYS | FA_WRITE);
    print_result("  f_open", fr);
    if (fr != FR_OK) return;

    const char *msg = "Ola, UpdateOS! Este texto foi gravado no disco FAT32.\r\n";
    fr = f_write(&f, msg, (UINT)strlen(msg), &bw);
    print_result("  f_write", fr);
    kputs("  bytes escritos: "); kput_uint(bw); kputs("\r\n");

    f_close(&f);

    kputs("\r\n[3] Lendo o arquivo de volta...\r\n");
    char buf[128];

    fr = f_open(&f, "teste.txt", FA_READ);
    print_result("  f_open", fr);
    if (fr != FR_OK) return;

    fr = f_read(&f, buf, sizeof(buf) - 1, &br);
    print_result("  f_read", fr);
    buf[br] = '\0';
    kputs("  bytes lidos: "); kput_uint(br); kputs("\r\n");
    kputs("  conteudo: ");
    kputs(buf);

    f_close(&f);
}

static void test_mkdir_and_subfile(void) {
    kputs("\r\n[4] Criando pasta e arquivo dentro dela...\r\n");

    FRESULT fr = f_mkdir("dados");
    print_result("  f_mkdir(dados)", fr);

    FIL f;
    UINT bw;
    fr = f_open(&f, "dados/info.txt", FA_CREATE_ALWAYS | FA_WRITE);
    print_result("  f_open(dados/info.txt)", fr);
    if (fr == FR_OK) {
        const char *txt = "Arquivo dentro da pasta /dados.\r\n";
        f_write(&f, txt, (UINT)strlen(txt), &bw);
        f_close(&f);
    }
}

static void test_list_root(void) {
    kputs("\r\n[5] Listando a raiz do disco...\r\n");

    DIR dir;
    FILINFO fno;
    FRESULT fr = f_opendir(&dir, "");
    print_result("  f_opendir", fr);
    if (fr != FR_OK) return;

    int count = 0;
    for (;;) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break;

        count++;
        kputs("  ");
        kputs((fno.fattrib & AM_DIR) ? "[DIR]  " : "[FILE] ");
        kputs(fno.fname);
        kputs("  (");
        kput_uint(fno.fsize);
        kputs(" bytes)\r\n");
    }
    f_closedir(&dir);

    kputs("  total de entradas: "); kput_uint(count); kputs("\r\n");
}

static void test_stat(void) {
    kputs("\r\n[6] Consultando informacoes do arquivo...\r\n");

    FILINFO fno;
    FRESULT fr = f_stat("teste.txt", &fno);
    print_result("  f_stat(teste.txt)", fr);
    if (fr != FR_OK) return;

    kputs("  tamanho: "); kput_uint(fno.fsize); kputs(" bytes\r\n");
    kputs("  data   : ");
    kput_uint(1980 + (fno.fdate >> 9)); kputc('-');
    kput_2digits((fno.fdate >> 5) & 0x0F); kputc('-');
    kput_2digits(fno.fdate & 0x1F); kputs("\r\n");
    kputs("  hora   : ");
    kput_2digits((fno.ftime >> 11) & 0x1F); kputc(':');
    kput_2digits((fno.ftime >> 5) & 0x3F); kputc(':');
    kput_2digits((fno.ftime & 0x1F) * 2); kputs("\r\n");
}

static void test_rename_delete(void) {
    kputs("\r\n[7] Renomeando e removendo...\r\n");

    FRESULT fr = f_rename("teste.txt", "renomeado.txt");
    print_result("  f_rename", fr);

    fr = f_unlink("renomeado.txt");
    print_result("  f_unlink(renomeado.txt)", fr);

    fr = f_unlink("dados/info.txt");
    print_result("  f_unlink(dados/info.txt)", fr);

    fr = f_unlink("dados");
    print_result("  f_unlink(dados)", fr);
}

static void test_free_space(void) {
    kputs("\r\n[8] Espaco livre no disco...\r\n");

    DWORD fre_clust, fre_sect, tot_sect;
    FATFS *fsp;
    FRESULT fr = f_getfree("", &fre_clust, &fsp);
    print_result("  f_getfree", fr);
    if (fr != FR_OK) return;

    tot_sect = (fsp->n_fatent - 2) * fsp->csize;
    fre_sect = fre_clust * fsp->csize;

    kputs("  total: "); kput_uint(tot_sect / 2048); kputs(" MB\r\n");
    kputs("  livre: "); kput_uint(fre_sect / 2048); kputs(" MB\r\n");
}

/* ==================================================================== */
/* Banner inicial                                                        */
/* ==================================================================== */

static void banner(void) {
    kputs("\r\n");
    kputs("========================================\r\n");
    kputs("  UpdateOS - Teste de FatFS\r\n");
    kputs("========================================\r\n");
}

/* ==================================================================== */
/* ENTRY POINT                                                           */
/* ==================================================================== */

void kernel_main(void) {
    serial_init();
    /* vga_init(); -- descomente se o VGA precisar de setup explícito */

    banner();

    /* --- 1. RTC --- */
    rtc_init();
    rtc_time_t now;
    rtc_get_time(&now);
    print_time("Hora atual: ", &now);

    /* --- 2. ATA --- */
    kputs("\r\nInicializando discos ATA...\r\n");
    int n = ata_init();
    kputs("Discos detectados: "); kput_uint((uint32_t)n); kputs("\r\n");

    for (int i = 0; i < 4; i++) {
        if (!ata_present(i)) continue;
        kputs("  drive "); kputc('0' + i);
        kputs(": "); kputs("setores=");
        kput_uint(ata_get_sector_count(i));
        kputs(" (~"); kput_uint(ata_get_sector_count(i) / 2048);
        kputs(" MB)\r\n");
    }

    if (n == 0) {
        kputs("!! Nenhum disco ATA. Encerrando.\r\n");
        for (;;) __asm__ volatile("cli; hlt");
    }

    /* --- 3. FatFS --- */
    test_mount();
    test_write_read();
    test_mkdir_and_subfile();
    test_list_root();
    test_stat();
    test_free_space();
    test_rename_delete();
    test_list_root();   /* de novo, para confirmar que ficou vazio */

    kputs("\r\n========================================\r\n");
    kputs("  Testes concluidos.\r\n");
    kputs("========================================\r\n\r\n");

    /* Desmonta antes de encerrar */
    f_mount(NULL, "", 0);

    /* Loop final */
    for (;;) __asm__ volatile("cli; hlt");
}
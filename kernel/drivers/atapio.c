/*-----------------------------------------------------------------------*/
/* atapio.c - Driver ATA PIO (LBA28) para o UpdateOS                     */
/*-----------------------------------------------------------------------*/
#include "atapio.h"
#include "io.h"
#include "../libs/string.h"

static ata_drive_t drives[4];

/* Escolhe porta base e valor do registrador DRIVE a partir do índice */
static void ata_pick(int drive, uint16_t *base, uint8_t *drive_sel) {
    switch (drive) {
    case 0:  *base = ATA_PRIMARY_IO;   *drive_sel = 0xE0; break; /* master prim, LBA */
    case 1:  *base = ATA_PRIMARY_IO;   *drive_sel = 0xF0; break; /* slave  prim, LBA */
    case 2:  *base = ATA_SECONDARY_IO; *drive_sel = 0xE0; break; /* master sec,  LBA */
    default: *base = ATA_SECONDARY_IO; *drive_sel = 0xF0; break; /* slave  sec,  LBA */
    }
}

/* Atraso padrão ATA: 4 leituras do alternate status ≈ 400ns */
static inline void ata_delay400(void) {
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
}

/* Espera BSY = 0 */
static int ata_wait_bsy(uint16_t base) {
    for (int i = 0; i < 1000000; i++) {
        if (!(inb(base + ATA_REG_STATUS) & ATA_SR_BSY)) return 0;
    }
    return -1;
}

/* Espera BSY = 0 e DRQ = 1 (ou erro) */
static int ata_wait_drq(uint16_t base) {
    for (int i = 0; i < 1000000; i++) {
        uint8_t st = inb(base + ATA_REG_STATUS);
        if (st & (ATA_SR_ERR | ATA_SR_DF)) return -1;
        if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ)) return 0;
    }
    return -1;
}

/* IDENTIFY DEVICE */
static int ata_identify(uint16_t base, uint8_t drive_sel, ata_drive_t *out) {
    out->present = 0;
    out->sectors = 0;
    out->model[0] = '\0';

    outb(base + ATA_REG_DRIVE, drive_sel);
    ata_delay400();

    /* Zera os registradores de LBA e contagem (exigido antes do IDENTIFY) */
    outb(base + ATA_REG_SECCOUNT, 0);
    outb(base + ATA_REG_LBA0, 0);
    outb(base + ATA_REG_LBA1, 0);
    outb(base + ATA_REG_LBA2, 0);

    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay400();

    /* Se status == 0, não há drive neste canal */
    if (inb(base + ATA_REG_STATUS) == 0) return -1;
    if (ata_wait_bsy(base) < 0) return -1;

    /* Se LBA1 ou LBA2 != 0, é ATAPI (não tratamos aqui) */
    if (inb(base + ATA_REG_LBA1) || inb(base + ATA_REG_LBA2)) return -1;

    if (ata_wait_drq(base) < 0) return -1;

    /* Lê 256 palavras (512 bytes) */
    uint16_t id[256];
    for (int i = 0; i < 256; i++)
        id[i] = inw(base + ATA_REG_DATA);

    /* Modelo: palavras 27..46, string com bytes trocados */
    for (int i = 0; i < 20; i++) {
        out->model[i * 2]     = (char)(id[27 + i] >> 8);
        out->model[i * 2 + 1] = (char)(id[27 + i] & 0xFF);
    }
    out->model[40] = '\0';

    /* Capacidade LBA28: palavras 60 (low) e 61 (high) */
    out->sectors = ((uint32_t)id[61] << 16) | id[60];
    out->present = 1;
    return 0;
}

int ata_init(void) {
    memset(drives, 0, sizeof(drives));

    ata_identify(ATA_PRIMARY_IO,   0xA0, &drives[0]);
    ata_identify(ATA_PRIMARY_IO,   0xB0, &drives[1]);
    ata_identify(ATA_SECONDARY_IO, 0xA0, &drives[2]);
    ata_identify(ATA_SECONDARY_IO, 0xB0, &drives[3]);

    int n = 0;
    for (int i = 0; i < 4; i++) if (drives[i].present) n++;
    return n;
}

int ata_present(int drive) {
    if (drive < 0 || drive > 3) return 0;
    return drives[drive].present;
}

uint32_t ata_get_sector_count(int drive) {
    if (drive < 0 || drive > 3) return 0;
    return drives[drive].sectors;
}

int ata_read_sectors(int drive, uint32_t lba, uint8_t count, void *buf) {
    if (drive < 0 || drive > 3 || !drives[drive].present) return -1;
    if (count == 0) return 0;
    if (lba + count > drives[drive].sectors) return -1;

    uint16_t base; uint8_t dsel;
    ata_pick(drive, &base, &dsel);

    /* LBA28: bits 24..27 vão no registrador DRIVE */
    uint8_t head = dsel | ((lba >> 24) & 0x0F);

    if (ata_wait_bsy(base) < 0) return -1;

    outb(base + ATA_REG_DRIVE, head);
    ata_delay400();
    outb(base + ATA_REG_SECCOUNT, count);
    outb(base + ATA_REG_LBA0, (uint8_t)( lba        & 0xFF));
    outb(base + ATA_REG_LBA1, (uint8_t)((lba >>  8) & 0xFF));
    outb(base + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ);
    ata_delay400();

    uint16_t *p = (uint16_t *)buf;
    for (int s = 0; s < count; s++) {
        if (ata_wait_drq(base) < 0) return -1;
        for (int i = 0; i < 256; i++)
            *p++ = inw(base + ATA_REG_DATA);
        ata_delay400();
    }
    return 0;
}

int ata_write_sectors(int drive, uint32_t lba, uint8_t count, const void *buf) {
    if (drive < 0 || drive > 3 || !drives[drive].present) return -1;
    if (count == 0) return 0;
    if (lba + count > drives[drive].sectors) return -1;

    uint16_t base; uint8_t dsel;
    ata_pick(drive, &base, &dsel);

    uint8_t head = dsel | ((lba >> 24) & 0x0F);

    if (ata_wait_bsy(base) < 0) return -1;

    outb(base + ATA_REG_DRIVE, head);
    ata_delay400();
    outb(base + ATA_REG_SECCOUNT, count);
    outb(base + ATA_REG_LBA0, (uint8_t)( lba        & 0xFF));
    outb(base + ATA_REG_LBA1, (uint8_t)((lba >>  8) & 0xFF));
    outb(base + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE);
    ata_delay400();

    const uint16_t *p = (const uint16_t *)buf;
    for (int s = 0; s < count; s++) {
        if (ata_wait_drq(base) < 0) return -1;
        for (int i = 0; i < 256; i++)
            outw(base + ATA_REG_DATA, *p++);
        ata_delay400();
    }

    /* Flush cache para garantir gravação física */
    outb(base + ATA_REG_COMMAND, ATA_CMD_FLUSH);
    ata_delay400();
    ata_wait_bsy(base);

    return 0;
}
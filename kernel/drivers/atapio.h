#ifndef KERNEL_ATAPIO_H
#define KERNEL_ATAPIO_H

#include <stdint.h>
#include <stddef.h>

#define ATA_SECTOR_SIZE     512

/* Barramentos IDE */
#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_IO    0x170
#define ATA_SECONDARY_CTRL  0x376

/* Offsets de registro a partir do base */
#define ATA_REG_DATA        0
#define ATA_REG_ERROR       1
#define ATA_REG_FEATURES    1
#define ATA_REG_SECCOUNT    2
#define ATA_REG_LBA0        3
#define ATA_REG_LBA1        4
#define ATA_REG_LBA2        5
#define ATA_REG_DRIVE       6
#define ATA_REG_STATUS      7
#define ATA_REG_COMMAND     7

/* Bits de status */
#define ATA_SR_BSY   0x80
#define ATA_SR_DRDY  0x40
#define ATA_SR_DF    0x20
#define ATA_SR_DSC   0x10
#define ATA_SR_DRQ   0x08
#define ATA_SR_ERR   0x01

/* Comandos */
#define ATA_CMD_READ        0x20
#define ATA_CMD_WRITE       0x30
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_FLUSH       0xE7

/* Índice global de drive:
 *   0 = master primário     1 = slave primário
 *   2 = master secundário   3 = slave secundário
 */
typedef struct {
    int      present;
    uint32_t sectors;
    char     model[41];
} ata_drive_t;

int      ata_init(void);                        /* Retorna nº de drives detectados */
int      ata_present(int drive);
uint32_t ata_get_sector_count(int drive);
int      ata_read_sectors (int drive, uint32_t lba, uint8_t count, void *buf);
int      ata_write_sectors(int drive, uint32_t lba, uint8_t count, const void *buf);

#endif /* KERNEL_ATAPIO_H */
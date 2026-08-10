#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// Configuração de Debug
// ============================================================
#define ATA_DEBUG 1   // 1 = ativa logs, 0 = desliga

// ============================================================
// Definições de hardware
// ============================================================
#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_IO    0x170
#define ATA_SECONDARY_CTRL  0x376

#define ATA_REG_DATA        0
#define ATA_REG_ERROR       1
#define ATA_REG_FEATURES    1
#define ATA_REG_SECTOR_COUNT 2
#define ATA_REG_LBA_LOW     3
#define ATA_REG_LBA_MID     4
#define ATA_REG_LBA_HIGH    5
#define ATA_REG_DRIVE_HEAD  6
#define ATA_REG_STATUS      7
#define ATA_REG_COMMAND     7

#define ATA_REG_ALT_STATUS  0
#define ATA_REG_DEVICE_CTRL 0

// ============================================================
// Comandos ATA
// ============================================================
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_SET_FEATURES    0xEF
#define ATA_CMD_FLUSH_CACHE     0xE7
#define ATA_CMD_FLUSH_CACHE_EXT 0xEA
#define ATA_CMD_STANDBY         0xE2
#define ATA_CMD_IDLE            0xE3
#define ATA_CMD_SLEEP           0xE6

// ============================================================
// Bits de Status
// ============================================================
#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF   0x20
#define ATA_SR_DSC  0x10
#define ATA_SR_DRQ  0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX  0x02
#define ATA_SR_ERR  0x01

// ============================================================
// Bits de Erro
// ============================================================
#define ATA_ER_BBK   0x80
#define ATA_ER_UNC   0x40
#define ATA_ER_MC    0x20
#define ATA_ER_IDNF  0x10
#define ATA_ER_MCR   0x08
#define ATA_ER_ABRT  0x04
#define ATA_ER_TK0NF 0x02
#define ATA_ER_AMNF  0x01

// ============================================================
// Estrutura de informações do drive
// ============================================================
typedef struct {
    bool present;
    uint8_t channel;          // 0=primário, 1=secundário
    uint8_t drive;            // 0=master, 1=slave
    char model[41];
    uint32_t sectors;         // capacidade LBA28 (se disponível)
    uint64_t sectors48;       // capacidade LBA48
    bool supports_lba;
    bool supports_lba48;
    bool supports_dma;
    bool supports_udma;
    uint16_t major_version;
    uint16_t minor_version;
    uint8_t  max_udma_mode;
} ata_drive_info_t;

// ============================================================
// Funções públicas
// ============================================================

// Inicializa o subsistema ATA (detecta drives)
void ata_init(void);

// Retorna informação de um drive (0-3). NULL se não presente.
const ata_drive_info_t* ata_get_drive_info(uint8_t drive_index);

// Lê um setor (512 bytes) com retries e fallback
int ata_read_sector(uint8_t drive_index, uint32_t lba, uint8_t *buffer);

// Lê múltiplos setores
int ata_read_sectors(uint8_t drive_index, uint32_t lba, uint32_t count, uint8_t *buffer);

// Escreve um setor
int ata_write_sector(uint8_t drive_index, uint32_t lba, const uint8_t *buffer);

// Escreve múltiplos setores
int ata_write_sectors(uint8_t drive_index, uint32_t lba, uint32_t count, const uint8_t *buffer);

// Flush do cache
int ata_flush_cache(uint8_t drive_index);

// Reset de um canal
void ata_reset_channel(uint8_t channel);

// Imprime informações de todos os drives
void ata_print_info(void);

// Função de diagnóstico: tenta ler setor 0 e exibe detalhes
void ata_diagnose(uint8_t drive_index);

#endif
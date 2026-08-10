#include "ata.h"
#include "../libs/io.h"
#include "../drivers/video.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// ============================================================
// Configurações de timeout e retry
// ============================================================
#define ATA_TIMEOUT_SHORT   100000   // 0.1s (aprox)
#define ATA_TIMEOUT_LONG    5000000  // 5s
#define ATA_MAX_RETRIES     5
#define ATA_RETRY_DELAY     100000   // pausa entre retries

// ============================================================
// Macros de debug
// ============================================================
#if ATA_DEBUG
    #define ATA_LOG(msg)          serial_print(msg)
    #define ATA_LOG_INT(val)      serial_print_int(val)
    #define ATA_LOG_HEX(val)      serial_print_hex(val)
    #define ATA_LOG_STR(str)      serial_print(str)
    #define ATA_LOG_NEWLINE()     serial_print("\n")
#else
    #define ATA_LOG(msg)
    #define ATA_LOG_INT(val)
    #define ATA_LOG_HEX(val)
    #define ATA_LOG_STR(str)
    #define ATA_LOG_NEWLINE()
#endif

// ============================================================
// Variáveis estáticas
// ============================================================
static ata_drive_info_t drives[4];
static bool ata_initialized = false;
static bool ata_debug_enabled = ATA_DEBUG;

// ============================================================
// Funções auxiliares de I/O
// ============================================================
static inline uint16_t ata_io_base(uint8_t channel) {
    return (channel == 0) ? ATA_PRIMARY_IO : ATA_SECONDARY_IO;
}

static inline uint16_t ata_ctrl_base(uint8_t channel) {
    return (channel == 0) ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL;
}

// ============================================================
// Funções de polling com timeout e logs
// ============================================================
static int ata_wait_ready(uint8_t channel, uint32_t timeout) {
    uint16_t base = ata_io_base(channel);
    uint32_t t = 0;
    while (t < timeout) {
        uint8_t status = inb(base + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) {
                ATA_LOG("ata_wait_ready: ERR, status=0x"); ATA_LOG_HEX(status); ATA_LOG_NEWLINE();
                return -1;
            }
            return 0;
        }
        t++;
    }
    ATA_LOG("ata_wait_ready: TIMEOUT\n");
    return -1;
}

static int ata_wait_drq(uint8_t channel, uint32_t timeout) {
    uint16_t base = ata_io_base(channel);
    uint32_t t = 0;
    while (t < timeout) {
        uint8_t status = inb(base + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            ATA_LOG("ata_wait_drq: ERR, status=0x"); ATA_LOG_HEX(status); ATA_LOG_NEWLINE();
            return -1;
        }
        if ((status & (ATA_SR_BSY | ATA_SR_DRQ)) == ATA_SR_DRQ) {
            return 0;
        }
        t++;
    }
    ATA_LOG("ata_wait_drq: TIMEOUT\n");
    return -1;
}

static int ata_wait_complete(uint8_t channel, uint32_t timeout) {
    uint16_t base = ata_io_base(channel);
    uint32_t t = 0;
    while (t < timeout) {
        uint8_t status = inb(base + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            ATA_LOG("ata_wait_complete: ERR, status=0x"); ATA_LOG_HEX(status); ATA_LOG_NEWLINE();
            return -1;
        }
        if (!(status & ATA_SR_BSY)) {
            return 0;
        }
        t++;
    }
    ATA_LOG("ata_wait_complete: TIMEOUT\n");
    return -1;
}

// ============================================================
// Seleção do drive (LBA)
// ============================================================
static void ata_select_drive(uint8_t channel, uint8_t drive, uint8_t lba_high) {
    uint16_t base = ata_io_base(channel);
    uint8_t dev = 0xE0 | (drive ? 0x10 : 0x00); // LBA=1, drive=0/1
    dev |= (lba_high & 0x0F);
    outb(base + ATA_REG_DRIVE_HEAD, dev);
    ATA_LOG("ata_select_drive: drive="); ATA_LOG_INT(drive);
    ATA_LOG(" dev=0x"); ATA_LOG_HEX(dev); ATA_LOG_NEWLINE();
}

// ============================================================
// IDENTIFY DEVICE
// ============================================================
static int ata_identify_device(uint8_t channel, uint8_t drive, uint16_t *buffer) {
    uint16_t base = ata_io_base(channel);

    // Seleciona drive
    ata_select_drive(channel, drive, 0);

    // Limpa registros
    outb(base + ATA_REG_SECTOR_COUNT, 0);
    outb(base + ATA_REG_LBA_LOW, 0);
    outb(base + ATA_REG_LBA_MID, 0);
    outb(base + ATA_REG_LBA_HIGH, 0);

    // Envia comando IDENTIFY
    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    // Verifica presença (status != 0)
    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status == 0) {
        ATA_LOG("ata_identify: drive não presente (status=0)\n");
        return -1;
    }

    // Espera DRQ
    if (ata_wait_drq(channel, ATA_TIMEOUT_LONG) != 0) {
        ATA_LOG("ata_identify: falha wait_drq\n");
        return -1;
    }

    // Lê 256 words
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(base + ATA_REG_DATA);
    }

    // Verifica se houve erro
    if (inb(base + ATA_REG_STATUS) & ATA_SR_ERR) {
        ATA_LOG("ata_identify: erro após leitura\n");
        return -1;
    }

    ATA_LOG("ata_identify: sucesso\n");
    return 0;
}

// ============================================================
// Leitura PIO (com logs detalhados)
// ============================================================
static int ata_read_sector_pio(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t *buffer) {
    uint16_t base = ata_io_base(channel);

    ATA_LOG("ata_read_pio: ch="); ATA_LOG_INT(channel);
    ATA_LOG(" drv="); ATA_LOG_INT(drive);
    ATA_LOG(" lba="); ATA_LOG_INT(lba); ATA_LOG_NEWLINE();

    // Wait ready
    if (ata_wait_ready(channel, ATA_TIMEOUT_LONG) != 0) {
        ATA_LOG("ata_read_pio: wait_ready failed\n");
        return -1;
    }

    // Select drive and LBA
    ata_select_drive(channel, drive, (lba >> 24) & 0x0F);

    // Write parameters
    outb(base + ATA_REG_SECTOR_COUNT, 1);
    outb(base + ATA_REG_LBA_LOW,   (lba & 0xFF));
    outb(base + ATA_REG_LBA_MID,   (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH,  (lba >> 16) & 0xFF);

    // Send command
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    ATA_LOG("ata_read_pio: comando enviado\n");

    // Wait for DRQ
    if (ata_wait_drq(channel, ATA_TIMEOUT_LONG) != 0) {
        ATA_LOG("ata_read_pio: wait_drq failed\n");
        // Lê erro para diagnóstico
        uint8_t err = inb(base + ATA_REG_ERROR);
        ATA_LOG("ata_read_pio: erro=0x"); ATA_LOG_HEX(err); ATA_LOG_NEWLINE();
        return -1;
    }

    // Read data
    uint16_t *buf16 = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        buf16[i] = inw(base + ATA_REG_DATA);
    }

    // Check final status
    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status & ATA_SR_ERR) {
        ATA_LOG("ata_read_pio: erro final status=0x"); ATA_LOG_HEX(status); ATA_LOG_NEWLINE();
        return -1;
    }

    ATA_LOG("ata_read_pio: sucesso\n");
    return 0;
}

// ============================================================
// Funções públicas
// ============================================================

void ata_init(void) {
    if (ata_initialized) return;

    ATA_LOG("ata_init: detectando drives...\n");

    memset(drives, 0, sizeof(drives));

    // Tenta resetar ambos os canais primeiro
    ata_reset_channel(0);
    ata_reset_channel(1);

    for (uint8_t ch = 0; ch < 2; ch++) {
        for (uint8_t drv = 0; drv < 2; drv++) {
            uint8_t idx = ch * 2 + drv;
            uint16_t ident[256];

            if (ata_identify_device(ch, drv, ident) == 0) {
                drives[idx].present = true;
                drives[idx].channel = ch;
                drives[idx].drive = drv;

                // Modelo
                char model[41];
                for (int i = 0; i < 20; i++) {
                    uint16_t w = ident[27 + i];
                    model[2*i]   = w >> 8;
                    model[2*i+1] = w & 0xFF;
                }
                model[40] = '\0';
                // Remove espaços à direita
                int len = 40;
                while (len > 0 && model[len-1] == ' ') model[--len] = '\0';
                strcpy(drives[idx].model, model);

                // Capacidades
                drives[idx].sectors = ident[60] | (ident[61] << 16);
                drives[idx].sectors48 = ((uint64_t)(ident[102] | (ident[103] << 16)) << 32) |
                                         (ident[100] | (ident[101] << 16));
                drives[idx].supports_lba = (ident[49] & (1 << 9)) != 0;
                drives[idx].supports_lba48 = (ident[83] & (1 << 10)) != 0;
                drives[idx].supports_dma = (ident[49] & (1 << 8)) != 0;
                drives[idx].supports_udma = (ident[88] & 0x00FF) != 0;
                drives[idx].major_version = ident[80];
                drives[idx].minor_version = ident[81];
                drives[idx].max_udma_mode = 0;
                if (drives[idx].supports_udma) {
                    // Encontra o modo UDMA mais alto suportado
                    uint16_t udma_modes = ident[88];
                    for (int m = 6; m >= 0; m--) {
                        if (udma_modes & (1 << m)) {
                            drives[idx].max_udma_mode = m;
                            break;
                        }
                    }
                }

                ATA_LOG("ata_init: drive encontrado: "); ATA_LOG_STR(drives[idx].model); ATA_LOG_NEWLINE();
            }
        }
    }

    ata_initialized = true;
    ATA_LOG("ata_init: concluído\n");
}

const ata_drive_info_t* ata_get_drive_info(uint8_t drive_index) {
    if (!ata_initialized) ata_init();
    if (drive_index >= 4) return NULL;
    return drives[drive_index].present ? &drives[drive_index] : NULL;
}

int ata_read_sector(uint8_t drive_index, uint32_t lba, uint8_t *buffer) {
    if (!ata_initialized) ata_init();
    if (drive_index >= 4 || !drives[drive_index].present) {
        ATA_LOG("ata_read_sector: drive inválido ou ausente\n");
        return -1;
    }

    uint8_t ch = drives[drive_index].channel;
    uint8_t drv = drives[drive_index].drive;

    // Retry loop com backoff
    int retries = ATA_MAX_RETRIES;
    while (retries > 0) {
        int result = ata_read_sector_pio(ch, drv, lba, buffer);
        if (result == 0) {
            return 0;
        }
        retries--;
        if (retries > 0) {
            ATA_LOG("ata_read_sector: tentativa falhou, retentando...\n");
            // Pausa antes de tentar novamente
            for (volatile int i = 0; i < ATA_RETRY_DELAY; i++);
            // Tenta resetar o canal
            ata_reset_channel(ch);
        }
    }

    ATA_LOG("ata_read_sector: todas as tentativas falharam\n");
    return -1;
}

int ata_read_sectors(uint8_t drive_index, uint32_t lba, uint32_t count, uint8_t *buffer) {
    for (uint32_t i = 0; i < count; i++) {
        if (ata_read_sector(drive_index, lba + i, buffer + i * 512) != 0) {
            return -1;
        }
    }
    return 0;
}

int ata_write_sector(uint8_t drive_index, uint32_t lba, const uint8_t *buffer) {
    // Implementação PIO Write (similar ao read)
    // Por enquanto, retorna erro se não implementado
    return -1;
}

int ata_write_sectors(uint8_t drive_index, uint32_t lba, uint32_t count, const uint8_t *buffer) {
    for (uint32_t i = 0; i < count; i++) {
        if (ata_write_sector(drive_index, lba + i, buffer + i * 512) != 0) {
            return -1;
        }
    }
    return 0;
}

int ata_flush_cache(uint8_t drive_index) {
    if (!ata_initialized) ata_init();
    if (drive_index >= 4 || !drives[drive_index].present) return -1;

    uint8_t ch = drives[drive_index].channel;
    uint16_t base = ata_io_base(ch);

    ata_select_drive(ch, drives[drive_index].drive, 0);
    outb(base + ATA_REG_COMMAND, ATA_CMD_FLUSH_CACHE);

    if (ata_wait_complete(ch, ATA_TIMEOUT_LONG) != 0) {
        return -1;
    }
    return 0;
}

void ata_reset_channel(uint8_t channel) {
    uint16_t ctrl = ata_ctrl_base(channel);
    ATA_LOG("ata_reset_channel: resetando canal "); ATA_LOG_INT(channel); ATA_LOG_NEWLINE();

    // Ativa SRST
    outb(ctrl + ATA_REG_DEVICE_CTRL, 0x04);
    // Aguarda 100ms
    for (volatile int i = 0; i < 200000; i++);
    // Desativa SRST
    outb(ctrl + ATA_REG_DEVICE_CTRL, 0x00);
    // Aguarda recuperação
    ata_wait_ready(channel, ATA_TIMEOUT_LONG);

    ATA_LOG("ata_reset_channel: concluído\n");
}

void ata_print_info(void) {
    ata_init();
    serial_print("ATA Drives detected:\n");
    for (int i = 0; i < 4; i++) {
        if (drives[i].present) {
            serial_print("  Drive ");
            serial_print_int(i);
            serial_print(": ");
            serial_print(drives[i].model);
            serial_print(" (");
            serial_print_int(drives[i].sectors);
            serial_print(" sectors, LBA: ");
            serial_print(drives[i].supports_lba ? "yes" : "no");
            serial_print(", LBA48: ");
            serial_print(drives[i].supports_lba48 ? "yes" : "no");
            serial_print(", UDMA");
            if (drives[i].supports_udma) {
                serial_print(drives[i].max_udma_mode);
            } else {
                serial_print("?");
            }
            serial_print(")\n");
        }
    }
}

void ata_diagnose(uint8_t drive_index) {
    serial_print("=== ATA Diagnose for drive ");
    serial_print_int(drive_index);
    serial_print(" ===\n");

    const ata_drive_info_t* info = ata_get_drive_info(drive_index);
    if (!info) {
        serial_print("Drive not present.\n");
        return;
    }

    serial_print("Model: ");
    serial_print(info->model);
    serial_print("\n");
    serial_print("Sectors (LBA28): ");
    serial_print_int(info->sectors);
    serial_print("\n");
    serial_print("Sectors (LBA48): ");
    serial_print_int(info->sectors48);
    serial_print("\n");
    serial_print("LBA: ");
    serial_print(info->supports_lba ? "yes" : "no");
    serial_print("\n");
    serial_print("LBA48: ");
    serial_print(info->supports_lba48 ? "yes" : "no");
    serial_print("\n");
    serial_print("UDMA: ");
    serial_print(info->supports_udma ? "yes" : "no");
    serial_print("\n");

    // Testa leitura do setor 0
    uint8_t sector[512];
    serial_print("Tentando ler setor 0...\n");
    int result = ata_read_sector(drive_index, 0, sector);
    serial_print("Resultado: ");
    serial_print_int(result);
    serial_print("\n");

    if (result == 0) {
        serial_print("Primeiros 16 bytes: ");
        for (int i = 0; i < 16; i++) {
            serial_print_hex(sector[i]);
            serial_putc(' ');
        }
        serial_print("\n");
    }
}
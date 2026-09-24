#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include <stdint.h>
#include <stddef.h>
#include "drivers/serial.h"

/* ============================================================
 *  OUT — envia dados para uma porta de I/O
 * ============================================================ */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* ============================================================
 *  IN — lê dados de uma porta de I/O
 * ============================================================ */

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* ============================================================
 *  I/O WAIT — pequeno atraso para dar tempo ao PIC/barramento
 *  Escrever em uma porta não usada (0x80) consome ~1 ciclo.
 * ============================================================ */

static inline void io_wait(void) {
    outb(0x80, 0);
}

/* ============================================================
 *  Alternativa portátil de io_wait (caso prefira)
 * ============================================================ */

static inline void io_wait_alt(void) {
    __asm__ volatile ("jmp 1f\n1:");
}

/* ============================================================
 *  SHUTDOWN e REBOOT
 *
 *  Estratégia:
 *    1. Tenta ACPI (porta PM1a_CNT_BLK + SLP_TYPa).
 *    2. Se falhar, tenta métodos específicos de emulador (QEMU).
 *    3. Para reboot, usa o registrador de reset do FADT ou o 8042.
 *    4. Como último recurso, causa um triple fault.
 * ============================================================ */

/* --- Constantes ACPI --- */
#define ACPI_SLP_EN         (1 << 13)
#define ACPI_SCI_EN         (1 << 0)
#define ACPI_PM1_CNT_LEN    2   /* Tamanho típico do PM1_CNT */

/* --- Portas específicas de emuladores --- */
#define QEMU_SHUTDOWN_PORT  0x604
#define QEMU_SHUTDOWN_VAL   0x2000
#define QEMU_DEBUG_EXIT_PORT 0x501
#define QEMU_DEBUG_EXIT_VAL  0x01  /* Código de saída (status = 2*N+1) */

/* --- Estruturas ACPI (simplificadas) --- */
typedef struct {
    char     signature[8];
    uint8_t  checksum;
    char     oem_id[6];
    uint8_t  revision;
    uint32_t rsdt_address;
} __attribute__((packed)) acpi_rsdp_t;

typedef struct {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct {
    acpi_sdt_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t  reserved;
    uint8_t  preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t  acpi_enable;
    uint8_t  acpi_disable;
    uint8_t  s4bios_req;
    uint8_t  pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t  pm1_evt_len;
    uint8_t  pm1_cnt_len;
    uint8_t  pm2_cnt_len;
    uint8_t  pm_tmr_len;
    uint8_t  gpe0_blk_len;
    uint8_t  gpe1_blk_len;
    uint8_t  gpe1_base;
    uint8_t  cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t  duty_offset;
    uint8_t  duty_width;
    uint8_t  day_alrm;
    uint8_t  mon_alrm;
    uint8_t  century;
    uint16_t iapc_boot_arch;
    uint8_t  reserved2;
    uint32_t flags;
    /* ... outros campos podem ser adicionados se necessário ... */
} __attribute__((packed)) acpi_fadt_t;

/* --- Variáveis globais para ACPI (definidas em algum .c se preferir) --- */
static uint32_t *acpi_pm1a_cnt = 0;
static uint32_t *acpi_pm1b_cnt = 0;
static uint16_t  acpi_slp_typa = 0;
static uint16_t  acpi_slp_typb = 0;
static int       acpi_ready = 0;

/* --- Função auxiliar para procurar string na memória --- */
static void *acpi_memfind(const void *haystack, size_t haystack_len,
                          const char *needle, size_t needle_len) {
    const uint8_t *h = (const uint8_t *)haystack;
    for (size_t i = 0; i + needle_len <= haystack_len; i++) {
        if (__builtin_memcmp(h + i, needle, needle_len) == 0) {
            return (void *)(h + i);
        }
    }
    return 0;
}

/* --- Verificador ACPI (chame uma vez no boot) --- */
static void acpi_init(void) {
    /* 1. Procura a RSDP na área de BIOS (0xE0000–0xFFFFF) */
    acpi_rsdp_t *rsdp = 0;
    for (uint32_t addr = 0xE0000; addr < 0x100000; addr += 16) {
        acpi_rsdp_t *p = (acpi_rsdp_t *)addr;
        if (__builtin_memcmp(p->signature, "RSD PTR ", 8) == 0) {
            /* Verifica checksum simples (soma dos 20 bytes deve ser 0) */
            uint8_t sum = 0;
            for (int i = 0; i < 20; i++) sum += ((uint8_t *)p)[i];
            if (sum == 0) { rsdp = p; break; }
        }
    }
    if (!rsdp) return;

    /* 2. Pega a RSDT */
    acpi_sdt_header_t *rsdt = (acpi_sdt_header_t *)(uintptr_t)rsdp->rsdt_address;
    if (!rsdt) return;

    /* 3. Percorre as tabelas da RSDT procurando FACP (FADT) */
    uint32_t entries = (rsdt->length - sizeof(acpi_sdt_header_t)) / 4;
    uint32_t *ptrs = (uint32_t *)((uint8_t *)rsdt + sizeof(acpi_sdt_header_t));

    acpi_fadt_t *fadt = 0;
    for (uint32_t i = 0; i < entries; i++) {
        acpi_sdt_header_t *hdr = (acpi_sdt_header_t *)(uintptr_t)ptrs[i];
        if (__builtin_memcmp(hdr->signature, "FACP", 4) == 0) {
            fadt = (acpi_fadt_t *)hdr;
            break;
        }
    }
    if (!fadt) return;

    /* 4. Pega os endereços dos blocos PM1 */
    acpi_pm1a_cnt = (uint32_t *)(uintptr_t)fadt->pm1a_cnt_blk;
    if (fadt->pm1b_cnt_blk) {
        acpi_pm1b_cnt = (uint32_t *)(uintptr_t)fadt->pm1b_cnt_blk;
    }

    /* 5. Procura o objeto _S5 no DSDT para obter SLP_TYPa */
    acpi_sdt_header_t *dsdt = (acpi_sdt_header_t *)(uintptr_t)fadt->dsdt;
    if (!dsdt) return;

    /* Assinatura do objeto _S5: 0x08 0x5F 0x53 0x35 0x5F ("\_S5_") */
    const uint8_t s5_sig[5] = {0x08, 0x5F, 0x53, 0x35, 0x5F};
    uint8_t *found = (uint8_t *)acpi_memfind((uint8_t *)dsdt + sizeof(acpi_sdt_header_t),
                                            dsdt->length - sizeof(acpi_sdt_header_t),
                                            (const char *)s5_sig, 5);
    if (!found) return;

    /* O SLP_TYPa está 4 bytes depois do início do objeto _S5.
       A estrutura típica é: 0x12 (PackageOp), PkgLength, NumElements,
       0x0A (byte prefix), SLP_TYPa, ... */
    uint8_t *p = found + 5; /* Pula a assinatura */
    /* Pula PackageOp (0x12), PkgLength, NumElements */
    while (*p == 0x12 || (*p >= 0x00 && *p <= 0x0F) || *p == 0x0A) p++;
    /* Agora p aponta para o primeiro byte do SLP_TYPa */
    acpi_slp_typa = *p & 0x07; /* SLP_TYPx são bits 10-12 */

    if (fadt->pm1b_cnt_blk) {
        acpi_slp_typb = *(p + 1) & 0x07;
    }

    acpi_ready = 1;
}

/* --- Desliga o computador --- */
static inline void shutdown(void) {
    /* 1. Tenta ACPI */
    if (acpi_ready && acpi_pm1a_cnt) {
        uint16_t val = (acpi_slp_typa << 10) | ACPI_SLP_EN;
        outw((uint16_t)(uintptr_t)acpi_pm1a_cnt, val);
        if (acpi_pm1b_cnt) {
            outw((uint16_t)(uintptr_t)acpi_pm1b_cnt,
                 (acpi_slp_typb << 10) | ACPI_SLP_EN);
        }
        /* Se ainda estamos aqui, ACPI falhou ou não é suportado */
    }

    /* 2. QEMU (versões novas) */
    outw(QEMU_SHUTDOWN_PORT, QEMU_SHUTDOWN_VAL);

    /* 3. QEMU/Bochs (antigo) */
    outw(0xB004, 0x2000);

    /* 4. VirtualBox */
    outw(0x4004, 0x3400);

    /* 5. Cloud Hypervisor */
    outw(0x600, 0x34);

    /* 6. QEMU com isa-debug-exit (requer -device isa-debug-exit) */
    outb(QEMU_DEBUG_EXIT_PORT, QEMU_DEBUG_EXIT_VAL);

    /* 7. Se nada funcionou, entra em loop infinito (halt) */
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

/* --- Reinicia o computador --- */
static inline void reboot(void) {
    /* 1. Tenta o registrador de reset do FADT (se tivermos os dados) */
    /* (O ideal é ler o RESET_REG do FADT; aqui usamos um fallback comum) */
    if (acpi_ready) {
        /* Endereço comum do RESET_REG em muitos sistemas */
        /* Você pode obter isso do FADT: fadt->reset_reg.address */
        /* Se tiver acesso ao FADT, substitua o endereço abaixo */
        /* Exemplo: outb(fadt->reset_reg.address, fadt->reset_value); */
        /* Por simplicidade, tentamos a porta 0xCF9 (chipset reset comum) */
        outb(0xCF9, 0x06); /* 0x06 = reset completo */
    }

    /* 2. Reset via 8042 (PS/2 Controller) */
    /* Espera o buffer de entrada estar limpo */
    while (inb(0x64) & 0x02) { /* Espera */ }
    outb(0x64, 0xFE); /* Comando de reset */

    /* 3. Triple Fault (como último recurso) */
    /* Cria uma IDT nula e dispara uma interrupção */
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) null_idt = {0, 0};

    __asm__ volatile (
        "lidt %0\n"
        "int $0\n"
        :
        : "m"(null_idt)
    );

    /* 4. Loop infinito se tudo falhar */
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

#endif /* KERNEL_IO_H */
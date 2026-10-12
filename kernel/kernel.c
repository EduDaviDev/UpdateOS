/* kernel/kernel.c
 *
 * Sequência de boot do UpOS:
 *   1. Serial (debug)
 *   2. GDT (6 entradas: null, kcode, kdata, ucode, udata, TSS reservado)
 *   3. IDT + ISRs + IRQs + page fault handler
 *   4. PMM (bitmap Multiboot2)
 *   5. Paging (identity map até 32 MiB)
 *   6. Heap (kmalloc sobre 0x00400000..0x01400000)
 *   7. TSS (precisa de stack de kernel válida)
 *   8. UPM (User Process Manager)
 *   9. Carrega o primeiro processo e cai em ring 3
 */

#include <stdint.h>
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/irq.h"
#include "cpu/tss.h"
#include "cpu/pmm.h"
#include "cpu/paging.h"
#include "cpu/heap.h"
#include "cpu/upm.h"
#include "drivers/serial.h"
#include "drivers/atapio.h"
#include "mboot.h"
#include "drivers/fatfs/ff.h"

/* ============================================================
 *  Stack de kernel dedicada do "processo 0" (o kernel em si).
 *
 *  É pra cá que o TSS vai apontar quando ainda não há processo
 *  de user rodando. Precisa existir ANTES do primeiro tss_init().
 * ============================================================ */

static uint8_t g_kernel_stack[16384] __attribute__((aligned(16)));

struct multiboot_info *g_multiboot_info = NULL;

#define KERNEL_STACK_TOP ((uint32_t)(g_kernel_stack + sizeof(g_kernel_stack)))

/* ============================================================
 *  Hook chamado quando o último processo sai
 * ============================================================ */

static void on_app_exit(process_t *p, int32_t code)
{
    serial_print("\n[kernel] ================================\n");
    serial_print("[kernel] processo terminou\n");
    if (p) {
        serial_print("[kernel]   pid  = ");
        serial_print_dec(p->pid);
        serial_print("\n[kernel]   nome = ");
        serial_print(p->name);
    }
    serial_print("\n[kernel]   code = ");
    serial_print_dec((uint32_t)code);
    serial_print("\n[kernel] ================================\n\n");

    upm_list();

    /* Sem shell ainda — halt. Quando você tiver um shell de kernel,
     * troque isso por um loop de input. */
    for (;;) __asm__ volatile ("cli; hlt");
}

/* ============================================================
 *  Boot
 * ============================================================ */

void kernel_main(uint32_t magic, struct multiboot_info *mb)
{
    serial_init();
    serial_print("\n");
    serial_print("========================================\n");
    serial_print("             UpOS - boot                \n");
    serial_print("========================================\n");

    /* Valida o magic do Multiboot2 */
    if (magic != 0x36D76289) {
        serial_print("[boot] ERRO: magic MB2 inválido: ");
        serial_print_hex32(magic);
        serial_print("\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }
    serial_print("[boot] magic MB2 OK\n");

    /* ------------------------------------------------------------
     *  2. GDT — precisa ser a PRIMEIRA coisa
     *     (segmentos válidos antes de qualquer interrupção)
     * ------------------------------------------------------------ */
    serial_print("[boot] gdt_init()\n");
    gdt_init();
    serial_print("[boot]   GDT com 6 entradas, 0x28 reservada pro TSS\n");

    /* ------------------------------------------------------------
     *  3. IDT + ISRs + IRQs
     *     O page fault handler tem que estar pronto ANTES de
     *     habilitar paging "de verdade" com address spaces por
     *     processo — se algo der errado no paging, queremos ver
     *     a mensagem, não um reset silencioso.
     * ------------------------------------------------------------ */
    serial_print("[boot] idt_init()\n");
    idt_init();

    serial_print("[boot] isr_install()\n");
    isr_install();

    serial_print("[boot] irq_install()\n");
    irq_install();

    /* ------------------------------------------------------------
     *  4. PMM — bitmap a partir do Multiboot2 mmap
     *     Nada aqui depende de paging ainda; o kernel roda em
     *     identity map do boot_page_directory (primeiros 4 MiB).
     * ------------------------------------------------------------ */
    serial_print("[boot] pmm_init()\n");
    pmm_init(mb);

    serial_print("[boot]   total frames: ");
    serial_print_dec(pmm_total_frames());
    serial_print("  livres: ");
    serial_print_dec(pmm_free_frames());
    serial_print("\n");

    /* ------------------------------------------------------------
     *  5. Paging — completa o identity map até 32 MiB
     *     Os primeiros 4 MiB já vêm do boot_page_table0 (asm).
     *     Aqui mapeamos [4 MiB, 32 MiB) pro heap + kernel stacks.
     * ------------------------------------------------------------ */
    serial_print("[boot] paging_init()\n");
    paging_init();

    /* ------------------------------------------------------------
     *  6. Heap — kmalloc sobre a região identity-mapped
     *     HEAP_START = 0x00400000 (4 MiB)
     *     HEAP_SIZE  = 0x01000000 (16 MiB)
     *     Caber tudo entre 4 MiB e 20 MiB, dentro dos 32 MiB mapeados.
     * ------------------------------------------------------------ */
    serial_print("[boot] heap_init()\n");
    heap_init();

    /* ------------------------------------------------------------
     *  7. TSS
     *     - Precisa de GDT pronta (índice 5)
     *     - Precisa de uma stack de kernel VÁLIDA em esp0
     *       (a CPU empilha lá quando uma interrupção vier de ring 3)
     * ------------------------------------------------------------ */
    serial_print("[boot] tss_init()\n");
    tss_init(KERNEL_STACK_TOP);

    /* Confirma que o TR aponta pra 0x28 */
    {
        uint16_t tr;
        __asm__ volatile ("str %%ax" : "=a"(tr));
        serial_print("[boot]   TR = ");
        serial_print_hex32(tr);
        serial_print(" (esperado: 0x00000028)\n");
    }

    serial_print("[boot] testando ATA...\n");

    int ndrives = ata_init();
    serial_print("[boot] drives detectados: ");
    serial_print_dec((uint32_t)ndrives);
    serial_print("\n");

    if (ndrives <= 0) {
        serial_print("[boot] nenhum drive ATA — problema no QEMU ou no driver\n");
    } else {
        for (int d = 0; d < 4; d++) {
            if (!ata_present(d)) continue;
            serial_print("[boot]   drive ");
            serial_print_dec((uint32_t)d);
            serial_print(": ");
            serial_print_dec(ata_get_sector_count(d));
            serial_print(" setores\n");
        }

        /* Tenta ler o MBR do drive 0 (primary master) */
        if (ata_present(0)) {
            uint8_t mbr[512];
            int r = ata_read_sectors(0, 0, 1, mbr);
            serial_print("[boot] read setor 0 do drive 0: ");
            serial_print_dec((uint32_t)r);
            serial_print("\n");

            if (r == 0) {
                serial_print("[boot] MBR[0..7]: ");
                for (int i = 0; i < 8; i++) {
                    serial_print_hex32(mbr[i]);
                    serial_print(" ");
                }
                serial_print("\n");
            }
        }
    }

    /* ------------------------------------------------------------
     *  8. UPM
     * ------------------------------------------------------------ */
    serial_print("[boot] upm_init()\n");
    upm_init();
    upm_set_exit_hook(on_app_exit);

    /* ------------------------------------------------------------
     *  9. Monta o disco e carrega o ELF
     * ------------------------------------------------------------ */
    static FATFS fs;
    FRESULT fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        serial_print("[boot] f_mount falhou (");
        serial_print_dec(fr);
        serial_print(")\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }
    serial_print("[boot] disco montado\n");

    FIL fil;
    fr = f_open(&fil, "0:/mainprog.elf", FA_READ);
    if (fr != FR_OK) {
        serial_print("[boot] f_open('0:/mainprog.elf') falhou (");
        serial_print_dec(fr);
        serial_print(")\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }

    uint32_t elf_size = f_size(&fil);
    uint8_t *elf_buf  = (uint8_t *)heap_alloc(elf_size, 4);
    if (!elf_buf) {
        serial_print("[boot] heap_alloc pro ELF falhou\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }

    UINT br = 0;
    fr = f_read(&fil, elf_buf, elf_size, &br);
    f_close(&fil);

    if (fr != FR_OK || br != elf_size) {
        serial_print("[boot] f_read incompleto\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }

    serial_print("[boot] ELF lido: ");
    serial_print_dec(elf_size);
    serial_print(" bytes\n");

    /* ------------------------------------------------------------
     *  10. Cria processo e cai em ring 3
     * ------------------------------------------------------------ */
    process_t *p = upm_create("helloworld", PROC_PROGRAM, elf_buf, elf_size);
    heap_free(elf_buf);

    if (!p) {
        serial_print("[boot] upm_create falhou\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }

    upm_list();

    serial_print("[boot] upm_run() — entrando em ring 3\n\n");
    upm_run(p);

    serial_print("[boot] ERRO: upm_run retornou!\n");
    for (;;) __asm__ volatile ("cli; hlt");
}
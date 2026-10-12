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
#include "mboot.h"

/* ============================================================
 *  Stack de kernel dedicada do "processo 0" (o kernel em si).
 *
 *  É pra cá que o TSS vai apontar quando ainda não há processo
 *  de user rodando. Precisa existir ANTES do primeiro tss_init().
 * ============================================================ */

static uint8_t g_kernel_stack[16384] __attribute__((aligned(16)));

#define KERNEL_STACK_TOP ((uint32_t)(g_kernel_stack + sizeof(g_kernel_stack)))

/* ============================================================
 *  ELF embutido
 *
 *  Por enquanto, hardcoded. Depois, quando você tiver um FS,
 *  vai vir de lá (initrd, FAT32, etc.).
 *
 *  COMO GERAR: use `xxd -i assets/Apps/Binaries/helloworld.elf`
 *  e cole o array abaixo, ou inclua um .S que faz `.incbin`.
 * ============================================================ */

extern const uint8_t  helloworld_elf[];
extern const uint32_t helloworld_elf_size;

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

void kernel_main(struct multiboot_info *mb)
{
    /* ------------------------------------------------------------
     *  1. Debug
     * ------------------------------------------------------------ */
    serial_init();
    serial_print("\n");
    serial_print("========================================\n");
    serial_print("             UpOS - boot                \n");
    serial_print("========================================\n");

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

    /* ------------------------------------------------------------
     *  8. UPM
     * ------------------------------------------------------------ */
    serial_print("[boot] upm_init()\n");
    upm_init();
    upm_set_exit_hook(on_app_exit);

    /* ------------------------------------------------------------
     *  9. Cria o primeiro processo
     * ------------------------------------------------------------ */
    serial_print("[boot] upm_create(\"helloworld\", PROC_PROGRAM)\n");
    process_t *p = upm_create("helloworld",
                              PROC_PROGRAM,
                              helloworld_elf,
                              helloworld_elf_size);
    if (!p) {
        serial_print("[boot] ERRO: falha ao criar processo\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }

    upm_list();

    /* ------------------------------------------------------------
     *  10. Cai em ring 3
     *      upm_run() configura CR3, tss.esp0 e faz iret.
     *      Quando o app fizer exit, o handler de syscall chama
     *      upm_exit() → on_app_exit() → halt.
     * ------------------------------------------------------------ */
    serial_print("[boot] upm_run() — entrando em ring 3\n\n");
    upm_run(p);

    /* Se chegar aqui, algo muito errado aconteceu */
    serial_print("[boot] ERRO: upm_run retornou!\n");
    for (;;) __asm__ volatile ("cli; hlt");
}
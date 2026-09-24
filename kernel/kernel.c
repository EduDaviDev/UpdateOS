/* kernel.c - UpdateOS: verificacao de recursos do kernel
 *
 * Todos os autotestes imprimem SOMENTE no serial (COM1).
 * No final:
 *   - Se o framebuffer Multiboot2 estiver disponivel, desenha 3 barras:
 *       verde    = OK
 *       vermelho = FAIL
 *       amarelo  = SKIP
 *   - Caso contrario, cai para VGA texto com o resumo.
 */

#include <stdint.h>
#include <stddef.h>

#include "cpu/page_fault.h"
#include "cpu/paging.h"
#include "cpu/heap.h"
#include "cpu/pmm.h"
#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/irq.h"
#include "cpu/pic.h"
#include "cpu/gdt.h"
#include "libs/memory.h"
#include "libs/string.h"
#include "drivers/vga.h"
#include "drivers/serial.h"
#include "drivers/video.h"
#include "mboot.h"

/* ------------------------------------------------------------------ */
/*  Global Multiboot2 (definido em mboot.h como extern)                */
/* ------------------------------------------------------------------ */
struct multiboot_info *g_multiboot_info = NULL;

/* ------------------------------------------------------------------ */
/*  Contadores dos testes                                              */
/* ------------------------------------------------------------------ */
static int g_pass = 0;
static int g_fail = 0;
static int g_skip = 0;

/* ================================================================== */
/*  IMPRESSAO DE TESTES — TUDO VIA SERIAL                              */
/* ================================================================== */
static void test_result(int ok, const char *name) {
    serial_print("  [");
    if (ok) { serial_print("OK  "); g_pass++; }
    else    { serial_print("FAIL"); g_fail++; }
    serial_print("] ");
    serial_print(name);
    serial_putc('\n');
}

static void test_skip(const char *name) {
    serial_print("  [SKIP] ");
    g_skip++;
    serial_print(name);
    serial_putc('\n');
}

static void section(const char *title) {
    serial_putc('\n');
    serial_print("== ");
    serial_print(title);
    serial_print(" ==\n");
}

/* ================================================================== */
/*  TESTE 1: Multiboot2                                                */
/* ================================================================== */
static void check_multiboot(uint32_t magic, void *info_ptr) {
    section("Multiboot2");

    test_result(magic == MULTIBOOT2_BOOTLOADER_MAGIC, "magic 0x36D76289");
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) return;

    test_result(info_ptr != NULL, "info ptr != NULL");
    if (!info_ptr) return;

    struct multiboot_info *info = (struct multiboot_info *)info_ptr;
    uint8_t *p   = (uint8_t *)info->tags;
    uint8_t *end = (uint8_t *)info + info->total_size;

    uint64_t ram_kib = 0;
    uint32_t n_mmap  = 0;
    int tem_basic_meminfo = 0;

    while (p < end) {
        struct multiboot_tag *tag = (struct multiboot_tag *)p;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO) {
            struct multiboot_tag_basic_meminfo *bi =
                (struct multiboot_tag_basic_meminfo *)p;
            tem_basic_meminfo = 1;
            ram_kib = (uint64_t)bi->mem_lower + bi->mem_upper;
        }
        else if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct multiboot_tag_mmap *mm = (struct multiboot_tag_mmap *)p;
            uint8_t *e    = (uint8_t *)mm->entries;
            uint8_t *eend = p + mm->size;
            uint64_t mmap_ram_kib = 0;
            while (e < eend) {
                struct multiboot_mmap_entry *ent = (struct multiboot_mmap_entry *)e;
                if (ent->type == 1) mmap_ram_kib += ent->len / 1024;
                n_mmap++;
                e += mm->entry_size;
            }
            if (mmap_ram_kib > ram_kib) ram_kib = mmap_ram_kib;
        }
        p += (tag->size + 7) & ~7u;
    }

    test_result(tem_basic_meminfo, "tag BASIC_MEMINFO presente");
    test_result(n_mmap > 0, "tag MMAP presente com entradas");

    serial_print("  Entradas no memory map: ");
    serial_print_dec(n_mmap);
    serial_putc('\n');

    serial_print("  RAM total disponivel:   ");
    serial_print_dec((uint32_t)(ram_kib / 1024));
    serial_print(" MiB\n");
}

/* ================================================================== */
/*  TESTE 2: Paginacao                                                 */
/* ================================================================== */
static void check_paging(void) {
    section("Paginacao");

    uint32_t cr0 = paging_read_cr0();
    test_result((cr0 & 0x80000000u) != 0, "CR0.PG (bit 31) ligado");

    uint32_t cr3 = paging_read_cr3();
    serial_print("  CR3 = ");
    serial_print_hex32(cr3);
    serial_putc('\n');
    test_result(cr3 != 0, "CR3 configurado (nao-zero)");

    volatile uint32_t *probe = (volatile uint32_t *)0x00100000u;
    uint32_t saved = *probe;
    *probe = 0xDEADBEEF;
    test_result(*probe == 0xDEADBEEF, "identity map 0x100000 leitura/escrita");
    *probe = saved;

    uint32_t virt = 0x003FF000u;
    paging_map_page(virt, virt, 0x3);
    volatile uint32_t *v = (volatile uint32_t *)virt;
    *v = 0xCAFEBABE;
    test_result(*v == 0xCAFEBABE, "paging_map_page + escrita/leitura");

    paging_invalidate_tlb(virt);
    test_result(1, "paging_invalidate_tlb acessivel");
}

/* ================================================================== */
/*  TESTE 3: Heap                                                      */
/* ================================================================== */
static int g_heap_mapped = 0;

static void check_heap(void) {
    section("Heap");

    if (!g_heap_mapped) {
        test_skip("heap: HEAP_START nao mapeado");
        return;
    }

    void *a = heap_alloc(64, 0);
    test_result(a != NULL, "heap_alloc(64)");
    if (a) {
        memset(a, 0xAA, 64);
        uint8_t *pa = (uint8_t *)a;
        int ok = 1;
        for (int i = 0; i < 64; i++) if (pa[i] != 0xAA) { ok = 0; break; }
        test_result(ok, "escrita/leitura em 64 bytes");
    }

    void *b = heap_alloc(13, 0);
    test_result(b != NULL && (((uintptr_t)b & 3) == 0), "alinhamento de heap_alloc(13)");

    void *ptrs[8];
    int count = 0;
    for (int i = 0; i < 8; i++) {
        ptrs[i] = heap_alloc(128, 0);
        if (!ptrs[i]) break;
        count++;
    }
    test_result(count > 0, "multiplas alocacoes consecutivas");

    if (count >= 2) {
        heap_free(ptrs[0]);
        void *c = heap_alloc(64, 0);
        test_result(c != NULL, "reutilizacao de bloco liberado");
        if (c) heap_free(c);
    }

    heap_free(NULL);
    test_result(1, "heap_free(NULL) seguro");

    for (int i = 0; i < count; i++) heap_free(ptrs[i]);
    if (a) heap_free(a);
    if (b) heap_free(b);
}

/* ================================================================== */
/*  TESTE 4: Biblioteca de memoria                                     */
/* ================================================================== */
static void check_memory_lib(void) {
    section("Biblioteca de memoria");

    char sa[16], sb[16];
    strcpy(sa, "UpdateOS");
    memcpy(sb, sa, 9);
    test_result(strcmp(sa, sb) == 0, "strcpy + memcpy + strcmp");

    uint8_t buf[8];
    memset(buf, 0x5A, 8);
    int ok = 1;
    for (int i = 0; i < 8; i++) if (buf[i] != 0x5A) { ok = 0; break; }
    test_result(ok, "memset (stack)");

    if (!g_heap_mapped) {
        test_skip("malloc/calloc/realloc/free: heap nao inicializado");
        return;
    }

    char *s = (char *)malloc(32);
    test_result(s != NULL, "malloc(32)");
    if (s) {
        strcpy(s, "UpdateOS");
        test_result(strcmp(s, "UpdateOS") == 0, "strcpy + strcmp (heap)");
    }

    uint8_t *z = (uint8_t *)calloc(1, 128);
    int zeroed = 1;
    if (z) for (int i = 0; i < 128; i++) if (z[i]) { zeroed = 0; break; }
    test_result(z && zeroed, "calloc zera memoria");

    if (s && z) {
        memcpy(z, s, 9);
        test_result(memcmp(z, s, 9) == 0, "memcpy + memcmp (heap)");
    }

    char *r = (char *)realloc(s, 64);
    test_result(r != NULL, "realloc(ptr, 64)");
    if (r) {
        test_result(strcmp(r, "UpdateOS") == 0, "realloc preserva conteudo");
        s = r;
    }

    void *n = realloc(NULL, 16);
    test_result(n != NULL, "realloc(NULL, 16) == malloc(16)");
    free(n);

    free(NULL);
    test_result(1, "free(NULL) seguro");

    if (s) free(s);
    if (z) free(z);
}

/* ================================================================== */
/*  Sumario no SERIAL                                                  */
/* ================================================================== */
static void print_serial_summary(void) {
    section("Resultado");
    serial_print("  Testes: ");
    serial_print_dec((uint32_t)(g_pass + g_fail + g_skip));
    serial_print("   OK: ");
    serial_print_dec((uint32_t)g_pass);
    serial_print("   FAIL: ");
    serial_print_dec((uint32_t)g_fail);
    serial_print("   SKIP: ");
    serial_print_dec((uint32_t)g_skip);
    serial_print("\n\n");
    if (g_fail == 0) serial_print("  *** TODOS OS TESTES EXECUTADOS PASSARAM ***\n");
    else             serial_print("  *** ALGUNS TESTES FALHARAM ***\n");
    serial_print("\n  Sistema pronto. Halt.\n");
}

/* ================================================================== */
/*  Sumario na TELA                                                    */
/* ================================================================== */

/* --- Fallback VGA texto --- */
static void vga_print_dec(uint32_t v) {
    char buf[11];
    int i = 0;
    if (v == 0) { vga_putc('0'); return; }
    while (v > 0 && i < 10) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i-- > 0) vga_putc(buf[i]);
}

static void show_vga_summary(void) {
    vga_init();
    vga_clear();

    vga_print("Testes=");
    vga_print_dec((uint32_t)(g_pass + g_fail + g_skip));
    vga_print(" FAIL=");
    vga_print_dec((uint32_t)g_fail);
    vga_print(" SKIP=");
    vga_print_dec((uint32_t)g_skip);
    vga_print(" OK=");
    vga_print_dec((uint32_t)g_pass);
    vga_putc('\n');
    vga_print("Verifique o serial.log\n");
}

/* --- Barra horizontal auxiliar --- */
static void draw_bar(uint32_t x0, uint32_t y0,
                     uint32_t x1, uint32_t y1, uint32_t col)
{
    for (uint32_t y = y0; y < y1; y++) {
        for (uint32_t x = x0; x < x1; x++) {
            gfx_putpixel(x, y, col);
        }
    }
}

/* --- Resumo no framebuffer: 3 barras horizontais --- */
static void show_fb_summary(void) {
    gfx_clear(0x101820);   /* fundo cinza-escuro */

    /* Escala: 20 pixels por teste, com clamp pra não estourar a tela.
     * Ajuste livre — é só visual.                                    */
    const uint32_t SCALE = 20;
    const uint32_t X0    = 10;

    uint32_t w_ok   = (uint32_t)g_pass * SCALE;
    uint32_t w_fail = (uint32_t)g_fail * SCALE;
    uint32_t w_skip = (uint32_t)g_skip * SCALE;

    /* Barra verde = OK */
    if (w_ok > 0)
        draw_bar(X0, 40, X0 + w_ok, 104, 0x00FF00);

    /* Barra vermelha = FAIL */
    if (w_fail > 0)
        draw_bar(X0, 120, X0 + w_fail, 184, 0xFF0000);

    /* Barra amarela = SKIP */
    if (w_skip > 0)
        draw_bar(X0, 200, X0 + w_skip, 264, 0xFFFF00);
}

static void show_screen_summary(void) {
    /* Tenta framebuffer primeiro */
    if (gfx_init()) {
        show_fb_summary();
        serial_print("[vga-fb] resumo desenhado no framebuffer\n");
        serial_print("         verde=OK  vermelho=FAIL  amarelo=SKIP\n");
        return;
    }

    /* Fallback: VGA texto */
    serial_print("[vga-fb] framebuffer indisponivel — usando VGA texto\n");
    show_vga_summary();
}

/* ================================================================== */
/*  Entry point                                                        */
/* ================================================================== */
void kernel_main(uint32_t magic, void *mb_info) {
    /* ------------------------------------------------------------------ */
    /*  1) Serial primeiro — sobrevive a triple fault                      */
    /* ------------------------------------------------------------------ */
    serial_init();
    serial_print("\n\n");
    serial_print("==========================================\n");
    serial_print("   UpdateOS - Verificacao de Recursos\n");
    serial_print("==========================================\n\n");
    serial_print("[boot] magic   = ");
    serial_print_hex32(magic);
    serial_putc('\n');
    serial_print("[boot] mb_info = ");
    serial_print_hex32((uint32_t)mb_info);
    serial_putc('\n');

    /* ------------------------------------------------------------------ */
    /*  2) Salva ponteiro Multiboot2 no global                             */
    /* ------------------------------------------------------------------ */
    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC && mb_info) {
        g_multiboot_info = (struct multiboot_info *)mb_info;
    }

    /* ------------------------------------------------------------------ */
    /*  3) IDT/ISR/IRQ/PIC — ANTES de qualquer coisa que possa faultar     */
    /* ------------------------------------------------------------------ */
    gdt_init();
	idt_init();
	isr_install();
    pic_remap();
    serial_print("\n[init] IDT/ISR/IRQ/PIC prontos\n");

    /* ------------------------------------------------------------------ */
    /*  4) PMM + mapeamento do heap                                        */
    /* ------------------------------------------------------------------ */
    if (g_multiboot_info) {
        pmm_init(g_multiboot_info);

        serial_print("[init] PMM: total=");
        serial_print_dec(pmm_total_frames());
        serial_print(" livres=");
        serial_print_dec(pmm_free_frames());
        serial_print("\n");

        uint32_t mapped = 0;
        for (uint32_t addr = HEAP_START; addr < HEAP_START + HEAP_SIZE; addr += 0x1000) {
            void *frame = pmm_alloc_frame();
            if (!frame) break;
            paging_map_page(addr, (uint32_t)frame, 0x3);
            mapped++;
        }
        serial_print("[init] heap mapeado: ");
        serial_print_dec(mapped * 4);
        serial_print(" KiB\n");

        heap_init();
        g_heap_mapped = 1;
    } else {
        serial_print("[init] sem Multiboot2 — PMM/heap desativados\n");
    }

    /* ------------------------------------------------------------------ */
    /*  5) Bateria de testes (SO SERIAL)                                   */
    /* ------------------------------------------------------------------ */
    check_multiboot(magic, mb_info);
    check_paging();
    check_heap();
    check_memory_lib();
    print_serial_summary();

    /* ------------------------------------------------------------------ */
    /*  6) Resumo na TELA (framebuffer OU VGA texto)                       */
    /* ------------------------------------------------------------------ */
    show_screen_summary();

	page_fault_test();

    /* ------------------------------------------------------------------ */
    /*  7) Halt                                                            */
    /* ------------------------------------------------------------------ */
    for (;;) __asm__ volatile ("hlt");
}
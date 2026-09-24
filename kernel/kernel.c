/* kernel.c - UpdateOS: verificação de recursos do kernel */

#include <stdint.h>
#include <stddef.h>

#include "cpu/paging.h"
#include "cpu/heap.h"
#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/irq.h"
#include "cpu/pic.h"
#include "cpu/pmm.h"
#include "libs/memory.h"
#include "libs/string.h"
#include "drivers/vga.h"
#include "drivers/serial.h"
#include "mboot.h"

/* ------------------------------------------------------------------ */
/*  Ajuste os nomes se seu vga.h for diferente                         */
/* ------------------------------------------------------------------ */
#define kputs(s)   serial_print(s)
#define kputc(c)   serial_putc(c)
#define kclear()
#define kinit()    serial_init()

/* ------------------------------------------------------------------ */
static int g_pass = 0;
static int g_fail = 0;
static int g_skip = 0;

/* ------------------------------------------------------------------ */
/*  Helpers de impressão                                               */
/* ------------------------------------------------------------------ */
static void print_hex32(uint32_t v) {
    static const char hex[] = "0123456789ABCDEF";
    kputs("0x");
    for (int i = 28; i >= 0; i -= 4) kputc(hex[(v >> i) & 0xF]);
}

static void print_dec(uint32_t v) {
    char buf[11];
    int i = 0;
    if (v == 0) { kputc('0'); return; }
    while (v > 0 && i < 10) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i-- > 0) kputc(buf[i]);
}

static void test_result(int ok, const char *name) {
    kputs("  [");
    if (ok) { kputs("OK  "); g_pass++; }
    else    { kputs("FAIL"); g_fail++; }
    kputs("] ");
    kputs(name);
    kputc('\n');
}

static void test_skip(const char *name) {
    kputs("  [SKIP] "); g_skip++;
    kputs(name); kputc('\n');
}

static void section(const char *title) {
    kputc('\n'); kputs("== "); kputs(title); kputs(" ==\n");
}

/* ------------------------------------------------------------------ */
/*  Teste 1: Multiboot2 (usando structs do seu mboot.h)                */
/* ------------------------------------------------------------------ */
static uint64_t g_total_ram_kib = 0;
static uint32_t g_mmap_entries  = 0;

static void check_multiboot(uint32_t magic, void *info_ptr) {
    section("Multiboot2");

    test_result(magic == MULTIBOOT2_BOOTLOADER_MAGIC, "magic 0x36D76289");
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) return;

    test_result(info_ptr != NULL, "info ptr != NULL");
    if (!info_ptr) return;

    struct multiboot_info *info = (struct multiboot_info *)info_ptr;
    uint8_t *p    = (uint8_t *)info->tags;
    uint8_t *end  = (uint8_t *)info + info->total_size;

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
            uint8_t *e     = (uint8_t *)mm->entries;
            uint8_t *eend  = p + mm->size;
            uint64_t mmap_ram_kib = 0;
            while (e < eend) {
                struct multiboot_mmap_entry *ent = (struct multiboot_mmap_entry *)e;
                if (ent->type == 1) {
                    mmap_ram_kib += ent->len / 1024;
                }
                n_mmap++;
                e += mm->entry_size;
            }
            if (mmap_ram_kib > ram_kib) ram_kib = mmap_ram_kib;
        }

        p += (tag->size + 7) & ~7u;   /* alinhar para 8 bytes */
    }

    g_total_ram_kib = ram_kib;
    g_mmap_entries  = n_mmap;

    test_result(tem_basic_meminfo, "tag BASIC_MEMINFO presente");
    test_result(n_mmap > 0, "tag MMAP presente com entradas");

    kputs("  Entradas no memory map: ");
    print_dec(n_mmap); kputc('\n');

    kputs("  RAM total disponivel:   ");
    print_dec((uint32_t)(ram_kib / 1024));
    kputs(" MiB\n");
}

/* ------------------------------------------------------------------ */
/*  Teste 2: Paginação (SOMENTE leitura — NÃO mexe nos page tables)    */
/* ------------------------------------------------------------------ */
static void check_paging(void) {
    section("Paginacao");

    uint32_t cr0 = paging_read_cr0();
    test_result((cr0 & 0x80000000u) != 0, "CR0.PG (bit 31) ligado");

    uint32_t cr3 = paging_read_cr3();
    kputs("  CR3 = "); print_hex32(cr3); kputc('\n');
    test_result(cr3 != 0, "CR3 configurado (nao-zero)");

    /* Identity map: escrever e ler no endereço do kernel */
    volatile uint32_t *probe = (volatile uint32_t *)0x00100000u;
    uint32_t saved = *probe;
    *probe = 0xDEADBEEF;
    test_result(*probe == 0xDEADBEEF, "identity map 0x100000 leitura/escrita");
    *probe = saved;

    /* Mapear/desmapear dentro dos primeiros 4 MiB (já cobertos) */
    uint32_t virt = 0x003FF000u;
    paging_map_page(virt, virt, 0x3);
    volatile uint32_t *v = (volatile uint32_t *)virt;
    *v = 0xCAFEBABE;
    test_result(*v == 0xCAFEBABE, "paging_map_page + escrita/leitura");

    /* NÃO desmapear — deixaria buraco. Só invalida TLB. */
    paging_invalidate_tlb(virt);
    test_result(1, "paging_invalidate_tlb acessivel");
}

/* ------------------------------------------------------------------ */
/*  Teste 3: Heap                                                      */
/*                                                                     */
/*  HEAP_START = 0x00400000 (4 MiB) NÃO está mapeado no boot.asm.      */
/*  Ativar esse teste só depois de mapear o heap (com PMM).            */
/* ------------------------------------------------------------------ */
static int g_heap_mapped = 0;

static void check_heap(void) {
    section("Heap");
    if (!g_heap_mapped) {
        test_skip("heap: HEAP_START 0x00400000 ainda nao mapeado");
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

/* ------------------------------------------------------------------ */
/*  Teste 4: Biblioteca de memória                                     */
/* ------------------------------------------------------------------ */
static void check_memory_lib(void) {
    section("Biblioteca de memoria");

    /* Testes de string (não dependem do heap — usam stack) */
    char a[16], b[16];
    strcpy(a, "UpdateOS");
    memcpy(b, a, 9);
    test_result(strcmp(a, b) == 0, "strcpy + memcpy + strcmp");

    uint8_t buf[8];
    memset(buf, 0x5A, 8);
    int ok = 1;
    for (int i = 0; i < 8; i++) if (buf[i] != 0x5A) { ok = 0; break; }
    test_result(ok, "memset (stack)");

    /* Testes de malloc dependem do heap */
	if (g_heap_mapped) {
	    char *s = (char *)malloc(32);
	    test_result(s != NULL, "malloc(32)");
	    if (s) { strcpy(s, "UpdateOS"); test_result(strcmp(s, "UpdateOS") == 0, "strcpy + strcmp (heap)"); }

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
	    if (r) { test_result(strcmp(r, "UpdateOS") == 0, "realloc preserva conteudo"); s = r; }

	    void *n = realloc(NULL, 16);
	    test_result(n != NULL, "realloc(NULL, 16) == malloc(16)");
	    free(n);

	    free(NULL);
	    test_result(1, "free(NULL) seguro");

	    if (s) free(s);
	    if (z) free(z);
	} else {
	    test_skip("malloc/calloc/realloc/free: heap nao inicializado");
	}
}

/* ------------------------------------------------------------------ */
/*  Sumário                                                            */
/* ------------------------------------------------------------------ */
static void print_summary(void) {
    section("Resultado");
    kputs("  Testes: "); print_dec((uint32_t)(g_pass + g_fail));
    kputs("   OK: ");    print_dec((uint32_t)g_pass);
    kputs("   FAIL: ");  print_dec((uint32_t)g_fail);
    kputs("   SKIP: ");  print_dec((uint32_t)g_skip);
    kputs("\n\n");
    if (g_fail == 0) kputs("  *** TODOS OS TESTES EXECUTADOS PASSARAM ***\n");
    else             kputs("  *** ALGUNS TESTES FALHARAM ***\n");
    kputs("\n  Sistema pronto. Halt.\n");
	vga_print("Verifique o serial.log!");
}

/* ------------------------------------------------------------------ */
/*  Entry point                                                        */
/* ------------------------------------------------------------------ */
void kernel_main(uint32_t magic, void *mb_info) {
    kinit();
    kclear();

    kputs("==========================================\n");
    kputs("   UpdateOS - Verificacao de Recursos\n");
    kputs("==========================================\n");

    /* --- 1) IDT/ISR/IRQ/PIC PRIMEIRO -------------------------------- */
    /* Sem isso, QUALQUER exceção = triple fault (foi o que aconteceu)  */
    /* Ajuste os nomes conforme seus headers.                           */
    idt_init();
    pic_remap();

    kputs("\n[init] IDT/ISR/IRQ/PIC prontos\n");

    /* --- 2) Paginação já pronta (paging_init_early em boot.asm) ------ */
    /* NÃO chamar paging_init() — ele zera os page tables em uso!       */

    /* --- 3) Bateria de testes ---------------------------------------- */
    check_multiboot(magic, mb_info);
    check_paging();
	if (magic == MULTIBOOT2_BOOTLOADER_MAGIC && mb_info) {
	    pmm_init((struct multiboot_info *)mb_info);

	    serial_print("[init] PMM: total=");
	    serial_print_dec(pmm_total_frames());
	    serial_print(" livres=");
	    serial_print_dec(pmm_free_frames());
	    serial_print("\n");

	    /* Mapeia HEAP_START..HEAP_START+HEAP_SIZE usando frames do PMM */
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
	}
    check_heap();
    check_memory_lib();
    print_summary();

    for (;;) __asm__ volatile ("hlt");
}
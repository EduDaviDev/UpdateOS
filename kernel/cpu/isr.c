#include "isr.h"
#include "idt.h"       /* ajuste o caminho do seu idt.h */
#include "paging.h"

extern int vga_printf(const char *fmt, ...) __attribute__((weak));

/* Stubs em asm (exceções) */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

/* Array global (definido AQUI, referenciado por irq.c) */
isr_handler_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_handler_t handler) {
    interrupt_handlers[n] = handler;
}

const char *isr_exception_name(uint32_t n) {
    static const char *names[32] = {
        "Divide Error",        "Debug",
        "NMI",                 "Breakpoint",
        "Overflow",            "Bound Range Exceeded",
        "Invalid Opcode",      "Device Not Available",
        "Double Fault",        "Coprocessor Segment Overrun",
        "Invalid TSS",         "Segment Not Present",
        "Stack-Segment Fault", "General Protection Fault",
        "Page Fault",          "Reserved",
        "x87 FP Exception",    "Alignment Check",
        "Machine Check",       "SIMD FP Exception",
        "Reserved", "Reserved", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved"
    };
    return (n < 32) ? names[n] : "Unknown";
}

void isr_init(void) {
    const uint8_t flags = 0x8E;
    const uint16_t sel  = 0x08;

    idt_set_gate( 0, (uint32_t)isr0,  sel, flags);
    idt_set_gate( 1, (uint32_t)isr1,  sel, flags);
    idt_set_gate( 2, (uint32_t)isr2,  sel, flags);
    idt_set_gate( 3, (uint32_t)isr3,  sel, flags);
    idt_set_gate( 4, (uint32_t)isr4,  sel, flags);
    idt_set_gate( 5, (uint32_t)isr5,  sel, flags);
    idt_set_gate( 6, (uint32_t)isr6,  sel, flags);
    idt_set_gate( 7, (uint32_t)isr7,  sel, flags);
    idt_set_gate( 8, (uint32_t)isr8,  sel, flags);
    idt_set_gate( 9, (uint32_t)isr9,  sel, flags);
    idt_set_gate(10, (uint32_t)isr10, sel, flags);
    idt_set_gate(11, (uint32_t)isr11, sel, flags);
    idt_set_gate(12, (uint32_t)isr12, sel, flags);
    idt_set_gate(13, (uint32_t)isr13, sel, flags);
    idt_set_gate(14, (uint32_t)isr14, sel, flags);
    idt_set_gate(15, (uint32_t)isr15, sel, flags);
    idt_set_gate(16, (uint32_t)isr16, sel, flags);
    idt_set_gate(17, (uint32_t)isr17, sel, flags);
    idt_set_gate(18, (uint32_t)isr18, sel, flags);
    idt_set_gate(19, (uint32_t)isr19, sel, flags);
    idt_set_gate(20, (uint32_t)isr20, sel, flags);
    idt_set_gate(21, (uint32_t)isr21, sel, flags);
    idt_set_gate(22, (uint32_t)isr22, sel, flags);
    idt_set_gate(23, (uint32_t)isr23, sel, flags);
    idt_set_gate(24, (uint32_t)isr24, sel, flags);
    idt_set_gate(25, (uint32_t)isr25, sel, flags);
    idt_set_gate(26, (uint32_t)isr26, sel, flags);
    idt_set_gate(27, (uint32_t)isr27, sel, flags);
    idt_set_gate(28, (uint32_t)isr28, sel, flags);
    idt_set_gate(29, (uint32_t)isr29, sel, flags);
    idt_set_gate(30, (uint32_t)isr30, sel, flags);
    idt_set_gate(31, (uint32_t)isr31, sel, flags);
}

/* ---------------------------------------------------------------------
 * Dispatcher de exceções
 * --------------------------------------------------------------------- */
void isr_handler_c(registers_t *r) {
    /* Se alguém registrou um handler para este vetor, chama. */
    if (interrupt_handlers[r->int_no]) {
        interrupt_handlers[r->int_no](r);
        return;
    }

    /* Page fault: handler dedicado, mais informativo. */
    if (r->int_no == 14) {
        paging_fault_handler(r->err_code, r);
    }

    /* Fallback genérico: dump e halt. */
    if (vga_printf) {
        vga_printf("\n[EXCEPTION] %u: %s\n",
                   r->int_no, isr_exception_name(r->int_no));
        vga_printf("  err_code = 0x%x\n", r->err_code);
        vga_printf("  eip=0x%08x  cs=0x%04x  eflags=0x%08x\n",
                   r->eip, r->cs, r->eflags);
        vga_printf("  eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
                   r->eax, r->ebx, r->ecx, r->edx);
        vga_printf("  esi=%08x edi=%08x ebp=%08x esp=%08x\n",
                   r->esi, r->edi, r->ebp, r->esp_dummy);
        vga_printf("  ds=%04x es=%04x fs=%04x gs=%04x\n",
                   r->ds, r->es, r->fs, r->gs);
        vga_printf("[HALT]\n");
    }

    for (;;) __asm__ volatile("cli; hlt");
}
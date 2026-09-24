#include "page_fault.h"
#include "paging.h"
#include "../drivers/serial.h"

/* ------------------------------------------------------------------ */
/*  Bits do error code do #PF                                          */
/*    bit 0 (P)   : 0 = página não presente, 1 = violação de proteção  */
/*    bit 1 (W)   : 0 = leitura, 1 = escrita                           */
/*    bit 2 (U)   : 0 = kernel mode, 1 = user mode                     */
/*    bit 3 (RSVD): 1 = bit reservado sobrescrito                      */
/*    bit 4 (I/D) : 1 = fetch de instrução                             */
/* ------------------------------------------------------------------ */
static void print_error_decode(uint32_t err) {
    serial_print("  Tipo: ");

    if (err & 1) serial_print("violacao de protecao");
    else         serial_print("pagina nao presente");

    serial_print(", acesso: ");
    if (err & 2) serial_print("escrita");
    else         serial_print("leitura");

    serial_print(", modo: ");
    if (err & 4) serial_print("user");
    else         serial_print("kernel");

    if (err & 8)  serial_print(" [bit reservado sobrescrito]");
    if (err & 16) serial_print(" [fetch de instrucao]");

    serial_putc('\n');
}

/* ------------------------------------------------------------------ */
/*  Handler agora tem a assinatura isr_t                               */
/* ------------------------------------------------------------------ */
void page_fault_handler(registers_t *regs) {
    uint32_t eip = regs->eip;
    uint32_t err = regs->err_code;
    uint32_t cr2 = paging_get_fault_address();  /* lê CR2 do CPU */

    serial_putc('\n');
    serial_print("==========================================\n");
    serial_print("              PAGE FAULT                  \n");
    serial_print("==========================================\n");
    serial_print("  EIP = ");
    serial_print_hex32(eip);
    serial_print("   (onde faltou)\n");

    serial_print("  CR2 = ");
    serial_print_hex32(cr2);
    serial_print("   (endereco acessado)\n");

    serial_print("  ERR = ");
    serial_print_hex32(err);
    serial_putc('\n');

    print_error_decode(err);

    serial_print("------------------------------------------\n");
    serial_print("  KERNEL PANIC - halt\n");
    serial_print("==========================================\n");

    for (;;) __asm__ volatile ("cli; hlt");
}

/* ------------------------------------------------------------------ */
/*  Trigger deliberado: lê de um endereço virtual NÃO mapeado.         */
/* ------------------------------------------------------------------ */
void page_fault_test(void) {
    serial_print("\n[pf-test] trigger em 0xDEADB000...\n");
    volatile uint32_t *bad = (volatile uint32_t *)0xDEADB000u;
    uint32_t val = *bad;                /* <- #PF aqui */
    (void)val;
}
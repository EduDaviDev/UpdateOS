global irq0_handler
global irq1_handler

extern timer_handler
extern keyboard_isr

irq0_handler:
    pusha
    call timer_handler
    popa
    iret

irq1_handler:
    pusha
    call keyboard_isr
    popa
    iret

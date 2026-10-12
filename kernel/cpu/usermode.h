#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>

#define USER_CS 0x1B   /* GDT_USER_CODE | RPL 3 */
#define USER_SS 0x23   /* GDT_USER_DATA | RPL 3 */

/* Nunca retorna — cai em ring 3 em `entry` com `user_stack` como ESP. */
void enter_user_mode(uint32_t entry, uint32_t user_stack)
    __attribute__((noreturn));

#endif
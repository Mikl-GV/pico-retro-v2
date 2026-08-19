#ifndef JOYPAD_H
#define JOYPAD_H

#include <stdint.h>

void joypad_init(void);
void joypad_poll(void);

/* Состояние кнопок в формате NES ($4016): биты 0=A, 1=B, 3=Start,
 * 4=Up, 5=Down, 6=Left, 7=Right. 0 = нажата, 1 = отпущена. */
uint8_t joypad_buttons(void);

#endif
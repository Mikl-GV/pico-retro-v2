#ifndef JOYPAD_H
#define JOYPAD_H

#include <stdint.h>

void joypad_init(void);
uint8_t joypad_buttons(void);
uint8_t joypad_snapshot(void);

#endif
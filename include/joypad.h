#ifndef JOYPAD_H
#define JOYPAD_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool up, down, left, right;
    bool a, b, c;
    bool start;
    bool x, y, z, mode;
} joypad_state_t;

void joypad_init(void);
joypad_state_t joypad_read(void);
void joypad_poll(void);

/* Последнее состояние кнопок: атомарный байт. Биты как в joypad_read. */
uint8_t joypad_buttons(void);
uint32_t joypad_poll_count(void);

#endif
#include "joypad.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define N_KEYS 8

static const struct {
    uint32_t pin;
    uint8_t  bit;
} keys[N_KEYS] = {
    { JOY_A_PIN,     0 },
    { JOY_B_PIN,     1 },
    { JOY_SELECT_PIN,2 },  // Исправляем очепятку JOY_SELECT_PIN
    { JOY_START_PIN, 3 },
    { JOY_UP_PIN,    4 },
    { JOY_DOWN_PIN,  5 },
    { JOY_LEFT_PIN,  6 },
    { JOY_RIGHT_PIN, 7 },
};

void joypad_init(void) {
    for (int i = 0; i < N_KEYS; i++) {
        gpio_init(keys[i].pin);
        gpio_set_dir(keys[i].pin, GPIO_IN);
        gpio_pull_up(keys[i].pin);
    }
}

uint8_t joypad_buttons(void) {
    uint8_t b = 0xFF;
    for (int i = 0; i < N_KEYS; i++) {
        if (!gpio_get(keys[i].pin))
            b &= ~(1u << keys[i].bit);
    }
    return b;
}
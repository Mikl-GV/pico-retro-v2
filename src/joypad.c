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
    { JOY_SELECT_PIN,2 },
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
    /* DB9 SEL (GP16) и DATA (GP17) для Sega Mega Drive 2 */
    gpio_init(DB9_SEL_PIN);
    gpio_set_dir(DB9_SEL_PIN, GPIO_OUT);
    gpio_put(DB9_SEL_PIN, 0);
    gpio_init(DB9_DATA_PIN);
    gpio_set_dir(DB9_DATA_PIN, GPIO_IN);
    gpio_pull_up(DB9_DATA_PIN);
}

uint8_t joypad_buttons(void) {
    uint8_t b = 0xFF;
    /* On-board buttons */
    for (int i = 0; i < N_KEYS; i++) {
        if (!gpio_get(keys[i].pin))
            b &= ~(1u << keys[i].bit);
    }
    /* Sega Mega Drive 2 via 74HC157: SEL=0 → направления, SEL=1 → кнопки */
    gpio_put(DB9_SEL_PIN, 0);
    busy_wait_us_32(2);
    if (!gpio_get(DB9_DATA_PIN)) b &= 0x0F; /* какое-то направление нажато */
    gpio_put(DB9_SEL_PIN, 1);
    busy_wait_us_32(2);
    if (!gpio_get(DB9_DATA_PIN)) b &= 0xF0; /* какая-то кнопка нажата */
    return b;
}
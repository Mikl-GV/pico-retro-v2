#include "joypad.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define N_KEYS 7

static const struct {
    uint32_t pin;
    uint8_t  nes_bit;
} keys[N_KEYS] = {
    { JOY_A_PIN,     0 }, // A     → NES bit 0
    { JOY_B_PIN,     1 }, // B     → NES bit 1
    { JOY_START_PIN, 3 }, // Start → NES bit 3
    { JOY_UP_PIN,    4 }, // Up    → NES bit 4
    { JOY_DOWN_PIN,  5 }, // Down  → NES bit 5
    { JOY_LEFT_PIN,  6 }, // Left  → NES bit 6
    { JOY_RIGHT_PIN, 7 }, // Right → NES bit 7
};

static uint32_t db_cnt[N_KEYS];
static uint8_t  db_last[N_KEYS];

static volatile uint8_t g_buttons = 0xFF; // NES-формат: 0=нажата

#define DEBOUNCE_CYCLES 5

void joypad_init(void) {
    for (int i = 0; i < N_KEYS; i++) {
        gpio_init(keys[i].pin);
        gpio_set_dir(keys[i].pin, GPIO_IN);
        gpio_pull_up(keys[i].pin);
        db_cnt[i]  = 0;
        db_last[i] = 1;
    }
}

void joypad_poll(void) {
    uint8_t out = 0xFF;
    for (int i = 0; i < N_KEYS; i++) {
        uint8_t raw = gpio_get(keys[i].pin) ? 1 : 0;
        if (raw == db_last[i]) {
            if (db_cnt[i] < DEBOUNCE_CYCLES) db_cnt[i]++;
        } else {
            db_cnt[i]  = 0;
            db_last[i] = raw;
        }
        uint8_t stable = (db_cnt[i] >= DEBOUNCE_CYCLES) ? raw : db_last[i];
        if (!stable) out &= ~(1u << keys[i].nes_bit);
    }
    g_buttons = out;
}

uint8_t joypad_buttons(void) {
    return g_buttons;
}
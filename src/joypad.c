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

static uint8_t  db_stable[N_KEYS];   // подтверждённое состояние (0=нажата)
static uint8_t  db_raw[N_KEYS];      // последнее сырое значение
static uint32_t db_cnt[N_KEYS];      // счётчик одинаковых подряд

static volatile uint8_t g_buttons = 0xFF; // NES-формат: 0=нажата

/*
 * DEBOUNCE_CYCLES × период опроса = время дребезга.
 * Опрос ядром 1 каждые 2 мс → 8 × 2 = 16 мс — перекрывает дребезг
 * дешёвых кнопок (5–20 мс) и укладывается в 1 кадр (16.7 мс).
 */
#define DEBOUNCE_CYCLES 8

void joypad_init(void) {
    for (int i = 0; i < N_KEYS; i++) {
        gpio_init(keys[i].pin);
        gpio_set_dir(keys[i].pin, GPIO_IN);
        gpio_pull_up(keys[i].pin);          // активны LOW
        db_stable[i] = 1;                   // изначально отпущена
        db_raw[i]    = 1;
        db_cnt[i]    = DEBOUNCE_CYCLES;     // считаем стабильным сразу
    }
}

void joypad_poll(void) {
    uint8_t out = 0xFF;
    for (int i = 0; i < N_KEYS; i++) {
        uint8_t raw = gpio_get(keys[i].pin) ? 1 : 0;
        if (raw == db_raw[i]) {
            if (db_cnt[i] < DEBOUNCE_CYCLES) db_cnt[i]++;
        } else {
            db_raw[i] = raw;
            db_cnt[i] = 0;
        }
        if (db_cnt[i] >= DEBOUNCE_CYCLES) db_stable[i] = db_raw[i];
        if (!db_stable[i]) out &= ~(1u << keys[i].nes_bit);
    }
    g_buttons = out;
}

uint8_t joypad_buttons(void) {
    return g_buttons;
}
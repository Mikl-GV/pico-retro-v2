#include "joypad.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define N_KEYS 8

/*
 * Кнопка          | Пин на Pico | NES-бит | Клавиша игры
 * JOY_A_PIN       | GP22        | 0       | A
 * JOY_B_PIN       | GP27        | 1       | B
 * JOY_SELECT_PIN  | GP16        | 2       | Select
 * JOY_START_PIN   | GP26        | 3       | Start
 * JOY_UP_PIN      | GP18        | 4       | Up
 * JOY_DOWN_PIN    | GP19        | 5       | Down
 * JOY_LEFT_PIN    | GP20        | 6       | Left
 * JOY_RIGHT_PIN   | GP21        | 7       | Right
 */
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

/*
 * Дебаунс: трёхрежимный автомат на каждую кнопку.
 * raw      — мгновенное значение GPIO (0=замкнута, 1=разомкнута)
 * stable   — подтверждённое значение (0=нажата), используется для вывода
 * cnt      — счётчик одинаковых raw подряд
 *
 * Порог = 5 × 2 мс = 10 мс. Меньше 1 кадра (16.7 мс), больше импульса дребезга.
 */
static uint8_t  db_raw[N_KEYS];
static uint8_t  db_stable[N_KEYS];
static uint32_t db_cnt[N_KEYS];

#define DB_THRESH 20

static volatile uint8_t g_buttons = 0xFF;

void joypad_init(void) {
    for (int i = 0; i < N_KEYS; i++) {
        gpio_init(keys[i].pin);
        gpio_set_dir(keys[i].pin, GPIO_IN);
        gpio_pull_up(keys[i].pin);   // кнопка замыкает на GND
        db_raw[i]    = 1;
        db_stable[i] = 1;
        db_cnt[i]    = DB_THRESH;
    }
}

void joypad_poll(void) {
    uint8_t out = 0xFF;
    for (int i = 0; i < N_KEYS; i++) {
        uint8_t cur = gpio_get(keys[i].pin) ? 1 : 0;  // 0=замкнута, 1=разомкнута

        if (cur == db_raw[i]) {
            if (db_cnt[i] < DB_THRESH) db_cnt[i]++;
        } else {
            db_raw[i] = cur;
            db_cnt[i] = 0;
        }

        if (db_cnt[i] >= DB_THRESH)
            db_stable[i] = db_raw[i];

        if (!db_stable[i])
            out &= ~(1u << keys[i].bit);
    }
    g_buttons = out;
}

uint8_t joypad_buttons(void) {
    uint8_t v = g_buttons;
    __asm volatile("dmb sy" ::: "memory");
    return v;
}
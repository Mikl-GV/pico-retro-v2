#include "joypad.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BTN_UP     0x01
#define BTN_DOWN   0x02
#define BTN_LEFT   0x04
#define BTN_RIGHT  0x08
#define BTN_A      0x10
#define BTN_START  0x20
#define BTN_B      0x40

static volatile uint8_t g_buttons;
static volatile uint32_t g_poll_count;

void joypad_init(void) {
    const uint32_t pins[] = {
        JOY_UP_PIN, JOY_DOWN_PIN, JOY_LEFT_PIN, JOY_RIGHT_PIN,
        JOY_AB_PIN, JOY_START_PIN, JOY_RESET_PIN
    };
    for (uint32_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }
    g_buttons = 0;
}

joypad_state_t joypad_read(void) {
    joypad_state_t s = {0};
    s.up    = !gpio_get(JOY_UP_PIN);
    s.down  = !gpio_get(JOY_DOWN_PIN);
    s.left  = !gpio_get(JOY_LEFT_PIN);
    s.right = !gpio_get(JOY_RIGHT_PIN);
    s.a     = !gpio_get(JOY_AB_PIN);
    s.start = !gpio_get(JOY_START_PIN);
    s.b     = !gpio_get(JOY_RESET_PIN);
    return s;
}

void joypad_poll(void) {
    uint8_t v = 0;
    if (!gpio_get(JOY_UP_PIN))    v |= BTN_UP;
    if (!gpio_get(JOY_DOWN_PIN))  v |= BTN_DOWN;
    if (!gpio_get(JOY_LEFT_PIN))  v |= BTN_LEFT;
    if (!gpio_get(JOY_RIGHT_PIN)) v |= BTN_RIGHT;
    if (!gpio_get(JOY_AB_PIN))    v |= BTN_A;
    if (!gpio_get(JOY_START_PIN)) v |= BTN_START;
    if (!gpio_get(JOY_RESET_PIN)) v |= BTN_B;
    g_buttons = v;
    g_poll_count++;
}

uint32_t joypad_poll_count(void) {
    return g_poll_count;
}

uint8_t joypad_buttons(void) {
    return g_buttons;
}
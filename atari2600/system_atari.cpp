/*
 * Atari 2600 system layer for pico-retro.
 * Wraps the mos6507/TIA/mos6532 core for the ILI9341 display + onboard buttons.
 */

#include <pico/stdlib.h>
#include <cstdio>
#include <cstring>

extern "C" {
#include "atari/Atari-TIA.h"
#include "atari/Atari-cart.h"
#include "mos6507/mos6507.h"
#include "mos6532/mos6532.h"
#include "display.h"
#include "joypad.h"
}

/* --- Atari 2600 framebuffer (160x192, 1 byte RGB222 per pixel) --- */
#define A2600_W 160
#define A2600_H 192
static uint8_t a2600_fb[A2600_W * A2600_H];

/* Convert RGB222 (2 bits per channel) to RGB565 */
static inline uint16_t rgb222_to_565(uint8_t c) {
    uint8_t r = (c >> 4) & 0x03;
    uint8_t g = (c >> 2) & 0x03;
    uint8_t b = c & 0x03;
    uint16_t r5 = (r * 255 / 3) >> 3;
    uint16_t g6 = (g * 255 / 3) >> 2;
    uint16_t b5 = (b * 255 / 3) >> 3;
    return (r5 << 11) | (g6 << 5) | b5;
}

/* LUT для быстрого рендера через display_stream_pixels (индекс = байт RGB222) */
static uint16_t atari_lut[256];
static bool atari_lut_ready = false;

/* Состояние кадровой синхронизации (переживает кадры, сбрасывается при init) */
static uint32_t atari_line_count = 0;
static uint32_t atari_vblank = 0;
static uint32_t atari_vsync = 0;

static void atari_init_lut(void) {
    if (atari_lut_ready) return;
    for (int i = 0; i < 256; i++) atari_lut[i] = rgb222_to_565((uint8_t)i);
    atari_lut_ready = true;
}

extern "C" void atari2600_init(const uint8_t *rom, uint32_t size) {
    opcode_populate_ISA_table();
    mos6532_init();
    TIA_init();
    cartridge_load_size(rom, size);
    mos6507_reset();
    /* Сброс состояния кадровой синхронизации и очистка кадра при новой игре */
    atari_line_count = 0;
    atari_vblank = 0;
    atari_vsync = 0;
    memset(a2600_fb, 0, sizeof(a2600_fb));
}

/* Run one emulated frame.
 * Счётчики статические (переживают кадры), но кадр завершается по
 * накоплению 192 видимых строк — гарантированно не зависает даже если
 * игра не генерирует VSYNC корректно. */
extern "C" void atari2600_run_frame(void) {
    int i, clock_count = 0;

    for (;;) {
        for (i = 0; i < TIA_COLOUR_CLOCK_TOTAL; i++) {
            clock_count = TIA_clock_tick();
            if (!TIA_get_WSYNC() && !((clock_count + 1) % 3)) {
                mos6532_clock_tick();
                if (mos6507_clock_tick()) return;
            }
        }

        if (atari_vsync && !TIA_get_VSYNC()) {
            atari_line_count = 0;
            atari_vblank = TIA_VERTICAL_BLANK_LINES;
        }
        atari_vsync = TIA_get_VSYNC();

        if (!atari_vsync && !atari_vblank && (atari_line_count < TIA_VERTICAL_PICTURE_LINES)) {
            for (int x = 0; x < TIA_COLOUR_CLOCK_VISIBLE; x++)
                a2600_fb[atari_line_count * A2600_W + x] = (uint8_t)(tia_raw_buffer[x] & 0xFF);
            TIA_reset_buffer();
            atari_line_count++;
        }

        if (atari_vblank) atari_vblank--;

        /* Кадр завершён — гарантированный выход (не ждём VSYNC) */
        if (atari_line_count >= TIA_VERTICAL_PICTURE_LINES) return;
    }
}

/* Render 160x192 framebuffer to display (1:1, centered in the 256x240 window).
 * Uses streaming + LUT for speed. */
extern "C" void atari2600_render(void) {
    atari_init_lut();
    int x0 = 32 + (256 - A2600_W) / 2;   /* 80 */
    int y0 = (240 - A2600_H) / 2;        /* 24 */
    display_stream_begin(x0, y0, A2600_W, A2600_H);
    for (int y = 0; y < A2600_H; y++) {
        display_stream_pixels(&a2600_fb[y * A2600_W], atari_lut, A2600_W, 1);
    }
    display_stream_end();
}

/* Buttons -> Atari 2600 joystick (SWCHA) + fire (INPT4).
 * Маппинг как в MCUME/x2600 (Keyboard.c):
 *   Up=D4, Down=D5, Right=D6, Left=D7 (джойстик 1, активный LOW)
 *   Fire: INPT4=0x00 нажат, 0x80 отпущен */
extern "C" void atari2600_poll_joy(void) {
    uint8_t pad = joypad_buttons();  /* 0 = pressed */
    uint8_t swcha = 0xFF;
    if (!(pad & 0x10)) swcha &= ~0x10;  /* Up   -> D4 */
    if (!(pad & 0x20)) swcha &= ~0x20;  /* Down -> D5 */
    if (!(pad & 0x80)) swcha &= ~0x40;  /* Right-> D6 */
    if (!(pad & 0x40)) swcha &= ~0x80;  /* Left -> D7 */
    mos6532_write(SWCHA, swcha);
    /* Fire: A (bit0). Пользователь подтвердил на железе: покой=1 (отпущена),
     * нажатие=0 (0 только при нажатии). */
    uint8_t state = (pad & 0x01) ? 0 : 1;
    TIA_joy1_state(state);
    TIA_joy2_state(state);
}
/*
 * Atari 2600 system layer for pico-retro.
 * Wraps the mos6507/TIA/mos6532 core for the ILI9341 display + onboard buttons.
 */

#include <pico/stdlib.h>
#include <cstdio>

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

/* Convert RGB222 (2 bits per channel, from tia_raw_buffer low byte) to RGB565 */
static uint16_t rgb222_to_565(uint8_t c) {
    uint8_t r = (c >> 4) & 0x03;  /* bits 5-6 */
    uint8_t g = (c >> 2) & 0x03;  /* bits 3-4 */
    uint8_t b = c & 0x03;         /* bits 1-2 */
    uint16_t r5 = (r * 255 / 3) >> 3;
    uint16_t g6 = (g * 255 / 3) >> 2;
    uint16_t b5 = (b * 255 / 3) >> 3;
    return (r5 << 11) | (g6 << 5) | b5;
}

extern "C" void atari2600_init(const uint8_t *rom, uint32_t size) {
    opcode_populate_ISA_table();
    mos6532_init();
    TIA_init();
    cartridge_load_size(rom, size);
    mos6507_reset();
}

/* Run one emulated frame, rendering each visible scanline into a2600_fb.
 * This mirrors the original main_loop structure, returning after one frame. */
extern "C" void atari2600_run_frame(void) {
    uint32_t line_count = 0;
    uint32_t vblank = 0;
    uint32_t vsync = 0;
    int i, clock_count = 0;

    for (;;) {
        for (i = 0; i < TIA_COLOUR_CLOCK_TOTAL; i++) {
            clock_count = TIA_clock_tick();
            if (!TIA_get_WSYNC() && !((clock_count + 1) % 3)) {
                mos6532_clock_tick();
                if (mos6507_clock_tick()) return;
            }
        }

        if (vsync && !TIA_get_VSYNC()) {
            line_count = 0;
            vblank = TIA_VERTICAL_BLANK_LINES;
        }
        vsync = TIA_get_VSYNC();

        if (!vsync && !vblank && (line_count < TIA_VERTICAL_PICTURE_LINES)) {
            for (int x = 0; x < TIA_COLOUR_CLOCK_VISIBLE; x++)
                a2600_fb[line_count * A2600_W + x] = (uint8_t)(tia_raw_buffer[x] & 0xFF);
            TIA_reset_buffer();
            line_count++;
        }

        if (vblank) vblank--;

        /* Один полный видимый кадр накоплен */
        if (line_count >= TIA_VERTICAL_PICTURE_LINES) break;
    }
}

/* Render 160x192 framebuffer to display (1:1, centered in the 256x240 window).
 * Uses streaming to GRAM (fast, like the NES path). */
extern "C" void atari2600_render(void) {
    int x0 = 32 + (256 - A2600_W) / 2;   /* 80 */
    int y0 = (240 - A2600_H) / 2;        /* 24 */
    /* Работаем напрямую с GRAM: ставим окно и пишем строки по 160 пикселей */
    display_stream_begin(x0, y0, A2600_W, A2600_H);
    for (int y = 0; y < A2600_H; y++) {
        for (int x = 0; x < A2600_W; x++) {
            display_stream_pixel16(rgb222_to_565(a2600_fb[y * A2600_W + x]));
        }
    }
    display_stream_end();
}

/* Buttons -> Atari 2600 joystick (SWCHA) + fire (INPT4) */
extern "C" void atari2600_poll_joy(void) {
    uint8_t pad = joypad_buttons();  /* 0 = pressed */
    /* SWCHA: D4=Up, D5=Down, D6=Left, D7=Right (joystick 1, active LOW) */
    uint8_t swcha = 0xFF;
    if (!(pad & 0x10)) swcha &= ~0x10;  /* Up (NES Up -> bit4) */
    if (!(pad & 0x20)) swcha &= ~0x20;  /* Down */
    if (!(pad & 0x40)) swcha &= ~0x40;  /* Left */
    if (!(pad & 0x80)) swcha &= ~0x80;  /* Right */
    mos6532_write(SWCHA, swcha);
    /* Fire: NES A (bit0) -> TIA INPT4 (joystick fire) */
    TIA_joy1_state(!(pad & 0x01));
}
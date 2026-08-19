#include <stdint.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "display.h"
#include "joypad.h"
#include "config.h"
#include "nes.h"
#include "rom_bomberman.h"
#include "rom_balloon.h"
#include "rom_battlecity.h"
#include "rom_ducktales.h"
#include "rom_saiyuuki.h"

#define BG      0x0000
#define CURSOR  RGB565(31, 31, 0)
#define TITLE   RGB565(31, 0, 0)
#define WHITE   RGB565(31, 63, 31)
#define BAR     RGB565(31, 63, 31)

static const char *items[] = {
    "Balloon Fight", "Battle City", "Bomberman",
    "Duck Tales", "Saiyuuki World", "About",
};
#define N_ITEMS (sizeof(items) / sizeof(items[0]))

static void draw_header(void) {
    display_text_center("p i c o - r e t r o", 0, 1, TITLE, BG);
    display_fill_rect(0, 8, LCD_WIDTH, 2, BAR);
}

static void draw_footer(void) {
    display_fill_rect(0, 216, LCD_WIDTH, 2, BAR);
    display_text_center("UP/DN  START=ok  B=back", 28, 1, WHITE, BG);
}

static void draw_menu(int cursor) {
    display_fill(BG);
    draw_header();
    for (int i = 0; i < N_ITEMS; i++) {
        uint16_t clr = (i == cursor) ? CURSOR : WHITE;
        display_text(">", 1, 2 + i * 2, 1, clr, BG);
        display_text(items[i], 3, 2 + i * 2, 1, clr, BG);
    }
    draw_footer();
    display_flush();
}

static nes_t nes;
static volatile bool core1_running = true;

/* Ядро 1: опрос кнопок с антидребезгом, без перерывов на дисплей */
static void core1_main(void) {
    while (core1_running) {
        joypad_poll();
        sleep_us(2000);
    }
}

/* NES-биты (0=нажата): 0=A, 1=B, 3=Start, 4=Up, 5=Down, 6=Left, 7=Right */
#define NES_A     0x01
#define NES_B     0x02
#define NES_START 0x08
#define NES_UP    0x10
#define NES_DOWN  0x20

static void run_nes(void) {
    uint32_t frame = 0;
    uint32_t hold_exit = 0;

    while (true) {
        uint8_t pad = joypad_buttons();
        nes_set_joy(&nes, pad);

        nes_run_frame(&nes);
        frame++;

        if (frame & 1) {
            display_stream_begin(32, 0, 256, 240);
            display_stream_pixels(&nes.fb[0][0], ppu_lut, 256, 240);
            display_stream_end();
        }

        if (!(pad & NES_START)) hold_exit++; else hold_exit = 0;
        if (hold_exit > 60) break;
    }
}

int main(void) {
    display_init();
    joypad_init();

    multicore_launch_core1(core1_main);

    int cursor = 0;
    draw_menu(cursor);

    uint32_t prev_down = 0;

    while (true) {
        uint8_t pad = joypad_buttons();
        uint8_t down = ~pad; /* 1 = нажата */

        /* меню: bit0=Up, bit1=Down, bit2=Start, bit3=B */
        uint32_t mask = (((uint32_t)down >> 4) & 1)      /* Up    */
                      | (((uint32_t)(down >> 5) & 1) << 1) /* Down  */
                      | (((uint32_t)(down >> 3) & 1) << 2) /* Start */
                      | (((uint32_t)(down >> 1) & 1) << 3); /* B     */
        uint32_t edge = mask & ~prev_down;

        if (edge & 1) { cursor = (cursor + N_ITEMS - 1) % N_ITEMS; draw_menu(cursor); }
        if (edge & 2) { cursor = (cursor + 1) % N_ITEMS; draw_menu(cursor); }
        if (edge & 4) { /* Start — запуск */
            switch (cursor) {
            case 0: nes_init(&nes, rom_balloon, ROM_BALLOON_SIZE); break;
            case 1: nes_init(&nes, rom_battlecity, ROM_BATTLECITY_SIZE); break;
            case 2: nes_init(&nes, rom_bomberman, ROM_BOMBERMAN_SIZE); break;
            case 3: nes_init(&nes, rom_ducktales, ROM_DUCKTALES_SIZE); break;
            case 4: nes_init(&nes, rom_saiyuuki, ROM_SAIYUUKI_SIZE); break;
            }
            run_nes();
            draw_menu(cursor);
        }
        prev_down = mask;
        sleep_ms(20);
    }
    return 0;
}
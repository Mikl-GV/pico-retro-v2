#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
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

bi_decl(bi_program_name("pico-retro"));

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
        display_text(">", 1, 2 + i * 2, 1, (i == cursor) ? CURSOR : WHITE, BG);
        display_text(items[i], 3, 2 + i * 2, 1, (i == cursor) ? CURSOR : WHITE, BG);
    }
    draw_footer();
    display_flush();
}

static nes_t nes;
static volatile bool core1_running = true;

/* Ядро 1: только опрос кнопок, без перерывов на дисплей */
static void core1_main(void) {
    while (core1_running) {
        joypad_poll();
        sleep_us(2000);
    }
}

static void run_nes(void) {
    uint32_t frame = 0;
    uint32_t hold_exit = 0;

    while (true) {
        uint8_t pad = joypad_buttons();

        uint8_t btns = 0x00;
        if (pad & 0x10) btns |= 0x01; /* A     = bit 0 */
        if (pad & 0x40) btns |= 0x02; /* B     = bit 1 */
        if (pad & 0x20) btns |= 0x08; /* Start = bit 3 */
        if (pad & 0x01) btns |= 0x10; /* Up    = bit 4 */
        if (pad & 0x02) btns |= 0x20; /* Down  = bit 5 */
        if (pad & 0x04) btns |= 0x40; /* Left  = bit 6 */
        if (pad & 0x08) btns |= 0x80; /* Right = bit 7 */
        nes_set_joy(&nes, btns);

        nes_run_frame(&nes);
        frame++;

        if (frame & 1) {
            display_stream_begin(32, 0, 256, 240);
            display_stream_pixels(&nes.fb[0][0], ppu_lut, 256, 240);
            display_stream_end();
        }

        if (pad & 0x20) hold_exit++; else hold_exit = 0;
        if (hold_exit > 60) break;

        if (frame % 30 == 0) {
            printf("latch=%02X strobe=%d rd4016=%lu rd4017=%lu btns=%02X pad=%02X\n",
                nes.joy1_latch, nes.joy1_strobe,
                nes.rd4016_cnt, nes.rd4017_cnt, nes.joy1_buttons, pad);
        }
    }
}

int main(void) {
    stdio_init_all();
    display_init();
    joypad_init();

    multicore_launch_core1(core1_main);

    int cursor = 0;
    draw_menu(cursor);

    uint32_t prev_mask = 0;
    bool in_screen = false;

    while (true) {
        uint8_t pad = joypad_buttons();
        uint32_t mask = ((uint32_t)(pad & 0x01)) | ((uint32_t)((pad >> 1) & 1) << 1) | ((uint32_t)((pad >> 5) & 1) << 2) | ((uint32_t)((pad >> 6) & 1) << 3);
        uint32_t edge = mask & ~prev_mask;

        if (in_screen) {
            if (edge & (1 << 3)) { in_screen = false; draw_menu(cursor); }
        } else {
            if (edge & 1) { cursor = (cursor + N_ITEMS - 1) % N_ITEMS; draw_menu(cursor); }
            if (edge & 2) { cursor = (cursor + 1) % N_ITEMS; draw_menu(cursor); }
            if (edge & (1 << 2)) {
                switch (cursor) {
                case 0: nes_init(&nes, rom_balloon, ROM_BALLOON_SIZE); run_nes(); break;
                case 1: nes_init(&nes, rom_battlecity, ROM_BATTLECITY_SIZE); run_nes(); break;
                case 2: nes_init(&nes, rom_bomberman, ROM_BOMBERMAN_SIZE); run_nes(); break;
                case 3: nes_init(&nes, rom_ducktales, ROM_DUCKTALES_SIZE); run_nes(); break;
                case 4: nes_init(&nes, rom_saiyuuki, ROM_SAIYUUKI_SIZE); run_nes(); break;
                }
                draw_menu(cursor);
            }
        }
        prev_mask = mask;
        sleep_ms(20);
    }
    return 0;
}
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"

#include "display.h"
#include "joypad.h"
#include "config.h"
#include "nes.h"

#define BG      0x0000
#define CURSOR  RGB565(31, 31, 0)
#define TITLE   RGB565(31, 0, 0)
#define WHITE   RGB565(31, 63, 31)
#define BAR     RGB565(31, 63, 31)
#define GREEN   RGB565(0, 63, 0)

typedef struct {
    const char     *name;
    const uint8_t  *rom;
    uint32_t        size;
} game_entry_t;

#include "rom_balloon.h"
#include "rom_battlecity.h"
#include "rom_bomberman.h"
#include "rom_ducktales.h"
#include "rom_duck2.h"
#include "rom_saiyuuki.h"
#include "rom_mario.h"

static const game_entry_t games[] = {
    {"Balloon Fight",  rom_balloon,  ROM_BALLOON_SIZE},
    {"Battle City",    rom_battlecity, ROM_BATTLECITY_SIZE},
    {"Bomberman",      rom_bomberman,  ROM_BOMBERMAN_SIZE},
    {"Duck Tales",     rom_ducktales,  ROM_DUCKTALES_SIZE},
    {"DuckTales 2",    rom_duck2,     rom_duck2_size},
    {"Saiyuuki World", rom_saiyuuki,   ROM_SAIYUUKI_SIZE},
    {"SMB",            rom_mario,     rom_mario_size},
};
#define N_GAMES (sizeof(games) / sizeof(games[0]))

#define MENU_ABOUT  (N_GAMES)
#define MENU_COUNT  (N_GAMES + 1)

static nes_t nes;
static volatile bool core1_running = true;

static void draw_header(void) {
    display_text_center("p i c o - r e t r o", 0, 2, TITLE, BG);
    display_fill_rect(0, 16, LCD_WIDTH, 2, BAR);
}

static void draw_footer(void) {
    display_fill_rect(0, 232, LCD_WIDTH, 2, BAR);
    display_text_center("UP/DN  START=ok  B=back", 29, 1, WHITE, BG);
}

static void draw_menu(int cursor) {
    display_fill(BG);
    draw_header();
    for (int i = 0; i < N_GAMES; i++) {
        uint16_t clr = (i == cursor) ? CURSOR : WHITE;
        display_text(">", 1, 3 + i, 1, clr, BG);
        display_text(games[i].name, 3, 3 + i, 1, clr, BG);
    }
    {
        uint16_t clr = (cursor == MENU_ABOUT) ? CURSOR : WHITE;
        display_text(">", 1, 3 + N_GAMES, 1, clr, BG);
        display_text("About", 3, 3 + N_GAMES, 1, clr, BG);
    }
    draw_footer();
    display_flush();
}

static void draw_about(void) {
    display_fill(BG);
    display_text_center("p i c o - r e t r o", 0, 2, TITLE, BG);
    display_fill_rect(0, 16, LCD_WIDTH, 2, BAR);

    display_text("NES emulator for RP2040", 1, 4, 1, WHITE, BG);
    display_text("CPU 6502 + PPU + MMC3", 1, 6, 1, WHITE, BG);
    display_text("ILI9341V 8-bit 8080", 1, 8, 1, WHITE, BG);
    display_text("8 buttons: A/B/Select/Start", 1, 10, 1, WHITE, BG);
    display_text("UART: GP16/17 115200", 1, 12, 1, WHITE, BG);
    display_text("github.com/Mikl-GV/pico-retro", 1, 14, 1, GREEN, BG);

    display_fill_rect(0, 232, LCD_WIDTH, 2, BAR);
    display_text_center("B=back", 29, 1, WHITE, BG);
    display_flush();
}

static void core1_main(void) {
    while (core1_running) {
        joypad_poll();
        sleep_us(2000);
    }
}

#define NES_START 0x08

static void run_nes(void) {
    uint32_t frame = 0;
    uint32_t hold_exit = 0;
    uint32_t last_print = 0;

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

        if (frame - last_print >= 30) {
            printf("joy=%02X cnt=%lu\n", pad, nes.rd4016_cnt);
            last_print = frame;
        }

        if (!(pad & NES_START)) hold_exit++; else hold_exit = 0;
        if (hold_exit > 60) break;
    }
}

int main(void) {
    set_sys_clock_khz(250000, true);
    stdio_uart_init_full(uart0, 115200, 16, 17);
    printf("\n=== pico-retro boot ===\n");
    display_init();
    joypad_init();
    multicore_launch_core1(core1_main);
    printf("core1 running\n");

    int cursor = 0;
    draw_menu(cursor);

    uint32_t prev_down = 0;

    for (;;) {
        uint8_t pad = joypad_buttons();
        uint8_t down = ~pad;

        uint32_t mask = (((uint32_t)down >> 4) & 1)
                      | (((uint32_t)(down >> 5) & 1) << 1)
                      | (((uint32_t)(down >> 3) & 1) << 2)
                      | (((uint32_t)(down >> 1) & 1) << 3);
        uint32_t edge = mask & ~prev_down;

        if (edge & 1) { cursor = (cursor + MENU_COUNT - 1) % MENU_COUNT; draw_menu(cursor); }
        if (edge & 2) { cursor = (cursor + 1) % MENU_COUNT; draw_menu(cursor); }
        if (edge & 4) {
            if (cursor == MENU_ABOUT) {
                draw_about();
                while (true) {
                    uint8_t p = joypad_buttons();
                    if (!(p & 0x02)) { sleep_ms(200); break; }
                    sleep_ms(20);
                }
                draw_menu(cursor);
            } else {
                nes_init(&nes, games[cursor].rom, games[cursor].size);
                run_nes();
                draw_menu(cursor);
            }
        }
        prev_down = mask;
        sleep_ms(20);
    }
    return 0;
}
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "display.h"
#include "joypad.h"
#include "config.h"
#include "nes.h"

#define BG      0x0000
#define CURSOR  RGB565(31, 31, 0)
#define TITLE   RGB565(31, 0, 0)
#define WHITE   RGB565(31, 63, 31)
#define BAR     RGB565(31, 63, 31)
#define RED     RGB565(31, 0, 0)
#define GREEN   RGB565(0, 63, 0)
#define BLUE    RGB565(0, 0, 31)

typedef struct {
    const char     *name;
    const uint8_t  *rom;
    uint32_t        size;
} game_entry_t;

#include "rom_balloon.h"
#include "rom_battlecity.h"
#include "rom_bomberman.h"
#include "rom_ducktales.h"
#include "rom_saiyuuki.h"
#include "rom_mario.h"

static const game_entry_t games[] = {
    {"SMB",            rom_mario,     rom_mario_size},
    {"Balloon Fight",  rom_balloon,  ROM_BALLOON_SIZE},
    {"Battle City",    rom_battlecity, ROM_BATTLECITY_SIZE},
    {"Bomberman",      rom_bomberman,  ROM_BOMBERMAN_SIZE},
    {"Duck Tales",     rom_ducktales,  ROM_DUCKTALES_SIZE},
    {"Saiyuuki World", rom_saiyuuki,   ROM_SAIYUUKI_SIZE},
};
#define N_GAMES (sizeof(games) / sizeof(games[0]))

static nes_t nes;
static volatile bool core1_running = true;

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
    for (int i = 0; i < N_GAMES; i++) {
        uint16_t clr = (i == cursor) ? CURSOR : WHITE;
        display_text(">", 1, 2 + i * 2, 1, clr, BG);
        display_text(games[i].name, 3, 2 + i * 2, 1, clr, BG);
    }
    draw_footer();
    display_flush();
}

static void core1_main(void) {
    while (core1_running) {
        joypad_poll();
        sleep_us(2000);
    }
}

#define NES_START 0x08

static void dump_regs(nes_t *n, int f) {
    printf("F%04d CT=%02X MS=%02X ST=%02X V=%04X T=%04X xf=%d wl=%d PC=%04X A=%02X X=%02X Y=%02X SP=%02X FL=%02X cy=%lu\n",
        f, n->ppu_ctrl, n->ppu_mask, n->ppu_status,
        n->v, n->t, n->x_fine, (int)n->w_latch,
        n->cpu.pc, n->cpu.a, n->cpu.x, n->cpu.y, n->cpu.sp, n->cpu.flags,
        (unsigned long)n->cpu.cycles);
}

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

        if (frame <= 3 || frame % 60 == 0)
            dump_regs(&nes, frame);

        if (!(pad & NES_START)) hold_exit++; else hold_exit = 0;
        if (hold_exit > 60) break;
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(500);

    printf("\n=== pico-retro boot ===\n");

    display_init();
    printf("display_init done\n");

    joypad_init();
    printf("joypad_init done\n");

    printf("pad at boot: %02X\n", joypad_buttons());

    display_fill(RED);   display_flush(); sleep_ms(500);
    printf("RED done\n");
    display_fill(GREEN); display_flush(); sleep_ms(500);
    printf("GREEN done\n");
    display_fill(BLUE);  display_flush(); sleep_ms(500);
    printf("BLUE done\n");

    multicore_launch_core1(core1_main);
    printf("core1 launched\n");

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

        if (edge & 1) { cursor = (cursor + N_GAMES - 1) % N_GAMES; draw_menu(cursor); }
        if (edge & 2) { cursor = (cursor + 1) % N_GAMES; draw_menu(cursor); }
        if (edge & 4) {
            printf("starting: %s mapper=%d prg=%ld idx\n", games[cursor].name, (long)nes.mapper, (unsigned long)cursor);
            nes_init(&nes, games[cursor].rom, games[cursor].size);
            printf("nes_init done: mapper=%d prg_sz=%ld chr_sz=%ld PC=%04X mask=%02X ctrl=%02X\n",
                (int)nes.mapper, (long)nes.prg_size, (long)nes.chr_size,
                nes.cpu.pc, nes.ppu_mask, nes.ppu_ctrl);
            run_nes();
            draw_menu(cursor);
        }
        prev_down = mask;
        sleep_ms(20);
    }
    return 0;
}
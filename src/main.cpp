#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"

extern "C" {
#include "display.h"
#include "joypad.h"
#include "config.h"
}

/* InfoNES system layer API from system_pico_retro.cpp */
extern "C" void infones_init(const uint8_t *rom, uint32_t size);
extern "C" void infones_run_frame(void);
extern "C" void infones_stop(void);
extern "C" void draw_film_strip(int first_idx, int highlight_idx);

/* InfoNES screen buffer (defined in system_pico_retro.cpp) */
extern uint8_t SCREEN[240][256];

#define BG      0x0000
#define CURSOR  RGB565(31, 31, 0)
#define TITLE   RGB565(31, 0, 0)
#define WHITE   RGB565(31, 63, 31)
#define BAR     RGB565(31, 63, 31)
#define GREEN   RGB565(0, 63, 0)
#define SPK_OUT  RGB565(20, 20, 20)
#define SPK_INN  RGB565(12, 12, 12)
#define SPK_HOLE RGB565(0, 0, 0)

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
#include "rom_mariobros.h"
#include "rom_mario.h"
#include "rom_contra.h"
#include "rom_dizzy.h"
#include "rom_nemo.h"

static const game_entry_t games[] = {
    {"Balloon Fight",  rom_balloon,  ROM_BALLOON_SIZE},
    {"Battle City",    rom_battlecity, ROM_BATTLECITY_SIZE},
    {"Bomberman",      rom_bomberman,  ROM_BOMBERMAN_SIZE},
    {"Contra",         rom_contra,    rom_contra_size},
    {"Duck Tales",     rom_ducktales,  ROM_DUCKTALES_SIZE},
    {"Duck Tales II",  rom_duck2,     rom_duck2_size},
    {"Fant. Adv. Dizzy", rom_dizzy,   rom_dizzy_size},
    {"Little Nemo",    rom_nemo,      rom_nemo_size},
    {"Mario Bros.",    rom_mariobros,  rom_mariobros_size},
    {"Saiyuuki World", rom_saiyuuki,   ROM_SAIYUUKI_SIZE},
    {"SMB",            rom_mario,     rom_mario_size},
};
#define N_GAMES (sizeof(games) / sizeof(games[0]))
#define MENU_SETTINGS (N_GAMES)
#define MENU_ABOUT    (N_GAMES + 1)
#define MENU_COUNT    (N_GAMES + 2)

static int gothic_font = 0;

static void draw_header(void) {
    display_text_12_gothic("PICO-RETRO", 8, 0, TITLE, BG);
    display_fill_rect(32, 16, 256, 2, BAR);
}

static void draw_footer(void) {
    display_fill_rect(32, 232, 256, 2, BAR);
    display_text_center("UP/DN  START  B=back", 29, 1, WHITE, BG);
}

static void draw_menu(int cursor) {
    display_fill(BG);
    draw_film_strip(cursor, cursor);
    draw_header();
    draw_header();

    int visible = 20;
    int scroll = cursor - visible / 2;
    if (scroll < 0) scroll = 0;
    if (MENU_COUNT <= visible) scroll = 0;
    else if (scroll > MENU_COUNT - visible) scroll = MENU_COUNT - visible;

    for (int i = scroll; i < scroll + visible && i < MENU_COUNT; i++) {
        int row = 3 + i - scroll;
        uint16_t clr = (i == cursor) ? CURSOR : WHITE;
        const char *name = (i < N_GAMES) ? games[i].name
                         : (i == MENU_SETTINGS) ? "Settings" : "About";
        if (gothic_font && i < N_GAMES) {
            display_text_12(">", 2, row, clr, BG);
            display_text_12(name, 3, row, clr, BG);
        } else {
            display_text(">", 4, row, 1, clr, BG);
            display_text(name, 5, row, 1, clr, BG);
        }
    }
    draw_footer();
    display_flush();
}

static void draw_about(void) {
    display_fill(BG);
    draw_header();

    display_text_center("About", 2, 2, CURSOR, BG);
    char buf[40];
    sprintf(buf, "Games: %d", N_GAMES);
    display_text(buf, 0, 7, 1, WHITE, BG);
    extern char __flash_binary_end;
    uint32_t flash = (uint32_t)&__flash_binary_end - 0x10000000;
    sprintf(buf, "Flash: %luK / 2048K", flash / 1024);
    display_text(buf, 0, 9, 1, WHITE, BG);
    extern uint8_t __bss_end__;
    extern uint8_t __StackLimit;
    uint32_t ram = (uint32_t)&__StackLimit - (uint32_t)&__bss_end__;
    sprintf(buf, "RAM:   %luK / 264K", ram / 1024);
    display_text(buf, 0, 11, 1, WHITE, BG);
    display_text("github.com/Mikl-GV/pico-retro", 0, 16, 1, GREEN, BG);
    draw_footer();
    display_flush();
}

static void draw_settings(int sel) {
    display_fill(BG);
    draw_header();
    display_text_center("Settings", 2, 2, CURSOR, BG);
    char clk[24];
    sprintf(clk, "CPU: %lu MHz", clock_get_hz(clk_sys) / 1000000);
    display_text(clk, 0, 7, 1, WHITE, BG);
    display_text("Disp: ILI9341V 8080", 0, 9, 1, WHITE, BG);
    display_text("Font: 8x8", 0, 11, 1, sel == 0 ? CURSOR : WHITE, BG);
    display_text("Font: Gothic", 0, 13, 1, sel == 1 ? CURSOR : WHITE, BG);
    draw_footer();
    display_flush();
}

static void run_settings(void) {
    int sel = gothic_font ? 1 : 0;
    uint32_t prev = 0;
    draw_settings(sel);
    while (true) {
        uint8_t pad = joypad_buttons();
        uint8_t down = ~pad;
        uint32_t mask = (((uint32_t)down >> 5) & 1) | (((uint32_t)(down >> 4) & 1) << 1);
        uint32_t edge = mask & ~prev;
        if (edge & 1) { sel = (sel + 1) % 2; draw_settings(sel); }
        if (edge & 2) { sel = (sel + 1) % 2; draw_settings(sel); }
        if (!(pad & 0x01)) { gothic_font = sel; sleep_ms(100); break; }
        if (!(pad & 0x02)) { sleep_ms(100); break; }
        prev = mask;
        sleep_ms(20);
    }
}

static void wait_b_press(void) {
    while (true) {
        uint8_t p = joypad_buttons();
        if (!(p & 0x02)) { sleep_ms(100); break; }
        sleep_ms(20);
    }
}

static void run_nes(void) {
    infones_run_frame();
    infones_stop();
}

int main(void) {
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(10);
    set_sys_clock_khz(250000, true);

    printf("\n=== pico-retro boot ===\n");
    display_init();
    joypad_init();
    printf("init done\n");

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
            if (cursor == MENU_ABOUT) { draw_about(); wait_b_press(); draw_menu(cursor); }
            else if (cursor == MENU_SETTINGS) { run_settings(); draw_menu(cursor); }
            else {
                infones_init(games[cursor].rom, games[cursor].size);
                run_nes();
                draw_menu(cursor);
            }
        }
        prev_down = mask;
        sleep_ms(20);
    }
}
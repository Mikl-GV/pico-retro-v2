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

/* InfoNES system layer */
extern "C" void infones_init(const uint8_t *rom, uint32_t size);
extern "C" void infones_run_frame(void);
extern "C" void infones_stop(void);
extern "C" void draw_film_strip(int first_idx, int highlight_idx);

/* Atari 2600 system layer */
extern "C" void atari2600_init(const uint8_t *rom, uint32_t size);
extern "C" void atari2600_run_frame(void);
extern "C" void atari2600_render(void);
extern "C" void atari2600_poll_joy(void);

#define BG      0x0000
#define CURSOR  RGB565(31, 31, 0)
#define TITLE   RGB565(31, 0, 0)
#define WHITE   RGB565(31, 63, 31)
#define BAR     RGB565(31, 63, 31)
#define GREEN   RGB565(0, 63, 0)

/* ---------- Игры NES ---------- */
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

static const game_entry_t nes_games[] = {
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
#define N_NES_GAMES (sizeof(nes_games) / sizeof(nes_games[0]))

/* ---------- Игры Atari 2600 ---------- */
#include "rom_halo2600.h"
#include "rom_thrust.h"
#include "rom_dkarcade2600.h"
#include "rom_airsea.h"
#include "rom_asteroids.h"
#include "rom_calgames.h"

static const game_entry_t a2600_games[] = {
    {"Halo 2600",        halo2600,    halo2600_size},
    {"Thrust",           thrust,      thrust_size},
    {"DK Arcade",        dkarcade2600, dkarcade2600_size},
    {"Air-Sea Battle",   airsea,      airsea_size},
    {"Asteroids",        asteroids,   asteroids_size},
    {"California Games", calgames,    calgames_size},
};
#define N_A2600_GAMES (sizeof(a2600_games) / sizeof(a2600_games[0]))

/* ---------- Системы ---------- */
typedef struct {
    const char *name;
    const game_entry_t *games;
    int count;
    int emu; /* 0 = NES, 1 = Atari 2600 */
} system_entry_t;

static const system_entry_t systems[] = {
    {"NES",          nes_games,  N_NES_GAMES,  0},
    {"Atari 2600",   a2600_games, N_A2600_GAMES, 1},
};
#define N_SYSTEMS (sizeof(systems) / sizeof(systems[0]))
#define SYS_SETTINGS (N_SYSTEMS)
#define SYS_ABOUT    (N_SYSTEMS + 1)
#define SYS_COUNT    (N_SYSTEMS + 2)

/* ---------- Экранные элементы ---------- */
static void draw_header(const char *subtitle) {
    display_text("PICO-RETRO", 5, 0, 2, TITLE, BG);
    display_fill_rect(32, 16, 256, 2, BAR);
    if (subtitle) display_text_center(subtitle, 2, 1, GREEN, BG);
}

static void draw_footer(void) {
    display_fill_rect(32, 232, 256, 2, BAR);
    display_text_center("UP/DN  START  B=back", 29, 1, WHITE, BG);
}

/* Универсальный список: заголовок + пункты (игры/системы) */
static void draw_list(const char *title, const char * const *items, int count,
                      int cursor, int start_x) {
    display_fill(BG);
    draw_film_strip(cursor, cursor);
    draw_header(title);
    for (int i = 0; i < count && i < 17; i++) {
        int py = 22 + i * 11;
        uint16_t clr = (i == cursor) ? CURSOR : WHITE;
        display_text_at_nobg(">", start_x, py, 1, clr);
        display_text_at_nobg(items[i], start_x + 8, py, 1, clr);
    }
    draw_footer();
    display_flush();
}

/* ---------- Меню систем ---------- */
static void draw_system_menu(int cursor) {
    static const char *names[SYS_COUNT];
    for (int i = 0; i < N_SYSTEMS; i++) names[i] = systems[i].name;
    names[SYS_SETTINGS] = "Settings";
    names[SYS_ABOUT] = "About";
    draw_list("SELECT SYSTEM", names, SYS_COUNT, cursor, 40);
}

/* ---------- Меню игр ---------- */
static void draw_game_menu(int sys, int cursor) {
    static const char *names[32];
    int n = systems[sys].count;
    for (int i = 0; i < n; i++) names[i] = systems[sys].games[i].name;
    char title[32];
    sprintf(title, "GAMES - %s", systems[sys].name);
    draw_list(title, names, n, cursor, 40);
}

static void draw_about(void) {
    display_fill(BG);
    draw_header(NULL);
    display_text_center("About", 2, 2, CURSOR, BG);
    char buf[40];
    sprintf(buf, "Games: %d", N_NES_GAMES + N_A2600_GAMES);
    display_text(buf, 0, 7, 1, WHITE, BG);
    display_text("github.com/Mikl-GV/pico-retro", 0, 16, 1, GREEN, BG);
    draw_footer();
    display_flush();
}

static void draw_settings(void) {
    display_fill(BG);
    draw_header(NULL);
    display_text_center("Settings", 2, 2, CURSOR, BG);
    char clk[24];
    sprintf(clk, "CPU: %lu MHz", clock_get_hz(clk_sys) / 1000000);
    display_text(clk, 0, 7, 1, WHITE, BG);
    display_text("Disp: ILI9341V 8080", 0, 9, 1, WHITE, BG);
    draw_footer();
    display_flush();
}

static void wait_b_press(void) {
    while (true) {
        uint8_t p = joypad_buttons();
        if (!(p & 0x02)) { sleep_ms(100); break; }
        sleep_ms(20);
    }
}

/* ---------- Запуск игры по системе ---------- */
static void run_system(int sys, int game) {
    const game_entry_t *g = &systems[sys].games[game];
    if (systems[sys].emu == 0) {
        /* NES */
        infones_init(g->rom, g->size);
        infones_run_frame();   /* InfoNES_Cycle() крутится внутри, до выхода */
        infones_stop();
    } else {
        /* Atari 2600 */
        atari2600_init(g->rom, g->size);
        uint32_t hold_frames = 0;
        while (true) {
            uint8_t pad = joypad_buttons();
            atari2600_poll_joy();
            atari2600_run_frame();
            atari2600_render();
            /* Выход: Start удержан ~2 сек (по кадрам, надёжно) */
            if (!(pad & 0x08)) hold_frames++; else hold_frames = 0;
            if (hold_frames > 120) break;
        }
    }
}

int main(void) {
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(10);
    set_sys_clock_khz(250000, true);

    printf("\n=== pico-retro boot ===\n");
    display_init();
    joypad_init();
    printf("init done\n");

    int sys_cursor = 0;
    draw_system_menu(sys_cursor);
    uint32_t prev_down = 0;

    for (;;) {
        uint8_t pad = joypad_buttons();
        uint8_t down = ~pad;
        uint32_t mask = (((uint32_t)down >> 4) & 1)
                      | (((uint32_t)(down >> 5) & 1) << 1)
                      | (((uint32_t)(down >> 3) & 1) << 2)
                      | (((uint32_t)(down >> 1) & 1) << 3);
        uint32_t edge = mask & ~prev_down;

        if (edge & 1) { sys_cursor = (sys_cursor + SYS_COUNT - 1) % SYS_COUNT; draw_system_menu(sys_cursor); }
        if (edge & 2) { sys_cursor = (sys_cursor + 1) % SYS_COUNT; draw_system_menu(sys_cursor); }
        if (edge & 4) {
            if (sys_cursor == SYS_ABOUT) { draw_about(); wait_b_press(); draw_system_menu(sys_cursor); }
            else if (sys_cursor == SYS_SETTINGS) { draw_settings(); wait_b_press(); draw_system_menu(sys_cursor); }
            else {
                /* Подменю игр выбранной системы */
                int game_cursor = 0;
                int n = systems[sys_cursor].count;
                draw_game_menu(sys_cursor, game_cursor);
                /* Ждём, пока пользователь отпустит Start (0x08) и A (0x01),
                 * чтобы они не сработали как выбор/запуск в подменю */
                while (!((joypad_buttons() & 0x09) == 0x09)) sleep_ms(10);
                uint32_t prev2 = 0;
                while (true) {
                    uint8_t p2 = joypad_buttons();
                    uint8_t d2 = ~p2;
                    uint32_t m2 = (((uint32_t)d2 >> 4) & 1) | (((uint32_t)(d2 >> 5) & 1) << 1)
                                | (((uint32_t)(d2 >> 3) & 1) << 2) | (((uint32_t)(d2 >> 1) & 1) << 3);
                    uint32_t e2 = m2 & ~prev2;
                    if (e2 & 1) { game_cursor = (game_cursor + n - 1) % n; draw_game_menu(sys_cursor, game_cursor); }
                    if (e2 & 2) { game_cursor = (game_cursor + 1) % n; draw_game_menu(sys_cursor, game_cursor); }
                    if (e2 & 4) { run_system(sys_cursor, game_cursor); draw_game_menu(sys_cursor, game_cursor); }
                    if (!(p2 & 0x02)) { sleep_ms(100); break; }  /* B — назад */
                    prev2 = m2;
                    sleep_ms(20);
                }
                draw_system_menu(sys_cursor);
            }
        }
        prev_down = mask;
        sleep_ms(20);
    }
}

/* System layer for InfoNES on pico-retro RP2040 */

#include "InfoNES_System.h"
#include "InfoNES.h"
#include "InfoNES_Mapper.h"
#include "InfoNES_pAPU.h"
#include "K6502.h"

#include <pico/stdlib.h>
#include <cstdio>
#include <cstdarg>

extern "C" {
#include "display.h"
#include "joypad.h"
}

#include "../include/thumbs.h"

/* SCREEN doubles as the shared RAM pool for the Atari core (buffers placed
 * inside it via system_atari_mcume.cpp). Must stay 8-byte aligned so the
 * Atari colvect buffer (cast to uint32*) keeps 4-byte alignment. */
__attribute__((aligned(8)))
uint8_t SCREEN[NES_DISP_HEIGHT][NES_DISP_WIDTH];
static uint16_t line_buf[NES_DISP_WIDTH];

extern WORD *WorkLine;
void InfoNES_SetLineBuffer(WORD *p, WORD size);

const BYTE NesPalette[64] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f
};

static uint16_t nes_pal_rgb565[64];
static bool running = false;
static uint32_t hold_exit = 0;

static void init_palette(void) {
    static const uint8_t pal[64][3] = {
        {84,84,84},{0,30,116},{8,16,144},{48,0,136},{68,0,100},{92,0,48},{84,4,0},{60,24,0},
        {32,42,0},{8,58,0},{0,64,0},{0,60,0},{0,50,60},{0,0,0},{0,0,0},{0,0,0},
        {152,150,152},{8,76,196},{48,50,236},{92,30,228},{136,20,176},{160,20,100},{152,34,32},{120,60,0},
        {84,90,0},{40,114,0},{8,124,0},{0,118,40},{0,102,120},{0,0,0},{0,0,0},{0,0,0},
        {236,238,236},{76,154,236},{120,124,236},{176,98,236},{228,84,236},{236,88,180},{236,106,100},{212,136,32},
        {160,170,0},{116,196,0},{76,208,32},{56,204,108},{56,180,204},{60,60,60},{0,0,0},{0,0,0},
        {236,238,236},{168,204,236},{188,188,236},{212,178,236},{236,174,236},{236,174,212},{236,180,176},{228,196,144},
        {204,210,120},{180,222,120},{168,226,144},{152,226,180},{160,214,228},{160,162,160},{0,0,0},{0,0,0},
    };
    for (int i = 0; i < 64; i++)
        nes_pal_rgb565[i] = (pal[i][0]>>3<<11) | (pal[i][1]>>2<<5) | (pal[i][2]>>3);
}

void InfoNES_PreDrawLine(int) {}
void InfoNES_PostDrawLine(int line) {
    if (line < 0 || line >= NES_DISP_HEIGHT) return;
    for (int x = 0; x < NES_DISP_WIDTH; x++)
        SCREEN[line][x] = (uint8_t)(line_buf[x] & 0xFF);
}
void RomSelect_PreDrawLine(int) {}

int InfoNES_LoadFrame(void) {
    display_stream_begin(32, 0, 256, 240);
    display_stream_pixels(&SCREEN[0][0], nes_pal_rgb565, 256, 240);
    display_stream_end();
    return 0;
}
int InfoNES_Menu(void) { return 0; }
int InfoNES_ReadRom(const char *) { return 0; }
void InfoNES_ReleaseRom(void) {}

void InfoNES_PadState(DWORD *pw1, DWORD *pw2, DWORD *pwSys) {
    uint8_t pad = joypad_buttons();
    *pw1 = (DWORD)(~pad) & 0xFF;
    *pw2 = 0;
    if (!(pad & 0x08)) hold_exit++; else hold_exit = 0;
    *pwSys = (hold_exit > 60) ? PAD_SYS_QUIT : 0;
}

void InfoNES_MessageBox(const char *m, ...) { va_list a; va_start(a,m); vprintf(m,a); va_end(a); }
void InfoNES_Error(const char *m, ...) { va_list a; va_start(a,m); vprintf(m,a); va_end(a); running=false; }
void InfoNES_DebugPrint(const char *m) { printf("%s", m); }
void InfoNES_SoundInit(void) {}
int InfoNES_SoundOpen(int,int) { return 0; }
void InfoNES_SoundClose(void) {}
void InfoNES_SoundOutput(int, const BYTE*, const BYTE*, const BYTE*, const BYTE*, const BYTE*) {}
int InfoNES_GetSoundBufferSize() { return 0; }

extern "C" {

void draw_film_strip(int first_idx, int highlight_idx) {
    static bool pal_ready = false;
    if (!pal_ready) { init_palette(); pal_ready = true; }
    static const uint8_t * const thumbs[] = {
        thumb_balloon, thumb_battlecity, thumb_bomberman, thumb_contra,
        thumb_ducktales, thumb_duck2, thumb_dizzy, thumb_nemo,
        thumb_mariobros, thumb_saiyuuki, thumb_smb,
        thumb_zelda, thumb_megaman, thumb_castlevania, thumb_metroid
    };
    uint16_t white = RGB565(31, 63, 31);

    /* Purple background with stars (9999-in-1 style) */
    for (int y = 0; y < 240; y++) {
        int bright = 8 + (y * 10) / 240;
        uint16_t purple = RGB565(bright, 0, bright);
        for (int x = 32; x < 288; x++)
            display_set_pixel(x, y, purple);
    }
    /* Random stars */
    static const uint8_t stars[] = {
        40,10, 80,5, 130,8, 180,3, 230,12, 50,25, 100,30, 150,20, 200,15, 250,28,
        60,40, 120,45, 170,35, 220,38, 255,22, 45,60, 90,55, 140,50, 190,48, 240,58,
        55,80, 110,75, 160,70, 210,85, 255,72, 70,100, 130,95, 180,90, 230,105, 255,88,
        50,130, 100,120, 150,115, 200,130, 250,118, 65,150, 120,155, 170,145, 220,160, 255,140,
        80,180, 130,185, 180,175, 230,190, 255,170, 60,210, 110,205, 160,200, 210,215, 250,195,
    };
    for (int i = 0; i < sizeof(stars); i += 2)
        display_set_pixel(32 + stars[i], stars[i+1], white);

    /* Orange gradient side panels */
    for (int y = 0; y < 240; y++) {
        int brightness = 15 + (y * 12) / 240;
        uint16_t orange = RGB565(31, brightness, 0);
        for (int x = 0; x < 32; x++) {
            display_set_pixel(x, y, orange);
            display_set_pixel(288 + x, y, orange);
        }
    }

    /* Sprocket holes (перфорация) — чёрные, у внутреннего края панелей */
    /* Рисуем ПОСЛЕ кадров, чтобы не перекрывались */

    for (int side = 0; side < 2; side++) {
        int x0 = side ? 292 : 3;
        for (int i = 0; i < 8; i++) {
            int gi = (first_idx + i + side * 8) % 15;
            int y = 4 + i * 29;
            const uint8_t *t = thumbs[gi];
            for (int py = 0; py < 25; py++)
                for (int px = 0; px < 25; px++)
                    display_set_pixel(x0 + px, y + py, nes_pal_rgb565[t[py*25+px] & 0x3F]);
            if (gi == highlight_idx && !(side == 0 && i == 0) && !(side == 1 && i == 7)) {
                for (int px = 0; px < 25; px++) {
                    display_set_pixel(x0 + px, y, white);
                    display_set_pixel(x0 + px, y + 24, white);
                }
                for (int py = 0; py < 25; py++) {
                    display_set_pixel(x0, y + py, white);
                    display_set_pixel(x0 + 24, y + py, white);
                }
            }
        }
    }

    /* Сплошная перфорация киноплёнки по ОБЕИМ сторонам кадров */
    /* Внешние края: x=0 (лево) и x=317 (право) */
    /* Внутренние края: x=28 (лево) и x=288 (право) */
    for (int y = 2; y < 238; y += 5) {
        display_fill_rect(1, y, 2, 2, 0x0000);    /* левый внешний */
        display_fill_rect(29, y, 2, 2, 0x0000);   /* левый внутренний */
        display_fill_rect(289, y, 2, 2, 0x0000);  /* правый внутренний */
        display_fill_rect(317, y, 2, 2, 0x0000);  /* правый внешний */
    }
}

void infones_init(const uint8_t *rom, uint32_t sz) {
    init_palette();
    InfoNES_SetLineBuffer(line_buf, NES_DISP_WIDTH);
    NesHeader.byID[0]=rom[0]; NesHeader.byID[1]=rom[1];
    NesHeader.byID[2]=rom[2]; NesHeader.byID[3]=rom[3];
    NesHeader.byRomSize=rom[4]; NesHeader.byVRomSize=rom[5];
    NesHeader.byInfo1=rom[6]; NesHeader.byInfo2=rom[7];
    for(int i=0;i<8;i++) NesHeader.byReserve[i]=rom[8+i];
    uint32_t ps = NesHeader.byRomSize*0x4000;
    ROM = (BYTE*)(rom+16);
    VROM = NesHeader.byVRomSize ? (BYTE*)(rom+16+ps) : NULL;
    hold_exit = 0;
    InfoNES_Init();
    InfoNES_Reset();
    running = true;
}

void infones_run_frame(void) { InfoNES_Cycle(); }
void infones_stop(void) { running = false; }

}
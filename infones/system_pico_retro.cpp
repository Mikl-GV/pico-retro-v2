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

void draw_film_strip(int first_idx) {
    static const uint8_t * const thumbs[] = {
        thumb_balloon, thumb_battlecity, thumb_bomberman, thumb_contra,
        thumb_ducktales, thumb_duck2, thumb_dizzy, thumb_nemo,
        thumb_mariobros, thumb_saiyuuki, thumb_smb
    };
    uint16_t out = RGB565(20,20,20);
    uint16_t inn = RGB565(12,12,12);
    uint16_t bdr = RGB565(31,31,31);
    display_fill_rect(0, 0, 26, 240, out);
    display_fill_rect(2, 2, 22, 236, inn);
    display_fill_rect(294, 0, 26, 240, out);
    display_fill_rect(296, 2, 22, 236, inn);
    for (int i = 0; i < 10; i++) {
        int y = 8 + i * 24;
        display_fill_rect(10, y, 6, 10, 0x0000);
        display_fill_rect(304, y, 6, 10, 0x0000);
    }
    for (int side = 0; side < 2; side++) {
        for (int i = 0; i < 4; i++) {
            int gi = (first_idx + i + side * 4) % 11;
            int y = 36 + i * 56;
            int x0 = side ? 295 : 1;
            display_fill_rect(x0, y, 24, 24, bdr);
            const uint8_t *t = thumbs[gi];
            for (int py = 0; py < 20; py++)
                for (int px = 0; px < 20; px++)
                    display_set_pixel(x0 + 2 + px, y + 2 + py, nes_pal_rgb565[t[py*20+px] & 0x3F]);
        }
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
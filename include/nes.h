#ifndef NES_H
#define NES_H

#include <stdint.h>
#include <stdbool.h>
#include "cpu6502.h"

typedef struct {
    uint8_t wram[0x800];
    const uint8_t *prg_rom;
    uint32_t prg_size;
    const uint8_t *chr_rom;
    uint32_t chr_size;
    uint8_t mapper;
    uint8_t mirror;

    /* Mapper 2 (UxROM) */
    uint8_t prg_bank;

    /* Mapper 4 (MMC3) */
    uint8_t mmc3_reg[8];
    uint8_t mmc3_bank_select;
    uint8_t mmc3_prg_mode;
    uint8_t mmc3_chr_mode;
    uint8_t mmc3_mirror;
    uint8_t mmc3_irq_latch;
    uint8_t mmc3_irq_counter;
    bool    mmc3_irq_enable;
    bool    mmc3_irq_reload;
    uint8_t mmc3_ram_protect;
    uint8_t mmc3_prg_ram[0x2000];

    /* CHR RAM (для игр с CHR=0 в заголовке) */
    uint8_t chr_ram[0x2000];

    /* PPU */
    uint8_t vram[0x800];
    uint8_t palette[32];
    uint8_t oam[256];
    uint8_t ppu_ctrl, ppu_mask, ppu_status;
    uint8_t ppu_scroll, ppu_addr, ppu_data;
    uint16_t v, t;
    uint8_t x_fine;
    bool w_latch;
    uint8_t ppu_read_buf;
    uint32_t frame;

    uint8_t fb[240][256];

    /* APU */
    uint8_t joy1_latch;
    uint8_t joy2_latch;
    uint8_t joy1_strobe;
    uint8_t joy1_buttons;
    uint32_t rd4016_cnt;
    uint32_t rd4017_cnt;

    cpu6502_t cpu;
} nes_t;

void nes_init(nes_t *nes, const uint8_t *rom, uint32_t rom_size);
void nes_run_frame(nes_t *nes);
void nes_set_joy(nes_t *nes, uint8_t btns);
uint16_t ppu_color(uint8_t idx);
extern uint16_t ppu_lut[64];

#endif
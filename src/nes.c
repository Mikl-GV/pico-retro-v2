#include "nes.h"
#include <string.h>

static nes_t *g;

static uint16_t nt_mirror(uint16_t addr) {
    addr &= 0xFFF;
    uint8_t nt = addr >> 10;
    if (g->mapper == 4 && (g->mmc3_mirror & 1)) {
        /* MMC3 controls mirroring via $A000 write */
        if (g->mmc3_mirror & 1) {
            return addr & 0x3FF;
        }
    }
    if (g->mirror == 0) {
        if (nt >= 2) addr = (addr & 0x3FF) | 0x400;
    } else {
        if (nt == 1 || nt == 3) addr = (addr & 0x3FF) | 0x400;
        if (nt == 2 || nt == 3) addr = (addr & 0x3FF) | 0x800;
    }
    return addr & 0x7FF;
}

static uint8_t chr_read(nes_t *nes, uint16_t addr) {
    if (nes->mapper == 4 && nes->chr_rom) {
        uint8_t bank = addr >> 10;
        uint16_t off = addr & 0x3FF;
        if (nes->mmc3_chr_mode == 0) {
            /* mode 0: R0(2KB) @ $0000, R1(2KB) @ $0800, R2..R5(1KB) @ $1000..$1C00 */
            if (bank == 0) return nes->chr_rom[((nes->mmc3_reg[0] & 0xFE) + 0) * 0x400 + off];
            if (bank == 1) return nes->chr_rom[((nes->mmc3_reg[0] & 0xFE) + 1) * 0x400 + off];
            if (bank == 2) return nes->chr_rom[((nes->mmc3_reg[1] & 0xFE) + 0) * 0x400 + off];
            if (bank == 3) return nes->chr_rom[((nes->mmc3_reg[1] & 0xFE) + 1) * 0x400 + off];
            return nes->chr_rom[(nes->mmc3_reg[bank] & 0xFE) * 0x400 + off];
        } else {
            /* mode 1: R2..R5(1KB) @ $0000..$0C00, R0(2KB) @ $1000, R1(2KB) @ $1800 */
            if (bank < 4)  return nes->chr_rom[(nes->mmc3_reg[bank + 2] & 0xFE) * 0x400 + off];
            if (bank == 4) return nes->chr_rom[((nes->mmc3_reg[0] & 0xFE) + 0) * 0x400 + off];
            if (bank == 5) return nes->chr_rom[((nes->mmc3_reg[0] & 0xFE) + 1) * 0x400 + off];
            if (bank == 6) return nes->chr_rom[((nes->mmc3_reg[1] & 0xFE) + 0) * 0x400 + off];
            return nes->chr_rom[((nes->mmc3_reg[1] & 0xFE) + 1) * 0x400 + off];
        }
    }
    if (nes->chr_rom) return nes->chr_rom[addr];
    return nes->chr_ram[addr];
}

static void chr_write(nes_t *nes, uint16_t addr, uint8_t v) {
    if (nes->mapper == 4 && !nes->chr_rom) {
        uint8_t bank = addr >> 10;
        uint16_t off = addr & 0x3FF;
        uint8_t *r = nes->chr_ram;
        if (nes->mmc3_chr_mode == 0) {
            if (bank == 0)      r[((nes->mmc3_reg[0] & 0xFE) + 0) * 0x400 + off] = v;
            else if (bank == 1) r[((nes->mmc3_reg[0] & 0xFE) + 1) * 0x400 + off] = v;
            else if (bank == 2) r[((nes->mmc3_reg[1] & 0xFE) + 0) * 0x400 + off] = v;
            else if (bank == 3) r[((nes->mmc3_reg[1] & 0xFE) + 1) * 0x400 + off] = v;
            else                r[(nes->mmc3_reg[bank] & 0xFE) * 0x400 + off] = v;
        } else {
            if (bank < 4)      r[(nes->mmc3_reg[bank + 2] & 0xFE) * 0x400 + off] = v;
            else if (bank == 4) r[((nes->mmc3_reg[0] & 0xFE) + 0) * 0x400 + off] = v;
            else if (bank == 5) r[((nes->mmc3_reg[0] & 0xFE) + 1) * 0x400 + off] = v;
            else if (bank == 6) r[((nes->mmc3_reg[1] & 0xFE) + 0) * 0x400 + off] = v;
            else                r[((nes->mmc3_reg[1] & 0xFE) + 1) * 0x400 + off] = v;
        }
        return;
    }
    if (!nes->chr_rom) nes->chr_ram[addr] = v;
}

static uint8_t rd(uint16_t a) {
    if (a < 0x2000) return g->wram[a & 0x7FF];
    if (a < 0x4000) {
        uint8_t reg = a & 7;
        if (reg == 2) { uint8_t s = g->ppu_status; g->ppu_status &= ~0x80; g->w_latch = false; return (s & 0xE0) | (g->ppu_read_buf & 0x1F); }
        if (reg == 7) {
            uint16_t va = g->v & 0x3FFF;
            uint8_t val;
            if (va < 0x2000) val = chr_read(g, va);
            else if (va >= 0x3F00) { uint8_t pi = va & 0x1F; if (pi >= 0x10 && (pi & 3) == 0) pi -= 0x10; val = g->palette[pi]; }
            else val = g->vram[nt_mirror(va)];
            g->v += (g->ppu_ctrl & 4) ? 32 : 1;
            if (va >= 0x3F00) { g->ppu_read_buf = val; return val; }
            uint8_t buf = g->ppu_read_buf; g->ppu_read_buf = val; return buf;
        }
        return 0;
    }
    if (a == 0x4015) return 0x00;
    if (a == 0x4016) { uint8_t j = g->joy1_latch & 1; if (!g->joy1_strobe) g->joy1_latch = (g->joy1_latch >> 1) | 0x80; g->rd4016_cnt++; return j | 0x40; }
    if (a == 0x4017) { uint8_t j = g->joy2_latch & 1; if (!g->joy1_strobe) g->joy2_latch = (g->joy2_latch >> 1) | 0x80; g->rd4017_cnt++; return j | 0x40; }
    if (a >= 0x4000 && a < 0x6000) return 0;
    if (a >= 0x6000 && a < 0x8000) {
        if (g->mapper == 4) return g->mmc3_prg_ram[a - 0x6000];
        return 0;
    }
    if (g->mapper == 4) {
        uint8_t b6 = g->mmc3_reg[6] & 0x3F;
        uint8_t b7 = g->mmc3_reg[7] & 0x3F;
        uint8_t last = (uint8_t)((g->prg_size / 0x2000) - 2);
        uint8_t last_b = (uint8_t)((g->prg_size / 0x2000) - 1);
        if (a >= 0xE000) {
            return g->prg_rom[last_b * 0x2000 + (a - 0xE000)];
        }
        if (a >= 0xC000) {
            uint8_t bank = g->mmc3_prg_mode ? b6 : last;
            return g->prg_rom[bank * 0x2000 + (a - 0xC000)];
        }
        if (a >= 0xA000) {
            return g->prg_rom[b7 * 0x2000 + (a - 0xA000)];
        }
        if (a >= 0x8000) {
            uint8_t bank = g->mmc3_prg_mode ? last_b : b6;
            return g->prg_rom[bank * 0x2000 + (a - 0x8000)];
        }
        return 0;
    }
    if (g->mapper == 2) {
        if (a >= 0xC000) return g->prg_rom[g->prg_size - 0x4000 + (a - 0xC000)];
        if (a >= 0x8000) return g->prg_rom[g->prg_bank * 0x4000 + (a - 0x8000)];
        return 0;
    }
    if (a >= 0xC000) return g->prg_rom[((a - 0xC000) & (g->prg_size - 1))];
    if (a >= 0x8000) return g->prg_rom[((a - 0x8000) & (g->prg_size - 1))];
    return 0;
}

static void wr(uint16_t a, uint8_t v) {
    if (a < 0x2000) { g->wram[a & 0x7FF] = v; return; }
    if (a < 0x4000) {
        uint8_t reg = a & 7;
        switch (reg) {
        case 0: g->ppu_ctrl = v; g->t = (g->t & 0xF3FF) | ((uint16_t)(v & 3) << 10); break;
        case 1: g->ppu_mask = v; break;
        case 5: if (!g->w_latch) { g->t = (g->t & 0xFFE0) | (v >> 3); g->x_fine = v & 7; } else { g->t = (g->t & 0xFC1F) | (((uint16_t)v & 0xF8) << 2); g->t = (g->t & 0x8FFF) | (((uint16_t)v & 7) << 12); } g->w_latch = !g->w_latch; break;
        case 6: if (!g->w_latch) g->t = (g->t & 0x00FF) | (((uint16_t)(v & 0x3F)) << 8); else { g->t = (g->t & 0xFF00) | v; g->v = g->t; } g->w_latch = !g->w_latch; break;
        case 7: { uint16_t va = g->v & 0x3FFF; g->v += (g->ppu_ctrl & 4) ? 32 : 1; if (va >= 0x3F00) { uint8_t pi = va & 0x1F; if (pi >= 0x10 && (pi & 3) == 0) pi -= 0x10; g->palette[pi] = v & 0x3F; return; } if (va >= 0x2000) g->vram[nt_mirror(va)] = v; else chr_write(g, va, v); break; }
        }
        return;
    }
    if (a == 0x4014) { uint16_t o = (uint16_t)v << 8; for (int i = 0; i < 256; i++) g->oam[i] = g->wram[o + i]; return; }
    if (a == 0x4016) { g->joy1_strobe = v & 1; if (g->joy1_strobe) g->joy1_latch = g->joy1_buttons; }
    if (a >= 0x6000 && a < 0x8000) {
        if (g->mapper == 4) {
            if (!g->mmc3_ram_protect) g->mmc3_prg_ram[a - 0x6000] = v;
        }
        return;
    }
    if (g->mapper == 4) {
        /* MMC3: $8000-$9FFF even = bank select, odd = bank data */
        if (a < 0xA000) {
            if (!(a & 1)) {
                g->mmc3_bank_select = v & 7;
                g->mmc3_prg_mode = (v >> 6) & 1;
                g->mmc3_chr_mode = (v >> 7) & 1;
            } else {
                g->mmc3_reg[g->mmc3_bank_select] = v;
            }
            return;
        }
        /* $A000-$BFFF: mirroring / PRG-RAM protect */
        if (a < 0xC000) {
            if (!(a & 1)) g->mmc3_mirror = v & 1;
            else g->mmc3_ram_protect = (v & 0xC0) ? 1 : 0;
            return;
        }
        /* $C000-$DFFF: IRQ latch / reload */
        if (a < 0xE000) {
            if (!(a & 1)) g->mmc3_irq_latch = v;
            else g->mmc3_irq_counter = 0;
            return;
        }
        /* $E000-$FFFF: IRQ enable / disable */
        if (!(a & 1)) { g->mmc3_irq_enable = false; }
        else { g->mmc3_irq_enable = true; }
        return;
    }
    if (a >= 0x8000 && g->mapper == 2) { g->prg_bank = v & 7; }
}

uint16_t ppu_lut[64];

static uint16_t make565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (uint16_t)(b >> 3);
}

uint16_t ppu_color(uint8_t idx) {
    return ppu_lut[idx & 0x3F];
}

void nes_init(nes_t *nes, const uint8_t *rom, uint32_t sz) {
    memset(nes, 0, sizeof(*nes));
    g = nes;

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
    for (int i = 0; i < 64; i++) ppu_lut[i] = make565(pal[i][0], pal[i][1], pal[i][2]);

    nes->prg_size = rom[4] * 0x4000;
    nes->chr_size = rom[5] * 0x2000;
    nes->mapper = (rom[6] >> 4) | (rom[7] & 0xF0);
    nes->mirror = rom[6] & 1;
    nes->prg_rom = rom + 16;
    nes->chr_rom = (rom[5] == 0) ? NULL : rom + 16 + nes->prg_size;

    if (nes->mapper == 4) {
        nes->mmc3_reg[6] = 0;
        nes->mmc3_reg[7] = 1;
        nes->mmc3_mirror = nes->mirror;
    }

    cpu6502_init(&nes->cpu, rd, wr);
    memset(nes->fb, 0, sizeof(nes->fb));
    nes->joy1_buttons = 0xFF;
    nes->joy1_latch = 0xFF;
}

void nes_set_joy(nes_t *nes, uint8_t btns) {
    g = nes;
    g->joy1_buttons = btns;
}

void nes_run_frame(nes_t *nes) {
    g = nes;
    g->rd4016_cnt = 0;
    g->rd4017_cnt = 0;

    int32_t dot_bank = 0;

    for (int scanline = 0; scanline < 262; scanline++) {
        if (scanline == 241) {
            g->ppu_status |= 0x80;
            if (g->ppu_ctrl & 0x80) cpu6502_nmi(&g->cpu);
        }

        if (g->mapper == 4 && g->mmc3_irq_enable && scanline < 240) {
            if (g->mmc3_irq_counter == 0) {
                g->mmc3_irq_counter = g->mmc3_irq_latch;
            } else {
                g->mmc3_irq_counter--;
                if (g->mmc3_irq_counter == 0) {
                    g->cpu.irq_pending = true;
                }
            }
        }

        if (scanline < 240 && (g->ppu_mask & 0x08)) {
            int fine_y = (scanline + (g->t >> 12)) & 7;
            int coarse_y = ((scanline + (g->t >> 12)) >> 3) & 31;
            int nt_x_off = (g->t >> 10) & 3;
            int nt_base = 0x2000 + (nt_x_off * 0x400);
            int pt_base = (g->ppu_ctrl & 0x10) ? 0x1000 : 0;
            int scroll = (g->t & 0x1F) | (g->x_fine << 3);

            for (int tx = 0; tx < 33; tx++) {
                int phys = (tx * 8 + scroll) >> 3;
                int px_start = tx * 8 - (scroll & 7);

                if (phys < 0 || phys >= 32) continue;

                uint8_t tile = g->vram[nt_mirror(nt_base + (uint16_t)(coarse_y * 32 + phys))];
                uint8_t at   = g->vram[nt_mirror(nt_base + 0x3C0 + (uint16_t)((coarse_y >> 2) * 8 + (phys >> 2)))];
                int ps = ((phys & 2) ? 2 : 0) | ((coarse_y & 2) ? 4 : 0);
                int pal = ((at >> ps) & 3) << 2;

                uint16_t taddr = (uint16_t)(pt_base + tile * 16 + fine_y);
                uint8_t lo = chr_read(g, taddr);
                uint8_t hi = chr_read(g, taddr + 8);

                for (int px = 0; px < 8; px++) {
                    int sx = px_start + px;
                    if (sx < 0 || sx >= 256) continue;
                    int ci = ((hi & 0x80) ? 2 : 0) | ((lo & 0x80) ? 1 : 0);
                    hi <<= 1; lo <<= 1;
                    g->fb[scanline][sx] = ci ? g->palette[pal + ci] : g->palette[0];
                }
            }
        }

        if (scanline < 240 && (g->ppu_mask & 0x10)) {
            int spr_h = (g->ppu_ctrl & 0x20) ? 16 : 8;
            int spr_count = 0;
            int spr_scan[8];
            for (int i = 0; i < 64 && spr_count < 8; i++) {
                int oy = (int)g->oam[i*4] - 1;
                if (scanline >= oy && scanline < oy + spr_h) {
                    spr_scan[spr_count++] = i;
                }
            }
            for (int si = spr_count - 1; si >= 0; si--) {
                int i = spr_scan[si];
                int oy = (int)g->oam[i*4] - 1;
                int ox = g->oam[i*4 + 3];
                uint8_t attr = g->oam[i*4 + 2];
                uint8_t tile_idx = g->oam[i*4 + 1];
                int pal = 0x10 + ((attr & 3) << 2);
                bool flip_h = attr & 0x40;
                bool flip_v = attr & 0x20;
                uint8_t spr_y = scanline - oy;
                if (flip_v) spr_y = (spr_h - 1) - spr_y;
                uint16_t taddr = (uint16_t)((g->ppu_ctrl & 0x08) ? 0x1000 : 0) + tile_idx * 16 + spr_y;
                uint8_t lo = chr_read(g, taddr);
                uint8_t hi = chr_read(g, taddr + 8);
                for (int px = 0; px < 8; px++) {
                    int sx = ox + (flip_h ? (7 - px) : px);
                    if (sx < 0 || sx >= 256) continue;
                    int ci = ((hi & 0x80) ? 2 : 0) | ((lo & 0x80) ? 1 : 0);
                    hi <<= 1; lo <<= 1;
                    if (ci && g->fb[scanline][sx] == g->palette[0])
                        g->fb[scanline][sx] = g->palette[pal + ci];
                }
            }
        }

        dot_bank += 341;
        while (dot_bank >= 3) {
            if (g->cpu.irq_pending && !(g->cpu.flags & FLAG_I)) {
                g->cpu.write(0x100 | g->cpu.sp, (uint8_t)(g->cpu.pc >> 8)); g->cpu.sp--;
                g->cpu.write(0x100 | g->cpu.sp, (uint8_t)g->cpu.pc); g->cpu.sp--;
                g->cpu.write(0x100 | g->cpu.sp, g->cpu.flags & ~FLAG_B); g->cpu.sp--;
                g->cpu.flags |= FLAG_I;
                g->cpu.irq_pending = false;
                uint8_t lo = g->cpu.read(0xFFFE), hi = g->cpu.read(0xFFFF);
                g->cpu.pc = lo | ((uint16_t)hi << 8);
                g->cpu.cycles += 7;
                dot_bank -= 7 * 3;
            } else {
                uint32_t cy = cpu6502_step(&g->cpu);
                if (cy == 0) cy = 2;
                dot_bank -= (int32_t)cy * 3;
            }
        }
    }
}
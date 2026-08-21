/*
 * File: Atari-cart.c
 * Author: dgrubb
 * Date: 07/07/2017
 *
 * Mimics ROM space (that is, a game cartridge).
 * Extended to support bank switching:
 *   F8: 8 KB ROM  -> 2 banks of 4 KB, select via $1FF8/$1FF9
 *   F6: 16 KB ROM -> 4 banks of 4 KB, select via $1FF6..$1FF9
 *   F4: 32 KB ROM -> 8 banks of 4 KB, select via $1FF4..$1FFB
 * 4 KB ROM needs no bank switching.
 */

#include "Atari-cart.h"
#include <stdint.h>

static const uint8_t *cartridge = 0;
static uint32_t cartridge_size = 0;
static uint8_t current_bank = 0;

void cartridge_read(uint16_t address, uint8_t * data)
{
    if (!cartridge) { *data = 0; return; }
    /* address is 0..0xFFF (4K window) */
    uint32_t pos;
    if (cartridge_size <= 0x1000) {
        /* 2K ROM зеркалится в 4K окне, 4K без банков */
        pos = address & (cartridge_size - 1);
    } else {
        pos = (uint32_t)current_bank * 0x1000 + address;
    }
    *data = cartridge[pos];
}

/* Bank select. Адреса маскируются как в x2600/MCUME:
 *   F8 (2 банка):  $0FF8 -> bank 0, $0FF9 -> bank 1
 *   F6 (4 банка):  $0FF6..$0FF9 -> bank 0..3
 *   F4 (8 банков): $0FF4..$0FFB -> bank 0..7
 * Переключение и при чтении, и при записи hotspot (как в x2600). */
void cartridge_bank_select(uint16_t address, uint8_t data)
{
    if (!cartridge) return;
    (void)data;
    uint8_t banks = (uint8_t)(cartridge_size / 0x1000);
    if (banks <= 1) { current_bank = 0; return; }
    address &= 0xFFF;
    uint16_t base;
    if (banks == 2)      base = 0x0FF8;
    else if (banks == 4) base = 0x0FF6;
    else                 base = 0x0FF4;
    if (address >= base && address < base + banks) {
        current_bank = (uint8_t)(address - base);
    }
}

void cartridge_load(const uint8_t *cart)
{
    if (cartridge) {
        cartridge_eject();
    }
    cartridge = cart;
    cartridge_size = 0x1000;
    current_bank = 0;
}

void cartridge_load_size(const uint8_t *cart, uint32_t size)
{
    cartridge_load(cart);
    cartridge_size = size;
    /* Стартовый банк — последний (как в x2600/MCUME: theRom = &theCart[size-4096]).
     * Ресет-вектор лежит в последнем банке ($FFFC), поэтому CPU стартует оттуда. */
    uint8_t banks = (uint8_t)(size / 0x1000);
    current_bank = (banks > 0) ? (uint8_t)(banks - 1) : 0;
}

void cartridge_eject(void)
{
    cartridge = 0;
    cartridge_size = 0;
    current_bank = 0;
}

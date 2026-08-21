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
    if (cartridge) {
        /* address is 0..0xFFF (4K window) */
        *data = cartridge[(uint32_t)current_bank * 0x1000 + address];
    }
}

/* Bank select is triggered by writes to the top of the 4K window.
 * The base hotspot depends on ROM size:
 *   8 KB (2 banks):  base $1FF8 -> bank 0..1
 *  16 KB (4 banks):  base $1FF6 -> bank 0..3
 *  32 KB (8 banks):  base $1FF4 -> bank 0..7
 */
void cartridge_bank_select(uint16_t address, uint8_t data)
{
    if (!cartridge) return;
    (void)data;
    uint8_t banks = (uint8_t)(cartridge_size / 0x1000);
    if (banks <= 1) { current_bank = 0; return; }
    /* База hotspot зависит от количества банков:
     * 2 банка (F8)  -> $1FF8..$1FF9
     * 4 банка (F6)  -> $1FF6..$1FF9
     * 8 банков (F4) -> $1FF4..$1FFB */
    uint16_t base;
    if (banks == 2)      base = 0x1FF8;
    else if (banks == 4) base = 0x1FF6;
    else                 base = 0x1FF4;
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
    cartridge_size = 0x1000; /* placeholder; real size set via cartridge_load_size */
    current_bank = 0;
}

void cartridge_load_size(const uint8_t *cart, uint32_t size)
{
    cartridge_load(cart);
    cartridge_size = size;
}

void cartridge_eject(void)
{
    cartridge = 0;
    cartridge_size = 0;
    current_bank = 0;
}

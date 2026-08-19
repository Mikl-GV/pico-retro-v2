#include "sd_card.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

/*
 * Драйвер microSD в режиме SPI (bit-bang).
 *
 * Почему bit-bang: на RP2040 аппаратные SPI закреплены за пинами
 *   SPI0: SCK=GP18, TX=GP19, RX=GP16, CS=GP17
 *   SPI1: SCK=GP10, TX=GP11, RX=GP12, CS=GP13
 * В этой конфигурации GP10..GP13 заняты дисплеем (RD/CS/RST/BL),
 * а GP16..GP19 — звуком и джойстиком. Поэтому SD висит на свободных
 * GPIO (24/26/27/28) и тактуется программно. Скорость ~1-4 МГц — достаточно
 * для загрузки ROM. TODO: заменить на PIO SPI (pio) для полной скорости.
 */

#define SD_CMD0    0
#define SD_CMD8    8
#define SD_CMD17   17
#define SD_CMD24   24
#define SD_CMD55   55
#define SD_CMD58   58
#define SD_ACMD41  41

#define SD_DATA_TOKEN 0xFE
#define SD_DATA_ACCEPT 0x05

static inline void sd_cs_low(void)  { gpio_put(SD_CS_PIN, 0); }
static inline void sd_cs_high(void) { gpio_put(SD_CS_PIN, 1); }

static inline void sd_delay(void) { busy_wait_us_32(1); }

static uint8_t sd_xfer(uint8_t out) {
    uint8_t in = 0;
    for (int i = 7; i >= 0; i--) {
        gpio_put(SD_MOSI_PIN, (out >> i) & 1);
        gpio_put(SD_SCK_PIN, 1);
        sd_delay();
        in = (uint8_t)((in << 1) | (gpio_get(SD_MISO_PIN) ? 1 : 0));
        gpio_put(SD_SCK_PIN, 0);
        sd_delay();
    }
    return in;
}

/* Отправляет команду, возвращает R1. CS остаётся низким после вызова. */
static int sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    sd_cs_low();
    sd_xfer(0xFF);
    sd_xfer((uint8_t)(0x40 | cmd));
    sd_xfer((uint8_t)(arg >> 24));
    sd_xfer((uint8_t)(arg >> 16));
    sd_xfer((uint8_t)(arg >> 8));
    sd_xfer((uint8_t)(arg));
    sd_xfer(crc);
    for (int i = 0; i < 200; i++) {
        uint8_t r1 = sd_xfer(0xFF);
        if (!(r1 & 0x80)) return r1;
    }
    return -1;
}

bool sd_init(void) {
    gpio_init(SD_CS_PIN);
    gpio_set_dir(SD_CS_PIN, GPIO_OUT);
    gpio_put(SD_CS_PIN, 1);

    gpio_init(SD_SCK_PIN);
    gpio_set_dir(SD_SCK_PIN, GPIO_OUT);
    gpio_put(SD_SCK_PIN, 0);

    gpio_init(SD_MOSI_PIN);
    gpio_set_dir(SD_MOSI_PIN, GPIO_OUT);
    gpio_put(SD_MOSI_PIN, 1);

    gpio_init(SD_MISO_PIN);
    gpio_set_dir(SD_MISO_PIN, GPIO_IN);
    gpio_pull_up(SD_MISO_PIN);

    /* 80 тактов с CS=1 для перехода карты в SPI-режим */
    sd_cs_high();
    for (int i = 0; i < 10; i++) sd_xfer(0xFF);

    if (sd_cmd(SD_CMD0, 0, 0x95) != 0x01) return false;
    sd_cs_high();
    sd_xfer(0xFF);

    int r = sd_cmd(SD_CMD8, 0x1AA, 0x87);
    bool is_v2 = (r == 0x01);
    if (is_v2) {
        for (int i = 0; i < 4; i++) sd_xfer(0xFF); /* R7 */
    }
    sd_cs_high();
    sd_xfer(0xFF);

    /* ACMD41: CMD55 + ACMD41, CS держим низким */
    for (int i = 0; i < 500; i++) {
        sd_cmd(SD_CMD55, 0, 0x01);
        r = sd_cmd(SD_ACMD41, is_v2 ? 0x40000000 : 0, 0x01);
        sd_cs_high();
        sd_xfer(0xFF);
        if (r == 0) break;
        sleep_ms(10);
    }
    if (r != 0) return false;

    return true;
}

bool sd_read_block(uint32_t block, uint8_t *buf) {
    if (sd_cmd(SD_CMD17, block, 0x01) != 0x00) {
        sd_cs_high();
        return false;
    }

    int token = -1;
    for (int i = 0; i < 5000; i++) {
        int b = sd_xfer(0xFF);
        if (b == SD_DATA_TOKEN) { token = b; break; }
        if (b != 0xFF) break;
    }
    if (token != SD_DATA_TOKEN) {
        sd_cs_high();
        return false;
    }

    for (int i = 0; i < SD_BLOCK_SIZE; i++) buf[i] = sd_xfer(0xFF);
    sd_xfer(0xFF);
    sd_xfer(0xFF);
    sd_cs_high();
    sd_xfer(0xFF);
    return true;
}

bool sd_write_block(uint32_t block, const uint8_t *buf) {
    if (sd_cmd(SD_CMD24, block, 0x01) != 0x00) {
        sd_cs_high();
        return false;
    }

    sd_xfer(SD_DATA_TOKEN);
    for (int i = 0; i < SD_BLOCK_SIZE; i++) sd_xfer(buf[i]);
    sd_xfer(0xFF);
    sd_xfer(0xFF);

    uint8_t resp = sd_xfer(0xFF);
    if ((resp & 0x1F) != SD_DATA_ACCEPT) {
        sd_cs_high();
        return false;
    }

    for (int i = 0; i < 5000; i++) {
        if (sd_xfer(0xFF) != 0x00) break;
    }
    sd_cs_high();
    sd_xfer(0xFF);
    return true;
}

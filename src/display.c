#include "display.h"
#include "config.h"
#include "font8x8.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <string.h>

#define ILI9341_NOP      0x00
#define ILI9341_SWRESET  0x01
#define ILI9341_SLPOUT   0x11
#define ILI9341_DISPOFF  0x28
#define ILI9341_DISPON   0x29
#define ILI9341_CASET    0x2A
#define ILI9341_PASET    0x2B
#define ILI9341_RAMWR    0x2C
#define ILI9341_MADCTL   0x36
#define ILI9341_PIXFMT   0x3A
#define ILI9341_PWCTR1   0xC0
#define ILI9341_PWCTR2   0xC1
#define ILI9341_VMCTR1   0xC5
#define ILI9341_VMCTR2   0xC7
#define ILI9341_PWCTRA   0xCB
#define ILI9341_PWCTRB   0xCF
#define ILI9341_DTCA     0xE8
#define ILI9341_DTCB     0xEA
#define ILI9341_POWER_SEQ 0xED
#define ILI9341_FRMCTR1  0xB1
#define ILI9341_DFUNCTR  0xB6
#define ILI9341_PRC      0xF7
#define ILI9341_3GAMMA_EN 0xF2
#define ILI9341_GAMMASET 0x26
#define ILI9341_GMCTRP1  0xE0
#define ILI9341_GMCTRP2  0xE1

static uint8_t framebuffer[LCD_WIDTH * LCD_HEIGHT * 2];

static const uint8_t gamma_pos[15] = {
    0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
    0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00
};
static const uint8_t gamma_neg[15] = {
    0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
    0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
};

static inline void lcd_bus_write(uint8_t b) {
    gpio_put_masked(LCD_DATA_MASK, (uint32_t)b << LCD_DATA_BASE);
}

static inline void lcd_wr_strobe(void) {
    gpio_put(LCD_WR_PIN, 0);
    asm volatile("nop\nnop\nnop\nnop\n");
    gpio_put(LCD_WR_PIN, 1);
}

static void lcd_write_cmd(uint8_t cmd) {
    gpio_put(LCD_RS_PIN, 0);
    lcd_bus_write(cmd);
    lcd_wr_strobe();
}

static void lcd_write_data(uint8_t d) {
    gpio_put(LCD_RS_PIN, 1);
    lcd_bus_write(d);
    lcd_wr_strobe();
}

static void lcd_write_data16(uint16_t d) {
    lcd_write_data((uint8_t)(d >> 8));
    lcd_write_data((uint8_t)(d & 0xFF));
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_write_cmd(ILI9341_CASET);
    lcd_write_data16(x0);
    lcd_write_data16(x1);
    lcd_write_cmd(ILI9341_PASET);
    lcd_write_data16(y0);
    lcd_write_data16(y1);
}

static void lcd_hard_reset(void) {
    gpio_put(LCD_RST_PIN, 0);
    sleep_ms(10);
    gpio_put(LCD_RST_PIN, 1);
    sleep_ms(120);
}

static void lcd_init_sequence(void) {
    lcd_write_cmd(ILI9341_SWRESET);
    sleep_ms(150);
    lcd_write_cmd(ILI9341_DISPOFF);

    lcd_write_cmd(ILI9341_PWCTRB);
    lcd_write_data(0x00); lcd_write_data(0xC1); lcd_write_data(0x30);

    lcd_write_cmd(ILI9341_POWER_SEQ);
    lcd_write_data(0x64); lcd_write_data(0x03);
    lcd_write_data(0x12); lcd_write_data(0x81);

    lcd_write_cmd(ILI9341_DTCA);
    lcd_write_data(0x85); lcd_write_data(0x00); lcd_write_data(0x78);

    lcd_write_cmd(ILI9341_PWCTRA);
    lcd_write_data(0x39); lcd_write_data(0x2C); lcd_write_data(0x00);
    lcd_write_data(0x34); lcd_write_data(0x02);

    lcd_write_cmd(ILI9341_PRC);
    lcd_write_data(0x20);

    lcd_write_cmd(ILI9341_DTCB);
    lcd_write_data(0x00); lcd_write_data(0x00);

    lcd_write_cmd(ILI9341_PWCTR1);
    lcd_write_data(0x23);

    lcd_write_cmd(ILI9341_PWCTR2);
    lcd_write_data(0x10);

    lcd_write_cmd(ILI9341_VMCTR1);
    lcd_write_data(0x3E); lcd_write_data(0x28);

    lcd_write_cmd(ILI9341_VMCTR2);
    lcd_write_data(0x86);

    lcd_write_cmd(ILI9341_MADCTL);
    lcd_write_data(LCD_MADCTL);

    lcd_write_cmd(ILI9341_PIXFMT);
    lcd_write_data(0x55);

    lcd_write_cmd(ILI9341_FRMCTR1);
    lcd_write_data(0x00); lcd_write_data(0x18);

    lcd_write_cmd(ILI9341_DFUNCTR);
    lcd_write_data(0x08); lcd_write_data(0x82); lcd_write_data(0x27);

    lcd_write_cmd(ILI9341_3GAMMA_EN);
    lcd_write_data(0x00);

    lcd_write_cmd(ILI9341_GAMMASET);
    lcd_write_data(0x01);

    lcd_write_cmd(ILI9341_GMCTRP1);
    for (int i = 0; i < 15; i++) lcd_write_data(gamma_pos[i]);

    lcd_write_cmd(ILI9341_GMCTRP2);
    for (int i = 0; i < 15; i++) lcd_write_data(gamma_neg[i]);

    lcd_write_cmd(ILI9341_SLPOUT);
    sleep_ms(120);

    lcd_write_cmd(ILI9341_DISPON);
    sleep_ms(25);
}

void display_init(void) {
    for (int i = 0; i < 8; i++) {
        gpio_init(LCD_DATA_BASE + i);
        gpio_set_dir(LCD_DATA_BASE + i, GPIO_OUT);
    }
    gpio_init(LCD_RS_PIN); gpio_set_dir(LCD_RS_PIN, GPIO_OUT);
    gpio_init(LCD_WR_PIN); gpio_set_dir(LCD_WR_PIN, GPIO_OUT);
    gpio_init(LCD_RD_PIN); gpio_set_dir(LCD_RD_PIN, GPIO_OUT);
    gpio_init(LCD_RST_PIN); gpio_set_dir(LCD_RST_PIN, GPIO_OUT);

    gpio_put(LCD_WR_PIN, 1);
    gpio_put(LCD_RD_PIN, 1);
    gpio_put(LCD_RS_PIN, 0);

    gpio_set_function(LCD_BL_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LCD_BL_PIN);
    pwm_config c = pwm_get_default_config();
    pwm_config_set_wrap(&c, 255);
    pwm_init(slice, &c, true);
    pwm_set_gpio_level(LCD_BL_PIN, 255);

    lcd_hard_reset();
    lcd_init_sequence();
}

void display_set_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) return;
    uint32_t i = ((uint32_t)y * LCD_WIDTH + x) * 2;
    framebuffer[i]     = (uint8_t)(color >> 8);
    framebuffer[i + 1] = (uint8_t)(color & 0xFF);
}

void display_blit(const uint8_t *src, const uint16_t *lut, int x, int y, int w, int h) {
    for (int yy = 0; yy < h; yy++) {
        uint32_t base = ((uint32_t)(y + yy) * LCD_WIDTH + x) * 2;
        const uint8_t *row = src + yy * 256;
        for (int xx = 0; xx < w; xx++) {
            uint16_t c = lut[row[xx] & 0x3F];
            framebuffer[base + xx * 2]     = (uint8_t)(c >> 8);
            framebuffer[base + xx * 2 + 1] = (uint8_t)(c & 0xFF);
        }
    }
}

/* Прямая запись пикселей в GRAM дисплея (8080), минуя локальный framebuffer.
 * Экономит 153 КБ RAM и ускоряет вывод. */
void display_stream_begin(int x, int y, int w, int h) {
    lcd_set_window(x, y, x + w - 1, y + h - 1);
    lcd_write_cmd(ILI9341_RAMWR);
    gpio_put(LCD_RS_PIN, 1);
}

void display_stream_pixels(const uint8_t *src, const uint16_t *lut, int w, int h) {
    for (int yy = 0; yy < h; yy++) {
        const uint8_t *row = src + yy * 256;
        for (int xx = 0; xx < w; xx++) {
            uint16_t c = lut[row[xx] & 0x3F];
            lcd_bus_write((uint8_t)(c >> 8));
            lcd_wr_strobe();
            lcd_bus_write((uint8_t)(c & 0xFF));
            lcd_wr_strobe();
        }
    }
}

void display_stream_end(void) {
}

void display_fill_rect(int x, int y, int w, int h, uint16_t color) {
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);
    for (int yy = y; yy < y + h; yy++) {
        uint32_t base = ((uint32_t)yy * LCD_WIDTH + x) * 2;
        for (int xx = 0; xx < w; xx++) {
            framebuffer[base + xx * 2]     = hi;
            framebuffer[base + xx * 2 + 1] = lo;
        }
    }
}

void display_fill(uint16_t color) {
    display_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void display_flush(void) {
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    lcd_write_cmd(ILI9341_RAMWR);
    gpio_put(LCD_RS_PIN, 1);
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT * 2; i++) {
        lcd_bus_write(framebuffer[i]);
        lcd_wr_strobe();
    }
}

uint32_t display_read_reg(uint8_t reg, int len) {
    uint32_t val = 0;
    lcd_write_cmd(reg);
    gpio_put(LCD_RS_PIN, 1);
    for (int i = 0; i < 8; i++) gpio_set_dir(LCD_DATA_BASE + i, GPIO_IN);
    busy_wait_us_32(10);
    gpio_put(LCD_RD_PIN, 0);
    busy_wait_us_32(2);
    gpio_put(LCD_RD_PIN, 1);
    busy_wait_us_32(1);
    for (int i = 0; i < len && i < 4; i++) {
        gpio_put(LCD_RD_PIN, 0);
        busy_wait_us_32(2);
        val = (val << 8) | ((gpio_get_all()&LCD_DATA_MASK)>>LCD_DATA_BASE);
        gpio_put(LCD_RD_PIN, 1);
        busy_wait_us_32(1);
    }
    for (int i = 0; i < 8; i++) gpio_set_dir(LCD_DATA_BASE + i, GPIO_OUT);
    gpio_put(LCD_RS_PIN, 0);
    return val;
}

/* Символ 8x8 в координаты (x,y), цвет текста + фона.
 * Координаты — в единицах символов (шаг 8 пикселей). */
void display_text(const char *s, int x, int y, int scale, uint16_t color, uint16_t bg) {
    int stride = 8 * scale;
    while (*s) {
        uint8_t c = (uint8_t)*s;
        if (c < 32 || c > 126) c = '?';
        const uint8_t *g = font8x8[c - 32];
        int px = x * stride, py = y * stride;
        for (int row = 0; row < 8; row++) {
            uint8_t bits = g[row];
            for (int col = 7; col >= 0; col--) {
                uint16_t clr = (bits & 0x80) ? color : bg;
                bits <<= 1;
                int bx = px + col * scale;
                int by = py + row * scale;
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        display_set_pixel(bx + sx, by + sy, clr);
                    }
                }
            }
        }
        s++;
        x++;
    }
}

void display_text_center(const char *s, int y, int scale, uint16_t color, uint16_t bg) {
    int stride = 8 * scale;
    display_text(s, (LCD_WIDTH / stride - (int)strlen(s)) / 2, y, scale, color, bg);
}

void display_set_madctl(uint8_t v) {
    lcd_write_cmd(ILI9341_MADCTL);
    lcd_write_data(v);
}
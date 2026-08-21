#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define RGB565(r, g, b) ((((r) & 0x1F) << 11) | (((g) & 0x3F) << 5) | ((b) & 0x1F))

void display_init(void);
void display_fill(uint16_t color);
void display_fill_rect(int x, int y, int w, int h, uint16_t color);
void display_set_pixel(int x, int y, uint16_t color);
void display_blit(const uint8_t *src, const uint16_t *lut, int x, int y, int w, int h);
void display_stream_begin(int x, int y, int w, int h);
void display_stream_pixels(const uint8_t *src, const uint16_t *lut, int w, int h);
void display_stream_pixels_full(const uint8_t *src, const uint16_t *lut, int w, int h);
void display_stream_pixel16(uint16_t c);
void display_stream_end(void);
void display_flush(void);
uint32_t display_read_reg(uint8_t reg, int len);

void display_text(const char *s, int x, int y, int scale, uint16_t color, uint16_t bg);
void display_text_at(const char *s, int px, int py, int scale, uint16_t color, uint16_t bg);
void display_text_at_nobg(const char *s, int px, int py, int scale, uint16_t color);
void display_text_center_nobg(const char *s, int y, int scale, uint16_t color);
void display_text_center(const char *s, int y, int scale, uint16_t color, uint16_t bg);
void display_set_madctl(uint8_t v);

#endif

#include "video.h"
#include "config.h"

#ifdef VIDEO_SCANVIDEO

#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"

static void __not_in_flash_func(render_loop)(void) {
    while (true) {
        scanvideo_scanline_buffer_t *dest =
            scanvideo_begin_scanline_generation(true);

        uint32_t *buf = dest->data;
        uint32_t words = dest->data_max;

        /* TODO: рендер из framebuffer (display.h), а не тестовая картинка.
         * Каждое 32-битное слово = два 16-битных пикселя RGB565. */
        for (uint32_t w = 0; w < words; w++) {
            uint16_t c = ((w >> 5) & 1) ? 0xF800 : 0x07E0;
            buf[w] = ((uint32_t)c << 16) | c;
        }
        dest->data_used = words;

        scanvideo_end_scanline_generation(dest);
    }
}

void video_init(void) {
    scanvideo_setup(&vga_mode_320x240_60);
    scanvideo_timing_enable(true);
    multicore_launch_core1(render_loop);
}

#else

void video_init(void) {
}

#endif

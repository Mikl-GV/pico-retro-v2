#ifndef CONFIG_H
#define CONFIG_H

/* ============ Дисплей WF28ETLAJDNN0 (ILI9341V, 8-бит 8080) ============
 * ВНИМАНИЕ: в даташите ошибка. Для 8-битного режима (IM0 = 0) шина данных —
 * это МЛАДШИЙ байт DB0-DB7 (контакты FPC 23..30), а НЕ DB8-DB15.
 * D0..D7 идут на ПОДРЯД идущие GPIO, начиная с LCD_DATA_BASE (для PIO).
 *
 * /RD и /CS НЕ занимают GPIO: /RD жёстко на 3V3 (чтение не используется),
 * /CS жёстко на GND (одно устройство на шине). Освобождает 1 пин. */
#define LCD_RD_PIN      12      /* держать HIGH — чтение не используется */
#define LCD_DATA_BASE   0
#define LCD_DATA_MASK   (((1u << 8) - 1) << LCD_DATA_BASE)

#define LCD_RS_PIN      8
#define LCD_WR_PIN      9
#define LCD_RST_PIN     10
#define LCD_BL_PIN      11      /* PWM подсветки, через транзистор (80 мА) */

#define LCD_WIDTH       320
#define LCD_HEIGHT      240
#define LCD_MADCTL      0x28

/* Делитель такта PIO для записи (такт байта ~2*clkdiv/sysclk = ~96 нс при 125 МГц) */
#define LCD_PIO_CLKDIV  6.0f

/* ============ Композитный видеовыход (pico-extras / pico_scanvideo) ============ */
#define VIDEO_PIN       13

/* ============ Звук (PWM) ============ */
#define AUDIO_L_PIN     14
#define AUDIO_R_PIN     15
#define AUDIO_PWM_WRAP  255

/* ============ Кнопки (напрямую, активны LOW, с внутренней подтяжкой) ============ */
#define JOY_UP_PIN      18
#define JOY_DOWN_PIN    19
#define JOY_LEFT_PIN    20
#define JOY_RIGHT_PIN   21
#define JOY_AB_PIN      22
#define JOY_START_PIN   26
#define JOY_RESET_PIN   27

/* ============ microSD (SPI, bit-bang) ============
 * Аппаратные SPI заняты: SPI0 (GP16..19) — джойстик/звук,
 * SPI1 (GP10..13) — дисплей. SD висит на свободных GPIO и тактуется
 * программно (см. sd_card.c). TODO: PIO SPI для полной скорости. */
#define SD_CS_PIN       15
#define SD_SCK_PIN      26
#define SD_MOSI_PIN     27
#define SD_MISO_PIN     28

/* Заняты все доступные GPIO: GP0..22 и GP26..28.
 * GP23 (SMPS), GP24 (VBUS detect), GP25 (LED) на разъём Pico НЕ выведены. */

#endif

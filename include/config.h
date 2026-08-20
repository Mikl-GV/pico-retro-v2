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

#define LCD_WIDTH       320
#define LCD_HEIGHT      240
#define LCD_MADCTL      0x28

/* ============ Кнопки (напрямую, активны LOW, с внутренней подтяжкой) ============ */
#define JOY_UP_PIN      18
#define JOY_DOWN_PIN    19
#define JOY_LEFT_PIN    20
#define JOY_RIGHT_PIN   21
#define JOY_A_PIN       22
#define JOY_SELECT_PIN  15
#define JOY_START_PIN   26
#define JOY_B_PIN       27

/* ============ Аудио (PWM, RC-фильтр) ============ */
#define AUDIO_L_PIN     13
#define AUDIO_R_PIN     14
#define AUDIO_PWM_WRAP  255

/* Заняты: GP0..9 (дисплей), GP10 (RST), GP11 (3V3), GP12 (3V3).
 * GP13/14 (аудио), GP15 (Select), GP16, GP17, GP18..22, GP26..27 (кнопки), GP28 свободен.
 * GP23 (SMPS), GP24 (VBUS), GP25 (LED) не выведены на разъём. */

#define JOY_UP_BIT      0x01
#define JOY_DOWN_BIT    0x02
#define JOY_LEFT_BIT    0x04
#define JOY_RIGHT_BIT   0x08
#define JOY_A_BIT       0x10
#define JOY_START_BIT   0x20
#define JOY_B_BIT       0x40

#endif

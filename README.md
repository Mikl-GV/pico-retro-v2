# pico-retro

Ретроконсоль на Raspberry Pi Pico (RP2040):

- Дисплей **WF28ETLAJDNN0** — 2.8", 240×320, контроллер ILI9341V, **8-бит 8080, bit-bang**
- Управление — **7 кнопок** напрямую на GPIO (активны LOW, внутренняя pull-up)

## Статус модулей

| Модуль | Статус |
|---|---|
| Дисплей ILI9341V (8-бит 8080) | Готов: команды bit-bang, пиксели bit-bang |
| Кнопки (7 шт.) | Готов |

## Важное про дисплей

**В даташите ошибка.** Для 8-битного режима (`IM0 = 0`) шина данных — это
**младший байт `DB0-DB7` (контакты FPC 23..30)**, а не `DB8-DB15`, как написано
в таблице. Подключаем именно `DB0-DB7`.

## Схема подключения

### Дисплей (FPC 40 pin)

| Сигнал | Pico GPIO | Контакт FPC |
|---|---|---|
| D0..D7 | 0..7 | 23..30 |
| RS | 8 | 8 |
| /WR | 9 | 9 |
| /RD | 3V3 (жёстко) | 10 |
| /CS | GND (жёстко) | 7 |
| /RESET | 10 | 31 |
| LEDA (подсветка) | 11 (PWM, через транзистор) | 16 |
| IM0 | → GND (8-бит) | 21 |
| VCI, IOVCC | 3V3 | 6, 32, 33 |
| GND | GND | 5, 11, 34 |
| LEDK1..4 | GND | 17..20 |

### Кнопки (7 шт., напрямую, активны LOW)

| Кнопка | Pico GPIO |
|---|---|
| Up | 18 |
| Down | 19 |
| Left | 20 |
| Right | 21 |
| A | 22 |
| Start | 26 |
| B | 27 |

Подтяжка: внутренняя pull-up Pico. GND — общий для всех кнопок.

GP23 (SMPS), GP24 (VBUS detect), GP25 (LED) на разъём Pico не выведены.

## Сборка

```bash
git clone https://github.com/raspberrypi/pico-sdk.git
git submodule add https://github.com/raspberrypi/pico-extras.git lib/pico-extras  # опционально (видео)
export PICO_SDK_PATH=$PWD/pico-sdk
export PICO_EXTRAS_PATH=$PWD/lib/pico-extras

cd pico-retro
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=$PICO_SDK_PATH -DPICO_EXTRAS_PATH=$PICO_EXTRAS_PATH
make -j4
```

Прошивка: `picotool load pico_retro.uf2` или скопировать `pico_retro.uf2` в BOOTSEL-режиме.

## Структура

```
pico-retro/
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── ili9341_8bit.pio     # PIO-программа 8-бит 8080 (WR side-set)
├── BOM.md               # список компонентов
├── include/
│   ├── config.h         # карта пинов, размеры, MADCTL
│   ├── display.h
│   ├── joypad.h
│   ├── audio.h
│   ├── video.h
│   └── sd_card.h
├── src/                 # реализации + main.c
└── lib/                 # внешние submodules (pico-extras)
```

## Дорожная карта эмулятора

1. Композитное видео: рендер framebuffer в scanline-буфер (уже каркас).
2. Микшер звука + DMA PWM (для эмуляции SN76489/YM2612).
3. microSD: FatFs поверх драйвера блоков → загрузка ROM.
4. Ядро эмуляции (по архитектуре на выбор).

## Замечание по памяти

RP2040 имеет 264 КБ RAM. Полный framebuffer 240×320×16 бит = 150 КБ.
При добавлении композит-видео и звуковых буферов придётся ужать буфер экрана
(8-бит палитра, чанкинг строк) или перейти на RP2350.

# pico-retro

Ретроконсоль на Raspberry Pi Pico (RP2040):

- Дисплей **WF28ETLAJDNN0** — 2.8", 240×320, контроллер ILI9341V, **8-бит 8080, bit-bang**
- Управление — **8 кнопок** напрямую на GPIO (A/B/Select/Start/Up/Down/Left/Right)
- Эмуляция **NES** — CPU 6502 (все 256 опкодов), PPU (тайлы/спрайты/скролл), Mapper 0/2/4 (MMC3)
- UART отладка на GP16/17 (115200 бод)
- CPU: 250 МГц (set_sys_clock_khz), дисплейный bit-bang через NOP с сохранением таймингов

## Статус модулей

| Модуль | Статус |
|---|---|
| Дисплей ILI9341V (8-бит 8080) | Готов: команды bit-bang, пиксели bit-bang |
| Кнопки (8 шт., антидребезг, NES-протокол) | Готов |
| CPU 6502 (полный) | Готов |
| PPU (тайлы, спрайты, скролл, NMI, t→v) | Готов |
| Mapper 0 (NROM) | Готов |
| Mapper 2 (UxROM) | Готов |
| Mapper 4 (MMC3, CHR/PRG банкинг, IRQ) | Готов |
| APU (звук) | Не реализован (заглушка) |
| microSD + FatFs | Не реализованы |

## Работающие игры

- Balloon Fight (mapper 0)
- Battle City (mapper 0)
- Bomberman (mapper 0)
- Duck Tales (mapper 0)
- DuckTales 2 (mapper 2)
- Saiyuuki World (mapper 2)
- SMB — не работает (серый экран, баг CPU/PPU тайминга)

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

### Кнопки (8 шт., напрямую, активны LOW)

| Кнопка | Pico GPIO | NES бит |
|---|---|---|
| A | 22 | 0 |
| B | 27 | 1 |
| Select | 15 | 2 |
| Start | 26 | 3 |
| Up | 18 | 4 |
| Down | 19 | 5 |
| Left | 20 | 6 |
| Right | 21 | 7 |

Подтяжка: внутренняя pull-up Pico. GND — общий для всех кнопок.

### UART (отладка)

| Сигнал | Pico GPIO |
|---|---|
| TX | 16 |
| RX | 17 |

115200 бод, 8N1.

GP23 (SMPS), GP24 (VBUS detect), GP25 (LED) на разъём Pico не выведены.

## Сборка

```bash
git clone https://github.com/Mikl-GV/pico-retro.git
cd pico-retro
mkdir build && cd build
cmake .. -G Ninja
ninja
```

Требования: Pico SDK 1.5.1+, cmake, ninja, arm-none-eabi-gcc.

Прошивка: скопировать `build/pico_retro.uf2` в BOOTSEL-режиме.

## Структура

```
pico-retro/
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── ili9341_8bit.pio     # PIO-программа 8-бит 8080 (не используется)
├── BOM.md               # список компонентов
├── embed_rom.ps1        # скрипт вшивки ROM в C-заголовки
├── include/
│   ├── config.h         # карта пинов, размеры, MADCTL
│   ├── display.h / joypad.h / nes.h / cpu6502.h / audio.h / video.h / sd_card.h
│   └── rom_*.h          # вшитые образы ROM
├── src/
│   ├── main.c           # меню + игровой цикл
│   ├── display.c        # дисплей 8-bit 8080 bit-bang
│   ├── joypad.c         # кнопки + антидребезг + NES-формат
│   ├── cpu6502.c        # процессор 6502 (полный)
│   └── nes.c            # PPU + мапперы + рендер
└── lib/
```

## Замечание по памяти

RP2040 имеет 264 КБ RAM. `framebuffer[320×240×2]` = 150 КБ (дисплей) +
`fb[240×256]` = 60 КБ (NES). Остаётся ~54 КБ для остального. При добавлении
APU/SD придётся ужать буфер экрана или перейти на RP2350.
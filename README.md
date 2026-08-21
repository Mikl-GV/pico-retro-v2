# pico-retro v2

Ретроконсоль на Raspberry Pi Pico (RP2040) — **мультисистемная**: NES и Atari 2600.

## Возможности

- Дисплей **WF28ETLAJDNN0** — 2.8", 240×320, контроллер ILI9341V, **8-бит 8080, bit-bang**
- Управление — **8 кнопок** напрямую на GPIO (A/B/Select/Start/Up/Down/Left/Right)
- **Двухуровневое меню**: выбор системы → выбор игры
- **NES** (InfoNES): 138 мапперов, полный 6502 + PPU
- **Atari 2600**: ядро x2600 (mos6507 + TIA + mos6532), банковское переключение F8/F6/F4
- Меню в стиле мультикартриджей «9999-in-1»:
  - фиолетовый фон со звёздами
  - оранжевые градиентные боковые панели
  - киноплёночные бордюры: кадры с миниатюрами, сплошная перфорация
  - логотип «PICO-RETRO», подсказка внизу
- CPU: 250 МГц (vreg 1.20 В + `set_sys_clock_khz`)

## Статус модулей

| Модуль | Статус |
|---|---|
| Дисплей ILI9341V (8-бит 8080 bit-bang, streaming) | Готов |
| Кнопки (8 шт., NES-протокол, полярность) | Готов |
| InfoNES (CPU 6502 + PPU + мапперы ×138) | Готов |
| APU (звук) | Заглушка; аудио моно на GP13 (PWM) |
| microSD + FatFs | Не реализованы |
| Джойстик Sega Mega Drive 2 (DB9) | Не подключён (код удалён) |

## Игры NES (11)

| Игра | Маппер |
|---|---|
| Balloon Fight | 0 (NROM) |
| Battle City | 0 (NROM) |
| Bomberman | 0 (NROM) |
| Contra (J) | 23 (VRC2) |
| Duck Tales | 0 (NROM) |
| Duck Tales II | 2 (UxROM) |
| Fant. Adv. Dizzy | 71 (Camerica) |
| Little Nemo | 4 (MMC3) |
| Mario Bros. | 0 (NROM) |
| Saiyuuki World | 2 (UxROM) |
| SMB | 4 (MMC3) |

## Игры Atari 2600 (6)

| Игра | Размер | Маппер |
|---|---|---|
| Halo 2600 | 4 КБ | 4K (без банков) |
| Thrust | 16 КБ | F6 |
| DK Arcade | 32 КБ | F4 |
| Air-Sea Battle | 2 КБ | 2K (без банков) |
| Asteroids | 8 КБ | F8 |
| California Games | 16 КБ | F6 |

## Меню

```
PICO-RETRO
  > NES           <- выбор системы
    Atari 2600
    Settings
    About
        ↓ Start
  > Halo 2600     <- выбор игры
    Thrust
    ...
```

Выход из игры — удержание Start (~2 сек).

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
| LEDA (подсветка) | 3V3 (жёстко, не управляется) | 16 |
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

### Аудио (моно)

| Сигнал | Pico GPIO |
|---|---|
| Аудио PWM | 13 (RC-фильтр → джек/усилитель) |

### Свободные выводы

**GP11, GP14, GP28** — свободны (можно использовать для SD, I2S и т.п.).

GP23 (SMPS), GP24 (VBUS detect), GP25 (LED) на разъём Pico не выведены.

## Сборка

```bash
git clone https://github.com/Mikl-GV/pico-retro.git
cd pico-retro
mkdir build && cd build
cmake .. -G Ninja
ninja
```

Требования: Pico SDK, cmake, ninja, arm-none-eabi-gcc.

**Windows:** GNU `arm-none-eabi-ld` не открывает .ld-скрипты при кириллице в пути.
Собирайте через ASCII-путь `C:\pico-work\pico-retro` (junction на репозиторий):

```powershell
cd C:\pico-work\pico-retro
$env:PICO_SDK_PATH = "C:\pico-sdk"
.\build.ps1
```

Прошивка: скопировать `build/pico_retro.uf2` в BOOTSEL-режиме.

## Структура

```
pico-retro/
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── build.ps1            # сборка под Windows (ASCII-путь)
├── embed_rom.ps1        # конвертация .nes → C-заголовок
├── BOM.md               # список компонентов
├── CHANGES.md           # журнал изменений
├── include/
│   ├── config.h         # карта пинов, размеры
│   ├── display.h / joypad.h
│   ├── font8x8.h        # пиксельный шрифт 8×8
│   ├── thumbs.h         # миниатюры игр (25×25, NES-палитра)
│   └── rom_*.h          # вшитые образы ROM
├── src/
│   ├── main.cpp         # меню (системы → игры) + запуск
│   ├── display.c        # ILI9341 8-bit 8080 bit-bang + шрифт
│   └── joypad.c         # 8 кнопок, NES-формат
├── infones/             # ядро InfoNES (NES)
│   ├── InfoNES.cpp / .h
│   ├── K6502.cpp / .h
│   ├── InfoNES_Mapper.cpp + mapper/*.cpp  # 138 мапперов
│   ├── InfoNES_pAPU.cpp
│   └── system_pico_retro.cpp  # системный слой: экран, кнопки, бордюры
├── atari2600/           # ядро Atari 2600 (x2600)
│   ├── atari/           # TIA, картриджи, memmap
│   ├── mos6507/         # CPU 6507
│   ├── mos6532/         # RIOT
│   ├── rom_*.h          # вшитые ROM игр 2600
│   └── system_atari.cpp # системный слой Atari
└── lib/
```

## Память

RP2040: 264 КБ RAM. Прошивка ~1.6 МБ / 2 МБ flash. Дисплей работает в режиме
streaming (без framebuffer), что экономит ~150 КБ RAM.

## Лицензия

InfoNES распространяется под собственной лицензией (см. файлы в `infones/`).
Остальной код — по лицензии проекта.

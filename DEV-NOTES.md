# DEV-NOTES — шпаргалка по pico-retro v2

> Быстрый старт для новой сессии. Подробнее: README.md, CHANGES.md.

## Что это
Мультисистемная ретроконсоль на **Raspberry Pi Pico (RP2040)**:
- **NES** — ядро InfoNES (138 мапперов)
- **Atari 2600** — ядро **Virtual VCS (x2600) из MCUME** (`atari2600_mcume/`)
- В планах: **Sega Master System** (ядро лежит в `smsplus/`, не интегрировано)

## Atari 2600 — ядро MCUME (с 2026-08-21)
Заменил примитивный x2600 (dgrubb) на порт **Virtual VCS / x2600 из MCUME** (mcume/MCUME_pico/picovcs/):
- Папка **`atari2600_mcume/`** — ядро + `system_atari_mcume.cpp` (наш слой, без emuapi).
- Файлы ядра собираются как **-std=gnu89** (код 1996 г.: implicit int `register p1;`, `__inline`, GNU-расширения).
- **ROM читается напрямую из flash (XIP)** через `theCart` → экономия ~32 КБ RAM. Только 2K-карты (Air-Sea Battle) зеркалятся в маленький буфер 4К.
- **Добавлен банкинг F4 (32К, 8 банков, $FF4..$FFB)** — в оригинальном MCUME его нет (только F8/F6/E0/FA/F6SC). Это чинит **DK Arcade (32К F4)**.
- Автоопределение банка по размеру ROM: 8К→F8(1), 16К→F6(2), 32К→F4(6), иначе 0.
- Рендер: `mainloop()` сам зовёт `tv_display()` → `emu_DrawScreenPal16` → наш `display_stream_pixels`. `atari2600_render()` пустой.
- Ввод: `vcs_Input()` каждый кадр читает `joypad_buttons()` (A=fire). Кнопка B не используется.
- RAM: ~241 КБ из 264 КБ занято (InfoNES + MCUME вместе). Статическая аллокация.

## Репозиторий
- **pico-retro-v2**: https://github.com/Mikl-GV/pico-retro-v2 (ветка main)
- Remote в git: `v2` (оригинальный pico-retro = `origin`, он для v1)

## Сборка (Windows)
```powershell
# Путь с кириллицей не работает для GNU ld → junction
# C:\pico-work\pico-retro должен указывать на папку проекта
cd C:\pico-work\pico-retro
$env:PICO_SDK_PATH = "C:\Users\PCB\pico-sdk"
# инструменты (winget): xpack arm-none-eabi, WinLibs mingw, ninja
.\build.ps1          # или см. ниже вручную
```
Прошивка: `build/pico_retro.uf2`
Плата: **16 МБ флеша** (`PICO_FLASH_SIZE_BYTES=16777216` задан в CMakeLists ДО pico_sdk_init).

## Структура
```
├── src/
│   ├── main.cpp         # меню (системы → игры), настройки, тест джойстика
│   ├── display.c        # ILI9341 8-bit 8080 bit-bang + шрифт 8x8
│   └── joypad.c         # 8 кнопок GPIO, активны LOW, 0=нажато
├── infones/             # ядро NES (InfoNES) + system_pico_retro.cpp (бордюры)
├── atari2600_mcume/     # ядро Atari (Virtual VCS из MCUME) + system_atari_mcume.cpp
│                        #   + emu_stubs.h (замена emuapi) + vcs_display.h
├── atari2600/           # НЕ компилируется; только rom_*.h (вшитые ROM Atari)
├── smsplus/             # ядро SMS (НЕ интегрировано, ждёт подключения)
├── include/
│   ├── config.h         # карта пинов, LCD, кнопки
│   ├── font8x8.h        # единственный шрифт (8x8)
│   ├── thumbs.h         # миниатюры игр для бордюров
│   └── rom_*.h          # вшитые ROM NES
└── atari2600/rom_*.h    # вшитые ROM Atari
```

## Меню и управление
- **Меню систем**: Up/Down выбор, **Start** — вход, B — назад
- **Меню игр**: Up/Down выбор, **Start** — запуск, B — назад
- **Settings → Pad Test**: тест кнопок с нарисованным NES-геймпадом, **выход по удержанию Select**
- **About**: версия, RAM/Flash, CPU, разработчики (Mikl-GV + MultiTool)

## Кнопки (GPIO, активны LOW)
| Кнопка | GPIO | NES bit |
|---|---|---|
| A | 22 | 0 |
| B | 27 | 1 |
| Select | 15 | 2 |
| Start | 26 | 3 |
| Up/Down/Left/Right | 18/19/20/21 | 4/5/6/7 |

## Atari 2600 — особенности (ядро MCUME Virtual VCS)
- Банки: F8 (8К→$0FF8), F6 (16К→$0FF6), **F4 (32К→$0FF4, добавлен нами)**; 2К/4К без банков (2К зеркалится в 4К)
- Стартовый банк — последний (ресет-вектор в конце), ставится в `init_banking()`
- Банк-селект и при чтении, и при записи hotspot (Memory.c `bank_switch_read/write`)
- Fire: кнопка A (MASK_JOY2_BTN) → INPT4. Полярность: покой=1, нажатие=0 (keytrig)
- Направления: JOY2-маски → SWCHA D4..D7 (keyjoy), **Right=D6, Left=D7**
- Сложность: `nOptions_P1Diff` → SWCHB bit6 (1=amateur). `atari2600_set_difficulty(1)` инвертирует (expert)
- Выход из игры: удержание Start (в main.cpp, по кадрам)
- `atari2600_run_frame()` → `mainloop()` (7600 итераций CPU) — сам рисует кадр через `tv_display()`
- `atari2600_init()` — копирует ROM (2К→зеркало 4К) или ставит `theCart` на flash (XIP), авто-банк
- **RAM-оптимизация**: ROM из flash (XIP), не в RAM — экономия ~32 КБ
- Прошлый лимит (dgrubb x2600 не тянул DK/California/DD) снят — теперь F4 есть

## Игры Atari (11)
Halo 2600 (4K), Thrust (16K F6), DK Arcade (32K F4), Air-Sea Battle (2K),
Asteroids (8K F8), California Games (16K F6), Dark Cavern (4K),
Double Dragon (16K F6), Space Tunnel (4K), **Bumper Bash (4K, пинбол)**,
**Video Pinball (4K, пинбол)**

## Игры NES (11)
Balloon Fight, Battle City, Bomberman, Contra, Duck Tales, Duck Tales II,
Fant. Adv. Dizzy, Little Nemo, Mario Bros., Saiyuuki World, SMB

## Settings
- **Pad Test** — тест кнопок с нарисованным NES-геймпадом, выход по удержанию Select
- **Atari Diff** — переключатель сложности P1 (Novice/Expert)

## Известные TODO
- [x] Переход Atari на ядро MCUME x2600 (Virtual VCS) — сделано, F4/DK работает
- [ ] Проверить на железе все 11 Atari-игр (особенно DK Arcade, Double Dragon, California)
- [ ] Интеграция SMS (ядро в `smsplus/`, подключить в CMake + меню)
- [ ] Возможно: переиспользование RAM через union (RAM почти вся занята: ~241/264 КБ)

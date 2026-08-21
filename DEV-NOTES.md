# DEV-NOTES — шпаргалка по pico-retro v2

> Быстрый старт для новой сессии. Подробнее: README.md, CHANGES.md.

## Что это
Мультисистемная ретроконсоль на **Raspberry Pi Pico (RP2040)**:
- **NES** — ядро InfoNES (138 мапперов)
- **Atari 2600** — ядро x2600 (mos6507 + TIA + mos6532)
- В планах: **Sega Master System** (ядро лежит в `smsplus/`, не интегрировано)

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
├── atari2600/           # ядро Atari (atari/ mos6507/ mos6532/) + system_atari.cpp
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

## Atari 2600 — особенности
- Банки: F8 (8К→$1FF8), F6 (16К→$1FF6), F4 (32К→$1FF4); 2К/4К без банков (2К зеркалится)
- Fire: кнопка A → INPT4 (джойстик 1) + INPT5 (джойстик 2)
- Направления: SWCHA D4-D7
- Выход из игры: удержание Start (~1.5-2 сек)
- `atari2600_run_frame` использует статические счётчики кадра, сбрасываются в `atari2600_init`
- Известные проблемы: чёрный экран/мусор у части игр (Thrust/DK/California), может быть из-за мапперов

## RAM/Flash
- RAM: 264 КБ (RP2040). Текущий BSS ~185 КБ → запас ~80 КБ
- Flash: 16 МБ. Прошивка ~1.7 МБ → запас ~14 МБ
- Ограничение — RAM, не flash. SMS нужно впихивать в остаток.

## Полезные команды
```powershell
# Пересобрать после изменения CMakeLists
cmake -S C:\pico-work\pico-retro -B C:\pico-work\pico-retro\out -G Ninja `
  -DPICO_SDK_PATH=C:\Users\PCB\pico-sdk -DPICO_BOARD=pico -DPICO_FLASH_SIZE_BYTES=16777216
ninja -C C:\pico-work\pico-retro\out
# Пуш в v2
git push v2 HEAD:main
```

## Известные TODO
- [ ] Интеграция SMS (ядро в `smsplus/`, подключить в CMake + меню)
- [ ] Отладить Atari (чёрный экран у части игр)
- [ ] Возможно: переиспользование RAM через union (все эмуляторы сразу)

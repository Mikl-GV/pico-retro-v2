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
- Банки: F8 (8К→$0FF8), F6 (16К→$0FF6), F4 (32К→$0FF4); 2К/4К без банков (2К зеркалится)
- Стартовый банк — последний (ресет-вектор в конце)
- Банк-селект и при чтении, и при записи hotspot (как в x2600/MCUME)
- Fire: кнопка A → INPT4/INPT5. **Полярность (подтверждена на железе): покой=1 (0x80), нажатие=0 (0x00)**
- Направления: SWCHA — Up=D4, Down=D5, **Right=D6, Left=D7** (не перепутать!)
- Сложность: SWCHB bit6=P1 (0=amateur,1=expert), настройка в Settings→Atari Diff, применяется при старте игры
- Выход из игры: удержание Start (~1.5-2 сек)
- `atari2600_run_frame` — статические счётчики кадра + предохранитель (3000 итераций), кадр = 192 видимых строк
- `atari2600_init` очищает кадровый буфер (memset), сбрасывает синхронизацию
- Известные проблемы: примитивное ядро x2600 (dgrubb) — часть банкованных игр (Double Dragon, California, DK) могут не работать. Планируется переход на полноценное ядро MCUME x2600 (в mcume/MCUME_pico/picovcs/)

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
- [ ] Интеграция SMS (ядро в `smsplus/`, подключить в CMake + меню)
- [ ] Переход Atari на ядро MCUME x2600 (для совместимости с банкованными играми)
- [ ] Возможно: переиспользование RAM через union (все эмуляторы сразу)

# Описание изменений pico-retro

Дата: 20 августа 2026. Ветка: `cursor/windows-build-and-menu-fixes`.

Прошивка собрана: `build/pico_retro.uf2`  
Flash: ~1953 КБ / 2048 КБ. RAM (BSS): ~234 КБ / 264 КБ. SYSCLK: 250 МГц.

## Сборка под Windows

GNU `arm-none-eabi-ld` не открывает скрипты линковки, если в пути есть кириллица (`D:\Программы\...`).

- Собирать через ASCII-путь: `C:\pico-work\pico-retro` (junction на этот репозиторий).
- SDK: `C:\pico-sdk`.
- Скрипт: `.\build.ps1` (из корня репозитория).
- Каталог `out/` добавлен в `.gitignore`.
- Убрана генерация неиспользуемого PIO-заголовка (`ili9341_8bit.pio` — дисплей bit-bang).

```powershell
cd C:\pico-work\pico-retro
$env:PICO_SDK_PATH = "C:\pico-sdk"
$env:Path = "$env:USERPROFILE\scoop\shims;$env:USERPROFILE\scoop\apps\gcc-arm-none-eabi\current\bin;$env:USERPROFILE\scoop\apps\mingw\current\bin;$env:Path"
cmake -S . -B out -G Ninja -DPICO_SDK_PATH=C:\pico-sdk -DPICO_BOARD=pico
cmake --build out
```

## Меню и About

- Скролл меню больше не уходит в отрицательный индекс, если пунктов меньше 20 (раньше читался `games[-6]`).
- About: свободная RAM считается как `__StackLimit - __bss_end__`, а не наоборот.
- Добавлены заголовки `hardware/clocks.h` и `pico/stdio_uart.h`.

## Разгон CPU 250 МГц

Раньше вызывался только `set_sys_clock_khz(250000)` при штатных 1.10 В — частота часто не держалась.

Теперь при старте:

1. `vreg_set_voltage(VREG_VOLTAGE_1_20)`
2. пауза 10 мс
3. `set_sys_clock_khz(250000, true)`

В UART: `sysclk=250 MHz`. В Settings показывается фактическая частота (`clock_get_hz`).

## Мапперы

По заголовкам iNES в прошивке:

| Игра | Маппер | Было | Стало |
|---|---|---|---|
| Balloon Fight, Battle City, Bomberman, Mario Bros. | 0 NROM | ок | mirroring по спецификации iNES |
| Duck Tales, Duck Tales II | 2 UxROM | ок | маска банка от размера PRG |
| Saiyuuki World | 2 UxROM 256 КБ | банк `v & 7` (только 128 КБ) | маска по PRG (16 банков) |
| SMB, SMB 6, Little Nemo | 4 MMC3 | несколько ошибок банков/IRQ | исправлено |
| Contra (J) | 23 VRC2 | не реализован, шёл как NROM | PRG 8 КБ + CHR 1 КБ + mirroring |
| Fant. Adv. Dizzy | 71 Camerica | не реализован | как UxROM |

### MMC3 (mapper 4)

- PRG mode 1: `$8000–$9FFF` — **предпоследний** банк, не последний.
- CHR-адрес 32-битный (128 КБ CHR больше не обрезается `uint16_t`).
- `$A000` bit0: 0 = vertical, 1 = horizontal (больше не 1-screen).
- `$C001` ставит флаг reload IRQ, а не сразу грузит счётчик.
- `$E000` выключает IRQ и сбрасывает `irq_pending`.
- `$A001`: bit7 = enable PRG-RAM, bit6 = write-protect.
- Запись в `$4016` больше не проваливается в банк-селект MMC3.

### PPU / CPU регистры

- Реализованы `$2003` (OAMADDR) и `$2004` (OAMDATA).
- Sprite 0 hit (`$2002` bit 6) при непрозрачном пикселе спрайта 0 поверх фона.
- Спрайты 8×16: таблица из bit0 тайла, два тайла подряд.
- Pre-render (scanline 261): сброс VBlank / sprite 0 / overflow; `v ← t` при включённом рендере (раньше `t→v` ошибочно на 241).
- `$2002` по-прежнему сбрасывает только VBlank и w-latch.
- `cpu6502_init` обнуляет `nmi_pending`.

При старте игры в UART: `mapper=… prg=…K chr=…K mirror=…`.

## Джойстик `$4016` (самопроизвольное движение)

GPIO кнопки — активный LOW: `joypad_buttons()` даёт **0 = нажато**, в покое `0xFF`. Этот байт клали прямо в serial latch.

На NES bit0 `$4016` — **1 = нажато**. Игра читала восемь единиц и считала все кнопки зажатыми (в том числе направления).

Исправлено:

- в latch идёт `~buttons` (полярность NES);
- пока strobe = 1, каждый read отдаёт живую кнопку A без сдвига;
- начальный latch `0x00`, а не `0xFF`;
- меню не трогали: там по-прежнему `~pad`.

## Файлы

- `src/main.c` — разгон, меню, About, UART
- `src/nes.c`, `include/nes.h` — мапперы, PPU, `$4016`
- `src/cpu6502.c` — сброс NMI
- `CMakeLists.txt`, `build.ps1`, `.gitignore` — сборка Windows

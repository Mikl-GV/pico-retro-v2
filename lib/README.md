# Внешние библиотеки (git submodules)

```
git submodule add https://github.com/raspberrypi/pico-extras.git lib/pico-extras
```

- `pico-extras` — `pico_scanvideo` для композитного видеовыхода (PAL/NTSC).

Сборка с видео: укажите путь к pico-extras при конфигурации:

```bash
export PICO_EXTRAS_PATH=$PWD/lib/pico-extras
cmake .. -DPICO_EXTRAS_PATH=$PICO_EXTRAS_PATH
```

Если pico-extras не найден — `video.c` компилируется как заглушка,
всё остальное работает без него.

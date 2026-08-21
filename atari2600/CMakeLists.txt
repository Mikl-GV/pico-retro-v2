add_library(atari2600 INTERFACE)

target_sources(atari2600
INTERFACE
    system_atari.cpp
    atari/Atari-TIA.c
    atari/Atari-cart.c
    atari/Atari-memmap.c
    mos6507/mos6507.c
    mos6507/mos6507-microcode.c
    mos6507/mos6507-opcodes.c
    mos6532/mos6532.c
)

target_include_directories(atari2600
INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
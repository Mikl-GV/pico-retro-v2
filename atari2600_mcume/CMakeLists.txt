add_library(atari2600_mcume STATIC
    system_atari_mcume.cpp
    Vcsemu.c
    Vmachine.c
    Cpu.c
    Memory.c
    Raster.c
    Table.c
    Display.c
    Collision.c
    Tiasound.c
    Options.c
    Keyboard.c
    Exmacro.c
)

# The x2600/Virtual-VCS core is 1996 C: implicit int (register p1;), __inline,
# GNU extensions. Compile the core .c files as gnu89 to keep them compiling.
# Force -O0 on BOTH the core and the system layer: verified on the Pico and
# reproduced on host that any -O1+ on this legacy code breaks games (blank
# frames / hangs). Atari 2600 is cheap to emulate; -O0 keeps full speed on
# the 250 MHz RP2040.
set_source_files_properties(
    Vcsemu.c Vmachine.c Cpu.c Memory.c Raster.c Table.c Display.c
    Collision.c Tiasound.c Options.c Keyboard.c Exmacro.c
    PROPERTIES COMPILE_OPTIONS "-std=gnu89;-O0"
)
set_source_files_properties(
    system_atari_mcume.cpp
    PROPERTIES COMPILE_OPTIONS "-O0"
)

target_include_directories(atari2600_mcume
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

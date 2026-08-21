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
set_source_files_properties(
    Vcsemu.c Vmachine.c Cpu.c Memory.c Raster.c Table.c Display.c
    Collision.c Tiasound.c Options.c Keyboard.c Exmacro.c
    PROPERTIES COMPILE_OPTIONS "-std=gnu89"
)

# system_atari_mcume.cpp misbehaves under -O1/-O2/-O3 (games hang/crash on
# host with all optimization levels >= -O1, fine at -O0). Keep it at -O0;
# the core itself stays at -O3 for speed.
set_source_files_properties(
    system_atari_mcume.cpp
    PROPERTIES COMPILE_OPTIONS "-O0"
)

target_include_directories(atari2600_mcume
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

cmake_minimum_required(VERSION 3.13)

include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)

project(pico_retro C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# GNU arm-none-eabi-ld on Windows cannot open linker scripts if CMAKE_BINARY_DIR
# contains non-ASCII characters (e.g. D:\Программы\...).
if (CMAKE_BINARY_DIR MATCHES "[^ -~]")
    message(FATAL_ERROR
        "Путь сборки содержит не-ASCII символы, GNU ld на Windows их не открывает.\n"
        "Собирайте через ASCII-путь:\n"
        "  cd C:\\pico-work\\pico-retro\n"
        "  cmake -S . -B out -G Ninja -DPICO_SDK_PATH=C:\\pico-sdk -DPICO_BOARD=pico\n"
        "  cmake --build out")
endif()

pico_sdk_init()

add_executable(pico_retro
    src/main.c
    src/display.c
    src/joypad.c
    src/cpu6502.c
    src/nes.c
)

target_include_directories(pico_retro PRIVATE include)

target_link_libraries(pico_retro PRIVATE
    pico_stdlib
    hardware_gpio
    hardware_pwm
    hardware_uart
    hardware_vreg
    pico_multicore
)

pico_enable_stdio_uart(pico_retro 1)
target_compile_options(pico_retro PRIVATE
    -O3
    -funroll-loops
    -finline-functions
)

target_compile_definitions(pico_retro PRIVATE
    PICO_DEFAULT_UART_TX_PIN=16
    PICO_DEFAULT_UART_RX_PIN=17
    PICO_NO_HARDWARE_ASSERT=1
)

# pico-extras: композитное видео через pico_scanvideo (пока не используется)
if (DEFINED ENV{PICO_EXTRAS_PATH})
    set(PICO_EXTRAS_PATH $ENV{PICO_EXTRAS_PATH})
endif()

if (PICO_EXTRAS_PATH AND EXISTS "${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake")
    include("${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake")
    message(STATUS "pico-extras найден (видео пока не задействовано)")
endif()

pico_add_extra_outputs(pico_retro)

# Extra -L on an ASCII path: ld cannot search CMAKE_BINARY_DIR if it has Cyrillic.
if (WIN32)
    set(_ascii_ld_dir "C:/pico-ld/pico-retro")
    file(MAKE_DIRECTORY "${_ascii_ld_dir}")
    set(_gen_ld_dir "${CMAKE_BINARY_DIR}/pico-sdk/src/rp2_common/pico_standard_link")
    foreach(_ld pico_flash_region.ld pico_psram_region.ld)
        if (EXISTS "${_gen_ld_dir}/${_ld}")
            configure_file("${_gen_ld_dir}/${_ld}" "${_ascii_ld_dir}/${_ld}" COPYONLY)
        endif()
    endforeach()
    target_link_options(pico_retro PRIVATE "LINKER:-L${_ascii_ld_dir}")
endif()

cmake_minimum_required(VERSION 3.13)

include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)

project(pico_retro C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

pico_sdk_init()

add_executable(pico_retro
    src/main.c
    src/display.c
    src/joypad.c
    src/cpu6502.c
    src/nes.c
)

pico_generate_pio_header(pico_retro ${CMAKE_CURRENT_LIST_DIR}/ili9341_8bit.pio)

target_include_directories(pico_retro PRIVATE include)

target_link_libraries(pico_retro PRIVATE
    pico_stdlib
    hardware_gpio
    hardware_pwm
    hardware_uart
    pico_multicore
)

pico_enable_stdio_uart(pico_retro 1)
target_compile_definitions(pico_retro PRIVATE
    PICO_DEFAULT_UART_TX_PIN=16
    PICO_DEFAULT_UART_RX_PIN=17
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

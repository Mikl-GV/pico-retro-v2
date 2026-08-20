add_library(infones INTERFACE)

target_sources(infones
INTERFACE
    InfoNES.cpp
    InfoNES_Mapper.cpp
    InfoNES_pAPU.cpp
    K6502.cpp
    system_pico_retro.cpp
)

target_include_directories(infones
INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mapper
)
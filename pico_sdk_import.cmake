if (NOT DEFINED PICO_SDK_PATH)
    if (DEFINED ENV{PICO_SDK_PATH})
        set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    endif()
endif()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH не задан. Укажите путь к pico-sdk: cmake -DPICO_SDK_PATH=...")
endif()

if (NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "В ${PICO_SDK_PATH} не найден pico-sdk")
endif()

include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

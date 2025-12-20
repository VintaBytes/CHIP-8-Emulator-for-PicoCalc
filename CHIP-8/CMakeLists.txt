# Generated CMake project file (reorganizado para SDK + apps)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == VSCode + Pico SDK integration (tomado del archivo original) ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ================================================================

set(PICO_BOARD pico CACHE STRING "Board type")

# Importar Pico SDK
include(pico_sdk_import.cmake)

project(picocalc-sdk C CXX ASM)

pico_sdk_init()

#
# Libreria comun con los drivers de PicoCalc
#
set(PICOCALC_DRIVERS
    drivers/audio.c
    drivers/clib.c
    drivers/display.c
    drivers/fat32.c
    drivers/font-5x10.c
    drivers/font-8x10.c
    drivers/keyboard.c
    drivers/lcd.c
    drivers/onboard_led.c
    drivers/picocalc.c
    drivers/sdcard.c
    drivers/southbridge.c
    drivers/console.c
    drivers/gfx.c
)

add_library(picocalc_drivers STATIC ${PICOCALC_DRIVERS})

# Generar header de PIO para el driver de audio
pico_generate_pio_header(picocalc_drivers ${CMAKE_CURRENT_LIST_DIR}/drivers/audio.pio)

# Activar warnings para los drivers
target_compile_options(picocalc_drivers PRIVATE -Wall -Werror)

# Librerias necesarias para los drivers
target_link_libraries(picocalc_drivers
    pico_stdlib
    pico_printf
    pico_float
    pico_status_led
    pico_rand
    hardware_gpio
    hardware_i2c
    hardware_spi
    hardware_pio
    hardware_clocks
)

# Incluir headers desde la raiz (para drivers/ y demas)
target_include_directories(picocalc_drivers PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

#
# Apps
#
# add_subdirectory(apps/repl)
# add_subdirectory(apps/hello)
# add_subdirectory(apps/prueba1)
# add_subdirectory(apps/demo1)
# add_subdirectory(apps/graficos1)
add_subdirectory(apps/chip8)
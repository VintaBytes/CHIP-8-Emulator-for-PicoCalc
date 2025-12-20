add_executable(picocalc-chip8
    src/app/main.c
    src/app/rom_menu.c
    src/core/chip8.c
    src/platform/chip8_picocalc.c
)

pico_set_program_name(picocalc-chip8 "picocalc-chip8")
pico_set_program_version(picocalc-chip8 "0.1")

pico_enable_stdio_uart(picocalc-chip8 0)
pico_enable_stdio_usb(picocalc-chip8 0)

target_include_directories(picocalc-chip8 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/app
    ${CMAKE_CURRENT_LIST_DIR}/src/core
    ${CMAKE_CURRENT_LIST_DIR}/src/platform
    ${CMAKE_CURRENT_LIST_DIR}/../../drivers
)

target_link_libraries(picocalc-chip8
    picocalc_drivers
)

pico_add_extra_outputs(picocalc-chip8)

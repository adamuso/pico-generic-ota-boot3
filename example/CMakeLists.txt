cmake_minimum_required(VERSION 3.13)

# If this directory is added as a subdirectory of the repository root, the
# root already provides `boot3` and sets `PICO_SDK_PATH`. If not, fall back
# to a sensible default relative path so this example can be configured on
# its own for testing.
if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/../lib/pico-sdk")
endif()

set(PICO_BOARD waveshare_rp2040_zero)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(example_blink C CXX ASM)

pico_sdk_init()

## If this example is configured on its own (out-of-tree relative to the
## repository root) we must provide an explicit binary directory when
## adding a source directory that lives outside this source dir.
## Add the repository root so the `boot3` INTERFACE target is available.
add_subdirectory(${CMAKE_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/rp2040_ota_bootloader_root_build)

add_executable(bare_blink 
    ws2812.c
    main.c
)

# Ensure the generated memmap with .boot3 is used by this target
pico_boot3_init(bare_blink) 

set_property(TARGET bare_blink PROPERTY C_STANDARD 11)

# Link standard pico libs and our boot3 interface target (adds linker script)
target_link_libraries(bare_blink PRIVATE 
    pico_stdlib
    pico_stdio_usb
    boot3
)

target_link_options(bare_blink PUBLIC 
    "-Wl,-Map=bare_blink.map"
    "-Wl,--cref"
    "-Wl,--print-memory-usage"
)

pico_enable_stdio_usb(bare_blink 1)
pico_enable_stdio_uart(bare_blink 0)

pico_add_extra_outputs(bare_blink)

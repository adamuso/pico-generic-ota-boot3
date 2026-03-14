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

# If this example is configured on its own (out-of-tree relative to the
# repository root) we must provide an explicit binary directory when
# adding a source directory that lives outside this source dir.
# Add the repository root so the `boot3` INTERFACE target is available.
add_subdirectory(${CMAKE_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/pico-ota-boot3)

project(example_blink C CXX ASM)

pico_sdk_init()

add_executable(example_serial_ota 
    ws2812.c
    main.c
)

target_compile_definitions(example_serial_ota PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=5000
)

# Ensure the generated memmap with .boot3 is used by this target
pico_boot3_init(example_serial_ota) 

set_property(TARGET example_serial_ota PROPERTY C_STANDARD 11)

# Link standard pico libs and our boot3 interface target (adds linker script)
target_link_libraries(example_serial_ota PRIVATE 
    pico_stdlib
    pico_stdio_usb
)

target_link_options(example_serial_ota PUBLIC 
    "-Wl,-Map=bare_blink.map"
    "-Wl,--cref"
    "-Wl,--print-memory-usage"
)

pico_enable_stdio_usb(example_serial_ota 1)
pico_enable_stdio_uart(example_serial_ota 0)

pico_add_extra_outputs(example_serial_ota)

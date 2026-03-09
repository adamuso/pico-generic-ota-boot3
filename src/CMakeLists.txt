# include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# pico_sdk_init()

# Build an object library containing the boot3 image (assembly + C).
add_library(boot3_image OBJECT
    boot3.S
    boot3.c
    flash.c
)

# set_target_properties(boot3_image PROPERTIES
#     C_VISIBILITY_PRESET hidden
#     VISIBILITY_INLINES_HIDDEN ON
# )

# target_compile_options(boot3_image PRIVATE -fvisibility=hidden)
target_link_options(boot3_image PRIVATE "--specs=nosys.specs")
target_link_options(boot3_image PRIVATE "-nostartfiles")

# Ensure assembler files are treated appropriately (some toolchains need this)
set_target_properties(boot3_image PROPERTIES LINKER_LANGUAGE C)

# Provide includes from the pico SDK so the hardware headers used by boot3.c
# are available when this project is built as a subdirectory of an example.
if(DEFINED PICO_SDK_PATH)
    target_include_directories(boot3_image PRIVATE 
        ./
        ${PICO_SDK_PATH}/src/common/pico_base_headers/include
        ${PICO_SDK_PATH}/src/rp2_common/pico_platform_common/include
        ${PICO_SDK_PATH}/src/rp2_common/pico_platform_compiler/include
        ${PICO_SDK_PATH}/src/rp2_common/pico_platform_sections/include
        ${PICO_SDK_PATH}/src/rp2_common/pico_platform_panic/include
        ${PICO_SDK_PATH}/src/rp2_common/boot_bootrom_headers/include
        ${PICO_SDK_PATH}/src/rp2_common/pico_bootrom/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_base/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_boot_lock/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_flash/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_xip_cache/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_gpio/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_irq/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_resets/include
        ${PICO_SDK_PATH}/src/rp2_common/pico_flash/include
        ${PICO_SDK_PATH}/src/rp2040/hardware_regs/include
        ${PICO_SDK_PATH}/src/rp2040/hardware_structs/include
        ${PICO_SDK_PATH}/src/rp2040/pico_platform/include
    )
endif()

# target_link_libraries(boot3_image
#     hardware_flash
# )


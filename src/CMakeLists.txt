# Default PICO SDK path (projects can override by setting this variable)

# Build an object library containing the boot3 image (assembly + C).
add_library(boot3_image OBJECT
    boot3.S
    boot3.c
)

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
        ${PICO_SDK_PATH}/src/rp2_common/hardware_flash/include
        ${PICO_SDK_PATH}/src/rp2040/hardware_regs/include
        ${PICO_SDK_PATH}/src/rp2040/pico_platform/include
    )
endif()

# Expose the object library so callers can link it into their final image by
# linking against the `boot3` interface target from the root CMakeLists.


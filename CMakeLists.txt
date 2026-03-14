cmake_minimum_required(VERSION 3.22)

# Allow this project to be configured on its own for testing
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_C_COMPILER_WORKS 1)
    set(CMAKE_CXX_COMPILER_WORKS 1)
endif()

project(rp2040_boot3 C CXX ASM)

# Option to control injection (on by default)
option(BOOT3_INJECT_MEMMAP "Inject .boot3 section into pico SDK memmap_default.ld" ON)

set(PICO_FLASH_SIZE_BYTES "${PICO_FLASH_SIZE_BYTES} / 2" PARENT_SCOPE)

# Default PICO SDK path (projects can override by setting this variable)
if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/lib/pico-sdk")
endif()

set(BOOT3_MEMMAP_PATH "${PICO_SDK_PATH}/src/rp2_common/pico_crt0/rp2040/memmap_default.ld" CACHE STRING "Path to base memmap file")

# Non-invasive injection: do NOT modify files under the pico-sdk source tree.
# Instead generate a modified linker script in the CMake binary dir and expose
# its path to callers. This keeps the SDK folder immutable.
set(MEMMAP_GENERATED "${CMAKE_BINARY_DIR}/boot3_memmap.ld")

if(BOOT3_INJECT_MEMMAP)
    if(EXISTS ${BOOT3_MEMMAP_PATH})
        file(READ "${BOOT3_MEMMAP_PATH}" _mem)

        # Insert a .boot3 section immediately before the .text section
        set(_pattern "\n    .text : {")

        # Insert a .boot3 section immediately before .text and reserve exactly
        # 4 KiB for it. Define start/end symbols and ASSERT if actual contents
        # overflow the reserved area, then advance the location counter to the
        # end of the reserved area to effect padding.
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/boot3.ld" _replace)
        string(REPLACE "${_pattern}" "${_replace}" _new_mem "${_mem}")

        set(_pattern "\n    .ram_vector_table (NOLOAD): {")
        set(_replace "\n    . = ORIGIN(RAM);\n\n    .ram_vector_table . (NOLOAD): {")

        string(REPLACE "${_pattern}" "${_replace}" _new_mem "${_new_mem}")

        if(NOT "${_mem}" STREQUAL "${_new_mem}")
            # write modified file into build dir (non-invasive)
            file(WRITE "${MEMMAP_GENERATED}" "${_new_mem}")
            message(STATUS "Generated injected memmap at ${MEMMAP_GENERATED} (pico-sdk left unchanged)")
        else()
            message(STATUS "No injection needed; .boot3 already present or insertion point not found in ${BOOT3_MEMMAP_PATH}")
        endif()
    else()
        message(FATAL_ERROR "Base memmap file not found at ${BOOT3_MEMMAP_PATH}; skipping .boot3 generation")
    endif()
endif()

# Minimal interface library so other projects can do add_subdirectory(...) and link to `boot3`
add_library(boot3 INTERFACE)
target_compile_definitions(boot3 INTERFACE USE_BOOT3=1)
target_include_directories(boot3 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Build the boot3 image (assembly + C) and attach it to the `boot3`
# interface target so callers linking to `boot3` will automatically
# include the .boot3 contents in their final image.

# Add the local `src` directory and place its build output inside this
# project's binary dir so it is correctly built when this project is
# included via add_subdirectory(...) from other CMake projects.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tools/fnv1a_checksum)

# The object library was created, expose its objects through the
# `boot3` INTERFACE target so callers get the compiled .boot3 contents
# automatically when they link to `boot3`.
target_link_libraries(boot3 INTERFACE $<TARGET_OBJECTS:boot3_image>)

# Expose the path as an internal cached variable for callers
if(EXISTS "${MEMMAP_GENERATED}")
    set(BOOT3_MEMMAP_INJECTED "${MEMMAP_GENERATED}" CACHE INTERNAL "Path of generated memmap file injected with .boot3")
else()
    set(BOOT3_MEMMAP_INJECTED "${MEMMAP_DEFAULT}" CACHE INTERNAL "Path of memmap file (original pico-sdk)")
endif()

# A command that will inject the .boot3 section into a given memmap file and write the result to a specified output path. This is for users who want to do the injection themselves or need to support older CMake versions that don't have the `file(READ/WRITE)` commands used above.
function(pico_boot3_init TARGET)
    if(EXISTS "${BOOT3_MEMMAP_INJECTED}")
        # Prefer using the pico SDK helper to set the target linker script so
        # pico_standard_link's logic is used and we don't pass multiple -T/--script
        # options to the linker which would redeclare memory regions.
        if(COMMAND pico_set_linker_script)
            message(STATUS "Using pico_set_linker_script to set injected memmap for target ${TARGET}")
            pico_set_linker_script(${TARGET} "${BOOT3_MEMMAP_INJECTED}")
        else()
            target_link_options(${TARGET} INTERFACE "-T${BOOT3_MEMMAP_INJECTED}")
        endif()
    endif()

    target_link_libraries(${TARGET} PRIVATE boot3)

    # Generate checksum based on the produced .bin file. Create a file-based
    # custom command that depends on the .bin (so it runs after the .bin is
    # generated), and a custom target to include the checksum generation in the
    # default build.
    set(_flash_bin "${CMAKE_BINARY_DIR}/${TARGET}.bin")
    set(_flash_checksum "${CMAKE_BINARY_DIR}/${TARGET}_checksum.bin")
    set(_flash_program_size "${CMAKE_BINARY_DIR}/${TARGET}_program_size.bin")
  
    # add_custom_command(
    #     OUTPUT ${_flash_checksum}
    #     COMMAND ${BOOT3_FNV1A_HOST} ${_flash_bin} 0x2000 > ${_flash_checksum}
    #     DEPENDS boot3_fnv1a_checksum ${_flash_bin}
    #     COMMENT "Generating checksum data for .boot3_state_checksum section..."
    #     VERBATIM
    # )

    # add_custom_target(flash_checksum_for_${TARGET} ALL
    #     DEPENDS ${_flash_checksum}
    # )

    # add_dependencies(flash_checksum_for_${TARGET} boot3_fnv1a_checksum)

    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET}> ${_flash_bin}
        COMMAND ${BOOT3_FNV1A_HOST} ${_flash_bin} 0x2000 > ${_flash_checksum}
        COMMAND ${BOOT3_FNV1A_HOST} ${_flash_bin} 0x2000 calc > ${_flash_program_size}
        COMMAND ${CMAKE_OBJCOPY} --update-section .boot3_state_checksum=${_flash_checksum} --update-section .boot3_state_program_size=${_flash_program_size} $<TARGET_FILE:${TARGET}>
        COMMENT "Updating .boot3_state_checksum section in final ELF with generated checksum..."
        VERBATIM
    )
endfunction(pico_boot3_init)

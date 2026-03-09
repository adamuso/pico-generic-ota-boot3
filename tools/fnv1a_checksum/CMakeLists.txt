cmake_minimum_required(VERSION 3.10)

project(fnv1a_checksum C)

find_program(HOST_C_COMPILER NAMES gcc clang cc)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fnv1a_checksum
    COMMAND ${HOST_C_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/main.c -o ${CMAKE_CURRENT_BINARY_DIR}/fnv1a_checksum
    DEPENDS main.c
    COMMENT "Building host tool for FNV-1a checksum"
)

# Creates a target that can be built
add_custom_target(boot3_fnv1a_checksum ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fnv1a_checksum)

# Expose the path of the built host tool so callers can reference it reliably
set(BOOT3_FNV1A_HOST "${CMAKE_CURRENT_BINARY_DIR}/fnv1a_checksum" CACHE INTERNAL "Path to built fnv1a_checksum_host")


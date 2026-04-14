cmake_minimum_required(VERSION 3.19)
project(Wyvern LANGUAGES C ASM)

# Architecture selection (defaults to the processor set in the toolchain)
if(NOT ARCH)
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

# Create the kernel executable with common sources
add_executable(wyvern 
    main.c
)

target_link_options(wyvern PRIVATE 
    -T ${CMAKE_SOURCE_DIR}/arch/${ARCH}/link.ld
    -nostdlib
    -static
)
set_target_properties(wyvern PROPERTIES LINK_DEPENDS "${CMAKE_SOURCE_DIR}/arch/${ARCH}/link.ld")

# Include the architecture-specific configuration
# This will add arch-specific sources and flags to the 'wyvern' target
add_subdirectory(arch/${ARCH})

# Setup include directories
target_include_directories(wyvern PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/arch
)

# Set output name to wyvern.elf
set_target_properties(wyvern PROPERTIES OUTPUT_NAME "wyvern.elf")

add_custom_command(TARGET wyvern POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary wyvern.elf wyvern.bin
    COMMENT "Converting ELF to flat binary..."
)

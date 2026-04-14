set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(TOOLCHAIN_PREFIX "riscv64-none-elf-")
set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy" CACHE INTERNAL "")

# Skip the "compiler works" check
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)

# Architecture-specific flags
set(RISCV_FLAGS "-march=rv64gc -mabi=lp64d -mcmodel=medany")
set(CMAKE_C_FLAGS "${RISCV_FLAGS} -ffreestanding -nostdlib -Wall -Wextra" CACHE STRING "")
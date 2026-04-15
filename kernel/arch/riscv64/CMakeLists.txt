# RISC-V 64-bit specific sources
target_sources(wyvern PRIVATE
    arch.c
    boot.S
    trap.S
    trap.c
)
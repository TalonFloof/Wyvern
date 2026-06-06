// payload/start.S
.section ".text.boot"
.global _start

_start:
    // Set up stack below our load address
    ldr     x0, =0x1F0000
    mov     sp, x0

    // Clear BSS
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    mov     x2, #0
.Lbss_loop:
    cmp     x0, x1
    b.ge    .Lbss_done
    str     x2, [x0], #8
    b       .Lbss_loop
.Lbss_done:

    bl      main

.Lpark:
    wfe
    b       .Lpark
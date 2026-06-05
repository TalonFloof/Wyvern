.section ".text.boot"
.global _start

_start:
    // Check core ID, park anything that isn't core 0
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3
    cbnz    x0, .Lpark

    // Determine current exception level
    mrs     x0, CurrentEL
    lsr     x0, x0, #2

    // If we're in EL3, drop to EL2
    cmp     x0, #3
    b.eq    .Lel3_to_el2

    // If we're in EL2, drop to EL1
    cmp     x0, #2
    b.eq    .Lel2_to_el1

    // Already in EL1, just set up and go
    b       .Lel1_entry

.Lel3_to_el2:
    // Set up SCR_EL3 — enable AArch64 at EL2, non-secure
    mov     x0, #0x5b1
    msr     scr_el3, x0
    mov     x0, #0x3c9         // SPSR_EL3: mask all exceptions, EL2h
    msr     spsr_el3, x0
    adr     x0, .Lel2_to_el1
    msr     elr_el3, x0
    eret

.Lel2_to_el1:
    // Enable AArch64 at EL1 via HCR_EL2
    mov     x0, #(1 << 31)
    msr     hcr_el2, x0
    mov     x0, #0x3c5         // SPSR_EL2: mask all exceptions, EL1h
    msr     spsr_el2, x0
    adr     x0, .Lel1_entry
    msr     elr_el2, x0
    eret

.Lel1_entry:
    // Set up stack pointer — grows down from load address
    ldr     x0, =_start
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

    // Should never return, but park if it does
.Lpark:
    wfe
    b       .Lpark
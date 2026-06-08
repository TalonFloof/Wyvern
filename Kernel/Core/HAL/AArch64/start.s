.global _start
.global trap_vectors

_start:
    // Save DTB address — x0 will be clobbered before we reach C
    mov     x19, x0

    // Mask all exceptions immediately — DAIF set before anything else
    msr     daifset, #0xF
    isb

    // Select SP_EL1 as stack pointer for all EL1 operation
    msr     spsel, #1
    isb

    // Park non-boot harts until kernel is ready to wake them
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3
    cbnz    x0, .Lpark

    // Set up kernel stack
    ldr     x0, =_stack_top
    mov     sp, x0

    // Clear BSS before any C code runs
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    mov     x2, #0
.Lbss_loop:
    cmp     x0, x1
    b.ge    .Lbss_done
    str     x2, [x0], #8
    b       .Lbss_loop
.Lbss_done:

    // HAL early init — drops from EL2 to EL1, sets up CPU state
    // DTB passed as argument so HAL can stash it if needed
    mov     x0, x19
    bl      hal_early_init

    // Install exception vectors now that we're in EL1
    ldr     x0, =trap_vectors
    msr     vbar_el1, x0
    isb

    // Unmask exceptions now that vectors are installed
    // hal_irq_enable() would work here too but this is pre-C
    msr     daifclr, #0xF
    isb

    // Jump to kernel main with DTB address
    mov     x0, x19
    bl      main

    // main should never return, but park if it does
.Lpark:
    // TODO: Wake these harts when kernel multicore support is ready
    wfe
    b       .Lpark

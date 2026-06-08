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

    // Initialize D-Cache, we do this by invalidating all lines before we can enable it
    // This is a very similar process to what MIPS has you do to initialize cache lines
    // The main difference being we don't have to manually write data, we just invalidate lines using fencing
    mrs x0, clidr_el1 // Cache Level ID Register
    and x3, x0, #0x7000000 // Extract LoC (Level of Coherency)
    lsr x3, x3, #23 // Shift to get level count * 2

    cbz x3, .Lcache_done
    mov x10, #0 // Start at level 0

.Lcache_loop1:
    add x2, x10, x10, lsr #1  // x2 = level * 3
    lsr x1, x0, x2             // Extract cache type for this level
    and x1, x1, #7
    cmp x1, #2                 // Is it data or unified?
    b.lt .Lskip

    msr csselr_el1, x10        // Select cache level
    isb
    mrs x1, ccsidr_el1         // Read cache geometry
    and x2, x1, #7             // Line size encoding
    add x2, x2, #4             // Actual line size shift
    ubfx x4, x1, #3, #10       // Number of ways - 1
    clz  w5, w4                // Bit position of way field
    ubfx x6, x1, #13, #15      // Number of sets - 1
.Lcache_loop2:
    mov x7, x4                 // Way counter
.Lloop3:
    lsl x9, x7, x5
    orr x11, x10, x9
    lsl x9, x6, x2
    orr x11, x11, x9
    dc isw, x11                // Invalidate by set/way
    subs x7, x7, #1
    b.ge .Lcache_loop3
    subs x6, x6, #1
    b.ge .Lcache_loop2

.Lskip:
    add x10, x10, #2
    cmp x10, x3
    b.lt .Lcache_loop1
.Lcache_done:
    dsb sy

    // Park non-boot harts until kernel is ready to wake them
    mrs x0, mpidr_el1
    and x0, x0, #0x3
    cbnz x0, .Lpark

    // Set up kernel stack
    ldr x0, =_stack_top
    mov sp, x0

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

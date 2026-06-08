.balign 2048
.global trap_vectors
trap_vectors:

// Current EL with SP_EL0 (Unused)
.balign 128
    b       trap_unhandled           // Synchronous
.balign 128
    b       trap_unhandled           // IRQ
.balign 128
    b       trap_unhandled           // FIQ
.balign 128
    b       trap_unhandled           // SError

// Current EL with SP_EL1 (kernel exceptions)
.balign 128
    b       trap_sync_el1            // Synchronous — faults, undefined insns
.balign 128
    b       trap_irq_el1             // IRQ
.balign 128
    b       trap_unhandled           // FIQ — unused
.balign 128
    b       trap_serror              // SError — async abort

// Lower EL AArch64 (userspace exceptions)
.balign 128
    b       trap_sync_el0            // Synchronous — syscalls, faults
.balign 128
    b       trap_irq_el0             // IRQ from userspace
.balign 128
    b       trap_unhandled           // FIQ — unused
.balign 128
    b       trap_unhandled           // SError from userspace

// Lower EL AArch32 (Unused)
.balign 128
    b       trap_unhandled
.balign 128
    b       trap_unhandled
.balign 128
    b       trap_unhandled
.balign 128
    b       trap_unhandled

//////////////////////////////////////////////////////////////////////////////////////////

.macro trap_entry
    stp     x0,  x1,  [sp, #-16]!
    stp     x2,  x3,  [sp, #-16]!
    stp     x4,  x5,  [sp, #-16]!
    stp     x6,  x7,  [sp, #-16]!
    stp     x8,  x9,  [sp, #-16]!
    stp     x10, x11, [sp, #-16]!
    stp     x12, x13, [sp, #-16]!
    stp     x14, x15, [sp, #-16]!
    stp     x16, x17, [sp, #-16]!
    stp     x18, x19, [sp, #-16]!
    stp     x20, x21, [sp, #-16]!
    stp     x22, x23, [sp, #-16]!
    stp     x24, x25, [sp, #-16]!
    stp     x26, x27, [sp, #-16]!
    stp     x28, x29, [sp, #-16]!
    // Save x30 (LR) and ELR_EL1
    mrs     x0,  elr_el1
    stp     x30, x0,  [sp, #-16]!
    // Save SPSR_EL1
    mrs     x0,  spsr_el1
    stp     x0,  xzr, [sp, #-16]!
.endm

.macro trap_exit
    ldp     x0,  xzr, [sp], #16
    msr     spsr_el1, x0
    ldp     x30, x0,  [sp], #16
    msr     elr_el1, x0
    ldp     x28, x29, [sp], #16
    ldp     x26, x27, [sp], #16
    ldp     x24, x25, [sp], #16
    ldp     x22, x23, [sp], #16
    ldp     x20, x21, [sp], #16
    ldp     x18, x19, [sp], #16
    ldp     x16, x17, [sp], #16
    ldp     x14, x15, [sp], #16
    ldp     x12, x13, [sp], #16
    ldp     x10, x11, [sp], #16
    ldp     x8,  x9,  [sp], #16
    ldp     x6,  x7,  [sp], #16
    ldp     x4,  x5,  [sp], #16
    ldp     x2,  x3,  [sp], #16
    ldp     x0,  x1,  [sp], #16
    eret
.endm

trap_unhandled:
    b trap_unhandled
.section ".text.boot"
.global _start

_start:
    mov     x20, x0
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3
    cbz     x0, .Lboot

    // For Non-zero harts 
    // Drop exception level first using adr (safe, no literal pool)
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #3
    b.eq    .Lhart_el3_to_el2
    b       .Lhart_el2_spin

.Lhart_el3_to_el2:
    mov     x0, #0x5b1
    msr     scr_el3, x0
    mov     x0, #0x3c9
    msr     spsr_el3, x0
    adr     x0, .Lhart_el2_spin
    msr     elr_el3, x0
    eret

.Lhart_el2_spin:
    // Phase 1: spin on flag at 0x1000
    // No literal pool — movz only
    movz    x1, #0x1000

.Lphase1_spin:
    wfe
    ldr     x2, [x1]
    cbz     x2, .Lphase1_spin
    dsb     sy                      // ensure we see all stores before the flag

    // Jump to relocated copy of mailbox setup
    // Compute offset of .Lmailbox_setup from _start
    // then add to relocation base 0x8000000
    adr     x3, .Lmailbox_setup     // PC-relative from runtime base (0x80000)
    adr     x4, _start              // runtime base
    sub     x3, x3, x4             // isolate offset from _start
    movz    x0, #0x0800, lsl #16   // relocation base 0x8000000
    add     x0, x0, x3             // relocated address of .Lmailbox_setup
    br      x0                     // jump into relocated copy

    ////// Everything below this point runs from 0x8000000

.Lmailbox_setup:
    // Recompute mailbox address — we're now in the relocated copy
    // mailbox[hart_id] = 0x1000 + hart_id * 8
    // hart 1 = 0x1008, hart 2 = 0x1010, hart 3 = 0x1018
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3
    movz    x1, #0x1000
    add     x1, x1, x0, lsl #3

.Lmailbox_spin:
    wfe
    ldr     x4, [x1]            // load entry point into x4 instead of x2
    cbz     x4, .Lmailbox_spin
    dsb     sy

    // Clear mailbox
    mov     x3, #0
    str     x3, [x1]
    isb

    // Zero all argument registers per ARM64 boot protocol
    mov     x0, #0
    mov     x1, #0
    mov     x2, #0
    mov     x3, #0
    br      x4                  // jump to payload

    /////// Core 0 boot path

.Lboot:
    // Relocate to 0x8000000
    adr     x0, _start              // actual runtime address (0x80000)
    movz    x1, #0x0800, lsl #16   // destination 0x8000000

    // Already at destination? Skip copy (handles re-entry edge case)
    cmp     x0, x1
    b.eq    .Lrelocated

    // Copy 64KB — more than enough for our bootloader
    mov     x2, #65536
.Lcopy:
    ldp     x3, x4, [x0], #16
    stp     x3, x4, [x1], #16
    subs    x2, x2, #16
    b.gt    .Lcopy

    dsb     sy          // ensure all stores complete
    ic      ialluis     // invalidate instruction cache inner shareable
    dsb     sy          // ensure invalidation completes
    isb                 // flush pipeline

    // Now safe to compute jump target
    adr     x3, .Lrelocated
    adr     x4, _start
    sub     x3, x3, x4
    movz    x0, #0x0800, lsl #16
    add     x0, x0, x3
    br      x0

.Lrelocated:
    // Signal other harts to jump to relocated mailbox spin
    // dsb ensures the copy is visible before we set the flag
    movz    x1, #0x1000
    mov     x2, #1
    str     x2, [x1]
    dsb     sy                      // ensure flag write visible before sev
    sev                             // wake harts from wfe

    // Drop exception levels
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #3
    b.eq    .Lel3_to_el2
    b       .Lel2_entry

.Lel3_to_el2:
    mov     x0, #0x5b1
    msr     scr_el3, x0
    mov     x0, #0x3c9
    msr     spsr_el3, x0
    adr     x0, .Lel2_entry
    msr     elr_el3, x0
    eret

.Lel2_entry:
    // Set up stack — must happen here, in EL2, so SP_EL2 is initialised.
    // Setting SP earlier would only write to the SP_EL3 stack.
    movz    x0, #0x0800, lsl #16   // 0x08000000 — stack grows down from relocation base
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

    mov     x0, x20
    bl      main

.Lpark:
    wfe
    b       .Lpark
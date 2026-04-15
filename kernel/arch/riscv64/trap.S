#  Copyright 2026 Talon Kettuso & Contributors
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

.section .text
.align 4
.globl _trap
_trap:
    # 1. Swap HCB pointer into a2, original a2 moves to sscratch
    csrrw a2, sscratch, a2

    # Save a0 and a1 into HCB temporarily (offsets 0 and 8)
    sd a0, 0(a2)
    sd a1, 8(a2)

    # Check if we came from User Mode (sstatus.SPP is bit 8 of sstatus)
    csrr a0, sstatus
    andi a0, a0, 0x100
    bnez a0, .kernel_trap

.user_trap:
    sd sp, 16(a2)  # Save user SP to WyvernHCB.prev_trap_stack
    ld sp, 24(a2)  # Load kernel stack from WyvernHCB.trap_stack
    j .save_context

.kernel_trap:
    sd sp, 16(a2)  # Save kernel SP for unified restoration

.save_context:
    addi sp, sp, -256

    # Save the Instruction Pointer (sepc) to offset 0
    csrr t0, sepc
    sd t0, 0(sp)

    # Save all general purpose registers except x0 (zero) and x2 (sp)
    sd x1, 8(sp)
    # x2 (sp) is handled via HCB (offset 16)
    sd x3, 24(sp)
    sd x4, 32(sp)
    sd x5, 40(sp)
    sd x6, 48(sp)
    sd x7, 56(sp)
    sd x8, 64(sp)
    sd x9, 72(sp)
    # x10 (a0), x11 (a1), x12 (a2) handled below
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)

    # 2. Extract original registers from HCB and sscratch into the TrapFrame
    ld t0, 0(a2)   # Original a0
    sd t0, 80(sp)
    ld t0, 8(a2)   # Original a1
    sd t0, 88(sp)
    ld t0, 16(a2)  # Original sp
    sd t0, 16(sp)
    
    csrr t0, sscratch # Original a2
    sd t0, 96(sp)

    # 3. Restore HCB pointer to sscratch for future traps
    csrw sscratch, a2

    # 2. Prepare arguments for the C handler
    # Move the pointer to the TrapFrame (currently in sp) to a0 (first argument)
    mv a0, sp

.extern arch_trap_handler
    jal arch_trap_handler

.restore_regs:
    # Restore the Instruction Pointer
    ld t0, 0(sp)
    csrw sepc, t0

    ld x1, 8(sp)
    ld x3, 24(sp)
    ld x4, 32(sp)
    ld x5, 40(sp)
    ld x6, 48(sp)
    ld x7, 56(sp)
    ld x8, 64(sp)
    ld x9, 72(sp)
    ld x10, 80(sp)
    ld x11, 88(sp)
    ld x12, 96(sp)
    ld x13, 104(sp)
    ld x14, 112(sp)
    ld x15, 120(sp)
    ld x16, 128(sp)
    ld x17, 136(sp)
    ld x18, 144(sp)
    ld x19, 152(sp)
    ld x20, 160(sp)
    ld x21, 168(sp)
    ld x22, 176(sp)
    ld x23, 184(sp)
    ld x24, 192(sp)
    ld x25, 200(sp)
    ld x26, 208(sp)
    ld x27, 216(sp)
    ld x28, 224(sp)
    ld x29, 232(sp)
    ld x30, 240(sp)
    ld x31, 248(sp)

    # Finalize by switching back to the original stack.
    # We must load x5 (t0) BEFORE we move the stack pointer back.
    ld x5, 40(sp)
    ld t0, 16(sp)
    mv sp, t0

    sret

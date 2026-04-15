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

# This is a Position-Independent Trampoline
# It both obtains the base address of Wyvern, as well as sets up
# the initial page directory that we'll jump into the higher half of memory.
# 
# Wyvern is REQUIRED to be 2 MB aligned. Since the Linux Boot Protocol also requires this, this isn't a problem.
.option push
.option norelax
.option pic
.section .text.boot
.extern _begin, _end
.globl _entry
_entry:
    # Get the absolute base address of Wyvern
    auipc a2, 0
    # Disable interrupts just in case
    csrw sie, zero
    # Coherence Check to ensure that we are 2 MiB aligned
    mv t0, a2
    li t1, 0x1fffff
    and t0, t0, t1
    beqz t0, .continue
    # Make an SBI call telling the user we don't support this memory layout and halt this hart
    li a7, 0x4442434E # EID
    li a6, 0 # FID
    li a0, 53 # length
    li t0, 0xffffffff
    lla a1, bad_alignment_error # base_low
    mv a2, a1 # base_high
    and a1, a1, t0
    srl a2, a2, 32
    ecall
.halt:
    wfi
    beqz zero, .halt
.continue:
    # Setup the initial page directory
    # This has both a lower-half identity map and the kernel mapped at 0xffffffff80000000
    # We use a Pv39 map to save on memory since we shouldn't need more than 512 GiB of virtual memory
    # Also because not every RISC-V processor supports Pv48 (4-level paging) or higher.

    # Identity map first 256 GiB of memory
    lla t0, initial_l2_table
    li t1, 256
    li t2, 0xcf
    li t3, (1 << 28)          # PTE increment for 1 GiB (PhysAddr >> 2)
.identity_map:
    sd t2, 0(t0)
    add t2, t2, t3
    addi t0, t0, 8
    addi t1, t1, -1
    bgtz t1, .identity_map
    # Zero these out just to be safe
    or t0, zero, zero
    or t1, zero, zero
    or t2, zero, zero
    or t3, zero, zero

    # Map the kernel into the higher half at 0xffffffff80000000
    lla s0, _begin
    lla s1, _end
    lla s2, initial_l1_table
    lla s3, initial_l0_table

    # Link L1 into L2 (Index 510 for 0xffffffff80000000)
    lla t0, initial_l2_table
    srli t1, s2, 12           # PPN of L1
    li   t2, 0x01             # Valid bit only (it's a pointer to next table)
    slli t1, t1, 10
    or   t1, t1, t2
    li s4, 4080               # Index 510 for 0xffffffff80000000 (VPN[2])
    add  s4, t0, s4
    sd   t1, 0(s4)

    li   t3, 0                # Initialize L1 index (moved up to prevent uninitialized use)
    sub  t0, s1, s0           # Total size
    srli t2, t0, 21           # t2 = Number of full 2MB Mega Pages
    li   t1, 0x1fffff
    and  s4, t0, t1           # s4 = Remaining tail size
    li   t1, 0x200000         # 2MB constant for stepping
    beqz t2, .map_tail        # Skip if kernel is < 2MB
.map_mega_loop:
    srli t4, s0, 12       # PPN of current physical 2MB chunk
    li   t5, 0xCF         # DA--XWRV (Mega Page)
    slli t4, t4, 10
    or   t4, t4, t5
    
    slli t6, t3, 3        # Index * 8
    add  t6, s2, t6
    sd   t4, 0(t6)        # Write to L1
    
    add  s0, s0, t1       # Move physical addr 2MB forward
    addi t3, t3, 1        # Next L1 index
    addi t2, t2, -1
    bnez t2, .map_mega_loop
.map_tail:
    beqz s4, .switch_satp
    srli t0, s3, 12       # PPN of L0 table
    li   t1, 0x01         # Valid bit (Pointer to L0)
    slli t0, t0, 10
    or   t0, t0, t1
    
    slli t2, t3, 3        # Current L1 index * 8
    add  t2, s2, t2
    sd   t0, 0(t2)        # L1 now points to L0
    li   t4, 0            # Reset index for L0 (t0 is used for PTE)
.fill_l0_loop:
    srli t1, s0, 12       # PPN of current 4KB physical page
    li   t2, 0xCF         # DA--XWRV (4KB Page)
    slli t1, t1, 10
    or   t1, t1, t2
    
    slli t5, t4, 3        # index * 8
    add  t3, s3, t5
    sd   t1, 0(t3)

    li   t2, 4096
    add  s0, s0, t2       # Move physical 4KB forward
    addi t4, t4, 1        # Next L0 entry
    blt  s0, s1, .fill_l0_loop # Continue until we pass _end

    # (FINALLY) Switch to our new page directory
.switch_satp:
    lla  t0, initial_l2_table
    srli t0, t0, 12
    li   t1, (8 << 60)        # Sv39 mode
    or   t0, t0, t1
    csrw satp, t0
    sfence.vma

    # Set the stack pointer
    lla t0, stack_ptr
    ld  sp, 0(t0)

    # Initialize Global Pointer (gp) for linker relaxation.
    lla t0, gp_ptr
    ld  gp, 0(t0)

    # Jump to higher-half
    lla t0, jump_addr
    ld t0, 0(t0)
    jr t0
.option pop

bad_alignment_error: .string "wyvern: Kernel is not properly aligned, cannot boot\r\n" # length 53
.align 3
.extern arch_main
jump_addr:
    .quad arch_main
.option push
.option norelax
gp_ptr:
    .quad __global_pointer$
.option pop
stack_ptr:
    .quad boot_stack_top

.section .data.boot
.align 12                     # Page tables MUST be 4KB aligned
initial_l0_table:
    .zero (512 * 8)
initial_l1_table:
    .zero (512 * 8)
initial_l2_table:
    .zero (512 * 8)
boot_stack:
    .zero 16384
.globl boot_stack_top
boot_stack_top:
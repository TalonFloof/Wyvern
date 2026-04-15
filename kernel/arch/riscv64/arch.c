/*
   Copyright 2026 Talon Kettuso & Contributors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <stdint.h>
#include "../arch.h"
#include "sbi.h"
#include "../../main.h"

WyvernHCB boot_hart;
extern void* boot_stack_top;

extern void* _trap;

void arch_main(int hartid, void* fdt, uintptr_t kernel_base) {
    // Initialize sscratch with the current stack pointer (boot stack)
    // In a multi-threaded system, this would be the per-CPU kernel stack
    boot_hart.hart_id = hartid;
    boot_hart.trap_stack = (void*)&boot_stack_top;
    asm volatile("csrw sscratch, %0" : : "r"(&boot_hart));

    // Set the trap address
    asm volatile("csrw stvec, %0" : : "r"(&_trap));

    early_main(fdt, kernel_base);
    panic("riscv64 initialization is incomplete");
}

static WyvernArchInfo arch_info = {
    .name = "riscv64",
    .is64bit = true,
    .page_levels = 3,
    .page_shifts = {30,21,12,0}
};

inline WyvernArchInfo* arch_get_info() {
    return &arch_info;
}
bool arch_mask_ints(bool enabled) {
    uintptr_t sstatus;
    uint16_t oldstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    oldstatus = sstatus;
    if(enabled) {
        sstatus |= 0x2;
    } else {
        sstatus &= ~0x2;
    }
    asm volatile("csrw sstatus, %0" : : "r"(sstatus));
    return oldstatus & 2;
}
void arch_int_wait() {
    asm volatile("wfi");
}
inline void arch_mmu_switch(void* page_dir) {
    
}
void arch_debug_putc(char c) {
    if(c == '\n')
        sbi_call(0x4442434E, 2, '\r',0,0,0);
    sbi_call(0x4442434E, 2, c,0,0,0);
}
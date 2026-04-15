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
#pragma once
#include <stdint.h>

// Kernel-defined Page Types:
// "FreePage", "ZeroPage", "ActvPage", "RsvdPage", "WyvnKrnl", "WyvnData", "ObjChain"
// "PageDir ", "PageTabl", "ThreadCB", "PMemAObj"
// All types are 8 characters large and are condensed into a 64-bit number.

typedef struct WyvernPFNEntry {
    struct WyvernPFNEntry* prev;
    struct WyvernPFNEntry* next;
    union {
        uint64_t type_num;
        char type[8];
    };
    uintptr_t references;
    uintptr_t pte;
} WyvernPFNEntry;

void pfn_init_from_fdt(void* fdt, uintptr_t kernel_base);
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

typedef struct SBIReturn {
    long error;
    long value;
} SBIReturn;

static inline SBIReturn sbi_call(uintptr_t ext, uintptr_t fid, 
                                     uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    SBIReturn ret;
    register uintptr_t a0 __asm__("a0") = arg0;
    register uintptr_t a1 __asm__("a1") = arg1;
    register uintptr_t a2 __asm__("a2") = arg2;
    register uintptr_t a3 __asm__("a3") = arg3;
    register uintptr_t a6 __asm__("a6") = fid;
    register uintptr_t a7 __asm__("a7") = ext;

    __asm__ volatile ("ecall"
                      : "+r" (a0), "+r" (a1)
                      : "r" (a2), "r" (a3), "r" (a6), "r" (a7)
                      : "memory");
    ret.error = a0;
    ret.value = a1;
    return ret;
}
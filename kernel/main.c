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
#include "pfn.h"
#include "printf.h"
#include "arch/arch.h"
#include <stdarg.h>

void early_main(void* fdt, uintptr_t kernel_base) {
    printf("Copyright (c) 2026\n        Talon Kettuso & Contributors. Licensed under the Apache License 2.0\n        (http://www.apache.org/licenses/LICENSE-2.0)\n");
    if(fdt) {
        pfn_init_from_fdt(fdt,kernel_base);
    }
}

void panic(const char * msg, ...) {
    printf("panic: ");
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
    printf("\n");
    arch_mask_ints(false);
    for(;;)
        arch_int_wait();
}
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
// Hart Control Block

typedef struct WyvernHCB {
    // Used for storing some registers temporarily
    uintptr_t temp_reg1;
    uintptr_t temp_reg2;
    void* prev_trap_stack;
    void* trap_stack;
    int hart_id;
    
} WyvernHCB;
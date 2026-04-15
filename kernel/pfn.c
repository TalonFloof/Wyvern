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
#include "arch/fdt.h"
#include "endian.h"
#include "string.h"
#include "printf.h"

extern void* _begin;
extern void* _end;
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))

static uintptr_t pfn_addr_base = 0;
static WyvernPFNEntry* pfn_database = NULL;
static uint64_t pfn_entry_count = 0;

static void pfn_set_type(uint64_t pfn, uint64_t type) {
    if (pfn >= pfn_entry_count) return;
    pfn_database[pfn].type_num = htole64(type);
}

static void pfn_mark_range(uint64_t start, uint64_t size, const char type[8]) {
    uint64_t type_num = *((uint64_t*)type);
    if (start < pfn_addr_base) {
        if (start + size <= pfn_addr_base) return;
        size -= (pfn_addr_base - start);
        start = pfn_addr_base;
    }
    uint64_t start_pfn = (start - pfn_addr_base) / 4096;
    uint64_t end_pfn = (start + size - pfn_addr_base + 4095) / 4096;
    for (uint64_t i = start_pfn; i < end_pfn && i < pfn_entry_count; i++) {
        pfn_set_type(i, type_num);
    }
}

static int overlaps(uint64_t b1, uint64_t s1, uint64_t b2, uint64_t s2) {
    return b1 < (b2 + s2) && b2 < (b1 + s1);
}

void pfn_init_from_fdt(void* fdt, uintptr_t kernel_base) {
    uintptr_t kernel_size = ALIGN_UP(((uintptr_t)&_end)-((uintptr_t)&_begin),4096);
    uint32_t fdt_size = be32toh(((struct fdt_header*)fdt)->totalsize);
    uintptr_t rsvd_size = 0;

    pfn_addr_base = ~0;
    // Pass 1: Find Maximum and Minimum Physical Address
    uint64_t max_phys_addr = 0;
    struct fdt_header* hdr = (struct fdt_header*)fdt;
    if (be32toh(hdr->magic) != FDT_MAGIC) return;

    uint32_t* ptr = (void*)((uintptr_t)fdt + be32toh(hdr->off_dt_struct));
    const char* strings = (void*)((uintptr_t)fdt + be32toh(hdr->off_dt_strings));
    uint32_t addr_cells = 2, sz_cells = 2;
    int is_mem = 0, depth = 0;

    while (be32toh(*ptr) != FDT_END) {
        uint32_t token = be32toh(*ptr++);
        if (token == FDT_BEGIN_NODE) {
            depth++;
            is_mem = (depth == 2 && memcmp((char*)ptr, "memory", 6) == 0);
            ptr += (strlen((char*)ptr) + 1 + 3) / 4;
        } else if (token == FDT_END_NODE) {
            depth--; is_mem = 0;
        } else if (token == FDT_PROP) {
            uint32_t len = be32toh(*ptr++);
            const char* propname = strings + be32toh(*ptr++);
            if (depth == 1 && strcmp(propname, "#address-cells") == 0) addr_cells = be32toh(*ptr);
            if (depth == 1 && strcmp(propname, "#size-cells") == 0) sz_cells = be32toh(*ptr);
            if (is_mem && strcmp(propname, "reg") == 0) {
                uint32_t* reg_ptr = ptr;
                for (uint32_t i = 0; i < len / ((addr_cells + sz_cells) * 4); i++) {
                    uint64_t b = 0, s = 0;
                    for (uint32_t j = 0; j < addr_cells; j++) b = (b << 32) | be32toh(reg_ptr[j]);
                    for (uint32_t j = 0; j < sz_cells; j++) s = (s << 32) | be32toh(reg_ptr[addr_cells + j]);
                    if (b + s > max_phys_addr) max_phys_addr = b + s;
                    if (b < pfn_addr_base) pfn_addr_base = b;
                    reg_ptr += (addr_cells + sz_cells);
                }
            }
            ptr += (len + 3) / 4;
        }
    }

    pfn_entry_count = (max_phys_addr-pfn_addr_base) / 4096;
    uintptr_t pfn_db_size = ALIGN_UP(pfn_entry_count * sizeof(WyvernPFNEntry), 4096);

    // Pass 2: Find a safe place for the PFN database
    // For simplicity, we search for a gap after the kernel or FDT.
    uintptr_t pfn_phys = ALIGN_UP(kernel_base + kernel_size, 4096);
    if (overlaps(pfn_phys, pfn_db_size, (uintptr_t)fdt, fdt_size)) {
        pfn_phys = ALIGN_UP((uintptr_t)fdt + fdt_size, 4096);
    }
    pfn_database = (WyvernPFNEntry*)pfn_phys;

    // Initialize everything as Reserved first
    memset(pfn_database, 0, pfn_db_size);
    for (uint64_t i = 0; i < pfn_entry_count; i++)
        pfn_database[i].type_num = htole64(0x6567615064767352); // RsvdPage

    // Pass 3: Mark all physically present RAM as Free
    ptr = (void*)((uintptr_t)fdt + be32toh(hdr->off_dt_struct));
    depth = 0; int is_memory_node = 0;
    addr_cells = 2; sz_cells = 2; // Reset defaults

    while (be32toh(*ptr) != FDT_END) {
        uint32_t token = be32toh(*ptr++);
        if (token == FDT_BEGIN_NODE) {
            depth++;
            const char* name = (const char*)ptr;
            if (depth == 2) {
                // Top-level children of the root node
                is_memory_node = (memcmp(name, "memory", 6) == 0);
            }
            ptr += (strlen(name) + 1 + 3) / 4;
        } else if (token == FDT_END_NODE) {
            if (depth == 2) is_memory_node = 0;
            depth--;
		} else if (token == FDT_PROP) {
            uint32_t len = be32toh(*ptr++);
            uint32_t nameoff = be32toh(*ptr++);
            const char* propname = strings + nameoff;

            // Only update cell sizes if we're at the root node (depth 1)
            if (strcmp(propname, "#address-cells") == 0 && depth == 1) {
                addr_cells = be32toh(*ptr);
            } else if (strcmp(propname, "#size-cells") == 0 && depth == 1) {
                sz_cells = be32toh(*ptr);
            } else if (strcmp(propname, "device_type") == 0 && strcmp((char*)ptr, "memory") == 0) {
                is_memory_node = 1;
            } else if (is_memory_node && strcmp(propname, "reg") == 0) {
                uint32_t* reg_ptr = ptr;
                uint32_t entry_cells = addr_cells + sz_cells;
                for (uint32_t i = 0; i < len / (entry_cells * 4); i++) {
                    uint64_t base = 0, size = 0;
                    for (uint32_t j = 0; j < addr_cells; j++) base = (base << 32) | be32toh(reg_ptr[j]);
                    for (uint32_t j = 0; j < sz_cells; j++) size = (size << 32) | be32toh(reg_ptr[addr_cells + j]);
                    
                    pfn_mark_range(base, size, "FreePage");
                    reg_ptr += entry_cells;
                }
            }
            ptr += (len + 3) / 4;
        }
    }

    // Pass 4: Carve out reservations (Reserved Memory Nodes)
    ptr = (void*)((uintptr_t)fdt + be32toh(hdr->off_dt_struct));
    depth = 0; int is_reserved_parent = 0; int is_reserved_node = 0;
    addr_cells = 2; sz_cells = 2;

    while (be32toh(*ptr) != FDT_END) {
        uint32_t token = be32toh(*ptr++);
        if (token == FDT_BEGIN_NODE) {
            depth++;
            if (depth == 2) is_reserved_parent = (strcmp((char*)ptr, "reserved-memory") == 0);
            else if (depth == 3 && is_reserved_parent) is_reserved_node = 1;
            ptr += (strlen((char*)ptr) + 1 + 3) / 4;
        } else if (token == FDT_END_NODE) {
            if (depth == 2) is_reserved_parent = 0;
            else if (depth == 3) is_reserved_node = 0;
            depth--;
        } else if (token == FDT_PROP) {
            uint32_t len = be32toh(*ptr++);
            const char* propname = strings + be32toh(*ptr++);
            if (depth == 1 && strcmp(propname, "#address-cells") == 0) addr_cells = be32toh(*ptr);
            if (depth == 1 && strcmp(propname, "#size-cells") == 0) sz_cells = be32toh(*ptr);
            if (is_reserved_node && strcmp(propname, "reg") == 0) {
                uint32_t* reg_ptr = ptr;
                for (uint32_t i = 0; i < len / ((addr_cells + sz_cells) * 4); i++) {
                    uint64_t base = 0, size = 0;
                    for (uint32_t j = 0; j < addr_cells; j++) base = (base << 32) | be32toh(reg_ptr[j]);
                    for (uint32_t j = 0; j < sz_cells; j++) size = (size << 32) | be32toh(reg_ptr[addr_cells + j]);
                    rsvd_size += size;
                    pfn_mark_range(base, size, "RsvdPage");
                    reg_ptr += (addr_cells + sz_cells);
                }
            }
            ptr += (len + 3) / 4;
        }
    }

    // Search through the extra reserve entry table
    struct fdt_reserve_entry* rsv = (void*)((uintptr_t)fdt + be32toh(hdr->off_mem_rsvmap));
    while (be64toh(rsv->address) != 0 || be64toh(rsv->size) != 0) {
        pfn_mark_range(be64toh(rsv->address), be64toh(rsv->size), "RsvdPage");
        rsv++;
    }
    
    pfn_mark_range(kernel_base, kernel_size, "WyvnKrnl");
    pfn_mark_range((uintptr_t)fdt, fdt_size, "RsvdPage");
    pfn_mark_range(pfn_phys, pfn_db_size, "RsvdPage");

    printf("pfn: Initialized %llu entries @ 0x%llx (%llu \bk reserved, %llu \bk kernel, %llu \bk fdt, %llu \bk pfn)\n", (unsigned long long)pfn_entry_count, (unsigned long long)pfn_phys, (unsigned long long)rsvd_size/1024, (unsigned long long)kernel_size/1024, (unsigned long long)ALIGN_UP(fdt_size,4096)/1024, (unsigned long long)ALIGN_UP(pfn_db_size,4096)/1024);
}
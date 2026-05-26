#pragma once
#include <stdint.h>
#include <stddef.h>

typedef uint64_t VMCell;

#define CELL_INT 0
#define CELL_REF 1

typedef enum VMHeapType {
    HEAP_ARRAY_INT = 0,
    HEAP_ARRAY_REF = 1,
    HEAP_ARRAY_BYTE = 2,
    HEAP_STRUCTURE = 3,
} VMHeapType;

#define MAX_STACK 4096

typedef struct VMStack {
    VMCell cells[MAX_STACK];
    uint8_t kinds[MAX_STACK / 8];
    int top;
} VMStack;

